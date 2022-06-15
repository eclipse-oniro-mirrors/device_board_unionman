/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>
#include "sound/soc.h"
#include "sound/tlv.h"

#include "meson_t9015.h"

#define BLOCK_EN    0x00
#define  LORN_EN    0
#define  LORP_EN    1
#define  LOLN_EN    2
#define  LOLP_EN    3
#define  DACR_EN    4
#define  DACL_EN    5
#define  DACR_INV   20
#define  DACL_INV   21
#define  DACR_SRC   22
#define  DACL_SRC   23
#define  REFP_BUF_EN        BIT(12)
#define  BIAS_CURRENT_EN    BIT(13)
#define  VMID_GEN_FAST  BIT(14)
#define  VMID_GEN_EN    BIT(15)
#define  I2S_MODE       BIT(30)
#define VOL_CTRL0       0x04
#define  GAIN_H     31
#define  GAIN_L     23
#define VOL_CTRL1   0x08
#define  DAC_MONO   8
#define  RAMP_RATE  10
#define  VC_RAMP_MODE   12
#define  MUTE_MODE      13
#define  UNMUTE_MODE    14
#define  DAC_SOFT_MUTE  15
#define  DACR_VC    16
#define  DACL_VC    24
#define LINEOUT_CFG 0x0c
#define  LORN_POL   0
#define  LORP_POL   4
#define  LOLN_POL   8
#define  LOLP_POL   12
#define POWER_CFG   0x10

struct t9015 {
    struct device *dev;
    struct clk *pclk;
    struct regulator *avdd;
    struct regmap *map;
    unsigned int fmt;
};

struct t9015 *g_t9015 = NULL;

static int t9015_update_bits(unsigned int reg, unsigned int mask, unsigned int val)
{
    int ret = -1;

    if (g_t9015) {
        ret = regmap_update_bits(g_t9015->map, reg, mask, val);
    }

    pr_info("%s(reg=0x%x, mask=0x%x, val=0x%x): %d\n",
            __FUNCTION__, reg, mask, val, ret);

    return ret;
}

int t9015_start(void)
{
    int ret;
    unsigned int val;

    if (!g_t9015) {
        return -1;
    }

    ret = regulator_enable(g_t9015->avdd);
    if (ret) {
        dev_err(g_t9015->dev, "AVDD enable failed\n");
        return ret;
    }

    // bias standby
    t9015_update_bits(BLOCK_EN,
                      VMID_GEN_EN | VMID_GEN_FAST | REFP_BUF_EN,
                      VMID_GEN_EN | VMID_GEN_FAST | REFP_BUF_EN);

    mdelay(200U);
    t9015_update_bits(BLOCK_EN,
                      VMID_GEN_FAST,
                      0);

    // bias prepare
    t9015_update_bits(BLOCK_EN,
                      BIAS_CURRENT_EN,
                      0);

    // set dai fmt
    val = ((g_t9015->fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS) ? 0 : I2S_MODE;
    t9015_update_bits(BLOCK_EN,
                      I2S_MODE,
                      val);

    // Right/Left DAC
    t9015_update_bits(BLOCK_EN,
                      BIT(DACR_EN) | BIT(DACL_EN),
                      BIT(DACR_EN) | BIT(DACL_EN));

    // Right-, Left-, Right+, Left+ Driver
    t9015_update_bits(BLOCK_EN,
                      BIT(LORN_EN) | BIT(LORP_EN) | BIT(LOLN_EN) | BIT(LOLP_EN),
                      BIT(LORN_EN) | BIT(LORP_EN) | BIT(LOLN_EN) | BIT(LOLP_EN));

    // bias On
    t9015_update_bits(BLOCK_EN,
                      BIAS_CURRENT_EN,
                      BIAS_CURRENT_EN);

    return 0;
}

int t9015_stop(void)
{
    if (!g_t9015) {
        return -1;
    }

    // bias prepare
    t9015_update_bits(BLOCK_EN,
                      BIAS_CURRENT_EN,
                      0);

    // Right-, Left-, Right+, Left+ Driver
    t9015_update_bits(BLOCK_EN,
                      BIT(LORN_EN) | BIT(LORP_EN) | BIT(LOLN_EN) | BIT(LOLP_EN),
                      0);

    // Right/Left DAC
    t9015_update_bits(BLOCK_EN,
                      BIT(DACR_EN) | BIT(DACL_EN),
                      0);

    // bias Off
    t9015_update_bits(BLOCK_EN,
                      VMID_GEN_EN | VMID_GEN_FAST | REFP_BUF_EN,
                      0);

    regulator_disable(g_t9015->avdd);

    return 0;
}

int meson_t9015_dai_set_fmt(unsigned int fmt)
{
    if (!g_t9015) {
        return -1;
    }

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBM_CFM:
            break;
        case SND_SOC_DAIFMT_CBS_CFS:
            break;
        default:
            return -EINVAL;
    }

    if (((fmt & SND_SOC_DAIFMT_FORMAT_MASK) != SND_SOC_DAIFMT_I2S) &&
        ((fmt & SND_SOC_DAIFMT_FORMAT_MASK) != SND_SOC_DAIFMT_LEFT_J)) {
        return -EINVAL;
    }

    g_t9015->fmt = fmt;

    dev_info(g_t9015->dev, "%s(), set fmt %u SUCCESS.\n", __FUNCTION__, fmt);

    return 0;
}

int meson_t9015_volume_set(unsigned int vol_left, unsigned int vol_right)
{
    unsigned int mask, val;
    if (vol_left > T9015_VOLUME_MAX || vol_right > T9015_VOLUME_MAX) {
        dev_err(g_t9015->dev, "invalid volume: %u, %u\n", vol_left, vol_right);
        return -EINVAL;
    }

    mask = (0xff << DACL_VC) | (0xff << DACR_VC);
    val = ((vol_left & 0xff) << DACL_VC) | ((vol_right & 0xff) << DACR_VC);
    return t9015_update_bits(VOL_CTRL1, mask, val);
}

int meson_t9015_mute_set(bool mute)
{
    unsigned int mask, val;
    mask = 0x1 << DAC_SOFT_MUTE;
    val = (!!mute) << DAC_SOFT_MUTE;
    return t9015_update_bits(VOL_CTRL1, mask, val);
}

int meson_t9015_enable(bool enable)
{
    int ret;

    if (enable) {
        ret = t9015_start();
    } else {
        ret = t9015_stop();
    }

    return ret;
}

static const struct regmap_config t9015_regmap_config = {
    .reg_bits       = 32,
    .reg_stride     = 4,
    .val_bits       = 32,
    .max_register   = POWER_CFG,
};

static int t9015_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct t9015 *priv;
    struct regmap *regmap;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, priv);

    priv->pclk = devm_clk_get(dev, "pclk");
    if (IS_ERR(priv->pclk)) {
        if (PTR_ERR(priv->pclk) != -EPROBE_DEFER) {
            dev_err(dev, "failed to get core clock\n");
        }
        return PTR_ERR(priv->pclk);
    }

    priv->avdd = devm_regulator_get(dev, "AVDD");
    if (IS_ERR(priv->avdd)) {
        if (PTR_ERR(priv->avdd) != -EPROBE_DEFER) {
            dev_err(dev, "failed to AVDD\n");
        }
        return PTR_ERR(priv->avdd);
    }

    ret = clk_prepare_enable(priv->pclk);
    if (ret) {
        dev_err(dev, "core clock enable failed\n");
        return ret;
    }

    ret = devm_add_action_or_reset(dev, (void (*)(void *))clk_disable_unprepare, priv->pclk);
    if (ret) {
        return ret;
    }

    ret = device_reset(dev);
    if (ret) {
        dev_err(dev, "reset failed\n");
        return ret;
    }

    regmap = devm_regmap_init_mmio(dev, devm_platform_ioremap_resource(pdev, 0),
                                   &t9015_regmap_config);
    if (IS_ERR(regmap)) {
        dev_err(dev, "regmap init failed\n");
        return PTR_ERR(regmap);
    }
    priv->map = regmap;

    priv->fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS; // 0x4001

    priv->dev = dev;

    /*
     * Initialize output polarity:
     * ATM the output polarity is fixed but in the future it might useful
     * to add DT property to set this depending on the platform needs
     */
    regmap_write(regmap, LINEOUT_CFG, 0x1111);

    g_t9015 = priv;

    dev_info(dev, "t9015_probe SUCCESS.\n");
    return 0;
}

static const struct of_device_id t9015_ids[] = {
    { .compatible = "amlogic,t9015", },
    {}
};
MODULE_DEVICE_TABLE(of, t9015_ids);

static struct platform_driver t9015_driver = {
    .driver = {
        .name = "t9015-codec",
        .of_match_table = of_match_ptr(t9015_ids),
    },
    .probe = t9015_probe,
};

module_platform_driver(t9015_driver);

