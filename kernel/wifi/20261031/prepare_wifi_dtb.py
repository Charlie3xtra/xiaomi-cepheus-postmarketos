#!/usr/bin/env python3
"""Audit a cepheus DTB; optionally add a per-device MAC without touching other nodes.

Requires fdtput (device-tree-compiler). Never flashes or edits the input DTB.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

WIFI = '/soc@0/wifi@18800000'
SUPPLIES = {
    'vdd-0.8-cx-mx-supply': 'pm8150_l1a_0p75',
    'vdd-1.8-xo-supply': 'pm8150_l7a_1p8',
    'vdd-1.3-rfa-supply': 'pm8150l_l2c_1p3',
    'vdd-3.3-ch0-supply': 'pm8150l_l11c_3p3',
    'vdd-3.3-ch1-supply': 'pm8150l_l11c_3p3',
}


def parse_fdt(data):
    if len(data) < 40:
        raise ValueError('Truncated FDT header')
    h = struct.unpack_from('>10I', data)
    magic, total, off_struct, off_strings, off_reserve, version, compatible, boot_cpu, size_strings, size_struct = h
    if magic != 0xd00dfeed or version < 17 or total > len(data):
        raise ValueError('Unsupported or truncated FDT')
    if off_struct + size_struct > total or off_strings + size_strings > total:
        raise ValueError('FDT sections extend outside the blob')
    strings = data[off_strings:off_strings + size_strings]
    block = data[off_struct:off_struct + size_struct]
    nodes, stack, pos, ended = {}, [], 0, False
    while pos < len(block):
        token = struct.unpack_from('>I', block, pos)[0]
        pos += 4
        if token == 1:
            end = block.index(b'\0', pos)
            name = block[pos:end].decode('ascii')
            stack.append(name)
            path = '/' + '/'.join(stack[1:])
            if path in nodes:
                raise ValueError('Duplicate node: ' + path)
            nodes[path] = {}
            pos = (end + 4) & ~3
        elif token == 2:
            if not stack:
                raise ValueError('Unbalanced node end')
            stack.pop()
        elif token == 3:
            length, offset = struct.unpack_from('>II', block, pos)
            pos += 8
            if not stack or offset >= len(strings) or pos + length > len(block):
                raise ValueError('Invalid property bounds')
            end = strings.index(b'\0', offset)
            name = strings[offset:end].decode('ascii')
            props = nodes['/' + '/'.join(stack[1:])]
            if name in props:
                raise ValueError('Duplicate property: ' + name)
            props[name] = block[pos:pos + length]
            pos = (pos + length + 3) & ~3
        elif token == 4:
            continue
        elif token == 9:
            if stack:
                raise ValueError('FDT ended with unclosed nodes')
            ended = True
            break
        else:
            raise ValueError('Unknown FDT token')
    if not ended:
        raise ValueError('FDT end token missing')
    pos = off_reserve
    while True:
        if pos + 16 > total:
            raise ValueError('Truncated reservation map')
        entry = struct.unpack_from('>QQ', data, pos)
        pos += 16
        if entry == (0, 0):
            break
    return nodes, data[off_reserve:pos], boot_cpu


def text(value):
    return value.rstrip(b'\0').decode('ascii')


def audit(nodes):
    if b'xiaomi,cepheus' not in nodes.get('/', {}).get('compatible', b'').split(b'\0'):
        raise ValueError('Input is not a Xiaomi cepheus DTB')
    wifi = nodes.get(WIFI, {})
    if wifi.get('compatible') != b'qcom,wcn3990-wifi\0' or wifi.get('status') != b'okay\0':
        raise ValueError('Expected enabled WCN3990 node is missing')
    handles = {}
    for path, props in nodes.items():
        for key in ('phandle', 'linux,phandle'):
            if key in props:
                if len(props[key]) != 4:
                    raise ValueError('Invalid phandle size')
                number = struct.unpack('>I', props[key])[0]
                if number in handles and handles[number] != path:
                    raise ValueError(f'Duplicate phandle {number:#x}')
                handles[number] = path
    supplies = {}
    for name, expected in SUPPLIES.items():
        value = wifi.get(name, b'')
        if len(value) != 4:
            raise ValueError('Missing or malformed supply: ' + name)
        path = handles.get(struct.unpack('>I', value)[0])
        if not path or text(nodes[path].get('regulator-name', b'')) != expected:
            raise ValueError('Unexpected regulator for ' + name + '; inspect wiring before changing it')
        supplies[name] = {'path': path, 'name': expected}
    return {'supplies': supplies, 'wifi_firmware_subnode': WIFI + '/wifi-firmware' in nodes,
            'host_cap_8bit_quirk': 'qcom,snoc-host-cap-8bit-quirk' in wifi}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--mac', help='Per-device address; no shared default is supplied')
    parser.add_argument('--manifest', type=Path)
    args = parser.parse_args()
    original = args.input.read_bytes()
    before, reservations, boot_cpu = parse_fdt(original)
    result = audit(before)
    result['input_sha256'] = hashlib.sha256(original).hexdigest()
    if bool(args.mac) != bool(args.output):
        parser.error('--mac and --output must be provided together')
    if args.output:
        if args.output.resolve() == args.input.resolve() or args.output.exists():
            parser.error('Output must be a new file distinct from input')
        if not re.fullmatch(r'(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}', args.mac):
            parser.error('Invalid MAC syntax')
        mac = bytes.fromhex(args.mac.replace(':', ''))
        if mac[0] & 1 or mac == bytes(6):
            parser.error('A nonzero unicast MAC is required')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=args.output.parent) as directory:
            candidate = Path(directory) / 'candidate.dtb'
            candidate.write_bytes(original)
            subprocess.run(['fdtput', '-t', 'bx', str(candidate), WIFI,
                            'local-mac-address', *[f'{x:02x}' for x in mac]], check=True)
            modified = candidate.read_bytes()
            after, reservations_after, boot_cpu_after = parse_fdt(modified)
            audit(after)
            expected = {path: dict(props) for path, props in before.items()}
            expected[WIFI]['local-mac-address'] = mac
            if after != expected or reservations_after != reservations or boot_cpu_after != boot_cpu:
                raise ValueError('Unexpected DT change; candidate rejected')
            # Exclusive creation prevents overwriting an existing artifact.
            with args.output.open('xb') as stream:
                stream.write(modified)
        result.update(output_sha256=hashlib.sha256(modified).hexdigest(),
                      changed_property=WIFI + '/local-mac-address', mac=args.mac.lower())
    encoded = json.dumps(result, ensure_ascii=False, indent=2)
    if args.manifest:
        args.manifest.write_text(encoded + '\n', encoding='utf-8')
    print(encoded)


if __name__ == '__main__':
    main()
