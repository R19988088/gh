#include "gh_guest_continuation_waits.h"
#include "gh_audio_state_io.h"
#include "gh_kernel.h"
#include "gh_system_kernel_state.h"
#include "gh_system_xthread.h"
#include <rex/chrono/clock.h>
#include <rex/thread.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace gh::guest::continuation {
namespace {
using namespace rex::system;
using rex::X_STATUS;
using IO = gh::audio::state_io::Reader;
enum class Import : uint32_t { Delay, KeSingle, NtSingle, KeMultiple, NtMultiple, SignalWait };

class Wait final : public PendingHost {
 public:
  Wait(Import import, GuestContext& context, uint8_t*, std::span<const uint8_t> saved)
      : thread_(XThread::GetCurrentThread()), kernel_(thread_->kernel_state()), import_(import) {
    try {
    if (!saved.empty()) { Restore(saved); return; }
    const std::array args{context.r3.u32, context.r4.u32, context.r5.u32, context.r6.u32,
                          context.r7.u32, context.r8.u32, context.r9.u32};
    const auto arg = [&](unsigned n) { return args[n]; };
    uint32_t timeout = 0;
    object_ref<XObject> signal;
    std::vector<object_ref<XObject>> objects;
    const auto lookup = [&](uint32_t value, bool native) {
      return native ? XObject::GetNativeObject<XObject>(kernel_, kernel_->memory()->TranslateVirtual(value))
                    : kernel_->object_table()->LookupObject<XObject>(value);
    };
    switch (import) {
      case Import::Delay: alertable_ = arg(1) != 0; timeout = arg(2); break;
      case Import::KeSingle:
        objects.push_back(lookup(arg(0), true)); alertable_ = arg(3) != 0; timeout = arg(4); break;
      case Import::NtSingle:
        objects.push_back(lookup(arg(0), false)); alertable_ = arg(2) != 0; timeout = arg(3); break;
      case Import::KeMultiple:
      case Import::NtMultiple: {
        const bool native = import == Import::KeMultiple;
        const auto count = arg(0), list = arg(1), type = arg(2);
        if (!count || type > 1) { Fail(X_STATUS_INVALID_PARAMETER); return; }
        all_ = type == 0;
        alertable_ = arg(native ? 5 : 4) != 0; timeout = arg(native ? 6 : 5);
        for (uint32_t i = 0; i < count; ++i)
          objects.push_back(lookup(rex::memory::load_and_swap<uint32_t>(
              kernel_->memory()->TranslateVirtual(list + i * 4)), native));
        break;
      }
      case Import::SignalWait:
        signal = lookup(arg(0), false); objects.push_back(lookup(arg(1), false));
        alertable_ = arg(2) != 0; timeout = arg(4);
        if (!signal) { Fail(X_STATUS_INVALID_HANDLE); return; }
        if (signal->type() != XObject::Type::Event && signal->type() != XObject::Type::Semaphore &&
            signal->type() != XObject::Type::Mutant) { Fail(X_STATUS_OBJECT_TYPE_MISMATCH); return; }
        signal_failure_ = signal->type() == XObject::Type::Mutant
            ? X_STATUS_MUTANT_NOT_OWNED : X_STATUS_SEMAPHORE_LIMIT_EXCEEDED;
        break;
    }
    for (size_t i = 0; i < objects.size(); ++i) {
      const auto& object = objects[i];
      if (!object) {
        Fail(import == Import::KeSingle ? X_STATUS_ABANDONED_WAIT_0 :
            import == Import::NtSingle || import == Import::SignalWait
            ? X_STATUS_INVALID_HANDLE : X_STATUS_INVALID_PARAMETER); return;
      }
      if (!object->GetKernelWaitable()) {
        Fail(import == Import::KeSingle || import == Import::NtSingle
            ? X_STATUS_SUCCESS : X_STATUS_OBJECT_TYPE_MISMATCH); return;
      }
      for (size_t j = 0; j < i; ++j)
        if (objects[j].get() == object.get()) { Fail(X_STATUS_INVALID_PARAMETER); return; }
    }
    if (import == Import::Delay && alertable_) DeliverAPCs(context);
    if (timeout) {
      finite_ = true;
      const auto interval = rex::memory::load_and_swap<int64_t>(kernel_->memory()->TranslateVirtual(timeout));
      std::chrono::nanoseconds duration;
      if (import == Import::Delay && interval < 0) {
        const auto ticks = uint64_t(-(interval + 1)) + 1;
        duration = std::chrono::milliseconds(rex::chrono::Clock::ScaleGuestDurationMillis(
            uint32_t(std::min<uint64_t>(ticks / 10000, UINT32_MAX))));
      } else {
        duration = gh::kernel::GuestTimeoutToDuration(
            rex::chrono::Clock::ScaleGuestDurationFileTime(interval),
            rex::chrono::Clock::QueryGuestSystemTime());
      }
      if (import == Import::Delay && !alertable_ && duration == std::chrono::nanoseconds::zero()) {
        Fail(thread_->Delay(arg(0), 0, uint64_t(interval))); return;
      }
      // Use the same scaled duration as native imports, on the restorable guest
      // tick timeline so time spent saving/loading does not consume the wait.
      const auto now = rex::chrono::Clock::QueryGuestTickCount();
      const auto ratio = rex::chrono::Clock::guest_tick_ratio();
      const long double ticks = static_cast<long double>(duration.count()) *
          rex::chrono::Clock::QueryHostTickFrequency() * ratio.first / ratio.second / 1.0e9L;
      deadline_ = ticks >= static_cast<long double>(UINT64_MAX - now)
          ? UINT64_MAX : now + uint64_t(std::ceil(ticks));
    }
    // Each wait owns an anonymous handle. Closing the original guest handle
    // must not remove an object still referenced by a serialized wait.
    for (const auto& object : objects) {
      rex::X_HANDLE handle = 0;
      if (kernel_->object_table()->AddHandle(object.get(), &handle) != X_STATUS_SUCCESS)
        throw std::runtime_error("cannot retain queued wait object");
      handles_.push_back(handle); objects_.push_back(object);
    }
    Register(signal ? signal->GetKernelWaitable() : nullptr, nullptr);
    } catch (...) {
      for (const auto handle : handles_) kernel_->object_table()->ReleaseHandle(handle);
      throw;
    }
  }
  ~Wait() override {
    wait_.reset();
    for (const auto handle : handles_) kernel_->object_table()->ReleaseHandle(handle);
  }
  bool Poll(GuestContext& context, uint8_t*) override {
    XThread::CheckTitleTermination();
    if (import_ == Import::Delay && alertable_ && gh::kernel::HostThread::DispatchUserCallbacks())
      Fail(X_STATUS_USER_APC);
    if (!error_) {
      const auto result = wait_->Poll();
      if (!result) return false;
      switch (result->first) {
        case gh::kernel::WaitResult::Signaled:
          status_ = all_ ? X_STATUS_SUCCESS : X_STATUS(result->second); break;
        case gh::kernel::WaitResult::Abandoned:
          status_ = X_STATUS(X_STATUS_ABANDONED_WAIT_0 + result->second); break;
        case gh::kernel::WaitResult::Alerted: status_ = X_STATUS_USER_APC; break;
        case gh::kernel::WaitResult::Timeout:
          status_ = import_ == Import::Delay ? X_STATUS_SUCCESS : X_STATUS_TIMEOUT;
          if (import_ != Import::Delay) rex::thread::MaybeYield();
          break;
        case gh::kernel::WaitResult::Failed: status_ = signal_failure_; break;
      }
    }
    // Guest APC code remains atomic and must return before the next safepoint.
    if (alertable_ && status_ == X_STATUS_USER_APC) DeliverAPCs(context);
    context.r3.u64 = status_;
    return true;
  }
  std::vector<uint8_t> Save() const override {
    gh::audio::state_io::Writer out;
    out.Put(uint32_t{1}); out.Put(import_); out.Put(alertable_); out.Put(all_); out.Put(finite_);
    out.Put(deadline_); out.Put(error_); out.Put(status_); out.Put(signal_failure_);
    out.Put(uint32_t(handles_.size())); for (const auto handle : handles_) out.Put(handle);
    const auto state = wait_ ? wait_->Save() : gh::kernel::QueuedWait::State{};
    out.Put(state.order); out.Put(state.result); out.Put(uint32_t(state.index)); out.Put(state.completed);
    return std::move(out.bytes);
  }
  void WaitForWake(std::chrono::milliseconds maximum) override {
    if (wait_) wait_->WaitForWake(maximum);
  }
 private:
  void DeliverAPCs(GuestContext& context) {
    auto* target = thread_->thread_state()->context();
    static_assert(sizeof(*target) == sizeof(context));
    std::memcpy(target, &context, sizeof(context));
    thread_->DeliverAPCs();
    std::memcpy(&context, target, sizeof(context));
  }
  void Fail(X_STATUS status) { error_ = true; status_ = status; }
  void Register(gh::kernel::Waitable* signal, const gh::kernel::QueuedWait::State* state) {
    std::vector<gh::kernel::Waitable*> objects;
    for (const auto& object : objects_) objects.push_back(object->GetKernelWaitable());
    if (import_ == Import::Delay) objects.push_back(&delay_);
    wait_ = std::make_unique<gh::kernel::QueuedWait>(objects, all_,
        [this] { return kernel_->is_terminating_title() || (alertable_ && thread_->HasDeliverableAPCs()); },
        [this] { return finite_ && rex::chrono::Clock::QueryGuestTickCount() >= deadline_; }, signal, state);
  }
  void Restore(std::span<const uint8_t> saved) {
    IO in{saved};
    if (in.Get<uint32_t>() != 1 || in.Get<Import>() != import_)
      throw std::invalid_argument("invalid queued import record");
    alertable_ = in.Get<bool>(); all_ = in.Get<bool>(); finite_ = in.Get<bool>();
    deadline_ = in.Get<uint64_t>(); error_ = in.Get<bool>(); status_ = in.Get<X_STATUS>();
    signal_failure_ = in.Get<X_STATUS>();
    const auto count = in.Get<uint32_t>();
    for (uint32_t i = 0; i < count; ++i) {
      const auto handle = in.Get<rex::X_HANDLE>();
      auto object = kernel_->object_table()->LookupObject<XObject>(handle);
      if (!object || !object->GetKernelWaitable()) throw std::invalid_argument("missing queued wait object");
      handles_.push_back(handle); objects_.push_back(std::move(object));
    }
    gh::kernel::QueuedWait::State state;
    state.order = in.Get<uint64_t>(); state.result = in.Get<gh::kernel::WaitResult>();
    state.index = in.Get<uint32_t>(); state.completed = in.Get<bool>();
    if (!in.bytes.empty() || (!error_ && !state.order)) throw std::invalid_argument("invalid queued wait state");
    if (!error_) Register(nullptr, &state);
  }
  XThread* thread_;
  KernelState* kernel_;
  Import import_;
  bool alertable_ = false, all_ = false, finite_ = false, error_ = false;
  uint64_t deadline_ = 0;
  X_STATUS status_ = X_STATUS_SUCCESS, signal_failure_ = X_STATUS_UNSUCCESSFUL;
  std::vector<rex::X_HANDLE> handles_;
  std::vector<object_ref<XObject>> objects_;
  gh::kernel::Event delay_{false};
  std::unique_ptr<gh::kernel::QueuedWait> wait_;
};
}
#define GH_BEGIN_WAIT(name, kind) \
  std::unique_ptr<PendingHost> name(GuestContext& context, uint8_t* memory, std::span<const uint8_t> state) { \
    return std::make_unique<Wait>(Import::kind, context, memory, state); \
  }
GH_BEGIN_WAIT(BeginKeDelay, Delay)
GH_BEGIN_WAIT(BeginKeWaitSingle, KeSingle)
GH_BEGIN_WAIT(BeginNtWaitSingle, NtSingle)
GH_BEGIN_WAIT(BeginKeWaitMultiple, KeMultiple)
GH_BEGIN_WAIT(BeginNtWaitMultiple, NtMultiple)
GH_BEGIN_WAIT(BeginSignalAndWait, SignalWait)
#undef GH_BEGIN_WAIT
}  // namespace gh::guest::continuation
