# 相机链路与实际修复

## 链路

sensor RAW10 → CSIPHY → CSID → VFE/RDI/DMA → V4L2/media graph → libcamera simple pipeline + CPU software ISP → WirePlumber/PipeWire → GStreamer pipewiresrc → Snapshot 预览/JPEG。

驱动成功出 RAW 不等于色彩、对焦、应用切换和方向都已完成。沿链路逐段验收才能区分问题。

## 为什么后来能工作

- 根据实际 C-PHY 发送模式和接收配置修正模式/PHY 初始化，不能照搬其他手机 D-PHY 四 lane 模式。
- 关闭缺少正确宽高配置的 CSID 格式测量，避免错误导致 RDI halt；中断次数和粘滞状态位本身不足以证明有有效像素。
- 后续通过 CSIPHY 接收侧局部复位处理主摄竖纹/接收状态残留；精确代码以归档快照和最终 v3 为准。
- 最终 CAMSS 流切换重新配置、reset/IRQ 竞态和 PM 引用配对修复，以及 CAMCC 对应固定 847MHz 的 NOM 供电工作点，共同改善关闭/切换/空闲后恢复。仅改应用旋转或重复写寄存器不能替代这些修复。
- Snapshot 的拷贝兼容处理阻止此前每帧 FD 增长；方向处理用加速度计和安装角度修正 TAG。
- 软件 ISP 原本中央裁切较小区域导致主摄“放大”。全视野补丁独立缩放 CFA 四平面，保留完整输入视野，在比例不同时留边，统计排除留边。

## 四摄映射

| 型号 | 实测 Linux I2C client | PHY | 输出 |
| --- | --- | --- | --- |
| IMX586 主摄 | 2-001a | 1 / C-PHY 三 trio | 4000×3000 RGGB10 |
| IMX481 超广角 | 1-001a | 0 / D-PHY 四 lane | 4656×3496 RGGB10 |
| S5K3M5 长焦 | 4-0010 | 3 / D-PHY 四 lane | 4000×3000 GRBG10 |
| S5K3T1 前摄 | 4-002d | 2 / D-PHY 四 lane | 2592×1940 GRBG10 |

总线编号可能随树和枚举改变。辅摄节点由 DT overlay 创建，cepheus_aux.c 负责绑定、上电和控制，不是在驱动内部自行扫描创建三颗传感器。

## 曝光与增益

曝光写 0x0202、模拟增益写 0x0204、VTS 写 0x0340，都是两字节。内核 V4L2 控制传寄存器 code，倍率换算在 libcamera helper：

| 型号 | 当前软件采用公式 | code 范围 |
| --- | --- | --- |
| IMX586 | G=1024/(1024-code) | 0…992 |
| IMX481 | G=1024/(1024-code) | 0…960 |
| S5K3M5 / S5K3T1 | G=code/32 | 32…512 |

这记录当前实现，不是重新完成全部增益标定。曝光秒数约为 CIT×HTS/pixel_rate，时序还需对照实测帧周期。aux_ctrl 的动态曝光上限和 group hold 有待完善。

## AF

11 月 7 日实测只确认长焦 AK7374 单次 AF：10-bit code 左移 6 位，reg 0x02=0 进入 active，等待移动稳定后丢弃队列中旧帧，XRGB8888 转亮度梯度评分，stop 与 request callback 同步。

主摄 LC898217XC 的身份线索不等于其供电、地址、初始化或 OIS/AF 已验证。不要把老原型的主摄默认参数当成安全接口；发布工具有明确长焦访问白名单。

独立 lens subdevice、V4L2_CID_FOCUS_ABSOLUTE、sensor lens-focus 关联与 libcamera AF 算法才是应用集成的后续方向。当前独立工具不能宣称系统相机已经具有连续 AF。
