# Copyright (c) 2022 Unionman Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

OHOS_HOME := $(shell pwd)/../../../../..

KERNEL_SRC := $(OHOS_HOME)/out/kernel/src_tmp/linux-5.10
CROSS_COMPILE := $(OHOS_HOME)/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-7.5.0-2019.12-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-
CLANG_CC := $(OHOS_HOME)/prebuilts/clang/ohos/linux-x86_64/llvm/bin/clang
M := $(shell pwd)

CONFIGS := CONFIG_AMLOGIC_MEDIA_VDEC_MPEG12=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_MPEG2_MULTI=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_MPEG4=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_MPEG4_MULTI=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_VC1=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_H264=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_H264_MULTI=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_H264_MVC=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_H265=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_VP9=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_MJPEG=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_MJPEG_MULTI=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_REAL=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_AVS=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_AVS_MULTI=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_AVS2=m \
	CONFIG_AMLOGIC_MEDIA_VDEC_AV1=m \
	CONFIG_AMLOGIC_MEDIA_VENC_H264=m \
	CONFIG_AMLOGIC_MEDIA_VENC_JPEG=m \
	CONFIG_AMLOGIC_CPU_VERSION=y 


EXTRA_INCLUDE := -I$(M)/drivers/include
EXTRA_INCLUDE += -DCONFIG_AMLOGIC_MEDIA_GE2D \
		-DCONFIG_AMLOGIC_MEDIA_CANVAS \
		-DCONFIG_AMLOGIC_CPU_VERSION \
		-DCONFIG_AMLOGIC_MEDIA_VENC_H264

CONFIGS_BUILD := -Wno-parentheses-equality -Wno-pointer-bool-conversion \
				-Wno-unused-const-variable -Wno-typedef-redefinition \
				-Wno-logical-not-parentheses -Wno-sometimes-uninitialized

EXTRA_CFLAGS := $(EXTRA_INCLUDE) $(CONFIGS_BUILD) -Wall

.PHNOY: module all clean

all: module
	rm out -rf; mkdir -p out && find drivers -name "*.ko" -print | xargs -I {} cp {} out/

module:
	$(MAKE) -C  $(KERNEL_SRC) M=$(M)/drivers modules KCFLAGS="$(EXTRA_CFLAGS)" $(CONFIGS) CC=$(CLANG_CC) CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm64

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(M)/drivers clean
