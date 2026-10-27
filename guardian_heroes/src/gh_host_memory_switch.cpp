#include "gh_host_memory.h"

#if defined(__SWITCH__) || defined(__NX__)

#include "guest_memory_switch.h"

#include <switch.h>
#include <cerrno>
#include <cstdio>

namespace gh::memory::host {
namespace {
constexpr size_t kPage = 0x1000;

bool Fail(const char* operation, int error) {
  errno = error;
  std::fprintf(stderr, "GH Horizon memory: %s failed (core=0x%08X, errno=%d)\n",
               operation, RexGmLastResult(), error);
  return false;
}

bool InWindow(void* address, size_t length, bool aligned = true) {
  const uintptr_t start = reinterpret_cast<uintptr_t>(RexGmBase());
  const uintptr_t value = reinterpret_cast<uintptr_t>(address);
  return start && length && value >= start && value - start < RexGmSize() &&
         length <= RexGmSize() - (value - start) &&
         (!aligned || (!(value % kPage) && !(length % kPage)));
}

bool Supported(PageAccess access) {
  // Guest PPC instructions are statically recompiled. Host executable mappings
  // and emulated read-only accesses are separate capabilities, not RW aliases.
  if (access == PageAccess::kNoAccess || access == PageAccess::kReadWrite) return true;
  return Fail("read-only/executable protection requires exception integration", ENOTSUP);
}

// Map just the requested view. Other mirrors need RexGmFaultIn before direct
// access until the Horizon exception dispatcher is connected to the GH runtime.
bool MapWritableRange(void* address, size_t length) {
  uintptr_t current = reinterpret_cast<uintptr_t>(address);
  const uintptr_t end = current + length;
  while (current < end) {
    if (!RexGmShadowFor(current)) return Fail("map uncommitted guest memory", EFAULT);
    MemoryInfo info{};
    u32 page_info = 0;
    Result result = svcQueryMemory(&info, &page_info, current);
    if (R_FAILED(result)) return Fail("query mapped guest view", EIO);
    if ((info.perm & Perm_Rw) != Perm_Rw) {
      if (!RexGmFaultIn(current)) return Fail("map committed guest view", EIO);
      result = svcQueryMemory(&info, &page_info, current);
      if (R_FAILED(result) || (info.perm & Perm_Rw) != Perm_Rw)
        return Fail("verify guest view permissions", EIO);
    }
    const uintptr_t region_end = info.addr + info.size;
    if (region_end <= current) return Fail("invalid guest memory region", EFAULT);
    current = region_end < end ? region_end : end;
  }
  return true;
}

bool MappingOffset(void* address, size_t length, size_t* offset) {
  if (!InWindow(address, length)) return Fail("unaligned/outside guest window", EINVAL);
  size_t first = 0, last = 0;
  const uintptr_t value = reinterpret_cast<uintptr_t>(address);
  if (!RexGmWindowToOffset(value, &first) ||
      !RexGmWindowToOffset(value + length - 1, &last) ||
      last != first + length - 1)
    return Fail("allocation crosses guest alias views", EINVAL);
  *offset = first;
  return true;
}
}  // namespace

size_t page_size() { return kPage; }
size_t allocation_granularity() { return kPage; }
bool IsWritableExecutableMemorySupported() { return false; }

void* AllocFixed(void* address, size_t length, AllocationType type, PageAccess access) {
  size_t offset = 0;
  if (!Supported(access) || !MappingOffset(address, length, &offset)) return nullptr;
  if (type == AllocationType::kReserve) return address;
  if (type != AllocationType::kCommit && type != AllocationType::kReserveCommit) {
    Fail("unknown allocation type", EINVAL);
    return nullptr;
  }
  const RexGmAccess permission = access == PageAccess::kReadWrite ? REX_GM_WRITE : REX_GM_NONE;
  if (!RexGmCommit(offset, length, permission)) {
    Fail("commit guest backing", ENOMEM);
    return nullptr;
  }
  if (!RexGmProtect(static_cast<uint8_t*>(address), length, permission, nullptr)) {
    Fail("apply committed guest protection", EIO);
    return nullptr;
  }
  if (permission == REX_GM_WRITE && !MapWritableRange(address, length)) return nullptr;
  return address;
}

bool Protect(void* address, size_t length, PageAccess access, PageAccess* old_access) {
  if (!Supported(access) || !InWindow(address, length)) return false;
  RexGmAccess previous = REX_GM_NONE;
  size_t previous_length = kPage;
  if (!RexGmQueryProtect(static_cast<uint8_t*>(address), &previous_length, &previous))
    return Fail("query previous guest protection", EFAULT);
  const RexGmAccess permission = access == PageAccess::kReadWrite ? REX_GM_WRITE : REX_GM_NONE;
  if (!RexGmProtect(static_cast<uint8_t*>(address), length, permission, nullptr))
    return Fail("protect guest view", EIO);
  if (permission == REX_GM_WRITE && !MapWritableRange(address, length)) return false;
  if (old_access) *old_access = previous == REX_GM_WRITE ? PageAccess::kReadWrite : PageAccess::kNoAccess;
  return true;
}

bool DeallocFixed(void* address, size_t length, DeallocationType type) {
  if (type != DeallocationType::kRelease && type != DeallocationType::kDecommit)
    return Fail("unknown deallocation type", EINVAL);
  // A view does not own its backing exclusively. Preserve the shared bytes for
  // other mirrors; GuestArena::Close releases the backing after all views close.
  return Protect(address, length, PageAccess::kNoAccess);
}

bool QueryProtect(void* address, size_t& length, PageAccess& access) {
  if (!InWindow(address, 1, false)) {
    length = 0;
    return Fail("query outside guest window", EINVAL);
  }
  RexGmAccess permission = REX_GM_NONE;
  if (!RexGmQueryProtect(static_cast<uint8_t*>(address), &length, &permission)) {
    length = 0;
    return Fail("query guest protection", EFAULT);
  }
  access = permission == REX_GM_WRITE ? PageAccess::kReadWrite : PageAccess::kNoAccess;
  return true;
}

// GH GuestArena owns the anonymous window directly. File-backed mappings and
// arbitrary host allocations have no GH Switch caller and are not implemented.
FileMappingHandle CreateFileMappingHandle(const std::filesystem::path&, size_t, PageAccess, bool) {
  Fail("file mapping", ENOTSUP);
  return kFileMappingHandleInvalid;
}
void CloseFileMappingHandle(FileMappingHandle, const std::filesystem::path&) {
  Fail("close unsupported file mapping", ENOTSUP);
}
void* MapFileView(FileMappingHandle, void*, size_t, PageAccess, size_t) {
  Fail("map file view", ENOTSUP);
  return nullptr;
}
bool UnmapFileView(FileMappingHandle, void*, size_t) {
  return Fail("unmap file view", ENOTSUP);
}
}  // namespace gh::memory::host
#endif
