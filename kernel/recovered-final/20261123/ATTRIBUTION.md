# Attribution and licensing

The fixed base is the SM8150-mainline Linux fork's `v6.17.0-sm8150` source
archive, with the original Linux `COPYING`, SPDX declarations, copyright
notices and license texts retained. This is not wholly original code by the
cepheus recovery project.

- Linux and Qualcomm CAMSS/DRM/FastRPC/ath10k retain their original upstream
  authors and notices in the source archive and recovered source snapshots.
- SM8150-mainline and postmarketOS maintainers supplied the base integration
  and historical package recipes. Maintainer attribution is preserved in
  the archived APKBUILD files.
- The DRM imported DMA-BUF fix is upstream commit
  `c34e08ba6c0037a72a7433741225b020c989e4ae`, by Rob Clark. The retained stable
  patch preserves its From/Signed-off-by history in `attribution/`.
- The CAMSS work combines retained integration changes, upstream fixes and
  device bring-up work. Relevant upstream fixes include configuring CSID for
  every stream start, restricting VFE reset to the last active line, and
  balancing CSID parent VFE references on failure. Reference discussions:
  https://lists.openwall.net/linux-kernel/2026/10/05/1531
  https://www.mail-archive.com/linuxtv-commits@linuxtv.org/msg49933.html
  https://lists.openwall.net/linux-kernel/2026/10/28/772
- Xiaomi's GPL `cepheus-p-oss` kernel sources were used as register-table,
  clock/voltage and SSC integration references. This does not grant a license
  to redistribute proprietary phone firmware or factory calibration; neither
  is included here. Reference:
  https://github.com/MiCode/Xiaomi_Kernel_OpenSource/tree/cepheus-p-oss

The reconstructed patches are distributed subject to the licenses of the
files they modify. No blanket ownership claim or replacement license is
applied to third-party source. See per-file headers and `LICENSES/`.
