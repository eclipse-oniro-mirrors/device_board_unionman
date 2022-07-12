/*
 * Copyright (c) 2022 Unionman Technology Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "audio_core.h"
#include "audio_driver_log.h"
#include "a311d_dma_ops.h"

#define HDF_LOG_TAG a311d_dma_adapter

struct AudioDmaOps g_dmaDeviceOps = {
    .DmaBufAlloc        = A311DAudioDmaBufAlloc,
    .DmaBufFree         = A311DAudioDmaBufFree,
    .DmaRequestChannel  = A311DAudioDmaRequestChannel,
    .DmaConfigChannel   = A311DAudioDmaConfigChannel,
    .DmaPrep            = A311DAudioDmaPrep,
    .DmaSubmit          = A311DAudioDmaSubmit,
    .DmaPending         = A311DAudioDmaPending,
    .DmaPause           = A311DAudioDmaPause,
    .DmaResume          = A311DAudioDmaResume,
    .DmaPointer         = A311DAudioDmaPointer,
};

struct PlatformData g_platformData = {
    .PlatformInit       = A311DAudioDmaDeviceInit,
    .ops                = &g_dmaDeviceOps,
};

/* HdfDriverEntry implementations */
static int32_t DmaDriverBind(struct HdfDeviceObject *device)
{
    struct PlatformHost *platformHost = NULL;

    AUDIO_DRIVER_LOG_DEBUG("");

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    platformHost = (struct PlatformHost *)OsalMemCalloc(sizeof(*platformHost));
    if (platformHost == NULL) {
        AUDIO_DRIVER_LOG_ERR("malloc host fail!");
        return HDF_FAILURE;
    }

    platformHost->device = device;
    device->service = &platformHost->service;

    AUDIO_DRIVER_LOG_DEBUG("success!");
    return HDF_SUCCESS;
}

static int32_t DmaGetServiceName(const struct HdfDeviceObject *device)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("para is NULL.");
        return HDF_FAILURE;
    }

    node = device->property;
    if (node == NULL) {
        AUDIO_DRIVER_LOG_ERR("node is NULL.");
        return HDF_FAILURE;
    }

    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        AUDIO_DRIVER_LOG_ERR("get drsops object instance fail!");
        return HDF_FAILURE;
    }

    ret = drsOps->GetString(node, "serviceName", &g_platformData.drvPlatformName, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("read serviceName fail!");
        return ret;
    }
    AUDIO_DRIVER_LOG_DEBUG("success!");

    return HDF_SUCCESS;
}

static int32_t DmaDriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    AUDIO_DRIVER_LOG_DEBUG("");

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    ret = DmaGetServiceName(device);
    if (ret !=  HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("get service name fail.");
        return ret;
    }
    OsalMutexInit(&g_platformData.renderBufInfo.buffMutex);
    OsalMutexInit(&g_platformData.captureBufInfo.buffMutex);
    g_platformData.platformInitFlag = false;
    ret = AudioSocRegisterPlatform(device, &g_platformData);
    if (ret !=  HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("register dai fail.");
        return ret;
    }

    AUDIO_DRIVER_LOG_DEBUG("success!");
    return HDF_SUCCESS;
}

static void DmaDriverRelease(struct HdfDeviceObject *device)
{
    struct PlatformHost *platformHost = NULL;

    AUDIO_DRIVER_LOG_DEBUG("");

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL");
        return;
    }

    platformHost = (struct PlatformHost *)device->service;
    if (platformHost == NULL) {
        AUDIO_DRIVER_LOG_ERR("platformHost is NULL");
        return;
    }
    OsalMutexDestroy(&g_platformData.renderBufInfo.buffMutex);
    OsalMutexDestroy(&g_platformData.captureBufInfo.buffMutex);
    OsalMemFree(platformHost);

    AUDIO_DRIVER_LOG_DEBUG("success!");
}

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_platformDriverEntry = {
    .moduleVersion  = 1,
    .moduleName     = "DMA_A311D",
    .Bind           = DmaDriverBind,
    .Init           = DmaDriverInit,
    .Release        = DmaDriverRelease,
};
HDF_INIT(g_platformDriverEntry);
