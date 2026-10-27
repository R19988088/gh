/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/logging.h>
#include <rex/stream.h>
#include "gh_system_xevent.h"

#include "gh_kernel.h"

namespace rex::system {

XEvent::XEvent(KernelState* kernel_state) : XObject(kernel_state, kObjectType) {}

XEvent::~XEvent() = default;

gh::kernel::Waitable* XEvent::GetKernelWaitable() { return kernel_event_.get(); }

void XEvent::Initialize(bool manual_reset, bool initial_state) {
  assert_false(kernel_event_);
  manual_reset_ = manual_reset;
  CreateNative<X_KEVENT>();
  kernel_event_ = std::make_unique<gh::kernel::Event>(manual_reset_, initial_state);
}

void XEvent::InitializeNative(void* native_ptr, X_DISPATCH_HEADER* header) {
  assert_false(kernel_event_);
  switch (header->type) {
    case 0x00: manual_reset_ = true; break;
    case 0x01: manual_reset_ = false; break;
    default: assert_always(); return;
  }
  kernel_event_ = std::make_unique<gh::kernel::Event>(manual_reset_, bool(header->signal_state));
}

void XEvent::Query(uint32_t* out_type, uint32_t* out_state) {
  if (out_type) *out_type = manual_reset_ ? 0x00 : 0x01;
  if (out_state) *out_state = kernel_event_->signaled() ? 1 : 0;
}

int32_t XEvent::Set(uint32_t priority_increment, bool wait) {
  return kernel_event_->Set() ? 1 : 0;
}

int32_t XEvent::Pulse(uint32_t priority_increment, bool wait) {
  return kernel_event_->Pulse() ? 1 : 0;
}

int32_t XEvent::Reset() { return kernel_event_->Reset() ? 1 : 0; }

void XEvent::Clear() { kernel_event_->Reset(); }

bool XEvent::Save(stream::ByteStream* stream) {
  REXSYS_DEBUG("XEvent {:08X} ({})", handle(), manual_reset_ ? "manual" : "auto");
  SaveObject(stream);
  stream->Write<bool>(kernel_event_->signaled());
  stream->Write<bool>(manual_reset_);
  return true;
}

object_ref<XEvent> XEvent::Restore(KernelState* kernel_state, stream::ByteStream* stream) {
  auto evt = new XEvent(nullptr);
  evt->kernel_state_ = kernel_state;

  evt->RestoreObject(stream);
  bool signaled = stream->Read<bool>();
  evt->manual_reset_ = stream->Read<bool>();

  evt->kernel_event_ = std::make_unique<gh::kernel::Event>(evt->manual_reset_, signaled);

  return object_ref<XEvent>(evt);
}

}  // namespace rex::system
