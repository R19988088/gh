#!/usr/bin/env python3
"""Generate explicit PC-dispatch partitions and a title continuation registry."""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from gh_guest_continuation_codegen import FORBIDDEN_CONTROL, Unsupported, transform


def write_if_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def functions(source):
    lines = source.splitlines(keepends=True)
    cursor = 0
    while cursor < len(lines):
        start = cursor
        match = re.fullmatch(r'\s*DEFINE_REX_FUNC\((\w+)\)\s*\{\s*', lines[cursor])
        cursor += 1
        if not match:
            continue
        depth = 1
        while cursor < len(lines) and depth:
            depth += lines[cursor].count('{') - lines[cursor].count('}')
            cursor += 1
        if depth:
            raise Unsupported(f'{match[1]}: incomplete function')
        yield match[1], ''.join(lines[start:cursor])


def generate(source_dir, hook_dir, output_dir):
    symbols = {name: int(address, 16) for address, name in re.findall(
        r'\{\s*0x([0-9a-fA-F]+),\s*REX_GUEST_FUNCTION_POINTER\((\w+)\)',
        (source_dir / 'guardian_heroes_init.cpp').read_text())}
    hooks = set()
    for file in hook_dir.glob('*.cpp'):
        hooks.update(re.findall(r'(?:GH_HOOK|REX_HOOK_RAW|ATTRIBUTE_REBUILD)\(\s*(sub_[0-9A-Fa-f]+)\s*\)', file.read_text()))
    report = {'compiled': [], 'atomic': [], 'queued': [], 'unsupported': {}}
    registrations = []
    output_dir.mkdir(parents=True, exist_ok=True)
    sources = []
    for source in sorted(source_dir.glob('guardian_heroes_recomp.*.cpp')):
        partition = source.name.split('.')[-2]
        name = f'RegisterTitleContinuation_{partition}'
        steps, entries, declarations = [], [], []
        original = source.read_text()
        declarations += [line for line in original.splitlines() if re.fullmatch(r'(?:extern )?void Gh\w+\([^;{}]*\);', line)]
        for function, body in functions(original):
            address = symbols.get(function)
            if address is None:
                match = re.fullmatch(r'sub_([0-9A-Fa-f]+)', function)
                if not match:
                    raise Unsupported(f'{function}: missing title address')
                address = int(match[1], 16)
                symbols[function] = address
            if function in hooks:
                report['atomic'].append(function)
                declarations.append(f'DECLARE_REX_FUNC({function});')
                entries.append(f'  program.AddAtomicHost(0x{address:X}, &::{function});')
                continue
            try:
                result = transform(body, symbols=symbols)
            except Unsupported as error:
                reason = str(error)
                report['unsupported'][function] = reason
                entries.append(f'  program.AddUnsupported(0x{address:X}, {json.dumps(reason)});')
            else:
                compiled, registration = result.split('namespace {\n', 1)[1].rsplit('}\nvoid ', 1)
                steps.append(compiled)
                entries += registration.split('{\n', 1)[1].rsplit('}\n}', 1)[0].splitlines()
                report['compiled'].append(function)
        output = [f'#include "guardian_heroes_funcs.{partition}.h"', '#include "gh_guest_continuation.h"',
                  *declarations,
                  'namespace gh::guest::continuation {', 'namespace {', *steps,
                  '}', f'void {name}(Program& program) {{', *entries, '}', '}']
        path = output_dir / f'gh_title_continuation.{partition}.cpp'
        write_if_changed(path, '\n'.join(output)+'\n')
        sources.append(path.name)
        registrations.append(name)
    # Blocking imports keep a queued operation at safepoints. Other imports
    # complete atomically. The project kernel's existing
    # returning SEH stubs keep their original behavior; actual native longjmp
    # or context transfers may not cross this machine's frame.
    imports = []
    queued = {
        '__imp__KeDelayExecutionThread': 'BeginKeDelay',
        '__imp__KeWaitForSingleObject': 'BeginKeWaitSingle',
        '__imp__NtWaitForSingleObjectEx': 'BeginNtWaitSingle',
        '__imp__KeWaitForMultipleObjects': 'BeginKeWaitMultiple',
        '__imp__NtWaitForMultipleObjectsEx': 'BeginNtWaitMultiple',
        '__imp__NtSignalAndWaitForSingleObjectEx': 'BeginSignalAndWait',
    }
    for function, address in sorted(symbols.items()):
        if not function.startswith('__imp__'):
            continue
        if function in queued:
            report['queued'].append(function)
            imports.append(f'  program.AddQueuedHost(0x{address:X}, &{queued[function]});')
        elif FORBIDDEN_CONTROL.search(function):
            imports.append(f'  program.AddUnsupported(0x{address:X}, "unsupported guest non-local control: {function}");')
        else:
            report['atomic'].append(function)
            imports.append(f'  program.AddAtomicHost(0x{address:X}, &::{function});')
    registry = ['#include "guardian_heroes_pch.h"', '#include "gh_guest_continuation_title.h"',
                '#include "gh_guest_continuation_waits.h"']
    registry += [f'DECLARE_REX_FUNC({name});' for name in sorted(symbols) if name.startswith('__imp__')]
    registry += ['namespace gh::guest::continuation {']
    registry += [f'void {name}(Program&);' for name in registrations]
    registry += ['Program CreateTitleContinuationProgram() {', '  Program program;']
    registry += [f'  {name}(program);' for name in registrations]
    registry += imports + ['  return program;', '}', '}']
    write_if_changed(output_dir / 'gh_title_continuation_registry.cpp', '\n'.join(registry)+'\n')
    sources.append('gh_title_continuation_registry.cpp')
    write_if_changed(output_dir / 'gh_title_continuation_sources.cmake',
        'set(GUARDIAN_HEROES_TITLE_CONTINUATION_SOURCES\n'+
        ''.join(f'  "${{CMAKE_CURRENT_LIST_DIR}}/{source}"\n' for source in sources)+')\n')
    write_if_changed(output_dir / 'gh_title_continuation_report.json', json.dumps(report, indent=2)+'\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_dir', type=Path)
    parser.add_argument('hook_dir', type=Path)
    parser.add_argument('output_dir', type=Path)
    args = parser.parse_args()
    report = generate(args.source_dir, args.hook_dir, args.output_dir)
    print(f"{len(report['compiled'])} compiled, {len(report['atomic'])} atomic hooks/imports, "
          f"{len(report['queued'])} queued imports, "
          f"{len(report['unsupported'])} unsupported generated functions")
