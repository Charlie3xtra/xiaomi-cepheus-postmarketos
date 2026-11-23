# Source provenance and version relationships

Assembled 2026-11-23 from selected local archives, the 2026-11-06 source handoff, the current phone build tree and the original WSL build environment. SOURCES.json records repository-file SHA-256 and origins; kernel/recovered-final/20261123/SOURCE-MANIFEST.json is the kernel export's own manifest. Original SPDX/copyright notices are retained.

## Final kernel and external modules

See [the recovered kernel documentation](../kernel/recovered-final/20261123/README.md). The fixed v6.17.0-sm8150 archive (90,562 files) matches local root bdec564f41064359aa4e2f4cd9993c4badb9b7a7. Two complete patches reproduce local head 13caddd41289c63df0cdeeb41fee722cf6947051 plus its Image #8 worktree changes. Final external CAMSS v3 and sensor FastRPC sources/patches are provided separately because they postdate that Image's in-tree CAMSS.

The recovered .config is byte-identical to the phone's /proc/config.gz extraction. The published pure gzip kernel is byte-identical to the phone's current boot kernel payload (SHA-256 71bd7461194398b0192dba4d7836154ece89be606dea68849746d61476b9e506). The retained final CAMSS module reference also matches the phone (d997de3ba47d155e1c9e340e155c2d526e96f624c4842cf2377304d3f85cc96b).

Archive-to-root, committed tree, dirty worktree files, final external module sources and recompiled static DTS property/reservation comparisons all passed. This establishes recovered source/artifact provenance, not a fresh clean-room byte-for-byte kernel build. The old APKBUILD is retained as historical packaging provenance; the actual #8 build used GCC 14.2 and differs from that historical LLVM recipe.

Display/touch/Bluetooth support includes existing upstream drivers and final board configuration. No separate newly authored final driver patch was found for each feature; do not claim otherwise.

## Camera and userspace

- kernel/snapshots/20261106/drivers: complete sensor source handoff, compared with installed modules at the time. Sensor modules are separate from the userspace telephoto AF utility.
- kernel/device-tree/20261106: auxiliary runtime overlay and selected live power fragments. Numeric phandles only fit that tree; fragments are not a standalone board DTS.
- userspace/libcamera/patches: current device source compared with official v0.7.2, commit 191e202178f02430b5942397c70d215cdd2056fa. All 1,483 upstream regular files were compared; five source files differ. The feature patch also adds the RAW10 helper and five YAML files. Apply to the pristine archive reconstructed all 11 changed/added files successfully.
- userspace/libcamera/snapshots/20261123: software ISP, simple pipeline/IPA/helper/property source recovered from the current device build tree. Downloaded third-party subprojects are not treated as local modifications.
- userspace/integration/20261106 and userspace/installed-config/20261123: Snapshot copy/orientation shim source and current startup/udev configuration.
- tools/autofocus: Nov 7 corrected telephoto one-shot AF C++ prototype. Documentation comments corrected for publication; algorithm/access guard retained.

## Historical material

20261102 CAMSS, board DTS and integration directories precede the final switching/PM fixes and are explicitly historical. kernel/wifi/20261031 contains the earlier candidate and tests; the actual final #8 Wi-Fi delta is in recovered-final and must not be double-applied. The old patches remain useful records, not additional steps to apply indiscriminately.

## Publication boundaries

The original boot dump/ramdisk has per-device content and remains private. No device firmware, EEPROM/persist calibration, user keys, credentials, personal photos or raw private logs were published. The unchanged upstream Linux archive includes canonical public selftest key fixtures, not user credentials.

Historical boot reference SHA-256 74653da854b14450e199091e396c7a02cc6e8b09720c7e8d818fd92fd5bae07f is an identity reference, not a downloadable full boot image. /boot/boot.img was known to be stale and was not used as the final publication baseline.
