#!/usr/bin/env python3
"""Compare the native hook against the actual generated guest implementation."""

import argparse
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    suites = [source.stem for source in sorted((project / 'tests').glob('native_*_test.cpp'))]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--suite', action='append', choices=suites,
                        help='Run only the named suite; repeat to select several')
    args = parser.parse_args()
    build = args.build_dir or project / 'out/build/mac-arm64-release'
    # Reuse the configured game's SDK and dependency include paths.
    cache = (build / 'CMakeCache.txt').read_text()
    ninja = next(line.split('=', 1)[1] for line in cache.splitlines()
                 if line.startswith('CMAKE_MAKE_PROGRAM:'))
    command = subprocess.check_output([
        ninja, '-C', str(build), '-t', 'commands',
        'CMakeFiles/guardian_heroes.dir/src/gpu_wait.cpp.o'], text=True).splitlines()[-1]
    arguments = shlex.split(command)
    includes = []
    for i, argument in enumerate(arguments):
        if argument.startswith('-I'):
            includes.append(argument)
        elif argument == '-isystem':
            includes.extend([argument, arguments[i + 1]])

    sources = sorted((project / 'src').glob('native_*.cpp'))
    names = [name for source in sources for name in
             re.findall(r'GH_HOOK\((sub_[0-9A-F]+)\)', source.read_text())]
    references = {}
    for generated_file in sorted((project / 'generated/default').glob('guardian_heroes_recomp.*.cpp')):
        generated = generated_file.read_text()
        for name in names:
            start = generated.find(f'DEFINE_REX_FUNC({name})')
            if start == -1:
                continue
            end = generated.find('\nDEFINE_REX_FUNC(', start + 1)
            references[name] = generated[start:end if end != -1 else None].replace(
                f'DEFINE_REX_FUNC({name})',
                f'extern "C" __attribute__((noinline)) REX_FUNC(__imp__{name})', 1)
    reference = ''.join(references[name] for name in names)
    with tempfile.TemporaryDirectory(prefix='guardian-native-logic-') as directory:
        temporary = Path(directory)
        reference_file = temporary / 'reference.cpp'
        reference_file.write_text(
            '#include "generated/default/guardian_heroes_pch.h"\n' + reference)
        # Generated guest functions use strict floating-point evaluation in Release.
        compiler = [arguments[0], '-std=c++23', '-O3', '-ffp-model=strict',
                    '-fno-strict-aliasing', '-DSPDLOG_COMPILED_LIB',
                    '-DSPDLOG_FMT_EXTERNAL', *includes]
        objects = []
        subprocess.run([*compiler, '-c', str(reference_file), '-o', str(temporary / 'reference.o')], check=True)
        objects.append(str(temporary / 'reference.o'))
        native_compiler = [arguments[0], '-std=c++23', '-O3', '-ffp-model=strict',
                           '-fno-strict-aliasing', '-I', str(project / 'src')]
        for source in sources:
            target = temporary / (source.stem + '.o')
            subprocess.run([*native_compiler, '-c', str(source), '-o', str(target)], check=True)
            objects.append(str(target))
        for suite in args.suite or suites:
            executable = temporary / suite
            subprocess.run([*compiler, *objects, str(project / f'tests/{suite}.cpp'),
                            '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    main()
