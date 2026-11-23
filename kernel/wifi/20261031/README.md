# Historical candidate, 2026-10-31

This pairwise-key deletion quirk and package recipe were build/control-flow tested at the time, not confirmed as the deployed final Wi-Fi/5GHz series. The full final series is being recovered from the old computer. Upstream source/copyright notices are retained.

For the actual C-function harness, run python3 test_key_path.py (requires a C compiler). For the DT utility tests, run python3 test_dtb_tool.py /private/current.dtb (requires dtc/fdtput and a matching board tree). The private device fixture is intentionally excluded; the test MAC was changed during publication to the synthetic address 02:00:00:00:00:01. No test flashes a partition.

The pmaports-overlay is an archived recipe against its original fixed source tarball; it is not the current final kernel package. Its five local sha512sums entries were rechecked during publication.
