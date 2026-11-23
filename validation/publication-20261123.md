# Publication checks, 2026-11-23

- GitHub main eb1f41ca194d6c5741e774521189d02511b89663: all 216 published blob IDs matched the staged files; recursive tree was not truncated.
- Source manifest: all 215 listed file SHA-256 values matched after transferring the publication package to the device. (SOURCES.json excludes itself.)
- Python: 17 files parsed successfully with ast.parse.
- Historical ath10k key-path harness: 9 cases exercising the extracted C function passed on the device. This does not establish firmware/runtime behaviour of the candidate patch.
- Corrected telephoto AF C++ source: -std=c++17 -pthread -fsyntax-only passed against the custom libcamera headers. No motor operation was performed in this publication check.
- Historical APKBUILD: all five local SHA-512 entries matched.
- Relative documentation links checked; selected source/config content scanned for credentials and device network identifiers.

No fresh stream/stress, camera frame, complete kernel rebuild or kernel flash is claimed by these publication checks. Existing device/kernel hashes and earlier behavioural tests are described separately.

Additional source review: all 107 entries in the recovered kernel SOURCE-MANIFEST matched, with no selected-source credential findings; kernel config matched phone bytes. Complete libcamera patch application to pristine v0.7.2 reconstructed all 11 changed/added files. Original kernel export reconstruction/DTS verification was reviewed; no clean-room kernel rebuild is claimed.
