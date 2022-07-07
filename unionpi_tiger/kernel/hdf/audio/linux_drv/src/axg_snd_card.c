/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include "sound/soc.h"

#include "axg_fifo.h"
#include "axg_tdm.h"
#include "g12a_toacodec.h"
#include "g12a_tohdmitx.h"
#include "axg_tdm_interface.h"
#include "axg_fifo.h"
#include "meson_t9015.h"
#include "nau8540.h"

#include "axg_snd_card.h"

#define DAI_FORMAT_DEFAULT              (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS)
static uint32_t tx_mask[4] = {3, 0, 3, 3};
static uint32_t rx_mask[4] = {0, 3, 0, 0};

struct axg_snd_card {
    struct device *dev;
    const char *name;
    struct axg_fifo *frddr;
    struct axg_fifo *toddr;
    struct axg_tdm_iface *tdm_ifcae;
    bool inited;
};

static struct axg_snd_card *g_snd_card = NULL;

struct axg_tdm_iface *meson_axg_default_tdm_iface_get(void)
{
    if (!g_snd_card || !g_snd_card->inited) {
        return NULL;
    }

    return g_snd_card->tdm_ifcae;
}

struct axg_fifo *meson_axg_default_fifo_get(int dir)
{
    if (!g_snd_card || !g_snd_card->inited) {
        return NULL;
    }

    return dir ? g_snd_card->toddr : g_snd_card->frddr;
}

int meson_axg_snd_card_init(void)
{
    struct axg_fifo *frddr_b, *toddr_b;
    struct axg_tdm_iface *tdm_b;
    int ret;
    uint32_t slots = 2;

    if (!g_snd_card) {
        return -1;
    }

    if (g_snd_card->inited) {
        return 0;
    }

    frddr_b = meson_axg_fifo_get("FRDDR_B");
    toddr_b = meson_axg_fifo_get("TODDR_B");
    tdm_b = meson_axg_tdm_iface_get("TDM_B");
    if (!frddr_b || !toddr_b || !tdm_b) {
        pr_err("%s: get fifo&iface failed.", __FUNCTION__);
        return -1;
    }

    ret = meson_g12a_toacodec_enable(false);

    ret |= meson_g12a_tohdmitx_enable(false);

    ret |= meson_t9015_dai_set_fmt(DAI_FORMAT_DEFAULT);

    ret |= nau8540_set_fmt(DAI_FORMAT_DEFAULT);

    ret |= nau8540_set_tdm_slot(0x3, 0x3, slots, 0);

    // "TDMOUT_B SRC SEL" = "IN 1";   "TDMIN_B SRC SEL" = "IN 1"
    ret |= meson_axg_tdm_iface_init(tdm_b, "TDMOUT_B", 1, "TDMIN_B", 1);

    ret |= meson_axg_tdm_iface_set_fmt(tdm_b, DAI_FORMAT_DEFAULT);

    ret |= meson_axg_tdm_iface_set_tdm_slots(tdm_b, tx_mask, rx_mask, slots, 0);

    // "TOACODEC SRC" = "I2S B"
    ret |= meson_g12a_toacodec_src_sel(G12G_TOACODEC_SRC_I2S_B);

    // "TOHDMITX I2S SRC" = "I2S B"
    ret |= meson_g12a_tohdmitx_src_sel(G12G_TOHDMITX_SRC_I2S_B);

    // "FRDDR_B SINK 1 SEL" = "OUT 1"
    ret |= meson_axg_fifo_update_bits(frddr_b, FIFO_CTRL0, CTRL0_SEL_MASK, 0x1);

    // "FRDDR_B SRC 1 EN Switch" = "1"
    ret |= meson_axg_fifo_update_bits(frddr_b, FIFO_CTRL0, 0x8, 0x8);

    // "TODDR_B SRC SEL" = "IN 1"
    ret |= meson_axg_fifo_update_bits(toddr_b, FIFO_CTRL0, CTRL0_SEL_MASK, 0x1);
    if (ret) {
        pr_err("%s: failed.\n", __FUNCTION__);
        return ret;
    }

    g_snd_card->frddr = frddr_b;
    g_snd_card->toddr = toddr_b;
    g_snd_card->tdm_ifcae = tdm_b;

    g_snd_card->inited = true;

    pr_info("%s: SUCCESS.\n", __FUNCTION__);
    return 0;
}

static int meson_card_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct axg_snd_card *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, priv);

    priv->dev = dev;

    ret = of_property_read_string_index(dev->of_node, "model", 0, &priv->name);
    if (ret < 0) {
        return ret;
    }

    g_snd_card = priv;

    dev_info(dev, "meson_card_probe(%s) SUCCESS.\n", priv->name);
    return 0;
}

static const struct of_device_id axg_card_of_match[] = {
    {
        .compatible = "amlogic,axg-sound-card",
    }, {}
};
MODULE_DEVICE_TABLE(of, axg_card_of_match);

static struct platform_driver axg_card_pdrv = {
    .probe = meson_card_probe,
    .driver = {
        .name = "axg-sound-card",
        .of_match_table = axg_card_of_match,
    },
};
module_platform_driver(axg_card_pdrv);

