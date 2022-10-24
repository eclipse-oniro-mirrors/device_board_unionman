/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/soundcard.h>
#include "sound/asound.h"

#include "hdf_log.h"
#include "tinyalsa/asoundlib.h"

#include "audio_interface_lib_render.h"

#define HDF_LOG_TAG HDI_AUDIO_R_ALSA

// treat hats.
#define PLEASURE_HATS

#define SOUND_CARD_ID (0)
#define SOUND_DEV_ID (1)

#define VOLUME_DEFAULT (50)

struct AlsaCtx {
    struct pcm *pcmHandler;
    struct pcm_config config;
    struct mixer *mixerHandler;

    int initFlag;
    int renderRefCount;
    int ctlRefCount;
    int volume; // volume in db [ (volMax - volMin) / 2 * log10(Vol) + volMin]
    int volMin;
    int volMax;

    float gain;
    float gainMin;
    float gainMax;

    bool mute;
};

static struct AlsaCtx s_alsaCtx = {.initFlag = 0};

struct AlsaDevObject {
    char serviceName[64];
    struct AlsaCtx *pAlsaCtx;
};

static enum pcm_format ConvertFormatToAlsa(enum AudioFormat format)
{
    switch (format) {
        case AUDIO_FORMAT_TYPE_PCM_8_BIT:
            return PCM_FORMAT_S8;
        case AUDIO_FORMAT_TYPE_PCM_16_BIT:
            return PCM_FORMAT_S16_LE;
        case AUDIO_FORMAT_TYPE_PCM_24_BIT:
            return PCM_FORMAT_S24_LE;
        case AUDIO_FORMAT_TYPE_PCM_32_BIT:
            return PCM_FORMAT_S32_LE;
        case AUDIO_FORMAT_TYPE_AAC_MAIN:
        case AUDIO_FORMAT_TYPE_AAC_LC:
        case AUDIO_FORMAT_TYPE_AAC_LD:
        case AUDIO_FORMAT_TYPE_AAC_ELD:
        case AUDIO_FORMAT_TYPE_AAC_HE_V1:
        case AUDIO_FORMAT_TYPE_AAC_HE_V2:
        case AUDIO_FORMAT_TYPE_G711A:
        case AUDIO_FORMAT_TYPE_G711U:
        case AUDIO_FORMAT_TYPE_G726:
        default:
            return PCM_FORMAT_INVALID;
    }
}

static int32_t MixerInit(struct mixer *mixer)
{
    int32_t ret = 0;

    if (!mixer) {
        HDF_LOGE("Error: mixer is NULL.");
        return -1;
    }

    // config audio path
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "FRDDR_B SRC 1 EN Switch"), 0, 1);
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "FRDDR_B SINK 1 SEL"), "OUT 1");
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TDMOUT_B SRC SEL"), "IN 1");

    ret |= mixer_ctl_set_value(mixer_get_ctl_by_name(mixer, "TOHDMITX Switch"),
                               0, 1);
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TOHDMITX I2S SRC"), "I2S B");

    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "TOACODEC OUT EN Switch"), 0, 1);
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TOACODEC SRC"), "I2S B");

    // config volume
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "ACODEC Playback Volume"), 0, 255U);
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "ACODEC Playback Volume"), 1, 255U);
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "ACODEC Mute Ramp Switch"), 0, 1);
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "ACODEC Unmute Ramp Switch"), 0, 1);

    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(mixer, "TDMOUT_B Gain Enable Switch"), 0, 1);

    return ret;
}

static int32_t AlsaVolumeUpdate(struct AlsaCtx *aCtx)
{
    int32_t ret = 0;
    int volumeSet = 0;

    if (!aCtx->mixerHandler) {
        return -1;
    }

    if (aCtx->mute) {
        volumeSet = 0;
    } else {
        volumeSet = aCtx->volume * 255U /
                    (aCtx->volMax - aCtx->volMin); // need adjustment
    }

    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(aCtx->mixerHandler, "TDMOUT_B Lane 0 Volume"), 0,
        volumeSet);
    ret |= mixer_ctl_set_value(
        mixer_get_ctl_by_name(aCtx->mixerHandler, "TDMOUT_B Lane 0 Volume"), 1,
        volumeSet);

    HDF_LOGV("Set Volume: volume=%{public}d, mute=%{public}d, "
             "volumeSet=%{public}d, ret=%{public}d",
             aCtx->volume, aCtx->mute, volumeSet, ret);

    return ret;
}

static struct AlsaCtx *AlsaCtxInstanceGet(void)
{
    if (!s_alsaCtx.initFlag) {
        memset_s(&s_alsaCtx, sizeof(s_alsaCtx), 0, sizeof(s_alsaCtx));
        s_alsaCtx.renderRefCount = 0;
        s_alsaCtx.pcmHandler = NULL;
        s_alsaCtx.volMax = 100U;
        s_alsaCtx.volMin = 0;
        s_alsaCtx.volume = VOLUME_DEFAULT;
        s_alsaCtx.gainMin = 0.0;
        s_alsaCtx.gainMax = 15.0f;
        s_alsaCtx.gain = s_alsaCtx.gainMax;
        s_alsaCtx.mute = false;
        s_alsaCtx.mixerHandler = mixer_open(SOUND_CARD_ID);
        MixerInit(s_alsaCtx.mixerHandler);
        s_alsaCtx.initFlag = 1;

        AlsaVolumeUpdate(&s_alsaCtx);
    }

    return &s_alsaCtx;
}

static int AlsaOpen(struct AlsaCtx *aCtx)
{
    int sound_card_id = SOUND_CARD_ID;
    int sound_dev_id = SOUND_DEV_ID;

    HDF_LOGI(
        "%{public}s() rate=%{public}d, channels=%{public}d, format=%{public}d",
        __func__, aCtx->config.rate, aCtx->config.channels,
        aCtx->config.format);

    if (aCtx->pcmHandler) {
        HDF_LOGW("Alsa is opened already.");
        return 0;
    }

    struct pcm_config config;
    (void)memcpy_s(&config, sizeof(config), &aCtx->config, sizeof(config));
    struct pcm *pcm = pcm_open(sound_card_id, sound_dev_id, PCM_OUT, &config);
    if (!pcm_is_ready(pcm)) {
        HDF_LOGE("Error: %{public}s() Cannot open pcm_out(card %{public}d, "
                 "device %{public}d): %{public}s",
                 __func__, sound_card_id, sound_dev_id, pcm_get_error(pcm));
        pcm_close(pcm);

        return -1;
    }

    aCtx->pcmHandler = pcm;

    HDF_LOGI("Open Alsa Success.");

    return 0;
}

static int AlsaClose(struct AlsaCtx *aCtx)
{
    if (aCtx->pcmHandler) {
        pcm_close(aCtx->pcmHandler);
        aCtx->pcmHandler = NULL;
    }

    HDF_LOGI("Close Alsa Success.");

    return 0;
}

static int CheckHwParam(struct AudioHwRenderParam *handleData)
{
    if (handleData == NULL) {
        HDF_LOGE("Error: handleData is NULL!");
        return -1;
    }

    if (handleData->frameRenderMode.attrs.channelCount != 1U &&
        handleData->frameRenderMode.attrs.channelCount != 2U) {
        HDF_LOGE("Error: Unsupported channel count: %{public}d",
                 handleData->frameRenderMode.attrs.channelCount);
        return -1;
    }

    if (handleData->frameRenderMode.attrs.format == AUDIO_FORMAT_TYPE_PCM_8_BIT &&
        handleData->frameRenderMode.attrs.sampleRate == 8000U)
        return -1;

    if (handleData->frameRenderMode.attrs.format == AUDIO_FORMAT_TYPE_PCM_32_BIT &&
        handleData->frameRenderMode.attrs.sampleRate == 11025U)
        return -1;

    return 0;
}

static int AlsaConfig(struct AlsaCtx *aCtx, struct AudioHwRenderParam *handleData)
{
    struct pcm_config config;
    enum pcm_format format;

    if (CheckHwParam(handleData)) {
        return -1;
    }

    format = ConvertFormatToAlsa(handleData->frameRenderMode.attrs.format);
    if (PCM_FORMAT_INVALID == format) {
        HDF_LOGE("Error: Unsupported format: %{public}d", handleData->frameRenderMode.attrs.format);
        return -1;
    }

    memset_s(&config, sizeof(config), 0, sizeof(config));

    config.channels = handleData->frameRenderMode.attrs.channelCount;
    config.rate = handleData->frameRenderMode.attrs.sampleRate;
    config.format = format;
    config.period_count = handleData->frameRenderMode.periodCount; // 6
    config.period_size = handleData->frameRenderMode.periodSize;   // 512
    config.start_threshold = handleData->frameRenderMode.attrs.startThreshold;
    config.stop_threshold = handleData->frameRenderMode.attrs.stopThreshold;
    config.silence_threshold = handleData->frameRenderMode.attrs.silenceThreshold;

#ifdef PLEASURE_HATS
    if (config.rate == 12000) {
        config.rate = 8000;
    } else if (config.rate == 24000) {
        config.rate = 22050;
    }
#endif

    HDF_LOGV("DUMP Alsa Config 1#: channels=%{public}d, rate=%{public}d, "
             "format=%{public}d, period_count=%{public}d, period_size=%{public}d",
             config.channels, config.rate, config.format, config.period_count, config.period_size);

    HDF_LOGV("DUMP Alsa OldConfig 1#: channels=%{public}d, rate=%{public}d, "
             "format=%{public}d, period_count=%{public}d, period_size=%{public}d",
             aCtx->config.channels, aCtx->config.rate, aCtx->config.format,
             aCtx->config.period_count, aCtx->config.period_size);

    if (aCtx->pcmHandler && !memcmp(&config, &aCtx->config, sizeof(config))) {
        HDF_LOGW("Warn: Same Audio Hw config. No need to change.");
        return 0;
    }

    (void)memcpy_s(&aCtx->config, sizeof(aCtx->config), &config, sizeof(config));

    AlsaClose(aCtx);
    if (AlsaOpen(aCtx) < 0) {
        HDF_LOGE("Error: in %{public}s, AlsaOpen() failed.", __func__);
        return -1;
    }

    HDF_LOGV("Config Alsa SUCCESS.");

    return 0;
}

static int32_t DoCtlRenderSetVolume(const struct DevHandle *handle, int cmdId,
                                    struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    HDF_LOGV("INFO: enter %{public}s(). volume=%{public}f", __func__,
             handleData->renderMode.ctlParam.volume);

    int volume = (int)handleData->renderMode.ctlParam.volume;

    if (volume < aCtx->volMin || volume > aCtx->volMax) {
        HDF_LOGE("Error: Invalid volume: %{public}d", volume);
        return HDF_FAILURE;
    }

    aCtx->volume = volume;

    return AlsaVolumeUpdate(aCtx);
}

static int32_t DoCtlRenderGetVolume(const struct DevHandle *handle, int cmdId,
                                    struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    handleData->renderMode.ctlParam.volume = aCtx->volume;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSetPauseStu(const struct DevHandle *handle, int cmdId,
                                      struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    if (!((struct AlsaCtx *)aCtx)->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    if (pcm_ioctl(aCtx->pcmHandler, SNDRV_PCM_IOCTL_PAUSE,
                  handleData->renderMode.ctlParam.pause ? 1 : 0) < 0) {
        HDF_LOGE("Error: pcm_ioctl(SNDRV_PCM_IOCTL_PAUSE) failed: %{public}s\n",
                 pcm_get_error(aCtx->pcmHandler));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSetMuteStu(const struct DevHandle *handle, int cmdId,
                                     struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    aCtx->mute = handleData->renderMode.ctlParam.mute;

    return AlsaVolumeUpdate(aCtx);
}

static int32_t DoCtlRenderGetMuteStu(const struct DevHandle *handle, int cmdId,
                                     struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    handleData->renderMode.ctlParam.mute = aCtx->mute;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSetGainStu(const struct DevHandle *handle, int cmdId,
                                     struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    float gain = handleData->renderMode.ctlParam.audioGain.gain;

    if (gain < aCtx->gainMin || gain > aCtx->gainMax) {
        HDF_LOGE("Error: Invalid gain: %{public}f", gain);
        return HDF_FAILURE;
    }

    aCtx->gain = gain;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderGetGainStu(const struct DevHandle *handle, int cmdId,
                                     struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    handleData->renderMode.ctlParam.audioGain.gain = aCtx->gain;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSceneSelect(const struct DevHandle *handle, int cmdId,
                                      struct AudioHwRenderParam *handleData)
{
    HDF_LOGV("INFO: enter %{public}s(). portId=%{public}d", __func__,
             handleData->renderMode.hwInfo.deviceDescript.portId);

    HDF_LOGV("DUMP AudioHwRenderParam: %{public}d, %{public}d, %{public}d, "
             "%{public}d, %{public}d, %{public}d, "
             "%{public}d, %{public}d",
             handleData->frameRenderMode.attrs.channelCount,
             handleData->frameRenderMode.attrs.sampleRate,
             handleData->frameRenderMode.attrs.format,
             handleData->frameRenderMode.periodCount,
             handleData->frameRenderMode.periodSize,
             handleData->frameRenderMode.attrs.startThreshold,
             handleData->frameRenderMode.attrs.stopThreshold,
             handleData->frameRenderMode.attrs.silenceThreshold);

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSceneGetGainThreshold(const struct DevHandle *handle, int cmdId,
                                                struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    handleData->renderMode.ctlParam.audioGain.gainMax = aCtx->gainMax;
    handleData->renderMode.ctlParam.audioGain.gainMin = aCtx->gainMin;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderGetVolThreshold(const struct DevHandle *handle,
                                          int cmdId,
                                          struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    handleData->renderMode.ctlParam.volThreshold.volMax = aCtx->volMax;
    handleData->renderMode.ctlParam.volThreshold.volMin = aCtx->volMin;

    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSetChannelMode(const struct DevHandle *handle,
                                         int cmdId,
                                         struct AudioHwRenderParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoCtlRenderGetChannelMode(const struct DevHandle *handle,
                                         int cmdId,
                                         struct AudioHwRenderParam *handleData)
{
    handleData->frameRenderMode.mode = AUDIO_CHANNEL_NORMAL;
    return HDF_SUCCESS;
}

static int32_t DoCtlRenderSetAcodecMode(const struct DevHandle *handle,
                                        int cmdId,
                                        struct AudioHwRenderParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t HandleCtlRenderCmd(const struct DevHandle *handle, int cmdId,
                                  struct AudioHwRenderParam *handleData)
{
    switch (cmdId) {
        case AUDIODRV_CTL_IOCTL_ELEM_READ:
            return DoCtlRenderGetVolume(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE:
            return DoCtlRenderSetVolume(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_MUTE_READ:
            return DoCtlRenderGetMuteStu(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE:
            return DoCtlRenderSetMuteStu(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_CHANNEL_MODE_READ:
            return DoCtlRenderGetChannelMode(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_CHANNEL_MODE_WRITE:
            return DoCtlRenderSetChannelMode(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE:
            return DoCtlRenderSetGainStu(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_GAIN_READ:
            return DoCtlRenderGetGainStu(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_SCENESELECT_WRITE:
            return DoCtlRenderSceneSelect(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_READ:
            return DoCtlRenderSceneGetGainThreshold(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_IN:
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_OUT:
            return DoCtlRenderSetAcodecMode(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_READ:
            return DoCtlRenderGetVolThreshold(handle, cmdId, handleData);
        default:
            HDF_LOGE("Error: Output Mode not support!");
            break;
    }

    return HDF_FAILURE;
}

static int32_t DoOutputRenderHwParams(const struct DevHandle *handle, int cmdId,
                                      struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    if (AlsaConfig(aCtx, handleData) < 0) {
        HDF_LOGE("Error: in %{public}s, AlsaConfig() failed.", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DoOutputRenderWrite(const struct DevHandle *handle, int cmdId,
                                   struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    if (!((struct AlsaCtx *)aCtx)->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    if (!handleData->frameRenderMode.buffer ||
        handleData->frameRenderMode.bufferSize == 0) {
        HDF_LOGE("Error: %{public}s, empty buf", __func__);
        return HDF_FAILURE;
    }

    if (pcm_write(aCtx->pcmHandler, handleData->frameRenderMode.buffer,
                  handleData->frameRenderMode.bufferSize) < 0) {
        HDF_LOGE("Error: pcm_write() failed: %{public}s\n",
                 pcm_get_error(aCtx->pcmHandler));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DoOutputRenderStartPrepare(const struct DevHandle *handle,
                                          int cmdId,
                                          struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    AlsaOpen(aCtx);

    if (!((struct AlsaCtx *)aCtx)->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    // NOTE: depop here
    int bufsize = aCtx->config.period_count * aCtx->config.period_size;
    char *buf = calloc(1, bufsize);
    if (buf) {
        pcm_write(aCtx->pcmHandler, buf, bufsize);
        free(buf);
    }

    usleep(10000);

    return HDF_SUCCESS;
}

static int32_t DoOutputRenderStop(const struct DevHandle *handle, int cmdId,
                                  struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    if (!((struct AlsaCtx *)aCtx)->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    AlsaClose(aCtx);

    return HDF_SUCCESS;
}

static int32_t DoOutputRenderReqMmapBuffer(const struct DevHandle *handle, int cmdId,
                                           struct AudioHwRenderParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoOutputRenderGetMmapPosition(const struct DevHandle *handle, int cmdId,
                                             struct AudioHwRenderParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoOutputRenderOpen(const struct DevHandle *handle, int cmdId,
                                  struct AudioHwRenderParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoOutputRenderClose(const struct DevHandle *handle, int cmdId,
                                   struct AudioHwRenderParam *handleData)
{
    struct AlsaCtx *aCtx = ((struct AlsaDevObject *)handle->object)->pAlsaCtx;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    AlsaClose(aCtx);

    return HDF_SUCCESS;
}

static int32_t HandleOutputRenderCmd(const struct DevHandle *handle, int cmdId,
                                     struct AudioHwRenderParam *handleData)
{
    int32_t ret;
    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
            ret = DoOutputRenderHwParams(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_WRITE:
            ret = DoOutputRenderWrite(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_STOP:
            ret = DoOutputRenderStop(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_START:
        case AUDIO_DRV_PCM_IOCTL_PREPARE:
            ret = DoOutputRenderStartPrepare(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE:
            ret = DoCtlRenderSetPauseStu(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER:
            ret = DoOutputRenderReqMmapBuffer(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION:
            ret = DoOutputRenderGetMmapPosition(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_RENDER_OPEN:
            ret = DoOutputRenderOpen(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_RENDER_CLOSE:
            ret = DoOutputRenderClose(handle, cmdId, handleData);
            break;
        default:
            HDF_LOGE("Error: Output Mode not support!");
            ret = HDF_FAILURE;
            break;
    }
    return ret;
}

/************************************************************************************
    Exported Interface
*************************************************************************************/

/* CreatRender for Bind handle */
struct DevHandle *AudioBindServiceRender(const char *name)
{
    struct DevHandle *handle = NULL;
    if (!name) {
        HDF_LOGE("Error: service name NULL!");
        return NULL;
    }

    HDF_LOGD("AudioBindServiceRender() name=%{public}s", name);

    if (!strcmp(name, "render")) {
        if (AlsaCtxInstanceGet()->renderRefCount > 0) {
            HDF_LOGE("Error: Audio Render can only be bound to 1 handle.");
            return NULL;
        }
    } else if (!strcmp(name, "control")) {
        if (AlsaCtxInstanceGet()->ctlRefCount > 0) {
            HDF_LOGE("Error: Audio Controler can only be bound to 1 handle.");
            return NULL;
        }
    } else {
        HDF_LOGE("Error: invalid service name.");
        return NULL;
    }

    handle = (struct DevHandle *)calloc(1, sizeof(struct DevHandle));
    if (!handle) {
        HDF_LOGE("Error: Failed to OsalMemCalloc handle");
        return NULL;
    }

    struct AlsaDevObject *devObject = calloc(1, sizeof(struct AlsaDevObject));
    if (!devObject) {
        HDF_LOGE("Error: alloc AlsaDevObject failed.");
        free(handle);
        return NULL;
    }

    (void)sprintf_s(devObject->serviceName, sizeof(devObject->serviceName),
                    "%s", name);
    devObject->pAlsaCtx = AlsaCtxInstanceGet();
    if (!strcmp(devObject->serviceName, "render")) {
        devObject->pAlsaCtx->renderRefCount++;
    } else {
        devObject->pAlsaCtx->ctlRefCount++;
    }

    handle->object = devObject;

    HDF_LOGI("BIND SERVICE SUCCESS!");

    return handle;
}

void AudioCloseServiceRender(const struct DevHandle *handle)
{
    if (!handle || !handle->object) {
        HDF_LOGE("Error: Render handle or handle->object is NULL");
        return;
    }

    struct AlsaDevObject *devObject = (struct AlsaDevObject *)handle->object;
    if (!strcmp(devObject->serviceName, "render")) {
        if (--devObject->pAlsaCtx->renderRefCount <= 0) {
            // Do somthing here.
            devObject->pAlsaCtx->renderRefCount = 0;
        }
    } else {
        if (--devObject->pAlsaCtx->ctlRefCount <= 0) {
            // Do somthing here.
            devObject->pAlsaCtx->ctlRefCount = 0;
        }
    }

    HDF_LOGD("AudioCloseServiceRender(), name=%{public}s",
             devObject->serviceName);

    free((void *)handle->object);
    free((void *)handle);

    HDF_LOGD("CLOSE SERVICE SUCCESS!");

    return;
}

int32_t AudioInterfaceLibModeRender(const struct DevHandle *handle,
                                    struct AudioHwRenderParam *handleData,
                                    int cmdId)
{
    if (!handle || !handle->object || !handleData) {
        HDF_LOGE("Error: paras is NULL!");
        return HDF_FAILURE;
    }

    if (AUDIO_DRV_PCM_IOCTL_WRITE != cmdId) {
        HDF_LOGE("AudioInterfaceLibModeRender(cmdid=%{public}d)", cmdId);
    }

    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
        case AUDIO_DRV_PCM_IOCTL_WRITE:
        case AUDIO_DRV_PCM_IOCTRL_STOP:
        case AUDIO_DRV_PCM_IOCTRL_START:
        case AUDIO_DRV_PCM_IOCTL_PREPARE:
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE:
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER:
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION:
        case AUDIO_DRV_PCM_IOCTRL_RENDER_OPEN:
        case AUDIO_DRV_PCM_IOCTRL_RENDER_CLOSE:
            return HandleOutputRenderCmd(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE:
        case AUDIODRV_CTL_IOCTL_ELEM_READ:
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE:
        case AUDIODRV_CTL_IOCTL_MUTE_READ:
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE:
        case AUDIODRV_CTL_IOCTL_GAIN_READ:
        case AUDIODRV_CTL_IOCTL_CHANNEL_MODE_WRITE:
        case AUDIODRV_CTL_IOCTL_CHANNEL_MODE_READ:
        case AUDIODRV_CTL_IOCTL_SCENESELECT_WRITE:
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_READ:
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_IN:
        case AUDIODRV_CTL_IOCTL_ACODEC_CHANGE_OUT:
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_READ:
            return HandleCtlRenderCmd(handle, cmdId, handleData);
        default:
            HDF_LOGE("Error: Mode Error!");
            break;
    }

    return HDF_ERR_NOT_SUPPORT;
}
