#!/system/bin/sh

cmd=$ACTION
format=$ID_FS_TYPE
mount_point=${DEVNAME##*/}

if [ "x$cmd" = "xadd" ];then
    if [  -n $mount_point ];then
        mkdir -p /storage/$mount_point
        mount /dev/block/$mount_point /storage/$mount_point
    fi
else if [ "x$cmd" = "xremove" ];then
    if [ -n $mount_point ];then
        umount -l /storage/$mount_point
        ret=$?
        if [ $ret -eq 0 ];then
            rm -r /storage/$mount_point
        fi
    fi
fi
fi
