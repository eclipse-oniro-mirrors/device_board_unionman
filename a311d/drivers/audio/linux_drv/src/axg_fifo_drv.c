// SPDX-License-Identifier: (GPL-2.0 OR MIT)
//
// Copyright (c) 2018 BayLibre, SAS.
// Author: Jerome Brunet <jbrunet@baylibre.com>

/*
 * This driver implements the frontend playback DAI of AXG and G12A based SoCs
 */

#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <sound/soc.h>
#include <sound/soc-dai.h>

#include "axg_fifo.h"

static const struct axg_fifo_match_data axg_frddr_match_data = {
    .field_threshold    = REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 0,
    .is_frddr       = 1
};

static const struct axg_fifo_match_data g12a_frddr_match_data = {
    .field_threshold    = REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 1,
    .is_frddr       = 1
};

static const struct axg_fifo_match_data sm1_frddr_match_data = {
    .field_threshold    = REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 1,
    .is_frddr       = 1
};

static const struct axg_fifo_match_data axg_toddr_match_data = {
    .field_threshold	= REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 0,
    .is_frddr       = 0
};

static const struct axg_fifo_match_data g12a_toddr_match_data = {
    .field_threshold	= REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 1,
    .is_frddr       = 0
};

static const struct axg_fifo_match_data sm1_toddr_match_data = {
    .field_threshold	= REG_FIELD(FIFO_CTRL1, 12, 23),
    .is_g12a        = 1,
    .is_frddr       = 0
};

static const struct of_device_id axg_fifo_of_match[] = {
    {
        .compatible = "amlogic,axg-frddr",
        .data = &axg_frddr_match_data,
    }, {
        .compatible = "amlogic,g12a-frddr",
        .data = &g12a_frddr_match_data,
    }, {
        .compatible = "amlogic,sm1-frddr",
        .data = &sm1_frddr_match_data,
    },
    {
        .compatible = "amlogic,axg-toddr",
        .data = &axg_toddr_match_data,
    }, {
        .compatible = "amlogic,g12a-toddr",
        .data = &g12a_toddr_match_data,
    }, {
        .compatible = "amlogic,sm1-toddr",
        .data = &sm1_toddr_match_data,
    }, {}
};
MODULE_DEVICE_TABLE(of, axg_fifo_of_match);

static struct platform_driver axg_fifo_pdrv = {
    .probe = meson_axg_fifo_probe,
    .driver = {
        .name = "axg-fifo-drv",
        .of_match_table = axg_fifo_of_match,
    },
};
module_platform_driver(axg_fifo_pdrv);