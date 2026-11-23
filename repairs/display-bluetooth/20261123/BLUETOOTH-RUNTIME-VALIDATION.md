# Bluetooth runtime validation — 2026-11-23

Confirmed on the installed cepheus 6.17.0-sm8150 #8 kernel. This is not a claim that every Bluetooth feature or the new boot integration is finished.

## Confirmed runtime sequence

1. A normal reboot cleared the first QUP-wrapper experiment. The original Bluetooth UART at0xa84000 is the wrong route: its loaded GENI protocol is I2C. The vendor-routed UART at0xc8c000 (GPIO43–46) was unused and successfully enumerated with the guarded route-only module.
2. The physical controller responded with product0x0a, SOC0x40010224, ROM0x1001 and patch0x6699. The driver derived revision01 firmware names, which the distribution package does not provide.
3. The phone's original bluetooth partition was mounted read-only (vfat, nosuid,nodev,noexec), the stock21 rampatch/NVM and board variants were staged privately, and the partition was unmounted. The stock rampatch build label was BTFM.CHE.2.1.5-00254-QCACHROMZ-1.
4. Temporary diagnostic01 aliases selected the stock21 pair. The controller completed QCA UART setup. This diagnostic used the stock generic NVM; board-specific selection is a separate next test.
5. The controller then appeared only in the unconfigured MGMT index list, with missing public-address option2. Common persist-file locations had no Bluetooth identity record. A standard read-only QMI DMS Get MAC Address query returned the factory identity. Qualcomm HAL uses this Bluetooth result in HCI little-endian order; the private canonical file stores reversed bytes. No random identity was substituted.
6. Applying that identity through the standard MGMT Set Public Address command cleared missing options, produced one configured controller, and BlueZ Powered became true.
7. A12-second scan discovered14 distinct nearby devices. Concurrent wlan0 gateway pings were60/60 with zero loss, average1.856ms. No new Bluetooth timeout/error message appeared during that test. Display, orientation and WiFi watchdog services remained active, and the touch input was still enumerated. Discovery stopped after the test.

Neither the local address nor neighboring device names/addresses were published. The WLAN factory address was also retrieved privately, but the active WiFi address configuration has not been changed.

## Startup integration: trial prepared, reboot result pending

Commit9e3285e4be3a0ef35ca2081bfd6897ac06b162ac adds a firmware-selecting module that has the same guards as the route-only version. It uses this standard string-list property (NVM stem, then rampatch):

    firmware-name = "cepheus-crnv21", "cepheus-crbtfw21.tlv";

The NVM stem requests board ID and chooses the matching .bXX file, with .bin fallback. The phone privately holds original .b44/.b46/.b47/.b55/.b71 variants as well as generic .bin. The two diagnostic01 aliases were removed before the reboot test, so they cannot mask a failed explicit selection.

The accompanying userspace/cepheus-bluetooth-start and userspace/cepheus-bluetooth.service have been installed and staged for one trial. Python syntax and systemd verification passed. The helper's MGMT parser, UART identification and private identity comparison also passed a read-only integration check against the running configured controller.

The helper is specific to the verified kernel and guarded module, checks the installed module checksum and exact firmware-name property, configures only the UART13 controller, and refuses to replace an already configured different identity. The private address must be root-owned0600. A trial policy is atomically changed to disabled and fsynced before module loading, so a hardware failure cannot create an automatic repeated-loading loop. Only a verified boot should change policy to enabled. Disabling the service and rebooting removes the runtime overlay; do not rmmod an overlay module with no exit handler.

At the time of this record, the reboot did not restore USB SSH within the bounded observation, and no Qualcomm crash-mode USB device was seen. Manual power-state inspection was requested. It is not yet known whether shutdown, startup or Bluetooth caused the absence. Do not describe this native firmware-selection artifact or startup service as hardware-validated until post-boot logs establish that.

No boot partition was intentionally written. Installing linux-firmware-qca invoked the normal mkinitfs/boot-deploy trigger and regenerated files under /boot; those generated images are not validated replacements for the private accepted boot image. The stock firmware partition and persist partition were only read-mounted and unmounted. A temporary qmi-utils probe had a symbol mismatch with the existing SSC-customized libqmi; the utility package was removed and the customized library preserved. The successful address query used a narrowly scoped standard Python QRTR DMS request instead.

## Remaining validation

Native board-ID/NVM choice, repeatable reboot initialization, longer coexistence and power cycling, real pairing/reconnect, suspend/resume and Bluetooth audio remain separate checks. No pairing was attempted with unknown nearby devices.

Primary references: [Qualcomm controller binding](https://github.com/torvalds/linux/blob/v6.17/Documentation/devicetree/bindings/net/bluetooth/qualcomm-bluetooth.yaml), [driver firmware selection](https://github.com/torvalds/linux/blob/v6.17/drivers/bluetooth/btqca.c), [Qualcomm HAL address handling](https://github.com/Sushrut1101/android_vendor_qcom_proprietary/blob/39c82425849dc674f9e65a45fa3df3065f527652/bluetooth/hidl_transport/bt/1.0/default/bluetooth_address.cpp), [BlueZ MGMT protocol](https://github.com/bluez/bluez/blob/master/doc/mgmt-protocol.rst). A same-ID controller with the same revision derivation issue was independently tested by the [Hotdog port](https://github.com/Sr-0w/hotdog-linux-bringup/blob/main/docs/evidence/2026-08-04-mainline616-bluetooth.md); Mi9's own stock firmware and runtime results provide the device-specific evidence above.

## Follow-up: reboot with verified Bluetooth startup

The board-ID trial booted but Bluetooth setup failed. The controller reported its version and the rampatch transfer proceeded. The board-ID query and subsequent NVM transfer timed out. The trial automatically disarmed.

The installed helper now loads the verified route-only UART13 module. It checks that the two requested firmware paths are local symlinks from crbtfw01.tlv to cepheus-crbtfw21.tlv and from crnv01.bin to cepheus-crnv21.bin. These are the same original Xiaomi firmware and generic NVM that passed the runtime scan. Bypassing the board-ID query restored initialization. The module source and overlay are in runtime-uart13 at commit 1fea98777bd652eba47eb6a85b4b6db3493a3f51. The matching module is now included as artifacts/cepheus_uart13-route.ko.

- A normal software reboot restored USB SSH without physical intervention. The startup service completed, and BlueZ powered on the controller.
- A 15-second scan found 15 devices. Concurrent Wi-Fi pings at 5180 MHz were 60/60, with zero loss and 1.542 ms average gateway RTT.
- An additional Bluetooth off/on cycle completed. The driver logged a successful UART setup and zero firmware download errors.
- ServerDeck, touch enumeration and orientation recovered. The display patch persisted. The boot-time DSI timeout and Bluetooth frame-reassembly warnings still appeared.
- Factory addresses and original Xiaomi binary firmware remain stored on the individual phone. The startup policy was enabled after testing. Disable the service and reboot to roll back the runtime overlay.

This verifies startup, scanning, a power cycle and basic Wi-Fi coexistence on this unit. Pairing, audio, suspend/resume, board-specific NVM and sustained radio traffic remain open.
