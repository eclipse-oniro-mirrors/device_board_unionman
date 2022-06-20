/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef G12A_TOHDMITX_H
#define G12A_TOHDMITX_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

enum g12a_tohdmitx_src {
    G12G_TOHDMITX_SRC_I2S_A,
    G12G_TOHDMITX_SRC_I2S_B,
    G12G_TOHDMITX_SRC_I2S_C
};

int meson_g12a_tohdmitx_src_sel(enum g12a_tohdmitx_src src);

int meson_g12a_tohdmitx_enable(bool enable);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* G12A_TOHDMITX_H */
