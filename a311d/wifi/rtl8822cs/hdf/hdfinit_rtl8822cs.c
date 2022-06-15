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

#include "hdf_wifi_product.h"
#include "osal_time.h"
#include "wifi_mac80211_ops.h"
#include "net_device.h"
#include "net_adapter.h"
#include "hdf_wlan_utils.h"
#include <drv_types.h>

#define HDF_LOG_TAG RTL8822CS

int32_t InitRTL8822csChip(struct HdfWlanDevice *device)
{
    (void)device;
    HDF_LOGE("InitRTL8822csChip\n");
    return HDF_SUCCESS;
}

int32_t DeInitRTL8822csChip(struct HdfWlanDevice *device)
{
    (void)device;
    HDF_LOGE("DeInitRTL8822csChip\n");
    return HDF_SUCCESS;
}

int32_t RTL8822csInit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice)
{
    (void)chipDriver;

    if (InitNetdev(netDevice) != HDF_SUCCESS) {
        HDF_LOGE("InitNetdev fail!\n");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RTL8822csDeInit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice)
{
    int32_t ret;
    (void)chipDriver;

    ret = DeinitNetdev();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DeinitNetdev fail, ret = %d\n", ret);
    }

    ret = DeinitP2pNetDev();
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("DeinitP2pNetDev fail, ret = %d\n", ret);
    }

    return HDF_SUCCESS;
}
