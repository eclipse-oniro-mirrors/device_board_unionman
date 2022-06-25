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
 *	Module Name:
 *	    rtk_parse.h
 *
 *	Abstract:
 *	    Contains wifi-bt coex functions implemented by bluedroid stack
 *
 *	Major Change History:
 *	      When             Who       What
 *	    ---------------------------------------------------------------
 *	    2015-12-15      lamparten   modified
 *	    2014-10-23       kyle_xu    modified
 *
 *	Notes:
 *         This is designed for wifi-bt Coex in Android 6.0.
 *
 ******************************************************************************/

#ifndef RTK_PARSE_H
#define RTK_PARSE_H

#pragma once

#include <stdlib.h>
#include "bt_vendor_rtk.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/
#define HOST_PROFILE_INFO
#define RTKPARSE_NO_INTR(fn)                                                                                           \
    do {                                                                                                               \
    } while ((fn) == -1 && errno == EINTR)
/******************************************************************************
**  Type definitions
******************************************************************************/;
#define BD_ADDR_LEN 6               /* Device address length */
typedef struct BD_ADDR {unsigned char x[BD_ADDR_LEN];} BD_ADDR; /* Device address */
typedef void (*tINT_CMD_CBACK)(void *p_mem);

/******************************************************************************
**  Extern variables and functions
******************************************************************************/

void set_coex_log_onoff(long int coex_log_onoff);

/******************************************************************************
**  Functions
******************************************************************************/
typedef struct rtk_parse_manager_t {
    void (*rtk_parse_internal_event_intercept)(uint8_t *p);

    void (*rtk_parse_l2cap_data)(uint8_t *p, uint8_t direction);

    void (*rtk_parse_init)(void);

    void (*rtk_parse_cleanup)(void);

    void (*rtk_parse_command)(uint8_t *p);

    void (*rtk_add_le_profile)(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map);

    void (*rtk_delete_le_profile)(BD_ADDR bdaddr, uint16_t handle, uint8_t profile_map);

    void (*rtk_add_le_data_count)(uint8_t data_type);

    void (*rtk_set_bt_on)(uint8_t bt_on);
} rtk_parse_manager_t;

rtk_parse_manager_t *rtk_parse_manager_get_interface(void);

#ifdef __LITTLE_ENDIAN
struct sbc_frame_hdr {
    uint8_t syncword : 8;           /* Sync word */
    uint8_t subbands : 1;           /* Subbands */
    uint8_t allocation_method : 1;  /* Allocation method */
    uint8_t channel_mode : 2;       /* Channel mode */
    uint8_t blocks : 2;             /* Blocks */
    uint8_t sampling_frequency : 2; /* Sampling frequency */
    uint8_t bitpool : 8;            /* Bitpool */
    uint8_t crc_check : 8;          /* CRC check */
} __attribute__((packed));

/* NOTE: The code is copied from pa.
 * only the bit field in 8-bit is affected by endian, not the 16-bit or 32-bit.
 * why?
 */
struct rtp_header {
    unsigned cc : 4;
    unsigned x : 1;
    unsigned p : 1;
    unsigned v : 2;

    unsigned pt : 7;
    unsigned m : 1;

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[0];
} __attribute__((packed));

#else
/* big endian */
struct sbc_frame_hdr {
    uint8_t syncword : 8;           /* Sync word */
    uint8_t sampling_frequency : 2; /* Sampling frequency */
    uint8_t blocks : 2;             /* Blocks */
    uint8_t channel_mode : 2;       /* Channel mode */
    uint8_t allocation_method : 1;  /* Allocation method */
    uint8_t subbands : 1;           /* Subbands */
    uint8_t bitpool : 8;            /* Bitpool */
    uint8_t crc_check : 8;          /* CRC check */
} __attribute__((packed));

struct rtp_header {
    unsigned v : 2;
    unsigned p : 1;
    unsigned x : 1;
    unsigned cc : 4;

    unsigned m : 1;
    unsigned pt : 7;

    uint16_t sequence_number;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[0];
} __attribute__((packed));
#endif /* __LITTLE_ENDIAN */

void hw_process_event(HC_BT_HDR *p_buf);
void rtk_vendor_cmd_to_fw(uint16_t opcode, uint8_t parameter_len, uint8_t *parameter, tINT_CMD_CBACK p_cback);

#endif /* RTK_PARSE_H */
