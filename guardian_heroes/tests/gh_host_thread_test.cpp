#include "gh_kernel.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

using namespace std::chrono_literals;

int main() {
  using gh::kernel::HostThread;
  gh::kernel::Event started(true);
  std::atomic<bool> stop = false;
  std::atomic<unsigned> work = 0;
  auto worker = HostThread::Create({.create_suspended = true}, [&] {
    started.Set();
    while (!stop.load()) ++work;
  });
  assert(worker && work == 0);
  std::uint32_t previous = 0;
  assert(worker->Resume(&previous) && previous == 1);
  assert(started.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(worker->Suspend(&previous) && previous == 0);
  const auto paused_work = work.load();
  assert(worker->Suspend(&previous) && previous == 1);
  assert(worker->Resume(&previous) && previous == 2);
  std::this_thread::sleep_for(3ms);
  assert(work == paused_work);
  assert(worker->Resume(&previous) && previous == 1);
  const auto deadline = std::chrono::steady_clock::now() + 2s;
  while (work == paused_work && std::chrono::steady_clock::now() < deadline)
    std::this_thread::yield();
  assert(work != paused_work);
  stop = true;
  worker->Join();

  gh::kernel::Event ready(true), finish_ready(true), resumed(true);
  HostThread* self = nullptr;
  auto self_suspend = HostThread::Create({.create_suspended = true}, [&] {
    assert(self->Suspend(nullptr, [&] {
      ready.Set();
      assert(finish_ready.Wait(2s) == gh::kernel::WaitResult::Signaled);
    }));
    resumed.Set();
  });
  assert(self_suspend);
  self = self_suspend.get();
  assert(self_suspend->Resume());
  assert(ready.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(self_suspend->Resume(&previous) && previous == 1);
  finish_ready.Set();
  assert(resumed.Wait(2s) == gh::kernel::WaitResult::Signaled);
  self_suspend->Join();

  unsigned callback_order = 0;
  auto callbacks = HostThread::Create({.create_suspended = true}, [&] {
    assert(HostThread::DispatchUserCallbacks());
    assert(HostThread::DispatchUserCallbacks());
    assert(!HostThread::DispatchUserCallbacks());
  });
  assert(callbacks);
  callbacks->QueueUserCallback([&] { assert(callback_order++ == 0); });
  callbacks->QueueUserCallback([&] { assert(callback_order++ == 1); });
  assert(callbacks->Resume());
  callbacks->Join();
  assert(callback_order == 2);

  auto exits = HostThread::Create({}, [] { HostThread::Exit(7); });
  assert(exits);
  exits->Join();

  gh::kernel::Event cancel_ready(true);
  std::atomic<unsigned> cancel_work = 0;
  auto cancelled = HostThread::Create({}, [&] {
    cancel_ready.Set();
    for (;;) {
      ++cancel_work;
      std::this_thread::sleep_for(1ms);
    }
  });
  assert(cancelled);
  assert(cancel_ready.Wait(2s) == gh::kernel::WaitResult::Signaled);
  cancelled->Terminate(0);
  const auto ended_work = cancel_work.load();
  std::this_thread::sleep_for(3ms);
  assert(cancel_work == ended_work);
  std::puts("host threads: suspended startup, nested native suspend, early resume, callbacks, exit and joined waiting-thread cancellation passed");
}
