/*
*
* SPDX-License-Identifier: GPL-2.0
*
* Copyright (C) 2011-2018 ARM or its affiliates
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2.
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*/

#include "acamera_types.h"
#include "acamera_firmware_config.h"

extern void sensor_init_imx219( void** ctx, sensor_control_t*, void*) ;
extern void sensor_deinit_imx219( void *ctx );
extern int sensor_detect_imx219( void* sbp);

#define SENSOR_INIT_SUBDEV_FUNCTIONS_IMX219 sensor_init_imx219
#define SENSOR_DEINIT_SUBDEV_FUNCTIONS_IMX219 sensor_deinit_imx219
#define SENSOR_DETECT_FUNCTIONS_IMX219 sensor_detect_imx219
