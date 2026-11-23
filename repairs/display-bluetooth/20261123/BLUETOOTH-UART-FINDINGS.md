# Bluetooth follow-up: wrong UART route is the leading defect

The coordinating agent reports the guarded QUP0 overlay loaded successfully: wrapper and a84000.serial enumerated, then hci0 appeared. The UART driver repeatedly reported protocol3, and QCA version requests timed out (-110) after retries. **Bluetooth is not functional.** Enumeration only validates the disabled-parent probe problem; enabling this parent is insufficient and may be unnecessary after correcting the route. Do not persist the old QUP0-only candidate as a Bluetooth repair.

## Primary source chain

Official Xiaomi source is pinned to MiCode/Xiaomi_Kernel_OpenSource commit `136c2470e85cd0fb2fa164c7086536d472e74594` (cepheus-p-oss). Links/line evidence are in vendor-uart-provenance.json.

- cepheus-sm8150.dtsi includes xiaomi-sm8150-common.dtsi.
- xiaomi-sm8150-common.dtsi enables qupv3_se13_4uart (and the separate debug qupv3_se12_2uart).
- sm8150-qupv3.dtsi defines qupv3_se13_4uart at **0xc8c000**, GCC_QUPV3_WRAP2_S3_CLK, interrupt585, four-wire pins, wrapper qupv3_2. Debug UART is0xa90000.
- sm8150.dtsi maps **hsuart0 to qupv3_se13_4uart**, serial0 to the separate debug UART.
- drivers/tty/serial/msm_geni_serial.c uses the hsuart alias for the high-speed driver, whose device prefix is ttyHS. Thus the official hsuart0 route corresponds to /dev/ttyHS0. A proprietary userspace runtime trace was not obtained; the evidence is the official DTS/driver mapping.
- sm8150-pinctrl.dtsi selects function qup13, GPIO43/46 (CTS/RX),44(RTS),45(TX),2mA. The mainline sm8150.dtsi uart13 uses the same four pins and function; its default pull configuration is unified bias-disable rather than the vendor per-signal pulls. Do not change pulls before confirming actual UART traffic.

The current #8 board DTS instead attaches qcom,wcn3990-bt to **uart9 at0xa84000**, GCC_QUPV3_WRAP1_S1_CLK, interrupt354, under geniqup@ac0000. That is a materially different serial engine. The baseline does contain uart13 at0xc8c000 under **geniqup@cc0000**, currently disabled. The QUP2 wrapper is **already okay** because i2c@c80000 (touch) is enabled. Its i2c/spi alternatives at0xc8c000 are disabled. This was checked from the exact final static DTB.

## Meaning of protocol3

In the exact #8 include/linux/soc/qcom/geni-se.h enum: UART=2, I2C=3. geni_se_read_proto reads the configured firmware revision protocol field. qcom_geni_serial_port_setup rejects any value other than UART before FIFO setup and normal UART configuration. The UART driver has no request_firmware call in this tree. Protocol3 is **the serial engine's currently loaded I2C protocol**, not a Bluetooth HCI packet-format error and not evidence that crbtfw*.tlv is wrong. Firmware reloading on0xa84000 would risk converting an engine selected by the board bootloader for another purpose and would not fix physical routing.

The official route plus the observed I2C protocol strongly favor a wrong-SE DT route over an absent UART firmware-loader defect. They do not prove protocol2 is already loaded at0xc8c000; that must be checked through the standard UART driver's normal probe/start path, not raw MMIO.

## Recommended next experiment (not performed here)

1. Retain working USB/Wi-Fi. Reboot to undo the intentionally non-unloadable QUP0 overlay; do not force unload or hot-remove the live UART/serdev tree.
2. Prepare a fresh, guarded overlay or static DT from the known #8 baseline: keep uart9 and its bluetooth child unavailable; enable uart13 at0xc8c000 and put a qcom,wcn3990-bt child there using the existing verified regulator phandles/properties. Preserve every property of all unrelated children. Leave QUP0 at its original disabled state unless independently needed for the debug UART. **Do not disable/repopulate QUP2**, which carries the working touch I2C controller.
3. Validate UART13 clock, IRQ, pinctrl, supply phandles and that no I2C/SPI driver is bound at the same SE. Inspect pinctrl ownership via debugfs before loading, especially GPIO43-46. The existing four supply choices should be preserved initially; vendor DTS agrees on L7A/L2C/L11C roles but does not by itself establish that every mainline voltage is correct.
4. Let the standard qcom_geni_serial driver validate protocol. If UART13 is also protocol3, stop and reassess firmware loading/boot configuration with that new evidence. Do not suppress the protocol check or try random other engines.
5. Only after UART starts and QCA chip version is returned, resolve the exact firmware names requested by hci_qca. hci0 alone is not acceptance. Require HCI version/address (redacted from public logs), scan or paired-device traffic, repeated on/off and suspend/resume tests.

Following the later request to prepare a reviewable candidate, runtime-uart13 and a private static DTB were generated and validated; see UART13-CANDIDATE.md. No device operation, protocol reload, power change, or flashing was performed. Original source, main, and #8 rollback image remain unchanged.

## Display boundary update

The coordinating agent found serverdeck-agent.service writes /dev/fb0 periodically, overwriting the early patterns. Stopping that display service also runs cleanup that sets backlight brightness to0. A black screen during that test therefore does not establish DSI failure. After explicitly setting brightness300/2047, the test pattern reads back stably; physical user feedback remains pending. FBINFO_VIRTFB and the #9 diagnostics are still candidates, not a demonstrated artifact fix.
