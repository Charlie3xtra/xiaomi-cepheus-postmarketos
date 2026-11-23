# UART13 explicit revision21 firmware candidate

Prepared, compiled and statically validated; **this artifact has not been loaded or hardware-tested**. The coordinator separately reports route-only UART13 plus private original21 firmware aliases reached QCA setup, and factory-address provisioning reached configured/Powered=true. Those observations do not validate this new module, board-variant selection, scan/traffic, or reboot persistence.

The original runtime-uart13 C source and all fresh-boot guards are byte-for-byte unchanged. Module name remains cepheus_uart13 and rollback remains **reboot only**, with no cleanup_module. Do not load over the previous QUP0 or UART13 experiment; first reboot to the original #8 DT. No automatic startup installation was performed here. The coordinating agent decides when to test.

The only functional overlay change from the previous route candidate is:

```dts
firmware-name = "cepheus-crnv21", "cepheus-crbtfw21.tlv";
```

This is the actual6.17 **single string-list** property, NVM first and rampatch second; there is no standalone rampatch-name property. btqca prefixes qca/ itself. The explicit rampatch filename bypasses the broken revision01 filename derivation. The NVM stem has no extension: the driver queries the controller board ID after patch download, then attempts qca/cepheus-crnv21.bXX, with qca/cepheus-crnv21.bin as the default/failed-file fallback. Board ID0/ffff selects .bin. Verify the actual requested and successfully loaded NVM filename during testing; fallback success is not proof of the correct board variant.

The coordinator privately stages original-device firmware under these names, including only variants actually present in that device's own partition. The listed available suffixes are b44,b46,b47,b55,b71 plus .bin. All proprietary firmware, factory addresses and calibration bytes stay private and are absent from this source/module publication. No distribution firmware file is replaced or renamed by the candidate. No firmware copy or board-ID query was performed from this old computer.

compatible remains the previous qcom,wcn3990-bt for this isolated filename experiment; no SoC-type change, upstream WCN3998 ROM parser backport, regulator alteration, serial register/pin/clock change, wrapper reset, or universal btqca patch is bundled. The upstream WCN3998 fix99b2c531e0e797119ae1b9195a8764ee98b00e65 remains the source-based explanation for the revision01 derivation and a separate possible future correction.

Build command is unchanged in UART13-CANDIDATE.md. Vermagic6.17.0-sm8150 SMP preempt mod_unload aarch64; exact .config and Module.symvers remain the verified #8 values. The build tree contains only the existing #9 built-in DRM diagnostics over #8; no module ABI change. uart13-firmware-validation.json records the module/static-DTB hashes and full-tree whitelist result. Full DTB and boot remain private.

The generic NVM file itself need not contain a usable public Bluetooth address. This module does not provision an address. The coordinator's separate private factory-address startup service is still needed; a controller left unconfigured must not be diagnosed as a failed UART overlay solely from BlueZ state.

Acceptance after coordinator review: fresh boot with old guards, correct named firmware requests, board-ID result and loaded NVM, configured public address (never in public logs), Powered=true, scan/traffic, Wi-Fi coexistence, and subsequent reboot/on-off tests. Keep original boot and known-working display pwrite fix intact; no boot partition writes are part of this preparation.
