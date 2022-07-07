/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AXG_TDM_INTERFACE_H
#define AXG_TDM_INTERFACE_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct axg_tdm_iface;

enum axg_tdm_iface_stream {
    AXG_TDM_IFACE_STREAM_PLAYBACK,
    AXG_TDM_IFACE_STREAM_CAPTURE,
};

struct axg_pcm_hw_params {
    unsigned int channels;
    unsigned int bit_width;
    unsigned int physical_width;
    unsigned int rate;
};

int meson_axg_tdm_iface_set_tdm_slots(struct axg_tdm_iface *iface,
                                      unsigned int tx_mask[4],
                                      unsigned int rx_mask[4], unsigned int slots,
                                      unsigned int slot_width);

int meson_axg_tdm_iface_set_fmt(struct axg_tdm_iface *iface,
                                unsigned int fmt);

int meson_axg_tdm_iface_set_src_in_sel(struct axg_tdm_iface *iface,
                                       unsigned int index);

int meson_axg_tdm_iface_hw_params(struct axg_tdm_iface *iface,
                                  enum axg_tdm_iface_stream stream,
                                  struct axg_pcm_hw_params *params);

int meson_axg_tdm_iface_unprepare(struct axg_tdm_iface *iface,
                                  enum axg_tdm_iface_stream stream);

int meson_axg_tdm_iface_prepare(struct axg_tdm_iface *iface,
                                enum axg_tdm_iface_stream stream);

int meson_axg_tdm_iface_stop(struct axg_tdm_iface *iface,
                             enum axg_tdm_iface_stream stream);

int meson_axg_tdm_iface_start(struct axg_tdm_iface *iface,
                              enum axg_tdm_iface_stream stream);

int meson_axg_tdm_iface_init(struct axg_tdm_iface *iface,
                             const char *playback_formatter, unsigned int sink_port,
                             const char *capture_formatter, unsigned int src_port);

struct axg_tdm_iface *meson_axg_tdm_iface_get(const char *name_prefix);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* AXG_TDM_INTERFACE_H */
