/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef HARDWARE_UART_H
#define HARDWARE_UART_H

void hw_config_cback(void *p_evt_buf);
void hw_config_start(char transtype);

#endif
