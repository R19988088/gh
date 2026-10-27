#!/usr/bin/env python3
"""Turn the supported full-context generated C++ subset into guest-PC steps.

This is an offline execution foundation, not an alternate title build. Unknown
control flow, persistent C++ locals, setjmp/SEH and imports without a guest
address stop generation. The original generated source is never changed.
"""
import argparse
from pathlib import Path
import re


# The current project kernel's RtlUnwind/C_specific_handler are returning
# diagnostics and RtlRaiseException names threads or breaks into a debugger.
# They perform no guest stack transfer and remain ordinary atomic imports;
# this preserves their current behavior without implementing guest SEH.
FORBIDDEN_CONTROL = re.compile(r'setjmp|longjmp|NtContinue|RtlRestoreContext', re.I)


class Unsupported(ValueError):
    pass



PROJECT_OPERATIONS = {
    'GhYieldGpuFence', 'GhAppendLanguageOption', 'GhHandleLanguageOption',
    'GhRefreshTitleOptions', 'GhRefreshPauseOptions', 'GhKeepHealthMax',
    'GhKeepSharedHealthMax',
}


def validate_operations(name, code, atomic=False):
    """Validate one instruction (or one closed guest critical region).

    C++ SIMD locals and byte loops finish within this step; none survive the
    guest instruction boundary. Guest calls/branches use explicit transitions.
    """
    local_names = set()
    depth = 0
    normalized = []
    pending = ''
    for line in code:
        pending = pending + ' ' + line if pending else line
        if pending.count('(') > pending.count(')'):
            continue
        normalized.append(pending)
        pending = ''
    if pending: raise Unsupported(f'{name}: incomplete generated expression')
    for original in normalized:
        line = original
        if line in {'{', '}'}:
            depth += 1 if line == '{' else -1
            if depth < 0: raise Unsupported(f'{name}: unmatched instruction scope')
            continue
        if atomic and (re.fullmatch(r'atomic_loc_[0-9A-F]+:', line) or
                       re.fullmatch(r'(?:if \(.*\) )?goto atomic_loc_[0-9A-F]+;', line)):
            continue
        if re.fullmatch(r'for \(size_t i = 0; i < (?:\(16 - \(ea & 0xF\)\)|\(ea & 0xF\)); i\+\+\)', line):
            continue
        if line.startswith('if ('):
            nesting = 0
            for index, character in enumerate(line[3:], 3):
                nesting += character == '('
                nesting -= character == ')'
                if nesting == 0:
                    line = line[index+1:].strip()
                    break
        declaration = re.match(r'(simde__m128i|simde__m128) (\w+) = ', line)
        if declaration:
            if depth <= 0: raise Unsupported(f'{name}: unscoped instruction local')
            local_names.add(declaration[2])
        lhs = r'(?:ctx\.\w+(?:\.\w+)*(?:\[[^;\]]+\])?|ea|temp\.\w+|vTemp\.\w+(?:\[[^;\]]+\])?)'
        if local_names: lhs += '|(?:'+ '|'.join(local_names)+')'
        assignment = re.match(r'(?:'+lhs+r')\s*(?:=|\+=|-=|\|=|&=|\^=)', line)
        compare = re.fullmatch(r'ctx\.\w+\.(?:compare|setFromMask)(?:<[^>]+>)?\(.*\);', line)
        decrement = line == '--ctx.ctr.u64;'
        fp = re.fullmatch(r'ctx\.fpscr\.(?:(?:enableFlushMode|disableFlushMode)(?:Unconditional)?|storeFromGuest|restoreGuestBits|setcsr)\(.*\);', line)
        operation = re.fullmatch(r'(?:REX_(?:MM_)?STORE_U(?:8|16|32|64)|simde_\w+|memset|std::atomic_thread_fence|ppc_trap|'+ '|'.join(PROJECT_OPERATIONS)+r')\(.*\);', line)
        lock = atomic and line in {'REX_ENTER_GLOBAL_LOCK();', 'REX_LEAVE_GLOBAL_LOCK();'}
        calls = re.findall(r'(?<![.\w])([A-Za-z_]\w*(?:::\w+)*)\s*(?:<[^;()]*>)?\s*\(', line)
        allowed = {'static_cast', 'reinterpret_cast', 'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t',
                   'int8_t', 'int16_t', 'int32_t', 'int64_t', 'int', 'char', 'short', 'float', 'double',
                   'memset', 'sqrt', 'log2f', 'exp2f', 'std::fma', 'std::isnan', 'std::atomic_thread_fence',
                   'ppc_trap', 'REX_QUERY_TIMEBASE', 'REX_CHECK_GLOBAL_LOCK', '__sync_bool_compare_and_swap'} | PROJECT_OPERATIONS
        if atomic: allowed |= {'REX_ENTER_GLOBAL_LOCK', 'REX_LEAVE_GLOBAL_LOCK'}
        def pure(call):
            return call in allowed or call.startswith(('REX_LOAD_', 'REX_STORE_', 'REX_MM_LOAD_',
                'REX_MM_STORE_', 'REX_RAW_ADDR', 'simde_', 'rex::ppc::simde_', '__builtin_'))
        if (not (assignment or compare or decrement or fp or operation or lock or declaration)
                or any(not pure(call) for call in calls)
                or any(token in line for token in ['{', '}', 'throw', 'setjmp', 'longjmp'])):
            raise Unsupported(f'{name}: unsupported generated operation: {original}')
    if depth: raise Unsupported(f'{name}: instruction local crosses a guest boundary')


def merge_critical_regions(name, groups):
    """Compile each closed lock-held control-flow graph as one atomic step."""
    indices = {pc: index for index, (pc, _) in enumerate(groups)}
    consumed, result = set(), []
    for start, (pc, code) in enumerate(groups):
        if start in consumed: continue
        if 'REX_ENTER_GLOBAL_LOCK();' not in code:
            if 'REX_LEAVE_GLOBAL_LOCK();' in code:
                raise Unsupported(f'{name}: guest critical region has no local entry')
            result.append((pc, code, False))
            continue
        pending, depths, region = [(start, 0)], {}, {}
        while pending:
            index, depth = pending.pop()
            if index >= len(groups):
                raise Unsupported(f'{name}: guest critical region leaves its function')
            if index in depths:
                if depths[index] != depth:
                    raise Unsupported(f'{name}: guest critical region branch changes native lock ownership')
                continue
            depths[index] = depth
            address, block = groups[index]
            transfers = []
            for line in block:
                if line == 'REX_ENTER_GLOBAL_LOCK();': depth += 1
                if line == 'REX_LEAVE_GLOBAL_LOCK();': depth -= 1
                if depth < 0:
                    raise Unsupported(f'{name}: guest critical region releases an unowned native lock')
                jump = re.fullmatch(r'(?:if \(.*\) )?goto loc_([0-9A-Fa-f]+);', line)
                if jump:
                    if int(jump[1], 16) not in indices:
                        raise Unsupported(f'{name}: guest critical region leaves its function')
                    transfers.append((indices[int(jump[1], 16)], line.startswith('if ')))
                elif re.search(r'\breturn\b|\w+\(ctx, base\);|REX_CALL_INDIRECT_FUNC', line):
                    raise Unsupported(f'{name}: guest critical region calls or returns across a native lock')
            if len(transfers) > 1:
                raise Unsupported(f'{name}: unsupported critical region control')
            operations = [line for line in block if not re.fullmatch(r'(?:if \(.*\) )?goto loc_[0-9A-Fa-f]+;', line)]
            validate_operations(name, operations, atomic=True)
            if depth == 0:
                if transfers:
                    raise Unsupported(f'{name}: branch shares a guest unlock instruction')
                region[index] = (block, None)
                continue
            successors = [transfers[0][0]] if transfers else []
            if not transfers or transfers[0][1]: successors.append(index+1)
            for successor in successors: pending.append((successor, depth))
            region[index] = (block, successors)
        interior = set(region) - {start}
        for outside, (_, block) in enumerate(groups):
            if outside in region: continue
            for line in block:
                if any(indices.get(int(target, 16)) in interior for target in re.findall(r'goto loc_([0-9A-Fa-f]+)', line)):
                    raise Unsupported(f'{name}: external branch enters an atomic guest critical region')
        combined = [f'goto atomic_loc_{pc:X};']
        for index, (block, successors) in sorted(region.items()):
            address = groups[index][0]
            combined.append(f'atomic_loc_{address:X}:')
            combined += [re.sub(r'loc_([0-9A-Fa-f]+)', lambda m: f'atomic_loc_{int(m[1],16):X}', line) for line in block]
            if successors is None:
                next_pc = groups[index+1][0] if index+1 < len(groups) else address+4
                combined.append(f'return Transition::Next(0x{next_pc:X});')
            elif not block or not re.fullmatch(r'goto loc_[0-9A-Fa-f]+;', block[-1]):
                combined.append(f'goto atomic_loc_{groups[index+1][0]:X};')
        result.append((pc, combined, True))
        consumed.update(region)
    return result


def transform(source, preamble=None, symbols=None, selected=None, registration="RegisterGeneratedContinuation"):
    symbols = dict(symbols or {})
    functions = []
    lines = source.splitlines()
    cursor = 0
    while cursor < len(lines):
        match = re.fullmatch(r'\s*DEFINE_REX_FUNC\((\w+)\)\s*\{\s*', lines[cursor])
        cursor += 1
        if not match:
            continue
        name = match[1]
        body = []
        depth = 1
        while cursor < len(lines) and depth:
            line = lines[cursor]
            cursor += 1
            depth += line.count('{') - line.count('}')
            if depth:
                body.append(line.strip())
        if depth:
            raise Unsupported(f'{name}: incomplete function')
        if selected and name not in selected:
            continue
        if name not in symbols:
            address = re.fullmatch(r'sub_([0-9A-Fa-f]+)', name)
            if not address:
                raise Unsupported(f'{name}: missing guest address')
            symbols[name] = int(address[1], 16)
        functions.append((name, symbols[name], body))
    if not functions or (selected and selected - {f[0] for f in functions}):
        raise Unsupported('requested generated function was not found')
    includes = ([f'#include "{preamble}"'] if preamble else
                [line for line in lines if line.startswith('#include')])
    includes += [line for line in lines if re.fullmatch(r'(?:extern )?void Gh\w+\([^;{}]*\);', line)]
    output = includes + ['#include "gh_guest_continuation.h"',
                        'namespace gh::guest::continuation {', 'namespace {']
    registrations = []
    for name, entry, body in functions:
        groups, code = [], []
        pc = entry
        next_label = None
        started = False
        for line in body:
            if not line or line == 'REX_FUNC_PROLOGUE();':
                continue
            if line in {'uint32_t ea{};', 'PPCRegister temp{};', 'PPCVRegister vTemp{};'} and not started:
                continue
            if line.startswith('// '):
                if not re.fullmatch(r'// [a-z][a-z0-9_.+-]*(?: .*?)?', line):
                    raise Unsupported(f'{name}: unsupported instruction comment {line}')
                if started:
                    groups.append((pc, code))
                    pc = next_label if next_label is not None else pc + 4
                next_label = None
                started, code = True, []
                continue
            label = re.fullmatch(r'loc_([0-9A-Fa-f]+):', line)
            if label:
                if next_label is not None and next_label != int(label[1], 16):
                    raise Unsupported(f'{name}: aliased internal labels require a PC map')
                next_label = int(label[1], 16)
                if not started:
                    pc = next_label
                    next_label = None
                continue
            if not started:
                raise Unsupported(f'{name}: persistent local or pre-instruction code: {line}')
            code.append(line)
        if started:
            groups.append((pc, code))
        if not groups:
            raise Unsupported(f'{name}: no PPC instructions')
        end = pc + 4
        groups = merge_critical_regions(name, groups)
        valid = {p for p, _, _ in groups} | {end, entry}
        output += [f'Transition Step_{name}(GuestContext& ctx, [[maybe_unused]] uint8_t* base, Frame& frame) {{',
                   '  [[maybe_unused]] auto& ea = frame.ea;',
                   '  [[maybe_unused]] auto& temp = frame.temp;',
                   '  [[maybe_unused]] auto& vTemp = frame.vTemp;', '  switch (frame.pc) {']
        if groups[0][0] != entry:
            output.append(f'    case 0x{entry:X}: return Transition::Next(0x{groups[0][0]:X});')
        for group_index, (pc, code, atomic) in enumerate(groups):
            resume_pc = groups[group_index+1][0] if group_index+1 < len(groups) else end
            output.append(f'    case 0x{pc:X}: {{')
            terminal = False
            if atomic:
                output += ['      '+line for line in code]
                output += [f'      return Transition::Next(0x{resume_pc:X});', '    }']
                continue
            # The emitter's jump tables contain only cases, guest-label gotos,
            # and the original out-of-range trap.
            if code and re.fullmatch(r'switch \(ctx\.\w+\.u32\) \{', code[0]):
                if code[-1] != '}':
                    raise Unsupported(f'{name}: incomplete jump table')
                skip_return = False
                for index, line in enumerate(code):
                    if skip_return and line == 'return;':
                        skip_return = False
                        continue
                    jump = re.fullmatch(r'goto loc_([0-9A-Fa-f]+);', line)
                    tail = re.fullmatch(r'(\w+)\(ctx, base\);', line)
                    if jump:
                        target = int(jump[1], 16)
                        if target not in valid:
                            raise Unsupported(f'{name}: branch target outside function')
                        line = f'return Transition::Next(0x{target:X});'
                    elif tail and code[index+1:index+2] == ['return;']:
                        called = tail[1]
                        address = re.fullmatch(r'sub_([0-9A-Fa-f]+)', called)
                        target = symbols.get(called, int(address[1], 16) if address else None)
                        if target is None or FORBIDDEN_CONTROL.search(called):
                            raise Unsupported(f'{name}: unmapped native hook/import {called}')
                        line = f'return Transition::TailCall(0x{target:X});'
                        skip_return = True
                    elif not (line in {code[0], '}', 'default:', '__builtin_trap(); // Switch case out of range'}
                              or re.fullmatch(r'case \d+:', line)):
                        raise Unsupported(f'{name}: unsupported jump table operation {line}')
                    output.append('      '+line)
                output += ['    }']
                continue
            condition = re.fullmatch(r'if \((.*)\) \{', code[0]) if code else None
            if condition and len(code) == 4 and code[2:] == ['return;', '}']:
                call = re.fullmatch(r'(\w+)\(ctx, base\);', code[1])
                indirect = re.fullmatch(r'REX_CALL_INDIRECT_FUNC\((.*)\);', code[1])
                if call:
                    called = call[1]
                    target = symbols.get(called)
                    address = re.fullmatch(r'sub_([0-9A-Fa-f]+)', called)
                    if target is None and address: target = int(address[1], 16)
                    if target is None or FORBIDDEN_CONTROL.search(called):
                        raise Unsupported(f'{name}: unmapped native hook/import {called}')
                    target = f'0x{target:X}'
                elif indirect:
                    target = f'uint32_t({indirect[1]})'
                else:
                    raise Unsupported(f'{name}: unsupported conditional tail call')
                output += [f'      if ({condition[1]}) return Transition::TailCall({target});',
                           f'      return Transition::Next(0x{resume_pc:X});', '    }']
                continue
            for index, line in enumerate(code):
                if terminal:
                    raise Unsupported(f'{name}: code after control transfer')
                branch = re.fullmatch(r'(?:if \((.*)\) )?goto loc_([0-9A-Fa-f]+);', line)
                ret = re.fullmatch(r'(?:if \((.*)\) )?return;', line)
                call = re.fullmatch(r'(\w+)\(ctx, base\);', line)
                indirect = re.fullmatch(r'REX_CALL_INDIRECT_FUNC\((.*)\);', line)
                if branch:
                    target = int(branch[2], 16)
                    if target not in valid:
                        raise Unsupported(f'{name}: branch target outside function')
                    expression = f'Transition::Next(0x{target:X})'
                    if branch[1]: expression = f'({branch[1]}) ? {expression} : Transition::Next(0x{resume_pc:X})'
                    output.append(f'      return {expression};')
                    terminal = True
                elif ret:
                    expression = 'Transition::Return()'
                    if ret[1]: expression = f'({ret[1]}) ? {expression} : Transition::Next(0x{resume_pc:X})'
                    output.append(f'      return {expression};')
                    terminal = True
                elif call or indirect:
                    if call:
                        called = call[1]
                        address = re.fullmatch(r'sub_([0-9A-Fa-f]+)', called)
                        target = symbols.get(called, int(address[1], 16) if address else None)
                        if target is None or FORBIDDEN_CONTROL.search(called):
                            raise Unsupported(f'{name}: unmapped native hook/import {called}')
                        target = f'0x{target:X}'
                    else:
                        target = f'uint32_t({indirect[1]})'
                    trailing = code[index+1:]
                    if trailing == ['return;']:
                        output.append(f'      return Transition::TailCall({target});')
                        terminal = True
                        break
                    if trailing:
                        raise Unsupported(f'{name}: call shares instruction with unsupported post-call code')
                    output.append(f'      return Transition::Call({target}, 0x{resume_pc:X});')
                    terminal = True
                else:
                    # Validate the complete instruction together so SIMD
                    # scopes and their temporary locals remain one atomic step.
                    if any(re.fullmatch(r'(?:\w+\(ctx, base\)|REX_CALL_INDIRECT_FUNC\(.*\));|(?:if \(.*\) )?(?:return;|goto loc_[0-9A-Fa-f]+;)', operation) for operation in code[index+1:]):
                        validate_operations(name, [line])
                        output.append('      '+line)
                    else:
                        validate_operations(name, code[index:])
                        output += ['      '+operation for operation in code[index:]]
                        break
            if not terminal:
                output.append(f'      return Transition::Next(0x{resume_pc:X});')
            output.append('    }')
        output += [f'    case 0x{end:X}: return Transition::Return();',
                   '    default: throw std::runtime_error("invalid generated guest PC");', '  }', '}']
        pcs = ', '.join(f'0x{p:X}' for p in sorted(valid))
        registrations.append(f'  program.AddCompiled(0x{entry:X}, {{{pcs}}}, Step_{name});')
    output += ['}', f'void {registration}(Program& program) {{'] + registrations + ['}', '}']
    return '\n'.join(output)+'\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--preamble')
    parser.add_argument('--symbols', type=Path, help='existing generated init.cpp address mappings')
    parser.add_argument('--function', action='append', default=[])
    args = parser.parse_args()
    symbols = {}
    if args.symbols:
        for address, name in re.findall(r'\{\s*0x([0-9a-fA-F]+),\s*REX_GUEST_FUNCTION_POINTER\((\w+)\)', args.symbols.read_text()):
            symbols[name] = int(address, 16)
    try:
        result = transform(args.source.read_text(), args.preamble, symbols, set(args.function))
    except Unsupported as error:
        parser.exit(2, f'{error}\n')
    args.output.write_text(result)


if __name__ == '__main__':
    main()
