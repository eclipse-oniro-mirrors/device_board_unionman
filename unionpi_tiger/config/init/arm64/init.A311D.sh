#!/system/bin/sh

/system/bin/insmod /vendor/modules/gpu-sched.ko
/system/bin/insmod /vendor/modules/panfrost.ko

/system/bin/insmod /vendor/modules/iv009_isp_iq.ko
/system/bin/insmod /vendor/modules/iv009_isp_lens.ko
/system/bin/insmod /vendor/modules/iv009_isp_sensor.ko
/system/bin/insmod /vendor/modules/iv009_isp.ko

/system/bin/insmod /vendor/modules/amvout_legacy.ko
/system/bin/insmod /vendor/modules/registers.ko
/system/bin/insmod /vendor/modules/media_clock.ko
/system/bin/insmod /vendor/modules/firmware.ko
/system/bin/insmod /vendor/modules/amvfm.ko
/system/bin/insmod /vendor/modules/amvpu.ko
/system/bin/insmod /vendor/modules/dil.ko
/system/bin/insmod /vendor/modules/amrdma.ko
/system/bin/insmod /vendor/modules/amvideo.ko
/system/bin/insmod /vendor/modules/amionvideo.ko
/system/bin/insmod /vendor/modules/decoder_common.ko
/system/bin/insmod /vendor/modules/stream_input.ko
/system/bin/insmod /vendor/modules/di.ko
/system/bin/insmod /vendor/modules/ppmgr.ko
/system/bin/insmod /vendor/modules/amvdec_mmpeg12.ko
/system/bin/insmod /vendor/modules/amvdec_mpeg12.ko
/system/bin/insmod /vendor/modules/amvdec_mh264.ko
/system/bin/insmod /vendor/modules/amvdec_h264.ko
/system/bin/insmod /vendor/modules/amvdec_h264mvc.ko
/system/bin/insmod /vendor/modules/amvdec_av1.ko
/system/bin/insmod /vendor/modules/amvdec_avs.ko
/system/bin/insmod /vendor/modules/amvdec_avs2.ko
/system/bin/insmod /vendor/modules/amvdec_h265.ko
/system/bin/insmod /vendor/modules/amvdec_mavs.ko
/system/bin/insmod /vendor/modules/amvdec_mjpeg.ko
/system/bin/insmod /vendor/modules/amvdec_mmjpeg.ko
/system/bin/insmod /vendor/modules/amvdec_mmpeg4.ko
/system/bin/insmod /vendor/modules/amvdec_mpeg4.ko
/system/bin/insmod /vendor/modules/amvdec_real.ko
/system/bin/insmod /vendor/modules/amvdec_vc1.ko
/system/bin/insmod /vendor/modules/amvdec_vp9.ko
/system/bin/insmod /vendor/modules/encoder.ko
/system/bin/insmod /vendor/modules/jpegenc.ko
echo power_on 0 > /sys/class/vdec/debug
sleep 0.5
echo power_off 0 > /sys/class/vdec/debug

/system/bin/begetctl service_control stop usb_service
sleep 0.1
/system/bin/begetctl service_control start usb_service

/system/bin/chmod 666 /dev/HDF_PLATFORM_*
LD_LIBRARY_PATH=/vendor/lib64/glibc/ /vendor/bin/iv009_isp &
