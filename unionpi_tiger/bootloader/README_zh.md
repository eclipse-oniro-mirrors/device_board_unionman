# bootloader

## 简介

编译打包配置文件, 其中uboot镜像可从uboot仓库[下载](https://gitee.com/umspark/uboot)。

```
.
└── images
 ├── aml_sdc_burn.ini
 ├── LICENSE
 ├── openharmony.conf    #打包配置文件
 └── platform.conf       #芯片平台配置
```

> 说明：uboot镜像，默认提供预编译的二进制文件，请从uboot仓库的[链接](https://gitee.com/umspark/uboot)下载相应版本

## 镜像打包

在OpenHarmony根目录下执行：

```shell
./device/board/unionman/unionpi_tiger/common/tools/packer-unionpi.sh 
```

> 说明：如果需要采用Amlogic芯片平台的USB Burn烧录工具烧录镜像，需要在根目录执行以下命令后，再执行以上打包命令。
> 
> ```
> ./build.sh --product-name unionpi_tiger --ccache
> ```
