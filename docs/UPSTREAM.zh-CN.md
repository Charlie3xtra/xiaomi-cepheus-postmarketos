# 向 postmarketOS 贡献

核对日期 2026-11-23。官方 pmaports 文档当前位于 Nura 域名，原 postmarketOS 文档链接会重定向。

**可以贡献。** 通常先 fork 官方 pmaports，在自己的 fork 建分支，再向官方 main 发 Merge Request；GitHub 公开仓库能用来保存源码和协作，但不会自动进入官方镜像，也不代表获得官方支持状态。

参考：[MR 审批规则](https://docs.nura.eco/pmaports/main/merge-requests/approval-rules.html)、[提交风格](https://docs.nura.eco/pmaports/main/merge-requests/commitstyle.html)、[打包规范](https://docs.nura.eco/pmaports/main/packaging.html)。多数常规 MR 需要维护者及团队审核；以官方现行规则为准。

## 建议拆分

1. 基线审计：对照官方现有 xiaomi-cepheus 设备包和 SM8150 内核，固定 commit。不能在未核对官方设备页时声称“官方只能开机”，也不能重复归功于已有触控/蓝牙驱动。
2. 板级 DTS：保留显示/MSS，整理 CAMCC OPP、SSC reserved-memory/FastRPC 绑定及摄像头端点。改成 label/phandle 引用，移除只适用于一棵运行树的数字常量和临时 overlay 模块。
3. 内核：把 CAMSS C-PHY、接收复位、切换与 PM 生命周期修复拆开。GPU 已有上游 Rob Clark 的 c34e08ba6c0037a72a7433741225b020c989e4ae，优先用上游修复/升级，不能冒充新写的补丁。独立 sensor/lens 驱动、binding、供电错误回退需审查。
4. 用户空间：libcamera sensor helper/属性、全视野缩放各自向对应上游提交；厂商 CCM 是可用的调参线索，不能据此宣称整套 Android ISP 已移植。
5. 打包与验证：按 APKBUILD 固定 source/checksum，更新设备依赖和版本，完成 kernel config/device/CI 检查。当前依赖 systemd、/opt 前缀、LD_PRELOAD、固定 UID/总线号和私有标定目录的脚本需改成发行版可维护方案。
6. 提供实机验收：四摄开关/切换/空闲恢复、GPU PRIME、冷启动传感器、2.4/5GHz、蓝牙、触控；记录每个版本的测试边界和已知问题。不能只用“可编译”代替“可运行”。

设备专属提交可采用官方允许的 xiaomi-cepheus: ... 标题。修改共享 SM8150/CAMSS 时，还需考虑其他设备回归测试。

## 本次状态

本次完成 GitHub 公开源码整理和贡献路径调查，尚未向官方发 MR，也没有改动官方设备状态。最终内核对应源码、配置、补丁和外置模块源码已找回并通过重建对照；下一步需要在干净环境完整构建和实机回归，再整理成小而可审查的 MR 补丁系列。
