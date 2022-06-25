EXTRA_CFLAGS += $(USER_EXTRA_CFLAGS)
EXTRA_CFLAGS += -O1
#EXTRA_CFLAGS += -O3
#EXTRA_CFLAGS += -Wall
#EXTRA_CFLAGS += -Wextra
#EXTRA_CFLAGS += -Werror
#EXTRA_CFLAGS += -pedantic
#EXTRA_CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes

EXTRA_CFLAGS += -Wno-unused-variable
#EXTRA_CFLAGS += -Wno-unused-value
#EXTRA_CFLAGS += -Wno-unused-label
#EXTRA_CFLAGS += -Wno-unused-parameter
EXTRA_CFLAGS += -Wno-unused-function
EXTRA_CFLAGS += -Wno-unused
#EXTRA_CFLAGS += -Wno-uninitialized

GCC_VER_49 := $(shell echo `$(CC) -dumpversion | cut -f1-2 -d.` \>= 4.9 | bc )
ifeq ($(GCC_VER_49),1)
EXTRA_CFLAGS += -Wno-date-time	# Fix compile error && warning on gcc 4.9 and later
endif

EXTRA_CFLAGS += -I$(src)/include

EXTRA_LDFLAGS += --strip-debug

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

ifeq ($(CONFIG_RTL8822CS_WIFI_HDF),y)
EXTRA_CFLAGS += -DCONFIG_RTL8822CS_WIFI_HDF
else
EXTRA_CFLAGS += -DCONFIG_CONCURRENT_MODE
endif

CONFIG_AUTOCFG_CP = n

########################## WIFI IC ############################
CONFIG_MULTIDRV = n
CONFIG_RTL8188E = n
CONFIG_RTL8812A = n
CONFIG_RTL8821A = n
CONFIG_RTL8192E = n
CONFIG_RTL8723B = n
CONFIG_RTL8814A = n
CONFIG_RTL8723C = n
CONFIG_RTL8188F = n
CONFIG_RTL8188GTV = n
CONFIG_RTL8822B = n
CONFIG_RTL8723D = n
CONFIG_RTL8821C = n
CONFIG_RTL8710B = n
CONFIG_RTL8192F = n
CONFIG_RTL8822C = y
CONFIG_RTL8814B = n
CONFIG_RTL8814C = n
CONFIG_RTL8723F = n
######################### Interface ###########################
CONFIG_USB_HCI = n
CONFIG_PCI_HCI = n
CONFIG_SDIO_HCI = y
CONFIG_GSPI_HCI = n
########################## Features ###########################
CONFIG_AP_MODE = y
CONFIG_P2P = y
CONFIG_MP_INCLUDED = y
CONFIG_POWER_SAVING = n
CONFIG_IPS_MODE = default
CONFIG_LPS_MODE = default
CONFIG_USB_AUTOSUSPEND = n
CONFIG_HW_PWRP_DETECTION = n
CONFIG_BT_COEXIST = y
CONFIG_WAPI_SUPPORT = n
CONFIG_EFUSE_CONFIG_FILE = y
CONFIG_EXT_CLK = n
CONFIG_TRAFFIC_PROTECT = n
CONFIG_LOAD_PHY_PARA_FROM_FILE = y
CONFIG_TXPWR_BY_RATE = y
CONFIG_TXPWR_BY_RATE_EN = y
CONFIG_TXPWR_LIMIT = y
CONFIG_TXPWR_LIMIT_EN = n
CONFIG_RTW_CHPLAN = 0xFFFF
CONFIG_RTW_ADAPTIVITY_EN = disable
CONFIG_RTW_ADAPTIVITY_MODE = normal
CONFIG_80211D = n
CONFIG_SIGNAL_SCALE_MAPPING = n
CONFIG_80211W = y
CONFIG_REDUCE_TX_CPU_LOADING = n
CONFIG_BR_EXT = y
CONFIG_TDLS = n
CONFIG_WIFI_MONITOR = n
CONFIG_MCC_MODE = n
CONFIG_APPEND_VENDOR_IE_ENABLE = n
CONFIG_RTW_NAPI = y
CONFIG_RTW_GRO = y
CONFIG_RTW_NETIF_SG = y
CONFIG_RTW_IPCAM_APPLICATION = n
CONFIG_RTW_REPEATER_SON = n
CONFIG_ICMP_VOQ = n
CONFIG_IP_R_MONITOR = n #arp VOQ and high rate
# user priority mapping rule : tos, dscp
CONFIG_RTW_UP_MAPPING_RULE = tos
CONFIG_RTW_MBO = n
CONFIG_WAKE_ON_BT = n
CONFIG_RTW_NBI = n

########################## Android ###########################
# CONFIG_RTW_ANDROID - 0: no Android, 4/5/6/7/8/9/10/11 : Android version
CONFIG_RTW_ANDROID = 0

ifeq ($(shell test $(CONFIG_RTW_ANDROID) -gt 0; echo $$?), 0)
EXTRA_CFLAGS += -DCONFIG_RTW_ANDROID=$(CONFIG_RTW_ANDROID)
endif

########################## Debug ###########################
CONFIG_RTW_DEBUG = n
# default log level is _DRV_INFO_ = 4,
# please refer to "How_to_set_driver_debug_log_level.doc" to set the available level.
CONFIG_RTW_LOG_LEVEL = 1

# enable /proc/net/rtlxxxx/ debug interfaces
CONFIG_PROC_DEBUG = y

######################## Wake On Lan ##########################
CONFIG_WOWLAN = n
#bit2: deauth, bit1: unicast, bit0: magic pkt.
CONFIG_WAKEUP_TYPE = 0x7
CONFIG_WOW_LPS_MODE = default
#bit0: disBBRF off, #bit1: Wireless remote controller (WRC)
CONFIG_SUSPEND_TYPE = 0
CONFIG_WOW_STA_MIX = n
CONFIG_GPIO_WAKEUP = n
# Please contact with RTK support team first. After getting the agreement from RTK support team, 
# you are just able to modify the CONFIG_WAKEUP_GPIO_IDX with customized requirement.
CONFIG_WAKEUP_GPIO_IDX = default
CONFIG_HIGH_ACTIVE_DEV2HST = n
######### only for USB #########
CONFIG_ONE_PIN_GPIO = n
CONFIG_HIGH_ACTIVE_HST2DEV = n
CONFIG_PNO_SUPPORT = n
CONFIG_PNO_SET_DEBUG = n
CONFIG_AP_WOWLAN = n
######### Notify SDIO Host Keep Power During Syspend ##########
CONFIG_RTW_SDIO_PM_KEEP_POWER = n
###################### MP HW TX MODE FOR VHT #######################
CONFIG_MP_VHT_HW_TX_MODE = n
###################### ROAMING #####################################
CONFIG_LAYER2_ROAMING = y
#bit0: ROAM_ON_EXPIRED, #bit1: ROAM_ON_RESUME, #bit2: ROAM_ACTIVE
CONFIG_ROAMING_FLAG = 0x3
####################### Security Memory ############################
# Define for using dma_declare_coherent_memory DMA API.
# User Must set the SECURITY_MEM_ADDR and SECURITY_MEM_SIZE.
# If SECURITY_MEM_ADDR is NULL, driver will show WARN_ON for notification.
CONFIG_SECURITY_MEM = n
CONFIG_SECURITY_MEM_ADDR = 0
CONFIG_SECURITY_MEM_SIZE = 3686400
###################### Platform Related #######################
CONFIG_PLATFORM_I386_PC = n
CONFIG_PLATFORM_ANDROID_X86 = n
CONFIG_PLATFORM_ANDROID_INTEL_X86 = n
CONFIG_PLATFORM_JB_X86 = n
CONFIG_PLATFORM_ARM_S3C2K4 = n
CONFIG_PLATFORM_ARM_PXA2XX = n
CONFIG_PLATFORM_ARM_S3C6K4 = n
CONFIG_PLATFORM_MIPS_RMI = n
CONFIG_PLATFORM_RTD2880B = n
CONFIG_PLATFORM_MIPS_AR9132 = n
CONFIG_PLATFORM_RTK_DMP = n
CONFIG_PLATFORM_MIPS_PLM = n
CONFIG_PLATFORM_MSTAR389 = n
CONFIG_PLATFORM_MT53XX = n
CONFIG_PLATFORM_ARM_MX51_241H = n
CONFIG_PLATFORM_FS_MX61 = n
CONFIG_PLATFORM_ACTIONS_ATJ227X = n
CONFIG_PLATFORM_TEGRA3_CARDHU = n
CONFIG_PLATFORM_TEGRA4_DALMORE = n
CONFIG_PLATFORM_ARM_TCC8900 = n
CONFIG_PLATFORM_ARM_TCC8920 = n
CONFIG_PLATFORM_ARM_TCC8920_JB42 = n
CONFIG_PLATFORM_ARM_TCC8930_JB42 = n
CONFIG_PLATFORM_ARM_RK2818 = n
CONFIG_PLATFORM_ARM_RK3066 = n
CONFIG_PLATFORM_ARM_RK3188 = n
CONFIG_PLATFORM_ARM_URBETTER = n
CONFIG_PLATFORM_ARM_TI_PANDA = n
CONFIG_PLATFORM_MIPS_JZ4760 = n
CONFIG_PLATFORM_DMP_PHILIPS = n
CONFIG_PLATFORM_MSTAR_TITANIA12 = n
CONFIG_PLATFORM_MSTAR = n
CONFIG_PLATFORM_SZEBOOK = n
CONFIG_PLATFORM_ARM_SUNxI = n
CONFIG_PLATFORM_ARM_SUN6I = n
CONFIG_PLATFORM_ARM_SUN7I = n
CONFIG_PLATFORM_ARM_SUN8I_W3P1 = n
CONFIG_PLATFORM_ARM_SUN8I_W5P1 = n
CONFIG_PLATFORM_ACTIONS_ATM702X = n
CONFIG_PLATFORM_ACTIONS_ATV5201 = n
CONFIG_PLATFORM_ACTIONS_ATM705X = n
CONFIG_PLATFORM_ARM_SUN50IW1P1 = n
CONFIG_PLATFORM_ARM_RTD299X = n
CONFIG_PLATFORM_ARM_LGE = n
CONFIG_PLATFORM_ARM_SPREADTRUM_6820 = n
CONFIG_PLATFORM_ARM_SPREADTRUM_8810 = n
CONFIG_PLATFORM_ARM_WMT = n
CONFIG_PLATFORM_TI_DM365 = n
CONFIG_PLATFORM_MOZART = n
CONFIG_PLATFORM_RTK119X = n
CONFIG_PLATFORM_RTK119X_AM = n
CONFIG_PLATFORM_RTK129X = n
CONFIG_PLATFORM_RTK1319 = n
CONFIG_PLATFORM_RTK390X = n
CONFIG_PLATFORM_NOVATEK_NT72668 = n
CONFIG_PLATFORM_HISILICON = n
CONFIG_PLATFORM_HISILICON_HI3798 = n
CONFIG_PLATFORM_NV_TK1 = n
CONFIG_PLATFORM_NV_TK1_UBUNTU = n
CONFIG_PLATFORM_RTL8197D = n
CONFIG_PLATFORM_AML_S905 = y
CONFIG_PLATFORM_ZTE_ZX296716 = n
########### CUSTOMER ################################
CONFIG_CUSTOMER_HUAWEI_GENERAL = n

CONFIG_DRVEXT_MODULE = n

export TopDIR ?= $(shell pwd)

########### COMMON  #################################
HCI_NAME = sdio

_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/linux/os_intfs.o \
			os_dep/linux/$(HCI_NAME)_intf.o \
			os_dep/linux/$(HCI_NAME)_ops_linux.o \
			os_dep/linux/ioctl_linux.o \
			os_dep/linux/xmit_linux.o \
			os_dep/linux/mlme_linux.o \
			os_dep/linux/recv_linux.o \
			os_dep/linux/ioctl_cfg80211.o \
			os_dep/linux/rtw_cfgvendor.o \
			os_dep/linux/wifi_regd.o \
			os_dep/linux/rtw_android.o \
			os_dep/linux/rtw_proc.o \
			os_dep/linux/nlrtw.o \
			os_dep/linux/rtw_rhashtable.o

ifeq ($(CONFIG_MP_INCLUDED), y)
_OS_INTFS_FILES += os_dep/linux/ioctl_mp.o
endif

ifeq ($(CONFIG_SDIO_HCI), y)
_OS_INTFS_FILES += os_dep/linux/custom_gpio_linux.o
_OS_INTFS_FILES += os_dep/linux/$(HCI_NAME)_ops_linux.o
endif

ifeq ($(CONFIG_GSPI_HCI), y)
_OS_INTFS_FILES += os_dep/linux/custom_gpio_linux.o
_OS_INTFS_FILES += os_dep/linux/$(HCI_NAME)_ops_linux.o
endif


_HAL_INTFS_FILES :=	hal/hal_intf.o \
			hal/hal_com.o \
			hal/hal_com_phycfg.o \
			hal/hal_phy.o \
			hal/hal_dm.o \
			hal/hal_dm_acs.o \
			hal/hal_btcoex_wifionly.o \
			hal/hal_btcoex.o \
			hal/hal_mp.o \
			hal/hal_mcc.o \
			hal/hal_hci/hal_$(HCI_NAME).o \
			hal/led/hal_led.o \
			hal/led/hal_$(HCI_NAME)_led.o


EXTRA_CFLAGS += -I$(src)/platform
_PLATFORM_FILES := platform/platform_ops.o

EXTRA_CFLAGS += -I$(src)/hal/btc

########### HAL_RTL8822B #################################
ifeq ($(CONFIG_RTL8822B), y)
RTL871X := rtl8822b
ifeq ($(CONFIG_USB_HCI), y)
ifeq ($(CONFIG_BT_COEXIST), n)
MODULE_NAME = 8812bu
else
MODULE_NAME = 88x2bu
endif
endif
ifeq ($(CONFIG_PCI_HCI), y)
MODULE_NAME = 88x2be
endif
ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 88x2bs
endif

endif
########### HAL_RTL8821C #################################
ifeq ($(CONFIG_RTL8821C), y)
RTL871X := rtl8821c
ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME = 8821cu
endif
ifeq ($(CONFIG_PCI_HCI), y)
MODULE_NAME = 8821ce
endif
ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 8821cs
endif

endif

########### HAL_RTL8822C #################################
ifeq ($(CONFIG_RTL8822C), y)
RTL871X := rtl8822c
ifeq ($(CONFIG_USB_HCI), y)
ifeq ($(CONFIG_BT_COEXIST), n)
MODULE_NAME = 8812cu
else
MODULE_NAME = 88x2cu
endif
endif
ifeq ($(CONFIG_PCI_HCI), y)
MODULE_NAME = 88x2ce
endif
ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME = 88x2cs
endif

endif

########### AUTO_CFG  #################################

ifeq ($(CONFIG_AUTOCFG_CP), y)

ifeq ($(CONFIG_MULTIDRV), y)
$(shell cp $(TopDIR)/autoconf_multidrv_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
else
ifeq ($(CONFIG_RTL8188E)$(CONFIG_SDIO_HCI),yy)
$(shell cp $(TopDIR)/autoconf_rtl8189e_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
else ifeq ($(CONFIG_RTL8188F)$(CONFIG_SDIO_HCI),yy)
$(shell cp $(TopDIR)/autoconf_rtl8189f_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
else ifeq ($(CONFIG_RTL8723C),y)
$(shell cp $(TopDIR)/autoconf_rtl8723c_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
else
$(shell cp $(TopDIR)/autoconf_$(RTL871X)_$(HCI_NAME)_linux.h $(TopDIR)/include/autoconf.h)
endif
endif

endif

########### END OF PATH  #################################

ifeq ($(CONFIG_AP_MODE), y)
EXTRA_CFLAGS += -DCONFIG_AP_MODE
endif

ifeq ($(CONFIG_P2P), y)
EXTRA_CFLAGS += -DCONFIG_P2P
ifneq ($(CONFIG_AP_MODE), y)
$(error "CONFIG_AP_MODE is required for CONFIG_P2P")
endif
endif

ifeq ($(CONFIG_USB_HCI), y)
ifeq ($(CONFIG_USB_AUTOSUSPEND), y)
EXTRA_CFLAGS += -DCONFIG_USB_AUTOSUSPEND
endif
endif

ifeq ($(CONFIG_MP_INCLUDED), y)
#MODULE_NAME := $(MODULE_NAME)_mp
EXTRA_CFLAGS += -DCONFIG_MP_INCLUDED
endif

ifeq ($(CONFIG_POWER_SAVING), y)
ifneq ($(CONFIG_IPS_MODE), default)
EXTRA_CFLAGS += -DRTW_IPS_MODE=$(CONFIG_IPS_MODE)
endif
ifneq ($(CONFIG_LPS_MODE), default)
EXTRA_CFLAGS += -DRTW_LPS_MODE=$(CONFIG_LPS_MODE)
endif
ifneq ($(CONFIG_WOW_LPS_MODE), default)
EXTRA_CFLAGS += -DRTW_WOW_LPS_MODE=$(CONFIG_WOW_LPS_MODE)
endif
EXTRA_CFLAGS += -DCONFIG_POWER_SAVING
endif

ifeq ($(CONFIG_HW_PWRP_DETECTION), y)
EXTRA_CFLAGS += -DCONFIG_HW_PWRP_DETECTION
endif

ifeq ($(CONFIG_BT_COEXIST), y)
EXTRA_CFLAGS += -DCONFIG_BT_COEXIST
endif

ifeq ($(CONFIG_WAPI_SUPPORT), y)
EXTRA_CFLAGS += -DCONFIG_WAPI_SUPPORT
endif


ifeq ($(CONFIG_EFUSE_CONFIG_FILE), y)
EXTRA_CFLAGS += -DCONFIG_EFUSE_CONFIG_FILE

#EFUSE_MAP_PATH
USER_EFUSE_MAP_PATH ?=
ifneq ($(USER_EFUSE_MAP_PATH),)
EXTRA_CFLAGS += -DEFUSE_MAP_PATH=\"$(USER_EFUSE_MAP_PATH)\"
else ifeq ($(MODULE_NAME), 8189es)
EXTRA_CFLAGS += -DEFUSE_MAP_PATH=\"/system/etc/wifi/wifi_efuse_8189e.map\"
else ifeq ($(MODULE_NAME), 8723bs)
EXTRA_CFLAGS += -DEFUSE_MAP_PATH=\"/system/etc/wifi/wifi_efuse_8723bs.map\"
else
EXTRA_CFLAGS += -DEFUSE_MAP_PATH=\"/system/etc/wifi/wifi_efuse_$(MODULE_NAME).map\"
endif

#WIFIMAC_PATH
USER_WIFIMAC_PATH ?=
ifneq ($(USER_WIFIMAC_PATH),)
EXTRA_CFLAGS += -DWIFIMAC_PATH=\"$(USER_WIFIMAC_PATH)\"
else
EXTRA_CFLAGS += -DWIFIMAC_PATH=\"/data/wifimac.txt\"
endif

endif

ifeq ($(CONFIG_EXT_CLK), y)
EXTRA_CFLAGS += -DCONFIG_EXT_CLK
endif

ifeq ($(CONFIG_TRAFFIC_PROTECT), y)
EXTRA_CFLAGS += -DCONFIG_TRAFFIC_PROTECT
endif

ifeq ($(CONFIG_LOAD_PHY_PARA_FROM_FILE), y)
EXTRA_CFLAGS += -DCONFIG_LOAD_PHY_PARA_FROM_FILE
#EXTRA_CFLAGS += -DREALTEK_CONFIG_PATH_WITH_IC_NAME_FOLDER
EXTRA_CFLAGS += -DREALTEK_CONFIG_PATH=\"/lib/firmware/\"
endif

ifeq ($(CONFIG_TXPWR_BY_RATE), n)
EXTRA_CFLAGS += -DCONFIG_TXPWR_BY_RATE=0
else ifeq ($(CONFIG_TXPWR_BY_RATE), y)
EXTRA_CFLAGS += -DCONFIG_TXPWR_BY_RATE=1
endif
ifeq ($(CONFIG_TXPWR_BY_RATE_EN), n)
EXTRA_CFLAGS += -DCONFIG_TXPWR_BY_RATE_EN=0
else ifeq ($(CONFIG_TXPWR_BY_RATE_EN), y)
EXTRA_CFLAGS += -DCONFIG_TXPWR_BY_RATE_EN=1
else ifeq ($(CONFIG_TXPWR_BY_RATE_EN), auto)
EXTRA_CFLAGS += -DCONFIG_TXPWR_BY_RATE_EN=2
endif

ifeq ($(CONFIG_TXPWR_LIMIT), n)
EXTRA_CFLAGS += -DCONFIG_TXPWR_LIMIT=0
else ifeq ($(CONFIG_TXPWR_LIMIT), y)
EXTRA_CFLAGS += -DCONFIG_TXPWR_LIMIT=1
endif
ifeq ($(CONFIG_TXPWR_LIMIT_EN), n)
EXTRA_CFLAGS += -DCONFIG_TXPWR_LIMIT_EN=0
else ifeq ($(CONFIG_TXPWR_LIMIT_EN), y)
EXTRA_CFLAGS += -DCONFIG_TXPWR_LIMIT_EN=1
else ifeq ($(CONFIG_TXPWR_LIMIT_EN), auto)
EXTRA_CFLAGS += -DCONFIG_TXPWR_LIMIT_EN=2
endif

ifneq ($(CONFIG_RTW_CHPLAN), 0xFFFF)
EXTRA_CFLAGS += -DCONFIG_RTW_CHPLAN=$(CONFIG_RTW_CHPLAN)
endif

ifeq ($(CONFIG_CALIBRATE_TX_POWER_BY_REGULATORY), y)
EXTRA_CFLAGS += -DCONFIG_CALIBRATE_TX_POWER_BY_REGULATORY
endif

ifeq ($(CONFIG_CALIBRATE_TX_POWER_TO_MAX), y)
EXTRA_CFLAGS += -DCONFIG_CALIBRATE_TX_POWER_TO_MAX
endif

ifeq ($(CONFIG_RTW_ADAPTIVITY_EN), disable)
EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_EN=0
else ifeq ($(CONFIG_RTW_ADAPTIVITY_EN), enable)
EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_EN=1
else ifeq ($(CONFIG_RTW_ADAPTIVITY_EN), auto)
EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_EN=2
endif

ifeq ($(CONFIG_RTW_ADAPTIVITY_MODE), normal)
EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_MODE=0
else ifeq ($(CONFIG_RTW_ADAPTIVITY_MODE), carrier_sense)
EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_MODE=1
endif

ifeq ($(CONFIG_80211D), y)
EXTRA_CFLAGS += -DCONFIG_80211D
endif

ifeq ($(CONFIG_SIGNAL_SCALE_MAPPING), y)
EXTRA_CFLAGS += -DCONFIG_SIGNAL_SCALE_MAPPING
endif

ifeq ($(CONFIG_80211W), y)
EXTRA_CFLAGS += -DCONFIG_IEEE80211W
endif

ifeq ($(CONFIG_WOWLAN), y)
EXTRA_CFLAGS += -DCONFIG_WOWLAN -DRTW_WAKEUP_EVENT=$(CONFIG_WAKEUP_TYPE)
EXTRA_CFLAGS += -DRTW_SUSPEND_TYPE=$(CONFIG_SUSPEND_TYPE)
ifeq ($(CONFIG_WOW_STA_MIX), y)
EXTRA_CFLAGS += -DRTW_WOW_STA_MIX
endif
ifeq ($(CONFIG_SDIO_HCI), y)
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif
endif

ifeq ($(CONFIG_AP_WOWLAN), y)
EXTRA_CFLAGS += -DCONFIG_AP_WOWLAN
ifeq ($(CONFIG_AP_MODE), n)
EXTRA_CFLAGS += -DCONFIG_AP_MODE
endif
ifeq ($(CONFIG_SDIO_HCI), y)
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif
endif

ifeq ($(CONFIG_LAYER2_ROAMING), y)
EXTRA_CFLAGS += -DCONFIG_LAYER2_ROAMING -DCONFIG_ROAMING_FLAG=$(CONFIG_ROAMING_FLAG)
endif

ifeq ($(CONFIG_PNO_SUPPORT), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SUPPORT
ifeq ($(CONFIG_PNO_SET_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_PNO_SET_DEBUG
endif
endif

ifeq ($(CONFIG_GPIO_WAKEUP), y)
EXTRA_CFLAGS += -DCONFIG_GPIO_WAKEUP
ifeq ($(CONFIG_ONE_PIN_GPIO), y)
EXTRA_CFLAGS += -DCONFIG_RTW_ONE_PIN_GPIO
endif
ifeq ($(CONFIG_HIGH_ACTIVE_DEV2HST), y)
EXTRA_CFLAGS += -DHIGH_ACTIVE_DEV2HST=1
else
EXTRA_CFLAGS += -DHIGH_ACTIVE_DEV2HST=0
endif
endif

ifeq ($(CONFIG_HIGH_ACTIVE_HST2DEV), y)
EXTRA_CFLAGS += -DHIGH_ACTIVE_HST2DEV=1
else
EXTRA_CFLAGS += -DHIGH_ACTIVE_HST2DEV=0
endif

ifneq ($(CONFIG_WAKEUP_GPIO_IDX), default)
EXTRA_CFLAGS += -DWAKEUP_GPIO_IDX=$(CONFIG_WAKEUP_GPIO_IDX)
endif

ifeq ($(CONFIG_RTW_SDIO_PM_KEEP_POWER), y)
ifeq ($(CONFIG_SDIO_HCI), y)
EXTRA_CFLAGS += -DCONFIG_RTW_SDIO_PM_KEEP_POWER
endif
endif

ifeq ($(CONFIG_REDUCE_TX_CPU_LOADING), y)
EXTRA_CFLAGS += -DCONFIG_REDUCE_TX_CPU_LOADING
endif

ifeq ($(CONFIG_BR_EXT), y)
BR_NAME = br0
EXTRA_CFLAGS += -DCONFIG_BR_EXT
EXTRA_CFLAGS += '-DCONFIG_BR_EXT_BRNAME="'$(BR_NAME)'"'
endif


ifeq ($(CONFIG_TDLS), y)
EXTRA_CFLAGS += -DCONFIG_TDLS
endif

ifeq ($(CONFIG_WIFI_MONITOR), y)
EXTRA_CFLAGS += -DCONFIG_WIFI_MONITOR
endif

ifeq ($(CONFIG_MCC_MODE), y)
EXTRA_CFLAGS += -DCONFIG_MCC_MODE
endif

ifeq ($(CONFIG_RTW_NAPI), y)
EXTRA_CFLAGS += -DCONFIG_RTW_NAPI
endif

ifeq ($(CONFIG_RTW_GRO), y)
EXTRA_CFLAGS += -DCONFIG_RTW_GRO
endif

ifeq ($(CONFIG_RTW_REPEATER_SON), y)
EXTRA_CFLAGS += -DCONFIG_RTW_REPEATER_SON
endif

ifeq ($(CONFIG_RTW_IPCAM_APPLICATION), y)
EXTRA_CFLAGS += -DCONFIG_RTW_IPCAM_APPLICATION
ifeq ($(CONFIG_WIFI_MONITOR), n)
EXTRA_CFLAGS += -DCONFIG_WIFI_MONITOR
endif
endif

ifeq ($(CONFIG_RTW_NETIF_SG), y)
EXTRA_CFLAGS += -DCONFIG_RTW_NETIF_SG
endif

ifeq ($(CONFIG_ICMP_VOQ), y)
EXTRA_CFLAGS += -DCONFIG_ICMP_VOQ
endif

ifeq ($(CONFIG_IP_R_MONITOR), y)
EXTRA_CFLAGS += -DCONFIG_IP_R_MONITOR
endif

ifeq ($(CONFIG_MP_VHT_HW_TX_MODE), y)
EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
ifeq ($(CONFIG_PLATFORM_I386_PC), y)
## For I386 X86 ToolChain use Hardware FLOATING
EXTRA_CFLAGS += -mhard-float
else
## For ARM ToolChain use Hardware FLOATING
EXTRA_CFLAGS += -mfloat-abi=hard
endif
endif

ifeq ($(CONFIG_APPEND_VENDOR_IE_ENABLE), y)
EXTRA_CFLAGS += -DCONFIG_APPEND_VENDOR_IE_ENABLE
endif

ifeq ($(CONFIG_RTW_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_RTW_DEBUG
EXTRA_CFLAGS += -DRTW_LOG_LEVEL=$(CONFIG_RTW_LOG_LEVEL)
endif

ifeq ($(CONFIG_PROC_DEBUG), y)
EXTRA_CFLAGS += -DCONFIG_PROC_DEBUG
endif

ifeq ($(CONFIG_RTW_UP_MAPPING_RULE), dscp)
EXTRA_CFLAGS += -DCONFIG_RTW_UP_MAPPING_RULE=1
else
EXTRA_CFLAGS += -DCONFIG_RTW_UP_MAPPING_RULE=0
endif

EXTRA_CFLAGS += -DDM_ODM_SUPPORT_TYPE=0x04

ifeq ($(CONFIG_RTW_MBO), y)
EXTRA_CFLAGS += -DCONFIG_RTW_MBO -DCONFIG_RTW_80211K -DCONFIG_RTW_WNM -DCONFIG_RTW_BTM_ROAM
EXTRA_CFLAGS += -DCONFIG_RTW_80211R
endif

ifeq ($(CONFIG_WAKE_ON_BT), y)
EXTRA_CFLAGS += -DCONFIG_WAKE_ON_BT
endif

ifeq ($(CONFIG_RTW_NBI), y)
EXTRA_CFLAGS += -DCONFIG_RTW_NBI
endif

ifeq ($(CONFIG_SECURITY_MEM), y)
EXTRA_CFLAGS += -DCONFIG_SECURITY_MEM
EXTRA_CFLAGS += -DSECURITY_MEM_ADDR=$(CONFIG_SECURITY_MEM_ADDR)
EXTRA_CFLAGS += -DSECURITY_MEM_SIZE=$(CONFIG_SECURITY_MEM_SIZE)
endif

ifeq ($(CONFIG_PLATFORM_AML_S905), y)
EXTRA_CFLAGS += -DCONFIG_PLATFORM_AML_S905
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN -fno-pic
# default setting for Android
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211
EXTRA_CFLAGS += -DRTW_USE_CFG80211_STA_EVENT
# default setting for Android 5.x and later
EXTRA_CFLAGS += -DCONFIG_RADIO_WORK

ifeq ($(CONFIG_SDIO_HCI), y)
EXTRA_CFLAGS += -DCONFIG_PLATFORM_OPS
_PLATFORM_FILES += platform/platform_aml_s905_sdio.o
endif

ARCH ?= arm64
CROSS_COMPILE ?= /4.4_S905L_8822bs_compile/gcc-linaro-aarch64-linux-gnu-4.9-2014.09_linux/bin/aarch64-linux-gnu-
ifndef KSRC
KSRC := /4.4_S905L_8822bs_compile/common
# To locate output files in a separate directory.
KSRC += O=/4.4_S905L_8822bs_compile/KERNEL_OBJ
endif

ifeq ($(CONFIG_RTL8822B), y)
ifeq ($(CONFIG_SDIO_HCI), y)
CONFIG_RTL8822BS ?= m
USER_MODULE_NAME := 8822bs
endif
endif

endif

ifeq ($(CONFIG_PLATFORM_ZTE_ZX296716), y)
EXTRA_CFLAGS += -Wno-error=date-time
EXTRA_CFLAGS += -DCONFIG_PLATFORM_ZTE_ZX296716
EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
# default setting for Android
EXTRA_CFLAGS += -DCONFIG_CONCURRENT_MODE
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211
EXTRA_CFLAGS += -DRTW_USE_CFG80211_STA_EVENT
# default setting for Android 5.x and later
#EXTRA_CFLAGS += -DCONFIG_RADIO_WORK

ifeq ($(CONFIG_SDIO_HCI), y)
# mark this temporarily
#EXTRA_CFLAGS += -DCONFIG_PLATFORM_OPS
#_PLATFORM_FILES += platform/platform_zte_zx296716_sdio.o
endif

ARCH ?= arm64
CROSS_COMPILE ?=
KSRC ?=

ifeq ($(CONFIG_RTL8822B), y)
ifeq ($(CONFIG_SDIO_HCI), y)
CONFIG_RTL8822BS ?= m
USER_MODULE_NAME := 8822bs
endif
endif

endif

########### CUSTOMER ################################
ifeq ($(CONFIG_CUSTOMER_HUAWEI_GENERAL), y)
CONFIG_CUSTOMER_HUAWEI = y
endif

ifeq ($(CONFIG_CUSTOMER_HUAWEI), y)
EXTRA_CFLAGS += -DCONFIG_HUAWEI_PROC
endif

CONFIG_PLATFORM_CMAP_INTFS = n
ifeq ($(CONFIG_PLATFORM_CMAP_INTFS), y)
PLATFORM_CMAP_INTFS_TYPE = 00
EXTRA_CFLAGS += -DCONFIG_PLATFORM_CMAP_INTFS -DCMAP_UNASSOC_METRICS_STA_MAX=32
_OS_INTFS_FILES += os_dep/linux/custom_multiap_intfs/custom_multiap_intfs.o
_PLATFORM_FILES += platform/custom_multiap_intfs_$(PLATFORM_CMAP_INTFS_TYPE).o
endif

ifeq ($(CONFIG_MULTIDRV), y)

ifeq ($(CONFIG_SDIO_HCI), y)
MODULE_NAME := rtw_sdio
endif

ifeq ($(CONFIG_USB_HCI), y)
MODULE_NAME := rtw_usb
endif

ifeq ($(CONFIG_PCI_HCI), y)
MODULE_NAME := rtw_pci
endif


endif

USER_MODULE_NAME ?=
ifneq ($(USER_MODULE_NAME),)
MODULE_NAME := $(USER_MODULE_NAME)
endif

ifneq ($(KERNELRELEASE),)

########### this part for *.mk ############################
include $(src)/hal/phydm/phydm.mk

########### HAL_RTL8822C #################################
ifeq ($(CONFIG_RTL8822C), y)
include $(src)/rtl8822c.mk
endif

rtk_core :=	core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_query.o \
		core/rtw_ioctl_set.o \
		core/rtw_ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_mi.o \
		core/rtw_wlan_util.o \
		core/rtw_vht.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_chplan.o \
		core/monitor/rtw_radiotap.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_ap.o \
		core/wds/rtw_wds.o \
		core/mesh/rtw_mesh.o \
		core/mesh/rtw_mesh_pathtbl.o \
		core/mesh/rtw_mesh_hwmp.o \
		core/rtw_xmit.o	\
		core/rtw_p2p.o \
		core/rtw_rson.o \
		core/rtw_tdls.o \
		core/rtw_br_ext.o \
		core/rtw_iol.o \
		core/rtw_sreset.o \
		core/rtw_btcoex_wifionly.o \
		core/rtw_btcoex.o \
		core/rtw_beamforming.o \
		core/rtw_odm.o \
		core/rtw_rm.o \
		core/rtw_rm_fsm.o \
		core/rtw_ft.o \
		core/rtw_wnm.o \
		core/rtw_mbo.o \
		core/rtw_rm_util.o \
		core/efuse/rtw_efuse.o \
		core/rtw_roch.o

ifeq ($(CONFIG_SDIO_HCI), y)
rtk_core += core/rtw_sdio.o
endif

EXTRA_CFLAGS += -I$(src)/core/crypto
rtk_core += \
		core/crypto/aes-internal.o \
		core/crypto/aes-internal-enc.o \
		core/crypto/aes-gcm.o \
		core/crypto/aes-ccm.o \
		core/crypto/aes-omac1.o \
		core/crypto/ccmp.o \
		core/crypto/gcmp.o \
		core/crypto/aes-siv.o \
		core/crypto/aes-ctr.o \
		core/crypto/sha256-internal.o \
		core/crypto/sha256.o \
		core/crypto/sha256-prf.o \
		core/crypto/rtw_crypto_wrap.o \
		core/rtw_swcrypto.o

$(MODULE_NAME)-y += $(rtk_core)

$(MODULE_NAME)-$(CONFIG_WAPI_SUPPORT) += core/rtw_wapi.o	\
					core/rtw_wapi_sms4.o

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)
$(MODULE_NAME)-y += $(_PHYDM_FILES)
$(MODULE_NAME)-y += $(_BTC_FILES)
$(MODULE_NAME)-y += $(_PLATFORM_FILES)

$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += core/rtw_mp.o

ifeq ($(CONFIG_RTL8723B), y)
$(MODULE_NAME)-$(CONFIG_MP_INCLUDED)+= core/rtw_bt_mp.o
endif

obj-$(CONFIG_RTL8822CS) += $(MODULE_NAME).o

else

export CONFIG_RTL8822CS = m

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a ${KVER}

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}

backup_rtlwifi:
	@echo "Making backup rtlwifi drivers"
ifneq (,$(wildcard $(STAGINGMODDIR)/rtl*))
	@tar cPf $(wildcard $(STAGINGMODDIR))/backup_rtlwifi_driver.tar $(wildcard $(STAGINGMODDIR)/rtl*)
	@rm -rf $(wildcard $(STAGINGMODDIR)/rtl*)
endif
ifneq (,$(wildcard $(MODDESTDIR)realtek))
	@tar cPf $(MODDESTDIR)backup_rtlwifi_driver.tar $(MODDESTDIR)realtek
	@rm -fr $(MODDESTDIR)realtek
endif
ifneq (,$(wildcard $(MODDESTDIR)rtl*))
	@tar cPf $(MODDESTDIR)../backup_rtlwifi_driver.tar $(wildcard $(MODDESTDIR)rtl*)
	@rm -fr $(wildcard $(MODDESTDIR)rtl*)
endif
	@/sbin/depmod -a ${KVER}
	@echo "Please reboot your system"

restore_rtlwifi:
	@echo "Restoring backups"
ifneq (,$(wildcard $(STAGINGMODDIR)/backup_rtlwifi_driver.tar))
	@tar xPf $(STAGINGMODDIR)/backup_rtlwifi_driver.tar
	@rm $(STAGINGMODDIR)/backup_rtlwifi_driver.tar
endif
ifneq (,$(wildcard $(MODDESTDIR)backup_rtlwifi_driver.tar))
	@tar xPf $(MODDESTDIR)backup_rtlwifi_driver.tar
	@rm $(MODDESTDIR)backup_rtlwifi_driver.tar
endif
ifneq (,$(wildcard $(MODDESTDIR)../backup_rtlwifi_driver.tar))
	@tar xPf $(MODDESTDIR)../backup_rtlwifi_driver.tar
	@rm $(MODDESTDIR)../backup_rtlwifi_driver.tar
endif
	@/sbin/depmod -a ${KVER}
	@echo "Please reboot your system"

config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in


.PHONY: modules clean

clean:
	#$(MAKE) -C $(KSRC) M=$(shell pwd) clean
	cd hal ; rm -fr */*/*/*.mod.c */*/*/*.mod */*/*/*.o */*/*/.*.cmd */*/*/*.ko
	cd hal ; rm -fr */*/*.mod.c */*/*.mod */*/*.o */*/.*.cmd */*/*.ko
	cd hal ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd hal ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd core ; rm -fr */*.mod.c */*.mod */*.o */.*.cmd */*.ko
	cd core ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd os_dep/linux ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd os_dep ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	cd platform ; rm -fr *.mod.c *.mod *.o .*.cmd *.ko
	rm -fr Module.symvers ; rm -fr Module.markers ; rm -fr modules.order
	rm -fr *.mod.c *.mod *.o .*.cmd *.ko *~
	rm -fr .tmp_versions
endif

