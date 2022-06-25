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

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"
#define RTKBT_RELEASE_NAME "20190717_BT_ANDROID_9.0"

#include "hardware.h"

#include <byteswap.h>
#include <ctype.h>
#include <dirent.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utils/Log.h>

#include "bt_hci_bdroid.h"
#include "bt_vendor_lib.h"
#include "bt_vendor_rtk.h"
#include "upio.h"
#include "userial.h"
#include "userial_vendor.h"

/******************************************************************************
**  Constants &  Macros
******************************************************************************/

/******************************************************************************
**  Externs
******************************************************************************/

/******************************************************************************
**  Static variables
******************************************************************************/
bt_hw_cfg_cb_t hw_cfg_cb;

int getmacaddr(unsigned char *addr)
{
    int i = 0;
    char data[256], *str;
    int addr_fd;
    uint8_t *p_vnd_local_bd_addr = get_vnd_local_bd_addr();

    char property[100] = {0};
    (void)memset_s(property, sizeof(property), 0, sizeof(property));
    (void)strcpy_s(property, sizeof(property), "none");
    if (strncmp(property, "none", 4L) == 0) {
        if (strcmp(property, "none") == 0) {
            return -1;
        } else if (strcmp(property, "default") == 0) {
            (void)memcpy_s(addr, BD_ADDR_LEN, p_vnd_local_bd_addr, BD_ADDR_LEN);
            return 0;
        } else if ((addr_fd = open(property, O_RDONLY)) != -1) {
            (void)memset_s(data, sizeof(data), 0, sizeof(data));
            int ret = read(addr_fd, data, 17L);
            if (ret < 17L) {
                HILOGE("%s, read length = %d", __func__, ret);
                close(addr_fd);
                return -1;
            }
            for (i = 0, str = data; i < 6L; i++) {
                addr[5L - i] = (unsigned char)strtoul(str, &str, 16L);
                str++;
            }
            close(addr_fd);
            return 0;
        }
    }
    return -1;
}

int rtk_get_bt_firmware(uint8_t **fw_buf, char *fw_short_name)
{
    char filename[PATH_MAX] = {0};
    struct stat st;
    int fd = -1;
    size_t fwsize = 0;
    size_t buf_size = 0;

    (void)sprintf_s(filename, sizeof(filename), FIRMWARE_DIRECTORY, fw_short_name);
    HILOGI("BT fw file: %s", filename);

    if (stat(filename, &st) < 0) {
        HILOGE("Can't access firmware, errno:%d", errno);
        return -1;
    }

    fwsize = st.st_size;
    buf_size = fwsize;

    if ((fd = open(filename, O_RDONLY)) < 0) {
        HILOGE("Can't open firmware, errno:%d", errno);
        return -1;
    }

    if (!(*fw_buf = malloc(buf_size))) {
        HILOGE("Can't alloc memory for fw&config, errno:%d", errno);
        if (fd >= 0) {
            close(fd);
        }
        return -1;
    }

    if (read(fd, *fw_buf, fwsize) < (ssize_t)fwsize) {
        free(*fw_buf);
        *fw_buf = NULL;
        if (fd >= 0) {
            close(fd);
        }
        return -1;
    }

    if (fd >= 0) {
        close(fd);
    }

    HILOGI("Load FW OK");
    return buf_size;
}

uint8_t rtk_get_fw_project_id(uint8_t *p_buf)
{
    uint8_t opcode;
    uint8_t len;
    uint8_t data = 0;

    do {
        opcode = *p_buf;
        len = *(p_buf - 1);
        if (opcode == 0x00) {
            if (len == 1) {
                data = *(p_buf - 2L);
                HILOGI("bt_hw_parse_project_id: opcode %d, len %d, data %d", opcode, len, data);
                break;
            } else {
                HILOGW("bt_hw_parse_project_id: invalid len %d", len);
            }
        }
        p_buf -= len + 2L;
    } while (*p_buf != 0xFF);

    return data;
}

uint8_t get_heartbeat_from_hardware(void)
{
    return hw_cfg_cb.heartbeat;
}

struct rtk_epatch_entry *rtk_get_patch_entry(bt_hw_cfg_cb_t *cfg_cb)
{
    uint16_t i;
    struct rtk_epatch *patch;
    struct rtk_epatch_entry *entry;
    uint8_t *p;
    uint16_t chip_id;

    patch = (struct rtk_epatch *)cfg_cb->fw_buf;
    entry = (struct rtk_epatch_entry *)malloc(sizeof(*entry));
    if (!entry) {
        HILOGE("rtk_get_patch_entry: failed to allocate mem for patch entry");
        return NULL;
    }

    patch->number_of_patch = le16_to_cpu(patch->number_of_patch);

    HILOGI("rtk_get_patch_entry: fw_ver 0x%08x, patch_num %d", le32_to_cpu(patch->fw_version), patch->number_of_patch);

    for (i = 0; i < patch->number_of_patch; i++) {
        p = cfg_cb->fw_buf + 14L + 2L * i;
        STREAM_TO_UINT16(chip_id, p);
        if (chip_id == cfg_cb->eversion + 1) {
            entry->chip_id = chip_id;
            p = cfg_cb->fw_buf + 14L + 2L * patch->number_of_patch + 2L * i;
            STREAM_TO_UINT16(entry->patch_length, p);
            p = cfg_cb->fw_buf + 14L + 4L * patch->number_of_patch + 4L * i;
            STREAM_TO_UINT32(entry->patch_offset, p);
            HILOGI("rtk_get_patch_entry: chip_id %d, patch_len 0x%x, patch_offset 0x%x", entry->chip_id,
                   entry->patch_length, entry->patch_offset);
            break;
        }
    }

    if (i == patch->number_of_patch) {
        HILOGE("rtk_get_patch_entry: failed to get etnry");
        free(entry);
        entry = NULL;
    }

    return entry;
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_lpm_ctrl_cback
**
** Description      Callback function for lpm enable/disable request
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_ctrl_cback(HC_BT_HDR *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *)p_mem;

    if (*((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET) == 0) {
    }

    if (bt_vendor_cbacks) {
    }
}

#if (HW_END_WITH_HCI_RESET == TRUE)
/******************************************************************************
 *
 **
 ** Function         hw_epilog_cback
 **
 ** Description      Callback function for Command Complete Events from HCI
 **                  commands sent in epilog process.
 **
 ** Returns          None
 **
 *******************************************************************************/
void hw_epilog_cback(HC_BT_HDR *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *)p_mem;
    uint8_t *p, status;
    uint16_t opcode;

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_OFFSET);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE_OFFSET;
    STREAM_TO_UINT16(opcode, p);

    BTVNDDBG("%s Opcode:0x%04X Status: %d", __FUNCTION__, opcode, status);

    if (bt_vendor_cbacks) {
        /* Must free the RX event buffer */
        bt_vendor_cbacks->dealloc(p_evt_buf);

        /* Once epilog process is done, must call epilog_cb callback
           to notify caller */
        bt_vendor_cbacks->init_cb(BTC_OP_RESULT_SUCCESS);
    }
}

/******************************************************************************
 *
 **
 ** Function         hw_epilog_process
 **
 ** Description      Sample implementation of epilog process
 **
 ** Returns          None
 **
 *******************************************************************************/
void hw_epilog_process(void)
{
    HC_BT_HDR *p_buf = NULL;
    uint8_t *p;

    BTVNDDBG("hw_epilog_process");

    /* Sending a HCI_RESET */
    if (bt_vendor_cbacks) {
        /* Must allocate command buffer via HC's alloc API */
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE);
    }

    if (p_buf) {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = HCI_CMD_PREAMBLE_SIZE;

        p = (uint8_t *)(p_buf + 1);
        UINT16_TO_STREAM(p, HCI_RESET);
        *p = 0; /* parameter length */

        /* Send command via HC's xmit_cb API */
        bt_vendor_cbacks->xmit_cb(HCI_RESET, p_buf);
    } else {
        if (bt_vendor_cbacks) {
            HILOGE("vendor lib epilog process aborted [no buffer]");
            bt_vendor_cbacks->init_cb(BTC_OP_RESULT_FAIL);
        }
    }
}
#endif // (HW_END_WITH_HCI_RESET == TRUE)
