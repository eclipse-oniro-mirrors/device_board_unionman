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

KDIR = $(src)/../../../../../../../../../../out/kernel/src_tmp/linux-5.10

obj-m += iv009_isp_lens.o

iv009_isp_lens-objs += app/soc_lens.o
iv009_isp_lens-objs += src/fw_lib/acamera_sbus_isp_sw.o \
				src/fw_lib/acamera_sbus_isp.o \
				src/fw_lib/acamera_math.o \
				src/fw_lib/sensor_init.o \
				src/fw_lib/acamera_sbus.o \
				src/fw_lib/acamera_logger.o \
				src/fw_lib/acamera_sbus_spi.o \
				src/fw_lib/acamera_sbus_i2c.o
iv009_isp_lens-objs += src/platform/system_timer.o \
				src/platform/system_sw_io.o \
				src/platform/system_stdlib.o \
				src/platform/system_spi.o \
				src/platform/system_log.o \
				src/platform/system_spinlock.o \
				src/platform/system_sensor.o \
				src/platform/system_i2c.o \
				src/platform/system_hw_io.o
iv009_isp_lens-objs += src/driver/sensor/Dummy_drv.o \
				src/driver/sensor/sensor_bus_config.o \
				src/driver/sensor/ACameraDefault.o \
				src/driver/lens/null_vcm.o \
				src/driver/lens/lens_init.o

#INCLUDE_DIRS := $(addprefix -I,$(shell find ../ -type d ))

ccflags-y:=-I$(src)/app -I$(src)/inc -I$(src)/app/control -I$(src)/inc/api -I$(src)/inc/isp -I$(src)/inc/sys -I$(src)/src/platform -I$(src)/src/fw  -I$(src)/src/fw_lib -I$(src)/src/calibration -I$(src)/src/driver/sensor -I$(src)/src/driver/lens -I$(KDIR)/include/linux/

ccflags-y += -Wno-declaration-after-statement
