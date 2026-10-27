#!/usr/bin/env python3
"""Compare instruction-local operations and closed critical CFGs with emitted C++."""
from pathlib import Path
import importlib.util
import re
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
project = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('continuation_codegen', project / 'tools/gh_guest_continuation_codegen.py')
codegen = importlib.util.module_from_spec(spec)
spec.loader.exec_module(codegen)
source = ''
for partition, function in [(12, 'sub_825F7158'), (30, 'sub_825E5C10'), (77, 'sub_826E2960')]:
    original = (project / f'generated/default/guardian_heroes_recomp.{partition}.cpp').read_text()
    source += re.search(r'DEFINE_REX_FUNC\('+function+r'\) \{.*?\n\}', original, re.S)[0]+'\n'
source += '''
DEFINE_REX_FUNC(sub_00008000) {
  REX_FUNC_PROLOGUE();
  PPCRegister temp{};
  uint32_t ea{};
  // vsrh v0,v1,v2
  {
    simde__m128i a = simde_mm_load_si128((simde__m128i*)ctx.v1.u8);
    simde__m128i b = simde_mm_load_si128((simde__m128i*)ctx.v2.u8);
    simde__m128i sh = simde_mm_and_si128(b, simde_mm_set1_epi16(0xF));
    simde_mm_store_si128((simde__m128i*)ctx.v0.u8, simde_mm_or_si128(
        rex::ppc::simde_mm_sllv_epi16(a, sh),
        rex::ppc::simde_mm_srlv_epi16(a, sh)));
  }
  // sraw r11,r6,r9
  temp.u32 = ctx.r9.u32 & 0x3F;
  if (temp.u32 > 0x1F) temp.u32 = 0x1F;
  ctx.xer.ca = (ctx.r6.s32 < 0) & (((ctx.r6.s32 >> temp.u32) << temp.u32) != ctx.r6.s32);
  ctx.r11.s64 = ctx.r6.s32 >> temp.u32;
  // stvlx128 v0,r0,r3
  ea = ctx.r3.u32;
  for (size_t i = 0; i < (16 - (ea & 0xF)); i++)
    REX_STORE_U8(ea + i, ctx.v0.u8[15 - i]);
  // fsqrts f1,f0
  ctx.f1.f64 = double(float(sqrt(ctx.f0.f64)));
  // blr
  return;
}
'''
# Refuse cross-frame, unbalanced and bypassed critical regions before emitting code.
for body in [
    '// mtmsrd\nREX_ENTER_GLOBAL_LOCK();\n// bl\nsub_00002000(ctx, base);\n// mtmsrd\nREX_LEAVE_GLOBAL_LOCK();',
    '// mtmsrd\nREX_ENTER_GLOBAL_LOCK();\n// blr\nreturn;',
    '// b\ngoto loc_1008;\n// mtmsrd\nREX_ENTER_GLOBAL_LOCK();\nloc_1008:\n// addi\nctx.r3.u64 = 1;\n// mtmsrd\nREX_LEAVE_GLOBAL_LOCK();',
    '// mtmsrd\nREX_ENTER_GLOBAL_LOCK();\n// b\ngoto loc_1000;\n// mtmsrd\nREX_LEAVE_GLOBAL_LOCK();',
]:
    try:
        codegen.transform('DEFINE_REX_FUNC(sub_1000) {\n'+body+'\n}')
    except codegen.Unsupported:
        pass
    else:
        raise AssertionError('unsafe native lock boundary accepted')

support = r'''
#pragma once
#include "gh_guest_continuation_support.h"
#include "gh_threading.h"
#include <rex/ppc/intrinsics.h>
#include <atomic>
using rex::ppc::VectorMaskL;
using rex::ppc::VectorShiftTableL;
inline uint64_t Read64(const uint8_t* base, uint32_t p) {
  return uint64_t(ReadFixture32(base, p)) << 32 | ReadFixture32(base, p+4);
}
#define REX_LOAD_U16(p) (uint16_t(base[uint32_t(p)]) << 8 | base[uint32_t(p)+1])
#define REX_LOAD_U64(p) Read64(base, uint32_t(p))
#define REX_STORE_U8(p, v) (base[uint32_t(p)] = uint8_t(v))
#define REX_STORE_U16(p, v) (base[uint32_t(p)] = uint8_t(uint16_t(v) >> 8), base[uint32_t(p)+1] = uint8_t(v))
#define REX_RAW_ADDR(p) (base + uint32_t(p))
#define REX_CHECK_GLOBAL_LOCK() gh::thread::GlobalCriticalRegion::Check()
#define REX_ENTER_GLOBAL_LOCK() gh::thread::GlobalCriticalRegion::Enter()
#define REX_LEAVE_GLOBAL_LOCK() gh::thread::GlobalCriticalRegion::Leave()
'''
test = r'''
#include "operations_support.h"
#include "gh_guest_continuation.h"
#include "operations_original.h"
#include <array>
#include <cassert>
#include <iostream>
namespace c = gh::guest::continuation;
namespace gh::guest::continuation { void RegisterGeneratedContinuation(Program&); }
using Memory = std::array<uint8_t, 8192>;
using Function = void (*)(GuestContext&, uint8_t*);
static void Compare(c::Program& program, uint32_t address, Function original,
                    GuestContext context, Memory memory) {
  auto expected = context;
  auto expected_memory = memory;
  GuestFPSCRRegister::setHostCSR(context.fpscr.csr);
  original(expected, expected_memory.data());
  c::Machine machine(program);
  machine.context = context;
  machine.Start(address);
  unsigned pauses = 0;
  for (;;) {
    auto result = machine.Run(memory.data(), 1);
    assert(gh::thread::GlobalCriticalRegion::depth() == 0);
    if (result.status == c::Status::Complete) break;
    assert(result.status == c::Status::Paused && ++pauses < 1000);
    c::Machine replacement(program);
    replacement.Restore(machine.Save());
    machine.Restore(replacement.Save());
  }
  assert(memory == expected_memory);
  auto actual = machine.context;
  actual.fpscr.csr = expected.fpscr.csr = 0;
  assert(std::memcmp(&actual, &expected, sizeof(actual)) == 0);
}
int main() {
  c::Program program;
  c::RegisterGeneratedContinuation(program);
  GuestContext initial{};
  initial.fpscr.csr = GuestFPSCRRegister::getHostCSR();
  initial.r1.u64 = 4096;
  initial.r3.u64 = 256;
  initial.r4.u64 = 512;
  initial.r5.u64 = 516;
  alignas(32) Memory memory{};
  for (unsigned i = 0; i < 8; ++i) {
    memory[1024+i*2] = uint8_t(0x38+i);
    memory[1025+i*2] = uint8_t(i*17);
    initial.v1.u16[i] = uint16_t(0x8101 + i*997);
    initial.v2.u16[i] = uint16_t(i*3);
  }
  auto half = initial;
  half.r4.u64 = 1024;
  half.r5.u64 = 8;
  Compare(program, 0x825F7158, sub_825F7158, half, memory);
  auto locked = initial;
  locked.r4.u64 = 0xABCD1234;
  Compare(program, 0x826E2960, sub_826E2960, locked, memory);
  Compare(program, 0x825E5C10, sub_825E5C10, initial, memory);
  for (unsigned shift : {0u, 1u, 31u, 32u, 63u}) {
    for (int value : {-2147483647, -1, 0, 1234567}) {
      auto arithmetic = initial;
      arithmetic.r3.u64 = 257+shift%16;
      arithmetic.r6.s64 = value;
      arithmetic.r9.u64 = shift;
      arithmetic.f0.f64 = 3.141592653589793;
      Compare(program, 0x8000, sub_00008000, arithmetic, memory);
    }
  }
  // Change guest memory between ld and ldarx, taking the second unlock path.
  c::Machine alternate(program);
  alternate.context = initial;
  alternate.Start(0x825E5C10);
  assert(alternate.Run(memory.data(), 3).status == c::Status::Paused);
  assert(alternate.frames().back().pc == 0x825E5C1C);
  WriteFixture32(memory.data(), 260, 17);
  assert(alternate.Run(memory.data(), 2).status == c::Status::Paused);
  assert(alternate.frames().back().pc == 0x825E5C48);
  assert(gh::thread::GlobalCriticalRegion::depth() == 0);
  alternate.Restore(alternate.Save());
  assert(alternate.Run(memory.data(), 100).status == c::Status::Complete);
  assert(gh::thread::GlobalCriticalRegion::depth() == 0);
  assert(ReadFixture32(memory.data(), 512) == 17);
  std::cout << "SIMD scopes, multiline operations, clamps, byte loops, real half conversion, and both native unlock paths passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='gh-continuation-operations-') as directory:
    temporary = Path(directory)
    (temporary / 'operations_support.h').write_text(support)
    (temporary / 'operations_original.h').write_text(source)
    (temporary / 'operations.cpp').write_text(codegen.transform(source, 'operations_support.h'))
    (temporary / 'test.cpp').write_text(test)
    binary = temporary / 'test'
    subprocess.run(['clang++', '-std=c++23', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(project / 'src'), '-I'+str(project / 'tests'),
                    '-I'+str(project / 'runtime/include'), '-I'+str(project / 'runtime/thirdparty/simde'),
                    '-I'+str(temporary), str(project / 'src/gh_guest_continuation.cpp'),
                    str(project / 'src/gh_threading.cpp'), str(temporary / 'operations.cpp'),
                    str(temporary / 'test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
