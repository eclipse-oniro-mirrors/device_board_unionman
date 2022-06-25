/*
 * Copyright (c) 2021 Unionman Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include "sound/soc.h"
#include "sound/soc-dai.h"

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
    .field_threshold    = REG_FIELD(FIFO_CTRL1, 16, 23),
    .is_g12a        = 0,
    .is_frddr       = 0
};

static const struct axg_fifo_match_data g12a_toddr_match_data = {
    .field_threshold    = REG_FIELD(FIFO_CTRL1, 16, 23),
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