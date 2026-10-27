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
#include "gh_system_kernel_state.h"
#include "gh_system_xmutant.h"
#include "gh_system_xthread.h"

#include "gh_kernel.h"

namespace rex::system {

XMutant::XMutant(KernelState* kernel_state) : XObject(kernel_state, kObjectType) {}

XMutant::XMutant() : XObject(kObjectType) {}

XMutant::~XMutant() = default;

void XMutant::Initialize(bool initial_owner) {
  assert_false(kernel_mutant_);

  kernel_mutant_ = std::make_unique<gh::kernel::Mutant>(initial_owner);
}

void XMutant::InitializeNative(void* native_ptr, X_DISPATCH_HEADER* header) {
  assert_false(kernel_mutant_);

  // Haven't seen this yet, but it's possible.
  assert_always();
}

X_STATUS XMutant::ReleaseMutant(uint32_t priority_increment, bool abandon, bool wait) {
  if (abandon || !kernel_mutant_->Release()) return X_STATUS_MUTANT_NOT_OWNED;
  return X_STATUS_SUCCESS;
}

void XMutant::AbandonOwnedBy(XThread* thread) {
  if (thread && kernel_mutant_) kernel_mutant_->AbandonOwner(thread->handle());
}

gh::kernel::Waitable* XMutant::GetKernelWaitable() { return kernel_mutant_.get(); }

bool XMutant::Save(stream::ByteStream* stream) {
  if (!SaveObject(stream)) {
    return false;
  }

  uint32_t owning_thread_handle = static_cast<uint32_t>(kernel_mutant_->owner_token());
  stream->Write<uint32_t>(owning_thread_handle);
  REXSYS_DEBUG("XMutant {:08X} (owner: {:08X})", handle(), owning_thread_handle);

  return true;
}
bool XMutant::SaveContinuation(stream::ByteStream* stream) {
  const auto state = kernel_mutant_->SaveState();
  if (state.owner) {
    auto owner = kernel_state_->object_table()->LookupObject<XThread>(state.owner);
    if (!owner || !owner->is_guest_thread() || !owner->is_alive()) return false;
  }
  if (!SaveObject(stream)) return false;
  stream->Write<uint32_t>(static_cast<uint32_t>(state.owner));
  stream->Write(state.recursion);
  stream->Write(state.abandoned);
  return true;
}
bool XMutant::RestoreOwnershipOnCurrentThread(uint32_t recursion) {
  return kernel_mutant_->RestoreOwnershipOnCurrentThread(recursion);
}
object_ref<XMutant> XMutant::RestoreContinuation(KernelState* kernel_state,
                                               stream::ByteStream* stream) {
  auto mutant = object_ref<XMutant>(new XMutant());
  mutant->kernel_state_ = kernel_state;
  if (!mutant->RestoreObject(stream)) return nullptr;
  const auto owner_handle = stream->Read<uint32_t>();
  const auto recursion = stream->Read<uint32_t>();
  const auto abandoned = stream->Read<bool>();
  if ((owner_handle != 0) != (recursion != 0) || (abandoned && recursion)) return nullptr;
  mutant->Initialize(false);
  mutant->kernel_mutant_->RestoreUnowned(abandoned);
  if (owner_handle) {
    auto owner = kernel_state->object_table()->LookupObject<XThread>(owner_handle);
    if (!owner || !owner->has_restored_continuation()) return nullptr;
    owner->AcquireMutantOnStartup(mutant, recursion);
  }
  return mutant;
}

object_ref<XMutant> XMutant::Restore(KernelState* kernel_state, stream::ByteStream* stream) {
  auto mutant = object_ref<XMutant>(new XMutant());
  mutant->kernel_state_ = kernel_state;

  if (!mutant->RestoreObject(stream)) {
    return nullptr;
  }

  mutant->Initialize(false);

  auto owning_thread_handle = stream->Read<uint32_t>();
  if (owning_thread_handle) {
    auto owner = kernel_state->object_table()->LookupObject<XThread>(owning_thread_handle);
    if (!owner) return nullptr;
    owner->AcquireMutantOnStartup(mutant);
  }

  return mutant;
}

}  // namespace rex::system
