# 构建和取流

这是已恢复源码的复核说明，尚非完整最终内核发布配方。请先看 PROVENANCE.md；同一个版本字符串不能替代 ABI、配置和 Module.symvers 匹配。

## Sensor 模块

在具有匹配 #8 内核完整构建树及 Module.symvers 的 Linux/WSL 中，从仓库根目录运行：

```sh
export KERNEL_BUILD=/path/to/matching/linux-v6.17.0-sm8150
make -C "$KERNEL_BUILD" M="$PWD/kernel/snapshots/20261106/drivers" \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 modules
```

这是编译 sensor 模块的命令，不是构建最终内核的替代品。20261102 CAMSS 是历史版本，不能直接覆盖最终 v3。

## 辅摄 overlay

```sh
dtc -@ -I dts -O dtb -o kernel/device-tree/20261106/cepheus-aux.dtbo \
  kernel/device-tree/20261106/cepheus-aux-overlay.dts
python3 tools/boot/embed-overlay.py kernel/device-tree/20261106/cepheus-aux.dtbo \
  kernel/device-tree/20261106/overlay_blob.h
make -C "$KERNEL_BUILD" M="$PWD/kernel/device-tree/20261106" \
  ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 modules
```

overlay 包含特定原始 DTB 的数字 phandle，并有机器/电源节点校验。不能拿它直接套其他版本的设备树。live-camera-power-fragments.dtsi 是摘录，非独立可编译板级 DTS。

## DTB 重打 boot.img

实际使用的是 Android boot v0、4096 字节页、gzip Image 尾部附加 DTB。历史修复没有使用一条通用 mkbootimg 命令，而是保留头、内核、ramdisk 和 cmdline，只替换附加 DTB 并重算 kernel_size/boot ID。

```sh
python3 tools/boot/repack-current-dtb.py --base-boot /private/verified-current-boot.img \
  --dtb /path/to/edited-static.dtb --out /path/to/new-boot.img
```

工具只生成文件，不刷分区。保留 SSC 内存池/FastRPC 设置、CAMCC NOM/256，以及现有显示/MSS/Wi-Fi 节点。不能使用已叠加 runtime overlay 的整棵运行树作为新的静态 boot 基线；不能直接拿已知过时的 /boot/boot.img。

make-ssc-pool-boot.py 和 prepare-nominal-dtb.py 是历史配方副本，依赖原 evidence/boot 布局和固定哈希，不是通用打包器。

## libcamera 和应用

原版本为 libcamera 0.7.2，安装前缀 /opt/cepheus-camera。20261123 目录从手机实际构建树恢复软件 ISP、simple pipeline、simple IPA、sensor helper/properties 等代码；20261106 另有全视野补丁。后者不是针对干净上游的独立全套补丁，不要重复应用到已经包含此改动的 20261123 快照。完整基线/本地差异仍需核对。

Snapshot 使用 userspace/installed-config/20261123 中的启动环境；方向 shim 源码为 userspace/integration/20261106/snapshot-orientation.c，重建示例：

```sh
cc -shared -fPIC -O2 userspace/integration/20261106/snapshot-orientation.c \
  -o /tmp/libcepheus-camera-orientation.so $(pkg-config --cflags --libs gstreamer-1.0) -ldl
```

先只生成临时产物，按已验证的部署布局审查后再安装。复制、方向 TAG、libcamera ABI、PipeWire、会话环境和服务依赖须一起检查。应用仍特意使用 cairo/CPU 路径以避开特定 DMA-BUF 兼容问题；GPU 修复不等于已经启用硬件相机 ISP。

## 取流

手机用户的 XDG_RUNTIME_DIR 当前是 /run/user/10000；其他安装应换成自己的会话路径。先关闭其他相机客户端，等待 PipeWire 枚举完成：

```sh
XDG_RUNTIME_DIR=/run/user/10000 gst-launch-1.0 -e \
  pipewiresrc target-object=libcamera_input._base_soc_0_cci_ac4a000_i2c-bus_1_camera_1a \
  always-copy=true num-buffers=300 ! video/x-raw,width=640,height=480 ! fakesink sync=false
```

节点后缀：主摄 ac4a000 / i2c-bus_1 / camera_1a；超广角 ac4a000 / i2c-bus_0 / camera_1a；长焦 ac4b000 / i2c-bus_1 / camera_10；前摄 ac4b000 / i2c-bus_1 / camera_2d。以当前 pw-cli 实际枚举为准。

保存最后一张已应用方向的 JPEG（目录自行准备）：

```sh
XDG_RUNTIME_DIR=/run/user/10000 \
LD_PRELOAD=/opt/cepheus-camera/lib/libcepheus-camera-orientation.so \
gst-launch-1.0 -e \
  pipewiresrc target-object=libcamera_input._base_soc_0_cci_ac4a000_i2c-bus_1_camera_1a \
  always-copy=true num-buffers=60 ! video/x-raw,width=640,height=480 ! \
  videoconvert ! videoflip video-direction=auto ! jpegenc quality=95 ! \
  multifilesink location=/tmp/cepheus-main-%03d.jpg max-files=1
```

不要同时用直接 V4L2 诊断脚本和 PipeWire 改同一 media graph。早期脚本中的固定设备编号不保证在其他系统相同。
