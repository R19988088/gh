#!/usr/bin/env python3
"""Exercise resumable queued imports across guest memory/kernel restore."""
from pathlib import Path
import shutil
import subprocess
import sys

project = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1]) if len(sys.argv) > 1 else project / 'out/native-app'
cmake = shutil.which('cmake') or '/opt/homebrew/bin/cmake'
subprocess.run([cmake, '--build', str(build), '--target',
                'guardian_heroes_continuation_waits_test', '-j', '6'], check=True)
binary = build / 'guardian_heroes_continuation_waits_test'
if sys.platform == 'win32':
    binary = binary.with_suffix('.exe')
subprocess.run([str(binary)], check=True, timeout=20)
