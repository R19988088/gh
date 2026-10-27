#include "gh_guest_continuation_title.h"
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

namespace cont = gh::guest::continuation;
using namespace std::chrono_literals;

struct Control {
  std::atomic<bool> finish{false}, can_pause{true}, entered{false};
  std::mutex mutex;
  std::condition_variable changed;
  bool released = false;
  uint64_t result = 0;
};
thread_local Control* control;
static bool CanPause() { return control->can_pause.load(); }
static cont::Transition Count(GuestContext& context, uint8_t*, cont::Frame&) {
  if (!control->entered.exchange(true)) {
    std::lock_guard lock(control->mutex);
    control->changed.notify_all();
  }
  if (control->finish.load()) return cont::Transition::Return();
  ++context.r3.u64;
  return cont::Transition::Next(0x1000);
}
static void AtomicWait(GuestContext&, uint8_t*) {
  std::unique_lock lock(control->mutex);
  control->entered = true;
  control->changed.notify_all();
  control->changed.wait(lock, [] { return control->released; });
}
static std::thread Worker(cont::TitleCoordinator& coordinator, uint32_t id, Control& state,
                          uint32_t address = 0x1000) {
  return std::thread([&, id, address] {
    control = &state;
    cont::TitleCoordinator::Registration registration(coordinator, id, CanPause);
    const std::array self{id};
    assert(!coordinator.Pause(self, 1ms).stop);
    GuestContext context{};
    context.fpscr.csr = GuestFPSCRRegister::getHostCSR();
    assert(cont::TryExecuteTitle(context, nullptr, address));
    state.result = context.r3.u64;
  });
}
static std::vector<uint8_t> WithCounter(const cont::Program& program,
                                      const cont::TitleCoordinator::ThreadImage& image,
                                      uint64_t counter) {
  cont::Machine machine(program);
  machine.Restore(image.execution);
  machine.context.r3.u64 = counter;
  return machine.Save();
}
static void Release(Control& state) {
  std::lock_guard lock(state.mutex);
  state.released = true;
  state.changed.notify_all();
}
static void WaitEntered(Control& state) {
  std::unique_lock lock(state.mutex);
  assert(state.changed.wait_for(lock, 2s, [&] { return state.entered.load(); }));
}

int main() {
  cont::Program program;
  program.AddCompiled(0x1000, {0x1000}, Count);
  program.AddAtomicHost(0x2000, AtomicWait);
  cont::InstallTitleProgram(&program);
  const std::array ids{1u, 2u};
  {
    cont::TitleCoordinator coordinator;
    Control first, second;
    auto a = Worker(coordinator, 1, first), b = Worker(coordinator, 2, second);
    WaitEntered(first); WaitEntered(second);
    auto paused = coordinator.Pause(ids, 2s);
    assert(paused.stop && paused.stop->threads().size() == 2);
    auto accepted = paused.stop->threads();
    for (auto& image : accepted)
      image.execution = WithCounter(program, image, 1000 + image.thread_id);
    paused.stop->Restore(accepted);
    auto invalid = accepted;
    invalid[0].execution = WithCounter(program, invalid[0], 9999);
    invalid[1].execution.pop_back();
    bool rejected = false;
    try { paused.stop->Restore(invalid); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    // The invalid second image must not have published the valid first one.
    first.finish = second.finish = true;
    paused.stop.reset();
    a.join(); b.join();
    assert(first.result == 1001 && second.result == 1002);
  }
  {
    cont::TitleCoordinator coordinator;
    Control state;
    state.can_pause = false;
    auto worker = Worker(coordinator, 1, state);
    WaitEntered(state);
    const std::array missing{1u, 999u};
    assert(!coordinator.Pause(missing, 30ms).stop);
    const std::array expected{1u};
    assert(!coordinator.Pause(expected, 30ms).stop);
    state.can_pause = true;
    auto paused = coordinator.Pause(expected, 2s);
    assert(paused.stop);
    state.finish = true;
    coordinator.Cancel();
    bool rejected = false;
    try { paused.stop->Restore(paused.stop->threads()); }
    catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    worker.join();
  }
  {
    cont::TitleCoordinator coordinator;
    Control state, waiting;
    auto worker = Worker(coordinator, 1, state);
    WaitEntered(state);
    std::thread unstarted([&] {
      control = &waiting;
      cont::TitleCoordinator::Registration registration(coordinator, 2, CanPause);
      std::unique_lock lock(waiting.mutex);
      waiting.entered = true;
      waiting.changed.notify_all();
      waiting.changed.wait(lock, [&] { return waiting.released; });
    });
    {
      std::unique_lock lock(waiting.mutex);
      assert(waiting.changed.wait_for(lock, 2s, [&] { return waiting.entered.load(); }));
    }
    assert(!coordinator.Pause(ids, 30ms).stop);
    state.finish = true;
    Release(waiting);
    worker.join(); unstarted.join();
  }
  {
    cont::TitleCoordinator coordinator;
    Control state;
    auto worker = Worker(coordinator, 1, state, 0x2000);
    {
      std::unique_lock lock(state.mutex);
      assert(state.changed.wait_for(lock, 2s, [&] { return state.entered.load(); }));
    }
    const std::array expected{1u};
    assert(!coordinator.Pause(expected, 30ms).stop);
    Release(state);
    worker.join();
  }
  cont::InstallTitleProgram(nullptr);
  std::cout << "continuation stop: two-thread capture/restore, atomic rejection, lock gate, "
               "missing frames, self-stop and cancellation passed\n";
}
