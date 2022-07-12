/*
 * Copyright (c) 2022 Unionman Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
 
#include "audio_codec_base.h"
#include "audio_core.h"
#include "audio_driver_log.h"

#include "a311d_codec_ops.h"

#define HDF_LOG_TAG a311d_codec_adapter

struct CodecData g_codecData = {
    .Init   = A311DCodecDeviceInit,
    .Read   = A311DCodecDeviceReadReg,
    .Write  = A311DCodecDeviceWriteReg,
};

struct AudioDaiOps g_codecDaiDeviceOps = {
    .Startup    = A311DCodecDaiStartup,
    .HwParams   = A311DCodecDaiHwParams,
    .Trigger    = A311DCodecDaiTrigger,
};

struct DaiData g_codecDaiData = {
    .DaiInit    = A311DCodecDaiDeviceInit,
    .ops        = &g_codecDaiDeviceOps,
};

/* HdfDriverEntry implementations */
static int32_t CodecDriverBind(struct HdfDeviceObject *device)
{
    struct CodecHost *codecHost;

    AUDIO_DRIVER_LOG_DEBUG("");

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    codecHost = (struct CodecHost *)OsalMemCalloc(sizeof(*codecHost));
    if (codecHost == NULL) {
        AUDIO_DRIVER_LOG_ERR("malloc codecHost fail!");
        return HDF_FAILURE;
    }
    codecHost->device = device;
    device->service = &codecHost->service;

    AUDIO_DRIVER_LOG_DEBUG("success!");

    return HDF_SUCCESS;
}

static int32_t CodecDriverInit(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (CodecGetConfigInfo(device, &g_codecData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    if (CodecSetConfigInfoOfControls(&g_codecData, &g_codecDaiData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    if (CodecGetServiceName(device, &g_codecData.drvCodecName) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    if (CodecGetDaiName(device, &g_codecDaiData.drvDaiName) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    if (AudioRegisterCodec(device, &g_codecData, &g_codecDaiData) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void CodecDriverRelease(struct HdfDeviceObject *device)
{
    struct CodecHost *codecHost;

    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL");
        return;
    }

    if (device->priv != NULL) {
        OsalMemFree(device->priv);
    }
    codecHost = (struct CodecHost *)device->service;
    if (codecHost == NULL) {
        HDF_LOGE("CodecDriverRelease: codecHost is NULL");
        return;
    }
    OsalMemFree(codecHost);
}

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_codecDriverEntry = {
    .moduleVersion  = 1,
    .moduleName     = "CODEC_A311D",
    .Bind           = CodecDriverBind,
    .Init           = CodecDriverInit,
    .Release        = CodecDriverRelease,
};
HDF_INIT(g_codecDriverEntry);
