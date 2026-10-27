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

#include <algorithm>
#include <cstring>
#include <utility>

#include <rex/assert.h>
#include <rex/exception_handler.h>
#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/platform.h>
#include "gh_system_mmio_handler.h"
#include <rex/types.h>
#include <rex/thread/mutex.h>

using namespace rex::arch;

namespace rex::runtime {

MMIOHandler* MMIOHandler::global_handler_ = nullptr;

MMIOHandler* MMIOHandler::global_handler() {
  return global_handler_;
}

std::unique_ptr<MMIOHandler> MMIOHandler::Install(uint8_t* virtual_membase,
                                                  uint8_t* physical_membase, uint8_t* membase_end,
                                                  HostToGuestVirtual host_to_guest_virtual,
                                                  const void* host_to_guest_virtual_context,
                                                  AccessViolationCallback access_violation_callback,
                                                  void* access_violation_callback_context) {
  // There can be only one handler at a time.
  assert_null(global_handler_);
  if (global_handler_) {
    return nullptr;
  }

  auto handler = std::unique_ptr<MMIOHandler>(new MMIOHandler(
      virtual_membase, physical_membase, membase_end, host_to_guest_virtual,
      host_to_guest_virtual_context, access_violation_callback, access_violation_callback_context));

  // Install exception handler for memory coherence (SharedMemory write tracking).
  // Note: MMIO operations are handled at the recompiler level via REX_MM_LOAD/STORE
  // macros that call CheckLoad/CheckStore directly.
  arch::ExceptionHandler::Install(ExceptionCallbackThunk, handler.get());

  global_handler_ = handler.get();
  return handler;
}

MMIOHandler::MMIOHandler(uint8_t* virtual_membase, uint8_t* physical_membase, uint8_t* membase_end,
                         HostToGuestVirtual host_to_guest_virtual,
                         const void* host_to_guest_virtual_context,
                         AccessViolationCallback access_violation_callback,
                         void* access_violation_callback_context)
    : core_(*this, virtual_membase, physical_membase, membase_end,
            host_to_guest_virtual, host_to_guest_virtual_context,
            access_violation_callback, access_violation_callback_context) {}

MMIOHandler::~MMIOHandler() {
  arch::ExceptionHandler::Uninstall(ExceptionCallbackThunk, this);

  if (global_handler_ == this) {
    global_handler_ = nullptr;
  } else {
    REXLOG_ERROR("~MMIOHandler: global_handler_ does not match this instance");
  }
}

std::recursive_mutex& MMIOHandler::GlobalMutex() {
  return rex::thread::global_critical_region::mutex();
}
uint32_t MMIOHandler::HostPageSize() const { return rex::memory::page_size(); }
bool MMIOHandler::QueryProtection(void* address, size_t& size, gh::memory::PageAccess& access) {
  return rex::memory::QueryProtect(address, size, access);
}
void MMIOHandler::FaultLog(std::string_view message) { REXLOG_ERROR("{}", message); }

bool MMIOHandler::ExceptionCallbackThunk(arch::Exception* ex, void* data) {
  return static_cast<MMIOHandler*>(data)->ExceptionCallback(ex);
}
bool MMIOHandler::ExceptionCallback(arch::Exception* ex) {
  if (ex->code() != arch::Exception::Code::kAccessViolation) return false;
  const auto operation = ex->access_violation_operation();
  if (operation != arch::Exception::AccessViolationOperation::kRead &&
      operation != arch::Exception::AccessViolationOperation::kWrite) return false;
  gh::memory::FaultContext fault;
  fault.address = ex->fault_address();
  fault.instruction = ex->pc();
  fault.is_write = operation == arch::Exception::AccessViolationOperation::kWrite;
  auto& context = *ex->thread_context();
#if REX_ARCH_ARM64
  std::copy_n(context.x, 31, fault.x.begin());
  for (size_t i = 0; i < 32; ++i) fault.v[i].u32[0] = context.v[i].u32[0];
  fault.sp = context.sp;
#elif REX_ARCH_AMD64
  std::copy_n(context.int_registers, 16, fault.x.begin());
#endif
  if (!core_.HandleFault(fault)) return false;
#if REX_ARCH_ARM64
  for (uint32_t i = 0; i < 31; ++i)
    if (fault.changed_x & (uint32_t(1) << i)) ex->ModifyXRegister(i) = fault.x[i];
  for (uint32_t i = 0; i < 32; ++i)
    if (fault.changed_v & (uint32_t(1) << i)) ex->ModifyVRegister(i).u32[0] = fault.v[i].u32[0];
  context.sp = fault.sp;
#elif REX_ARCH_AMD64
  for (uint32_t i = 0; i < 16; ++i)
    if (fault.changed_x & (uint32_t(1) << i)) ex->ModifyIntRegister(i) = fault.x[i];
#endif
  if (fault.resume_pc) ex->set_resume_pc(fault.resume_pc);
  return true;
}
}  // namespace rex::runtime
