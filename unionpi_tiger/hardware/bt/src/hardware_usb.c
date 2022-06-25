/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg_usb"
#define RTKBT_RELEASE_NAME "20190717_BT_ANDROID_9.0"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include <unistd.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"

#include "bt_vendor_lib.h"
#include "hardware.h"
#include "rtk_common.h"

/******************************************************************************
**  Constants &  Macros
******************************************************************************/
void hw_usb_config_cback(HC_BT_HDR *p_evt_buf);

#define EXTRA_CONFIG_FILE "/vendor/etc/bluetooth/rtk_btconfig.txt"
static struct rtk_bt_vendor_config_entry *extra_extry;
static struct rtk_bt_vendor_config_entry *extra_entry_inx = NULL;

#define BT_VENDOR_CFG_TIMEDELAY_ 40

typedef void (*tTIMER_HANDLE_CBACK)(union sigval sigval_value);

static timer_t OsAllocateTimer(tTIMER_HANDLE_CBACK timer_callback)
{
    struct sigevent sigev;
    timer_t timerid;

    (void)memset_s(&sigev, sizeof(struct sigevent), 0, sizeof(struct sigevent));
    // Create the POSIX timer to generate signo
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_notify_function = timer_callback;
    sigev.sigev_value.sival_ptr = &timerid;

    // Create the Timer using timer_create signal

    if (timer_create(CLOCK_REALTIME, &sigev, &timerid) == 0) {
        return timerid;
    } else {
        HILOGE("timer_create error!");
        return (timer_t)-1;
    }
}

static int OsFreeTimer(timer_t timerid)
{
    int ret = 0;
    ret = timer_delete(timerid);
    if (ret != 0) {
        HILOGE("timer_delete fail with errno(%d)", errno);
    }

    return ret;
}

static int OsStartTimer(timer_t timerid, int msec, int mode)
{
    struct itimerspec itval;

#define MSEC_1000 1000
    itval.it_value.tv_sec = msec / MSEC_1000;
    itval.it_value.tv_nsec = (long)(msec % MSEC_1000) * (1000000L);

    if (mode == 1) {
        itval.it_interval.tv_sec = itval.it_value.tv_sec;
        itval.it_interval.tv_nsec = itval.it_value.tv_nsec;
    } else {
        itval.it_interval.tv_sec = 0;
        itval.it_interval.tv_nsec = 0;
    }

    // Set the Timer when to expire through timer_settime

    if (timer_settime(timerid, 0, &itval, NULL) != 0) {
        HILOGE("time_settime error!");
        return -1;
    }

    return 0;
}

static timer_t localtimer = 0;
static void local_timer_handler(union sigval sigev_value)
{
    bt_vendor_cbacks->init_cb(BTC_OP_RESULT_SUCCESS);
    OsFreeTimer(localtimer);
}

static void start_fwcfg_cbtimer(void)
{
    if (localtimer == 0) {
        localtimer = OsAllocateTimer(local_timer_handler);
    }
    OsStartTimer(localtimer, BT_VENDOR_CFG_TIMEDELAY_, 0);
}

/******************************************************************************
**  Static variables
******************************************************************************/

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint16_t lmp_sub_default;
    uint16_t lmp_sub;
    uint16_t eversion;
    char *mp_patch_name;
    char *patch_name;
    char *config_name;
    uint8_t *fw_cache;
    int fw_len;
    uint16_t mac_offset;
    uint32_t max_patch_size;
} usb_patch_info;

static usb_patch_info usb_fw_patch_table[] = {
    /* { vid, pid, lmp_sub_default, lmp_sub, everion, mp_fw_name,
    fw_name, config_name, fw_cache, fw_len, mac_offset } */
    {0x0BDA, 0xD723, 0x8723, 0, 0, "mp_rtl8723d_fw", "rtl8723d_fw", "rtl8723d_config", NULL, 0,
     CONFIG_MAC_OFFSET_GEN_3PLUS, MAX_PATCH_SIZE_40K}, /* RTL8723DU */
    /* todo: RTL8703CU */

    /* NOTE: must append patch entries above the null entry */
    {0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, 0, 0, 0}};

uint16_t usb_project_id[] = {ROM_LMP_8723a, ROM_LMP_8723b, ROM_LMP_8821a, ROM_LMP_8761a, ROM_LMP_8703a, ROM_LMP_8763a,
                             ROM_LMP_8703b, ROM_LMP_8723c, ROM_LMP_8822b, ROM_LMP_8723d, ROM_LMP_8821c, ROM_LMP_NONE,
                             ROM_LMP_NONE,  ROM_LMP_8822c, ROM_LMP_8761b, ROM_LMP_NONE};
// signature: realtech
static const uint8_t RTK_EPATCH_SIGNATURE[8] = {0x52, 0x65, 0x61, 0x6C, 0x74, 0x65, 0x63, 0x68};
// Extension Section IGNATURE:0x77FD0451
static const uint8_t EXTENSION_SECTION_SIGNATURE[4] = {0x51, 0x04, 0xFD, 0x77};

static void usb_line_process(char *buf, unsigned short *offset, int *t)
{
    char *head = buf;
    char *ptr = buf;
    char *argv[32];
    int argc = 0;
    unsigned char len = 0;
    int i = 0;
    static int alt_size = 0;

    if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '[') {
        return;
    }
    if (alt_size > MAX_ALT_CONFIG_SIZE - 4L) {
        HILOGW("Extra Config file is too large");
        return;
    }
    if (extra_entry_inx == NULL) {
        extra_entry_inx = extra_extry;
    }
    HILOGI("line_process:%s", buf);
    while ((ptr = strsep(&head, ", \t")) != NULL) {
        if (!ptr[0]) {
            continue;
        }
        argv[argc++] = ptr;
        if (argc >= 32L) {
            HILOGW("Config item is too long");
            break;
        }
    }
    if (argc < 4L) {
        HILOGE("Invalid Config item, ignore");
        return;
    }
    offset[(*t)] = (unsigned short)((strtoul(argv[0], NULL, 16L)) |
                                    (strtoul(argv[1], NULL, 16L) << 8L));
    HILOGI("Extra Config offset %04x", offset[(*t)]);
    extra_entry_inx->offset = offset[(*t)];
    (*t)++;
    len = (unsigned char)strtoul(argv[2L], NULL, 16L);
    if (len != (unsigned char)(argc - 3L)) {
        HILOGE("Extra Config item len %d is not match, we assume the actual len is %ld", len, (argc - 3L));
        len = argc - 3L;
    }
    extra_entry_inx->entry_len = len;

    alt_size += len + sizeof(struct rtk_bt_vendor_config_entry);
    if (alt_size > MAX_ALT_CONFIG_SIZE) {
        HILOGW("Extra Config file is too large");
        extra_entry_inx->offset = 0;
        extra_entry_inx->entry_len = 0;
        alt_size -= (len + sizeof(struct rtk_bt_vendor_config_entry));
        return;
    }
    for (i = 0; i < len; i++) {
        extra_entry_inx->entry_data[i] = (uint8_t)strtoul(argv[3L + i], NULL, 16L);
        HILOGI("data[%d]:%02x", i, extra_entry_inx->entry_data[i]);
    }
    extra_entry_inx = (struct rtk_bt_vendor_config_entry *)
        ((uint8_t *)extra_entry_inx + len + sizeof(struct rtk_bt_vendor_config_entry));
}

static void usb_parse_extra_config(const char *path, usb_patch_info *patch_entry, unsigned short *offset, int *t)
{
    int fd, ret;
    unsigned char buf[1024];

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        HILOGI("Couldn't open extra config %s, err:%s", path, strerror(errno));
        return;
    }

    ret = read(fd, buf, sizeof(buf));
    if (ret == -1) {
        HILOGE("Couldn't read %s, err:%s", path, strerror(errno));
        close(fd);
        return;
    } else if (ret == 0) {
        HILOGE("%s is empty", path);
        close(fd);
        return;
    }

#define USB_PARSE_EXTRA_CONFIG_RET_1022 1022
    if (ret > USB_PARSE_EXTRA_CONFIG_RET_1022) {
        HILOGE("Extra config file is too big");
        close(fd);
        return;
    }
    buf[ret++] = '\n';
    buf[ret++] = '\0';
    close(fd);
    char *head = (void *)buf;
    char *ptr = (void *)buf;
    ptr = strsep(&head, "\n\r");
    if (strncmp(ptr, patch_entry->config_name, strlen(ptr))) {
        HILOGW("Extra config file not set for %s, ignore", patch_entry->config_name);
        return;
    }
    while ((ptr = strsep(&head, "\n\r")) != NULL) {
        if (!ptr[0]) {
            continue;
        }
        usb_line_process(ptr, offset, t);
    }
}

// (patch_info *patch_entry, unsigned short *offset, int max_group_cnt)
static inline int getUsbAltSettings(usb_patch_info *patch_entry, unsigned short *offset)
{
    int n = 0;
    if (patch_entry) {
        offset[n++] = patch_entry->mac_offset;
    } else {
        return n;
    }
    /*
    //sample code, add special settings

        offset[n++] = 0x15B;
    */
    if (extra_extry) {
        usb_parse_extra_config(EXTRA_CONFIG_FILE, patch_entry, offset, &n);
    }

    return n;
}

#define USB_VAL_5 5
#define USB_VAL_4 4
#define USB_VAL_3 3
#define USB_VAL_2 2
#define USB_VAL_1 1
#define USB_VAL_0 0

static inline int getUsbAltSettingVal(usb_patch_info *patch_entry, unsigned short offset, unsigned char *val)
{
    int res = 0;

    int i = 0;
    struct rtk_bt_vendor_config_entry *ptr = extra_extry;

    while (ptr->offset) {
        if (ptr->offset == offset) {
            if (offset != patch_entry->mac_offset) {
                (void)memcpy_s(val, ptr->entry_len, ptr->entry_data, ptr->entry_len);
                res = ptr->entry_len;
                HILOGI("Get Extra offset:%04x, val:", offset);
                for (i = 0; i < ptr->entry_len; i++) {
                    HILOGI("%02x", ptr->entry_data[i]);
                }
            }
            break;
        }
        ptr = (struct rtk_bt_vendor_config_entry *)((uint8_t *)ptr + ptr->entry_len +
                                                    sizeof(struct rtk_bt_vendor_config_entry));
    }

    /*    switch(offset)
        {
    //sample code, add special settings
            case 0x15B:
                val[0] = 0x0B;
                val[1] = 0x0B;
                val[2] = 0x0B;
                val[3] = 0x0B;
                res = 4;
                break;

            default:
                res = 0;
                break;
        }
    */
    if ((patch_entry) && (offset == patch_entry->mac_offset) && (res == 0)) {
        if (getmacaddr(val) == 0) {
            HILOGI("MAC: %02x:%02x:%02x:%02x:%02x:%02x", val[USB_VAL_5], val[USB_VAL_4], val[USB_VAL_3], val[USB_VAL_2],
                   val[USB_VAL_1], val[USB_VAL_0]);
            res = 6L;
        }
    }
    return res;
}

#define USB_CONFIG_BUF_PTR_15 15
#define USB_CONFIG_BUF_PTR_14 14
#define USB_CONFIG_BUF_PTR_13 13
#define USB_CONFIG_BUF_PTR_12 12
#define USB_CONFIG_BUF_PTR_11 11
#define USB_CONFIG_BUF_PTR_10 10
#define USB_CONFIG_BUF_PTR_9 9
#define USB_CONFIG_BUF_PTR_8 8
#define USB_CONFIG_BUF_PTR_7 7
#define USB_CONFIG_BUF_PTR_6 6
#define USB_CONFIG_BUF_PTR_5 5
#define USB_CONFIG_BUF_PTR_4 4
#define USB_CONFIG_BUF_PTR_3 3
#define USB_CONFIG_BUF_PTR_2 2
#define USB_CONFIG_BUF_PTR_1 1

static void rtk_usb_update_altsettings(usb_patch_info *patch_entry, unsigned char *config_buf_ptr,
                                       size_t *config_len_ptr)
{
    unsigned short offset[256], data_len;
    unsigned char val[256];

    struct rtk_bt_vendor_config *config = (struct rtk_bt_vendor_config *)config_buf_ptr;
    struct rtk_bt_vendor_config_entry *entry = config->entry;
    size_t config_len = *config_len_ptr;
    unsigned int i = 0;
    int count = 0, temp = 0, j;

    if ((extra_extry = (struct rtk_bt_vendor_config_entry *)malloc(MAX_ALT_CONFIG_SIZE)) == NULL) {
        HILOGE("malloc buffer for extra_extry failed");
    } else {
        (void)memset_s(extra_extry, MAX_ALT_CONFIG_SIZE, 0, MAX_ALT_CONFIG_SIZE);
    }

    HILOGI("ORG Config len=%08zx:\n", config_len);
    for (i = 0; i <= config_len; i += 0x10) {
        HILOGI("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", i,
               config_buf_ptr[i], config_buf_ptr[i + USB_CONFIG_BUF_PTR_1], config_buf_ptr[i + USB_CONFIG_BUF_PTR_2],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_3], config_buf_ptr[i + USB_CONFIG_BUF_PTR_4],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_5], config_buf_ptr[i + USB_CONFIG_BUF_PTR_6],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_7], config_buf_ptr[i + USB_CONFIG_BUF_PTR_8],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_9], config_buf_ptr[i + USB_CONFIG_BUF_PTR_10],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_11], config_buf_ptr[i + USB_CONFIG_BUF_PTR_12],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_13], config_buf_ptr[i + USB_CONFIG_BUF_PTR_14],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_15]);
    }

    (void)memset_s(offset,  sizeof(offset), 0, sizeof(offset));
    (void)memset_s(val, sizeof(val), 0, sizeof(val));
    data_len = le16_to_cpu(config->data_len);

    count = getUsbAltSettings(patch_entry, offset);
    if (count <= 0) {
        HILOGI("rtk_update_altsettings: No AltSettings");
        return;
    } else {
        HILOGI("rtk_update_altsettings: %d AltSettings", count);
    }

    if (data_len != config_len - sizeof(struct rtk_bt_vendor_config)) {
        HILOGE("rtk_update_altsettings: config len(%x) is not right(%lx)", data_len,
               (unsigned long)(config_len - sizeof(struct rtk_bt_vendor_config)));
        return;
    }

    for (i = 0; i < data_len;) {
        for (j = 0; j < count; j++) {
            if (le16_to_cpu(entry->offset) == offset[j]) {
                if (offset[j] == patch_entry->mac_offset) {
                    offset[j] = 0;
                } else {
                    struct rtk_bt_vendor_config_entry *t = extra_extry;
                    while (t->offset) {
                        if (t->offset == le16_to_cpu(entry->offset)) {
                            if (t->entry_len == entry->entry_len) {
                                offset[j] = 0;
                            }
                            break;
                        }
                        t = (struct rtk_bt_vendor_config_entry *)((uint8_t *)t + t->entry_len +
                                                                  sizeof(struct rtk_bt_vendor_config_entry));
                    }
                }
            }
        }
        if (getUsbAltSettingVal(patch_entry, le16_to_cpu(entry->offset), val) == entry->entry_len) {
            HILOGI("rtk_update_altsettings: replace %04x[%02x]", le16_to_cpu(entry->offset), entry->entry_len);
            (void)memcpy_s(entry->entry_data, entry->entry_len, val, entry->entry_len);
        }
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry *)((uint8_t *)entry + temp);
    }

    for (j = 0; j < count; j++) {
        if (offset[j] == 0) {
            continue;
        }
        entry->entry_len = getUsbAltSettingVal(patch_entry, offset[j], val);
        if (entry->entry_len <= 0) {
            continue;
        }
        entry->offset = cpu_to_le16(offset[j]);
        (void)memcpy_s(entry->entry_data, entry->entry_len, val, entry->entry_len);
        HILOGI("rtk_update_altsettings: add %04x[%02x]", le16_to_cpu(entry->offset), entry->entry_len);
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry *)((uint8_t *)entry + temp);
    }
    config->data_len = cpu_to_le16(i);
    *config_len_ptr = i + sizeof(struct rtk_bt_vendor_config);

    if (extra_extry) {
        free(extra_extry);
        extra_extry = NULL;
        extra_entry_inx = NULL;
    }

    HILOGI("NEW Config len=%08zx:\n", *config_len_ptr);
    for (i = 0; i <= (*config_len_ptr); i += 0x10) {
        HILOGI("%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", i,
               config_buf_ptr[i], config_buf_ptr[i + USB_CONFIG_BUF_PTR_1], config_buf_ptr[i + USB_CONFIG_BUF_PTR_2],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_3], config_buf_ptr[i + USB_CONFIG_BUF_PTR_4],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_5], config_buf_ptr[i + USB_CONFIG_BUF_PTR_6],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_7], config_buf_ptr[i + USB_CONFIG_BUF_PTR_8],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_9], config_buf_ptr[i + USB_CONFIG_BUF_PTR_10],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_11], config_buf_ptr[i + USB_CONFIG_BUF_PTR_12],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_13], config_buf_ptr[i + USB_CONFIG_BUF_PTR_14],
               config_buf_ptr[i + USB_CONFIG_BUF_PTR_15]);
    }
    return;
}

static void rtk_usb_parse_config_file(unsigned char **config_buf, size_t *filelen, uint8_t bt_addr[6],
                                      uint16_t mac_offset)
{
    struct rtk_bt_vendor_config *config = (struct rtk_bt_vendor_config *)*config_buf;
    uint16_t config_len = le16_to_cpu(config->data_len), temp = 0;
    struct rtk_bt_vendor_config_entry *entry = config->entry;
    unsigned int i = 0;
    uint8_t heartbeat_buf = 0;
    uint8_t *p;

    HILOGD("bt_addr = %x", bt_addr[0]);
    if (le32_to_cpu(config->signature) != RTK_VENDOR_CONFIG_MAGIC) {
        HILOGE("config signature magic number(0x%x) is not set to RTK_VENDOR_CONFIG_MAGIC", config->signature);
        return;
    }

    if (config_len != *filelen - sizeof(struct rtk_bt_vendor_config)) {
        HILOGE("config len(0x%x) is not right(0x%zx)", config_len, *filelen - sizeof(struct rtk_bt_vendor_config));
        return;
    }

    hw_cfg_cb.heartbeat = 0;
    for (i = 0; i < config_len;) {
        switch (le16_to_cpu(entry->offset)) {
            case 0x017a: {
                if (mac_offset == CONFIG_MAC_OFFSET_GEN_1_2) {
                    p = (uint8_t *)entry->entry_data;
                    STREAM_TO_UINT8(heartbeat_buf, p);
                    if ((heartbeat_buf & 0x02) && (heartbeat_buf & 0x10)) {
                        hw_cfg_cb.heartbeat = 1;
                    } else {
                        hw_cfg_cb.heartbeat = 0;
                    }

                    HILOGI("config 0x017a heartbeat = %d", hw_cfg_cb.heartbeat);
                }
                break;
            }
            case 0x01be: {
                if (mac_offset == CONFIG_MAC_OFFSET_GEN_3PLUS || mac_offset == CONFIG_MAC_OFFSET_GEN_4PLUS) {
                    p = (uint8_t *)entry->entry_data;
                    STREAM_TO_UINT8(heartbeat_buf, p);
                    if ((heartbeat_buf & 0x02) && (heartbeat_buf & 0x10)) {
                        hw_cfg_cb.heartbeat = 1;
                    } else {
                        hw_cfg_cb.heartbeat = 0;
                    }

                    HILOGI("config 0x01be heartbeat = %d", hw_cfg_cb.heartbeat);
                }
                break;
            }
            default:
                HILOGI("config offset(0x%x),length(0x%x)", entry->offset, entry->entry_len);
                break;
        }
        temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
        i += temp;
        entry = (struct rtk_bt_vendor_config_entry *)((uint8_t *)entry + temp);
    }

    return;
}

static uint32_t rtk_usb_get_bt_config(unsigned char **config_buf, char *config_file_short_name, uint16_t mac_offset)
{
    char bt_config_file_name[PATH_MAX] = {0};
    struct stat st;
    size_t filelen;
    int fd;
    uint8_t *p_vnd_local_bd_addr = get_vnd_local_bd_addr();

    (void)sprintf_s(bt_config_file_name, sizeof(bt_config_file_name), BT_CONFIG_DIRECTORY, config_file_short_name);
    HILOGI("BT config file: %s", bt_config_file_name);

    if (stat(bt_config_file_name, &st) < 0) {
        HILOGE("can't access bt config file:%s, errno:%d\n", bt_config_file_name, errno);
        return 0;
    }

    filelen = st.st_size;
    if (filelen > MAX_ORG_CONFIG_SIZE) {
        HILOGE("bt config file is too large(>0x%04x)", MAX_ORG_CONFIG_SIZE);
        return 0;
    }

    if ((fd = open(bt_config_file_name, O_RDONLY)) < 0) {
        HILOGE("Can't open bt config file");
        return 0;
    }

    if ((*config_buf = malloc(MAX_ORG_CONFIG_SIZE + MAX_ALT_CONFIG_SIZE)) == NULL) {
        HILOGE("malloc buffer for config file fail(0x%zx)\n", filelen);
        close(fd);
        return 0;
    }

    if (read(fd, *config_buf, filelen) < (ssize_t)filelen) {
        HILOGE("Can't load bt config file");
        free(*config_buf);
        close(fd);
        return 0;
    }

    rtk_usb_parse_config_file(config_buf, &filelen, p_vnd_local_bd_addr, mac_offset);

    close(fd);
    return filelen;
}

static usb_patch_info *rtk_usb_get_fw_table_entry(uint16_t vid, uint16_t pid)
{
    usb_patch_info *patch_entry = usb_fw_patch_table;

    uint32_t entry_size = sizeof(usb_fw_patch_table) / sizeof(usb_fw_patch_table[0]);
    uint32_t i;

    for (i = 0; i < entry_size; i++, patch_entry++) {
        if ((vid == patch_entry->vid) && (pid == patch_entry->pid)) {
            break;
        }
    }

    if (i == entry_size) {
        HILOGE("%s: No fw table entry found", __func__);
        return NULL;
    }

    return patch_entry;
}

#define USB_MEMCPY_8 8
#define USB_MEMCPY_4 4
#define USB_MEMCPY_S_12 12
#define USB_MEMCPY_S_8 8
#define USB_MEMCPY_S_4 4
#define USB_LEN_5 5
#define USB_ENTRY_16 16
#define USB_ENTRY_27 27
#define USB_ENTRY_10000 10000

static void rtk_usb_get_bt_final_patch(bt_hw_cfg_cb_t *cfg_cb)
{
    uint8_t proj_id = 0;
    struct rtk_epatch_entry *entry = NULL;
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;

    if (cfg_cb->lmp_subversion == LMPSUBVERSION_8723a) {
        if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, USB_MEMCPY_8) == 0) {
            HILOGE("8723as check signature error!");
            cfg_cb->dl_fw_flag = 0;
        } else {
            cfg_cb->total_len = cfg_cb->fw_len + cfg_cb->config_len;
            if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len))) {
                HILOGE("can't alloc memory for fw&config, errno:%d", errno);
                cfg_cb->dl_fw_flag = 0;
                if (cfg_cb->fw_len > 0) {
                    free(cfg_cb->fw_buf);
                    cfg_cb->fw_len = 0;
                }

                if (cfg_cb->config_len > 0) {
                    free(cfg_cb->config_buf);
                    cfg_cb->config_len = 0;
                }

                if (entry) {
                    free(entry);
                }
                return;
            } else {
                HILOGI("8723as, fw copy direct");
                (void)memcpy_s(cfg_cb->total_buf, cfg_cb->fw_len, cfg_cb->fw_buf, cfg_cb->fw_len);
                (void)memcpy_s(cfg_cb->total_buf + cfg_cb->fw_len,
                               cfg_cb->config_len, cfg_cb->config_buf, cfg_cb->config_len);
                cfg_cb->dl_fw_flag = 1;
                if (cfg_cb->fw_len > 0) {
                    free(cfg_cb->fw_buf);
                    cfg_cb->fw_len = 0;
                }

                if (cfg_cb->config_len > 0) {
                    free(cfg_cb->config_buf);
                    cfg_cb->config_len = 0;
                }

                if (entry) {
                    free(entry);
                }
                return;
            }
        }
    }

    if (memcmp(cfg_cb->fw_buf, RTK_EPATCH_SIGNATURE, USB_MEMCPY_8)) {
        HILOGE("check signature error");
        cfg_cb->dl_fw_flag = 0;
        if (cfg_cb->fw_len > 0) {
            free(cfg_cb->fw_buf);
            cfg_cb->fw_len = 0;
        }

        if (cfg_cb->config_len > 0) {
            free(cfg_cb->config_buf);
            cfg_cb->config_len = 0;
        }

        if (entry) {
            free(entry);
        }
        return;
    }

    /* check the extension section signature */
    if (memcmp(cfg_cb->fw_buf + cfg_cb->fw_len - USB_MEMCPY_8, EXTENSION_SECTION_SIGNATURE, USB_MEMCPY_8)) {
        HILOGE("check extension section signature error");
        cfg_cb->dl_fw_flag = 0;
        if (cfg_cb->fw_len > 0) {
            free(cfg_cb->fw_buf);
            cfg_cb->fw_len = 0;
        }

        if (cfg_cb->config_len > 0) {
            free(cfg_cb->config_buf);
            cfg_cb->config_len = 0;
        }

        if (entry) {
            free(entry);
        }
        return;
    }

    proj_id = rtk_get_fw_project_id(cfg_cb->fw_buf + cfg_cb->fw_len - USB_LEN_5);
    if (usb_project_id[proj_id] != hw_cfg_cb.lmp_subversion_default) {
        HILOGE("usb_project_id is 0x%08x, fw project_id is %d, does not match!!!", usb_project_id[proj_id],
               hw_cfg_cb.lmp_subversion);
        cfg_cb->dl_fw_flag = 0;
        if (cfg_cb->fw_len > 0) {
            free(cfg_cb->fw_buf);
            cfg_cb->fw_len = 0;
        }

        if (cfg_cb->config_len > 0) {
            free(cfg_cb->config_buf);
            cfg_cb->config_len = 0;
        }

        if (entry) {
            free(entry);
        }
        return;
    }

    entry = rtk_get_patch_entry(cfg_cb);
    if (entry) {
        cfg_cb->total_len = entry->patch_length + cfg_cb->config_len;
    } else {
        cfg_cb->dl_fw_flag = 0;
        if (cfg_cb->fw_len > 0) {
            free(cfg_cb->fw_buf);
            cfg_cb->fw_len = 0;
        }

        if (cfg_cb->config_len > 0) {
            free(cfg_cb->config_buf);
            cfg_cb->config_len = 0;
        }

        if (entry) {
            free(entry);
        }
        return;
    }

    HILOGI("total_len = 0x%x", cfg_cb->total_len);

    if (!(cfg_cb->total_buf = malloc(cfg_cb->total_len))) {
        HILOGE("Can't alloc memory for multi fw&config, errno:%d", errno);
        cfg_cb->dl_fw_flag = 0;
        if (cfg_cb->fw_len > 0) {
            free(cfg_cb->fw_buf);
            cfg_cb->fw_len = 0;
        }

        if (cfg_cb->config_len > 0) {
            free(cfg_cb->config_buf);
            cfg_cb->config_len = 0;
        }

        if (entry) {
            free(entry);
        }
        return;
    } else {
        (void)memcpy_s(cfg_cb->total_buf,
                       entry->patch_length, cfg_cb->fw_buf + entry->patch_offset, entry->patch_length);
        (void)memcpy_s(cfg_cb->total_buf + entry->patch_length - USB_MEMCPY_S_4,
                       sizeof(cfg_cb->total_buf) + entry->patch_length, &patch->fw_version, USB_MEMCPY_S_4);
        (void)memcpy_s(&entry->svn_version,
                       entry->patch_length, cfg_cb->total_buf + entry->patch_length - USB_MEMCPY_S_8, USB_MEMCPY_S_4);
        (void)memcpy_s(&entry->coex_version,
                       entry->patch_length, cfg_cb->total_buf + entry->patch_length - USB_MEMCPY_S_12, USB_MEMCPY_S_4);

        HILOGI("BTCOEX:20%06d-%04x svn_version:%d lmp_subversion:0x%x hci_version:0x%x hci_revision:0x%x chip_type:%d "
               "Cut:%d libbt-vendor_uart version:%s, patch->fw_version = %x\n",
               ((entry->coex_version >> USB_ENTRY_16) & 0x7ff) +
                   ((entry->coex_version >> USB_ENTRY_27) * USB_ENTRY_10000),
               (entry->coex_version & 0xffff), entry->svn_version, cfg_cb->lmp_subversion, cfg_cb->hci_version,
               cfg_cb->hci_revision, cfg_cb->chip_type, cfg_cb->eversion + 1, RTK_VERSION, patch->fw_version);
    }

    if (cfg_cb->config_len) {
        (void)memcpy_s(cfg_cb->total_buf + entry->patch_length,
                       cfg_cb->config_len, cfg_cb->config_buf, cfg_cb->config_len);
    }

    cfg_cb->dl_fw_flag = 1;
    HILOGI("Fw:%s exists, config file:%s exists", (cfg_cb->fw_len > 0) ? "" : "not",
           (cfg_cb->config_len > 0) ? "" : "not");
}

static int usb_hci_download_patch_h4(HC_BT_HDR *p_buf, int index, uint8_t *data, int len)
{
    int retval = FALSE;
    uint8_t *p = (uint8_t *)(p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_FW_PATCH);
    *p++ = 1 + len; /* parameter length */
    *p++ = index;
    (void)memcpy_s(p, len, data, len);
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1 + len;

    hw_cfg_cb.state = HW_CFG_DL_FW_PATCH;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_DOWNLOAD_FW_PATCH, p_buf);
    return retval;
}

static void rtk_usb_get_fw_version(bt_hw_cfg_cb_t *cfg_cb)
{
    struct rtk_epatch *patch = (struct rtk_epatch *)cfg_cb->fw_buf;

    if (cfg_cb->lmp_subversion == LMPSUBVERSION_8723a) {
        cfg_cb->lmp_sub_current = 0;
    } else {
        cfg_cb->lmp_sub_current = (uint16_t)patch->fw_version;
    }
}
/*******************************************************************************
**
** Function         hw_usb_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_usb_config_cback(HC_BT_HDR *p_evt_buf)
{
    HC_BT_HDR *p_mem = NULL;
    uint8_t *p = NULL; // , *pp=NULL;
    uint8_t status = 0;
    uint16_t opcode = 0;
    HC_BT_HDR *p_buf = NULL;
    int is_proceeding = FALSE;
    uint8_t iIndexRx = 0;
    usb_patch_info *prtk_usb_patch_file_info = NULL;

#if (USE_CONTROLLER_BDADDR == TRUE)
#endif

    if (p_evt_buf != NULL) {
        p_mem = (HC_BT_HDR *)p_evt_buf;
        status = *((uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
        p = (uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
        STREAM_TO_UINT16(opcode, p);
    }

    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    /* a cut fc6d status==1 */
    if (((status == 0) || (opcode == HCI_VSC_READ_ROM_VERSION)) && bt_vendor_cbacks) {
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_MAX_LEN);
    }

    if (p_buf != NULL) {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        BTVNDDBG("hw_cfg_cb.state = %i", hw_cfg_cb.state);
        switch (hw_cfg_cb.state) {
            case HW_CFG_RESET_CHANNEL_CONTROLLER: {
#define USLEEP_300000 300000
                usleep(USLEEP_300000);
                hw_cfg_cb.state = HW_CFG_READ_LOCAL_VER;
                p = (uint8_t *)(p_buf + 1);
                UINT16_TO_STREAM(p, HCI_READ_LMP_VERSION);
                *p++ = 0;
                p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_READ_LMP_VERSION, p_buf);
                break;
            }
            case HW_CFG_READ_LOCAL_VER: {
                if (status == 0 && p_mem) {
                    p = ((uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_VERSION_OFFSET);
                    STREAM_TO_UINT16(hw_cfg_cb.hci_version, p);
                    p = ((uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_OP1001_HCI_REVISION_OFFSET);
                    STREAM_TO_UINT16(hw_cfg_cb.hci_revision, p);
                    p = (uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_OP1001_LMP_SUBVERSION_OFFSET;
                    STREAM_TO_UINT16(hw_cfg_cb.lmp_subversion, p);

                    prtk_usb_patch_file_info = rtk_usb_get_fw_table_entry(hw_cfg_cb.vid, hw_cfg_cb.pid);
                    if ((prtk_usb_patch_file_info == NULL) || (prtk_usb_patch_file_info->lmp_sub_default == 0)) {
                        HILOGE("get patch entry error");
                        is_proceeding = FALSE;
                        break;
                    }
                    hw_cfg_cb.config_len =
                        rtk_usb_get_bt_config(&hw_cfg_cb.config_buf, prtk_usb_patch_file_info->config_name,
                                              prtk_usb_patch_file_info->mac_offset);
                    hw_cfg_cb.fw_len = rtk_get_bt_firmware(&hw_cfg_cb.fw_buf, prtk_usb_patch_file_info->patch_name);
                    rtk_usb_get_fw_version(&hw_cfg_cb);

                    hw_cfg_cb.lmp_subversion_default = prtk_usb_patch_file_info->lmp_sub_default;
                    BTVNDDBG("lmp_subversion = 0x%x hw_cfg_cb.hci_version = 0x%x hw_cfg_cb.hci_revision = 0x%x, "
                             "hw_cfg_cb.lmp_sub_current = 0x%x",
                             hw_cfg_cb.lmp_subversion, hw_cfg_cb.hci_version, hw_cfg_cb.hci_revision,
                             hw_cfg_cb.lmp_sub_current);

                    if (prtk_usb_patch_file_info->lmp_sub_default == hw_cfg_cb.lmp_subversion) {
                        BTVNDDBG("%s: Cold BT controller startup", __func__);
                        hw_cfg_cb.state = HW_CFG_READ_ECO_VER;
                        p = (uint8_t *)(p_buf + 1);
                        UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
                        *p++ = 0;
                        p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                        is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_ROM_VERSION, p_buf);
                    } else if (hw_cfg_cb.lmp_subversion != hw_cfg_cb.lmp_sub_current) {
                        BTVNDDBG("%s: Warm BT controller startup with updated lmp", __func__);
                        goto RESET_HW_CONTROLLER;
                    } else {
                        BTVNDDBG("%s: Warm BT controller startup with same lmp", __func__);
                        userial_vendor_usb_ioctl(DWFW_CMPLT, &hw_cfg_cb.lmp_sub_current);
                        free(hw_cfg_cb.total_buf);
                        hw_cfg_cb.total_len = 0;

                        bt_vendor_cbacks->dealloc(p_buf);
                        start_fwcfg_cbtimer();
                        hw_cfg_cb.state = 0;
                        is_proceeding = TRUE;
                    }
                } else {
                    HILOGE("status = %d, or p_mem is NULL", status);
                }
                break;
            }
RESET_HW_CONTROLLER:
            case HW_RESET_CONTROLLER: {
                if (status == 0) {
                    userial_vendor_usb_ioctl(RESET_CONTROLLER, NULL); // reset controller
                    hw_cfg_cb.state = HW_CFG_READ_ECO_VER;
                    p = (uint8_t *)(p_buf + 1);
                    UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
                    *p++ = 0;
                    p_buf->len = HCI_CMD_PREAMBLE_SIZE;
                    is_proceeding = bt_vendor_cbacks->xmit_cb(HCI_VSC_READ_ROM_VERSION, p_buf);
                }
                break;
            }
            case HW_CFG_READ_ECO_VER: {
                if (status == 0 && p_mem) {
                    hw_cfg_cb.eversion = *((uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_OPFC6D_EVERSION_OFFSET);
                    BTVNDDBG("hw_usb_config_cback chip_id of the IC:%d", hw_cfg_cb.eversion + 1);
                } else if (status == 1) {
                    hw_cfg_cb.eversion = 0;
                } else {
                    is_proceeding = FALSE;
                    break;
                }

                hw_cfg_cb.state = HW_CFG_START;
                goto CFG_USB_START;
            }
CFG_USB_START:
            case HW_CFG_START: {
                // get efuse config file and patch code file
                prtk_usb_patch_file_info = rtk_usb_get_fw_table_entry(hw_cfg_cb.vid, hw_cfg_cb.pid);
                if ((prtk_usb_patch_file_info == NULL) || (prtk_usb_patch_file_info->lmp_sub_default == 0)) {
                    HILOGE("get patch entry error");
                    is_proceeding = FALSE;
                    break;
                }
                hw_cfg_cb.max_patch_size = prtk_usb_patch_file_info->max_patch_size;
                hw_cfg_cb.config_len = rtk_usb_get_bt_config(
                    &hw_cfg_cb.config_buf, prtk_usb_patch_file_info->config_name, prtk_usb_patch_file_info->mac_offset);
                if (hw_cfg_cb.config_len) {
                    HILOGE("update altsettings");
                    rtk_usb_update_altsettings(prtk_usb_patch_file_info, hw_cfg_cb.config_buf, &(hw_cfg_cb.config_len));
                }

                hw_cfg_cb.fw_len = rtk_get_bt_firmware(&hw_cfg_cb.fw_buf, prtk_usb_patch_file_info->patch_name);
                if (hw_cfg_cb.fw_len < 0) {
                    HILOGE("Get BT firmware fail");
                    hw_cfg_cb.fw_len = 0;
                    is_proceeding = FALSE;
                    break;
                } else {
                    rtk_usb_get_bt_final_patch(&hw_cfg_cb);
                }

                BTVNDDBG("Check total_len(0x%08x) max_patch_size(0x%08x)", hw_cfg_cb.total_len,
                         hw_cfg_cb.max_patch_size);
                if (hw_cfg_cb.total_len > hw_cfg_cb.max_patch_size) {
                    HILOGE("total length of fw&config(0x%08x) larger than max_patch_size(0x%08x)", hw_cfg_cb.total_len,
                           hw_cfg_cb.max_patch_size);
                    is_proceeding = FALSE;
                    break;
                }

                if ((hw_cfg_cb.total_len > 0) && hw_cfg_cb.dl_fw_flag) {
                    hw_cfg_cb.patch_frag_cnt = hw_cfg_cb.total_len / PATCH_DATA_FIELD_MAX_SIZE;
                    hw_cfg_cb.patch_frag_tail = hw_cfg_cb.total_len % PATCH_DATA_FIELD_MAX_SIZE;
                    if (hw_cfg_cb.patch_frag_tail) {
                        hw_cfg_cb.patch_frag_cnt += 1;
                    } else {
                        hw_cfg_cb.patch_frag_tail = PATCH_DATA_FIELD_MAX_SIZE;
                    }
                    BTVNDDBG("patch fragment count %d, tail len %d", hw_cfg_cb.patch_frag_cnt,
                             hw_cfg_cb.patch_frag_tail);
                } else {
                    is_proceeding = FALSE;
                    break;
                }

                goto DOWNLOAD_USB_FW;
            }
                /* fall through intentionally */
DOWNLOAD_USB_FW:
            case HW_CFG_DL_FW_PATCH:
                BTVNDDBG("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, opcode:0x%x", status, opcode);

                // recv command complete event for patch code download command
                if (opcode == HCI_VSC_DOWNLOAD_FW_PATCH) {
                    iIndexRx = *((uint8_t *)(p_mem + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET + 1);
                    BTVNDDBG("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, iIndexRx:%i", status, iIndexRx);
                    hw_cfg_cb.patch_frag_idx++;

                    if (iIndexRx & 0x80) {
                        BTVNDDBG("vendor lib fwcfg completed");
                        userial_vendor_usb_ioctl(DWFW_CMPLT, &hw_cfg_cb.lmp_sub_current);
                        free(hw_cfg_cb.total_buf);
                        hw_cfg_cb.total_len = 0;

                        bt_vendor_cbacks->dealloc(p_buf);
                        start_fwcfg_cbtimer();

                        hw_cfg_cb.state = 0;
                        is_proceeding = TRUE;
                        break;
                    }
                }

                if (hw_cfg_cb.patch_frag_idx < hw_cfg_cb.patch_frag_cnt) {
                    iIndexRx = hw_cfg_cb.patch_frag_idx ? ((hw_cfg_cb.patch_frag_idx - 1) % 0x7f + 1) : 0;
                    if (hw_cfg_cb.patch_frag_idx == hw_cfg_cb.patch_frag_cnt - 1) {
                        BTVNDDBG("HW_CFG_DL_FW_PATCH: send last fw fragment");
                        iIndexRx |= 0x80;
                        hw_cfg_cb.patch_frag_len = hw_cfg_cb.patch_frag_tail;
                    } else {
                        iIndexRx &= 0x7F;
                        hw_cfg_cb.patch_frag_len = PATCH_DATA_FIELD_MAX_SIZE;
                    }
                }

                is_proceeding = usb_hci_download_patch_h4(
                    p_buf, iIndexRx, hw_cfg_cb.total_buf + (hw_cfg_cb.patch_frag_idx * PATCH_DATA_FIELD_MAX_SIZE),
                    hw_cfg_cb.patch_frag_len);
                break;
            default:
                break;
        } // switch(hw_cfg_cb.state)
    }     // if (p_buf != NULL)

    if (is_proceeding == FALSE) {
        HILOGE("vendor lib fwcfg aborted!!!");
        if (bt_vendor_cbacks) {
            if (p_buf != NULL) {
                bt_vendor_cbacks->dealloc(p_buf);
            }

            userial_vendor_usb_ioctl(DWFW_CMPLT, &hw_cfg_cb.lmp_sub_current);

            bt_vendor_cbacks->init_cb(BTC_OP_RESULT_FAIL);
        }

        if (hw_cfg_cb.config_len) {
            free(hw_cfg_cb.config_buf);
            hw_cfg_cb.config_len = 0;
        }

        if (hw_cfg_cb.fw_len) {
            free(hw_cfg_cb.fw_buf);
            hw_cfg_cb.fw_len = 0;
        }

        if (hw_cfg_cb.total_len) {
            free(hw_cfg_cb.total_buf);
            hw_cfg_cb.total_len = 0;
        }
        hw_cfg_cb.state = 0;
    }
}

/*******************************************************************************
**
** Function        hw__usb_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_usb_config_start(char transtype, uint32_t usb_id)
{
    RTK_UNUSED(transtype);
    memset_s(&hw_cfg_cb, sizeof(bt_hw_cfg_cb_t), 0, sizeof(bt_hw_cfg_cb_t));
    hw_cfg_cb.dl_fw_flag = 1;
    hw_cfg_cb.chip_type = CHIPTYPE_NONE;
    hw_cfg_cb.pid = usb_id & 0x0000ffff;
#define USB_ID_16 16
    hw_cfg_cb.vid = (usb_id >> USB_ID_16) & 0x0000ffff;
    BTVNDDBG("RTKBT_RELEASE_NAME: %s", RTKBT_RELEASE_NAME);
    BTVNDDBG("\nRealtek libbt-vendor_usb Version %s \n", RTK_VERSION);
    HC_BT_HDR *p_buf = NULL;
    uint8_t *p;

    BTVNDDBG("hw_usb_config_start, transtype = 0x%x, pid = 0x%04x, vid = 0x%04x \n", transtype, hw_cfg_cb.pid,
             hw_cfg_cb.vid);

    if (bt_vendor_cbacks) {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE);
        if (p_buf) {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            p = (uint8_t *)(p_buf + 1);

            p = (uint8_t *)(p_buf + 1);
            UINT16_TO_STREAM(p, HCI_VENDOR_RESET);
            *p++ = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE;

            hw_cfg_cb.state = HW_CFG_RESET_CHANNEL_CONTROLLER;
            bt_vendor_cbacks->xmit_cb(HCI_VENDOR_RESET, p_buf);
        } else {
            HILOGE("%s buffer alloc fail!", __func__);
            bt_vendor_cbacks->init_cb(BTC_OP_RESULT_FAIL);
        }
    } else {
        HILOGE("%s call back is null", __func__);
    }
}
