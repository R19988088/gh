#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include "gh_system_xobject.h"
#include <rex/system/xtypes.h>
#include <rex/thread.h>

namespace rex::system {

class XThread;

}  // namespace rex::system

namespace gh::kernel {
class Waitable;
class Mutant;
}

namespace rex::system {

class XMutant : public XObject {
 public:
  static const XObject::Type kObjectType = XObject::Type::Mutant;

  explicit XMutant(KernelState* kernel_state);
  ~XMutant() override;

  void Initialize(bool initial_owner);
  void InitializeNative(void* native_ptr, X_DISPATCH_HEADER* header);

  X_STATUS ReleaseMutant(uint32_t priority_increment, bool abandon, bool wait);

  // Releases ownership during the owning thread's exit rundown.
  void AbandonOwnedBy(XThread* thread);

  bool Save(stream::ByteStream* stream) override;
  bool SaveContinuation(stream::ByteStream* stream);
  bool RestoreOwnershipOnCurrentThread(uint32_t recursion);
  static object_ref<XMutant> Restore(KernelState* kernel_state, stream::ByteStream* stream);
  static object_ref<XMutant> RestoreContinuation(KernelState*, stream::ByteStream*);

 protected:
  gh::kernel::Waitable* GetKernelWaitable() override;

 private:
  XMutant();

  std::unique_ptr<gh::kernel::Mutant> kernel_mutant_;
};

}  // namespace rex::system
