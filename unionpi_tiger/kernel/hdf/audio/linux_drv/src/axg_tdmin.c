/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include "sound/soc.h"
#include "sound/soc-dai.h"

#include "axg_tdm_formatter.h"
#include "axg_tdm.h"

#define TDMIN_SRC_IN_SEL_MAX    15

#define TDMIN_CTRL              0x00
#define  TDMIN_CTRL_ENABLE      BIT(31)
#define  TDMIN_CTRL_I2S_MODE    BIT(30)
#define  TDMIN_CTRL_RST_OUT     BIT(29)
#define  TDMIN_CTRL_RST_IN      BIT(28)
#define  TDMIN_CTRL_WS_INV      BIT(25)
#define  TDMIN_CTRL_SEL_SHIFT   20
#define  TDMIN_CTRL_IN_BIT_SKEW_MASK    GENMASK(18, 16)
#define  TDMIN_CTRL_IN_BIT_SKEW(x)  ((x) << 16)
#define  TDMIN_CTRL_LSB_FIRST       BIT(5)
#define  TDMIN_CTRL_BITNUM_MASK     GENMASK(4, 0)
#define  TDMIN_CTRL_BITNUM(x)       ((x) << 0)
#define TDMIN_SWAP              0x04
#define TDMIN_MASK0             0x08
#define TDMIN_MASK1             0x0c
#define TDMIN_MASK2             0x10
#define TDMIN_MASK3             0x14
#define TDMIN_STAT              0x18
#define TDMIN_MUTE_VAL          0x1c
#define TDMIN_MUTE0             0x20
#define TDMIN_MUTE1             0x24
#define TDMIN_MUTE2             0x28
#define TDMIN_MUTE3             0x2c

static const struct regmap_config axg_tdmin_regmap_cfg = {
    .reg_bits   = 32,
    .val_bits   = 32,
    .reg_stride = 4,
    .max_register   = TDMIN_MUTE3,
};

static void axg_tdmin_enable(struct regmap *map)
{
    /* Apply both reset */
    regmap_update_bits(map, TDMIN_CTRL,
            TDMIN_CTRL_RST_OUT | TDMIN_CTRL_RST_IN, 0);

    /* Clear out reset before in reset */
    regmap_update_bits(map, TDMIN_CTRL,
            TDMIN_CTRL_RST_OUT, TDMIN_CTRL_RST_OUT);
    regmap_update_bits(map, TDMIN_CTRL,
            TDMIN_CTRL_RST_IN,  TDMIN_CTRL_RST_IN);

    /* Actually enable tdmin */
    regmap_update_bits(map, TDMIN_CTRL,
            TDMIN_CTRL_ENABLE, TDMIN_CTRL_ENABLE);
}

static void axg_tdmin_disable(struct regmap *map)
{
    regmap_update_bits(map, TDMIN_CTRL, TDMIN_CTRL_ENABLE, 0);
}

static int axg_tdmin_src_in_sel(struct regmap *map, unsigned int index)
{
    unsigned int mask, val;

    if (index > TDMIN_SRC_IN_SEL_MAX) {
        pr_err("%s, invalid param: index=%u\n", __FUNCTION__, index);
        return -1;
    }

    mask = 0xf << TDMIN_CTRL_SEL_SHIFT;
    val = index << TDMIN_CTRL_SEL_SHIFT;
    regmap_update_bits(map, TDMIN_CTRL, mask, val);
    pr_info("%s, update_bits: reg=0x%x, mask=0x%x, val=0x%x\n",
            __FUNCTION__, TDMIN_CTRL, mask, val);
    return 0;
}

static int axg_tdmin_prepare(struct regmap *map,
                             const struct axg_tdm_formatter_hw *quirks,
                             struct axg_tdm_stream *ts)
{
    unsigned int val, skew = quirks->skew_offset;

    /* Set stream skew */
    switch (ts->iface->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
        case SND_SOC_DAIFMT_DSP_A:
            skew += 1;
            break;
        case SND_SOC_DAIFMT_LEFT_J:
        case SND_SOC_DAIFMT_DSP_B:
            break;
        default:
            pr_err("Unsupported format: %u\n",
                   ts->iface->fmt & SND_SOC_DAIFMT_FORMAT_MASK);
            return -EINVAL;
    }

    val = TDMIN_CTRL_IN_BIT_SKEW(skew);

    /* Set stream format mode */
    switch (ts->iface->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
        case SND_SOC_DAIFMT_LEFT_J:
        case SND_SOC_DAIFMT_RIGHT_J:
            val |= TDMIN_CTRL_I2S_MODE;
            break;
        default:
            break;
    }

    /* If the sample clock is inverted, invert it back for the formatter */
    if (axg_tdm_lrclk_invert(ts->iface->fmt)) {
        val |= TDMIN_CTRL_WS_INV;
    }

    /* Set the slot width */
    val |= TDMIN_CTRL_BITNUM(ts->iface->slot_width - 1);

    /*
     * The following also reset LSB_FIRST which result in the formatter
     * placing the first bit received at bit 31
     */
    regmap_update_bits(map, TDMIN_CTRL,
                       (TDMIN_CTRL_IN_BIT_SKEW_MASK | TDMIN_CTRL_WS_INV |
                        TDMIN_CTRL_I2S_MODE | TDMIN_CTRL_LSB_FIRST |
                        TDMIN_CTRL_BITNUM_MASK),
                       val);

    /* Set static swap mask configuration */
    regmap_write(map, TDMIN_SWAP, 0x76543210);

    return axg_tdm_formatter_set_channel_masks(map, ts, TDMIN_MASK0);
}

static const struct axg_tdm_formatter_ops axg_tdmin_ops = {
    .src_in_sel = axg_tdmin_src_in_sel,
    .prepare    = axg_tdmin_prepare,
    .enable     = axg_tdmin_enable,
    .disable    = axg_tdmin_disable,
};

static const struct axg_tdm_formatter_driver axg_tdmin_drv = {
    .regmap_cfg = &axg_tdmin_regmap_cfg,
    .ops        = &axg_tdmin_ops,
    .quirks     = &(const struct axg_tdm_formatter_hw) {
        .skew_offset = 3,
    },
};

static const struct of_device_id axg_tdmin_of_match[] = {
    {
        .compatible = "amlogic,axg-tdmin",
        .data = &axg_tdmin_drv,
    }, {
        .compatible = "amlogic,g12a-tdmin",
        .data = &axg_tdmin_drv,
    }, {
        .compatible = "amlogic,sm1-tdmin",
        .data = &axg_tdmin_drv,
    }, {}
};
MODULE_DEVICE_TABLE(of, axg_tdmin_of_match);

static struct platform_driver axg_tdmin_pdrv = {
    .probe = axg_tdm_formatter_probe,
    .driver = {
        .name = "axg-tdmin",
        .of_match_table = axg_tdmin_of_match,
    },
};
module_platform_driver(axg_tdmin_pdrv);

