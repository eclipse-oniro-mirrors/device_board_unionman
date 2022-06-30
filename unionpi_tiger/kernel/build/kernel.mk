# Copyright (c) 2022 Unionman Technology Co., Ltd.
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# ohos makefile to build kernel

KERNEL_VERSION = linux-5.10
KERNEL_ARCH = arm64
BUILD_TYPE = standard
OHOS_ROOT_PATH = $(realpath $(shell pwd)/../..)

KERNEL_SRC_TMP_PATH := $(KERNEL_OBJ_PATH)/kernel/src_tmp/linux-5.10
KERNEL_OBJ_TMP_PATH := $(KERNEL_OBJ_PATH)/kernel/src_tmp/linux-5.10

KERNEL_SRC_PATH := $(OHOS_ROOT_PATH)/kernel/linux/${KERNEL_VERSION}

PREBUILTS_GCC_DIR := $(OHOS_ROOT_PATH)/prebuilts/gcc
CLANG_HOST_TOOLCHAIN := $(OHOS_ROOT_PATH)/prebuilts/clang/ohos/linux-x86_64/llvm/bin
KERNEL_HOSTCC := $(CLANG_HOST_TOOLCHAIN)/clang
KERNEL_PREBUILT_MAKE := make

ifeq ($(BUILD_TYPE), standard)
ifeq ($(KERNEL_ARCH), arm)
    KERNEL_ARCH := arm
    KERNEL_TARGET_TOOLCHAIN := $(PREBUILTS_GCC_DIR)/linux-x86/arm/gcc-linaro-7.5.0-arm-linux-gnueabi/bin
    KERNEL_TARGET_TOOLCHAIN_PREFIX := $(KERNEL_TARGET_TOOLCHAIN)/arm-linux-gnueabi-
endif
ifeq ($(KERNEL_ARCH), arm64)
    KERNEL_ARCH := arm64
    KERNEL_TARGET_TOOLCHAIN := $(PREBUILTS_GCC_DIR)/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin
    KERNEL_TARGET_TOOLCHAIN_PREFIX := $(KERNEL_TARGET_TOOLCHAIN)/aarch64-linux-gnu-
endif
    CLANG_CC := $(CLANG_HOST_TOOLCHAIN)/clang
else ifeq ($(BUILD_TYPE), small)
    KERNEL_ARCH := arm64
    ifeq ($(CLANG_CC), "")
        CLANG_CC := $(CLANG_HOST_TOOLCHAIN)/clang
    endif
endif

KERNEL_PERL := /usr/bin/perl

KERNEL_CROSS_COMPILE :=
KERNEL_CROSS_COMPILE += CC="$(CLANG_CC)"
ifeq ($(BUILD_TYPE), standard)
    KERNEL_CROSS_COMPILE += HOSTCC="$(KERNEL_HOSTCC)"
    KERNEL_CROSS_COMPILE += PERL=$(KERNEL_PERL)
    KERNEL_CROSS_COMPILE += CROSS_COMPILE="$(KERNEL_TARGET_TOOLCHAIN_PREFIX)"
else ifeq ($(BUILD_TYPE), small)
    KERNEL_CROSS_COMPILE += CROSS_COMPILE="arm-linux-gnueabi-"
endif

KERNEL_MAKE := \
    PATH="$$PATH" \
    $(KERNEL_PREBUILT_MAKE)

KERNEL_PATCH_FILE := $(DEVICE_PATH)/../../../../kernel/linux/patches/linux-5.10/unionpi_tiger_pacth/linux-5.10.patch
HDF_PATCH_FILE := $(DEVICE_PATH)/../../../../kernel/linux/patches/linux-5.10/unionpi_tiger_pacth/hdf.patch
KERNEL_CONFIG_FILE := $(DEVICE_PATH)/../../../../kernel/linux/config/linux-5.10/arch/arm64/configs/unionpi_tiger_standard_defconfig
KERNEL_LOGO_FILE := $(DEVICE_PATH)/resource/logo/logo.ppm

ifeq ($(KERNEL_ARCH), arm)
KERNEL_IMAGE_FILE := $(KERNEL_SRC_TMP_PATH)/arch/arm/boot/uImage
else ifeq ($(KERNEL_ARCH), arm64)
KERNEL_IMAGE_FILE := $(KERNEL_SRC_TMP_PATH)/arch/arm64/boot/uImage
endif
DEFCONFIG_FILE := defconfig

export KBUILD_OUTPUT=$(KERNEL_OBJ_TMP_PATH)

$(KERNEL_IMAGE_FILE):
	@rm -rf $(KERNEL_SRC_TMP_PATH);mkdir -p $(KERNEL_SRC_TMP_PATH);cp -arfL $(KERNEL_SRC_PATH)/* $(KERNEL_SRC_TMP_PATH)/
	@cd $(KERNEL_SRC_TMP_PATH) && patch -p1 < $(KERNEL_PATCH_FILE)
	@$(DEVICE_PATH)/kernel/build/patch_hdf.sh $(OHOS_ROOT_PATH) $(KERNEL_SRC_TMP_PATH) $(HDF_PATCH_FILE)
	@cp -rf $(KERNEL_LOGO_FILE) $(KERNEL_SRC_TMP_PATH)/drivers/video/logo/logo_linux_clut224.ppm
	@cp -rf $(KERNEL_CONFIG_FILE) $(KERNEL_SRC_TMP_PATH)/arch/arm64/configs/defconfig
	@$(KERNEL_MAKE) -C $(KERNEL_SRC_TMP_PATH) ARCH=$(KERNEL_ARCH) TEXT_OFFSET=0x01080000 $(KERNEL_CROSS_COMPILE) $(DEFCONFIG_FILE)
	@$(KERNEL_MAKE) -C $(KERNEL_SRC_TMP_PATH) ARCH=$(KERNEL_ARCH) TEXT_OFFSET=0x01080000 $(KERNEL_CROSS_COMPILE) modules_prepare
	$(KERNEL_MAKE) -C $(KERNEL_SRC_TMP_PATH) ARCH=$(KERNEL_ARCH) TEXT_OFFSET=0x01080000 $(KERNEL_CROSS_COMPILE) -j128 modules Image Image.gz dtbs
	@$(DEVICE_PATH)/common/tools/linux/dtbTool -o $(IMAGES_PATH)/dtb.img $(KERNEL_OBJ_TMP_PATH)/arch/arm64/boot/dts/amlogic/ > /dev/null
	@gzip $(IMAGES_PATH)/dtb.img
	@mv $(IMAGES_PATH)/dtb.img.gz $(IMAGES_PATH)/dtb.img
	@chmod 777 $(KERNEL_OBJ_TMP_PATH)/make-boot.sh
	@mkdir -p $(IMAGES_PATH)../modules
ifeq ($(RAMDISK_ENABLE), false)
	@$(DEVICE_PATH)/tools/linux/mkbootimg --kernel $(KERNEL_OBJ_TMP_PATH)/arch/arm64/boot/Image.gz --base 0x0 --kernel_offset 0x1080000 --cmdline "" --ramdisk  $(DEVICE_PATH)/tools/linux/rootfs.cpio.gz --second $(IMAGES_PATH)/dtb.img --output $(IMAGES_PATH)/boot.img > /dev/null
endif

.PHONY: build-kernel
build-kernel: $(KERNEL_IMAGE_FILE)
