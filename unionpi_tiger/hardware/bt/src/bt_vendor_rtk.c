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
 *  Filename:      bt_vendor_rtk.c
 *
 *  Description:   Realtek vendor specific library implementation
 *
 ******************************************************************************/

#undef NDEBUG
#define LOG_TAG "libbt_vendor"
#define RTKBT_RELEASE_NAME "20190717_BT_ANDROID_9.0"

#include "bt_vendor_rtk.h"

#include <string.h>
#include <utils/Log.h>

#include "upio.h"
#include "userial_vendor.h"
#include "hardware_uart.h"
#include "hardware_usb.h"
#include "rtk_btservice.h"
#include "rtk_parse.h"

#ifndef BTVND_DBG
#define BTVND_DBG FALSE
#endif
#undef BTVNDDBG
#if (BTVND_DBG == TRUE)
#define BTVNDDBG(param, ...)                                                                                           \
    {                                                                                                                  \
        HILOGD(param, ##__VA_ARGS__);                                                                                  \
    }
#else
#define BTVNDDBG(param, ...)                                                                                           \
    {                                                                                                                  \
        HILOGD(param, ##__VA_ARGS__);                                                                                  \
    }
#endif

/******************************************************************************
**  Externs
******************************************************************************/

#if (HW_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif

/******************************************************************************
**  Variables
******************************************************************************/
bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[BD_ADDR_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
bool rtkbt_auto_restart = false;

/******************************************************************************
**  Local type definitions
******************************************************************************/
#define DEVICE_NODE_MAX_LEN 512
#define RTKBT_CONF_FILE "/vendor/etc/bluetooth/rtkbt.conf"
#define USB_DEVICE_DIR "/sys/bus/usb/devices"
#define DEBUG_SCAN_USB FALSE

/******************************************************************************
**  Static Variables
******************************************************************************/
// transfer_type(4 bit) | transfer_interface(4 bit)
char rtkbt_transtype = RTKBT_TRANS_H5 | RTKBT_TRANS_UART; /* RTL8822CS Uart H5 default */

bool get_rtkbt_auto_restart(void)
{
    bool auto_restart_onoff = rtkbt_auto_restart;
    return auto_restart_onoff;
}

uint8_t *get_vnd_local_bd_addr(void)
{
    uint8_t *p_vnd_local_bd_addr = vnd_local_bd_addr;
    return p_vnd_local_bd_addr;
}

char get_rtkbt_transtype(void)
{
    return rtkbt_transtype;
}

static char rtkbt_device_node[DEVICE_NODE_MAX_LEN] = {0};

static const tUSERIAL_CFG userial_H5_cfg = {(USERIAL_DATABITS_8 | USERIAL_PARITY_EVEN | USERIAL_STOPBITS_1),
                                            USERIAL_BAUD_115200, USERIAL_HW_FLOW_CTRL_OFF};
static const tUSERIAL_CFG userial_H4_cfg = {(USERIAL_DATABITS_8 | USERIAL_PARITY_NONE | USERIAL_STOPBITS_1),
                                            USERIAL_BAUD_115200, USERIAL_HW_FLOW_CTRL_OFF};

/******************************************************************************
**  Functions
******************************************************************************/
static int Check_Key_Value(char *path, char *key, int value)
{
    FILE *fp;
    char newpath[100];
    char string_get[6];
    int value_int = 0;
    char *fgetret = NULL;
#define MEMSET_S_100 100
    (void)memset_s(newpath, sizeof(newpath), 0, MEMSET_S_100);
    (void)sprintf_s(newpath, sizeof(newpath), "%s/%s", path, key);
    if ((fp = fopen(newpath, "r")) != NULL) {
#define MEMSET_S_6 6
        (void)memset_s(string_get, sizeof(string_get), 0, MEMSET_S_6);
        fgetret = fgets(string_get, 5L, fp);
        if (fgetret != NULL) {
            if (DEBUG_SCAN_USB) {
                HILOGE("string_get %s =%s\n", key, string_get);
            }
        }
        (void)fclose(fp);
#define VENDOR_STRTOUT_16 16
        value_int = strtol(string_get, NULL, VENDOR_STRTOUT_16);
        if (value_int == value) {
            return 1;
        }
    }
    return 0;
}

static int Scan_Usb_Devices_For_RTK(char *path)
{
    char newpath[100];
    char subpath[100];
    DIR *pdir;
    DIR *newpdir;
    struct dirent *ptr;
    struct dirent *newptr;
    struct stat filestat;
    struct stat subfilestat;
    if (stat(path, &filestat) != 0) {
        HILOGE("The file or path(%s) can not be get stat!\n", newpath);
        return -1;
    }
    if ((filestat.st_mode & S_IFDIR) != S_IFDIR) {
        HILOGE("(%s) is not be a path!\n", path);
        return -1;
    }
    pdir = opendir(path);
    /* enter sub direc */
    while ((ptr = readdir(pdir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) {
            continue;
        }
#define MEMSET_100 100
        (void)memset_s(newpath, sizeof(newpath), 0, MEMSET_100);
        (void)sprintf_s(newpath, sizeof(newpath), "%s/%s", path, ptr->d_name);
        if (DEBUG_SCAN_USB) {
            HILOGE("The file or path(%s)\n", newpath);
        }
        if (stat(newpath, &filestat) != 0) {
            HILOGE("The file or path(%s) can not be get stat!\n", newpath);
            continue;
        }
        /* Check if it is path. */
        if ((filestat.st_mode & S_IFDIR) == S_IFDIR) {
            if (!Check_Key_Value(newpath, "idVendor", 0x0bda)) {
                continue;
            }
            newpdir = opendir(newpath);
            /* read sub directory */
            while ((newptr = readdir(newpdir)) != NULL) {
                if (strcmp(newptr->d_name, ".") == 0 || strcmp(newptr->d_name, "..") == 0) {
                    continue;
                }
                (void)memset_s(subpath, sizeof(subpath), 0, MEMSET_100);
                (void)sprintf_s(subpath, sizeof(subpath), "%s/%s", newpath, newptr->d_name);
                if (DEBUG_SCAN_USB) {
                    HILOGE("The file or path(%s)\n", subpath);
                }
                if (stat(subpath, &subfilestat) != 0) {
                    HILOGE("The file or path(%s) can not be get stat!\n", newpath);
                    continue;
                }
                /* Check if it is path. */
                if ((subfilestat.st_mode & S_IFDIR) == S_IFDIR) {
                    if (Check_Key_Value(subpath, "bInterfaceClass", 0xe0) &&
                        Check_Key_Value(subpath, "bInterfaceSubClass", 0x01) &&
                        Check_Key_Value(subpath, "bInterfaceProtocol", 0x01)) {
                        closedir(newpdir);
                        closedir(pdir);
                        return 1;
                    }
                }
            }
            closedir(newpdir);
        }
    }
    closedir(pdir);
    return 0;
}

static char *rtk_trim(char *str)
{
    char *str_tmp = str;
    while (isspace(*str_tmp)) {
        ++str_tmp;
    }

    if (!*str_tmp) {
        return str_tmp;
    }

    char *end_str = str_tmp + strlen(str_tmp) - 1;
    while (end_str > str_tmp && isspace(*end_str)) {
        --end_str;
    }

    end_str[1] = '\0';
    return str_tmp;
}

static void load_rtkbt_conf(void)
{
    char *split;
    int line_num = 0;
    char line[1024];
    char *lineptr = NULL;

    (void)memset_s(rtkbt_device_node, sizeof(rtkbt_device_node), 0, sizeof(rtkbt_device_node));
    FILE *fp = fopen(RTKBT_CONF_FILE, "rt");
    if (!fp) {
        HILOGE("%s unable to open file '%s': %s", __func__, RTKBT_CONF_FILE, strerror(errno));
        (void)strcpy_s(rtkbt_device_node, sizeof(rtkbt_device_node), "/dev/rtkbt_dev");
        return;
    }

    while ((lineptr = fgets(line, sizeof(line), fp)) != NULL) {
        char *line_ptr = rtk_trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#' || *line_ptr == '[') {
            continue;
        }

        split = strchr(line_ptr, '=');
        if (!split) {
            HILOGE("%s no key/value separator found on line %d.", __func__, line_num);
            (void)strcpy_s(rtkbt_device_node, sizeof(rtkbt_device_node), "/dev/rtkbt_dev");
            fclose(fp);
            return;
        }

        *split = '\0';
        if (!strcmp(rtk_trim(line_ptr), "BtDeviceNode")) {
            (void)strcpy_s(rtkbt_device_node, sizeof(rtkbt_device_node), rtk_trim(split + 1));
        }
    }

    (void)fclose(fp);

    /* default H5 (Uart) */
    rtkbt_transtype |= RTKBT_TRANS_H5;
    rtkbt_transtype |= RTKBT_TRANS_UART;
}

static void rtkbt_stack_conf_cleanup(void)
{
    set_rtkbt_h5logfilter(0);
    set_h5_log_enable(0);
    set_rtk_btsnoop_dump(false);
    set_rtk_btsnoop_net_dump(false);
}

static void load_rtkbt_stack_conf(void)
{
    char *split;
    int line_num = 0;
    char line[1024];
    char *lineptr = NULL;
    int ret_coex_log_onoff = 0;
    unsigned int ret_h5_log_enable = 0;
    unsigned int ret_h5logfilter = 0x01;
    char *btsnoop_path = get_rtk_btsnoop_path();

    FILE *fp = fopen(RTKBT_CONF_FILE, "rt");
    if (!fp) {
        HILOGE("%s unable to open file '%s': %s", __func__, RTKBT_CONF_FILE, strerror(errno));
        return;
    }

    while ((lineptr = fgets(line, sizeof(line), fp)) != NULL) {
        char *line_ptr = rtk_trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#' || *line_ptr == '[') {
            continue;
        }

        split = strchr(line_ptr, '=');
        if (!split) {
            HILOGE("%s no key/value separator found on line %d.", __func__, line_num);
            continue;
        }

        *split = '\0';
        char *endptr;
        if (!strcmp(rtk_trim(line_ptr), "RtkbtLogFilter")) {
            ret_h5logfilter = strtol(rtk_trim(split + 1), &endptr, 0);
            set_rtkbt_h5logfilter(ret_h5logfilter);
        } else if (!strcmp(rtk_trim(line_ptr), "H5LogOutput")) {
            ret_h5_log_enable = strtol(rtk_trim(split + 1), &endptr, 0);
            set_h5_log_enable(ret_h5_log_enable);
        } else if (!strcmp(rtk_trim(line_ptr), "RtkBtsnoopDump")) {
            if (!strcmp(rtk_trim(split + 1), "true")) {
                set_rtk_btsnoop_dump(true);
            }
        } else if (!strcmp(rtk_trim(line_ptr), "RtkBtsnoopNetDump")) {
            if (!strcmp(rtk_trim(split + 1), "true")) {
                set_rtk_btsnoop_net_dump(true);
            }
        } else if (!strcmp(rtk_trim(line_ptr), "BtSnoopFileName")) {
            (void)sprintf_s(btsnoop_path, 1024L, "%s_rtk", rtk_trim(split + 1));
        } else if (!strcmp(rtk_trim(line_ptr), "BtSnoopSaveLog")) {
            if (!strcmp(rtk_trim(split + 1), "true")) {
                set_rtk_btsnoop_save_log (true);
            }
        } else if (!strcmp(rtk_trim(line_ptr), "BtCoexLogOutput")) {
            ret_coex_log_onoff = strtol(rtk_trim(split + 1), &endptr, 0);
            set_coex_log_onoff(ret_coex_log_onoff);
        } else if (!strcmp(rtk_trim(line_ptr), "RtkBtAutoRestart")) {
            if (!strcmp(rtk_trim(split + 1), "true")) {
                rtkbt_auto_restart = true;
            }
        }
    }

    (void)fclose(fp);
}

static void byte_reverse(unsigned char *data, int len)
{
    int i;
    int tmp;
#define LEN_2 2

    for (i = 0; i < len / LEN_2; i++) {
        tmp = len - i - 1;
        data[i] ^= data[tmp];
        data[tmp] ^= data[i];
        data[i] ^= data[tmp];
    }
}

#define LOCAL_BDADDR_0 0
#define LOCAL_BDADDR_1 1
#define LOCAL_BDADDR_2 2
#define LOCAL_BDADDR_3 3
#define LOCAL_BDADDR_4 4
#define LOCAL_BDADDR_5 5

static int init(const bt_vendor_callbacks_t *p_cb, unsigned char *local_bdaddr)
{
    int retval = -1;
    bool if_rtk_btsnoop_dump = get_rtk_btsnoop_dump();
    HILOGD("init, bdaddr:%02x:%02x:%02x:%02x:%02x:%02x", local_bdaddr[LOCAL_BDADDR_0], local_bdaddr[LOCAL_BDADDR_1],
           local_bdaddr[LOCAL_BDADDR_2], local_bdaddr[LOCAL_BDADDR_3], local_bdaddr[LOCAL_BDADDR_4],
           local_bdaddr[LOCAL_BDADDR_5]);
    bool if_btsnoop_net_dump = get_rtk_btsnoop_net_dump();

    if (p_cb == NULL) {
        HILOGE("init failed with no user callbacks!");
        return retval;
    }

#if (VENDOR_LIB_RUNTIME_TUNING_ENABLED == TRUE)
    HILOGD("*****************************************************************");
    HILOGD("*****************************************************************");
    HILOGD("** Warning - BT Vendor Lib is loaded in debug tuning mode!");
    HILOGD("**");
    HILOGD("** If this is not intentional, rebuild libbt-vendor.so ");
    HILOGD("** with VENDOR_LIB_RUNTIME_TUNING_ENABLED=FALSE and ");
    HILOGD("** check if any run-time tuning parameters needed to be");
    HILOGD("** carried to the build-time configuration accordingly.");
    HILOGD("*****************************************************************");
    HILOGD("*****************************************************************");
#endif

    load_rtkbt_conf();
    load_rtkbt_stack_conf();
    if (p_cb == NULL) {
        HILOGE("init failed with no user callbacks!");
        return -1;
    }

    userial_vendor_init(rtkbt_device_node);

    if (rtkbt_transtype & RTKBT_TRANS_UART) {
        upio_init();
        HILOGE("bt_wake_up_host_mode_set(1)");
        bt_wake_up_host_mode_set(1);
    }

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *)p_cb;

    /* This is handed over from the stack */
    retval = memcpy_s(vnd_local_bd_addr, BD_ADDR_LEN, local_bdaddr, BD_ADDR_LEN);

    byte_reverse(vnd_local_bd_addr, BD_ADDR_LEN);

    if (if_rtk_btsnoop_dump) {
        rtk_btsnoop_open();
    }
    if (if_btsnoop_net_dump) {
        rtk_btsnoop_net_open();
    }

    return retval;
}

/** Requested operations */
static int op(bt_opcode_t opcode, void *param)
{
    int retval = 0;

    switch (opcode) {
        case BT_OP_POWER_ON: // Power on the BT Controller.
            if (rtkbt_transtype & RTKBT_TRANS_UART) {
                upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
                usleep(20000L);
                BTVNDDBG("set power off and delay 200ms");
                upio_set_bluetooth_power(UPIO_BT_POWER_ON);
                BTVNDDBG("set power on and delay 00ms");
            }
            break;

        case BT_OP_POWER_OFF: // Power off the BT Controller.
            if (rtkbt_transtype & RTKBT_TRANS_UART) {
                upio_set_bluetooth_power(UPIO_BT_POWER_OFF);
                usleep(20000L);
                BTVNDDBG("set power off and delay 200ms");
            }
            break;

        // Establish hci channels. it will be called after BT_OP_POWER_ON.
        case BT_OP_HCI_CHANNEL_OPEN: {
            if ((rtkbt_transtype & RTKBT_TRANS_UART) && (rtkbt_transtype & RTKBT_TRANS_H5)) {
                int fd, idx;
                int(*fd_array)[] = (int(*)[])param;
                if (userial_vendor_open((tUSERIAL_CFG *)&userial_H5_cfg) != -1) {
                    retval = 1;
                }

                fd = userial_socket_open();
                if (fd != -1) {
                    for (idx = 0; idx < HCI_MAX_CHANNEL; idx++) {
                        (*fd_array)[idx] = fd;
                    }
                } else {
                    retval = 0;
                }
            } else if ((rtkbt_transtype & RTKBT_TRANS_UART) && (rtkbt_transtype & RTKBT_TRANS_H4)) {
                /* retval contains numbers of open fd of HCI channels */
                int(*fd_array)[] = (int(*)[])param;
                int fd, idx;
                if (userial_vendor_open((tUSERIAL_CFG *)&userial_H4_cfg) != -1) {
                    retval = 1;
                }
                fd = userial_socket_open();
                if (fd != -1) {
                    for (idx = 0; idx < HCI_MAX_CHANNEL; idx++) {
                        (*fd_array)[idx] = fd;
                    }
                } else {
                    retval = 0;
                }
                /* retval contains numbers of open fd of HCI channels */
            } else {
                BTVNDDBG("USB op for %d", opcode);
                int fd, idx = 0;
                int(*fd_array)[] = (int(*)[])param;
                for (idx = 0; idx < 10L; idx++) {
                    if (userial_vendor_usb_open() != -1) {
                        retval = 1;
                        break;
                    }
                }
                fd = userial_socket_open();
                if (fd != -1) {
                    for (idx = 0; idx < HCI_MAX_CHANNEL; idx++) {
                        (*fd_array)[idx] = fd;
                    }
                } else {
                    retval = 0;
                }
            }
        }
            break;

        // Close all the hci channels which is opened.
        case BT_OP_HCI_CHANNEL_CLOSE: {
            userial_vendor_close();
        }
            break;

        // initialization the BT Controller. it will be called after
        // BT_OP_HCI_CHANNEL_OPEN. Controller Must call init_cb to notify the host
        // once it has been done.
        case BT_OP_INIT: {
            if (rtkbt_transtype & RTKBT_TRANS_UART) {
                hw_config_start(rtkbt_transtype);
            } else {
                int usb_info = 0;
                retval = userial_vendor_usb_ioctl(GET_USB_INFO, &usb_info);
                if (retval == -1) {
                    HILOGE("get usb info fail");
                    return retval;
                } else {
                    hw_usb_config_start(RTKBT_TRANS_H4, usb_info);
                }
            }
            RTK_btservice_init();
        }
            break;

            // Get the LPM idle timeout in milliseconds.
        case BT_OP_GET_LPM_TIMER: {
        }
            break;

        // Enable LPM mode on BT Controller.
        case BT_OP_LPM_ENABLE: {
        }
            break;

        // Disable LPM mode on BT Controller.
        case BT_OP_LPM_DISABLE: {
        }
            break;

        // Wakeup lock the BTC.
        case BT_OP_WAKEUP_LOCK: {
        }
            break;

        // Wakeup unlock the BTC.
        case BT_OP_WAKEUP_UNLOCK: {
        }
            break;

        // transmit event response to vendor lib.
        case BT_OP_EVENT_CALLBACK: {
            hw_process_event((HC_BT_HDR *)param);
        }
            break;
    }

    return retval;
}

/** Closes the interface */
static void cleanup(void)
{
    bool if_rtk_btsnoop_dump = get_rtk_btsnoop_dump();
	bool if_btsnoop_net_dump = get_rtk_btsnoop_net_dump();
    BTVNDDBG("cleanup");

    if (rtkbt_transtype & RTKBT_TRANS_UART) {
        upio_cleanup();
        bt_wake_up_host_mode_set(0);
    }
    bt_vendor_cbacks = NULL;

    if (if_rtk_btsnoop_dump) {
        rtk_btsnoop_close();
    }
    if (if_btsnoop_net_dump) {
        rtk_btsnoop_net_close();
    }
    rtkbt_stack_conf_cleanup();
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {sizeof(bt_vendor_interface_t), init, op, cleanup};
