#include "generated/default/guardian_heroes_pch.h"

#include <bit>
#include <type_traits>

static_assert(REX_PLATFORM_SWITCH);
static_assert(REX_ARCH_ARM64);
static_assert(sizeof(void*) == 8);
static_assert(std::endian::native == std::endian::little);
static_assert(std::is_same_v<PPCFunc, GuestFunction>);

// Compile the same entry, RAM, SIMD, FPCR and atomic operations used by the
// generated title without linking a desktop runtime or allocating guest RAM.
DEFINE_REX_FUNC(gh_switch_guest_abi_probe) {
  REX_FUNC_PROLOGUE();
  const uint32_t address = ctx.r3.u32;
  REX_STORE_U32(address, ctx.r4.u32);
  ctx.r5.u64 = REX_LOAD_U32(address);
  ctx.r6.u64 = __builtin_rotateleft64(ctx.r5.u64, 12);

  // Matches the lwarx/stwcx sequence in sub_825DFxxx generated code.
  ctx.reserved.u32 = *reinterpret_cast<uint32_t*>(REX_RAW_ADDR(address));
  ctx.cr0.eq = __sync_bool_compare_and_swap(
      reinterpret_cast<uint32_t*>(REX_RAW_ADDR(address)), ctx.reserved.s32,
      __builtin_bswap32(ctx.r4.s32));
  std::atomic_thread_fence(std::memory_order_seq_cst);

  simde_mm_store_si128(reinterpret_cast<simde__m128i*>(ctx.v0.u8),
      simde_mm_shuffle_epi8(
          simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(ctx.v1.u8)),
          simde_mm_load_si128(reinterpret_cast<const simde__m128i*>(ctx.v2.u8))));
  const uint32_t saved = ctx.fpscr.getcsr();
  ctx.fpscr.storeFromGuest(ctx.r7.u32);
  REX_SET_FLUSH_MODE(false);
  ctx.fpscr.setcsr(saved);
}

// Instantiate the project's runtime entry adapter as well as its bare body.
PPCFunc* gh_switch_guest_abi_entry =
    REX_GUEST_FUNCTION_POINTER(gh_switch_guest_abi_probe);
