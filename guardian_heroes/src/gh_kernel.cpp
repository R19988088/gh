#include "gh_kernel.h"
#include "native_memory.h"

#include <algorithm>
#include <atomic>
#include <deque>
#include <list>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__SWITCH__)
#include <cerrno>
#include <csignal>
#include <semaphore.h>
#include <unistd.h>
#else
#include <pthread.h>
#include <sched.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#else
#include <cerrno>
#include <csignal>
#include <semaphore.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
#endif

namespace gh::kernel {
namespace {
thread_local std::uintptr_t thread_local_value = 0;
std::mutex wait_mutex;
std::condition_variable wait_condition;

struct CriticalState {
  std::mutex mutex;
  std::condition_variable condition;
  std::uint32_t owner = 0;
  std::uint32_t recursion = 0;
  bool cancelled = false;
};
struct PendingWait {
  std::span<Waitable* const> waitables;
  std::function<bool()> complete;
  uint64_t order = 0;
  bool persistent = false;
  std::function<bool()> ready = {};
};
std::list<PendingWait*> pending_waits;
uint64_t next_wait_order = 0;
void CompleteQueuedWaits() {
  for (auto* pending : pending_waits)
    if (pending->persistent) pending->complete();
}

std::atomic_ref<std::int32_t> GuestLockCount(const GuestMemory& memory,
                                          std::uint32_t address) {
  // ReX's existing guest layout stores only lock_count in host byte order.
  auto* bytes = memory.Address(address, 28);
  return std::atomic_ref<std::int32_t>(*reinterpret_cast<std::int32_t*>(bytes + 16));
}

void UpdateGuestCriticalOwner(const GuestMemory& memory, std::uint32_t address,
                              const CriticalState& state, std::uint32_t guest_thread) {
  memory.Write32(address + 20, state.recursion);
  memory.Write32(address + 24, state.recursion ? guest_thread : 0);
}
}  // namespace

struct CriticalSections::Impl {
  std::mutex mutex;
  std::unordered_map<std::uint32_t, std::shared_ptr<CriticalState>> sections;
  bool cancelled = false;
  bool restore_guest_ownership = true;

  std::shared_ptr<CriticalState> Get(const GuestMemory& memory, std::uint32_t address,
                                   bool restore_owner = true) {
    std::lock_guard lock(mutex);
    auto& state = sections[address];
    if (!state) {
      state = std::make_shared<CriticalState>();
      state->cancelled = cancelled;
      if (!cancelled) {
        state->recursion = restore_owner && restore_guest_ownership
                               ? memory.Read32(address + 20) : 0;
        state->owner = state->recursion ? memory.Read32(address + 24) : 0;
        // Saved waiters have no host wait stack in this incarnation.
        GuestLockCount(memory, address).store(
            static_cast<std::int32_t>(state->recursion) - 1);
      }
    }
    return state;
  }
};

CriticalSections::CriticalSections() : impl_(std::make_unique<Impl>()) {}
CriticalSections::~CriticalSections() = default;

bool CriticalSections::IsOwnedBy(std::uint32_t guest_thread) const {
  std::lock_guard table_lock(impl_->mutex);
  for (const auto& [address, state] : impl_->sections) {
    std::lock_guard lock(state->mutex);
    if (state->recursion && state->owner == guest_thread) return true;
  }
  return false;
}

void CriticalSections::CancelWaits() {
  std::lock_guard table_lock(impl_->mutex);
  impl_->cancelled = true;
  for (auto& [address, state] : impl_->sections) {
    std::lock_guard lock(state->mutex);
    state->cancelled = true;
    state->condition.notify_all();
  }
}

void CriticalSections::Reset(bool restore_guest_ownership) {
  CancelWaits();
  std::lock_guard lock(impl_->mutex);
  impl_->sections.clear();
  impl_->cancelled = false;
  impl_->restore_guest_ownership = restore_guest_ownership;
}

std::pair<WaitResult, std::size_t> WaitMultiple(
    std::span<Waitable* const> waitables, bool wait_all,
    std::chrono::nanoseconds timeout,
    const std::function<bool()>& alert_pending, Waitable* signal_before_wait) {
  if (waitables.empty()) throw std::invalid_argument("wait set is empty");
  for (std::size_t i = 0; i < waitables.size(); ++i) {
    if (waitables[i] == nullptr) throw std::invalid_argument("wait set contains null");
    for (std::size_t j = 0; j < i; ++j) {
      if (waitables[i] == waitables[j])
        throw std::invalid_argument("wait set contains duplicate objects");
    }
  }

  std::unique_lock lock(wait_mutex);
  if (signal_before_wait) {
    if (!signal_before_wait->SignalForWait()) return {WaitResult::Failed, 0};
    CompleteQueuedWaits();
    wait_condition.notify_all();
  }
  bool completed = false;
  std::pair<WaitResult, std::size_t> result{WaitResult::Timeout, 0};
  const auto waiter = std::this_thread::get_id();
  const auto token = ThreadLocalValue();
  const auto complete = [&](bool check_alert) {
    if (completed) return true;
    if (check_alert && alert_pending && alert_pending()) {
      result = {WaitResult::Alerted, 0};
      return completed = true;
    }
    if (wait_all) {
      for (auto* waitable : waitables) {
        if (!waitable->IsSignaled(waiter)) return false;
      }
      result = {WaitResult::Signaled, 0};
      for (std::size_t i = 0; i < waitables.size(); ++i) {
        if (waitables[i]->IsAbandoned() && result.first != WaitResult::Abandoned)
          result = {WaitResult::Abandoned, i};
        waitables[i]->ConsumeSignal(waiter, token);
        waitables[i]->ClearAbandoned();
      }
      return completed = true;
    }
    for (std::size_t i = 0; i < waitables.size(); ++i) {
      if (!waitables[i]->IsSignaled(waiter)) continue;
      result = {waitables[i]->IsAbandoned() ? WaitResult::Abandoned
                                           : WaitResult::Signaled, i};
      waitables[i]->ConsumeSignal(waiter, token);
      waitables[i]->ClearAbandoned();
      return completed = true;
    }
    return false;
  };
  if (complete(true) || timeout <= std::chrono::nanoseconds::zero()) return result;

  PendingWait pending{waitables, [&] { return complete(false); }, ++next_wait_order};
  pending.ready = [&] {
    if (completed) return false;
    return wait_all ? std::all_of(waitables.begin(), waitables.end(),
        [&](auto* object) { return object->IsSignaled(waiter); })
        : std::any_of(waitables.begin(), waitables.end(),
        [&](auto* object) { return object->IsSignaled(waiter); });
  };
  const auto registration = pending_waits.insert(pending_waits.end(), &pending);
  struct Unregister {
    std::list<PendingWait*>::iterator registration;
    ~Unregister() {
      pending_waits.erase(registration);
      CompleteQueuedWaits();
      wait_condition.notify_all();
    }
  } unregister{registration};

  if (timeout == std::chrono::nanoseconds::max()) {
    wait_condition.wait(lock, [&] { return complete(true); });
  } else {
    const auto now = std::chrono::steady_clock::now();
    const auto remaining = std::chrono::steady_clock::time_point::max() - now;
    const auto deadline = timeout >= remaining
                              ? std::chrono::steady_clock::time_point::max()
                              : now + timeout;
    wait_condition.wait_until(lock, deadline, [&] { return complete(true); });
  }
  return result;
}

void NotifyWaiters() noexcept {
  std::lock_guard lock(wait_mutex);
  CompleteQueuedWaits();
  wait_condition.notify_all();
}

struct QueuedWait::Impl {
  std::vector<Waitable*> objects;
  PendingWait pending;
  State state;
  std::thread::id owner = std::this_thread::get_id();
  uintptr_t token = ThreadLocalValue();
  bool all;
  std::function<bool()> alert, expired;
  std::list<PendingWait*>::iterator position;
  bool registered = false;
};

QueuedWait::QueuedWait(std::span<Waitable* const> objects, bool all,
    std::function<bool()> alert, std::function<bool()> expired, Waitable* signal,
    const State* restored) : impl_(std::make_unique<Impl>()) {
  auto& self = *impl_;
  if (objects.empty()) throw std::invalid_argument("wait set is empty");
  self.objects.assign(objects.begin(), objects.end()); self.all = all;
  for (size_t i = 0; i < objects.size(); ++i) {
    if (!objects[i]) throw std::invalid_argument("wait set contains null");
    for (size_t j = 0; j < i; ++j)
      if (objects[i] == objects[j]) throw std::invalid_argument("duplicate wait object");
  }
  self.alert = std::move(alert); self.expired = std::move(expired);
  self.pending.waitables = self.objects;
  self.pending.persistent = true;
  self.pending.complete = [&self] {
    if (self.state.completed) return true;
    if (self.alert && self.alert()) {
      self.state.result = WaitResult::Alerted; return self.state.completed = true;
    }
    if (self.expired && self.expired()) {
      self.state.result = WaitResult::Timeout; return self.state.completed = true;
    }
    // Let an older eligible native waiter run its alert check on its own host.
    // Its unregister then commits queued successors with any remaining signal.
    for (auto* earlier : pending_waits) {
      if (earlier->order >= self.state.order) break;
      if (earlier->persistent || !earlier->ready || !earlier->ready()) continue;
      for (auto* object : earlier->waitables)
        if (std::find(self.objects.begin(), self.objects.end(), object) != self.objects.end())
          return false;
    }
    if (self.all) {
      for (auto* object : self.objects) if (!object->IsSignaled(self.owner)) return false;
      self.state.result = WaitResult::Signaled;
      for (size_t i = 0; i < self.objects.size(); ++i) {
        auto* object = self.objects[i];
        if (object->IsAbandoned() && self.state.result != WaitResult::Abandoned) {
          self.state.result = WaitResult::Abandoned; self.state.index = i;
        }
        object->ConsumeSignal(self.owner, self.token); object->ClearAbandoned();
      }
      return self.state.completed = true;
    }
    for (size_t i = 0; i < self.objects.size(); ++i) {
      auto* object = self.objects[i];
      if (!object->IsSignaled(self.owner)) continue;
      self.state.result = object->IsAbandoned() ? WaitResult::Abandoned : WaitResult::Signaled;
      self.state.index = i;
      object->ConsumeSignal(self.owner, self.token); object->ClearAbandoned();
      return self.state.completed = true;
    }
    return false;
  };
  std::lock_guard lock(wait_mutex);
  self.state = restored ? *restored : State{++next_wait_order};
  if (!self.state.order || self.state.index >= objects.size())
    throw std::invalid_argument("invalid queued wait state");
  next_wait_order = std::max(next_wait_order, self.state.order);
  self.pending.order = self.state.order;
  if (!restored) {
    if (signal && !signal->SignalForWait()) {
      self.state.result = WaitResult::Failed; self.state.completed = true;
    } else {
      if (signal) { CompleteQueuedWaits(); wait_condition.notify_all(); }
      // Zero-time waits still inspect objects once before reporting timeout.
      auto expiration = std::move(self.expired);
      self.expired = {};
      self.pending.complete(); self.expired = std::move(expiration);
      if (!self.state.completed && self.expired && self.expired()) {
        self.state.result = WaitResult::Timeout; self.state.completed = true;
      }
    }
  }
  self.position = pending_waits.insert(std::find_if(pending_waits.begin(), pending_waits.end(),
      [&](const auto* pending) { return pending->order > self.state.order; }), &self.pending);
  self.registered = true;
}
QueuedWait::~QueuedWait() {
  std::lock_guard lock(wait_mutex);
  if (impl_->registered) pending_waits.erase(impl_->position);
}
std::optional<std::pair<WaitResult, size_t>> QueuedWait::Poll() {
  std::lock_guard lock(wait_mutex);
  if (!impl_->pending.complete()) return std::nullopt;
  return std::pair{impl_->state.result, impl_->state.index};
}
QueuedWait::State QueuedWait::Save() const {
  std::lock_guard lock(wait_mutex); return impl_->state;
}
void QueuedWait::WaitForWake(std::chrono::milliseconds maximum) {
  std::unique_lock lock(wait_mutex);
  if (!impl_->state.completed) wait_condition.wait_for(lock, maximum);
}

void CriticalSections::Initialize(const GuestMemory& memory, std::uint32_t address,
                               std::uint32_t spin_count) {
  auto count = GuestLockCount(memory, address);
  auto state = impl_->Get(memory, address, false);
  std::lock_guard lock(state->mutex);
  if (state->cancelled) return;
  if (state->recursion != 0)
    throw std::logic_error("cannot initialize an owned critical section");
  memory.Write8(address, 1);
  memory.Write8(address + 1, static_cast<std::uint8_t>(
      std::min<std::uint64_t>((std::uint64_t{spin_count} + 255) >> 8, 255)));
  memory.Write32(address + 4, 0);
  count.store(-1);
  UpdateGuestCriticalOwner(memory, address, *state, 0);
}

bool CriticalSections::Enter(const GuestMemory& memory, std::uint32_t address,
                          std::uint32_t guest_thread) {
  auto count = GuestLockCount(memory, address);
  auto state = impl_->Get(memory, address);
  std::unique_lock lock(state->mutex);
  if (state->cancelled) return false;
  count.fetch_add(1);
  state->condition.wait(lock, [&] {
    return state->cancelled || state->recursion == 0 || state->owner == guest_thread;
  });
  if (state->cancelled) {
    count.fetch_sub(1);
    return false;
  }
  state->owner = guest_thread;
  ++state->recursion;
  UpdateGuestCriticalOwner(memory, address, *state, guest_thread);
  return true;
}

bool CriticalSections::TryEnter(const GuestMemory& memory, std::uint32_t address,
                             std::uint32_t guest_thread) {
  auto count = GuestLockCount(memory, address);
  auto state = impl_->Get(memory, address);
  std::lock_guard lock(state->mutex);
  if (state->cancelled || (state->recursion != 0 && state->owner != guest_thread)) return false;
  state->owner = guest_thread;
  ++state->recursion;
  count.fetch_add(1);
  UpdateGuestCriticalOwner(memory, address, *state, guest_thread);
  return true;
}

bool CriticalSections::Leave(const GuestMemory& memory, std::uint32_t address,
                          std::uint32_t guest_thread) {
  auto count = GuestLockCount(memory, address);
  auto state = impl_->Get(memory, address);
  std::lock_guard lock(state->mutex);
  if (state->cancelled || state->recursion == 0 || state->owner != guest_thread ||
      memory.Read32(address + 24) != guest_thread) return false;
  --state->recursion;
  UpdateGuestCriticalOwner(memory, address, *state, guest_thread);
  count.fetch_sub(1);
  if (state->recursion == 0) {
    state->owner = 0;
    state->condition.notify_one();
  }
  return true;
}

std::chrono::nanoseconds GuestTimeoutToDuration(
    std::int64_t timeout_100ns, std::uint64_t guest_now_100ns) noexcept {
  std::uint64_t duration_100ns = 0;
  if (timeout_100ns < 0) {
    duration_100ns = static_cast<std::uint64_t>(-(timeout_100ns + 1)) + 1;
  } else if (static_cast<std::uint64_t>(timeout_100ns) > guest_now_100ns) {
    duration_100ns = static_cast<std::uint64_t>(timeout_100ns) - guest_now_100ns;
  }

  constexpr auto maximum_duration = std::chrono::nanoseconds::max().count();
  constexpr auto maximum_ticks = static_cast<std::uint64_t>(maximum_duration / 100);
  if (duration_100ns > maximum_ticks) return std::chrono::nanoseconds::max();
  return std::chrono::nanoseconds(static_cast<std::int64_t>(duration_100ns * 100));
}

Event::Event(bool manual_reset, bool signaled) noexcept
    : manual_reset_(manual_reset), signaled_(signaled) {}

bool Event::Set() noexcept {
  std::lock_guard lock(wait_mutex);
  const bool was_signaled = signaled_;
  signaled_ = true;
  CompleteQueuedWaits();
  wait_condition.notify_all();
  return was_signaled;
}

bool Event::Reset() noexcept {
  std::lock_guard lock(wait_mutex);
  const bool was_signaled = signaled_;
  signaled_ = false;
  return was_signaled;
}

bool Event::signaled() const noexcept {
  std::lock_guard lock(wait_mutex);
  return signaled_;
}

bool Event::Pulse() {
  std::lock_guard lock(wait_mutex);
  const bool was_signaled = signaled_;
  signaled_ = true;
  // Commit eligible waits while holding the dispatcher lock. Set/Reset would
  // lose the pulse before a sleeping thread could reacquire that lock.
  for (auto* pending : pending_waits) {
    if (std::find(pending->waitables.begin(), pending->waitables.end(), this) !=
        pending->waitables.end())
      pending->complete();
  }
  signaled_ = false;
  wait_condition.notify_all();
  return was_signaled;
}

WaitResult Event::Wait(std::chrono::nanoseconds timeout) {
  Waitable* waitable = this;
  return WaitMultiple(std::span<Waitable* const>(&waitable, 1), false, timeout).first;
}

bool Event::IsSignaled(std::thread::id) const noexcept { return signaled_; }

void Event::ConsumeSignal(std::thread::id, std::uintptr_t) noexcept {
  if (!manual_reset_) signaled_ = false;
}

bool Event::SignalForWait() noexcept {
  signaled_ = true;
  return true;
}

Semaphore::Semaphore(std::uint32_t initial_count, std::uint32_t maximum_count)
    : count_(initial_count), maximum_count_(maximum_count) {
  if (maximum_count == 0 || initial_count > maximum_count)
    throw std::invalid_argument("invalid semaphore count");
}

bool Semaphore::Release(std::uint32_t count, std::uint32_t* previous_count) noexcept {
  std::lock_guard lock(wait_mutex);
  if (count > maximum_count_ - count_) return false;
  if (previous_count) *previous_count = count_;
  count_ += count;
  if (count > 0) CompleteQueuedWaits();
  if (count > 0) wait_condition.notify_all();
  return true;
}

WaitResult Semaphore::Wait(std::chrono::nanoseconds timeout) {
  Waitable* waitable = this;
  return WaitMultiple(std::span<Waitable* const>(&waitable, 1), false, timeout).first;
}

std::uint32_t Semaphore::count() const noexcept {
  std::lock_guard lock(wait_mutex);
  return count_;
}

Mutant::Mutant(bool initial_owner) noexcept
    : owner_(initial_owner ? std::this_thread::get_id() : std::thread::id{}),
      recursion_(initial_owner ? 1 : 0),
      owner_token_(initial_owner ? ThreadLocalValue() : 0) {}

bool Mutant::Release() noexcept {
  std::lock_guard lock(wait_mutex);
  if (owner_ != std::this_thread::get_id() || recursion_ == 0) return false;
  if (--recursion_ == 0) {
    owner_ = {};
    owner_token_ = 0;
    CompleteQueuedWaits();
    wait_condition.notify_all();
  }
  return true;
}

std::uint32_t Mutant::Abandon() noexcept {
  std::lock_guard lock(wait_mutex);
  if (owner_ != std::this_thread::get_id() || recursion_ == 0) return 0;
  const auto depth = recursion_;
  owner_ = {};
  recursion_ = 0;
  owner_token_ = 0;
  abandoned_ = true;
  CompleteQueuedWaits();
  wait_condition.notify_all();
  return depth;
}

std::uint32_t Mutant::AbandonOwner(std::uintptr_t owner_token) noexcept {
  std::lock_guard lock(wait_mutex);
  if (recursion_ == 0 || owner_token_ != owner_token) return 0;
  const auto depth = recursion_;
  owner_ = {};
  recursion_ = 0;
  owner_token_ = 0;
  abandoned_ = true;
  CompleteQueuedWaits();
  wait_condition.notify_all();
  return depth;
}

std::uintptr_t Mutant::owner_token() const noexcept {
  std::lock_guard lock(wait_mutex);
  return owner_token_;
}
Mutant::State Mutant::SaveState() const noexcept {
  std::lock_guard lock(wait_mutex);
  return {owner_token_, recursion_, abandoned_};
}
void Mutant::RestoreUnowned(bool abandoned) noexcept {
  std::lock_guard lock(wait_mutex);
  owner_ = {};
  owner_token_ = 0;
  recursion_ = 0;
  abandoned_ = abandoned;
}
bool Mutant::RestoreOwnershipOnCurrentThread(std::uint32_t recursion) noexcept {
  std::lock_guard lock(wait_mutex);
  if (!recursion || recursion_ || abandoned_) return false;
  owner_ = std::this_thread::get_id();
  owner_token_ = ThreadLocalValue();
  recursion_ = recursion;
  return true;
}

bool Mutant::owned_by_current_thread() const noexcept {
  std::lock_guard lock(wait_mutex);
  return owner_ == std::this_thread::get_id() && recursion_ != 0;
}

WaitResult Mutant::Wait(std::chrono::nanoseconds timeout) {
  Waitable* waitable = this;
  return WaitMultiple(std::span<Waitable* const>(&waitable, 1), false, timeout).first;
}

bool Mutant::IsSignaled(std::thread::id waiter) const noexcept {
  return abandoned_ || recursion_ == 0 || owner_ == waiter;
}

bool Mutant::IsAbandoned() const noexcept { return abandoned_; }

void Mutant::ConsumeSignal(std::thread::id current, std::uintptr_t token) noexcept {
  if (owner_ == current) {
    ++recursion_;
  } else {
    owner_ = current;
    owner_token_ = token;
    recursion_ = 1;
  }
}

void Mutant::ClearAbandoned() noexcept { abandoned_ = false; }

bool Mutant::SignalForWait() noexcept {
  if (owner_ != std::this_thread::get_id() || recursion_ == 0) return false;
  if (--recursion_ == 0) {
    owner_ = {};
    owner_token_ = 0;
    wait_condition.notify_all();
  }
  return true;
}

bool Semaphore::IsSignaled(std::thread::id) const noexcept { return count_ != 0; }

void Semaphore::ConsumeSignal(std::thread::id, std::uintptr_t) noexcept { --count_; }

bool Semaphore::SignalForWait() noexcept {
  if (count_ == maximum_count_) return false;
  ++count_;
  return true;
}

struct HostThread::Impl {
  std::mutex mutex;
  std::condition_variable condition;
  std::mutex callback_mutex;
  std::deque<std::function<void()>> callbacks;
  std::function<void()> entry;
  std::string name;
  std::uint32_t suspend_count = 0;
  std::atomic<std::uint32_t> native_suspend_count{0};
  std::uint32_t system_id = 0;
  bool started = false;
  std::atomic<bool> finished{false};
  std::atomic<bool> joined{false};
  static thread_local Impl* current;
  struct ExitRequest { int code; };
#if defined(_WIN32)
  HANDLE native = nullptr;
#else
  pthread_t native{};
#if !defined(__APPLE__) && !defined(__SWITCH__)
  sem_t suspend_signal{};
  bool suspend_signal_initialized = false;
  static int SuspendSignal() { return SIGRTMIN + 4; }
  static bool InstallSuspendSignal() {
    static std::once_flag once;
    static bool installed = false;
    std::call_once(once, [] {
      struct sigaction action {};
      action.sa_handler = [](int) {
        auto* self = current;
        if (!self) return;
        while (self->native_suspend_count.load(std::memory_order_acquire)) {
          while (sem_wait(&self->suspend_signal) != 0 && errno == EINTR) {}
        }
      };
      sigemptyset(&action.sa_mask);
      installed = SuspendSignal() <= SIGRTMAX &&
                  sigaction(SuspendSignal(), &action, nullptr) == 0;
    });
    return installed;
  }
#endif
#endif

  ~Impl() {
#if defined(_WIN32)
    if (native) CloseHandle(native);
#elif !defined(__APPLE__) && !defined(__SWITCH__)
    if (suspend_signal_initialized) sem_destroy(&suspend_signal);
#endif
  }

  void Finish() {
    finished.store(true, std::memory_order_release);
    condition.notify_all();
    if (current == this) current = nullptr;
  }

  void Enter() {
    current = this;
    {
      std::unique_lock lock(mutex);
#if defined(_WIN32)
      system_id = GetCurrentThreadId();
#elif defined(__APPLE__)
      system_id = static_cast<std::uint32_t>(pthread_mach_thread_np(pthread_self()));
#elif defined(__SWITCH__)
      system_id = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pthread_self()));
#else
      system_id = static_cast<std::uint32_t>(syscall(SYS_gettid));
#endif
      started = true;
      condition.notify_all();
      condition.wait(lock, [this] { return suspend_count == 0; });
#if defined(__APPLE__)
      if (!name.empty()) pthread_setname_np(name.substr(0, 63).c_str());
#endif
    }
    entry();
  }

#if defined(_WIN32)
  static DWORD WINAPI Run(void* opaque) {
    auto self = std::move(*static_cast<std::shared_ptr<Impl>*>(opaque));
    delete static_cast<std::shared_ptr<Impl>*>(opaque);
    DWORD code = 0;
    try {
      self->Enter();
    } catch (const ExitRequest& request) {
      code = static_cast<DWORD>(request.code);
    }
    self->Finish();
    return code;
  }
#elif defined(__SWITCH__)
  static void* Run(void* opaque) {
    auto self = std::move(*static_cast<std::shared_ptr<Impl>*>(opaque));
    delete static_cast<std::shared_ptr<Impl>*>(opaque);
    void* code = nullptr;
    try { self->Enter(); }
    catch (const ExitRequest& request) {
      code = reinterpret_cast<void*>(static_cast<std::intptr_t>(request.code));
    }
    self->Finish();
    return code;
  }
#else
  static void* Run(void* opaque) {
    auto self = std::move(*static_cast<std::shared_ptr<Impl>*>(opaque));
    delete static_cast<std::shared_ptr<Impl>*>(opaque);
    void* code = nullptr;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    pthread_cleanup_push([](void* state) { static_cast<Impl*>(state)->Finish(); }, self.get());
    try {
      self->Enter();
    } catch (const ExitRequest& request) {
      code = reinterpret_cast<void*>(static_cast<std::intptr_t>(request.code));
    }
    pthread_cleanup_pop(1);
    return code;
  }
#endif
};

thread_local HostThread::Impl* HostThread::Impl::current = nullptr;

HostThread::HostThread(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

std::unique_ptr<HostThread> HostThread::Create(CreationParameters parameters,
                                              std::function<void()> entry) {
  if (!entry) return nullptr;
  auto impl = std::make_shared<Impl>();
  impl->entry = std::move(entry);
  impl->suspend_count = parameters.create_suspended ? 1 : 0;
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__SWITCH__)
  if (!Impl::InstallSuspendSignal() || sem_init(&impl->suspend_signal, 0, 0) != 0)
    return nullptr;
  impl->suspend_signal_initialized = true;
#endif
  auto start = std::make_unique<std::shared_ptr<Impl>>(impl);
#if defined(_WIN32)
  impl->native = CreateThread(nullptr, parameters.stack_size, Impl::Run, start.get(), 0, nullptr);
  if (!impl->native) return nullptr;
#else
  pthread_attr_t attributes;
  if (pthread_attr_init(&attributes) != 0) return nullptr;
  const int attribute_result = pthread_attr_setstacksize(&attributes, parameters.stack_size);
  const int create_result = attribute_result ? attribute_result :
      pthread_create(&impl->native, &attributes, Impl::Run, start.get());
  pthread_attr_destroy(&attributes);
  if (create_result != 0) return nullptr;
#endif
  start.release();
  {
    std::unique_lock lock(impl->mutex);
    impl->condition.wait(lock, [&] { return impl->started; });
  }
  return std::unique_ptr<HostThread>(new HostThread(std::move(impl)));
}

HostThread::~HostThread() {
  if (Impl::current == impl_.get()) {
#if !defined(_WIN32)
    if (!impl_->joined.exchange(true)) pthread_detach(impl_->native);
#endif
  } else {
    Join();
  }
}

void HostThread::Join() {
  if (Impl::current == impl_.get())
    throw std::logic_error("cannot join the current thread");
  if (impl_->joined.exchange(true)) return;
#if defined(_WIN32)
  WaitForSingleObject(impl_->native, INFINITE);
#else
  pthread_join(impl_->native, nullptr);
#endif
}

[[noreturn]] void HostThread::Exit(int exit_code) {
  // pthread_exit does not unwind automatic C++ objects on every supported host.
  // Keep Machine, guest-context and registration cleanup on the C++ exit path.
  throw Impl::ExitRequest{exit_code};
}

void HostThread::QueueUserCallback(std::function<void()> callback) {
  {
    std::lock_guard lock(impl_->callback_mutex);
    impl_->callbacks.push_back(std::move(callback));
  }
  NotifyWaiters();
}

bool HostThread::DispatchUserCallbacks() {
  auto* self = Impl::current;
  if (!self) return false;
  std::function<void()> callback;
  {
    std::lock_guard lock(self->callback_mutex);
    if (self->callbacks.empty()) return false;
    callback = std::move(self->callbacks.front());
    self->callbacks.pop_front();
  }
  if (callback) callback();
  return true;
}

bool HostThread::Suspend(std::uint32_t* previous_count,
                         const std::function<void()>& ready) {
  std::unique_lock lock(impl_->mutex);
  if (previous_count) *previous_count = impl_->suspend_count;
  if (impl_->finished) return false;
  if (Impl::current == impl_.get()) {
    ++impl_->suspend_count;
    lock.unlock();
    if (ready) ready();
    lock.lock();
    impl_->condition.wait(lock, [&] { return impl_->suspend_count == 0; });
    return true;
  }
  ++impl_->native_suspend_count;
#if defined(_WIN32)
  const bool suspended = SuspendThread(impl_->native) != DWORD(-1);
#elif defined(__APPLE__)
  const bool suspended = thread_suspend(pthread_mach_thread_np(impl_->native)) == KERN_SUCCESS;
#elif defined(__SWITCH__)
  const bool suspended = false;
#else
  const bool suspended = pthread_kill(impl_->native, Impl::SuspendSignal()) == 0;
#endif
  if (!suspended) {
    --impl_->native_suspend_count;
    return false;
  }
  ++impl_->suspend_count;
  lock.unlock();
  if (ready) ready();
  return true;
}

bool HostThread::Resume(std::uint32_t* previous_count) {
  std::lock_guard lock(impl_->mutex);
  if (previous_count) *previous_count = impl_->suspend_count;
  if (impl_->finished || !impl_->suspend_count) return false;
  if (impl_->native_suspend_count.load()) {
#if defined(_WIN32)
    if (ResumeThread(impl_->native) == DWORD(-1)) return false;
#elif defined(__APPLE__)
    if (thread_resume(pthread_mach_thread_np(impl_->native)) != KERN_SUCCESS) return false;
#elif defined(__SWITCH__)
    return false;
#endif
    const auto remaining = --impl_->native_suspend_count;
#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__SWITCH__)
    if (!remaining) sem_post(&impl_->suspend_signal);
#else
    (void)remaining;
#endif
  }
  if (--impl_->suspend_count == 0) impl_->condition.notify_all();
  return true;
}

void HostThread::Terminate(int exit_code) {
  if (Impl::current == impl_.get()) Exit(exit_code);
  std::uint32_t previous = 0;
  while (Resume(&previous) && previous > 1) {}
#if defined(_WIN32)
  if (TerminateThread(impl_->native, static_cast<DWORD>(exit_code))) impl_->Finish();
#else
  (void)exit_code;
  // macOS delivers pthread cancellation at cancellation points even when the
  // asynchronous mode was requested. Join keeps the caller from releasing a
  // guest stack still in use; an uncooperative busy loop cannot be terminated.
  pthread_cancel(impl_->native);
#endif
  Join();
}

std::uint32_t HostThread::system_id() const noexcept { return impl_->system_id; }

void HostThread::set_name(std::string name) {
  std::lock_guard lock(impl_->mutex);
  impl_->name = std::move(name);
#if defined(_WIN32)
  const std::wstring wide(impl_->name.begin(), impl_->name.end());
  SetThreadDescription(impl_->native, wide.c_str());
#elif defined(__APPLE__)
  if (Impl::current == impl_.get()) pthread_setname_np(impl_->name.substr(0, 63).c_str());
#else
  pthread_setname_np(impl_->native, impl_->name.substr(0, 15).c_str());
#endif
}

int HostThread::priority() const {
#if defined(_WIN32)
  return GetThreadPriority(impl_->native);
#elif defined(__SWITCH__)
  return 0;
#else
  int policy = 0;
  sched_param parameter{};
  return pthread_getschedparam(impl_->native, &policy, &parameter) == 0
             ? parameter.sched_priority : -1;
#endif
}

void HostThread::set_priority(int value) {
#if defined(_WIN32)
  SetThreadPriority(impl_->native, value);
#elif defined(__SWITCH__)
  (void)value;
#else
  sched_param parameter{};
  parameter.sched_priority = value;
  pthread_setschedparam(impl_->native, SCHED_FIFO, &parameter);
#endif
}

void HostThread::set_affinity_mask(std::uint64_t mask) {
#if defined(_WIN32)
  SetThreadAffinityMask(impl_->native, static_cast<DWORD_PTR>(mask));
#elif defined(__APPLE__)
  (void)mask;
#elif defined(__SWITCH__)
  (void)mask;
#else
  cpu_set_t set;
  CPU_ZERO(&set);
  for (unsigned bit = 0; bit < 64; ++bit) {
    if (mask & (std::uint64_t{1} << bit)) CPU_SET(bit, &set);
  }
  pthread_setaffinity_np(impl_->native, sizeof(set), &set);
#endif
}

Thread::Thread(std::function<void()> entry) {
  if (!entry) throw std::invalid_argument("thread entry is empty");
  thread_ = std::thread([this, entry = std::move(entry)] {
    try {
      entry();
    } catch (...) {
      exception_ = std::current_exception();
    }
  });
}

Thread::~Thread() {
  if (thread_.joinable()) thread_.join();
}

void Thread::Join() {
  if (thread_.joinable()) thread_.join();
  if (exception_) std::rethrow_exception(exception_);
}

bool Thread::joinable() const noexcept { return thread_.joinable(); }

Timer::Timer() : thread_([this] { Run(); }) {}

Timer::~Timer() {
  std::function<void()> previous;
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
    armed_ = false;
    previous.swap(callback_);
    ++generation_;
  }
  condition_.notify_one();
  thread_.join();
}

void Timer::Set(std::chrono::nanoseconds due, std::chrono::nanoseconds period,
                std::function<void()> callback) {
  if (due < std::chrono::nanoseconds::zero() || period < std::chrono::nanoseconds::zero() ||
      !callback)
    throw std::invalid_argument("invalid timer configuration");
  std::function<void()> previous;
  {
    std::lock_guard lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    if (due > std::chrono::steady_clock::time_point::max() - now)
      throw std::invalid_argument("timer deadline exceeds clock range");
    due_ = now + due;
    if (paused_) paused_remaining_ = due;
    period_ = period;
    previous.swap(callback_);
    callback_.swap(callback);
    armed_ = true;
    ++generation_;
  }
  condition_.notify_one();
}

void Timer::Cancel() noexcept {
  std::function<void()> previous;
  std::unique_lock lock(mutex_);
  armed_ = false;
  paused_remaining_ = std::chrono::nanoseconds::zero();
  previous.swap(callback_);
  ++generation_;
  condition_.notify_one();
  if (thread_.get_id() != std::this_thread::get_id())
    condition_.wait(lock, [this] { return !callback_running_; });
}

void Timer::RethrowCallbackException() {
  std::exception_ptr exception;
  {
    std::lock_guard lock(mutex_);
    exception = std::exchange(exception_, {});
  }
  if (exception) std::rethrow_exception(exception);
}

void Timer::Pause() {
  std::unique_lock lock(mutex_);
  if (paused_) return;
  paused_ = true;
  condition_.notify_all();
  condition_.wait(lock, [this] { return !callback_running_; });
  paused_remaining_ = armed_ ? std::max(std::chrono::nanoseconds::zero(),
      std::chrono::duration_cast<std::chrono::nanoseconds>(due_ - std::chrono::steady_clock::now()))
      : std::chrono::nanoseconds::zero();
}

void Timer::Resume() {
  std::lock_guard lock(mutex_);
  if (!paused_) return;
  if (armed_) due_ = std::chrono::steady_clock::now() + paused_remaining_;
  paused_ = false;
  condition_.notify_all();
}

Timer::State Timer::CapturePausedState() {
  std::lock_guard lock(mutex_);
  if (!paused_ || callback_running_) throw std::logic_error("timer must be paused before capture");
  return {armed_, paused_remaining_, period_};
}

void Timer::ElapsePaused(std::chrono::nanoseconds elapsed) {
  std::lock_guard lock(mutex_);
  if (!paused_) throw std::logic_error("timer must be paused before adjusting its deadline");
  paused_remaining_ -= std::min(paused_remaining_, elapsed);
}

void Timer::Run() {
  std::unique_lock lock(mutex_);
  while (!stopping_) {
    condition_.wait(lock, [this] { return stopping_ || (armed_ && !paused_); });
    if (stopping_) break;
    const auto generation = generation_;
    const auto due = due_;
    if (condition_.wait_until(lock, due, [this, generation] {
          return stopping_ || paused_ || !armed_ || generation_ != generation;
        }))
      continue;
    std::function<void()> callback;
    callback.swap(callback_);
    callback_running_ = true;
    const auto period = period_;
    armed_ = false;
    lock.unlock();
    std::exception_ptr exception;
    try {
      callback();
    } catch (...) {
      exception = std::current_exception();
    }
    lock.lock();
    if (exception) exception_ = exception;
    if (generation_ == generation && !stopping_ && !exception &&
        period > std::chrono::nanoseconds::zero()) {
      const auto now = std::chrono::steady_clock::now();
      if (period > std::chrono::steady_clock::time_point::max() - now) {
        exception_ = std::make_exception_ptr(
            std::invalid_argument("timer period exceeds clock range"));
      } else {
        due_ = now + period;
        callback_.swap(callback);
        armed_ = true;
      }
    }
    // A capture destructor may reenter the timer or acquire a kernel lock.
    lock.unlock();
    callback = {};
    lock.lock();
    callback_running_ = false;
    condition_.notify_all();
  }
}

void SetThreadLocalValue(std::uintptr_t value) noexcept { thread_local_value = value; }
std::uintptr_t ThreadLocalValue() noexcept { return thread_local_value; }

}  // namespace gh::kernel
