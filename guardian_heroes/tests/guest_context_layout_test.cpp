#include "../src/guest_context.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

int main() {
  static_assert(GuestContext::kNonVolatileSaveSize == 1616);

  GuestContext context{};
  context.r14.u64 = 0x1414141414141414ull;
  context.r31.u64 = 0x3131313131313131ull;
  context.f14.f64 = 14.25;
  context.f31.f64 = 31.5;
  context.v14.u32[0] = 0x14141414u;
  context.v31.u32[3] = 0x31313131u;
  context.v64.u64[0] = 0x6464646464646464ull;
  context.v127.u64[1] = 0x7F7F7F7F7F7F7F7Full;
  context.cr2.set_raw(0x5);
  context.cr3.set_raw(0xA);
  context.cr4.set_raw(0x3);

  const uint32_t original_host_csr = GuestFPSCRRegister::getHostCSR();
  context.fpscr.csr = (original_host_csr & ~GuestFPSCRRegister::FlushMask) |
                     GuestFPSCRRegister::FlushMask;
  std::array<uint8_t, GuestContext::kNonVolatileSaveSize> saved{};
  context.SaveNonVolatiles(saved.data());

  context.r14.u64 = 0;
  context.r31.u64 = 0;
  context.f14.f64 = 0;
  context.f31.f64 = 0;
  context.v14.u32[0] = 0;
  context.v31.u32[3] = 0;
  context.v64.u64[0] = 0;
  context.v127.u64[1] = 0;
  context.cr2.set_raw(0);
  context.cr3.set_raw(0);
  context.cr4.set_raw(0);
  context.fpscr.csr = 0;
  context.RestoreNonVolatiles(saved.data());

  assert(context.r14.u64 == 0x1414141414141414ull);
  assert(context.r31.u64 == 0x3131313131313131ull);
  assert(context.f14.f64 == 14.25);
  assert(context.f31.f64 == 31.5);
  assert(context.v14.u32[0] == 0x14141414u);
  assert(context.v31.u32[3] == 0x31313131u);
  assert(context.v64.u64[0] == 0x6464646464646464ull);
  assert(context.v127.u64[1] == 0x7F7F7F7F7F7F7F7Full);
  assert(context.cr2.raw() == 0x5);
  assert(context.cr3.raw() == 0xA);
  assert(context.cr4.raw() == 0x3);
  const uint32_t guest_mask = GuestFPSCRRegister::FlushMask | GuestFPSCRRegister::RoundMask;
  uint32_t saved_fpscr = 0;
  std::memcpy(&saved_fpscr, saved.data() + GuestContext::kNonVolatileSaveSize -
              sizeof(saved_fpscr), sizeof(saved_fpscr));
  assert((context.fpscr.csr & guest_mask) == (saved_fpscr & guest_mask));
  assert((context.fpscr.csr & ~guest_mask) == (original_host_csr & ~guest_mask));
  GuestFPSCRRegister::setHostCSR(original_host_csr);

  std::puts("PASS: GuestContext non-volatile save/restore layout and FPSCR ownership");
}
