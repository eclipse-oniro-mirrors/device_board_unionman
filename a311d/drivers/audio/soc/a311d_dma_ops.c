/*
 * Copyright (C) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_dma.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/suspend.h>
#include "sound/memalloc.h"

#include "axg_snd_card.h"

#include "audio_platform_base.h"
#include "osal_io.h"
#include "osal_uaccess.h"
#include "audio_driver_log.h"
#include "a311d_dma_ops.h"

#define HDF_LOG_TAG a311d_dma_ops

static struct axg_fifo *g_fifoDev[2]; // [0]: capture, [1]: playback

int32_t A311DAudioDmaDeviceInit(const struct AudioCard *card, const struct PlatformDevice *platform)
{
    struct PlatformData *data = NULL;

    if (meson_axg_snd_card_init()) {
        AUDIO_DRIVER_LOG_ERR("axg_snd_card_init() failed.");
        return HDF_FAILURE;
    }

    g_fifoDev[AUDIO_CAPTURE_STREAM] = meson_axg_default_fifo_get(1);
    g_fifoDev[AUDIO_RENDER_STREAM] = meson_axg_default_fifo_get(0);

    if (!g_fifoDev[AUDIO_CAPTURE_STREAM] || !g_fifoDev[AUDIO_RENDER_STREAM]) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_fifo_get failed.");
        return HDF_FAILURE;
    }

    data = PlatformDataFromCard(card);
    if (data == NULL) {
        AUDIO_DRIVER_LOG_ERR("PlatformDataFromCard failed.");
        return HDF_FAILURE;
    }

    if (data->platformInitFlag == true) {
        AUDIO_DRIVER_LOG_INFO("platform already inited.");
        return HDF_SUCCESS;
    }

    data->platformInitFlag = true;

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t A311DAudioDmaBufAlloc(struct PlatformData *data, const enum AudioStreamType streamType)
{
    uint32_t cirBufMax;
    struct axg_fifo *fifo;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    if (data == NULL) {
        AUDIO_DRIVER_LOG_ERR("data is null");
        return HDF_FAILURE;
    }

    fifo = g_fifoDev[streamType];
    cirBufMax = (streamType == AUDIO_CAPTURE_STREAM) ? data->captureBufInfo.cirBufMax : data->renderBufInfo.cirBufMax;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d, cirBufMax = %u", streamType, cirBufMax);

    if (cirBufMax > fifo->dma_area) {
        AUDIO_DRIVER_LOG_ERR("requested buffer size(%u) is larger than dma_area(%u).",
                             cirBufMax, fifo->dma_area);
        return HDF_FAILURE;
    }

    if (streamType == AUDIO_CAPTURE_STREAM) {
        data->captureBufInfo.virtAddr = (uint32_t *)fifo->dma_vaddr;
        data->captureBufInfo.phyAddr = fifo->dma_addr;
    } else {
        data->renderBufInfo.virtAddr = (uint32_t *)fifo->dma_vaddr;
        data->renderBufInfo.phyAddr = fifo->dma_addr;
    }

    AUDIO_DRIVER_LOG_DEBUG("success.");

    return HDF_SUCCESS;
}

int32_t A311DAudioDmaBufFree(struct PlatformData *data, const enum AudioStreamType streamType)
{
    (void)data;
    AUDIO_DRIVER_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t A311DAudioDmaRequestChannel(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    (void)data;
    AUDIO_DRIVER_LOG_DEBUG("sucess");
    return HDF_SUCCESS;
}

int32_t A311DAudioDmaConfigChannel(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    uint32_t period, cir_buf_size;
    struct axg_fifo *fifo;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    fifo = g_fifoDev[streamType];

    if (streamType == AUDIO_RENDER_STREAM) {
        period = data->renderBufInfo.periodSize;
        cir_buf_size = data->renderBufInfo.cirBufSize;
    } else {
        period = data->captureBufInfo.periodSize;
        cir_buf_size = data->captureBufInfo.cirBufSize;
    }

    meson_axg_fifo_pcm_hw_free(fifo);
    if (meson_axg_fifo_pcm_hw_params(fifo, period, cir_buf_size)) {
        AUDIO_DRIVER_LOG_ERR("meson_axg_fifo_pcm_hw_params(%u, %u) failed.\n",
                             period, cir_buf_size);
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_INFO("success. stream=%d, period=%u, cir_buf_size=%u",
                          streamType, period, cir_buf_size);
    return HDF_SUCCESS;
}

static inline uint32_t BytesToFrames(uint32_t frameBytes, uint32_t size)
{
    if (frameBytes == 0) {
        AUDIO_DRIVER_LOG_ERR("input error. frameBits==0");
        return 0;
    }
    return size / frameBytes;
}

int32_t A311DAudioDmaPointer(struct PlatformData *data, const enum AudioStreamType streamType, uint32_t *pointer)
{
    uint32_t currentPointer;
    uint32_t frameBytes;

    currentPointer = meson_axg_fifo_pcm_pointer(g_fifoDev[streamType]);

    frameBytes = (streamType == AUDIO_RENDER_STREAM) ? data->renderPcmInfo.frameSize : data->capturePcmInfo.frameSize;

    *pointer = BytesToFrames(frameBytes, currentPointer);

    return HDF_SUCCESS;
}

int32_t A311DAudioDmaPrep(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    (void)data;
    AUDIO_DRIVER_LOG_DEBUG("sucess");
    return HDF_SUCCESS;
}

int32_t A311DAudioDmaSubmit(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;

    (void)data;
    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    ret = meson_axg_fifo_pcm_prepare(g_fifoDev[streamType]);

    AUDIO_DRIVER_LOG_DEBUG("ret: %d", ret);

    return ret;
}

int32_t A311DAudioDmaPending(struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    ret = meson_axg_fifo_pcm_enable(g_fifoDev[streamType], true);

    AUDIO_DRIVER_LOG_DEBUG("ret: %d", ret);

    return ret;
}

int32_t A311DAudioDmaPause(struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    ret = meson_axg_fifo_pcm_enable(g_fifoDev[streamType], false);

    AUDIO_DRIVER_LOG_DEBUG("success");
    return ret;
}

int32_t A311DAudioDmaResume(const struct PlatformData *data, const enum AudioStreamType streamType)
{
    int32_t ret;

    AUDIO_DRIVER_LOG_DEBUG("streamType = %d", streamType);

    ret = meson_axg_fifo_pcm_enable(g_fifoDev[streamType], true);

    AUDIO_DRIVER_LOG_DEBUG("ret: %d", ret);
    return ret;
}
