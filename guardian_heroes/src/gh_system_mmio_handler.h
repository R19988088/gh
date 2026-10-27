/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2014 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once

#include <memory>
#include <rex/platform.h>
#include "gh_mmio.h"
namespace rex::arch { class Exception; }
namespace rex::runtime {
using gh::memory::MMIOReadCallback;
using gh::memory::MMIOWriteCallback;
using gh::memory::MMIORange;
class MMIOHandler : private gh::memory::FaultHost {
 public:
  ~MMIOHandler();
  using HostToGuestVirtual = gh::memory::MMIO::HostToGuestVirtual;
  using AccessViolationCallback = gh::memory::MMIO::AccessViolationCallback;
  static std::unique_ptr<MMIOHandler> Install(uint8_t*, uint8_t*, uint8_t*, HostToGuestVirtual,
      const void*, AccessViolationCallback, void*);
  static MMIOHandler* global_handler();
  bool RegisterRange(uint32_t address, uint32_t mask, uint32_t size, void* context,
      MMIOReadCallback read, MMIOWriteCallback write) {
    return core_.RegisterRange(address, mask, size, context, read, write);
  }
  MMIORange* LookupRange(uint32_t address) { return core_.LookupRange(address); }
  bool CheckLoad(uint32_t address, uint32_t* value) { return core_.CheckLoad(address, value); }
  bool CheckStore(uint32_t address, uint32_t value) { return core_.CheckStore(address, value); }
 private:
  MMIOHandler(uint8_t*, uint8_t*, uint8_t*, HostToGuestVirtual, const void*, AccessViolationCallback, void*);
  static bool ExceptionCallbackThunk(arch::Exception*, void*);
  bool ExceptionCallback(arch::Exception*);
  std::recursive_mutex& GlobalMutex() override;
  uint32_t HostPageSize() const override;
  bool QueryProtection(void*, size_t&, gh::memory::PageAccess&) override;
  void FaultLog(std::string_view message) override;
  gh::memory::MMIO core_;
  static MMIOHandler* global_handler_;
};
}  // namespace rex::runtime
