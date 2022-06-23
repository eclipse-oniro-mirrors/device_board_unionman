/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef A311D_CODEC_OPS_H
#define A311D_CODEC_OPS_H

#include "audio_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int32_t A311DCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *codec);
int32_t A311DCodecDeviceReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *value);
int32_t A311DCodecDeviceWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t value);

int32_t A311DCodecDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device);
int32_t A311DCodecDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param);
int32_t A311DCodecDaiStartup(const struct AudioCard *card, const struct DaiDevice *device);
int32_t A311DCodecDaiTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* A311D_CODEC_OPS_H */
