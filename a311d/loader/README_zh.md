# loader

预编译好的镜像及编译打包配置文件

```
.
└── images
 ├── aml_sdc_burn.ini
 ├── LICENSE
 ├── openharmony.conf #打包配置文件
 ├── platform.conf #芯片平台配置
 └── u-boot.bin #预编译好的uboot镜像, 同时可以用于USB升级
```

## 打包命令

在OpenHarmony根目录下执行：

```shell
./device/board/unionman/a311d/common/tools/packer-unionpi.sh 
```

> 注意：如果需要采用Amlogic芯片平台的USB Burn烧录工具烧录镜像，需要在执行以下命令后，再执行以上打包命令。
> 
> ```
> ./build.sh --product-name a311d --ccache
> ```
