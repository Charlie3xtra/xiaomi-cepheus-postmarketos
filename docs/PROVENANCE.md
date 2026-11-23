# Source provenance and recovery gaps

Publication assembled 2026-11-23 from explicitly selected source archives and the 2026-11-06 handoff. Per-file SHA-256 and archive provenance are recorded in SOURCES.json. Generated modules, personal media, device logs containing identifiers, boot partition dumps, EEPROM/persist and proprietary firmware are excluded.

## Recovered

- kernel/snapshots/20261102: historical CAMSS stripe-fix snapshot, sensor sources and early board DTS. Most files contain existing upstream code; this is not a claim of new authorship of the complete files.
- kernel/snapshots/20261106/drivers: complete four-camera sensor source handoff. Sensor source hashes match the earlier quality archive; this handoff also documents comparison with the installed modules on that date.
- kernel/device-tree/20261106: auxiliary overlay and selected live camera/power fragments. Numeric phandles are tied to that exact tree. The fragments are not a complete standalone board DTS.
- userspace/libcamera/snapshots/20261106: sensor helper/properties, RAW10 full-field helper and patch. The patch is relative to a locally modified libcamera tree, not guaranteed to apply to pristine upstream by itself.
- userspace/integration/20261106: Snapshot copy/orientation interposition source, launch wrappers and orientation bridge. Full service/firmware setup is not included in this handoff.
- tools/autofocus: 2026-11-07 corrected telephoto AF C++ source derived from the local ZCode prototype. Documentation comments are corrected during publication; algorithm and access guard retained.
- kernel/wifi/20261031: historical candidate ath10k patch and package recipe. The candidate was build/control-flow tested at the time, not established as the deployed final Wi-Fi/5GHz fix.

## Missing / not yet independently matched

- Complete source, base commit and patch series for Linux 6.17.0-sm8150 #8.
- Final CAMSS v3 used for the later stream/switch/idle tests; the Nov 2 snapshot predates those fixes.
- Exact final GPU backport/build tree and FastRPC/SSC module source. The original upstream GPU fix is archived under kernel/upstream-references; it is not a newly authored patch.
- Full final display/touch/Bluetooth/Wi-Fi diff series and any per-device firmware packaging.
- Fully audited base/diff for the latest ZCode libcamera rebuild. The current software-ISP/simple-pipeline/IPA/helper files have now been recovered from its device build tree, together with installed startup services.

No compiled kernel is released under a claim that the recovered fragments alone are its complete corresponding source. The local state backup contains /boot files, but /boot/boot.img was already known to be stale; it must not be promoted to the verified final image.

## Historical identity references (not downloadable artifacts)

Verified installed boot-gpu-ssc-nominal.img: 25,145,344 bytes; SHA-256 74653da854b14450e199091e396c7a02cc6e8b09720c7e8d818fd92fd5bae07f.

Final CAMSS v3 module reference: SHA-256 d997de3ba47d155e1c9e340e155c2d526e96f624c4842cf2377304d3f85cc96b.

These hashes identify earlier verification records; they are not a substitute for recovering the matching sources or auditing a new device dump before publication.

## Read-only check on 2026-11-23

USB SSH confirmed Linux 6.17.0-sm8150 #8; installed CAMSS and libcamera hashes still match the Nov 7 recorded state. The current kernel configuration was read from /proc/config.gz and archived under kernel/configs. This is an identity check, not a fresh stream/stress test. Additional current libcamera source files and startup/udev configuration are under the dated 20261123 directories.
