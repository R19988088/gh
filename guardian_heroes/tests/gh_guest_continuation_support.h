#pragma once
#include "guest_context.h"
#include <cstdint>
#include <cstring>
inline uint32_t ReadFixture32(const uint8_t* base, uint32_t address) {
  return uint32_t(base[address]) << 24 | uint32_t(base[address+1]) << 16 |
         uint32_t(base[address+2]) << 8 | base[address+3];
}
inline void WriteFixture32(uint8_t* base, uint32_t address, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) base[address+i] = uint8_t(value >> (24-i*8));
}
#define REX_STORE_U32(address, value) WriteFixture32(base, uint32_t(address), uint32_t(value))
#define REX_LOAD_U32(address) ReadFixture32(base, uint32_t(address))
#define REX_FUNC_PROLOGUE() ((void)0)
#define DEFINE_REX_FUNC(name) void name(GuestContext& ctx, [[maybe_unused]] uint8_t* base)
#define PPCRegister GuestRegister
#define PPCVRegister GuestVectorRegister
void sub_8243AAE8(GuestContext&, uint8_t*);
void DispatchFixtureIndirect(GuestContext&, uint8_t*, uint32_t);
#define REX_CALL_INDIRECT_FUNC(address) DispatchFixtureIndirect(ctx, base, uint32_t(address))
