#include "gh_kernel.h"
#include "native_memory.h"

#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <limits>
#include <stdexcept>

using namespace std::chrono_literals;
using gh::kernel::Event;
using gh::kernel::Mutant;
using gh::kernel::Semaphore;
using gh::kernel::Thread;
using gh::kernel::Timer;
using gh::kernel::WaitResult;

namespace {
void RequireSignal(Event& event) {
  assert(event.Wait(2s) == WaitResult::Signaled);
}

void TestEvents() {
  Event manual(true);
  assert(manual.Wait(-1ns) == WaitResult::Timeout);
  assert(!manual.Reset());
  assert(!manual.Set());
  assert(manual.Set());
  Thread first([&] { RequireSignal(manual); });
  Thread second([&] { RequireSignal(manual); });
  first.Join();
  second.Join();
  assert(manual.Wait(0ns) == WaitResult::Signaled);
  assert(manual.Reset());
  assert(!manual.Reset());
  assert(manual.Wait(1ms) == WaitResult::Timeout);

  Event automatic(false);
  Event first_released(false);
  std::atomic<int> released = 0;
  auto wait = [&] {
    RequireSignal(automatic);
    if (++released == 1) first_released.Set();
  };
  Thread waiter_a(wait);
  Thread waiter_b(wait);
  automatic.Set();
  RequireSignal(first_released);
  assert(released == 1);
  assert(automatic.Wait(0ns) == WaitResult::Timeout);
  automatic.Set();
  waiter_a.Join();
  waiter_b.Join();
  assert(released == 2);
}

void TestSemaphore() {
  Semaphore semaphore(1, 2);
  assert(semaphore.Wait(0ns) == WaitResult::Signaled);
  assert(semaphore.Wait(-1ns) == WaitResult::Timeout);
  assert(semaphore.Release(2));
  assert(!semaphore.Release(1));
  assert(semaphore.count() == 2);
  assert(semaphore.Wait(0ns) == WaitResult::Signaled);
  assert(semaphore.Wait(0ns) == WaitResult::Signaled);
  assert(semaphore.Wait(1ms) == WaitResult::Timeout);
  Thread waiter([&] { assert(semaphore.Wait(2s) == WaitResult::Signaled); });
  assert(semaphore.Release());
  waiter.Join();
  assert(semaphore.count() == 0);
  try {
    Semaphore invalid(2, 1);
    assert(false);
  } catch (const std::invalid_argument&) {
  }
}

void TestMutant() {
  Mutant mutant;
  assert(!mutant.Release());
  assert(mutant.Wait(0ns) == WaitResult::Signaled);
  assert(mutant.Wait(0ns) == WaitResult::Signaled);
  assert(mutant.owned_by_current_thread());
  bool other_thread_released = true;
  Thread wrong_owner([&] { other_thread_released = mutant.Release(); });
  wrong_owner.Join();
  assert(!other_thread_released);
  Thread blocked([&] {
    assert(mutant.Wait(2s) == WaitResult::Signaled);
    assert(mutant.Release());
  });
  assert(mutant.Release());
  assert(mutant.Release());
  blocked.Join();
  assert(!mutant.owned_by_current_thread());
  assert(!mutant.Release());

  Mutant initially_owned(true);
  assert(initially_owned.Wait(0ns) == WaitResult::Signaled);
  assert(initially_owned.Release());
  assert(initially_owned.Release());
  assert(initially_owned.Wait(0ns) == WaitResult::Signaled);
  assert(initially_owned.Release());
}

void TestMutantAbandonment() {
  Mutant mutant;
  Event acquired(false);
  Event abandon(false);
  Thread owner([&] {
    assert(mutant.Wait(0ns) == WaitResult::Signaled);
    acquired.Set();
    RequireSignal(abandon);
    assert(mutant.Abandon() == 1);
  });
  RequireSignal(acquired);
  abandon.Set();
  auto result = mutant.Wait(2s);
  assert(result == WaitResult::Abandoned);
  assert(mutant.owned_by_current_thread());
  assert(mutant.Release());
  owner.Join();
}

void TestGuestCriticalSection() {
  gh::kernel::CriticalSections sections;
  alignas(4) std::array<std::uint8_t, 256> bytes{};
  GuestMemory memory(bytes.data(), bytes.size());
  constexpr std::uint32_t address = 0x40;
  constexpr std::uint32_t owner = 0x81234560;
  constexpr std::uint32_t contender_owner = 0x81234600;
  auto lock_count = std::atomic_ref<std::int32_t>(
      *reinterpret_cast<std::int32_t*>(memory.Address(address + 16, 4)));
  bytes[address + 2] = 0xA5;
  memory.Write32(address + 8, 0x12345678);
  sections.Initialize(memory, address, 257);
  assert(bytes[address] == 1 && bytes[address + 1] == 2);
  assert(bytes[address + 2] == 0xA5);
  assert(memory.Read32(address + 8) == 0x12345678);
  assert(lock_count.load() == -1);
  assert(memory.Read32(address + 4) == 0);
  assert(memory.Read32(address + 20) == 0);
  assert(memory.Read32(address + 24) == 0);

  sections.Enter(memory, address, owner);
  assert(sections.TryEnter(memory, address, owner));
  assert(lock_count.load() == 1);
  assert(memory.Read32(address + 20) == 2);
  assert(memory.Read32(address + 24) == owner);
  assert(bytes[address + 24] == 0x81 && bytes[address + 27] == 0x60);

  Event attempted(false);
  Event acquired(false);
  Thread contender([&] {
    assert(!sections.TryEnter(memory, address, contender_owner));
    assert(!sections.Leave(memory, address, contender_owner));
    attempted.Set();
    sections.Enter(memory, address, contender_owner);
    assert(memory.Read32(address + 24) == contender_owner);
    assert(memory.Read32(address + 20) == 1);
    acquired.Set();
    assert(sections.Leave(memory, address, contender_owner));
  });
  RequireSignal(attempted);
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (lock_count.load() != 2 && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  assert(lock_count.load() == 2);
  assert(sections.Leave(memory, address, owner));
  assert(lock_count.load() == 1);
  assert(memory.Read32(address + 20) == 1);
  assert(acquired.Wait(0ns) == WaitResult::Timeout);
  assert(sections.Leave(memory, address, owner));
  RequireSignal(acquired);
  contender.Join();
  assert(lock_count.load() == -1);
  assert(memory.Read32(address + 20) == 0);
  assert(memory.Read32(address + 24) == 0);
  assert(!sections.Leave(memory, address, owner));

  sections.Initialize(memory, address, 0xFFFFFFFF);
  assert(bytes[address + 1] == 255);
  assert(sections.TryEnter(memory, address, owner));
  assert(sections.Leave(memory, address, owner));
  sections.Initialize(memory, address);
  assert(bytes[address + 1] == 0);

  // Embedded game data can initialize a critical section without an RTL call.
  constexpr std::uint32_t embedded = 0x80;
  memory.Write32(embedded + 16, 0xFFFFFFFF);
  sections.Enter(memory, embedded, owner);
  assert(memory.Read32(embedded + 20) == 1);
  assert(sections.Leave(memory, embedded, owner));
}

void TestCriticalSectionLifetime() {
  alignas(4) std::array<std::uint8_t, 128> first_bytes{}, second_bytes{};
  GuestMemory first(first_bytes.data(), first_bytes.size());
  GuestMemory second(second_bytes.data(), second_bytes.size());
  constexpr std::uint32_t address = 0x40;
  gh::kernel::CriticalSections first_runtime, second_runtime;
  first_runtime.Initialize(first, address);
  second_runtime.Initialize(second, address);
  first_runtime.Enter(first, address, 0x100);
  Thread other_runtime([&] {
    assert(second_runtime.TryEnter(second, address, 0x200));
    assert(second_runtime.Leave(second, address, 0x200));
  });
  other_runtime.Join();
  assert(first_runtime.Leave(first, address, 0x100));
  // Destroying a runtime must not retain an owner for its next incarnation.
  { gh::kernel::CriticalSections old_runtime;
    old_runtime.Initialize(first, address);
    old_runtime.Enter(first, address, 0x100);
  }
  gh::kernel::CriticalSections new_runtime;
  new_runtime.Initialize(first, address);
  assert(new_runtime.TryEnter(first, address, 0x300));
  assert(new_runtime.Leave(first, address, 0x300));
}

void TestCriticalSectionTerminationAndReuse() {
  gh::kernel::CriticalSections sections;
  alignas(4) std::array<std::uint8_t, 128> bytes{};
  GuestMemory memory(bytes.data(), bytes.size());
  constexpr std::uint32_t address = 0x40;
  auto count = std::atomic_ref<std::int32_t>(
      *reinterpret_cast<std::int32_t*>(memory.Address(address + 16, 4)));
  sections.Initialize(memory, address);
  assert(sections.Enter(memory, address, 0x100));
  Thread contender([&] {
    assert(!sections.Enter(memory, address, 0x200));
    assert(!sections.TryEnter(memory, address, 0x200));
  });
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (count.load() != 1 && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  assert(count.load() == 1);
  sections.CancelWaits();
  contender.Join();
  assert(count.load() == 0);
  assert(memory.Read32(address + 24) == 0x100);
  assert(!sections.Enter(memory, address, 0x300));

  // New titles must not inherit either dynamic or static critical ownership.
  sections.Reset();
  assert(sections.Enter(memory, address, 0x300));
  assert(sections.Leave(memory, address, 0x300));
  sections.Initialize(memory, address);
  assert(sections.Enter(memory, address, 0x300));
  assert(sections.Leave(memory, address, 0x300));
  assert(count.load() == -1);
}

void TestCriticalSectionRestore() {
  gh::kernel::CriticalSections sections;
  alignas(4) std::array<std::uint8_t, 128> bytes{};
  GuestMemory memory(bytes.data(), bytes.size());
  constexpr std::uint32_t address = 0x40;
  sections.Initialize(memory, address);
  Thread old_owner([&] {
    assert(sections.Enter(memory, address, 0x100));
    assert(sections.Enter(memory, address, 0x100));
  });
  old_owner.Join();
  const auto saved = bytes;
  // Change the current host state before restoring the saved guest image.
  assert(sections.Leave(memory, address, 0x100));
  assert(sections.Leave(memory, address, 0x100));
  bytes = saved;
  auto count = std::atomic_ref<std::int32_t>(
      *reinterpret_cast<std::int32_t*>(memory.Address(address + 16, 4)));
  count.store(7);  // Old waiter stacks must not survive restoration.
  sections.Reset(/*restore_guest_ownership=*/true);
  assert(!sections.TryEnter(memory, address, 0x200));
  assert(count.load() == 1);
  Thread restored_owner([&] {
    assert(sections.Enter(memory, address, 0x100));
    assert(memory.Read32(address + 20) == 3);
    assert(sections.Leave(memory, address, 0x100));
    assert(sections.Leave(memory, address, 0x100));
    assert(sections.Leave(memory, address, 0x100));
  });
  restored_owner.Join();
  assert(count.load() == -1);
  assert(memory.Read32(address + 24) == 0);
  assert(sections.TryEnter(memory, address, 0x200));
  assert(sections.Leave(memory, address, 0x200));
}

void TestEventPulse() {
  for (bool manual : {false, true}) {
    Event event(manual);
    Semaphore available(1, 1);
    std::atomic<int> completed = 0;
    std::mutex rendezvous_mutex;
    std::condition_variable rendezvous;
    bool registered = false;
    Thread first([&] {
      gh::kernel::Waitable* objects[] = {&event, &available};
      auto result = gh::kernel::WaitMultiple(objects, true, 2s, [&] {
        std::lock_guard lock(rendezvous_mutex);
        registered = true;
        rendezvous.notify_one();
        return false;
      });
      assert(result.first == WaitResult::Signaled);
      ++completed;
    });
    {
      std::unique_lock lock(rendezvous_mutex);
      assert(rendezvous.wait_for(lock, 2s, [&] { return registered; }));
    }
    assert(!event.Pulse());
    first.Join();
    assert(completed == 1);
    assert(!event.signaled());
    assert(event.Wait(0ns) == WaitResult::Timeout);
    assert(available.count() == 0);
  }

  Event manual(true);
  std::mutex mutex;
  std::condition_variable condition;
  int registered = 0;
  auto wait = [&] {
    bool announced = false;
    gh::kernel::Waitable* objects[] = {&manual};
    assert(gh::kernel::WaitMultiple(objects, false, 2s, [&] {
      if (!announced) {
        std::lock_guard lock(mutex);
        announced = true;
        ++registered;
        condition.notify_one();
      }
      return false;
    }).first == WaitResult::Signaled);
  };
  Thread first(wait), second(wait);
  {
    std::unique_lock lock(mutex);
    assert(condition.wait_for(lock, 2s, [&] { return registered == 2; }));
  }
  manual.Pulse();
  first.Join();
  second.Join();
  assert(!manual.signaled());
  manual.Set();
  assert(manual.signaled());
  assert(manual.signaled());
  assert(manual.Pulse());
  assert(!manual.signaled());
}

void TestPulseWaiterOwnership() {
  Event event(false);
  Mutant mutant;
  std::mutex mutex;
  std::condition_variable condition;
  bool registered = false;
  Thread waiter([&] {
    gh::kernel::SetThreadLocalValue(0xABC);
    gh::kernel::Waitable* objects[] = {&event, &mutant};
    auto result = gh::kernel::WaitMultiple(objects, true, 2s, [&] {
      std::lock_guard lock(mutex);
      registered = true;
      condition.notify_one();
      return false;
    });
    assert(result.first == WaitResult::Signaled);
    assert(mutant.owned_by_current_thread());
    assert(mutant.owner_token() == 0xABC);
    assert(mutant.Release());
  });
  {
    std::unique_lock lock(mutex);
    assert(condition.wait_for(lock, 2s, [&] { return registered; }));
  }
  event.Pulse();
  waiter.Join();
  assert(!event.signaled());
  assert(mutant.owner_token() == 0);
}

void TestMutantOwnerToken() {
  Mutant mutant;
  gh::kernel::SetThreadLocalValue(0x100);
  assert(mutant.Wait(0ns) == WaitResult::Signaled);
  assert(mutant.Wait(0ns) == WaitResult::Signaled);
  assert(mutant.owner_token() == 0x100);
  assert(mutant.AbandonOwner(0x200) == 0);
  assert(mutant.AbandonOwner(0x100) == 2);
  gh::kernel::SetThreadLocalValue(0x200);
  assert(mutant.Wait(0ns) == WaitResult::Abandoned);
  assert(mutant.owner_token() == 0x200);
  assert(mutant.Release());
  assert(mutant.owner_token() == 0);
  gh::kernel::SetThreadLocalValue(0);

  Semaphore semaphore(1, 2);
  std::uint32_t previous = 99;
  assert(semaphore.Release(1, &previous));
  assert(previous == 1);
  assert(!semaphore.Release(1, &previous));
  assert(previous == 1);
}

void TestMixedWaitMultiple() {
  Event automatic(false, true);
  Semaphore semaphore(0, 1);
  gh::kernel::Waitable* waitables[] = {&automatic, &semaphore};

  auto timed_out = gh::kernel::WaitMultiple(waitables, true, 1ms);
  assert(timed_out.first == WaitResult::Timeout);
  assert(automatic.Wait(0ns) == WaitResult::Signaled);
  automatic.Set();

  assert(semaphore.Release());
  auto all = gh::kernel::WaitMultiple(waitables, true, 0ns);
  assert(all.first == WaitResult::Signaled);
  assert(automatic.Wait(0ns) == WaitResult::Timeout);
  assert(semaphore.count() == 0);

  Event first(false, true);
  Event second(true, true);
  gh::kernel::Waitable* any_waitables[] = {&first, &second};
  auto any = gh::kernel::WaitMultiple(any_waitables, false, 0ns);
  assert(any.first == WaitResult::Signaled);
  assert(any.second == 0);
  assert(first.Wait(0ns) == WaitResult::Timeout);
  assert(second.Wait(0ns) == WaitResult::Signaled);

  Semaphore duplicate(1, 1);
  gh::kernel::Waitable* duplicate_waitables[] = {&duplicate, &duplicate};
  try {
    gh::kernel::WaitMultiple(duplicate_waitables, true, 0ns);
    assert(false);
  } catch (const std::invalid_argument&) {
  }
  assert(duplicate.count() == 1);
}

void TestAlertableWaitMultiple() {
  Event manual(true, true);
  Semaphore semaphore(0, 1);
  gh::kernel::Waitable* waitables[] = {&manual, &semaphore};
  std::atomic<bool> apc_pending = false;

  Thread queue_apc([&] {
    apc_pending.store(true, std::memory_order_release);
    gh::kernel::NotifyWaiters();
  });
  auto result = gh::kernel::WaitMultiple(
      waitables, true, 2s, [&] { return apc_pending.load(std::memory_order_acquire); });
  queue_apc.Join();

  assert(result.first == WaitResult::Alerted);
  assert(manual.Wait(0ns) == WaitResult::Signaled);
  assert(semaphore.count() == 0);
}

void TestGuestTimeoutConversion() {
  using gh::kernel::GuestTimeoutToDuration;
  assert(GuestTimeoutToDuration(-1, 1000) == 100ns);
  assert(GuestTimeoutToDuration(-10'000'000, 1000) == 1s);
  assert(GuestTimeoutToDuration(1025, 1000) == 2500ns);
  assert(GuestTimeoutToDuration(999, 1000) == 0ns);
  assert(GuestTimeoutToDuration(std::numeric_limits<std::int64_t>::min(), 0) ==
         std::chrono::nanoseconds::max());
}

void TestSignalAndWait() {
  Event signal(false);
  Event wait(false);
  gh::kernel::Waitable* waitables[] = {&wait};
  auto timed_out = gh::kernel::WaitMultiple(waitables, false, 0ns, {}, &signal);
  assert(timed_out.first == WaitResult::Timeout);
  assert(signal.Wait(0ns) == WaitResult::Signaled);

  Event same(false);
  gh::kernel::Waitable* same_waitable[] = {&same};
  auto completed = gh::kernel::WaitMultiple(same_waitable, false, 0ns, {}, &same);
  assert(completed.first == WaitResult::Signaled);
  assert(same.Wait(0ns) == WaitResult::Timeout);

  Semaphore full(1, 1);
  auto failed = gh::kernel::WaitMultiple(same_waitable, false, 0ns, {}, &full);
  assert(failed.first == WaitResult::Failed);
  assert(full.count() == 1);
}

void TestTimer() {
  Event fired(false);
  std::atomic<int> calls = 0;
  Timer timer;
  timer.Set(0ns, 0ns, [&] { ++calls; fired.Set(); });
  RequireSignal(fired);
  timer.Cancel();
  assert(calls == 1);

  // Replacing a pending deadline and draining the new callback also proves the
  // canceled callback cannot fire later, without sleeping through its deadline.
  auto target = std::make_shared<int>(1);
  std::weak_ptr<int> retained_target = target;
  timer.Set(1h, 0ns, [&, target] { ++calls; });
  target.reset();
  assert(!retained_target.expired());
  timer.Cancel();
  assert(retained_target.expired());
  timer.Cancel();
  timer.Set(0ns, 0ns, [&] { fired.Set(); });
  RequireSignal(fired);
  timer.Cancel();
  assert(calls == 1);

  timer.Set(0ns, 1ms, [&] {
    if (++calls == 3) {
      timer.Cancel();  // Cancel from the worker must not deadlock.
      fired.Set();
    }
  });
  RequireSignal(fired);
  timer.Cancel();
  assert(calls == 3);

  Event callback_entered(false);
  Event release_callback(false);
  Event cancel_started(false);
  Event cancel_returned(true);
  timer.Set(0ns, 0ns, [&] {
    callback_entered.Set();
    RequireSignal(release_callback);
  });
  RequireSignal(callback_entered);
  Thread canceller([&] {
    cancel_started.Set();
    timer.Cancel();
    cancel_returned.Set();
  });
  RequireSignal(cancel_started);
  assert(cancel_returned.Wait(0ns) == WaitResult::Timeout);
  release_callback.Set();
  canceller.Join();
  RequireSignal(cancel_returned);

  timer.Set(0ns, 1ms, [&] {
    fired.Set();
    throw std::runtime_error("timer failure");
  });
  RequireSignal(fired);
  timer.Cancel();
  try {
    timer.RethrowCallbackException();
    assert(false);
  } catch (const std::runtime_error&) {
  }
  timer.RethrowCallbackException();
}

void TestTimerCaptureCleanup() {
  Timer timer;
  Event released(false);
  struct Target {
    Timer& timer;
    Event& released;
    ~Target() {
      timer.Cancel();
      released.Set();
    }
  };
  auto target = std::shared_ptr<Target>(new Target{timer, released});
  timer.Set(1h, 0ns, [target = std::move(target)] {});
  timer.Cancel();
  RequireSignal(released);

  target = std::shared_ptr<Target>(new Target{timer, released});
  timer.Set(1h, 0ns, [target = std::move(target)] {});
  timer.Set(1h, 0ns, [] {});
  RequireSignal(released);

  target = std::shared_ptr<Target>(new Target{timer, released});
  timer.Set(0ns, 0ns, [target = std::move(target)] {});
  RequireSignal(released);
  timer.Cancel();

  {
    Timer pending;
    target = std::shared_ptr<Target>(new Target{pending, released});
    pending.Set(1h, 0ns, [target = std::move(target)] {});
  }
  RequireSignal(released);
}

void TestQueuedWaits() {
  using gh::kernel::QueuedWait;
  using gh::kernel::Waitable;
  Event pulse(false);
  Waitable* one[] = {&pulse};
  auto first = std::make_unique<QueuedWait>(one, false);
  auto second = std::make_unique<QueuedWait>(one, false);
  auto first_state = first->Save(), second_state = second->Save();
  assert(first_state.order < second_state.order && !first_state.completed);
  first.reset(); second.reset();
  // Rebuild in reverse native-host startup order; the saved dispatcher order wins.
  second = std::make_unique<QueuedWait>(one, false, nullptr, nullptr, nullptr, &second_state);
  first = std::make_unique<QueuedWait>(one, false, nullptr, nullptr, nullptr, &first_state);
  pulse.Pulse();
  assert(!pulse.signaled());
  assert(first->Poll()->first == WaitResult::Signaled && !second->Poll());
  const auto consumed = first->Save();
  first.reset();
  first = std::make_unique<QueuedWait>(one, false, nullptr, nullptr, nullptr, &consumed);
  assert(first->Poll()->first == WaitResult::Signaled);
  pulse.Set();
  assert(second->Poll()->first == WaitResult::Signaled && !pulse.signaled());
  first.reset(); second.reset();

  Semaphore semaphore(1, 3);
  Event gate(false);
  Waitable* both[] = {&semaphore, &gate};
  QueuedWait all(both, true);
  assert(!all.Poll() && semaphore.count() == 1);
  gate.Pulse();
  assert(all.Poll()->first == WaitResult::Signaled && semaphore.count() == 0);

  Event target(false);
  Waitable* target_set[] = {&target};
  auto signal_wait = std::make_unique<QueuedWait>(target_set, false, nullptr, nullptr, &semaphore);
  assert(semaphore.count() == 1);
  const auto signal_state = signal_wait->Save();
  signal_wait.reset();
  signal_wait = std::make_unique<QueuedWait>(target_set, false, nullptr, nullptr, nullptr, &signal_state);
  assert(semaphore.count() == 1 && !signal_wait->Poll());
  target.Pulse();
  assert(signal_wait->Poll()->first == WaitResult::Signaled);

  bool alerted = true, expired = true;
  Event available(false, true);
  Waitable* available_set[] = {&available};
  QueuedWait alert(available_set, false, [&] { return alerted; }, [&] { return expired; });
  assert(alert.Poll()->first == WaitResult::Alerted && available.signaled());
  alerted = false;
  QueuedWait immediate(available_set, false, nullptr, [&] { return expired; });
  assert(immediate.Poll()->first == WaitResult::Signaled);
  QueuedWait timeout(available_set, false, nullptr, [&] { return expired; });
  assert(timeout.Poll()->first == WaitResult::Timeout);
  available.Set();
  assert(timeout.Poll()->first == WaitResult::Timeout && available.signaled());

  Mutant abandoned(true);
  assert(abandoned.Abandon() == 1);
  Waitable* abandoned_set[] = {&abandoned};
  QueuedWait acquire(abandoned_set, false);
  assert(acquire.Poll()->first == WaitResult::Abandoned);
  assert(abandoned.owned_by_current_thread() && abandoned.Release());

  Mutant transferred(true);
  Event transfer_gate(true), registered(false), inspect(false);
  Thread native_owner([&] {
    Waitable* wait_set[] = {&transferred, &transfer_gate};
    QueuedWait waiter(wait_set, true);
    registered.Set(); RequireSignal(inspect);
    assert(waiter.Poll()->first == WaitResult::Abandoned);
    assert(transferred.owned_by_current_thread() && transferred.Release());
  });
  RequireSignal(registered);
  transfer_gate.Set();
  assert(transferred.Abandon() == 1);
  inspect.Set(); native_owner.Join();

  Event mixed(false), native_finished(false);
  std::atomic<unsigned> checked{0};
  Thread earlier_native([&] {
    Waitable* wait_set[] = {&mixed};
    assert(gh::kernel::WaitMultiple(wait_set, false, 2s, [&] {
      ++checked; return false;
    }).first == WaitResult::Signaled);
    native_finished.Set();
  });
  const auto registration_deadline = std::chrono::steady_clock::now() + 2s;
  while (checked < 2 && std::chrono::steady_clock::now() < registration_deadline)
    std::this_thread::yield();
  assert(checked >= 2);
  Waitable* mixed_set[] = {&mixed};
  QueuedWait later_queued(mixed_set, false);
  mixed.Set(); RequireSignal(native_finished);
  assert(!later_queued.Poll());
  mixed.Set(); assert(later_queued.Poll()->first == WaitResult::Signaled);
  earlier_native.Join();
}

void TestThread() {
  gh::kernel::SetThreadLocalValue(0x1234);
  std::uintptr_t other_value = 1;
  Thread tls_thread([&] {
    other_value = gh::kernel::ThreadLocalValue();
    gh::kernel::SetThreadLocalValue(0x5678);
  });
  tls_thread.Join();
  tls_thread.Join();
  assert(!tls_thread.joinable());
  assert(other_value == 0);
  assert(gh::kernel::ThreadLocalValue() == 0x1234);

  bool exited = false;
  { Thread thread([&] { exited = true; }); }
  assert(exited);
  Thread throwing([] { throw std::runtime_error("thread failure"); });
  try {
    throwing.Join();
    assert(false);
  } catch (const std::runtime_error&) {
  }
  try {
    Thread empty({});
    assert(false);
  } catch (const std::invalid_argument&) {
  }
}
}  // namespace

int main() {
  TestEvents();
  TestSemaphore();
  TestMutant();
  TestMutantAbandonment();
  TestCriticalSectionLifetime();
  TestCriticalSectionTerminationAndReuse();
  TestCriticalSectionRestore();
  TestEventPulse();
  TestPulseWaiterOwnership();
  TestMutantOwnerToken();
  TestGuestCriticalSection();
  TestMixedWaitMultiple();
  TestAlertableWaitMultiple();
  TestGuestTimeoutConversion();
  TestSignalAndWait();
  TestTimer();
  TestTimerCaptureCleanup();
  TestQueuedWaits();
  TestThread();
}
