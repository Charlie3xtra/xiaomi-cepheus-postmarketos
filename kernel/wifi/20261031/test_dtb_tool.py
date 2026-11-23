import copy
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import prepare_wifi_dtb as tool

ROOT = Path(__file__).parent
INPUT = Path(sys.argv.pop(1)) if len(sys.argv) > 1 and not sys.argv[1].startswith('-') else ROOT / 'fixtures/phone-current.dtb'


class DtbTests(unittest.TestCase):
    def setUp(self):
        self.original = INPUT.read_bytes()
        self.nodes, self.reservations, self.cpu = tool.parse_fdt(self.original)

    def invoke(self, *args):
        return subprocess.run([sys.executable, str(ROOT / 'prepare_wifi_dtb.py'), str(INPUT), *map(str, args)], capture_output=True, text=True)

    def test_single_property_change_and_input_preservation(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'candidate.dtb'
            result = self.invoke('--output', output, '--mac', '02:00:00:00:00:01')
            self.assertEqual(result.returncode, 0, result.stderr)
            after, reservations, cpu = tool.parse_fdt(output.read_bytes())
            expected = copy.deepcopy(self.nodes)
            expected[tool.WIFI]['local-mac-address'] = bytes.fromhex('020000000001')
            self.assertEqual(after, expected)
            self.assertEqual((reservations, cpu), (self.reservations, self.cpu))
            self.assertEqual(INPUT.read_bytes(), self.original)

    def test_multicast_mac_rejected_without_output(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'invalid.dtb'
            self.assertNotEqual(self.invoke('--output', output, '--mac', '01:02:03:04:05:06').returncode, 0)
            self.assertFalse(output.exists())

    def test_existing_output_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'existing.dtb'
            output.write_bytes(b'keep this file')
            self.assertNotEqual(self.invoke('--output', output, '--mac', '02:00:00:00:00:01').returncode, 0)
            self.assertEqual(output.read_bytes(), b'keep this file')

    def test_input_cannot_be_overwritten(self):
        before = hashlib.sha256(INPUT.read_bytes()).digest()
        self.assertNotEqual(self.invoke('--output', INPUT, '--mac', '02:00:00:00:00:01').returncode, 0)
        self.assertEqual(hashlib.sha256(INPUT.read_bytes()).digest(), before)

    def test_broken_supply_reference_rejected(self):
        nodes = copy.deepcopy(self.nodes)
        nodes[tool.WIFI]['vdd-0.8-cx-mx-supply'] = bytes.fromhex('ffffffff')
        with self.assertRaises(ValueError):
            tool.audit(nodes)

    def test_duplicate_phandle_rejected(self):
        nodes = copy.deepcopy(self.nodes)
        nodes['/']['phandle'] = nodes[tool.WIFI]['vdd-0.8-cx-mx-supply']
        with self.assertRaises(ValueError):
            tool.audit(nodes)


if __name__ == '__main__':
    unittest.main()
