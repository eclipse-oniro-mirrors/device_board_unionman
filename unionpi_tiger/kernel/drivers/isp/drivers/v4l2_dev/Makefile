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

KDIR = $(src)/../../../../../../../../../out/kernel/src_tmp/linux-5.10

obj-m += iv009_isp.o
iv009_isp-objs += src/calibration/acamera_get_calibrations_v4l2.o \
			src/calibration/soc_iq_calibrations.o 
iv009_isp-objs += src/driver/lens/lens_init.o \
			src/driver/lens/v4l2_vcm.o \
			src/driver/sensor/ACameraDefault.o \
			src/driver/sensor/sensor_bus_config.o \
			src/driver/sensor/V4L2_drv.o
iv009_isp-objs += src/fw_lib/sbuf_func.o \
			src/fw_lib/autocapture_func.o \
			src/fw_lib/matrix_yuv_func.o \
			src/fw_lib/acamera_command_api.o \
			src/fw_lib/acamera_fsm_mgr.o \
			src/fw_lib/sensor_intf.o \
			src/fw_lib/monitor_intf.o \
			src/fw_lib/dma_writer.o \
			src/fw_lib/crop_fsm.o \
			src/fw_lib/sharpening_intf.o \
			src/fw_lib/crop_func.o \
			src/fw_lib/sensor_func.o \
			src/fw_lib/gamma_manual_fsm.o \
			src/fw_lib/acamera_sbus_isp_sw.o \
			src/fw_lib/ae_manual_intf.o \
			src/fw_lib/acamera_sbus_isp.o \
			src/fw_lib/af_manual_fsm.o \
			src/fw_lib/matrix_yuv_intf.o \
			src/fw_lib/metadata_func.o \
			src/fw_lib/dma_writer_intf.o \
			src/fw_lib/awb_manual_func.o \
			src/fw_lib/acamera_math.o \
			src/fw_lib/sbuf_intf.o \
			src/fw_lib/iridix8_manual_func.o \
			src/fw_lib/noise_reduction_intf.o \
			src/fw_lib/monitor_func.o \
			src/fw_lib/cmos_fsm.o \
			src/fw_lib/ae_manual_fsm.o \
			src/fw_lib/acamera.o \
			src/fw_lib/color_matrix_fsm.o \
			src/fw_lib/metadata_fsm.o \
			src/fw_lib/autocapture_fsm.o \
			src/fw_lib/sharpening_func.o \
			src/fw_lib/acamera_event_queue.o \
			src/fw_lib/cmos_func.o \
			src/fw_lib/acamera_calibrations.o \
			src/fw_lib/general_fsm.o \
			src/fw_lib/sbuf_fsm.o \
			src/fw_lib/af_manual_func.o \
			src/fw_lib/acamera_ctrl_channel_k2u.o \
			src/fw_lib/awb_manual_fsm.o \
			src/fw_lib/dma_writer_func.o \
			src/fw_lib/fsm_param.o \
			src/fw_lib/ae_manual_func.o \
			src/fw_lib/noise_reduction_func.o \
			src/fw_lib/sensor_init.o \
			src/fw_lib/iridix8_manual_intf.o \
			src/fw_lib/awb_manual_intf.o \
			src/fw_lib/af_manual_intf.o \
			src/fw_lib/metadata_intf.o \
			src/fw_lib/sharpening_fsm.o \
			src/fw_lib/fsm_util.o \
			src/fw_lib/acamera_loop_buf.o \
			src/fw_lib/crop_intf.o \
			src/fw_lib/gamma_manual_intf.o \
			src/fw_lib/matrix_yuv_fsm.o \
			src/fw_lib/color_matrix_func.o \
			src/fw_lib/gamma_manual_func.o \
			src/fw_lib/monitor_fsm.o \
			src/fw_lib/general_intf.o \
			src/fw_lib/acamera_sbus.o \
			src/fw_lib/iridix8_manual_fsm.o \
			src/fw_lib/general_func.o \
			src/fw_lib/cmos_intf.o \
			src/fw_lib/sensor_fsm.o \
			src/fw_lib/acamera_logger.o \
			src/fw_lib/acamera_sbus_spi.o \
			src/fw_lib/acamera_command_api_impl.o \
			src/fw_lib/noise_reduction_fsm.o \
			src/fw_lib/acamera_sbus_i2c.o \
			src/fw_lib/dma_writer_fsm.o \
			src/fw_lib/color_matrix_intf.o \
			src/fw_lib/autocapture_intf.o \
			src/fw_lib/acamera_fw.o
iv009_isp-objs += src/platform/system_profiler.o \
			src/platform/system_am_md.o \
			src/platform/system_timer.o \
			src/platform/system_sw_io.o \
			src/platform/system_stdlib.o \
			src/platform/system_spi.o \
			src/platform/system_dma.o \
			src/platform/system_log.o \
			src/platform/system_spinlock.o \
			src/platform/system_i2c.o \
			src/platform/system_interrupts.o \
			src/platform/system_am_sc.o \
			src/platform/system_hw_io.o \
			src/platform/system_semaphore.o \
			src/platform/system_control.o \
			src/platform/system_autowrite.o \
			src/platform/system_chardev.o \
			src/platform/system_hdmi.o
iv009_isp-objs += app/main_firmware.o \
			app/control/acamera_chardev.o \
			app/control/acamera_connection.o \
			app/control/application_command_api.o \
			app/main_kernel_juno_v4l2.o \
			app/v4l2_interface/isp-v4l2.o \
			app/v4l2_interface/isp-v4l2-stream.o \
			app/v4l2_interface/isp-v4l2-ctrl.o \
			app/v4l2_interface/isp-vb2-cmalloc.o \
			app/v4l2_interface/isp-vb2.o \
			app/v4l2_interface/fw-interface.o

#INCLUDE_DIRS := $(addprefix -I,$(shell find ../ -type d ))

ccflags-y:=-I$(src)/app -I$(src)/inc -I$(src)/app/control -I$(src)/inc/api -I$(src)/inc/isp -I$(src)/inc/sys -I$(src)/src/platform -I$(src)/src/fw  -I$(src)/src/fw_lib -I$(src)/src/calibration -I$(src)/src/driver/sensor -I$(src)/src/driver/lens -I$(KDIR)/include/linux/

ccflags-y += -Idrivers/staging/android/ -Idrivers/amlogic/media/common/ion_dev/
ccflags-y += -Wno-declaration-after-statement
ccflags-y += -DCONFIG_LINUX_OS
ccflags-y += -DCONFIG_G12B
ccflags-y += -DCONFIG_SEAMLESS_N
