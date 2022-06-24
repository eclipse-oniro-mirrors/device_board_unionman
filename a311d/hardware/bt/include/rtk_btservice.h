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
#ifndef RTK_BTSERVICE_H
#define RTK_BTSERVICE_H

#include <stdio.h>
#include <utils/Log.h>

#define HCI_RTKBT_AUTOPAIR_EVT 0x30

typedef struct Rtk_Service_Data {
    uint16_t opcode;
    uint8_t parameter_len;
    uint8_t *parameter;
    void (*complete_cback)(void *);
} Rtk_Service_Data;


void RTK_btservice_destroyed(void);
void Rtk_Service_Vendorcmd_Hook(Rtk_Service_Data *RtkData, int client_sock);
int RTK_btservice_init(void);

#endif
