/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AXG_TDM_H
#define AXG_TDM_H

#include <linux/clk.h>
#include <linux/regmap.h>
#include "sound/pcm.h"
#include "sound/soc.h"
#include "sound/soc-dai.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define AXG_TDM_NUM_LANES 4
#define AXG_TDM_CHANNEL_MAX 128
#define AXG_TDM_RATES (SNDRV_PCM_RATE_5512 | \
                       SNDRV_PCM_RATE_8000_192000)
#define AXG_TDM_FORMATS (SNDRV_PCM_FMTBIT_S8 |     \
                         SNDRV_PCM_FMTBIT_S16_LE | \
                         SNDRV_PCM_FMTBIT_S20_LE | \
                         SNDRV_PCM_FMTBIT_S24_LE | \
                         SNDRV_PCM_FMTBIT_S32_LE)

struct axg_tdm_iface {
    const char *name_prefix; /* such as TDM_A, TDM_B */
    struct device *dev;
    struct clk *sclk;
    struct clk *lrclk;
    struct clk *mclk;
    unsigned long mclk_rate;

    /* format is common to all the DAIs of the iface */
    unsigned int fmt;
    unsigned int mclk_fs;
    unsigned int slots;
    unsigned int slot_width;
    unsigned int tx_slots;
    unsigned int rx_slots;

    /* For component wide symmetry */
    int rate;

    void *playback_formatter;
    void *playback_ts; /* Playback TDM stream */

    void *capture_formatter;
    void *capture_ts; /* Capture TDM stream */

    struct list_head list;
};

static inline bool axg_tdm_lrclk_invert(unsigned int fmt)
{
    return ((fmt & SND_SOC_DAIFMT_FORMAT_MASK) == SND_SOC_DAIFMT_I2S) ^
           !!(fmt & (SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_NB_IF));
}

static inline bool axg_tdm_sclk_invert(unsigned int fmt)
{
    return fmt & (SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_IB_NF);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* AXG_TDM_H */
