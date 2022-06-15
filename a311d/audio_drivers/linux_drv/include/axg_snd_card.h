/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AXG_SND_CARD_H
#define AXG_SND_CARD_H

#include "axg_fifo.h"
#include "axg_tdm_interface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int meson_axg_snd_card_init(void);

struct axg_tdm_iface *meson_axg_default_tdm_iface_get(void);

/* dir: 1 - toddr, 0 - frddr */
struct axg_fifo *meson_axg_default_fifo_get(int dir);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* AXG_SND_CARD_H */
