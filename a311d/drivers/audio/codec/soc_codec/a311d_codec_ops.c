/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_sapm.h"
#include "audio_platform_base.h"
#include "audio_driver_log.h"
#include "audio_codec_base.h"
#include "audio_stream_dispatch.h"

#include "g12a_toacodec.h"
#include "g12a_tohdmitx.h"
#include "meson_t9015.h"
#include "nau8540.h"
#include "a311d_codec_ops.h"

#define HDF_LOG_TAG a311d_codec_ops

static int32_t A311DGetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    struct AudioMixerControl *mixerCtrl = NULL;

    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        AUDIO_DRIVER_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    AUDIO_DRIVER_LOG_INFO("kcontrol->name=%s", kcontrol->name);
    
    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (mixerCtrl == NULL) {
        AUDIO_DRIVER_LOG_ERR("mixerCtrl is NULL.");
        return HDF_FAILURE;
    }
    
    // FIXME: DO SOMETHING HERE.
    
    return HDF_SUCCESS;
}

static int32_t A311DSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    struct AudioMixerControl *mixerCtrl = NULL;
    
    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        AUDIO_DRIVER_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    AUDIO_DRIVER_LOG_INFO("kcontrol->name=%s", kcontrol->name);
    
    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    
    // FIXME: DO SOMETHING HERE.

    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device)
{
    if (audioCard == NULL || device == NULL || device->devData == NULL ||
        device->devData->sapmComponents == NULL || device->devData->controls == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (CodecSetCtlFunc(device->devData, A311DGetCtrlOps, A311DSetCtrlOps) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioCodecSetCtlFunc failed.");
        return HDF_FAILURE;
    }

    if (AudioAddControls(audioCard, device->devData->controls, device->devData->numControls) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add controls failed.");
        return HDF_FAILURE;
    }

    //meson_t9015_mute_set(false);
    meson_t9015_volume_set(255, 255);

    AUDIO_DRIVER_LOG_DEBUG("success. numControls=%d", device->devData->numControls);
    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceReadReg(unsigned long virtualAddress, uint32_t reg, uint32_t *value)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceWriteReg(unsigned long virtualAddress, uint32_t reg, uint32_t value)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DCodecDaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

static int32_t FormatToBitWidth(enum AudioFormat format, uint32_t *bitWidth)
{
    switch (format) {
        case AUDIO_FORMAT_PCM_32_BIT:
            *bitWidth = 32;
            break;
        case AUDIO_FORMAT_PCM_24_BIT:
            *bitWidth = 24;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            *bitWidth = 16;
            break;
        case AUDIO_FORMAT_PCM_8_BIT:
            *bitWidth = 8;
            break;
        default:
            return -1;
    }

    return 0;
}

int32_t A311DCodecDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    int32_t ret;
    uint32_t bitWidth;

    if (card == NULL || card->rtd == NULL || card->rtd->cpuDai == NULL ||
            param == NULL || param->cardServiceName == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is nullptr.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("streamType: %d", param->streamType);
    
    if (FormatToBitWidth(param->format, &bitWidth)) {
        AUDIO_DRIVER_LOG_ERR("FormatToBitWidth() failed.");
        return HDF_FAILURE;
    }

    if (param->streamType == AUDIO_RENDER_STREAM) {
        ret = HDF_SUCCESS;
    } else {
        ret = nau8540_hw_params(param->rate, bitWidth);
    }

    AUDIO_DRIVER_LOG_DEBUG("streamType: %d, ret: %d", param->streamType, ret);
    
    return ret;
}

int32_t A311DCodecDaiStartup(const struct AudioCard *card, const struct DaiDevice *device)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DCodecDaiTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device)
{
    int ret = HDF_FAILURE;
    AUDIO_DRIVER_LOG_DEBUG(" cmd -> %d", cmd);

    (void)card;
    (void)device;

    switch (cmd) {
        case AUDIO_DRV_PCM_IOCTL_RENDER_START:
        case AUDIO_DRV_PCM_IOCTL_RENDER_RESUME:
            ret = meson_g12a_toacodec_enable(true);
            ret |= meson_g12a_tohdmitx_enable(true);
            ret |= meson_t9015_enable(true);
            break;
        case AUDIO_DRV_PCM_IOCTL_RENDER_STOP:
        case AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE:
            ret = meson_g12a_toacodec_enable(false);
            ret |= meson_g12a_tohdmitx_enable(false);
            ret |= meson_t9015_enable(false);
            break;
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_START:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME:
            ret = nau8540_adc_enable(true);
            break;
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE:
            ret = nau8540_adc_enable(false);
            break;
        default:
            break;
    }

    AUDIO_DRIVER_LOG_DEBUG(" cmd -> %d, ret -> %d", cmd, ret);
    
    return ret;
}
