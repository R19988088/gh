/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 * Guest mapping and backing-store ownership adapted for Guardian Heroes.
 */
#include "gh_guest_arena.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>

#if defined(__SWITCH__) || defined(__NX__)
#include "guest_memory_switch.h"
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <memoryapi.h>
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM | \
                            WINAPI_PARTITION_GAMES)
#define GH_ARENA_WINDOWS_DESKTOP
#endif
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__ANDROID__)
#include <dlfcn.h>
#include <linux/ashmem.h>
#include <sys/ioctl.h>
#endif
#if defined(__APPLE__)
#define gh_arena_mmap mmap
#define gh_arena_ftruncate ftruncate
#else
#define gh_arena_mmap mmap64
#define gh_arena_ftruncate ftruncate64
#endif
#endif

namespace gh::memory {
namespace {
struct View {
  uint64_t start;
  uint64_t size;
  uint64_t offset;
};
[[maybe_unused]] constexpr View kViews[] = {
    {0x000000000, 0x40000000, 0x000000000},
    {0x040000000, 0x3F000000, 0x040000000},
    {0x07F000000, 0x01000000, 0x100000000},
    {0x080000000, 0x10000000, 0x080000000},
    {0x090000000, 0x10000000, 0x080000000},
    {0x0A0000000, 0x20000000, 0x100000000},
    {0x0C0000000, 0x20000000, 0x100000000},
    {0x0E0000000, 0x20000000, 0x100001000},
    {0x100000000, 0x20000000, 0x100000000},
};
[[maybe_unused]] constexpr size_t kArenaSize = 0x120000000ull;
}  // namespace

GuestArena::~GuestArena() { Close(); }

void GuestArena::SetSystemError(const char* operation) {
#if defined(__SWITCH__) || defined(__NX__)
  char result[16];
  std::snprintf(result, sizeof(result), "0x%08X", RexGmLastResult());
  error_ = std::string(operation) + ": " + result;
#elif defined(_WIN32)
  error_ = std::string(operation) + ": Windows error " + std::to_string(GetLastError());
#else
  error_ = std::string(operation) + ": " + std::strerror(errno);
#endif
}

bool GuestArena::Initialize() {
  Close();
#if defined(__SWITCH__) || defined(__NX__)
  if (base_) return false;  // Preserve the failed shutdown and its owner.
#endif
  error_.clear();
#if defined(__SWITCH__) || defined(__NX__)
  if (RexGmBase()) {
    error_ = "libnx guest memory supports one live GuestArena";
    return false;
  }
  granularity_ = 0x1000;
  base_ = RexGmInit(kArenaSize);
  if (!base_) {
    SetSystemError("Reserve libnx guest virtual arena");
    return false;
  }
  backing_ = 1;
  name_ = "gh_horizon_guest";
  if (!MapViews(base_)) {
    Close();
    return false;
  }
  return true;
#else
#if defined(_WIN32)
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  granularity_ = info.dwAllocationGranularity;
  const auto process = GetCurrentProcessId();
#else
  granularity_ = static_cast<size_t>(getpagesize());
  const auto process = getpid();
#endif
  // Stay below macOS's 31-byte shm name limit without truncating identities.
  static std::atomic<uint64_t> sequence{0};
  const auto ticks = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  char name[32];
  std::snprintf(name, sizeof(name), "gh_%x_%llx", static_cast<unsigned>(process),
                static_cast<unsigned long long>(ticks + sequence.fetch_add(1)));
  name_ = name;
  const size_t backing_size = (kArenaSize + granularity_ * 2 - 1) & ~(granularity_ - 1);
  if (!CreateBacking(backing_size)) return false;
#if defined(__APPLE__)
  const bool mapped = MapViews(nullptr);
#else
  bool mapped = false;
  for (size_t n = 32; n < 64; ++n) {
    if (MapViews(reinterpret_cast<uint8_t*>(uint64_t{1} << n))) {
      mapped = true;
      break;
    }
  }
#endif
  if (!mapped) {
    Close();
    return false;
  }
  error_.clear();
  return true;
#endif
}

bool GuestArena::CreateBacking(size_t size) {
#if defined(__SWITCH__) || defined(__NX__)
  (void)size;
  return false;
#elif defined(_WIN32)
  const std::wstring mapping_name = L"Local\\" + std::wstring(name_.begin(), name_.end());
#if defined(GH_ARENA_WINDOWS_DESKTOP)
  HANDLE handle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE | SEC_RESERVE,
                                     static_cast<DWORD>(size >> 32), static_cast<DWORD>(size),
                                     mapping_name.c_str());
#else
  HANDLE handle = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, nullptr,
                                           PAGE_READWRITE | SEC_RESERVE, ULONG64(size),
                                           mapping_name.c_str());
#endif
  if (!handle) {
    SetSystemError("Create guest backing");
    return false;
  }
  backing_ = reinterpret_cast<intptr_t>(handle);
  return true;
#elif defined(__ANDROID__)
  void* library = dlopen("libandroid.so", RTLD_NOW);
  using CreateSharedMemory = int (*)(const char*, size_t);
  auto create = library ? reinterpret_cast<CreateSharedMemory>(
                              dlsym(library, "ASharedMemory_create")) : nullptr;
  int fd = -1;
  if (create) {
    fd = create(name_.c_str(), size);
  } else {
    fd = open("/" ASHMEM_NAME_DEF, O_RDWR);
    if (fd >= 0 && (ioctl(fd, ASHMEM_SET_NAME, name_.c_str()) < 0 ||
                    ioctl(fd, ASHMEM_SET_SIZE, size) < 0)) {
      const int saved_error = errno;
      close(fd);
      errno = saved_error;
      fd = -1;
    }
  }
  if (fd < 0) SetSystemError("Create guest shared memory");
  if (library) dlclose(library);
  backing_ = fd;
  return fd >= 0;
#else
  const std::string shm_name = "/" + name_;
  const int fd = shm_open(shm_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0777);
  if (fd < 0) {
    SetSystemError("Create guest backing");
    return false;
  }
  if (gh_arena_ftruncate(fd, static_cast<off_t>(size)) != 0) {
    SetSystemError("Size guest backing");
    close(fd);
    shm_unlink(shm_name.c_str());
    return false;
  }
  backing_ = fd;
  return true;
#endif
}

bool GuestArena::MapViews(uint8_t* requested_base) {
#if defined(__SWITCH__) || defined(__NX__)
  for (size_t n = 0; n < std::size(kViews); ++n) {
    const auto& view = kViews[n];
    if (!RexGmAddView(requested_base + view.start, view.offset, view.size)) {
      SetSystemError("Register libnx guest alias view");
      return false;
    }
    views_[n] = requested_base + view.start;
  }
  return true;
#else
  uint8_t* mapping_base = requested_base;
#if !defined(_WIN32)
  // Reserve first so MAP_FIXED only replaces address space owned by this arena.
  void* reserved = gh_arena_mmap(requested_base, kArenaSize, PROT_NONE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (reserved == MAP_FAILED) {
    SetSystemError("Reserve contiguous guest arena");
    return false;
  }
  if (requested_base && reserved != requested_base) {
    munmap(reserved, kArenaSize);
    error_ = "Requested contiguous guest arena is unavailable";
    return false;
  }
  mapping_base = static_cast<uint8_t*>(reserved);
  base_ = mapping_base;
#endif
  for (size_t n = 0; n < std::size(kViews); ++n) {
    const auto& view = kViews[n];
    void* target = mapping_base + view.start;
    const size_t offset = view.offset & ~(granularity_ - 1);
    void* result = nullptr;
#if defined(_WIN32)
#if defined(GH_ARENA_WINDOWS_DESKTOP)
    result = MapViewOfFileEx(reinterpret_cast<HANDLE>(backing_), FILE_MAP_ALL_ACCESS,
                             static_cast<DWORD>(offset >> 32), static_cast<DWORD>(offset),
                             view.size, target);
#else
    HANDLE process = GetCurrentProcess();
    void* placeholder = VirtualAlloc2FromApp(process, target, view.size,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
    if (placeholder) {
      result = MapViewOfFile3FromApp(reinterpret_cast<HANDLE>(backing_), process, placeholder,
          ULONG64(offset), view.size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
      if (!result) {
        const DWORD saved_error = GetLastError();
        VirtualFree(placeholder, 0, MEM_RELEASE);
        SetLastError(saved_error);
      }
    }
#endif
    const bool failed = result == nullptr;
#else
    result = gh_arena_mmap(target, view.size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
                           static_cast<int>(backing_), static_cast<off_t>(offset));
    const bool failed = result == MAP_FAILED;
#endif
    if (failed) {
      SetSystemError("Map guest alias view");
      UnmapViews();
      return false;
    }
    views_[n] = static_cast<uint8_t*>(result);
  }
  base_ = mapping_base;
  return true;
#endif
}

void GuestArena::UnmapViews() {
#if defined(__SWITCH__) || defined(__NX__)
  if (base_ && !RexGmShutdown()) {
    SetSystemError("Release libnx guest arena (backing retained)");
    return;
  }
#elif defined(_WIN32)
  for (auto* view : views_) {
    if (view) UnmapViewOfFile(view);
  }
#else
  if (base_) munmap(base_, kArenaSize);
#endif
  views_.fill(nullptr);
  base_ = nullptr;
}

void GuestArena::Close() {
  UnmapViews();
#if defined(__SWITCH__) || defined(__NX__)
  if (base_) return;
#endif
  if (backing_ == -1) return;
#if defined(__SWITCH__) || defined(__NX__)
#elif defined(_WIN32)
  CloseHandle(reinterpret_cast<HANDLE>(backing_));
#else
  close(static_cast<int>(backing_));
#if !defined(__ANDROID__)
  shm_unlink(("/" + name_).c_str());
#endif
#endif
  backing_ = -1;
}

}  // namespace gh::memory
