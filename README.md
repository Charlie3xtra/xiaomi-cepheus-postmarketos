# Xiaomi Mi 9 (cepheus) — Linux / postmarketOS bring-up

小米 9 的社区适配源码、实验补丁和验证记录。项目建立在 Linux、SM8150 mainline、postmarketOS、libcamera 等上游工作的基础上；保留原作者及许可证，不把已有驱动归为本项目原创。

**发布状态：源码恢复版，尚非可一键刷写的发行镜像。** 本次公开整理于 2026-11-23；最近的相机行为验证记录截至 2026-11-07，11 月 23 日通过 USB 只读核对了现用内核、CAMSS 和 libcamera 校验值。已找到 11 月 6 日四摄交接源码、11 月 7 日长焦 AF 修复，并从手机恢复了现用软件 ISP/服务及内核配置；完整内核构建树、最终 CAMSS v3 和 FastRPC 改动仍在从旧电脑恢复核对，GPU 上游修复已附来源。因此本仓库没有将旧内核或旧 CAMSS 冒充为最终版本，也暂不提供对应源码不齐全的内核二进制。

## 功能状态

| 功能 | 已有结果 | 证据与边界 |
| --- | --- | --- |
| 屏幕、触控 | 项目使用中可用 | 屏幕修复涉及 boot 中附加 DTB；触控独立改动尚未完整追回 |
| Wi-Fi / 5GHz | 之前实机连接成功 | MSS 启动链和 ath10k 相关修复；本仓库旧 Wi-Fi 密钥删除补丁仍是候选，不能混作现用补丁 |
| 蓝牙 | 用户报告可用 | 尚未恢复完整源码差异及独立测试记录 |
| 主摄 IMX586 | RAW、预览、JPEG 可用 | C-PHY 三 trio；4000×3000 RAW10 |
| 超广角 IMX481 | RAW、预览、JPEG 可用 | 4656×3496 RAW10 |
| 长焦 S5K3M5 | RAW、预览、JPEG 可用 | 4000×3000 RAW10；独立单次 AF 已实测 |
| 前摄 S5K3T1 | RAW、预览、JPEG 可用 | 2592×1940 Bayer 模式；不直接使用旧 Quad Bayer 全尺寸模式 |
| 系统相机 | GNOME Snapshot 50 四摄切换可用 | libcamera 软件 ISP → PipeWire → GStreamer pipewiresrc；有应用兼容处理 |
| 方向 / 自动旋转 | 竖横屏及照片方向曾校验 | 来源是校准后的**加速度计**；未据此宣称陀螺仪全套已适配 |
| 全视野 | 取消软件 ISP 中央裁切 | 完整有效画面按比例缩放，比例不一致时留边；保留传感器自身模式的有效区域 |
| GPU | PRIME 导入及 EGL 压测曾通过 | 最终 #8 内核对应源码恢复中；相机仍走 CPU ISP |
| 稳定性 | 约五分钟取流、反复切换/关闭、空闲恢复曾通过 | 有限时长验证，不等同全天候/所有应用稳定 |

## 尚未解决

- 主摄自动对焦、OIS 尚未完成；长焦 AF 是独立单次扫描工具，未集成系统相机连续 AF。
- 软件 ISP 色彩、白平衡、曝光、低照度和噪声处理仍需标定，不能达到官方 Android 相机的完整 ISP 效果。
- 前摄 link frequency 元数据存在驱动与旧 overlay 不一致；辅摄曝光范围随 VBLANK 更新及成组控制仍需整理。
- 最终内核/CAMSS/传感器启动服务的完整可重复构建链尚未恢复；硬编码数字 phandle 的运行时 overlay 需要转成静态 DTS/binding。
- 通话、移动数据、音频、GPS、NFC、指纹、深度休眠及耗电未由这些相机测试覆盖；状态保持未知。

## 从哪里看

- [功能与验证边界](docs/STATUS.zh-CN.md)
- [源码来源和缺失项](docs/PROVENANCE.md)
- [构建、DTB 打包、取流命令](docs/BUILD.zh-CN.md)
- [相机链路、修复原因和 AF 接口](docs/CAMERAS.zh-CN.md)
- [向官方贡献的路径](docs/UPSTREAM.zh-CN.md)
- [11 月 6 日四摄驱动](kernel/snapshots/20261106/drivers/)
- [全视野补丁](userspace/libcamera/snapshots/20261106/)
- [长焦 AF 源码](tools/autofocus/)

这里的代码用于工程复核。20261102 目录明确是历史快照，不要用它覆盖最终运行中的 CAMSS。安装模块必须匹配完整内核构建和 Module.symvers；不能只因为 uname 相同就认为 ABI 兼容。不要裸读未上电 CAMSS/SNOC MMIO。

## License and attribution

Kernel files retain their SPDX notices (mostly GPL-2.0); libcamera files retain LGPL-2.1-or-later notices. See [licensing and provenance](NOTICE.md). This is an experimental recovery repository, not an official postmarketOS release. Firmware, proprietary vendor modules, factory EEPROM/persist calibration, personal photographs and credentials are not included.

English: Four-camera Linux bring-up, Snapshot integration, full-field RAW10 resampling and experimentally verified telephoto one-shot autofocus are archived here. Historical test results are distinguished from source availability. A complete reproducible final kernel release is still pending recovery; do not flash historical artifacts as a replacement.
