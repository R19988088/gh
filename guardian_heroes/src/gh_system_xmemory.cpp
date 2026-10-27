/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <cstring>
#include <utility>

#include "gh_memory_service.h"

#include <fmt/format.h>

#include <rex/assert.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/stream.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/mmio_handler.h>
#include "gh_system_xmemory.h"
#include <rex/thread.h>

// TODO(benvanik): move xbox.h out
#include <rex/system/xtypes.h>

REXCVAR_DEFINE_BOOL(protect_zero, true, "Memory", "Protect the zero page from reads and writes")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_BOOL(protect_on_release, false, "Memory",
                    "Protect released memory to prevent accesses");

REXCVAR_DEFINE_BOOL(scribble_heap, false, "Memory", "Scribble 0xCD into all allocated heap memory");

namespace rex::memory {

namespace {
class HeapStreamAdapter final : public gh::memory::HeapStream {
 public:
  explicit HeapStreamAdapter(stream::ByteStream& stream) : stream_(stream) {}
  void Read(void* data, size_t size) override { stream_.Read(data, size); }
  void Write(const void* data, size_t size) override { stream_.Write(data, size); }
 private:
  stream::ByteStream& stream_;
};
}  // namespace

static Memory* active_memory_ = nullptr;
void CrashDump() {
  static std::atomic<int> in_crash_dump(0);
  if (in_crash_dump.fetch_add(1)) return;
  active_memory_->DumpMap();
  --in_crash_dump;
}
Memory::Memory() {
  service_ = std::make_unique<gh::memory::MemoryService>(
      rex::thread::global_critical_region::mutex(),
      [] { return REXCVAR_GET(protect_on_release); },
      [] { return REXCVAR_GET(scribble_heap); },
      [](int level, std::string_view message) {
        if (level == 3) REXSYS_ERROR("{}", message);
        else if (level == 2) REXSYS_WARN("{}", message);
        else if (level == 1) REXSYS_INFO("{}", message);
        else REXSYS_DEBUG("{}", message);
      });
  assert_zero(active_memory_);
  active_memory_ = this;
}
Memory::~Memory() {
  assert_true(active_memory_ == this);
  active_memory_ = nullptr;
  mmio_handler_.reset();
  service_.reset();
}
bool Memory::Initialize() {
  return service_->Initialize(REXCVAR_GET(protect_zero), [this] {
    mmio_handler_ = runtime::MMIOHandler::Install(
        virtual_membase(), physical_membase(), physical_membase() + 0x1FFFFFFF,
        HostToGuestVirtualThunk, this, AccessViolationCallbackThunk, this);
    return mmio_handler_ != nullptr;
  });
}
void Memory::Reset() { service_->Reset(); }
const BaseHeap* Memory::LookupHeap(uint32_t address) const { return service_->LookupHeap(address); }
BaseHeap* Memory::LookupHeapByType(bool physical, uint32_t page_size) { return service_->LookupHeapByType(physical, page_size); }
VirtualHeap* Memory::GetPhysicalHeap() { return service_->GetPhysicalHeap(); }
uint32_t Memory::HostToGuestVirtual(const void* address) const { return service_->HostToGuestVirtual(address); }
uint32_t Memory::HostToGuestVirtualThunk(const void* context, const void* address) {
  return static_cast<const Memory*>(context)->HostToGuestVirtual(address);
}
uint32_t Memory::GetPhysicalAddress(uint32_t address) const { return service_->GetPhysicalAddress(address); }
void Memory::Zero(uint32_t address, uint32_t size) { service_->Zero(address, size); }
void Memory::Fill(uint32_t address, uint32_t size, uint8_t value) { service_->Fill(address, size, value); }
void Memory::Copy(uint32_t dest, uint32_t src, uint32_t size) { service_->Copy(dest, src, size); }
uint32_t Memory::SearchAligned(uint32_t start, uint32_t end, const uint32_t* values, size_t count) {
  return service_->SearchAligned(start, end, values, count);
}
bool Memory::AddVirtualMappedRange(uint32_t address, uint32_t mask, uint32_t size, void* context,
                                   runtime::MMIOReadCallback read, runtime::MMIOWriteCallback write) {
  return service_->PrepareMappedRange(address, size) &&
      mmio_handler_->RegisterRange(address, mask, size, context, read, write);
}
runtime::MMIORange* Memory::LookupVirtualMappedRange(uint32_t address) { return mmio_handler_->LookupRange(address); }
bool Memory::AccessViolationCallbackThunk(std::unique_lock<std::recursive_mutex> lock, void* context,
                                         void* address, bool write) {
  return static_cast<Memory*>(context)->service_->AccessViolationCallback(std::move(lock), address, write);
}
bool Memory::TriggerPhysicalMemoryCallbacks(std::unique_lock<std::recursive_mutex> lock,
                                            uint32_t address, uint32_t size, bool write, bool exact, bool unprotect) {
  return service_->TriggerPhysicalMemoryCallbacks(std::move(lock), address, size, write, exact, unprotect);
}
void* Memory::RegisterPhysicalMemoryInvalidationCallback(PhysicalMemoryInvalidationCallback callback, void* context) {
  return service_->RegisterPhysicalMemoryInvalidationCallback(callback, context);
}
void Memory::UnregisterPhysicalMemoryInvalidationCallback(void* handle) { service_->UnregisterPhysicalMemoryInvalidationCallback(handle); }
void Memory::EnablePhysicalMemoryAccessCallbacks(uint32_t address, uint32_t size, bool notifications, bool providers) {
  service_->EnablePhysicalMemoryAccessCallbacks(address, size, notifications, providers);
}
uint32_t Memory::SystemHeapAlloc(uint32_t size, uint32_t alignment, uint32_t flags) {
  return service_->SystemHeapAlloc(size, alignment, (flags & kSystemHeapPhysical) != 0);
}
void Memory::SystemHeapFree(uint32_t address, uint32_t* size) { service_->SystemHeapFree(address, size); }
void Memory::GetHeapsPageStatsSummary(const BaseHeap* const* heaps, size_t count,
                                     uint32_t& unreserved, uint32_t& reserved, uint32_t& used, uint32_t& bytes) {
  service_->GetHeapsPageStatsSummary(heaps, count, unreserved, reserved, used, bytes);
}
void Memory::DumpMap() { service_->DumpMap(); }
bool Memory::Save(stream::ByteStream* stream) {
  HeapStreamAdapter adapter(*stream);
  return service_->Save(&adapter);
}
bool Memory::Restore(stream::ByteStream* stream) {
  HeapStreamAdapter adapter(*stream);
  return service_->Restore(&adapter);
}

bool Memory::InitializeFunctionTable(uint32_t code_base, uint32_t code_size, uint32_t image_base,
                                     uint32_t image_size) {
  constexpr uint32_t kThunkReserveSize = runtime::FunctionDispatcher::kThunkReserveSize;
  uint32_t table_base = image_base + image_size;
  uint32_t table_size = (code_size + kThunkReserveSize) * 2;

  REXSYS_DEBUG(
      "Initializing function table at {:08X}, size {:08X} for code {:08X}-{:08X} "
      "(+{:08X} thunk reserve)",
      table_base, table_size, code_base, code_base + code_size, kThunkReserveSize);

  if (!service_->LookupHeap(0x80000000)->AllocFixed(
          table_base, table_size, 0x10000,
          memory::kMemoryAllocationReserve | memory::kMemoryAllocationCommit,
          memory::kMemoryProtectRead | memory::kMemoryProtectWrite)) {
    REXSYS_ERROR("Failed to allocate function table at {:08X}", table_base);
    return false;
  }

  Zero(table_base, table_size);

  std::lock_guard lock(function_tables_mutex_);
  function_tables_.push_back({
      .table_base = table_base,
      .code_base = code_base,
      .code_size = code_size,
      .thunk_reserve = kThunkReserveSize,
  });

  return true;
}

bool Memory::DestroyFunctionTable(uint32_t code_base) {
  uint32_t table_base = 0;
  uint32_t table_size = 0;
  uint32_t logged_code_base = 0;
  uint32_t logged_code_size = 0;
  {
    std::lock_guard lock(function_tables_mutex_);
    auto it = std::find_if(
        function_tables_.begin(), function_tables_.end(),
        [code_base](const FunctionTableEntry& entry) { return entry.code_base == code_base; });
    if (it == function_tables_.end()) {
      return false;
    }
    table_base = it->table_base;
    table_size = (it->code_size + it->thunk_reserve) * 2;
    logged_code_base = it->code_base;
    logged_code_size = it->code_size;
    function_tables_.erase(it);
  }

  REXSYS_DEBUG("Destroying function table at {:08X}, size {:08X} for code {:08X}-{:08X}",
               table_base, table_size, logged_code_base, logged_code_base + logged_code_size);

  service_->LookupHeap(0x80000000)->Release(table_base);
  return true;
}

bool Memory::SetFunction(uint32_t guest_address, PPCFunc* host_function) {
  uint32_t table_address = 0;
  {
    std::lock_guard lock(function_tables_mutex_);
    for (const auto& entry : function_tables_) {
      uint32_t range_end = entry.code_base + entry.code_size + entry.thunk_reserve;
      if (guest_address >= entry.code_base && guest_address < range_end) {
        uint64_t offset = (uint64_t(guest_address) - entry.code_base) * 2;
        table_address = entry.table_base + uint32_t(offset);
        break;
      }
    }
  }
  if (!table_address) {
    return false;
  }
  auto* slot = TranslateVirtual<PPCFunc**>(table_address);
  *slot = host_function;
  return true;
}

bool Memory::HasAnyFunctionTable() const {
  std::lock_guard lock(function_tables_mutex_);
  return !function_tables_.empty();
}

}  // namespace rex::memory
