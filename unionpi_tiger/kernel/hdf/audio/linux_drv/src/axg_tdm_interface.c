/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/clk.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/regmap.h>
#include "sound/soc.h"
#include "sound/soc-dai.h"

#include "axg_common.h"
#include "axg_tdm_formatter.h"
#include "axg_tdm_interface.h"

#define MCLK_FS_DEFAULT     256

static LIST_HEAD(axg_tdm_iface_list);

#define TDM_STREAM_GET(iface, stream) \
    ((stream) == AXG_TDM_IFACE_STREAM_PLAYBACK) ? (iface)->playback_ts : (iface)->capture_ts

static int tdm_iface_register(struct axg_tdm_iface *iface)
{
    struct axg_tdm_iface *f;
    list_for_each_entry(f, &axg_tdm_iface_list, list) {
        if (!strcmp(f->name_prefix, iface->name_prefix)) {
            dev_err(iface->dev, "duplicated dev[%s], no need to register\n",
                    iface->name_prefix);
            return -1;
        }
    }

    list_add_tail(&iface->list, &axg_tdm_iface_list);

    dev_info(iface->dev, "Register Snd TDM iface SUCCESS: %s\n", iface->name_prefix);

    return 0;
}

static struct axg_tdm_iface *tdm_iface_find(const char *name_prefix)
{
    struct axg_tdm_iface *f;
    list_for_each_entry(f, &axg_tdm_iface_list, list) {
        if (!strcmp(f->name_prefix, name_prefix)) {
            return f;
        }
    }

    return NULL;
}

static unsigned int tdm_slots_total(u32 *mask)
{
    unsigned int slots = 0;
    int i;

    if (!mask) {
        return 0;
    }

    /* Count the total number of slots provided by all 4 lanes */
    for (i = 0; i < AXG_TDM_NUM_LANES; i++) {
        slots += hweight32(mask[i]);
    }

    return slots;
}

int meson_axg_tdm_iface_set_tdm_slots(struct axg_tdm_iface *iface, unsigned int tx_mask[4],
                                      unsigned int rx_mask[4], unsigned int slots,
                                      unsigned int slot_width)
{
    struct axg_tdm_stream *tx = (struct axg_tdm_stream *)iface->playback_ts;
    struct axg_tdm_stream *rx = (struct axg_tdm_stream *)iface->capture_ts;
    unsigned int tx_slots, rx_slots;
    unsigned int slot_width_tmp = slot_width;

    /* We should at least have a slot for a valid interface */
    tx_slots = tdm_slots_total(tx_mask);
    rx_slots = tdm_slots_total(rx_mask);
    if (!tx_slots && !rx_slots) {
        dev_err(iface->dev, "interface has no slot\n");
        return -EINVAL;
    }

    iface->slots = slots;

    switch (slot_width) {
        case 0:
            slot_width_tmp = AXG_BIT_WIDTH32;
            fallthrough;
        case AXG_BIT_WIDTH32:
            fallthrough;
        case AXG_BIT_WIDTH24:
            fallthrough;
        case AXG_BIT_WIDTH16:
            fallthrough;
        case AXG_BIT_WIDTH8:
            break;
        default:
            dev_err(iface->dev, "unsupported slot width: %d\n", slot_width);
            return -EINVAL;
    }

    iface->slot_width = slot_width_tmp;

    /* Amend the iface driver */
    if (tx) {
        tx->mask = tx_mask;
        iface->tx_slots = tx_slots;
    }

    if (rx) {
        rx->mask = rx_mask;
        iface->rx_slots = rx_slots;
    }

    return 0;
}

int meson_axg_tdm_iface_set_fmt(struct axg_tdm_iface *iface, unsigned int fmt)
{
    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
        case SND_SOC_DAIFMT_CBS_CFS:
            if (!iface->mclk) {
                dev_err(iface->dev, "cpu clock master: mclk missing\n");
                return -ENODEV;
            }
            break;
        case SND_SOC_DAIFMT_CBM_CFM:
            break;

        case SND_SOC_DAIFMT_CBS_CFM:
        case SND_SOC_DAIFMT_CBM_CFS:
            dev_err(iface->dev, "only CBS_CFS and CBM_CFM are supported\n");
            fallthrough;
        default:
            return -EINVAL;
    }

    iface->fmt = fmt;
    return 0;
}

static int tdm_iface_set_sysclk(struct axg_tdm_iface *iface,
                                unsigned int freq)
{
    int ret = -ENOTSUPP;

    if (!iface->mclk) {
        dev_warn(iface->dev, "master clock not provided\n");
    } else {
        ret = clk_set_rate(iface->mclk, freq);
        if (!ret) {
            iface->mclk_rate = freq;
        }
    }

    dev_info(iface->dev, "%s: freq=%u\n", __FUNCTION__, freq);

    return ret;
}

static int tdm_iface_set_stream(struct axg_tdm_iface *iface,
                                enum axg_tdm_iface_stream stream,
                                struct axg_pcm_hw_params *params)
{
    struct axg_tdm_stream *ts = TDM_STREAM_GET(iface, stream);
    unsigned int channels = params->channels;
    unsigned int width = params->bit_width;

    if (!ts) {
        return -1;
    }

    /* Save rate and sample_bits for component symmetry */
    iface->rate = params->rate;

    /* Make sure this interface can cope with the stream */
    if (tdm_slots_total(ts->mask) < channels) {
        dev_err(iface->dev, "not enough slots for channels\n");
        return -EINVAL;
    }

    if (iface->slot_width < width) {
        dev_err(iface->dev, "incompatible slots width for stream\n");
        return -EINVAL;
    }

    /* Save the parameter for tdmout/tdmin widgets */
    ts->physical_width = params->physical_width;
    ts->width = width;
    ts->channels = channels;

    return 0;
}

static int tdm_iface_set_lrclk(struct axg_tdm_iface *iface,
                               unsigned int rate)
{
    unsigned int ratio_num;
    int ret;

    ret = clk_set_rate(iface->lrclk, rate);
    if (ret) {
        dev_err(iface->dev, "setting sample clock failed: %d\n", ret);
        return ret;
    }

    switch (iface->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
        case SND_SOC_DAIFMT_LEFT_J:
        case SND_SOC_DAIFMT_RIGHT_J:
            /* 50% duty cycle ratio */
            ratio_num = 1;
            break;
        case SND_SOC_DAIFMT_DSP_A:
        case SND_SOC_DAIFMT_DSP_B:
            /*
             * A zero duty cycle ratio will result in setting the mininum
             * ratio possible which, for this clock, is 1 cycle of the
             * parent bclk clock high and the rest low, This is exactly
             * what we want here.
             */
            ratio_num = 0;
            break;
        default:
            return -EINVAL;
    }

    ret = clk_set_duty_cycle(iface->lrclk, ratio_num, 2);
    if (ret) {
        dev_err(iface->dev,
                "setting sample clock duty cycle failed: %d\n", ret);
        return ret;
    }

    /* Set sample clock inversion */
    ret = clk_set_phase(iface->lrclk,
                        axg_tdm_lrclk_invert(iface->fmt) ? 180 : 0);
    if (ret) {
        dev_err(iface->dev,
                "setting sample clock phase failed: %d\n", ret);
        return ret;
    }

    return 0;
}

static int tdm_iface_set_sclk(struct axg_tdm_iface *iface,
                              unsigned int rate)
{
    unsigned long srate;
    int ret;

    srate = (unsigned long)(iface->slots * iface->slot_width * rate);

    if (!iface->mclk_rate) {
        /* If no specific mclk is requested, default to bit clock * 4 */
        clk_set_rate(iface->mclk, 4 * srate);
    } else {
        /* Check if we can actually get the bit clock from mclk */
        if (iface->mclk_rate % srate) {
            dev_err(iface->dev,
                    "can't derive sclk %lu from mclk %lu\n",
                    srate, iface->mclk_rate);
            return -EINVAL;
        }
    }

    ret = clk_set_rate(iface->sclk, srate);
    if (ret) {
        dev_err(iface->dev, "setting bit clock failed: %d\n", ret);
        return ret;
    }

    /* Set the bit clock inversion */
    ret = clk_set_phase(iface->sclk,
                        axg_tdm_sclk_invert(iface->fmt) ? 0 : 180);
    if (ret) {
        dev_err(iface->dev, "setting bit clock phase failed: %d\n", ret);
        return ret;
    }

    return ret;
}

int meson_axg_tdm_iface_hw_params(struct axg_tdm_iface *iface,
                                  enum axg_tdm_iface_stream stream,
                                  struct axg_pcm_hw_params *params)
{
    int ret;

    switch (iface->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
        case SND_SOC_DAIFMT_I2S:
        case SND_SOC_DAIFMT_LEFT_J:
        case SND_SOC_DAIFMT_RIGHT_J:
            if (iface->slots > 2) {
                dev_err(iface->dev, "bad slot number for format: %d\n",
                        iface->slots);
                return -EINVAL;
            }
            break;
        case SND_SOC_DAIFMT_DSP_A:
        case SND_SOC_DAIFMT_DSP_B:
            break;
        default:
            dev_err(iface->dev, "unsupported dai format\n");
            return -EINVAL;
    }

    dev_info(iface->dev, "%s(): rate=%u, chan=%u, bitwidth=%u, phys=%u . [fmt=0x%x, mclk_fs=%u]\n",
             __FUNCTION__, params->rate, params->channels, params->bit_width, params->physical_width,
             iface->fmt, iface->mclk_fs);

    ret = tdm_iface_set_sysclk(iface, params->rate * iface->mclk_fs);
    if (ret) {
        return ret;
    }

    ret = tdm_iface_set_stream(iface, stream, params);
    if (ret) {
        return ret;
    }

    if ((iface->fmt & SND_SOC_DAIFMT_MASTER_MASK) == SND_SOC_DAIFMT_CBS_CFS) {
        ret = tdm_iface_set_sclk(iface, params->rate);
        if (ret) {
            return ret;
        }

        ret = tdm_iface_set_lrclk(iface, params->rate);
        if (ret) {
            return ret;
        }
    }

    return 0;
}

int meson_axg_tdm_iface_unprepare(struct axg_tdm_iface *iface,
                                  enum axg_tdm_iface_stream stream)
{
    struct axg_tdm_stream *ts = TDM_STREAM_GET(iface, stream);

    if (!ts) {
        return -1;
    }

    /* Stop all attached formatters */
    axg_tdm_stream_stop(ts);

    return 0;
}

int meson_axg_tdm_iface_prepare(struct axg_tdm_iface *iface,
                                enum axg_tdm_iface_stream stream)
{
    struct axg_tdm_stream *ts = TDM_STREAM_GET(iface, stream);

    if (!ts) {
        return -1;
    }

    /* Force all attached formatters to update */
    return axg_tdm_stream_reset(ts);
}

static int tdm_iface_enable(struct axg_tdm_iface *iface, bool enable)
{
    int ret = 0;

    if (enable) {
        ret = clk_prepare_enable(iface->mclk);
    } else {
        clk_disable_unprepare(iface->mclk);
    }

    return ret;
}

int meson_axg_tdm_iface_stop(struct axg_tdm_iface *iface,
                             enum axg_tdm_iface_stream stream)
{
    struct axg_tdm_formatter *formatter;

    formatter = (stream == AXG_TDM_IFACE_STREAM_PLAYBACK) ? iface->playback_formatter : iface->capture_formatter;

    if (formatter) {
        axg_tdm_formatter_power_down(formatter);
    }

    tdm_iface_enable(iface, false);

    dev_info(iface->dev, "Axg TDM Iface stop success.\n");

    return 0;
}

int meson_axg_tdm_iface_start(struct axg_tdm_iface *iface,
                              enum axg_tdm_iface_stream stream)
{
    struct axg_tdm_formatter *formatter;
    struct axg_tdm_stream *ts = TDM_STREAM_GET(iface, stream);
    int ret = -1;

    ret = tdm_iface_enable(iface, true);
    if (ret) {
        dev_info(iface->dev, "tdm_iface_enable() failed. %s, %d\n",
                 iface->name_prefix, stream);
        return ret;
    }

    formatter = (stream == AXG_TDM_IFACE_STREAM_PLAYBACK) ? iface->playback_formatter : iface->capture_formatter;
    if (formatter && ts) {
        ret = axg_tdm_formatter_power_up(formatter, ts);
    }

    if (ret) {
        dev_info(iface->dev, "Axg TDM Iface startup failed. %s, %d\n",
                 iface->name_prefix, stream);
        return ret;
    }

    dev_info(iface->dev, "Axg TDM Iface start success. %s, %d\n",
             iface->name_prefix, stream);

    return 0;
}

// It's better to free stream when failed.
int meson_axg_tdm_iface_init(struct axg_tdm_iface *iface,
                             const char *playback_formatter, unsigned int sink_port,
                             const char *capture_formatter, unsigned int src_port)
{
    struct axg_tdm_formatter *formatter;
    int ret;

    if (playback_formatter) {
        iface->playback_ts = axg_tdm_stream_alloc(iface);
        if (!iface->playback_ts) {
            return -ENOMEM;
        }

        formatter = axg_tdm_formatter_get(playback_formatter);
        if (!formatter) {
            dev_err(iface->dev, "get playback formatter(%s) failed.\n", playback_formatter);
            return -EINVAL;
        }

        ret = axg_tdm_formatter_sink_out_sel(formatter, sink_port);
        if (ret) {
            dev_err(iface->dev, "set sink out sel(%u) for (%s) failed.\n", sink_port, playback_formatter);
            return -EINVAL;
        }

        iface->playback_formatter = formatter;
    }

    if (capture_formatter) {
        iface->capture_ts = axg_tdm_stream_alloc(iface);
        if (!iface->capture_ts) {
            return -ENOMEM;
        }

        formatter = axg_tdm_formatter_get(capture_formatter);
        if (!formatter) {
            dev_err(iface->dev, "get capture formatter(%s) failed.\n", capture_formatter);
            return -EINVAL;
        }

        ret = axg_tdm_formatter_src_in_sel(formatter, src_port);
        if (ret) {
            dev_err(iface->dev, "set src in sel(%u) for (%s) failed.\n", sink_port, playback_formatter);
            return -EINVAL;
        }

        iface->capture_formatter = formatter;
    }

    dev_info(iface->dev, "Axg TDM Iface init success.\n");

    return 0;
}

struct axg_tdm_iface *meson_axg_tdm_iface_get(const char *name_prefix)
{
    return tdm_iface_find(name_prefix);
}

static const struct of_device_id axg_tdm_iface_of_match[] = {
    { .compatible = "amlogic,axg-tdm-iface", },
    {}
};
MODULE_DEVICE_TABLE(of, axg_tdm_iface_of_match);

static int axg_tdm_iface_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct axg_tdm_iface *iface;
    int ret;

    iface = devm_kzalloc(dev, sizeof(*iface), GFP_KERNEL);
    if (!iface) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, iface);

    /* Bit clock provided on the pad */
    iface->sclk = devm_clk_get(dev, "sclk");
    if (IS_ERR(iface->sclk)) {
        ret = PTR_ERR(iface->sclk);
        if (ret != -EPROBE_DEFER) {
            dev_err(dev, "failed to get sclk: %d\n", ret);
        }
        return ret;
    }

    /* Sample clock provided on the pad */
    iface->lrclk = devm_clk_get(dev, "lrclk");
    if (IS_ERR(iface->lrclk)) {
        ret = PTR_ERR(iface->lrclk);
        if (ret != -EPROBE_DEFER) {
            dev_err(dev, "failed to get lrclk: %d\n", ret);
        }
        return ret;
    }

    /*
     * mclk maybe be missing when the cpu dai is in slave mode and
     * the codec does not require it to provide a master clock.
     * At this point, ignore the error if mclk is missing. We'll
     * throw an error if the cpu dai is master and mclk is missing
     */
    iface->mclk = devm_clk_get(dev, "mclk");
    if (IS_ERR(iface->mclk)) {
        ret = PTR_ERR(iface->mclk);
        if (ret == -ENOENT) {
            iface->mclk = NULL;
        } else {
            if (ret != -EPROBE_DEFER) {
                dev_err(dev, "failed to get mclk: %d\n", ret);
            }
            return ret;
        }
    }

    ret = of_property_read_string(dev->of_node, "sound-name-prefix", &iface->name_prefix);
    if (ret) {
        dev_err(dev, "failed to get sound-name-prefix\n");
        return ret;
    }

    iface->dev = dev;
    iface->fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_CBS_CFS; // 0x4001
    iface->mclk_fs = MCLK_FS_DEFAULT;

    tdm_iface_register(iface);

    return 0;
}

static struct platform_driver axg_tdm_iface_pdrv = {
    .probe = axg_tdm_iface_probe,
    .driver = {
        .name = "axg-tdm-iface",
        .of_match_table = axg_tdm_iface_of_match,
    },
};
module_platform_driver(axg_tdm_iface_pdrv);
