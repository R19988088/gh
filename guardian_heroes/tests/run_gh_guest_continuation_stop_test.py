#!/usr/bin/env python3
"""Check cooperative frame stops without game, device, or generated title code."""
from pathlib import Path
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='gh-continuation-stop-') as temporary:
    binary = Path(temporary) / 'stop-test'
    subprocess.run(['clang++', '-std=c++23', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-pthread', '-I'+str(project / 'src'),
                    str(project / 'src/gh_guest_continuation.cpp'),
                    str(project / 'src/gh_guest_continuation_title.cpp'),
                    str(project / 'tests/gh_guest_continuation_stop_test.cpp'),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=15)
