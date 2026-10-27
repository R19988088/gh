#include "gh_memory_rex_adapter.h"

#include "native_memory.h"

#include <rex/system/mmio_handler.h>

namespace gh::memory {
namespace {

uint32_t Load(void*, uint32_t address) {
  auto* handler = rex::runtime::MMIOHandler::global_handler();
  uint32_t value = 0;
  if (!handler || !handler->CheckLoad(address, &value)) return 0;
  return value;
}

void Store(void*, uint32_t address, uint32_t value) {
  auto* handler = rex::runtime::MMIOHandler::global_handler();
  if (handler) handler->CheckStore(address, value);
}

GuestMemory::MmioHandler& RuntimeHandler() {
  static GuestMemory::MmioHandler handler{nullptr, &Load, &Store};
  return handler;
}

}  // namespace

void BindReXGuestMemory() noexcept {
  GuestMemory::SetGlobalMmioHandler(&RuntimeHandler());
}

void UnbindGuestMemory() noexcept {
  GuestMemory::SetGlobalMmioHandler(nullptr);
}

}  // namespace gh::memory
