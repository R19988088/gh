#pragma once

#include <cstdint>
#include <cstddef>
#include <filesystem>

namespace gh::memory {
enum class PageAccess {
  kNoAccess = 0,
  kReadOnly = 1 << 0,
  kReadWrite = kReadOnly | 1 << 1,
  kExecuteReadOnly = kReadOnly | 1 << 2,
  kExecuteReadWrite = kReadWrite | 1 << 2,
};

enum class AllocationType {
  kReserve = 1 << 0,
  kCommit = 1 << 1,
  kReserveCommit = kReserve | kCommit,
};

enum class DeallocationType {
  kRelease = 1 << 0,
  kDecommit = 1 << 1,
};

namespace host {
size_t page_size();
size_t allocation_granularity();
bool IsWritableExecutableMemorySupported();
void* AllocFixed(void* base, size_t length, AllocationType, PageAccess);
bool DeallocFixed(void* base, size_t length, DeallocationType);
bool Protect(void* base, size_t length, PageAccess, PageAccess* old_access = nullptr);
bool QueryProtect(void* base, size_t& length, PageAccess&);
using FileMappingHandle = intptr_t;
constexpr FileMappingHandle kFileMappingHandleInvalid = -1;
FileMappingHandle CreateFileMappingHandle(const std::filesystem::path&, size_t, PageAccess, bool);
void CloseFileMappingHandle(FileMappingHandle, const std::filesystem::path&);
void* MapFileView(FileMappingHandle, void*, size_t, PageAccess, size_t);
bool UnmapFileView(FileMappingHandle, void*, size_t);
#if defined(__ANDROID__)
void AndroidInitialize(bool shared_memory_api_available);
void AndroidShutdown();
#endif
}  // namespace host
}  // namespace gh::memory
