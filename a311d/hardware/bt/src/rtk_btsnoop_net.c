/******************************************************************************
 *
 *  Copyright (C) 2013-2014 Google Inc.
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
#define LOG_TAG "rtk_btsnoop_net"
#include <unistd.h>
#include "bt_vendor_rtk.h"
#include "rtk_btsnoop_net.h"

#define RTK_NO_INTR(fn)                                                                                                \
    do {                                                                                                               \
    } while ((fn) == -1 && errno == EINTR)

#define DATA_DIRECT_2_ELLISY 1

#define HCI_COMMAND 0x01
#define HCI_ACL_DATA_H2C 0x02
#define HCI_ACL_DATA_C2H 0x82
#define HCI_SCO_DATA_H2C 0x03
#define HCI_SCO_DATA_C2H 0x83
#define HCI_EVENT 0x84

#define HCI_COMMAND_PKT 0x01
#define HCI_ACLDATA_PKT 0x02
#define HCI_SCODATA_PKT 0x03
#define HCI_EVENT_PKT 0x04

unsigned int rtkbt_h5logfilter = 0x01;
bool rtk_btsnoop_dump = false;
bool rtk_btsnoop_net_dump = false;
bool rtk_btsnoop_save_log = false;
char rtk_btsnoop_path[1024] = {'\0'};
static pthread_mutex_t btsnoop_log_lock;

static void rtk_safe_close_(int *fd);
static void *rtk_listen_fn_(void *context);

static const char *RTK_LISTEN_THREAD_NAME_ = "rtk_btsnoop_net";
static const int RTK_LOCALHOST_ = 0xC0A80AE2; // 192.168.10.226
static const int RTK_LISTEN_PORT_ = 8872;

static const int RTK_REMOTEHOST_ = 0xC0A80A03; // 192.168.10.21
static const int RTK_REMOTE_PORT_ = 24352;

static pthread_t rtk_listen_thread_;
static bool rtk_listen_thread_valid_ = false;
static pthread_mutex_t rtk_client_socket_lock_ = PTHREAD_MUTEX_INITIALIZER;
static int rtk_listen_socket_ = -1;

// File descriptor for btsnoop file.
static int hci_btsnoop_fd = -1;
// Epoch in microseconds since 01/01/0000.
static const uint64_t BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;

char *get_rtk_btsnoop_path(void)
{
    char *path = rtk_btsnoop_path;
    return path;
}

void set_rtk_btsnoop_save_log(bool btsnoop_save_log)
{
    rtk_btsnoop_save_log = btsnoop_save_log;
}

void set_rtk_btsnoop_net_dump(bool btsnoop_net_dump)
{
    rtk_btsnoop_dump = btsnoop_net_dump;
}

bool get_rtk_btsnoop_net_dump(void)
{
    return rtk_btsnoop_net_dump;
}


void set_rtk_btsnoop_dump(bool btsnoop_dump)
{
    rtk_btsnoop_dump = btsnoop_dump;
}

bool get_rtk_btsnoop_dump(void)
{
    return rtk_btsnoop_dump;
}

void set_rtkbt_h5logfilter(unsigned int h5logfilter)
{
    rtkbt_h5logfilter = h5logfilter;
}

static uint64_t rtk_btsnoop_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Timestamp is in microseconds.
    uint64_t timestamp = tv.tv_sec * 1000LL * 1000LL;
    timestamp += tv.tv_usec;
    timestamp += BTSNOOP_EPOCH_DELTA;
    return timestamp;
}

void rtk_btsnoop_open(void)
{
    pthread_mutex_init(&btsnoop_log_lock, NULL);
    char last_log_path[PATH_MAX];
    uint64_t timestamp;
    uint32_t usec;
    uint8_t sec, hour, minus, day;

    if (hci_btsnoop_fd != -1) {
        HILOGE("%s btsnoop log file is already open.", __func__);
        return;
    }

    if (rtk_btsnoop_save_log) {
        timestamp = rtk_btsnoop_timestamp() - BTSNOOP_EPOCH_DELTA;
        usec = (uint32_t)(timestamp % 1000000LL);
        timestamp /= 1000000LL;
        sec = (uint8_t)(timestamp % 60LL);
        timestamp /= 60LL;
        minus = (uint8_t)(timestamp % 60LL);
        timestamp /= 60LL;
        hour = (uint8_t)(timestamp % 24LL);
        timestamp /= 24LL;
        day = (uint8_t)(timestamp % 30LL);
        timestamp /= 30LL;
        (void)snprintf_s(last_log_path, PATH_MAX, PATH_MAX, "%s.%uY-%dD-%dH-%dM-%dS-%dUS",
            rtk_btsnoop_path, (uint32_t)timestamp, day, hour, minus, sec, usec);
        if (!rename(rtk_btsnoop_path, last_log_path) && errno != ENOENT) {
            HILOGE("%s unable to rename '%s' to '%s': %s", __func__, rtk_btsnoop_path, last_log_path, strerror(errno));
        }
    } else {
        (void)snprintf_s(last_log_path, PATH_MAX, PATH_MAX, "%s.last", rtk_btsnoop_path);
        if (!rename(rtk_btsnoop_path, last_log_path) && errno != ENOENT) {
            HILOGE("%s unable to rename '%s' to '%s': %s", __func__, rtk_btsnoop_path, last_log_path, strerror(errno));
        }
    }

    hci_btsnoop_fd =
        open(rtk_btsnoop_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (hci_btsnoop_fd == -1) {
        HILOGE("%s unable to open '%s': %s", __func__, rtk_btsnoop_path, strerror(errno));
        return;
    }

    write(hci_btsnoop_fd, "btsnoop\0\0\0\0\1\0\0\x3\xea", 16L);
}

void rtk_btsnoop_close(void)
{
    pthread_mutex_destroy(&btsnoop_log_lock);
    if (hci_btsnoop_fd != -1) {
        close(hci_btsnoop_fd);
    }
    hci_btsnoop_fd = -1;
}

static void rtk_btsnoop_write(const void *data, size_t length)
{
    if (hci_btsnoop_fd != -1) {
        write(hci_btsnoop_fd, data, length);
    }
}

static void rtk_btsnoop_write_packet(serial_data_type_t type, const uint8_t *packet, bool is_received)
{
    int length_he = 0;
    int length;
    int flags;
    int drops = 0;
    pthread_mutex_lock(&btsnoop_log_lock);
    switch (type) {
        case HCI_COMMAND_PKT:
            length_he = packet[2L] + 4L;
            flags = 2L;
            break;
        case HCI_ACLDATA_PKT:
            length_he = (packet[3L] << 8L) + packet[2L] + 5L;
            flags = is_received;
            break;
        case HCI_SCODATA_PKT:
            length_he = packet[2L] + 4L;
            flags = is_received;
            break;
        case HCI_EVENT_PKT:
            length_he = packet[1] + 3L;
            flags = 3L;
            break;
        default:
            break;
    }

    uint64_t timestamp = rtk_btsnoop_timestamp();
    uint32_t time_hi = timestamp >> 32L;
    uint32_t time_lo = timestamp & 0xFFFFFFFF;

    length = htonl(length_he);
    flags = htonl(flags);
    drops = htonl(drops);
    time_hi = htonl(time_hi);
    time_lo = htonl(time_lo);

    rtk_btsnoop_write(&length, 4L);
    rtk_btsnoop_write(&length, 4L);
    rtk_btsnoop_write(&flags, 4L);
    rtk_btsnoop_write(&drops, 4L);
    rtk_btsnoop_write(&time_hi, 4L);
    rtk_btsnoop_write(&time_lo, 4L);
    rtk_btsnoop_write(&type, 1);
    rtk_btsnoop_write(packet, length_he - 1);
    pthread_mutex_unlock(&btsnoop_log_lock);
}

void rtk_btsnoop_capture(const HC_BT_HDR *p_buf, bool is_rcvd)
{
    const uint8_t *p = (const uint8_t *)(p_buf + 1) + p_buf->offset;

    if (hci_btsnoop_fd == -1) {
        return;
    }

    switch (p_buf->event & MSG_EVT_MASK) {
        case MSG_HC_TO_STACK_HCI_EVT:
            if ((*(p + 3L) == 0x94) && (*(p + 4L) == 0xfc) && (*(p + 5L) == 0x00) && (rtkbt_h5logfilter & 1)) {
            } else {
                rtk_btsnoop_write_packet(HCI_EVENT_PKT, p, false);
            }
            break;
        case MSG_HC_TO_STACK_HCI_ACL:
        case MSG_STACK_TO_HC_HCI_ACL:
            rtk_btsnoop_write_packet(HCI_ACLDATA_PKT, p, is_rcvd);
            break;
        case MSG_HC_TO_STACK_HCI_SCO:
        case MSG_STACK_TO_HC_HCI_SCO:
            rtk_btsnoop_write_packet(HCI_SCODATA_PKT, p, is_rcvd);
            break;
        case MSG_STACK_TO_HC_HCI_CMD:
            if (((rtkbt_h5logfilter & 1) == 0) || (*p != 0x94) || (*(p + 1) != 0xfc)) {
                rtk_btsnoop_write_packet(HCI_COMMAND_PKT, p, true);
            }
            break;
    }
}

void rtk_btsnoop_net_open(void)
{
    rtk_listen_thread_valid_ = (pthread_create(&rtk_listen_thread_, NULL, rtk_listen_fn_, NULL) == 0);
    if (!rtk_listen_thread_valid_) {
        HILOGE("%s pthread_create failed: %s", __func__, strerror(errno));
    } else {
        HILOGD("initialized");
    }
}

void rtk_btsnoop_net_close(void)
{
    if (rtk_listen_thread_valid_) {
        shutdown(rtk_listen_socket_, SHUT_RDWR);
        pthread_join(rtk_listen_thread_, NULL);
        rtk_listen_thread_valid_ = false;
    }
}

void rtk_btsnoop_net_write(serial_data_type_t type, uint8_t *data, bool is_received)
{
    if (rtk_listen_socket_ == -1) {
        return;
    }
    int length = 0;
    uint8_t *p = data;

    switch (type) {
        case HCI_COMMAND_PKT:
            if (((rtkbt_h5logfilter & 1) == 0) || (*p != 0x94) || (*(p + 1) != 0xfc)) {
                length = data[2L] + 3L;
            } else {
                return;
            }
            break;
        case HCI_ACLDATA_PKT:
            length = (data[3L] << 8L) + data[2L] + 4L;
            break;
        case HCI_SCODATA_PKT:
            length = data[2L] + 3L;
            break;
        case HCI_EVENT_PKT:
            if ((*(p + 3L) == 0x94) && (*(p + 4L) == 0xfc) && (*(p + 5L) == 0x00) && (rtkbt_h5logfilter & 1)) {
                return;
            } else {
                length = data[1] + 2L;
            }
            break;
        default:
            break;
    }

    uint8_t buffer[4126] = {0};
    struct sockaddr_in client_addr;
    int i = 0;

#if DATA_DIRECT_2_ELLISY
    uint8_t bit_rate[4] = {0x00, 0x1b, 0x37, 0x4b};
    struct tm *t;
    time_t tt;
    time_t t_ret;
    t_ret = time(&tt);
    t = localtime(&tt);

    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t nano_time =
        (t->tm_hour * 3600LL + t->tm_min * 60LL + t->tm_sec) * 1000LL * 1000LL * 1000LL + tv.tv_usec * 1000LL;
    uint16_t year = (t->tm_year + 1900) & 0xFFFF;
    uint8_t month = (t->tm_mon + 1) & 0xFF;
    uint8_t day = buffer[0] = 0x02;
    buffer[1] = 0x00;
    buffer[2L] = 0x01;
    buffer[3L] = 0x02;
    // time
    (void)memcpy_s(&buffer[4L], sizeof(buffer), &year, 2L);
    buffer[6L] = month;
    buffer[7L] = day;
    (void)memcpy_s(&buffer[8L], sizeof(buffer), &nano_time, 6L);
    // bit rate
    buffer[14L] = 0x80;
    (void)memcpy_s(&buffer[15L], sizeof(buffer), bit_rate, 4L);
    // type
    buffer[19L] = 0x81;
    i = 20L;
#else
    (void)memcpy_s(&buffer[i], sizeof(buffer), &length, sizeof(int));
    i = 4L;
#endif
    switch (type) {
        case HCI_COMMAND_PKT:
            buffer[i] = HCI_COMMAND;
            break;

        case HCI_ACLDATA_PKT:
            if (is_received) {
                buffer[i] = HCI_ACL_DATA_C2H;
            } else {
                buffer[i] = HCI_ACL_DATA_H2C;
            }
            break;

        case HCI_SCODATA_PKT:
            if (is_received) {
                buffer[i] = HCI_SCO_DATA_C2H;
            } else {
                buffer[i] = HCI_SCO_DATA_H2C;
            }
            break;

        case HCI_EVENT_PKT:
            buffer[i] = HCI_EVENT;
            break;

        default:
            buffer[i] = 0;
            break;
    }
#if DATA_DIRECT_2_ELLISY
    buffer[21L] = 0x82;
    i = 22L;
#else
    i = 5L;
#endif
    (void)memcpy_s(&buffer[i], sizeof(buffer), data, length);
    (void)memset_s(&client_addr, sizeof(client_addr), 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(RTK_REMOTEHOST_);
    client_addr.sin_port = htons(RTK_REMOTE_PORT_);
    pthread_mutex_lock(&rtk_client_socket_lock_);
    int ret;
    RTK_NO_INTR(ret = sendto(rtk_listen_socket_, buffer, (length + i), 0, (struct sockaddr *)&client_addr,
                             sizeof(struct sockaddr_in)));
    pthread_mutex_unlock(&rtk_client_socket_lock_);
}

static void *rtk_listen_fn_(void *context)
{
    RTK_UNUSED(context);
    prctl(PR_SET_NAME, (unsigned long)RTK_LISTEN_THREAD_NAME_, 0, 0, 0);

    rtk_listen_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rtk_listen_socket_ == -1) {
        HILOGE("%s socket creation failed: %s", __func__, strerror(errno));
        rtk_safe_close_(&rtk_listen_socket_);
        return NULL;
    }

    struct sockaddr_in addr;
    (void)memset_s(&addr, sizeof(addr), 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(RTK_LOCALHOST_);
    addr.sin_port = htons(RTK_LISTEN_PORT_);

    struct sockaddr_in client_addr;
    (void)memset_s(&client_addr, sizeof(addr), 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(RTK_REMOTEHOST_);
    client_addr.sin_port = htons(RTK_REMOTE_PORT_);

    if (bind(rtk_listen_socket_, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        HILOGE("%s unable to bind listen socket: %s", __func__, strerror(errno));
        rtk_safe_close_(&rtk_listen_socket_);
        return NULL;
    }

    return NULL;
}

static void rtk_safe_close_(int *fd)
{
    assert(fd != NULL);
    if (*fd != -1) {
        close(*fd);
        *fd = -1;
    }
}
