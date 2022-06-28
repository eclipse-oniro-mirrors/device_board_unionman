/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_host.h"
#include "audio_control.h"
#include "audio_dai_if.h"
#include "audio_dai_base.h"
#include "audio_driver_log.h"
#include "osal_io.h"
#include "audio_stream_dispatch.h"

#include "axg_snd_card.h"
#include "a311d_dai_ops.h"

#define HDF_LOG_TAG a311d_dai_ops

static struct axg_fifo *g_fifoDev[2];
static struct axg_tdm_iface *g_ifaceDev;

int32_t A311DDeviceInit(struct AudioCard *audioCard, const struct DaiDevice *dai)
{
    int ret;
    struct DaiData *data;

    AUDIO_DRIVER_LOG_DEBUG("");
    if (dai == NULL || dai->device == NULL || dai->devDaiName == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }
    if (dai == NULL || dai->devData == NULL) {
        AUDIO_DRIVER_LOG_ERR("dai host is NULL.");
        return HDF_FAILURE;
    }
    data = dai->devData;
    if (DaiSetConfigInfo(data) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("set config info fail.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("numControls:%d", data->numControls);

    ret = AudioAddControls(audioCard, data->controls, data->numControls);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add controls failed.", data->numControls);
        return HDF_FAILURE;
    }

    if (data->daiInitFlag == true) {
        AUDIO_DRIVER_LOG_INFO("dai already inited.");
        return HDF_SUCCESS;
    }

    if (meson_axg_snd_card_init()) {
        AUDIO_DRIVER_LOG_ERR("axg_snd_card_init() failed.");
        return HDF_FAILURE;
    }

    g_fifoDev[AUDIO_CAPTURE_STREAM] = meson_axg_default_fifo_get(1);
    g_fifoDev[AUDIO_RENDER_STREAM] = meson_axg_default_fifo_get(0);

    if (!g_fifoDev[AUDIO_CAPTURE_STREAM] || !g_fifoDev[AUDIO_RENDER_STREAM]) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_fifo_get() failed.");
        return HDF_FAILURE;
    }

    g_ifaceDev = meson_axg_default_tdm_iface_get();
    if (!g_ifaceDev) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_tdm_iface_get() failed.");
        return HDF_FAILURE;
    }

    data->daiInitFlag = true;

    AUDIO_DRIVER_LOG_INFO("success");

    return HDF_SUCCESS;
}

int32_t A311DDeviceReadReg(const struct DaiDevice *dai, uint32_t reg, uint32_t *value)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DDeviceWriteReg(const struct DaiDevice *dai, uint32_t reg, uint32_t value)
{
    AUDIO_DRIVER_LOG_DEBUG("");
    return HDF_SUCCESS;
}

int32_t A311DDaiTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device)
{
    int32_t ret = HDF_FAILURE;

    AUDIO_DRIVER_LOG_DEBUG("cmd -> %d", cmd);

    switch (cmd) {
        case AUDIO_DRV_PCM_IOCTL_RENDER_START:
        case AUDIO_DRV_PCM_IOCTL_RENDER_RESUME:
            ret = meson_axg_tdm_iface_prepare(g_ifaceDev, AXG_TDM_IFACE_STREAM_PLAYBACK);
            ret |= meson_axg_tdm_iface_start(g_ifaceDev, AXG_TDM_IFACE_STREAM_PLAYBACK);
            break;
        case AUDIO_DRV_PCM_IOCTL_RENDER_STOP:
        case AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE:
            ret = meson_axg_tdm_iface_unprepare(g_ifaceDev, AXG_TDM_IFACE_STREAM_PLAYBACK);
            ret |= meson_axg_tdm_iface_stop(g_ifaceDev, AXG_TDM_IFACE_STREAM_PLAYBACK);
            break;
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_START:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME:
            ret = meson_axg_tdm_iface_prepare(g_ifaceDev, AXG_TDM_IFACE_STREAM_CAPTURE);
            ret |= meson_axg_tdm_iface_start(g_ifaceDev, AXG_TDM_IFACE_STREAM_CAPTURE);
            break;
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE:
            ret = meson_axg_tdm_iface_unprepare(g_ifaceDev, AXG_TDM_IFACE_STREAM_CAPTURE);
            ret |= meson_axg_tdm_iface_stop(g_ifaceDev, AXG_TDM_IFACE_STREAM_CAPTURE);
            break;
        default:
            AUDIO_DRIVER_LOG_ERR("invalid cmd");
            break;
    }

    AUDIO_DRIVER_LOG_DEBUG(" cmd -> %d, ret -> %d", cmd, ret);

    return ret;
}

int32_t A311DDaiStartup(const struct AudioCard *card, const struct DaiDevice *device)
{
    AUDIO_DRIVER_LOG_DEBUG("");

    (void)card;
    (void)device;

    return HDF_SUCCESS;
}

static int32_t FormatToBitWidth(enum AudioFormat format, uint32_t *bitWidth, uint32_t *phyBitWidth)
{
    switch (format) {
        case AUDIO_FORMAT_PCM_32_BIT:
            *bitWidth = BIT_WIDTH32;
            *phyBitWidth = BIT_WIDTH32;
            break;
        case AUDIO_FORMAT_PCM_24_BIT:
            *bitWidth = BIT_WIDTH24;
            *phyBitWidth = BIT_WIDTH32;
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            *bitWidth = BIT_WIDTH16;
            *phyBitWidth = BIT_WIDTH16;
            break;
        case AUDIO_FORMAT_PCM_8_BIT:
            *bitWidth = BIT_WIDTH8;
            *phyBitWidth = BIT_WIDTH8;
            break;
        default:
            return -1;
    }

    return 0;
}

int32_t A311DDaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    int32_t ret;
    uint32_t bitWidth, phyBitWidth;
    enum axg_tdm_iface_stream tdmStreamType;
    struct axg_pcm_hw_params hwParam = {0};
    struct axg_fifo *fifo;

    if (card == NULL || card->rtd == NULL || card->rtd->cpuDai == NULL ||
        param == NULL || param->cardServiceName == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is nullptr.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("streamType: %d", param->streamType);

    if (FormatToBitWidth(param->format, &bitWidth, &phyBitWidth)) {
        AUDIO_DRIVER_LOG_ERR("FormatToBitWidth() failed.");
        return HDF_FAILURE;
    }

    struct DaiData *data = DaiDataFromCard(card);

    tdmStreamType = (param->streamType == AUDIO_RENDER_STREAM) ?
                            AXG_TDM_IFACE_STREAM_PLAYBACK : AXG_TDM_IFACE_STREAM_CAPTURE;
    fifo = g_fifoDev[param->streamType];

    hwParam.bit_width = bitWidth;
    hwParam.channels = param->channels;
    hwParam.physical_width = phyBitWidth;
    hwParam.rate = param->rate;

    ret = meson_axg_fifo_pcm_close(fifo);
    ret |= meson_axg_fifo_pcm_open(fifo);
    if (ret) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_fifo_pcm_open() failed.");
        return HDF_FAILURE;
    }

    if (meson_axg_fifo_dai_hw_params(fifo, hwParam.bit_width,
                                     hwParam.physical_width)) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_fifo_dai_hw_params() failed.");
        return HDF_FAILURE;
    }

    if (meson_axg_tdm_iface_hw_params(g_ifaceDev, tdmStreamType, &hwParam)) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_tdm_iface_hw_params() failed.");
        return HDF_FAILURE;
    }

    data->pcmInfo.channels = param->channels;
    data->pcmInfo.bitWidth = bitWidth;
    data->pcmInfo.rate = param->rate;
    data->pcmInfo.streamType = param->streamType;

    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}
