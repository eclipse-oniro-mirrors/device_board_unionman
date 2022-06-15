/*
 * Copyright (C) 2022 Unionman Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "securectype.h"
#include "securec.h"
#include "net_device.h"
#include "hdf_device_desc.h"
#include "hdf_wifi_product.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "hdf_wlan_chipdriver_manager.h"
#include "wifi_module.h"

#define HDF_LOG_TAG RTL8822CS

int32_t InitRTL8822csChip(struct HdfWlanDevice *device);
int32_t DeInitRTL8822csChip(struct HdfWlanDevice *device);
int32_t RTL8822csDeInit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice);
int32_t RTL8822csInit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice);

void HiMac80211Init(struct HdfChipDriver *chipDriver);

static const char * const RTL8822CS_DRIVER_NAME = "rtl882x";

static struct HdfChipDriver *BuildRTL8822csDriver(struct HdfWlanDevice *device, uint8_t ifIndex)
{
    struct HdfChipDriver *specificDriver = NULL;
    if (device == NULL) {
        HDF_LOGE("%s fail : channel is NULL", __func__);
        return NULL;
    }
    (void)device;
    (void)ifIndex;
    specificDriver = (struct HdfChipDriver *)OsalMemCalloc(sizeof(struct HdfChipDriver));
    if (specificDriver == NULL) {
        HDF_LOGE("%s fail: OsalMemCalloc fail!", __func__);
        return NULL;
    }
    if (memset_s(specificDriver, sizeof(struct HdfChipDriver), 0, sizeof(struct HdfChipDriver)) != EOK) {
        HDF_LOGE("%s fail: memset_s fail!", __func__);
        OsalMemFree(specificDriver);
        return NULL;
    }

    if (strcpy_s(specificDriver->name, MAX_WIFI_COMPONENT_NAME_LEN, RTL8822CS_DRIVER_NAME) != EOK) {
        HDF_LOGE("%s fail : strcpy_s fail", __func__);
        OsalMemFree(specificDriver);
        return NULL;
    }
    specificDriver->init = RTL8822csInit;
    specificDriver->deinit = RTL8822csDeInit;

    HiMac80211Init(specificDriver);

    return specificDriver;
}

static void ReleaseRTL8822csDriver(struct HdfChipDriver *chipDriver)
{
    if (chipDriver == NULL) {
        return;
    }
    if (strcmp(chipDriver->name, RTL8822CS_DRIVER_NAME) != 0) {
        HDF_LOGE("%s:Not my driver!", __func__);
        return;
    }
    OsalMemFree(chipDriver);
}

static uint8_t GetRTL8822csGetMaxIFCount(struct HdfChipDriverFactory *factory)
{
    (void)factory;
    return 1;
}

/* rtl8822cs's register */
static int32_t HDFWlanRegDriverFactory(void)
{
    static struct HdfChipDriverFactory tmpFactory = { 0 };
    struct HdfChipDriverManager *driverMgr = NULL;
    driverMgr = HdfWlanGetChipDriverMgr();
    if (driverMgr == NULL) {
        return HDF_FAILURE;
    }
    tmpFactory.driverName = RTL8822CS_DRIVER_NAME;
    tmpFactory.GetMaxIFCount = GetRTL8822csGetMaxIFCount;
    tmpFactory.InitChip = InitRTL8822csChip;
    tmpFactory.DeinitChip = DeInitRTL8822csChip;
    tmpFactory.Build = BuildRTL8822csDriver;
    tmpFactory.Release = ReleaseRTL8822csDriver;
    tmpFactory.ReleaseFactory = NULL;
    if (driverMgr->RegChipDriver(&tmpFactory) != HDF_SUCCESS) {
        HDF_LOGE("%s fail: driverMgr is NULL!", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t HdfWlanChipDriverInit(struct HdfDeviceObject *device)
{
    (void)device;
    return HDFWlanRegDriverFactory();
}

static int HdfWlanDriverBind(struct HdfDeviceObject *dev)
{
    (void)dev;
    return HDF_SUCCESS;
}

static void HdfWlanChipRelease(struct HdfDeviceObject *object)
{
    (void)object;
}

struct HdfDriverEntry g_hdfHisiChipEntry = {
    .moduleVersion = 1,
    .Bind = HdfWlanDriverBind,
    .Init = HdfWlanChipDriverInit,
    .Release = HdfWlanChipRelease,
    .moduleName = "HDF_WLAN_CHIPS"
};

HDF_INIT(g_hdfHisiChipEntry);
