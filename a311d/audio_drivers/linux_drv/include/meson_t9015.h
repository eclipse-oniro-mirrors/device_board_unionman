/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef MESON_T9015_H
#define MESON_T9015_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#define T9015_VOLUME_MAX        (255)

int meson_t9015_dai_set_fmt(unsigned int fmt);

/* vol range: [0, 255] */
int meson_t9015_volume_set(unsigned int vol_left, unsigned int vol_right);

int meson_t9015_mute_set(bool mute);

int meson_t9015_enable(bool enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* MESON_T9015_H */
