/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef A311D_DAI_OPS_H
#define A311D_DAI_OPS_H

#include "audio_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int32_t A311DDeviceInit(struct AudioCard *audioCard, const struct DaiDevice *dai);
int32_t A311DDeviceReadReg(unsigned long virtualAddress, uint32_t reg, uint32_t *value);
int32_t A311DDeviceWriteReg(unsigned long virtualAddress, uint32_t reg, uint32_t value);

int32_t A311DDaiStartup(const struct AudioCard *card, const struct DaiDevice *device);
int32_t A311DDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param);
int32_t A311DDaiTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* A311D_CODEC_OPS_H */
