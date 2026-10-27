#!/usr/bin/env python3
"""Focused cheat/menu check using the original pause builder and action predicates."""
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
build = project / 'out/build/mac-arm64-release'
cache = (build / 'CMakeCache.txt').read_text()
ninja = next(line.split('=', 1)[1] for line in cache.splitlines() if line.startswith('CMAKE_MAKE_PROGRAM:'))
command = shlex.split(subprocess.check_output([
    ninja, '-C', str(build), '-t', 'commands',
    'CMakeFiles/guardian_heroes.dir/src/gpu_wait.cpp.o'], text=True).splitlines()[-1])
includes = []
for index, argument in enumerate(command):
    if argument.startswith('-I'):
        includes.append(argument)
    elif argument == '-isystem':
        includes.extend([argument, command[index + 1]])
names = {'824C1A68', '82493E48', '824A50F0', '824A51C0', '824A5880', '824A8488', '824A58C8', '8243EEA0', '82440228', '8243EEC0', '8243FDC8', '824411C0', '824BFF00', '824BFF78', '824BFFE8', '824C0058', '824C00C8', '824C0158'}
names = {'sub_' + name for name in names} | {'__savegprlr_25', '__restgprlr_25', '__savegprlr_28', '__restgprlr_28'}
references = []
for file in sorted((project / 'generated/default').glob('guardian_heroes_recomp.*.cpp')):
    source = file.read_text()
    for match in re.finditer(r'DEFINE_REX_FUNC\((sub_[0-9A-F]+|__(?:save|rest)gprlr_\d+)\)', source):
        if match[1] not in names:
            continue
        end = source.find('\nDEFINE_REX_FUNC(', match.end())
        function = source[match.start():end if end != -1 else None]
        # Retain the strong cheat hooks and compile the real guest entry separately.
        name = match[1]
        hooked = name.startswith('sub_') and name not in {'sub_82493E48', 'sub_824A50F0', 'sub_824A51C0', 'sub_824A5880', 'sub_824A8488', 'sub_824A58C8', 'sub_8243EEA0', 'sub_82440228', 'sub_8243EEC0', 'sub_8243FDC8'}
        function = function.replace('DEFINE_REX_FUNC(' + name + ')',
            'extern "C" REX_FUNC(' + ('__imp__' if hooked else '') + name + ')', 1)
        references.append(function)
assert len(references) == len(names)
references = ''.join(references)
declarations = ''.join('REX_EXTERN(' + name + ');\n' for name in sorted(set(
    re.findall(r'\b((?:sub_[0-9A-F]+|__(?:save|rest)gprlr_\d+))\(ctx, base\)', references))))
with tempfile.TemporaryDirectory(prefix='guardian-cheats-') as directory:
    temporary = Path(directory)
    reference = temporary / 'reference.cpp'
    reference.write_text('#include "generated/default/guardian_heroes_pch.h"\n' + declarations + references)
    executable = temporary / 'cheats_test'
    subprocess.run([command[0], '-std=c++23', '-O1', '-DSPDLOG_COMPILED_LIB',
        '-DSPDLOG_FMT_EXTERNAL', *includes, str(reference), str(project / 'tests/cheats_test.cpp'),
        '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
