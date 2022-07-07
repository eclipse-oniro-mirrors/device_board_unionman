# device_board_unionman

## 介绍

本仓用于放置unionman开发板相关内容，各开发板的使用教程和使用详细说明可点击下表开发板名称查看。

| 开发板名称                                       | SoC型号           | 应用领域                                   | 设备互联类型        |
|:-------------------------------------------:|:---------------:|:--------------------------------------:|:-------------:|
| [Unionpi Tiger](unionpi_tiger/README_zh.md) | `Amlogic A311D` | 智能家居、AI人脸识别、工业控制、智慧车载、多媒体处理、AI边缘计算、物联网 | 有线以太网、WIFI、蓝牙 |

#### 开发板简介

- [Unionpi-Tiger](https://gitee.com/openharmony-sig/device_board_unionman/blob/master/unionpi_tiger/README_zh.md)

Unionpi Tiger是一款应用于图像处理，音视频处理和深度学习等场景的智能硬件，其主芯片采用Amlogic A311D芯片方案。
A311D支持GPU和神经网络加速子系统，支持4K视频编解码器引擎和一流的HDR图像处理，并集成了所有标准音频/视频输入/输出接口。主系统的CPU采用大小核设计，主频高达2.2GHz，集成了四个Cortex-A73核心和两Cortex-A53核心 ，集成独立的5.0T NPU处理器。

#### 开发板架构
开发板分为底板与核心板

![扩展板架构](./figures/architecture-of-dev-board.png "底板")
![核心板架构](./figures/architecture-of-core-board.png "核心板")

## 目录

```
device/board/unionman
├── unionpi_tiger           # Unionpi-Tiger开发板目录
├── figures                 # 开发板或产品相关图片目录
├── LICENSE
├── OTA.xml
└── README_zh.md
```

## 使用说明

Unionpi-Tiger参考:

- [Unionpi-Tiger](https://gitee.com/openharmony-sig/device_board_unionman/blob/master/unionpi_tiger/README_zh.md)

## 相关仓

- device_board_unionman

- [vendor_unionman](https://gitee.com/openharmony-sig/vendor_unionman)

- [device_soc_amlogic](https://gitee.com/openharmony-sig/device_soc_amlogic)
