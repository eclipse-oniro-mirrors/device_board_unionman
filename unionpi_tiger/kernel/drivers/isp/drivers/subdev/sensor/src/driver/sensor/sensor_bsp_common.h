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

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of_gpio.h>

#define IR_CUT_GPIO_MAX_NUM 5   //config platform ir cut gpio max numbers

typedef struct sensor_bringup {
    struct device *dev;
    struct device_node *np;
	int vana;
	int vdig;
	int	power;
	int reset;
	struct clk *mclk;
	int ir_gname[IR_CUT_GPIO_MAX_NUM];  //there is a one-to-one correspondence between ir_gname[5] and ir_cut_gpio of g12b_a311d_skt.dts.
    int ir_gcount;
}sensor_bringup_t;

int sensor_bp_init(sensor_bringup_t* sbp, struct device* dev);
int pwr_am_enable(sensor_bringup_t* sensor_bp, const char* propname, int val);
int pwr_am_disable(sensor_bringup_t* sensor_bp);
int pwr_ir_cut_enable(sensor_bringup_t* sensor_bp, int propname, int val);
int reset_am_enable(sensor_bringup_t* sensor_bp, const char* propname, int val);
int reset_am_disable(sensor_bringup_t *sensor_bp);
int clk_am_enable(sensor_bringup_t* sensor_bp, const char* propname);
int gp_pl_am_enable(sensor_bringup_t* sensor_bp, const char* propname, uint32_t rate);
int clk_am_disable(sensor_bringup_t *sensor_bp);