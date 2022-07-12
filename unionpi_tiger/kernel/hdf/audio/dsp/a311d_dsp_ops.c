/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "spi_if.h"
#include "audio_dsp_if.h"
#include "audio_driver_log.h"

#include "a311d_dsp_ops.h"

#define HDF_LOG_TAG a311d_dsp_ops

int32_t DspDaiStartup(const struct AudioCard *card, const struct DaiDevice *device)
{
    (void)card;
    (void)device;
    return HDF_SUCCESS;
}

int32_t DspDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    (void)card;
    (void)param;
    return HDF_SUCCESS;
}

int32_t DspDeviceInit(const struct DspDevice *device)
{
    (void)device;
    return HDF_SUCCESS;
}

int32_t DspDeviceReadReg(const struct DspDevice *device, const void *msgs, const uint32_t len)
{
    (void)device;
    (void)msgs;
    return HDF_SUCCESS;
}

int32_t DspDeviceWriteReg(const struct DspDevice *device, const void *msgs, const uint32_t len)
{
    (void)device;
    (void)msgs;
    return HDF_SUCCESS;
}

int32_t DspDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device)
{
    (void)card;
    (void)device;
    return HDF_SUCCESS;
}

int32_t DspDecodeAudioStream(const struct AudioCard *card, const uint8_t *buf, const struct DspDevice *device)
{
    (void)card;
    (void)buf;
    (void)device;
    return HDF_SUCCESS;
}

int32_t DspEncodeAudioStream(const struct AudioCard *card, const uint8_t *buf, const struct DspDevice *device)
{
    (void)card;
    (void)buf;
    (void)device;
    return HDF_SUCCESS;
}

int32_t DspEqualizerActive(const struct AudioCard *card, const uint8_t *buf, const struct DspDevice *device)
{
    (void)card;
    (void)buf;
    (void)device;
    return HDF_SUCCESS;
}
