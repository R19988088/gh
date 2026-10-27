// The project owns host page and shared mapping operations.
#include <rex/memory/utils.h>
#if REX_PLATFORM_ANDROID
#include <rex/string.h>
#endif
namespace rex::memory {
size_t page_size() { return gh::memory::host::page_size(); }
size_t allocation_granularity() { return gh::memory::host::allocation_granularity(); }
bool IsWritableExecutableMemorySupported() { return gh::memory::host::IsWritableExecutableMemorySupported(); }
void* AllocFixed(void* base, size_t size, AllocationType type, PageAccess access) {
  return gh::memory::host::AllocFixed(base, size, type, access);
}
bool DeallocFixed(void* base, size_t size, DeallocationType type) {
  return gh::memory::host::DeallocFixed(base, size, type);
}
bool Protect(void* base, size_t size, PageAccess access, PageAccess* old_access) {
  return gh::memory::host::Protect(base, size, access, old_access);
}
bool QueryProtect(void* base, size_t& size, PageAccess& access) {
  return gh::memory::host::QueryProtect(base, size, access);
}
FileMappingHandle CreateFileMappingHandle(const std::filesystem::path& path, size_t size,
                                         PageAccess access, bool commit) {
  return gh::memory::host::CreateFileMappingHandle(path, size, access, commit);
}
void CloseFileMappingHandle(FileMappingHandle handle, const std::filesystem::path& path) {
  gh::memory::host::CloseFileMappingHandle(handle, path);
}
void* MapFileView(FileMappingHandle handle, void* base, size_t size, PageAccess access, size_t offset) {
  return gh::memory::host::MapFileView(handle, base, size, access, offset);
}
bool UnmapFileView(FileMappingHandle handle, void* base, size_t size) {
  return gh::memory::host::UnmapFileView(handle, base, size);
}
#if REX_PLATFORM_ANDROID
void AndroidInitialize() { gh::memory::host::AndroidInitialize(rex::GetAndroidApiLevel() >= 26); }
void AndroidShutdown() { gh::memory::host::AndroidShutdown(); }
#endif
}  // namespace rex::memory
