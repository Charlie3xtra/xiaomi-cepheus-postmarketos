# Recovered Xiaomi Mi 9 / cepheus kernel sources — 2026-11-23

This is an experimental recovery/export of the retained source for Linux
`6.17.0-sm8150 #8`, its final external CAMSS v3 and sensor FastRPC modules,
and the static device tree used by the verified GPU/SSC/NOM boot image.
It is an integration of Linux, SM8150-mainline, postmarketOS and other
attributed upstream/vendor work, **not a claim that those sources are original
work by this repository**. Original SPDX/copyright headers are preserved;
see `COPYING`, `LICENSES/` and `ATTRIBUTION.md`.

## Source anchor and exactness

- Fork source: `https://gitlab.com/sm8150-mainline/linux`.
- Archive tag/name: `v6.17.0-sm8150` (this is a fork tag, not an assertion that
  the retained local Git repository contains Torvalds' `v6.17` tag).
- Fixed archive URL:
  `https://gitlab.com/sm8150-mainline/linux/-/archive/v6.17.0-sm8150/linux-v6.17.0-sm8150.tar.gz`.
- Archive SHA512:
  `274270bb0391bca972b1dffae778ff6fc372457b658e9252104d9894c9f24739e25df7021378cf0ac9c650cc417d0cdcdf1baa88bfdcd494920ee183298e1f34`.
- All **90,562 archive files** match the local initial Git snapshot
  `bdec564f41064359aa4e2f4cd9993c4badb9b7a7`, including contents and modes.
- Retained committed head:
  `13caddd41289c63df0cdeeb41fee722cf6947051`.
- Actual Image #8 worktree: committed head plus the nine tracked dirty files
  recorded in `provenance/kernel-git.json`. No Git-untracked files were found
  in that tree; the ignored `.config` and separately built modules are exported
  explicitly here.
- Exact `.config` SHA256:
  `a823dbb4b67d87fc4dff4fa4b479c00cf2b7d83bb40592e87fe4e42570b848dc`.
  This also matches the coordinating agent's read-only extraction of the
  installed phone's `/proc/config.gz` on 2026-11-23.

The release `cepheus-6.17.0-8-recovery-20261123` is intended to mirror the
unchanged upstream archive together with this source package and checksums,
so rebuilding does not depend solely on the GitLab URL remaining available.

## Layout and patch order

1. Extract the fixed upstream archive into a fresh source directory.
2. Apply `kernel/patches/0001-archive-root-to-local-head.patch`.
3. Apply `kernel/patches/0002-local-head-to-image8-worktree.patch`.
4. Use `kernel/config-image8.aarch64` and the GCC toolchain recorded below.

These two patches reproduce the source state of the retained **Image #8**.
Do not apply the historical APKBUILD's patches again on top: they are archived
as provenance, not a second patch series.

The final CAMSS/FastRPC sources were built externally after that kernel Image:

- `modules/camss-v3/`: complete final CAMSS source, headers, Makefile/Kconfig.
- `modules/fastrpc-sensors/`: complete final sensor-pool transport source.
- `modules/patches/`: optional in-tree deltas from the reconstructed Image #8
  worktree to those final module sources. Do not accidentally use the older
  CAMSS files from the retained kernel worktree as the final installed driver.
- `modules/slpi-enable/`: guarded SLPI DT overlay, generator, module and
  startup scripts; firmware and factory sensor registry are **not included**.
- `modules/camera-nominal-vote/`: legacy-kernel NOM fallback. A corrected
  static DT already votes NOM, so this fallback need not hold CAMCC awake.
- `wifi/ath10k-final-5ghz.patch`: focused view of the same Wi-Fi changes already
  contained in kernel patch 0002. Do not apply it twice.
- `packaging/`: actual retained APKBUILD/config/source patches with observed
  SHA512 checks. **All six declared source checksums matched.** The recipe's
  LLVM build is historical; Image #8 was actually built manually with GCC.

The sensor drivers, later AF work and current userspace were recovered
separately by the coordinating agent and are not duplicated here. In
particular, the sensor module source in the historical kernel tree is not a
replacement for the later AF-enabled modules. No deployment scripts in this
export write a phone partition.

## Build commands and toolchain

Recorded tools: GCC `aarch64-linux-gnu-gcc (Debian 14.2.0-19) 14.2.0`, GNU
binutils `2.44`, GNU Make `4.4.1`, DTC `1.7.2`.

From a fresh extracted source tree, with `$EXPORT` pointing at this directory:

```sh
git apply --whitespace=nowarn "$EXPORT/kernel/patches/0001-archive-root-to-local-head.patch"
git apply --whitespace=nowarn "$EXPORT/kernel/patches/0002-local-head-to-image8-worktree.patch"
cp "$EXPORT/kernel/config-image8.aarch64" .config
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 Image modules
```

Then build the final external modules against that prepared kernel tree:

```sh
make -C "$KERNEL" M="$EXPORT/modules/camss-v3" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 modules
make -C "$KERNEL" M="$EXPORT/modules/fastrpc-sensors" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 modules
(cd "$EXPORT/modules/slpi-enable" && dtc -@ -I dts -O dtb -o cepheus-slpi.dtbo cepheus-slpi.dts && python3 embed.py)
make -C "$KERNEL" M="$EXPORT/modules/slpi-enable" ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- LOCALVERSION= -j8 modules
```

Use a writable build copy; do not build into a read-only source archive.
`Module.symvers` was available for the original external builds. Rebuilding
the kernel/modules as above produces the required kernel build metadata.

This export verifies source reconstruction and historical artifact hashes;
it **does not claim a fresh clean-room byte-for-byte kernel/module rebuild**.
Build timestamps, debug source paths, compiler versions and build numbering
can affect hashes. The retained binary reports build number 8 and timestamp
`Mon Nov 2 17:26:29 CST 2026`.

## Final static DT: display, touch, radio, camera and SSC

`dt/sm8150-cepheus-final-static.dts` is a complete decompilation of the verified
static DTB appended to the retained boot image, not a dynamically overlaid
live tree. It recompiles with all node properties and memory reservations
equal to the original DTB. Binary DTB hashes can differ due to layout/string
table choices even with identical properties. No identifier properties
required removal from this particular static DTB.

The authoritative final static tree includes:

- enabled DPU/DSI0/PHY and the restored panel connection on the correct port;
- `st,stmfts` touch on the enabled GENI I2C controller;
- `qcom,wcn3990-bt` Bluetooth on the enabled serial controller;
- WCN3990 Wi-Fi and the **enabled MPSS** remoteproc it depends on;
- CAMCC **NOM/256** for the fixed 847MHz VFE resource clock;
- dedicated 16MiB, 4MiB-aligned, below-4GiB sensor pool and FastRPC VMIDs 3/5/6.

SLPI stays disabled in the static tree and is enabled by the guarded runtime
module after prerequisites are installed. `dt/peripheral-nodes.json` records
the relevant actual paths and statuses.

No independent final display/touch/Bluetooth driver patch was found outside
the retained upstream base and DT states. Do not present upstream drivers as
new local implementations. Any phone-side touch initialization services,
Bluetooth runtime services and proprietary firmware must be checked against
the coordinating agent's separate userspace recovery; they are not inferred
or fabricated here.

Wi-Fi's final code includes WCN3990-specific pairwise-key deletion handling,
null firmware/board protection in checksums, optional passive 5GHz scans and
an optional VHT80 limit. The corresponding module parameter recipe is in
`wifi/ath10k-cepehus.conf`. Firmware, RMTFS/RFSA/QRTR/TQFTP dependencies are not
replaced by these patches, and no proprietary firmware is distributed here.

## Boot packing and artifact boundaries

`scripts/repack-current-dtb.py` handles this device's Android boot v0/page4096
format: it preserves the gzip Image and ramdisk from a supplied private boot
baseline, replaces only the appended DTB, updates `kernel_size` and boot ID
at offset 576, and only writes a new file. It never flashes hardware.

The original complete boot reference is **not included**: its Android header
contains per-device command-line identifiers and it includes a ramdisk. Its
reference SHA256 is recorded only to identify the correct historical file,
not to encourage redistribution of that dump. Never substitute an obsolete
`/boot/boot.img` for the known baseline.

The public `Image.gz` is **only a compressed Linux kernel**, not `boot.img`,
and must not be passed to `fastboot flash boot`. It contains no appended DTB
or ramdisk. `.config` has empty `CONFIG_EXTRA_FIRMWARE` and
`CONFIG_INITRAMFS_SOURCE`; no device firmware is embedded via those features.
Its decompressed Image matches the retained WSL build and the coordinating
agent's 2026-11-23 read-only extraction of the installed phone kernel.

| Artifact | Bytes | SHA256 |
| --- | ---: | --- |
| Image.gz | 12956799 | `71bd7461194398b0192dba4d7836154ece89be606dea68849746d61476b9e506` |
| decompressed Image | 32557568 | `b7b92e2053c3c782d70a5ce7f1848015c826a62b3db6ff42beae102577844421` |
| historical final CAMSS v3 module, private reference only | 1716640 | `d997de3ba47d155e1c9e340e155c2d526e96f624c4842cf2377304d3f85cc96b` |

## Verification, privacy and remaining limits

`provenance/reconstruction-verification.json` records successful archive/root,
committed tree, dirty files, final module source and static DT comparisons.
`provenance/reference-binary-fingerprints.json` distinguishes fingerprints
from a clean-room binary rebuild.

Only explicitly selected source/config/license files are exported. No SSH
credentials, private user keys, IMEI/MAC/serial values, factory calibration,
EEPROM/persist data, private photos, raw private logs, firmware or ramdisk are
included. Original source license/maintainer attribution is retained.

The mirrored **upstream** source archive remains unchanged for its fixed
checksum and complete source availability. Its two key-named files are
canonical public Linux selftest fixtures (`tools/testing/selftests/net/ovpn/data64.key`
and `tools/testing/selftests/sgx/sign_key.pem`), not this user's SSH credentials.
No `.bin`, `.fw`, `.mbn`, `.mdt` or modem segment firmware files were found in
that archive's filename audit.

Existing hardware/driver warnings, firmware requirements, later AF changes
and userspace tuning are not resolved merely by this export. This release is
experimental source recovery, not a claim of a production-ready phone port.
