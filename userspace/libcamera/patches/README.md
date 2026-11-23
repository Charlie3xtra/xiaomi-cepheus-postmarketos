# Current libcamera patch against pristine v0.7.2

Use v0.7.2-cepheus-current-20261123.patch for the recovered current CPU camera implementation. It includes all five modified upstream source files found by comparing the complete device source tree with pristine v0.7.2, plus the RAW10 helper and five tuning YAML files. Only line-ending differences were normalized. The duplicate handoff header and downloaded third-party subprojects are excluded. All 1,483 upstream regular files were compared; none were missing.

Base commit: 191e202178f02430b5942397c70d215cdd2056fa (annotated tag v0.7.2).

Base archive: https://codeload.github.com/libcamera-org/libcamera/tar.gz/refs/tags/v0.7.2

Base archive SHA-256: edc431e93c5a4030480a0784300486507662fa0bce910e203a5643861828528f

Patch SHA-256: 62659f1a6e1bd9c2837286508471e34b012aeb890f6148bc79e3b0f57fbf40fb

Validation: git apply --check and apply succeeded against the pristine archive; all 11 changed/added files then matched the device build tree after LF normalization. This is source reconstruction validation, not a new complete build or capture test.

Apply from an unpacked pristine source root: git apply /path/to/v0.7.2-cepheus-current-20261123.patch. Do not also apply the historical snapshots/patches.

Observed installed build options: --prefix=/opt/cepheus-camera, buildtype=release, gstreamer=enabled, qcam=disabled, lc-compliance=disabled, test=false, pipelines=auto; IPA list ipu3,mali-c55,rkisp1,rpi/pisp,rpi/vc4,simple,vimc. Other options were auto/default. These are the recorded build options, not a newly tested minimum-dependency recipe.

The custom YAML files live at src/ipa/simple/ in the recovered tree. The upstream Meson data install rules do not automatically install those extra files: copy the four named sensor YAML files to the custom prefix share/libcamera/ipa/simple/ at packaging time. Preserve the existing uncalibrated fallback and audit its intended install path. The scoped wrappers/services and orientation shim are separate; libcamera alone does not configure the sensor firmware, kernel modules or PipeWire.
