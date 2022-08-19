# Copyright (c) 2022 Unionman Technology Co., Ltd.
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

#obj-$(CONFIG_AMLOGIC_MEDIA_VENC_JPEG) += amvenc_jpeg.o
#obj-m += amvenc_jpeg.o
#amvenc_jpeg-objs += jpegenc.o
#obj-m += jpegenc.o

EXTRA_INCLUDE := -I$(src)/../../../../drivers/include
EXTRA_INCLUDE += -DCONFIG_AMLOGIC_MEDIA_GE2D \
		-DCONFIG_AMLOGIC_MEDIA_CANVAS \
		-DCONFIG_AMLOGIC_CPU_VERSION \
		-DCONFIG_AMLOGIC_MEDIA_VENC_H264 

CONFIGS_BUILD := -Wno-parentheses-equality -Wno-pointer-bool-conversion \
				-Wno-unused-const-variable -Wno-typedef-redefinition \
				-Wno-logical-not-parentheses -Wno-sometimes-uninitialized

EXTRA_CFLAGS := $(EXTRA_INCLUDE) $(CONFIGS_BUILD) -Wall

obj-m += jpegenc.o
