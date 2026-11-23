#!/usr/bin/python3
"""Publish calibrated SensorProxy posture for the local camera application."""
import os
import pathlib
import re
import subprocess
import sys

root = pathlib.Path('/run/cepheus-camera')
root.mkdir(mode=0o755, exist_ok=True)
target = root / 'orientation'
valid = {'normal', 'left-up', 'bottom-up', 'right-up'}
current = None

def publish(value):
    global current
    if value not in valid or value == current:
        return
    temp = root / ('orientation.' + str(os.getpid()))
    temp.write_text(value + '\n')
    temp.chmod(0o644)
    os.replace(temp, target)
    current = value
    print('Camera posture:', value, flush=True)

publish('normal')
env = dict(os.environ, LC_ALL='C')
with subprocess.Popen(['/usr/bin/stdbuf', '-oL', '/usr/bin/monitor-sensor', '--accel'],
                      stdout=subprocess.PIPE, stderr=sys.stderr, text=True, env=env) as proc:
    for line in proc.stdout:
        match = re.search(r'(?:orientation changed: |orientation: )([a-z-]+)', line)
        if match:
            publish(match.group(1))
    sys.exit(proc.wait() or 1)
