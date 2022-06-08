# images

预编译好的镜像

.
└── images
    ├── aml_sdc_burn.ini
    ├── LICENSE
    ├── logo.img            #开机Logo
    ├── openharmony.conf    #打包配置文件
    ├── platform.conf       #芯片平台配置
    ├── u-boot.bin          #预编译好的uboot镜像
    └── u-boot.bin.sd.bin   #预编译好的uboot镜像, 用于USB或SD卡升级

## 打包命令

在OpenHarmony项目目录下执行：

```shell
./device/board/unionman/a311d/tools/packer-unionpi.sh 
```

