/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
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

#define VOLUME_DEFAULT          (T9015_VOLUME_MAX * 4 / 5)

static const struct AudioSapmRoute g_audioRoutes[] = {
    { "SPKL", NULL, "SPKL PGA"},
    { "HPL", NULL, "HPL PGA"},
    { "HPR", NULL, "HPR PGA"},
    { "SPKL PGA", "Speaker1 Switch", "DAC1"},
    { "HPL PGA", "Headphone1 Switch", "DAC2"},
    { "HPR PGA", "Headphone2 Switch", "DAC3"},

    { "ADCL", NULL, "LPGA"},
    { "ADCR", NULL, "RPGA"},
    { "LPGA", "LPGA MIC Switch", "MIC1"},
    { "RPGA", "RPGA MIC Switch", "MIC2"},
};

static int32_t A311DGetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    struct AudioMixerControl *mixerCtrl = NULL;

    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        AUDIO_DRIVER_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);

    elemValue->value[0] = mixerCtrl->value & 0xffff;
    elemValue->value[1] = mixerCtrl->value >> 16U;

    AUDIO_DRIVER_LOG_INFO("GET kcontrol: \"%s\"=[%u,%u]", kcontrol->name, elemValue->value[0], elemValue->value[1]);

    return HDF_SUCCESS;
}

static int32_t A311DSetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    struct AudioMixerControl *mixerCtrl = NULL;
    uint32_t lvalue, rvalue;

    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        AUDIO_DRIVER_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    AUDIO_DRIVER_LOG_INFO("SET kcontrol: \"%s\"=[%u,%u]", kcontrol->name, elemValue->value[0], elemValue->value[1]);

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);

    lvalue = elemValue->value[0];
    rvalue = elemValue->value[1];

    if (lvalue < mixerCtrl->min || lvalue > mixerCtrl->max ||
        rvalue < mixerCtrl->min || rvalue > mixerCtrl->max) {
        AUDIO_DRIVER_LOG_ERR("ERROR: Value is out of range [%u,%u]", mixerCtrl->min, mixerCtrl->max);
        return HDF_ERR_INVALID_PARAM;
    }

    mixerCtrl->value = (rvalue << 16U) | lvalue;

    if (!strcmp(kcontrol->name, "Main Playback Volume")) {
        meson_t9015_volume_set(lvalue, rvalue);
    } else if (!strcmp(kcontrol->name, "Main Capture Volume")) {
    } else if (!strcmp(kcontrol->name, "Playback Mute")) {
        meson_t9015_mute_set(lvalue);
    } else if (!strcmp(kcontrol->name, "Capture Mute")) {
    } else if (!strcmp(kcontrol->name, "Mic Left Gain")) {
    } else if (!strcmp(kcontrol->name, "Mic Right Gain")) {
    } else if (!strcmp(kcontrol->name, "Render Channel Mode")) {
    } else if (!strcmp(kcontrol->name, "Captrue Channel Mode")) {
    }

    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device)
{
    if (audioCard == NULL || device == NULL || device->devData == NULL ||
        device->devData->sapmComponents == NULL || device->devData->controls == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (CodecSetCtlFunc(device->devData, AUDIO_CONTROL_MIXER, A311DGetCtrlOps, A311DSetCtrlOps) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioCodecSetCtlFunc failed.");
        return HDF_FAILURE;
    }

    if (AudioAddControls(audioCard, device->devData->controls, device->devData->numControls) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add controls failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmNewComponents(audioCard, device->devData->sapmComponents,
        device->devData->numSapmComponent) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("new components failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmAddRoutes(audioCard, g_audioRoutes, HDF_ARRAY_SIZE(g_audioRoutes)) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add route failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmNewControls(audioCard) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add sapm controls failed.");
        return HDF_FAILURE;
    }

    if (AudioSapmSleep(audioCard) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add sapm sleep modular failed.");
        return HDF_FAILURE;
    }

    meson_t9015_volume_set(VOLUME_DEFAULT, VOLUME_DEFAULT);

    AUDIO_DRIVER_LOG_DEBUG("success. numControls=%d", device->devData->numControls);
    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceReadReg(const struct CodecDevice *codec, uint32_t reg, uint32_t *value)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DCodecDeviceWriteReg(const struct CodecDevice *codec, uint32_t reg, uint32_t value)
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
            *bitWidth = BIT_WIDTH32;
            break;
        case AUDIO_FORMAT_PCM_24_BIT:
            *bitWidth = BIT_WIDTH24;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            *bitWidth = BIT_WIDTH16;
            break;
        case AUDIO_FORMAT_PCM_8_BIT:
            *bitWidth = BIT_WIDTH8;
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
