# Cepheus display diagnostics and QUP0 Bluetooth candidate (2026-11-23)

Experimental, AI-assisted local repair, not submitted to Nura/postmarketOS. No phone was accessed or flashed while preparing this branch. The existing #8 boot remains the rollback baseline. No claim of Bluetooth function or white-block resolution is made before hardware testing.

## Start with the small runtime module

`artifacts/cepheus_qup0.ko` enables only `/soc@0/geniqup@ac0000/status`. Guards require xiaomi,cepheus, disabled qcom,geni-se-qup, populated/available parent, no existing wrapper/UART platform devices, exactly two available children: serial@a84000 (qcom,geni-uart plus qcom,wcn3990-bt) and serial@a90000 (qcom,geni-debug-uart). Every other direct child must be disabled. The second UART is an existing enabled early/debug UART; enabling QUP0 allows both to probe. Existing wrapper driver devm_of_platform_populate handles enumeration. No MMIO access, regulator modifications, or manual device creation are added.

Module vermagic: `6.17.0-sm8150 SMP preempt mod_unload aarch64`. Built against the final #8 build metadata and Module.symvers in an independent copy. CONFIG_OF_DYNAMIC=y and CONFIG_OF_OVERLAY=y; original config SHA256 is in verification.json. It intentionally has **no module_exit**: normal unloading is not supported; reboot to undo. Do not force rmmod. Do not install for automatic startup before testing. OF overlay application success alone does not prove the controller probed or firmware loaded. Check the wrapper, both UARTs, /sys/class/bluetooth, rfkill and kernel firmware requests. Supply only firmware actually requested by the driver, with appropriate redistribution rights.

Build in a copy of the reconstructed #8 tree:

```sh
cd runtime-qup
dtc -@ -I dts -O dtb -o cepheus-qup0.dtbo cepheus-qup0.dts
python3 embed.py
make -C "$KERNEL" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= M="$PWD" modules
```

## Static DT candidate

`static-dt-validation.json` records the exact baseline/candidate DTB hashes and the only property difference among 890 nodes. All property keys, node paths, and memory reservations were compared. The full device DTB is retained privately; produce it using the already verified final static DTB (baseline SHA256 a46645a88482f8a4a8ca5f04de18048e3267401c121218425d2ebb2b4a3b429e):

```sh
cp final-static.dtb qup0-only.dtb
fdtput -t s qup0-only.dtb /soc@0/geniqup@ac0000 status okay
```

No sensor, camera, display, regulator, modem/WLAN, or reserved-memory properties change. Wi-Fi 5GHz code and MSS status remain the existing baseline; this work does not certify all Wi-Fi modes.

## Display findings and candidate #9

`artifacts/Image-display-diag-9.gz` is a pure gzip kernel **not an Android boot.img**. Do not pass it to fastboot flash/boot as a boot image. It retains all tracked final #8 source changes and identical .config, with only:

1. Upstream ea9dadeac79cef509a4b8b4a3e3b39a741e63313 FBINFO_VIRTFB fix, preserving original attribution and AI-assistance trailer. MSM uses a kernel virtual screen_buffer and deferred sysmem operations. This corrects metadata/warnings; the warning path still draws and is not evidence of the white blocks' cause.
2. Rate-limited logs of the raw ACK/FIFO/TIMEOUT status values already read inside the normal powered DSI error handler. No extra register reads or reset behavior changes.

In this exact tree DSI worker status=4 means FIFO, status=5 means FIFO plus timeout; MDP FIFO underflow is bit 0x8. ACK's software state constant is zero. Do not infer an MDP underflow or ACK from status=5. Encoder-disabled wait messages may occur during shutdown and do not establish a GPU failure.

fb0 uses pitch4352; boot simplefb used4320. Framebuffer writers must use line_length. Do not force the DRM pitch to4320. New-computer diagnostics identified serverdeck-agent.service (/opt/serverdeck/agent.py) as a continuous /dev/fb0 writer, correctly using stride4352/BGRX and mmap10186752. Its three-second refresh overwrote the first test patterns, so their absence was not evidence of DSI failure. With that display agent temporarily paused, the replacement pattern read back identically at0/1/2seconds; physical feedback is still pending. A black mmap capture while CRTC is inactive cannot establish the visible scanout contents. Correlate the physical test pattern result, active CRTC/FB, blank state, and timestamped DSI diagnostics before selecting a functional fix.

Panel backlight error paths leave MIPI_DSI_MODE_LPM cleared after a failed DCS transaction; this is a separate cleanup issue, not patched here because it has not been linked to the fault. Pending DSI error work versus poweroff also merits tracing; no speculative workqueue/power sequencing change is included. Upstream master retrieved on the preparation date still has the same disable_irq/worker shape.

Build:

```sh
# Reconstruct final #8 source from kernel/recovered-final/20261123 first.
# Work in an independent cp -a --reflink=auto copy, never hardlinks.
patch -p1 < patches/0001-msm-fbinfo-system-memory.patch
patch -p1 < patches/0002-dsi-existing-status-diagnostics.patch
make -j12 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= KBUILD_BUILD_VERSION=9 KBUILD_BUILD_USER=pmos KBUILD_BUILD_HOST=cepheus-repair Image
```

The image compiled successfully with GCC14.2.0/binutils2.44. No driver API or module symbol definitions were changed. Keep the installed final external CAMSS v3/FastRPC/sensor modules; the older in-tree CAMSS is not the deployed final external module. The panel is a separate module and was not changed. No hardware acceptance testing has happened here.

`repack-private-boot.py` preserves the owner's original Android v0 boot ramdisk/header and supports an optional new gzip Image plus DTB. Use only the verified current private boot (74653da854b14450e199091e396c7a02cc6e8b09720c7e8d818fd92fd5bae07f), never an old /boot/boot.img. Retain original boot for rollback. Candidate images are for **temporary fastboot boot by the coordinating agent only**, not partition writes. No private boot image or ramdisk is published.

## Complete source and provenance

The base #8 .config, pinned source archive, full patches, external modules and licenses are already in `kernel/recovered-final/20261123` and the repository Release `cepheus-6.17.0-8-recovery-20261123`. This branch adds only the two display patches, exact config, QUP overlay source/build recipe and small artifacts. Upstream code remains under its original licenses. New overlay code is GPL-2.0-only. SHA256SUMS covers published source and binaries. No proprietary firmware, calibration EEPROM/persist, private logs/photos, credentials or raw device boot dumps are included.
