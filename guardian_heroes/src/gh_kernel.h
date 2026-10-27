#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>

class GuestMemory;

namespace gh::kernel {

enum class WaitResult { Signaled, Abandoned, Timeout, Alerted, Failed };

class Waitable {
 public:
  virtual ~Waitable() = default;

 private:
  friend class QueuedWait;
  friend std::pair<WaitResult, std::size_t> WaitMultiple(
      std::span<Waitable* const>, bool, std::chrono::nanoseconds,
      const std::function<bool()>&, Waitable*);
  virtual bool IsSignaled(std::thread::id waiter) const noexcept = 0;
  virtual bool IsAbandoned() const noexcept { return false; }
  virtual void ConsumeSignal(std::thread::id waiter, std::uintptr_t token) noexcept = 0;
  virtual void ClearAbandoned() noexcept {}
  virtual bool SignalForWait() noexcept = 0;
};

std::pair<WaitResult, std::size_t> WaitMultiple(
    std::span<Waitable* const> waitables, bool wait_all,
    std::chrono::nanoseconds timeout = std::chrono::nanoseconds::max(),
    const std::function<bool()>& alert_pending = {},
    Waitable* signal_before_wait = nullptr);

// Wakes project waits after an external alert source changes state, such as a
// guest APC queue. The wait predicate is checked again while holding the wait
// lock, so notifications cannot consume object state on an alert.
void NotifyWaiters() noexcept;

// A registered dispatcher wait survives guest safepoints without removing and
// requeueing the waiter. State contains no native thread IDs or pointers.
class QueuedWait {
 public:
  struct State {
    uint64_t order = 0;
    WaitResult result = WaitResult::Timeout;
    size_t index = 0;
    bool completed = false;
  };
  QueuedWait(std::span<Waitable* const>, bool wait_all,
             std::function<bool()> alert = {}, std::function<bool()> expired = {},
             Waitable* signal = nullptr, const State* restored = nullptr);
  ~QueuedWait();
  QueuedWait(const QueuedWait&) = delete;
  QueuedWait& operator=(const QueuedWait&) = delete;
  std::optional<std::pair<WaitResult, size_t>> Poll();
  State Save() const;
  void WaitForWake(std::chrono::milliseconds maximum);
 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Reset requires stopped guest threads. Save restoration rebuilds ownership
// from guest fields without serializing host synchronization data.
class CriticalSections {
 public:
  CriticalSections();
  ~CriticalSections();
  CriticalSections(const CriticalSections&) = delete;
  CriticalSections& operator=(const CriticalSections&) = delete;

  void Initialize(const GuestMemory& memory, std::uint32_t address,
                  std::uint32_t spin_count = 0);
  bool Enter(const GuestMemory& memory, std::uint32_t address,
             std::uint32_t guest_thread);
  bool TryEnter(const GuestMemory& memory, std::uint32_t address,
                std::uint32_t guest_thread);
  bool Leave(const GuestMemory& memory, std::uint32_t address,
             std::uint32_t guest_thread);
  bool IsOwnedBy(std::uint32_t guest_thread) const;
  void CancelWaits();
  void Reset(bool restore_guest_ownership = false);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

// Converts the Xbox signed 100 ns timeout convention to a host duration.
// Negative values are relative; nonnegative values are absolute guest times.
std::chrono::nanoseconds GuestTimeoutToDuration(
    std::int64_t timeout_100ns, std::uint64_t guest_now_100ns) noexcept;

class Event : public Waitable {
 public:
  Event(bool manual_reset, bool signaled = false) noexcept;
  bool Set() noexcept;
  bool Reset() noexcept;
  bool Pulse();
  bool signaled() const noexcept;
  WaitResult Wait(std::chrono::nanoseconds timeout =
                      std::chrono::nanoseconds::max());

 private:
  bool IsSignaled(std::thread::id waiter) const noexcept override;
  void ConsumeSignal(std::thread::id waiter, std::uintptr_t token) noexcept override;
  bool SignalForWait() noexcept override;

  bool manual_reset_;
  bool signaled_;
};

class Semaphore : public Waitable {
 public:
  Semaphore(std::uint32_t initial_count, std::uint32_t maximum_count);
  bool Release(std::uint32_t count = 1,
               std::uint32_t* previous_count = nullptr) noexcept;
  WaitResult Wait(std::chrono::nanoseconds timeout =
                      std::chrono::nanoseconds::max());
  std::uint32_t count() const noexcept;

 private:
  bool IsSignaled(std::thread::id waiter) const noexcept override;
  void ConsumeSignal(std::thread::id waiter, std::uintptr_t token) noexcept override;
  bool SignalForWait() noexcept override;

  std::uint32_t count_;
  std::uint32_t maximum_count_;
};

class Mutant : public Waitable {
 public:
  struct State {
    std::uintptr_t owner;
    std::uint32_t recursion;
    bool abandoned;
  };
  explicit Mutant(bool initial_owner = false) noexcept;
  bool Release() noexcept;
  // Releases all ownership held by the exiting thread and wakes waiters.
  // Returns the recursion depth that was abandoned.
  std::uint32_t Abandon() noexcept;
  std::uint32_t AbandonOwner(std::uintptr_t owner_token) noexcept;
  std::uintptr_t owner_token() const noexcept;
  State SaveState() const noexcept;
  void RestoreUnowned(bool abandoned) noexcept;
  bool RestoreOwnershipOnCurrentThread(std::uint32_t recursion) noexcept;
  bool owned_by_current_thread() const noexcept;
  WaitResult Wait(std::chrono::nanoseconds timeout =
                      std::chrono::nanoseconds::max());

 private:
  bool IsSignaled(std::thread::id waiter) const noexcept override;
  bool IsAbandoned() const noexcept override;
  void ConsumeSignal(std::thread::id waiter, std::uintptr_t token) noexcept override;
  void ClearAbandoned() noexcept override;
  bool SignalForWait() noexcept override;

  std::thread::id owner_{};
  std::uint32_t recursion_ = 0;
  bool abandoned_ = false;
  std::uintptr_t owner_token_ = 0;
};

class HostThread {
 public:
  struct CreationParameters {
    std::size_t stack_size = 4 * 1024 * 1024;
    bool create_suspended = false;
  };
  static std::unique_ptr<HostThread> Create(CreationParameters parameters,
                                          std::function<void()> entry);
  ~HostThread();
  HostThread(const HostThread&) = delete;
  HostThread& operator=(const HostThread&) = delete;
  [[noreturn]] static void Exit(int exit_code);
  static bool DispatchUserCallbacks();
  void QueueUserCallback(std::function<void()> callback);
  bool Suspend(std::uint32_t* previous_count = nullptr,
               const std::function<void()>& ready = {});
  bool Resume(std::uint32_t* previous_count = nullptr);
  void Terminate(int exit_code);
  void Join();
  std::uint32_t system_id() const noexcept;
  void set_name(std::string name);
  int priority() const;
  void set_priority(int value);
  void set_affinity_mask(std::uint64_t mask);

 private:
  struct Impl;
  explicit HostThread(std::shared_ptr<Impl> impl);
  std::shared_ptr<Impl> impl_;
};

class Thread {
 public:
  explicit Thread(std::function<void()> entry);
  ~Thread();
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  // Explicit joins report an entry exception; destruction only joins.
  void Join();
  bool joinable() const noexcept;

 private:
  std::exception_ptr exception_;
  std::thread thread_;
};

class Timer {
 public:
  struct State {
    bool armed;
    std::chrono::nanoseconds remaining;
    std::chrono::nanoseconds period;
  };
  Timer();
  ~Timer();
  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  void Set(std::chrono::nanoseconds due, std::chrono::nanoseconds period,
           std::function<void()> callback);
  // Waits for an in-flight callback, except when called by that callback.
  void Cancel() noexcept;
  void RethrowCallbackException();
  void Pause();
  void Resume();
  State CapturePausedState();
  void ElapsePaused(std::chrono::nanoseconds elapsed);

 private:
  void Run();

  std::mutex mutex_;
  std::condition_variable condition_;
  std::chrono::steady_clock::time_point due_{};
  std::chrono::nanoseconds period_{};
  std::function<void()> callback_;
  std::uint64_t generation_ = 0;
  bool armed_ = false;
  bool stopping_ = false;
  bool callback_running_ = false;
  bool paused_ = false;
  std::chrono::nanoseconds paused_remaining_{};
  std::exception_ptr exception_;
  // Start the worker only after all state it reads has been initialized.
  std::thread thread_;
};

// The current runtime binds its guest thread handle here for mutant ownership.
void SetThreadLocalValue(std::uintptr_t value) noexcept;
std::uintptr_t ThreadLocalValue() noexcept;

}  // namespace gh::kernel
