#
# Copyright (c) 2022 Unionman Technology Co., Ltd.
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

EXTRA_CFLAGS += -O1 -Wno-unused-variable -Wno-unused-function -Wno-unused
EXTRA_CFLAGS += -Wno-date-time

EXTRA_CFLAGS += -I$(srctree)/drivers/net/wireless/realtek/rtl88x2cs/include
EXTRA_CFLAGS += -I$(srctree)/drivers/net/wireless/realtek/rtl88x2cs/os_dep/linux

EXTRA_LDFLAGS += --strip-debug

EXTRA_CFLAGS += -DDM_ODM_SUPPORT_TYPE=0x04
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -fno-pic
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211
EXTRA_CFLAGS += -DRTW_USE_CFG80211_STA_EVENT
EXTRA_CFLAGS += -DCONFIG_RADIO_WORK
EXTRA_CFLAGS += -DCONFIG_RTL8822CS_WIFI_HDF

HDF_NTEWORK_ADAPTER_INC := \
   -I$(srctree)/../../../../drivers/adapter/khdf/linux/network/include

HDF_FRAMEWORKS_INC := \
   -I$(srctree)/drivers/hdf/framework/core/common/include/host \
   -I$(srctree)/drivers/hdf/framework/core/host/include \
   -I$(srctree)/drivers/hdf/framework/core/manager/include \
   -I$(srctree)/drivers/hdf/framework/core/shared/include \
   -I$(srctree)/drivers/hdf/framework/include \
   -I$(srctree)/drivers/hdf/framework/include/config \
   -I$(srctree)/drivers/hdf/framework/include/core \
   -I$(srctree)/drivers/hdf/framework/include/platform \
   -I$(srctree)/drivers/hdf/framework/include/utils \
   -I$(srctree)/drivers/hdf/framework/support/platform/include \
   -I$(srctree)/drivers/hdf/framework/support/platform/include/platform \
   -I$(srctree)/drivers/hdf/framework/utils/include \
   -I$(srctree)/drivers/hdf/khdf/osal/include \
   -I$(srctree)/drivers/hdf/inner_api/osal/shared \
   -I$(srctree)/drivers/hdf/inner_api/host/shared \
   -I$(srctree)/drivers/hdf/inner_api/utils \
   -I$(srctree)/drivers/hdf/inner_api/core \
   -I$(srctree)/drivers/hdf/khdf/config/include \
   -I$(srctree)/include/hdf \
   -I$(srctree)/include/hdf/osal \
   -I$(srctree)/include/hdf/utils

HDF_WIFI_FRAMEWORKS_INC := \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core/components/eapol \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core/components/softap \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core/components/sta \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core/components/p2p \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/include \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/core/module \
   -I$(srctree)/drivers/hdf/framework/model/network/common/netdevice \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/platform/include \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/platform/include/message \
   -I$(srctree)/drivers/hdf/framework/model/network/wifi/client/include \
   -I$(srctree)/drivers/hdf/framework/include/wifi \
   -I$(srctree)/drivers/hdf/framework/include/net \
   -I$(srctree)/drivers/hdf/frameworks/model/network/wifi/bus

HDF_WIFI_ADAPTER_INC := \
   -I$(srctree)/drivers/hdf/khdf/network/include

HDF_WIFI_VENDOR_INC := \
   -I$(srctree)/drivers/hdf/wifi/driver/core

SECURE_LIB_INC := \
   -I$(srctree)/bounds_checking_function/include

EXTRA_CFLAGS += \
    $(HDF_FRAMEWORKS_INC) \
    $(HDF_WIFI_FRAMEWORKS_INC) \
    $(SECURE_LIB_INC) \
    $(HDF_WIFI_ADAPTER_INC) \
    $(HDF_NTEWORK_ADAPTER_INC)

MODULE_NAME = 88x2cs

$(MODULE_NAME)-y += \
    hdf/hdf_driver_register.o \
    hdf/hdfinit_rtl8822cs.o \
    hdf/net_adapter.o \
    hdf/wal_mac80211.o

obj-$(CONFIG_RTL8822CS_WIFI_HDF) := $(MODULE_NAME).o

