#!/system/bin/sh

cmd=$ACTION
format=$ID_FS_TYPE
mount_point=${DEVNAME##*/}

if [ "x$cmd" = "xadd" ];then
    if [  -n $mount_point ];then
        mount /dev/block/$mount_point /sdcard
    fi
else if [ "x$cmd" = "xremove" ];then
    if [ -n $mount_point ];then
        umount -l /sdcard
        ret=$?
        if [ $ret -eq 0 ];then
            rm -r /sdcard
        fi
    fi
fi
fi
