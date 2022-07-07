/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AXG_TDM_FORMATTER_H
#define AXG_TDM_FORMATTER_H

#include "axg_tdm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct platform_device;
struct regmap;

struct axg_tdm_formatter;

struct axg_tdm_stream {
    struct axg_tdm_iface *iface;
    struct list_head formatter_list;
    struct mutex lock;
    unsigned int channels;
    unsigned int width;
    unsigned int physical_width;
    unsigned int *mask;
    bool ready;
};

struct axg_tdm_formatter_hw {
    unsigned int skew_offset;
};

struct axg_tdm_formatter_ops {
    int (*sink_out_sel)(struct regmap *map, unsigned int index); // index range: [0, 2]
    int (*src_in_sel)(struct regmap *map, unsigned int index);   // index range: [0, 15]
    void (*enable)(struct regmap *map);
    void (*disable)(struct regmap *map);
    int (*prepare)(struct regmap *map,
                   const struct axg_tdm_formatter_hw *quirks,
                   struct axg_tdm_stream *ts);
};

struct axg_tdm_formatter_driver {
    const struct regmap_config *regmap_cfg;
    const struct axg_tdm_formatter_ops *ops;
    const struct axg_tdm_formatter_hw *quirks;
};

int axg_tdm_formatter_set_channel_masks(struct regmap *map,
                                        struct axg_tdm_stream *ts,
                                        unsigned int offset);

int axg_tdm_formatter_power_up(struct axg_tdm_formatter *formatter,
                               struct axg_tdm_stream *ts);

void axg_tdm_formatter_power_down(struct axg_tdm_formatter *formatter);

// For DTMIN. index range: [0, 15]
int axg_tdm_formatter_src_in_sel(struct axg_tdm_formatter *formatter,
                                 unsigned int index);

// For DTMOUT. index range: [0, 2]
int axg_tdm_formatter_sink_out_sel(struct axg_tdm_formatter *formatter,
                                   unsigned int index);

struct axg_tdm_formatter *axg_tdm_formatter_get(const char *name_prefix);

struct axg_tdm_stream *axg_tdm_stream_alloc(struct axg_tdm_iface *iface);

void axg_tdm_stream_free(struct axg_tdm_stream *ts);

int axg_tdm_stream_start(struct axg_tdm_stream *ts);

void axg_tdm_stream_stop(struct axg_tdm_stream *ts);

static inline int axg_tdm_stream_reset(struct axg_tdm_stream *ts)
{
    axg_tdm_stream_stop(ts);
    return axg_tdm_stream_start(ts);
}

int axg_tdm_formatter_probe(struct platform_device *pdev);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* AXG_TDM_FORMATTER_H */
