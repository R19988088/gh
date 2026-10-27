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

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>
#include <vector>

#include <rex/chrono/clock.h>
#include <rex/stream.h>
#include "gh_system_kernel_state.h"
#include "gh_system_util_string_utils.h"  // For TranslateAnsiStringAddress
#include "gh_system_xobject.h"
// #include <rex/kernel/xboxkrnl/private.h>  // TODO: JIT only
#include "gh_system_xenumerator.h"
#include "gh_system_xevent.h"
#include "gh_system_xfile.h"
#include "gh_system_xmodule.h"
#include "gh_system_xmutant.h"
#include "gh_system_xnotifylistener.h"
#include "gh_system_xsemaphore.h"
#include "gh_system_xsymboliclink.h"
#include "gh_system_xthread.h"
#include "gh_system_xtimer.h"

#include "gh_kernel.h"

namespace rex::system {
namespace {

std::chrono::nanoseconds GuestTimeoutDuration(uint64_t* opt_timeout) {
  if (!opt_timeout) return std::chrono::nanoseconds::max();
  const auto scaled_timeout = chrono::Clock::ScaleGuestDurationFileTime(
      static_cast<int64_t>(*opt_timeout));
  return gh::kernel::GuestTimeoutToDuration(
      scaled_timeout, chrono::Clock::QueryGuestSystemTime());
}

X_STATUS WaitProjectObjects(std::span<gh::kernel::Waitable* const> waitables,
                            bool wait_all, bool alertable, uint64_t* opt_timeout,
                            gh::kernel::Waitable* signal = nullptr,
                            X_STATUS signal_failure = X_STATUS_UNSUCCESSFUL) {
  XThread::CheckTitleTermination();
  const auto interrupted = [alertable] {
    if (!XThread::IsInThread()) return false;
    auto* thread = XThread::GetCurrentThread();
    return (thread->is_guest_thread() && thread->kernel_state()->is_terminating_title()) ||
           (alertable && thread->HasDeliverableAPCs());
  };
  std::pair<gh::kernel::WaitResult, size_t> result;
  try {
    result = gh::kernel::WaitMultiple(waitables, wait_all, GuestTimeoutDuration(opt_timeout),
                                      interrupted, signal);
  } catch (const std::invalid_argument&) {
    return X_STATUS_INVALID_PARAMETER;
  }
  XThread::CheckTitleTermination();
  switch (result.first) {
    case gh::kernel::WaitResult::Signaled:
      return wait_all ? X_STATUS_SUCCESS : X_STATUS(result.second);
    case gh::kernel::WaitResult::Abandoned:
      return X_STATUS(X_STATUS_ABANDONED_WAIT_0 + result.second);
    case gh::kernel::WaitResult::Alerted:
      return X_STATUS_USER_APC;
    case gh::kernel::WaitResult::Timeout:
      rex::thread::MaybeYield();
      return X_STATUS_TIMEOUT;
    case gh::kernel::WaitResult::Failed:
      return signal_failure;
  }
  return X_STATUS_UNSUCCESSFUL;
}

}  // namespace

XObject::XObject(Type type) : kernel_state_(nullptr), pointer_ref_count_(1), type_(type) {
  handles_.reserve(10);
}

XObject::XObject(KernelState* kernel_state, Type type)
    : kernel_state_(kernel_state),
      type_(type),
      pointer_ref_count_(1),
      guest_object_ptr_(0),
      allocated_guest_object_(false) {
  handles_.reserve(10);

  // TODO: Assert kernel_state != nullptr in this constructor.
  if (kernel_state) {
    kernel_state->object_table()->AddHandle(this, nullptr);
  }
}

XObject::~XObject() {
  // TODO: these are being asserted true... find out why
  // assert_true(handles_.empty());
  // assert_zero(pointer_ref_count_);

  if (allocated_guest_object_) {
    uint32_t ptr = guest_object_ptr_ - sizeof(X_OBJECT_HEADER);
    auto header = memory()->TranslateVirtual<X_OBJECT_HEADER*>(ptr);

    // Free the object creation info
    if (header->object_type_ptr) {
      memory()->SystemHeapFree(header->object_type_ptr);
    }

    memory()->SystemHeapFree(ptr);
  }
}

rex::Runtime* XObject::emulator() const {
  return kernel_state_->emulator_;
}
KernelState* XObject::kernel_state() const {
  return kernel_state_;
}
rex::memory::Memory* XObject::memory() const {
  return kernel_state_->memory();
}

XObject::Type XObject::type() const {
  return type_;
}

void XObject::RetainHandle() {
  auto self = retain_object(this);
  auto lock = rex::thread::global_critical_region::AcquireDirect();
  if (!handles_.empty()) {
    kernel_state_->object_table()->RetainHandle(handles_.front());
  }
}

bool XObject::ReleaseHandle() {
  // FIXME: Return true when handle is actually released.
  auto self = retain_object(this);
  auto lock = rex::thread::global_critical_region::AcquireDirect();
  return !handles_.empty() &&
         kernel_state_->object_table()->ReleaseHandle(handles_.front()) == X_STATUS_SUCCESS;
}

void XObject::Retain() {
  ++pointer_ref_count_;
}

void XObject::Release() {
  if (--pointer_ref_count_ == 0) {
    delete this;
  }
}

X_STATUS XObject::Delete() {
  if (kernel_state_ == nullptr) {
    // Fake return value for api-scanner
    return X_STATUS_SUCCESS;
  } else {
    if (!name_.empty()) {
      kernel_state_->object_table()->RemoveNameMapping(name_);
    }
    return kernel_state_->object_table()->RemoveHandle(handles_[0]);
  }
}

bool XObject::SaveObject(stream::ByteStream* stream) {
  stream->Write<uint32_t>(allocated_guest_object_);
  stream->Write<uint32_t>(guest_object_ptr_);

  stream->Write(uint32_t(handles_.size()));
  if (!handles_.empty()) {
    stream->Write(handles_.data(), handles_.size() * sizeof(X_HANDLE));
  }

  return true;
}

bool XObject::RestoreObject(stream::ByteStream* stream) {
  allocated_guest_object_ = stream->Read<uint32_t>() > 0;
  guest_object_ptr_ = stream->Read<uint32_t>();

  handles_.resize(stream->Read<uint32_t>());
  if (!handles_.empty()) {
    stream->Read(handles_.data(), handles_.size() * sizeof(X_HANDLE));
  }

  // Restore our pointer to our handles in the object table.
  for (size_t i = 0; i < handles_.size(); i++) {
    kernel_state_->object_table()->RestoreHandle(handles_[i], this);
  }

  return true;
}
void XObject::RestoreName(std::string name) {
  name_ = std::move(name);
  if (!name_.empty() && !handles_.empty())
    kernel_state_->object_table()->AddNameMapping(name_, handles_.front());
}

object_ref<XObject> XObject::Restore(KernelState* kernel_state, Type type,
                                     stream::ByteStream* stream) {
  switch (type) {
    case Type::Enumerator:
      break;
    case Type::Event:
      return XEvent::Restore(kernel_state, stream);
    case Type::File:
      return XFile::Restore(kernel_state, stream);
    case Type::IOCompletion:
      break;
    case Type::Module:
      return XModule::Restore(kernel_state, stream);
    case Type::Mutant:
      return XMutant::Restore(kernel_state, stream);
    case Type::NotifyListener:
      return XNotifyListener::Restore(kernel_state, stream);
    case Type::Semaphore:
      return XSemaphore::Restore(kernel_state, stream);
    case Type::Session:
      break;
    case Type::Socket:
      break;
    case Type::SymbolicLink:
      return XSymbolicLink::Restore(kernel_state, stream);
    case Type::Thread:
      return XThread::Restore(kernel_state, stream);
    case Type::Timer:
      return XTimer::Restore(kernel_state, stream);
    case Type::Undefined:
      break;
  }

  assert_always("No restore handler exists for this object!");
  return nullptr;
}

void XObject::SetAttributes(uint32_t obj_attributes_ptr) {
  if (!obj_attributes_ptr) {
    return;
  }

  auto name = util::TranslateAnsiStringAddress(
      memory(),
      memory::load_and_swap<uint32_t>(memory()->TranslateVirtual(obj_attributes_ptr + 4)));
  if (!name.empty()) {
    name_ = std::string(name);
    kernel_state_->object_table()->AddNameMapping(name_, handles_[0]);
  }
}

uint32_t XObject::TimeoutTicksToMs(int64_t timeout_ticks) {
  const auto scaled_timeout = chrono::Clock::ScaleGuestDurationFileTime(timeout_ticks);
  const auto duration = gh::kernel::GuestTimeoutToDuration(
      scaled_timeout, chrono::Clock::QueryGuestSystemTime());
  const auto nanoseconds = static_cast<uint64_t>(duration.count());
  const auto milliseconds = nanoseconds / 1'000'000 + (nanoseconds % 1'000'000 != 0);
  return static_cast<uint32_t>(std::min<uint64_t>(
      milliseconds, std::numeric_limits<uint32_t>::max()));
}

X_STATUS XObject::Wait(uint32_t wait_reason, uint32_t processor_mode, uint32_t alertable,
                       uint64_t* opt_timeout) {
  auto* waitable = GetKernelWaitable();
  if (!waitable) return X_STATUS_SUCCESS;
  gh::kernel::Waitable* waitables[] = {waitable};
  return WaitProjectObjects(waitables, false, alertable != 0, opt_timeout);
}

X_STATUS XObject::SignalAndWait(XObject* signal_object, XObject* wait_object, uint32_t wait_reason,
                                uint32_t processor_mode, uint32_t alertable,
                                uint64_t* opt_timeout) {
  const auto type = signal_object->type();
  if (type != Type::Event && type != Type::Semaphore && type != Type::Mutant)
    return X_STATUS_OBJECT_TYPE_MISMATCH;
  auto* signal = signal_object->GetKernelWaitable();
  auto* wait = wait_object->GetKernelWaitable();
  if (!signal || !wait) return X_STATUS_OBJECT_TYPE_MISMATCH;
  gh::kernel::Waitable* waitables[] = {wait};
  return WaitProjectObjects(waitables, false, alertable != 0, opt_timeout, signal,
      type == Type::Mutant ? X_STATUS_MUTANT_NOT_OWNED : X_STATUS_SEMAPHORE_LIMIT_EXCEEDED);
}

X_STATUS XObject::WaitMultiple(uint32_t count, XObject** objects, uint32_t wait_type,
                               uint32_t wait_reason, uint32_t processor_mode, uint32_t alertable,
                               uint64_t* opt_timeout) {
  if (!count || wait_type > 1) return X_STATUS_INVALID_PARAMETER;
  std::vector<gh::kernel::Waitable*> waitables;
  waitables.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    auto* waitable = objects[i]->GetKernelWaitable();
    if (!waitable) return X_STATUS_OBJECT_TYPE_MISMATCH;
    waitables.push_back(waitable);
  }
  return WaitProjectObjects(waitables, wait_type == 0, alertable != 0, opt_timeout);
}

uint8_t* XObject::CreateNative(uint32_t size) {
  auto global_lock = rex::thread::global_critical_region::AcquireDirect();

  uint32_t total_size = size + sizeof(X_OBJECT_HEADER);

  auto mem = memory()->SystemHeapAlloc(total_size);
  if (!mem) {
    // Out of memory!
    return nullptr;
  }

  allocated_guest_object_ = true;
  memory()->Zero(mem, total_size);
  SetNativePointer(mem + sizeof(X_OBJECT_HEADER), true);

  auto header = memory()->TranslateVirtual<X_OBJECT_HEADER*>(mem);

  auto object_type = memory()->SystemHeapAlloc(sizeof(X_OBJECT_TYPE));
  if (object_type) {
    // Set it up in the header.
    // Some kernel method is accessing this struct and dereferencing a member
    // @ offset 0x14
    header->object_type_ptr = object_type;
  }

  return memory()->TranslateVirtual(guest_object_ptr_);
}

void XObject::SetNativePointer(uint32_t native_ptr, bool uninitialized) {
  auto global_lock = rex::thread::global_critical_region::AcquireDirect();

  // If hit: We've already setup the native ptr with CreateNative!
  assert_zero(guest_object_ptr_);

  auto header = kernel_state_->memory()->TranslateVirtual<X_DISPATCH_HEADER*>(native_ptr);

  // Memory uninitialized, so don't bother with the check.
  if (!uninitialized) {
    assert_true(!(header->wait_list_blink & 0x1));
  }

  // Stash pointer in struct.
  // FIXME: This assumes the object has a dispatch header (some don't!)
  StashHandle(header, handle());

  guest_object_ptr_ = native_ptr;
}

object_ref<XObject> XObject::GetNativeObject(KernelState* kernel_state, void* native_ptr,
                                             int32_t as_type) {
  assert_not_null(native_ptr);

  // Unfortunately the XDK seems to inline some KeInitialize calls, meaning
  // we never see it and just randomly start getting passed events/timers/etc.
  // Luckily it seems like all other calls (Set/Reset/Wait/etc) are used and
  // we don't have to worry about PPC code poking the struct. Because of that,
  // we init on first use, store our handle in the struct, and dereference it
  // each time.
  // We identify this by setting wait_list_flink to a magic value. When set,
  // wait_list_blink will hold a handle to our object.

  auto global_lock = rex::thread::global_critical_region::AcquireDirect();

  auto header = reinterpret_cast<X_DISPATCH_HEADER*>(native_ptr);

  if (as_type == -1) {
    as_type = header->type;
  }

  if (header->wait_list_flink == kXObjSignature) {
    // Already initialized.
    // TODO: assert if the type of the object != as_type
    uint32_t handle = header->wait_list_blink;
    auto object = kernel_state->object_table()->LookupObject<XObject>(handle);

    // TODO(benvanik): assert nothing has been changed in the struct.
    return object;
  } else {
    // First use, create new.
    // https://www.nirsoft.net/kernel_struct/vista/KOBJECTS.html
    XObject* object = nullptr;
    switch (as_type) {
      case 0:  // EventNotificationObject
      case 1:  // EventSynchronizationObject
      {
        auto ev = new XEvent(kernel_state);
        ev->InitializeNative(native_ptr, header);
        object = ev;
      } break;
      case 2:  // MutantObject
      {
        auto mutant = new XMutant(kernel_state);
        mutant->InitializeNative(native_ptr, header);
        object = mutant;
      } break;
      case 5:  // SemaphoreObject
      {
        auto sem = new XSemaphore(kernel_state);
        auto success = sem->InitializeNative(native_ptr, header);
        // Can't report failure to the guest at late initialization:
        assert_true(success);
        object = sem;
      } break;
      case 3:   // ProcessObject
      case 4:   // QueueObject
      case 6:   // ThreadObject
      case 7:   // GateObject
      case 8:   // TimerNotificationObject
      case 9:   // TimerSynchronizationObject
      case 18:  // ApcObject
      case 19:  // DpcObject
      case 20:  // DeviceQueueObject
      case 21:  // EventPairObject
      case 22:  // InterruptObject
      case 23:  // ProfileObject
      case 24:  // ThreadedDpcObject
      default:
        assert_always();
        return NULL;
    }

    // Stash pointer in struct.
    // FIXME: This assumes the object contains a dispatch header (some don't!)
    StashHandle(header, object->handle());

    return object_ref<XObject>(object);
  }
}

}  // namespace rex::system
