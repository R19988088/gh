#!/usr/bin/env python3
"""Probe the selected real SDL endpoint with a short, quiet stereo tone."""
from pathlib import Path
import argparse
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--device', default='')
options = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='gh-audio-endpoint-') as directory:
    executable = Path(directory) / 'gh_audio_endpoint_test'
    command = ['clang++', '-std=c++23', '-Wall', '-Wextra', '-Werror', '-pthread',
               '-I', str(project / 'src'),
               '-I', str(project / 'runtime/thirdparty/sdl3/include'),
               str(project / 'src/gh_audio.cpp'),
               str(project / 'src/gh_audio_sdl.cpp'),
               str(project / 'tests/gh_audio_endpoint_test.cpp'),
               str(project / 'out/native-app/runtime/thirdparty/sdl3/libSDL3.a'),
               '-o', str(executable)]
    for framework in ['CoreMedia', 'CoreVideo', 'Cocoa', 'UniformTypeIdentifiers',
                      'IOKit', 'ForceFeedback', 'Carbon', 'CoreAudio', 'AudioToolbox',
                      'AVFoundation', 'Foundation', 'GameController', 'Metal',
                      'QuartzCore', 'CoreHaptics']:
        command.extend(['-framework', framework])
    subprocess.run(command, check=True)
    subprocess.run([str(executable), options.device] if options.device else [str(executable)],
                   check=True, timeout=6)
