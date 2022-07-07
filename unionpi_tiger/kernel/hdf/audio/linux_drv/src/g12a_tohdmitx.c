/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include "sound/pcm_params.h"
#include "sound/soc.h"
#include "sound/soc-dai.h"

#include "dt-bindings/sound/meson-g12a-tohdmitx.h"

#include "g12a_tohdmitx.h"

#define G12A_TOHDMITX_DRV_NAME "g12a-tohdmitx-shadow"

#define TOHDMITX_CTRL0              0x0
#define  CTRL0_ENABLE_SHIFT         31
#define  CTRL0_I2S_DAT_SEL_SHIFT    12
#define  CTRL0_I2S_DAT_SEL          (0x3 << CTRL0_I2S_DAT_SEL_SHIFT)
#define  CTRL0_I2S_LRCLK_SEL        GENMASK(9, 8)
#define  CTRL0_I2S_BLK_CAP_INV      BIT(7)
#define  CTRL0_I2S_BCLK_O_INV       BIT(6)
#define  CTRL0_I2S_BCLK_SEL         GENMASK(5, 4)
#define  CTRL0_SPDIF_CLK_CAP_INV    BIT(3)
#define  CTRL0_SPDIF_CLK_O_INV	    BIT(2)
#define  CTRL0_SPDIF_SEL_SHIFT      1
#define  CTRL0_SPDIF_SEL            (0x1 << CTRL0_SPDIF_SEL_SHIFT)
#define  CTRL0_SPDIF_CLK_SEL        BIT(0)

static struct regmap *g_regmap = NULL;

static int tohdmitx_update_bits(unsigned int reg, unsigned int mask, unsigned int val)
{
    int ret = -1;

    if (g_regmap) {
        ret = regmap_update_bits(g_regmap, reg, mask, val);
    }

    pr_info("%s(reg=0x%x, mask=0x%x, val=0x%x): %d\n",
            __FUNCTION__, reg, mask, val, ret);

    return ret;
}

int meson_g12a_tohdmitx_enable(bool enable)
{
    unsigned int val;
    val = enable ? (1<<CTRL0_ENABLE_SHIFT) : 0;
    return tohdmitx_update_bits(TOHDMITX_CTRL0, 1<<CTRL0_ENABLE_SHIFT, val);
}

int meson_g12a_tohdmitx_src_sel(enum g12a_tohdmitx_src src)
{
    int ret;
    unsigned int mux = src;

    ret = tohdmitx_update_bits(TOHDMITX_CTRL0,
                               CTRL0_I2S_DAT_SEL |
                                   CTRL0_I2S_LRCLK_SEL |
                                   CTRL0_I2S_BCLK_SEL,
                               FIELD_PREP(CTRL0_I2S_DAT_SEL, mux) |
                                   FIELD_PREP(CTRL0_I2S_LRCLK_SEL, mux) |
                                   FIELD_PREP(CTRL0_I2S_BCLK_SEL, mux));

    return ret;
}

static const struct regmap_config g12a_tohdmitx_regmap_cfg = {
    .reg_bits   = 32,
    .val_bits   = 32,
    .reg_stride = 4,
};

static const struct of_device_id g12a_tohdmitx_of_match[] = {
    { .compatible = "amlogic,g12a-tohdmitx", },
    {}
};
MODULE_DEVICE_TABLE(of, g12a_tohdmitx_of_match);

static int g12a_tohdmitx_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct regmap *map;
    int ret;

    ret = device_reset(dev);
    if (ret) {
        return ret;
    }
    
    map = devm_regmap_init_mmio(dev, devm_platform_ioremap_resource(pdev, 0),
                                &g12a_tohdmitx_regmap_cfg);
    if (IS_ERR(map)) {
        dev_err(dev, "failed to init regmap: %ld\n",
        PTR_ERR(map));
        return PTR_ERR(map);
    }
    g_regmap = map;
    
    ret = regmap_write(map, TOHDMITX_CTRL0,
                CTRL0_I2S_BLK_CAP_INV | CTRL0_SPDIF_CLK_CAP_INV);
    if (ret) {
        dev_err(dev, "init register failed.\n");
    }
    
    return ret;
}

static struct platform_driver g12a_tohdmitx_pdrv = {
    .driver = {
        .name = G12A_TOHDMITX_DRV_NAME,
        .of_match_table = g12a_tohdmitx_of_match,
    },
    .probe = g12a_tohdmitx_probe,
};
module_platform_driver(g12a_tohdmitx_pdrv);
