# Xiaomi Mi 9 (cepheus) — Linux / postmarketOS

小米 9 的社区适配源码、修复补丁、实验内核与验证记录，建立在 Linux、SM8150 mainline、postmarketOS、libcamera 等上游工作的基础上。保留原作者和许可证，不把已有驱动归为本项目原创。

**源码与实验内核已公开，尚非可一键刷写的发行镜像。** 2026-11-23 从原 WSL 构建树、归档和手机恢复并核对材料。当前手机的 Linux 6.17.0-sm8150 #8 内核、配置和最终 CAMSS v3 均有来源对应记录；补丁还原和静态 DTS 对照通过。本次没有重新完成干净环境下的整套内核编译，也没有把历史测试当作新一轮实机测试。

## 下载和源码

- [实验内核发布版：Image.gz、完整上游源码归档、修复源码包与校验值](https://github.com/Charlie3xtra/xiaomi-cepheus-postmarketos/releases/tag/cepheus-6.17.0-8-recovery-20261123)
- [最终 #8 内核、CAMSS v3、FastRPC/SSC、静态 DTS、Wi-Fi 补丁与构建说明](kernel/recovered-final/20261123/README.md)
- [四摄 sensor 驱动及 mode 表](kernel/snapshots/20261106/drivers/)
- [可应用到干净 libcamera v0.7.2 的完整补丁](userspace/libcamera/patches/README.md)
- [系统相机启动环境和传感器服务](userspace/installed-config/20261123/)
- [长焦单次 AF 源码](tools/autofocus/README.md)

发布的 Image.gz 是纯 Linux 内核，**不是可直接刷 boot 分区的 boot.img**，不含附加 DTB、ramdisk 或专有固件。它与手机实际启动分区中的 gzip 内核逐字节一致。

## 功能状态

| 功能 | 已有结果 | 证据与边界 |
| --- | --- | --- |
| 屏幕、触控 | 项目使用中可用 | 公开最终静态 DTS；显示/panel 与触控路由均有节点记录 |
| Wi-Fi / 5GHz | 之前实机连接成功 | 公开最终 ath10k 差异与模块参数；依赖 MSS/固件服务链 |
| 蓝牙 | 用户报告可用 | 最终 DTS 节点已公开；未新增独立运行验收记录 |
| 主摄 IMX586 | RAW、预览、JPEG 可用 | C-PHY 三 trio；4000×3000 RAW10 |
| 超广角 IMX481 | RAW、预览、JPEG 可用 | 4656×3496 RAW10 |
| 长焦 S5K3M5 | RAW、预览、JPEG 可用 | 4000×3000 RAW10；独立单次 AF 已实测 |
| 前摄 S5K3T1 | RAW、预览、JPEG 可用 | 2592×1940 Bayer 模式 |
| 系统相机 | GNOME Snapshot 50 四摄切换可用 | libcamera CPU 软件 ISP → PipeWire → GStreamer pipewiresrc |
| 方向 / 自动旋转 | 竖横屏及照片方向曾校验 | 使用校准后的加速度计及安装方向；未据此宣称所有惯性传感器均已适配 |
| 全视野 | 取消软件 ISP 中央裁切 | 按比例缩放完整有效画面，比例不同时留边 |
| GPU | PRIME 导入及 EGL 压测曾通过 | #8 内核含已有上游 DMA-BUF 释放修复；相机仍走 CPU ISP |
| 稳定性 | 约五分钟取流、反复切换/关闭、空闲恢复曾通过 | 有限时长测试，不代表全部应用及全天候稳定 |

相机行为验收记录主要截至 2026-11-07；2026-11-23 进行了只读设备身份核对、源码恢复和发布校验。当前纯 gzip 内核 SHA-256：71bd7461194398b0192dba4d7836154ece89be606dea68849746d61476b9e506。

## 尚未解决

- 主摄自动对焦和 OIS 尚未完成；长焦 AF 是独立单次工具，未集成系统相机连续 AF。
- 软件 ISP 的色彩、白平衡、曝光、低照度与降噪仍需标定。
- 前摄 link frequency 元数据不一致、辅摄动态曝光上限和成组控制仍需整理。
- 运行时 overlay 的数字 phandle、固定 UID/总线号、systemd/LD_PRELOAD 集成及固件依赖需要改成发行版可维护的方案；尚未提交官方 MR。
- 通话、移动数据、音频、GPS、NFC、指纹、深度休眠与功耗不在现有相机验收范围内，状态保持未知。

## 文档和贡献

- [功能与验证边界](docs/STATUS.zh-CN.md)
- [源码来源、版本关系和核验范围](docs/PROVENANCE.md)
- [构建、DTB 打包、取流命令](docs/BUILD.zh-CN.md)
- [相机链路、修复原因和 AF 接口](docs/CAMERAS.zh-CN.md)
- [向官方 postmarketOS 贡献的路径](docs/UPSTREAM.zh-CN.md)
- [本次发布校验](validation/publication-20261123.md)

20261102 目录是历史快照；最终 CAMSS 位于 recovered-final 目录。安装外置模块需要匹配内核配置和完整构建产生的 Module.symvers。请按构建文档复核设备树、电源和固件依赖。

## License and attribution

Kernel files retain their original SPDX/copyright notices; libcamera files retain LGPL-2.1-or-later notices. See [licensing and provenance](NOTICE.md). No personal photographs, credentials, factory EEPROM/persist calibration or proprietary firmware are included. Canonical public Linux selftest key fixtures remain in the unchanged upstream source archive and are not user credentials.

English: Experimental four-camera bring-up, Snapshot integration, full-field RAW10 resampling, orientation handling and verified telephoto one-shot AF. The release contains the kernel matching the installed device, the complete fixed upstream source archive and corresponding patches/configuration/final external module sources. Source reconstruction was verified; a fresh clean-room kernel rebuild and production readiness are not claimed.
