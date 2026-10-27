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
#include "gh_system_xsemaphore.h"

#include "gh_kernel.h"

namespace rex::system {

XSemaphore::XSemaphore(KernelState* kernel_state) : XObject(kernel_state, kObjectType) {}

XSemaphore::~XSemaphore() = default;

gh::kernel::Waitable* XSemaphore::GetKernelWaitable() { return kernel_semaphore_.get(); }

bool XSemaphore::Initialize(int32_t initial_count, int32_t maximum_count) {
  assert_false(kernel_semaphore_);
  if (initial_count < 0 || maximum_count <= 0 || initial_count > maximum_count) return false;

  CreateNative(sizeof(X_KSEMAPHORE));

  maximum_count_ = maximum_count;
  kernel_semaphore_ = std::make_unique<gh::kernel::Semaphore>(initial_count, maximum_count);
  return true;
}

bool XSemaphore::InitializeNative(void* native_ptr, X_DISPATCH_HEADER* header) {
  assert_false(kernel_semaphore_);

  auto semaphore = reinterpret_cast<X_KSEMAPHORE*>(native_ptr);
  if (semaphore->header.signal_state < 0 || semaphore->limit == 0 ||
      semaphore->header.signal_state > semaphore->limit) {
    return false;
  }
  maximum_count_ = semaphore->limit;
  kernel_semaphore_ = std::make_unique<gh::kernel::Semaphore>(
      semaphore->header.signal_state, semaphore->limit);
  return true;
}

bool XSemaphore::ReleaseSemaphore(int32_t release_count, int32_t* out_previous_count) {
  if (release_count < 0) return false;
  uint32_t previous = 0;
  if (!kernel_semaphore_->Release(static_cast<uint32_t>(release_count), &previous)) return false;
  if (out_previous_count) *out_previous_count = static_cast<int32_t>(previous);
  return true;
}

bool XSemaphore::Save(stream::ByteStream* stream) {
  if (!SaveObject(stream)) return false;
  stream->Write(maximum_count_);
  stream->Write(kernel_semaphore_->count());
  return true;
}

object_ref<XSemaphore> XSemaphore::Restore(KernelState* kernel_state, stream::ByteStream* stream) {
  auto sem = new XSemaphore(nullptr);
  sem->kernel_state_ = kernel_state;

  if (!sem->RestoreObject(stream)) {
    return nullptr;
  }

  sem->maximum_count_ = stream->Read<uint32_t>();
  auto free_count = stream->Read<uint32_t>();
  REXSYS_DEBUG("XSemaphore {:08X} (count {}/{})", sem->handle(), free_count, sem->maximum_count_);

  sem->kernel_semaphore_ =
      std::make_unique<gh::kernel::Semaphore>(free_count, sem->maximum_count_);

  return object_ref<XSemaphore>(sem);
}

}  // namespace rex::system
