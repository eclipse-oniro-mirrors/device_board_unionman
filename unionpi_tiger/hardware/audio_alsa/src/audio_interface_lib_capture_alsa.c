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

#include "audio_interface_lib_capture.h"

#define HDF_LOG_TAG HDI_AUDIO_C_ALSA

// treat hats.
#define PLEASURE_HATS

#define SOUND_CARD_ID (0)
#define SOUND_DEV_ID (4)

struct AlsaCtx {
    struct pcm *pcmHandler;
    struct pcm_config config;
    struct mixer *mixerHandler;

    int initFlag;
    int volume;
    int volMin;
    int volMax;
    float gainMax;
    float gainMin;
    float gain;
    bool mute;
};

static struct AlsaCtx s_alsaCtx = {.initFlag = 0};

static int32_t MixerInit(struct mixer *mixer)
{
    int32_t ret = 0;

    if (!mixer) {
        HDF_LOGE("Error: mixer is NULL.");
        return -1;
    }

    // config audio path
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TDMIN_LB SRC SEL"), "IN 1");
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TDMIN_B SRC SEL"), "IN 1");
    ret |= mixer_ctl_set_enum_by_string(
        mixer_get_ctl_by_name(mixer, "TODDR_B SRC SEL"), "IN 1");

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
        volumeSet = aCtx->volume * 1312U / (aCtx->volMax - aCtx->volMin);
    }

    ret = mixer_ctl_set_value(
        mixer_get_ctl_by_name(aCtx->mixerHandler, "Linein Mic1 Volume"), 0,
        volumeSet);

    HDF_LOGV("Set Volume: volume=%{public}d, mute=%{public}d, "
             "volumeSet=%{public}d, ret=%{public}d",
             aCtx->volume, aCtx->mute, volumeSet, ret);

    return ret;
}

static enum pcm_format ConvertFormatToAlsa(enum AudioFormat format)
{
    switch (format) {
        case AUDIO_FORMAT_PCM_8_BIT:
            return PCM_FORMAT_S8;
        case AUDIO_FORMAT_PCM_16_BIT:
            return PCM_FORMAT_S16_LE;
        case AUDIO_FORMAT_PCM_24_BIT:
            return PCM_FORMAT_S24_LE;
        case AUDIO_FORMAT_PCM_32_BIT:
            return PCM_FORMAT_S32_LE;
        case AUDIO_FORMAT_AAC_MAIN:
        case AUDIO_FORMAT_AAC_LC:
        case AUDIO_FORMAT_AAC_LD:
        case AUDIO_FORMAT_AAC_ELD:
        case AUDIO_FORMAT_AAC_HE_V1:
        case AUDIO_FORMAT_AAC_HE_V2:
        case AUDIO_FORMAT_G711A:
        case AUDIO_FORMAT_G711U:
        case AUDIO_FORMAT_G726:
        default:
            return PCM_FORMAT_INVALID;
    }
}

static struct AlsaCtx *AlsaCtxInstanceGet(void)
{
    if (!s_alsaCtx.initFlag) {
        memset_s(&s_alsaCtx, sizeof(s_alsaCtx), 0, sizeof(s_alsaCtx));
        s_alsaCtx.pcmHandler = NULL;
        s_alsaCtx.volMin = 0;
        s_alsaCtx.volMax = 100U;
        s_alsaCtx.volume = s_alsaCtx.volMax;
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
    (void)memcpy_s(&config, sioef(config), &aCtx->config, sizeof(config));
    struct pcm *pcm = pcm_open(sound_card_id, sound_dev_id, PCM_IN, &config);
    if (!pcm_is_ready(pcm)) {
        HDF_LOGE("Error: %{public}s() Cannot open PCM_IN(card %{public}d, "
                 "device %{public}d): %{public}s",
                 __func__, sound_card_id, sound_dev_id, pcm_get_error(pcm));
        pcm_close(pcm);

        return -1;
    }

    aCtx->pcmHandler = pcm;

    HDF_LOGI("Open Alsa PCM_IN Success.");

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

static int CheckHwParam(struct AudioHwCaptureParam *handleData)
{
    if (handleData == NULL) {
        HDF_LOGE("Error: handleData is NULL!");
        return -1;
    }

    if (handleData->frameCaptureMode.attrs.channelCount != 1U &&
        handleData->frameCaptureMode.attrs.channelCount != 2U) {
        HDF_LOGE("Error: Unsupported channel count: %{public}d",
                 handleData->frameCaptureMode.attrs.channelCount);
        return -1;
    }

    if (handleData->frameCaptureMode.attrs.format == AUDIO_FORMAT_PCM_8_BIT &&
        handleData->frameCaptureMode.attrs.sampleRate == 8000U)
        return -1;

    if (handleData->frameCaptureMode.attrs.format == AUDIO_FORMAT_PCM_32_BIT &&
        handleData->frameCaptureMode.attrs.sampleRate == 11025U)
        return -1;

    return 0;
}

static int AlsaConfig(struct AlsaCtx *aCtx, struct AudioHwCaptureParam *handleData)
{
    struct pcm_config config;
    enum pcm_format format;

    if (CheckHwParam(handleData)) {
        return -1;
    }

    format = ConvertFormatToAlsa(handleData->frameCaptureMode.attrs.format);
    if (PCM_FORMAT_INVALID == format) {
        HDF_LOGE("Error: Unsupported format: %{public}d", handleData->frameCaptureMode.attrs.format);
        return -1;
    }

    memset_s(&config, sizeof(config), 0, sizeof(config));

    config.channels = handleData->frameCaptureMode.attrs.channelCount;
    config.rate = handleData->frameCaptureMode.attrs.sampleRate;
    config.format = format;
    config.period_count = handleData->frameCaptureMode.periodCount; // 4
    config.period_size = handleData->frameCaptureMode.periodSize;   // 1024

#ifdef PLEASURE_HATS
    if (config.rate == 12000) {
        config.rate = 8000;
    } else if (config.rate == 24000) {
        config.rate = 22050;
    } else if (config.rate == 64000) {
        config.rate = 48000;
    } else if (config.rate == 96000) {
        config.rate = 48000;
    }

    if (config.format == PCM_FORMAT_S24_LE) {
        config.format = PCM_FORMAT_S16_LE;
    }
#endif

    HDF_LOGV("DUMP Alsa Config 1#: channels=%{public}d, rate=%{public}d, "
             "format=%{public}d, period_count=%{public}d, period_size=%{public}d",
             config.channels, config.rate, config.format, config.period_count,
             config.period_size);

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

static int32_t DoCtlCaptureSetPauseStu(const struct DevHandleCapture *handle,
                                       int cmdId,
                                       struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    if (!aCtx->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    if (pcm_ioctl(aCtx->pcmHandler, SNDRV_PCM_IOCTL_PAUSE,
                  handleData->captureMode.ctlParam.pause ? 1 : 0) < 0) {
        HDF_LOGE("Error: pcm_ioctl(SNDRV_PCM_IOCTL_PAUSE) failed: %{public}s\n",
                 pcm_get_error(aCtx->pcmHandler));
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureGetVolume(const struct DevHandleCapture *handle,
                                     int cmdId,
                                     struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    handleData->captureMode.ctlParam.volume = aCtx->volume;

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureSetVolume(const struct DevHandleCapture *handle,
                                     int cmdId,
                                     struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    HDF_LOGV("INFO: enter %{public}s(). volume=%{public}f", __func__,
             handleData->captureMode.ctlParam.volume);

    int volume = (int)handleData->captureMode.ctlParam.volume;

    if (volume < aCtx->volMin || volume > aCtx->volMax) {
        HDF_LOGE("Error: Invalid volume: %{public}d", volume);
        return HDF_FAILURE;
    }

    aCtx->volume = volume;

    return AlsaVolumeUpdate(aCtx);
}

static int32_t DoCtlCaptureSetMuteStu(const struct DevHandleCapture *handle,
                                      int cmdId,
                                      struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    aCtx->mute = handleData->captureMode.ctlParam.mute;

    return AlsaVolumeUpdate(aCtx);
}

static int32_t DoCtlCaptureGetMuteStu(const struct DevHandleCapture *handle,
                                      int cmdId,
                                      struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    handleData->captureMode.ctlParam.mute = aCtx->mute;

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureSetGainStu(const struct DevHandleCapture *handle,
                                      int cmdId,
                                      struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    float gain = handleData->captureMode.ctlParam.audioGain.gain;

    if (gain < aCtx->gainMin || gain > aCtx->gainMax) {
        HDF_LOGE("Error: Invalid gain: %{public}f", gain);
        return HDF_FAILURE;
    }

    aCtx->gain = gain;

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureGetGainStu(const struct DevHandleCapture *handle,
                                      int cmdId,
                                      struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    handleData->captureMode.ctlParam.audioGain.gain = aCtx->gain;

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureSceneSelect(const struct DevHandleCapture *handle,
                                       int cmdId,
                                       struct AudioHwCaptureParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureGetGainThreshold(const struct DevHandleCapture *handle, int cmdId,
                                            struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    handleData->captureMode.ctlParam.audioGain.gainMax = aCtx->gainMax;
    handleData->captureMode.ctlParam.audioGain.gainMin = aCtx->gainMin;

    return HDF_SUCCESS;
}

static int32_t DoCtlCaptureGetVolThreshold(const struct DevHandleCapture *handle, int cmdId,
                                           struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    handleData->captureMode.ctlParam.volThreshold.volMax = aCtx->volMax;
    handleData->captureMode.ctlParam.volThreshold.volMin = aCtx->volMin;

    return HDF_SUCCESS;
}

static int32_t HandleCtlCaptureCmd(const struct DevHandleCapture *handle,
                                   int cmdId,
                                   struct AudioHwCaptureParam *handleData)
{
    int32_t ret;

    switch (cmdId) {
        /* setPara: */
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE_CAPTURE:
            ret = DoCtlCaptureSetVolume(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE:
            ret = DoCtlCaptureSetMuteStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE:
            ret = DoCtlCaptureGetMuteStu(handle, cmdId, handleData);
            break;
        /* getPara: */
        case AUDIODRV_CTL_IOCTL_ELEM_READ_CAPTURE:
            ret = DoCtlCaptureGetVolume(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE_CAPTURE:
            ret = DoCtlCaptureSetGainStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAIN_READ_CAPTURE:
            ret = DoCtlCaptureGetGainStu(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_SCENESELECT_CAPTURE:
            ret = DoCtlCaptureSceneSelect(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_CAPTURE:
            ret = DoCtlCaptureGetGainThreshold(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_CAPTURE:
            ret = DoCtlCaptureGetVolThreshold(handle, cmdId, handleData);
            break;
        default:
            HDF_LOGE("Error: Ctl Mode not support!");
            ret = HDF_FAILURE;
            break;
    }

    return ret;
}

static int32_t DoOutputCaptureHwParams(const struct DevHandleCapture *handle,
                                       int cmdId,
                                       struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    if (AlsaConfig(aCtx, handleData) < 0) {
        HDF_LOGE("Error: in %{public}s, AlsaConfig() failed.", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureRead(const struct DevHandleCapture *handle,
                                   int cmdId,
                                   struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    if (!aCtx->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    uint32_t dataSize = 8192; // See to 'buffer_size' in audio_policy_config.xml

    if (!handleData->frameCaptureMode.buffer) {
        return HDF_FAILURE;
    }

    if (pcm_read(aCtx->pcmHandler, handleData->frameCaptureMode.buffer,
                 dataSize) < 0) {
        HDF_LOGE("Error: pcm_read() failed: %{public}s\n",
                 pcm_get_error(aCtx->pcmHandler));
        return HDF_FAILURE;
    }

    handleData->frameCaptureMode.bufferSize = dataSize;
    handleData->frameCaptureMode.bufferFrameSize =
        pcm_bytes_to_frames(aCtx->pcmHandler, dataSize);

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureStartPrepare(const struct DevHandleCapture *handle, int cmdId,
                                           struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    AlsaOpen(aCtx);

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureOpen(const struct DevHandleCapture *handle,
                                   int cmdId,
                                   const struct AudioHwCaptureParam *handleData)
{
    HDF_LOGV("INFO: enter %{public}s()", __func__);

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureClose(const struct DevHandleCapture *handle, int cmdId,
                                    const struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    if (!aCtx->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    AlsaClose(aCtx);

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureStop(const struct DevHandleCapture *handle,
                                   int cmdId,
                                   struct AudioHwCaptureParam *handleData)
{
    struct AlsaCtx *aCtx = (struct AlsaCtx *)handle->object;

    HDF_LOGV("INFO: enter %{public}s()", __func__);

    if (!aCtx->pcmHandler) {
        HDF_LOGE("Error: in %{public}s, Pcm is not opened!", __func__);
        return HDF_FAILURE;
    }

    AlsaClose(aCtx);

    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureReqMmapBuffer(const struct DevHandleCapture *handle, int cmdId,
                                            struct AudioHwCaptureParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t DoOutputCaptureGetMmapPosition(const struct DevHandleCapture *handle, int cmdId,
                                              struct AudioHwCaptureParam *handleData)
{
    return HDF_SUCCESS;
}

static int32_t HandleOutputCaptureCmd(const struct DevHandleCapture *handle,
                                      int cmdId,
                                      struct AudioHwCaptureParam *handleData)
{
    int32_t ret;

    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
            ret = DoOutputCaptureHwParams(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_READ:
            ret = DoOutputCaptureRead(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_START_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE:
            ret = DoOutputCaptureStartPrepare(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE:
            ret = DoOutputCaptureClose(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN:
            ret = DoOutputCaptureOpen(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE:
            ret = DoOutputCaptureStop(handle, cmdId, handleData);
            break;
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE:
            ret = DoCtlCaptureSetPauseStu(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER_CAPTURE:
            ret = DoOutputCaptureReqMmapBuffer(handle, cmdId, handleData);
            break;
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION_CAPTURE:
            ret = DoOutputCaptureGetMmapPosition(handle, cmdId, handleData);
            break;
        default:
            HDF_LOGE("Output Mode not support!");
            ret = HDF_FAILURE;
            break;
    }

    return ret;
}

/************************************************************************************
Exported Interface
*************************************************************************************/

/* CreatCapture for Bind handle */
struct DevHandleCapture *AudioBindServiceCapture(const char *name)
{
    struct DevHandleCapture *handle = NULL;
    if (!name) {
        HDF_LOGE("service name NULL!");
        return NULL;
    }
    handle =
        (struct DevHandleCapture *)calloc(1, sizeof(struct DevHandleCapture));
    if (handle == NULL) {
        HDF_LOGE("Failed to OsalMemCalloc handle");
        return NULL;
    }

    handle->object = AlsaCtxInstanceGet();

    HDF_LOGV("BIND SERVICE SUCCESS: %{public}s, %{public}p", name, handle);

    return handle;
}

void AudioCloseServiceCapture(const struct DevHandleCapture *handle)
{
    if (!handle || !handle->object) {
        HDF_LOGE("Capture handle or handle->object is NULL");
        return;
    }

    free((void *)handle);

    HDF_LOGV("CLOSE SERVICE SUCCESS: %{public}p", handle);

    return;
}

int32_t AudioInterfaceLibModeCapture(const struct DevHandleCapture *handle,
                                     struct AudioHwCaptureParam *handleData,
                                     int cmdId)
{
    if (!handle || !handle->object || !handleData) {
        HDF_LOGE("paras is NULL!");
        return HDF_FAILURE;
    }

    switch (cmdId) {
        case AUDIO_DRV_PCM_IOCTL_HW_PARAMS:
        case AUDIO_DRV_PCM_IOCTL_READ:
        case AUDIO_DRV_PCM_IOCTRL_START_CAPTURE:
        case AUDIO_DRV_PCM_IOCTRL_STOP_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_PREPARE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_PAUSE_WRITE_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_MMAP_BUFFER_CAPTURE:
        case AUDIO_DRV_PCM_IOCTL_MMAP_POSITION_CAPTURE:
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_OPEN:
        case AUDIO_DRV_PCM_IOCTRL_CAPTURE_CLOSE:
            return HandleOutputCaptureCmd(handle, cmdId, handleData);
        case AUDIODRV_CTL_IOCTL_ELEM_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_ELEM_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_MUTE_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_MUTE_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAIN_WRITE_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAIN_READ_CAPTURE:
        case AUDIODRV_CTL_IOCTL_SCENESELECT_CAPTURE:
        case AUDIODRV_CTL_IOCTL_GAINTHRESHOLD_CAPTURE:
        case AUDIODRV_CTL_IOCTL_VOL_THRESHOLD_CAPTURE:
            return HandleCtlCaptureCmd(handle, cmdId, handleData);
        default:
            HDF_LOGE("Error: Mode Error!");
            break;
    }

    return HDF_ERR_NOT_SUPPORT;
}
