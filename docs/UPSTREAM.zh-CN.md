# 官方贡献政策与当前限制

核对日期：2026-11-23。官方现名 Nura，pmaports 位于 https://gitlab.nura.eco/nura/pmaports 。

## 当前不提交官方 MR

官方 [AI Policy](https://docs.nura.eco/policies-and-processes/development/ai-policy.html) 明确禁止：

> Submitting contributions fully or in part created by generative AI tools to Nura.

本项目的代码、分析及文档由设备所有者与多个生成式 AI 开发助手协作完成，因此不符合该政策，不能按此前说明直接向官方提交。人工审阅或标记 Draft 不构成政策例外。本次没有创建官方 fork、Issue 或 MR，没有联系维护者请求绕过政策。

此前版本只核对了 fork/MR 流程、打包和审核规则，遗漏 AI 政策，因此“可以贡献”的无条件表述不准确，在此更正。公开的独立 GitHub 源码与内核归档继续保留；它们不代表官方接收、合并或支持认证。

## 已核对的技术差异

- 当前 pmaports main 基线：acc07c840169b591c4dfaf9c860f91de9b6ee495。
- 官方 linux-postmarketos-qcom-sm8150 包为 7.0.0-r3；本项目实机验证为 6.17.0-sm8150 #8。不能把旧包整套覆盖到官方共享内核。
- 当前 v7.0.0-sm8150 的 CAMSS 仍有 need_vc_update 条件路径，曾计划提取一个独立流切换修复；发现 AI 政策后已停止官方提交准备。未完成当前 7.0 的完整编译/实机验证，未提交该候选。
- 四摄驱动、静态 DTS、FastRPC/SSC、供电、libcamera 全视野及应用集成仍是独立项目中的实验成果。

## 独立项目后续整理方向

按功能拆分补丁，固定新内核基线，逐步替换数字 phandle 的运行时 overlay，核对 bindings/电源错误回退、固件依赖与发行版打包，补齐干净环境构建及实机回归。已有上游 GPU 修复保持原作者归属。这些技术工作本身不消除官方 AI 政策的限制。

官方通用文档：[审核规则](https://docs.nura.eco/pmaports/main/merge-requests/approval-rules.html)、[提交风格](https://docs.nura.eco/pmaports/main/merge-requests/commitstyle.html)、[打包规范](https://docs.nura.eco/pmaports/main/packaging.html)。这些流程要求不能替代 AI Policy。
