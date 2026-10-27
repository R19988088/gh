#pragma once

#include "guest_context.h"
#include "native_memory.h"

#include <cstdint>
#include <cstring>
#include <string_view>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i32 = int32_t;
using GuestFunction = void(GuestContext&, uint8_t*);

#define GH_EXTERN(name) extern "C" void name(GuestContext&, uint8_t*)
#define GH_HOOK(name) extern "C" void name(GuestContext& ctx, uint8_t* base)
#define GH_PROLOGUE() ((void)0)

#define GH_PHYS_HOST_OFFSET(addr) GuestMemory::HostOffset((u32)(addr))
#define GH_RAW(x) (GuestMemory(base).Address((u32)(x)))
#define GH_LOAD_U8(x) (GuestMemory(base).Read8((u32)(x)))
#define GH_LOAD_U16(x) (GuestMemory(base).Read16((u32)(x)))
#define GH_LOAD_U32(x) (GuestMemory(base).Read32((u32)(x)))
#define GH_LOAD_U64(x) (GuestMemory(base).Read64((u32)(x)))
#define GH_LOAD_STRING(x, len) \
  std::string_view(reinterpret_cast<const char*>(GH_RAW(x)), (len))
#define GH_STORE_U8(x, y) (GuestMemory(base).Write8((u32)(x), (uint8_t)(y)))
#define GH_STORE_U16(x, y) (GuestMemory(base).Write16((u32)(x), (uint16_t)(y)))
#define GH_STORE_U32(x, y) (GuestMemory(base).Write32((u32)(x), (uint32_t)(y)))
#define GH_STORE_U64(x, y) (GuestMemory(base).Write64((u32)(x), (uint64_t)(y)))

#define GH_MM_LOAD_U8(x) (GuestMemory(base).Read8((u32)(x)))
#define GH_MM_LOAD_U16(x) (GuestMemory(base).Read16((u32)(x)))
#define GH_MM_LOAD_U32(x) (GuestMemory(base).Read32((u32)(x)))
#define GH_MM_LOAD_U64(x) (GuestMemory(base).Read64((u32)(x)))
#define GH_MM_STORE_U8(x, y) GH_STORE_U8((x), (y))
#define GH_MM_STORE_U16(x, y) GH_STORE_U16((x), (y))
#define GH_MM_STORE_U32(x, y) GH_STORE_U32((x), (y))
#define GH_MM_STORE_U64(x, y) GH_STORE_U64((x), (y))
