# Review candidate: vendor-routed UART13

Prepared only; **not loaded, no working Bluetooth claim**. Review BLUETOOTH-UART-FINDINGS.md first. The original QUP0-only experiment is superseded for Bluetooth routing.

Files: artifacts/cepheus_uart13.ko, runtime-uart13/cepheus_uart13.c, cepheus-uart13.dts, compiled dtbo, generated overlay_blob.h and embed.py. SHA256SUMS covers all files. Vermagic is6.17.0-sm8150 SMP preempt mod_unload aarch64. Built using the isolated #9 source copy (only the two documented built-in DRM changes over #8), identical .config, and matching #8 Module.symvers. No module API change. Checkpatch source:0errors/0warnings. No cleanup_module symbol: rollback is by reboot, not rmmod.

The module refuses to apply while the previous QUP0 overlay is active: it requires QUP0 unavailable with no platform device, old UART9 available in DT but unpopulated, and the old BT child/supply values matching the exact baseline. Thus **first reboot to clear the QUP0 experiment**; do not hot-remove its UART or force unload. This guard may intentionally refuse other DT variants instead of attempting adaptation.

QUP2 must already be available, platform-populated, and OF_POPULATED_BUS. UART13 must be disabled and have no platform device or existing BT child. Same-address I2C/SPI alternatives must be disabled/unpopulated. It validates the old BT's four regulator phandles0x44..0x47 against the expected regulator names and max-speed3200000. These existing phandle values are deliberately specific to the verified final #8 DT; no regulator voltage is programmed by this module.

Overlay changes only:
- Disable old UART9 and its BT child.
- Enable existing UART13 at0xc8c000.
- Add a BT child there with a byte-for-byte copy of the old BT compatible, four regulator references and maximum speed.

Neither wrapper's status changes. In particular QUP2 is not reset, disabled, or repopulated as a whole, preserving the touch I2C sibling. No existing serial-engine register/IRQ/clock/pinctrl values are rewritten. Before the coordinator elects to load, review GPIO43-46 pinctrl owners and existing SE devices using standard sysfs/debugfs. Loading can invoke normal UART and QCA power/probe routines and is a hardware experiment; the guards are not proof of electrical correctness.

Successful overlay apply alone is insufficient. Require UART protocol2, QCA version response, the actual requested firmware, and later scan/traffic/on-off tests. On protocol mismatch stop, keep SSH, and reboot to undo; do not bypass the protocol check or load random GENI firmware.

Build:
```sh
cd runtime-uart13
dtc -@ -I dts -O dtb -o cepheus-uart13.dtbo cepheus-uart13.dts
python3 embed.py
make -C "$KERNEL" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= M="$PWD" modules
```

For a persistent candidate only after runtime validation, apply this dtbo to the exact private baseline with fdtoverlay. uart13-static-validation.json proves890->891nodes, all memory reservations preserved, and an exact property whitelist: old UART status, old BT status, new UART status and six new BT properties. The candidate DTB SHA256 is6f60a69343441026542a5a2bcabe8e103c9e6154f8d964fcd422e327b0efa284. The full DTB/boot remain private; repack-private-boot.py can combine it with the original #8 gzip Image and owner ramdisk. Temporary boot testing only; no partition writes are authorized by this candidate preparation.
