/******************************************************************************
 *
 *  Copyright (C) 2009-2018 Realtek Corporation.
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
 *  Filename:      poll.c
 *
 *  Description:   Contains host & controller handshake implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_poll"

#include <utils/Log.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "bt_hci_bdroid.h"
#include "rtk_poll.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#ifndef BTPOLL_DBG
#define BTPOLL_DBG false
#endif

#if (BTPOLL_DBG == true)
#define BTPOLLDBG(param, ...)                                                                                          \
    {                                                                                                                  \
        HILOGD(param, ##__VA_ARGS__);                                                                                  \
    }
#else
#define BTPOLLDBG(param, ...)                                                                                          \
    {                                                                                                                  \
    }
#endif

#ifndef ENABLE_BT_POLL_IN_ACTIVE_MODE
#define ENABLE_BT_POLL_IN_ACTIVE_MODE false
#endif

#ifndef DEFAULT_POLL_IDLE_TIMEOUT
#define DEFAULT_POLL_IDLE_TIMEOUT 2500
#endif

volatile uint32_t rtkbt_heartbeat_noack_num = 0;
volatile uint32_t rtkbt_heartbeat_evt_seqno = 0xffffffff;

timed_out poll_idle_timeout;

/******************************************************************************
**  Externs
******************************************************************************/

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Poll state */
enum {
    POLL_DISABLED = 0, /* initial state */
    POLL_ENABLED,
};

/* poll control block */
typedef struct {
    uint8_t state; /* poll state */
    uint8_t timer_created;
    timer_t timer_id;
    uint32_t timeout_ms;
} bt_poll_cb_t;

/******************************************************************************
**  Static variables
******************************************************************************/

static bt_poll_cb_t bt_poll_cb;

/******************************************************************************
**   Poll Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         poll_timer_stop
**
** Description      stop timer if allowed
**
** Returns          None
**
*******************************************************************************/
static void poll_timer_stop(void)
{
    int status;
    struct itimerspec ts;

    HILOGI("poll_timer_stop: timer_created %d", bt_poll_cb.timer_created);

    if (bt_poll_cb.timer_created == true) {
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(bt_poll_cb.timer_id, 0, &ts, 0);
        if (status == -1) {
            HILOGE("[STOP] Failed to set poll idle timeout");
        }
    }
}

/*****************************************************************************
**   POLL Interface Functions
*****************************************************************************/

/*******************************************************************************
**
** Function        poll_init
**
** Description     Init bt poll
**
** Returns         None
**
*******************************************************************************/
void poll_init(timed_out ptr_timeout, uint32_t timeout)
{
    (void)memset_s((void *)&bt_poll_cb, sizeof(bt_poll_cb_t), 0, sizeof(bt_poll_cb_t));
    poll_idle_timeout = ptr_timeout;
    bt_poll_cb.state = POLL_DISABLED;
    bt_poll_cb.timeout_ms = timeout;

    HILOGI("poll_init: state %d, timeout %d ms,timeout=%d", bt_poll_cb.state, bt_poll_cb.timeout_ms, timeout);
}

/*******************************************************************************
**
** Function        poll_cleanup
**
** Description     Poll clean up
**
** Returns         None
**
*******************************************************************************/
void poll_cleanup(void)
{
    HILOGI("poll_cleanup: timer_created %d", bt_poll_cb.timer_created);

    if (bt_poll_cb.timer_created == true) {
        timer_delete(bt_poll_cb.timer_id);
    }
}

/*******************************************************************************
**
** Function        poll_enable
**
** Description     Enalbe/Disable poll
**
** Returns         None
**
*******************************************************************************/
void poll_enable(uint8_t turn_on)
{
    HILOGI("poll_enable: turn_on %d, state %d", turn_on, bt_poll_cb.state);

    if ((turn_on == true) && (bt_poll_cb.state == POLL_ENABLED)) {
        HILOGI("poll_enable: poll is already on!!!");
        return;
    } else if ((turn_on == false) && (bt_poll_cb.state == POLL_DISABLED)) {
        HILOGI("poll_enable: poll is already off!!!");
        return;
    }

    if (turn_on == false) {
        poll_timer_stop();
        bt_poll_cb.state = POLL_DISABLED;
    } else {
        /* start poll timer when poll_timer_flush invoked first time */
        bt_poll_cb.state = POLL_ENABLED;
    }
}

/*******************************************************************************
**
** Function        poll_timer_flush
**
** Description     Called to delay notifying Bluetooth chip.
**                 Normally this is called when there is data to be sent
**                 over HCI.
**
** Returns         None
**
*******************************************************************************/
void poll_timer_flush(void)
{
    int status;
    struct itimerspec ts;
    struct sigevent se;

    (void)memset_s(&se, sizeof(struct sigevent), 0, sizeof(struct sigevent));
    BTPOLLDBG("poll_timer_flush: state %d", bt_poll_cb.state);

    if (bt_poll_cb.state != POLL_ENABLED) {
        return;
    }

    if (bt_poll_cb.timer_created == false) {
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = &bt_poll_cb.timer_id;
        se.sigev_notify_function = poll_idle_timeout;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_MONOTONIC, &se, &bt_poll_cb.timer_id);
        if (status == 0) {
            bt_poll_cb.timer_created = true;
        }
    }
#if (defined(ENABLE_BT_POLL_IN_ACTIVE_MODE) && (ENABLE_BT_POLL_IN_ACTIVE_MODE == false))
    if (bt_poll_cb.timer_created == true) {
        ts.it_value.tv_sec = bt_poll_cb.timeout_ms / 1000L;
        ts.it_value.tv_nsec = 1000L * 1000L * (bt_poll_cb.timeout_ms % 1000L);
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        status = timer_settime(bt_poll_cb.timer_id, 0, &ts, 0);
        if (status == -1) {
            HILOGE("[Flush] Failed to set poll idle timeout");
        }
    }
#endif
}
