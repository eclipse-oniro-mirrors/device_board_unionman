#!/system/bin/sh

/system/bin/insmod /vendor/modules/gpu-sched.ko
/system/bin/insmod /vendor/modules/panfrost.ko
/system/bin/insmod /vendor/modules/iv009_isp_iq.ko
/system/bin/insmod /vendor/modules/iv009_isp_lens.ko
/system/bin/insmod /vendor/modules/iv009_isp_sensor.ko
/system/bin/insmod /vendor/modules/iv009_isp.ko
/system/bin/insmod /vendor/modules/v4l2-mem2mem.ko
/system/bin/insmod /vendor/modules/registers.ko
/system/bin/insmod /vendor/modules/media_clock.ko
/system/bin/insmod /vendor/modules/firmware.ko
/system/bin/insmod /vendor/modules/decoder_common.ko
/system/bin/insmod /vendor/modules/encoder.ko
/system/bin/insmod /vendor/modules/jpegenc.ko
LD_LIBRARY_PATH=/vendor/lib/glibc/ /vendor/bin/iv009_isp &
