#include "gh_guest_continuation_title.h"
#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <unordered_set>

namespace gh::guest::continuation {
namespace {
std::atomic<const Program*> title_program{nullptr};
thread_local Machine* current_execution = nullptr;
thread_local TitleCoordinator* current_coordinator = nullptr;
thread_local uint32_t current_thread_id = 0;
thread_local bool (*current_can_pause)() = nullptr;
}

struct TitleCoordinator::State {
  struct Entry {
    uint64_t incarnation;
    Machine* machine = nullptr;
    bool parked = false;
  };
  std::mutex mutex;
  std::condition_variable changed;
  std::unordered_map<uint32_t, Entry> entries;
  bool requested = false;
  uint64_t generation = 0, next_incarnation = 0;

  bool Ready(std::span<const uint32_t> expected) const {
    for (auto id : expected) {
      const auto it = entries.find(id);
      if (it == entries.end() || !it->second.machine || !it->second.parked) return false;
    }
    return std::all_of(entries.begin(), entries.end(), [](const auto& item) {
      return item.second.machine && item.second.parked;
    });
  }
  void Release(uint64_t owner) {
    if (requested && generation == owner) {
      requested = false;
      changed.notify_all();
    }
  }
  bool Poll(uint32_t id, bool (*can_pause)()) {
    std::unique_lock lock(mutex);
    if (!requested) return false;
    // Guest lock queries may acquire kernel locks. Never hold this mutex while
    // asking them, or a thread entering its execution domain could deadlock.
    lock.unlock();
    if (!can_pause || !can_pause()) return false;
    lock.lock();
    if (!requested) return false;
    auto& entry = entries.at(id);
    entry.parked = true;
    changed.notify_all();
    changed.wait(lock, [&] { return !requested; });
    entry.parked = false;
    return true;
  }
};

TitleCoordinator::TitleCoordinator() : state_(std::make_shared<State>()) {}
TitleCoordinator::~TitleCoordinator() { Cancel(); }
TitleCoordinator::Registration::Registration(TitleCoordinator& coordinator, uint32_t id,
                                            bool (*can_pause)())
    : state_(coordinator.state_), thread_id_(id) {
  if (current_coordinator) throw std::logic_error("guest execution is already registered");
  std::unique_lock lock(state_->mutex);
  if (!state_->entries.emplace(id, State::Entry{++state_->next_incarnation}).second)
    throw std::logic_error("guest thread is already registered");
  current_coordinator = &coordinator;
  current_thread_id = id;
  current_can_pause = can_pause;
  state_->changed.notify_all();
  // A new thread cannot enter guest startup/APCs during an existing stop. Its
  // missing Machine also prevents a pending stop from reporting full capture.
  state_->changed.wait(lock, [&] { return !state_->requested; });
}
TitleCoordinator::Registration::~Registration() {
  std::lock_guard lock(state_->mutex);
  state_->entries.erase(thread_id_);
  state_->changed.notify_all();
  current_coordinator = nullptr;
  current_can_pause = nullptr;
  current_thread_id = 0;
}
void TitleCoordinator::Cancel() {
  std::lock_guard lock(state_->mutex);
  state_->requested = false;
  ++state_->generation;
  state_->changed.notify_all();
}
TitleCoordinator::PauseResult TitleCoordinator::Pause(
    std::span<const uint32_t> expected, std::chrono::milliseconds timeout) {
  if (current_coordinator == this)
    return {nullptr, "a guest thread cannot stop its own execution domain"};
  std::unique_lock lock(state_->mutex);
  if (state_->requested) return {nullptr, "guest continuation stop is already active"};
  const auto generation = ++state_->generation;
  state_->requested = true;
  state_->changed.notify_all();
  const bool reached = state_->changed.wait_for(lock, timeout, [&] {
    return !state_->requested || generation != state_->generation || state_->Ready(expected);
  });
  if (!state_->requested || generation != state_->generation)
    return {nullptr, "guest continuation stop was cancelled"};
  if (!reached) {
    state_->Release(generation);
    return {nullptr, "unfinished guest threads did not reach resumable, unlocked safepoints"};
  }
  try {
    std::vector<ThreadImage> threads;
    threads.reserve(state_->entries.size());
    for (const auto& [id, entry] : state_->entries)
      threads.push_back({id, entry.incarnation, entry.machine->Save()});
    std::sort(threads.begin(), threads.end(), [](const auto& a, const auto& b) {
      return a.thread_id < b.thread_id;
    });
    return {std::unique_ptr<Stop>(new Stop(state_, generation, std::move(threads))), {}};
  } catch (const std::exception& error) {
    state_->Release(generation);
    return {nullptr, error.what()};
  }
}
TitleCoordinator::Stop::Stop(std::shared_ptr<State> state, uint64_t generation,
                             std::vector<ThreadImage> threads)
    : state_(std::move(state)), generation_(generation), threads_(std::move(threads)) {}
TitleCoordinator::Stop::~Stop() {
  std::lock_guard lock(state_->mutex);
  state_->Release(generation_);
}
bool TitleCoordinator::Stop::IsActiveFor(const TitleCoordinator& coordinator) const {
  if (state_ != coordinator.state_) return false;
  std::lock_guard lock(state_->mutex);
  return state_->requested && state_->generation == generation_ && state_->Ready({});
}
void TitleCoordinator::RestoreStopped(const std::shared_ptr<State>& state, uint64_t generation,
                                      std::span<const ThreadImage> threads) {
  std::lock_guard lock(state->mutex);
  if (!state->requested || state->generation != generation || !state->Ready({}))
    throw std::logic_error("guest continuation stop no longer owns parked threads");
  if (threads.size() != state->entries.size())
    throw std::invalid_argument("continuation thread set changed");
  std::unordered_set<uint32_t> seen;
  std::vector<std::pair<Machine*, std::unique_ptr<Machine>>> prepared;
  prepared.reserve(threads.size());
  // Prepare every frame/context before publishing any of them. An invalid PC,
  // changed thread incarnation or allocation failure leaves all threads intact.
  for (const auto& image : threads) {
    const auto it = state->entries.find(image.thread_id);
    if (it == state->entries.end() || it->second.incarnation != image.incarnation ||
        !seen.insert(image.thread_id).second)
      throw std::invalid_argument("continuation thread identity changed");
    auto* machine = it->second.machine;
    auto restored = std::make_unique<Machine>(machine->program_);
    restored->Restore(image.execution);
    if (machine->pending_ || !machine->saved_pending_.empty() ||
        !restored->saved_pending_.empty())
      throw std::invalid_argument("queued imports require a full kernel and memory restore");
    prepared.emplace_back(machine, std::move(restored));
  }
  for (auto& [machine, restored] : prepared) {
    machine->context = restored->context;
    machine->frames_.swap(restored->frames_);
    machine->faulted_ = false;
  }
}
void TitleCoordinator::Stop::Restore(std::span<const ThreadImage> threads) {
  std::vector<ThreadImage> copied(threads.begin(), threads.end());
  TitleCoordinator::RestoreStopped(state_, generation_, copied);
  threads_ = std::move(copied);
}

void InstallTitleProgram(const Program* program) { title_program.store(program); }
const Program* TitleProgram() { return title_program.load(); }
Machine* CurrentTitleExecution() { return current_execution; }

void RunTitleMachine(Machine& machine, GuestContext& context, uint8_t* base,
                     Safepoint safepoint) {
  auto coordinator = current_coordinator ? current_coordinator->state_ : nullptr;
  struct Scope {
    Machine& machine;
    GuestContext& context;
    Machine* previous;
    std::shared_ptr<TitleCoordinator::State> coordinator;
    uint32_t thread_id;
    ~Scope() {
      if (coordinator) {
        std::lock_guard lock(coordinator->mutex);
        coordinator->entries.at(thread_id).machine = nullptr;
        coordinator->changed.notify_all();
      }
      context = machine.context;
      current_execution = previous;
    }
  } scope{machine, context, current_execution, coordinator, current_thread_id};
  if (coordinator) {
    std::lock_guard lock(coordinator->mutex);
    coordinator->entries.at(current_thread_id).machine = &machine;
    coordinator->changed.notify_all();
  }
  current_execution = &machine;
  if (coordinator) coordinator->Poll(current_thread_id, current_can_pause);
  for (;;) {
    if (safepoint) safepoint();
    const auto result = machine.Run(base, 4096);
    context = machine.context;
    if (result.status == Status::Unsupported || result.status == Status::Faulted)
      throw std::runtime_error("title continuation stopped: " + result.reason);
    if (safepoint) safepoint();
    if (coordinator && coordinator->Poll(current_thread_id, current_can_pause)) {
      context = machine.context;
      if (safepoint) safepoint();
    }
    // Restore may replace a completed machine with a previously saved frame.
    if (machine.frames().empty()) return;
    if (result.status == Status::Blocked) machine.WaitForWake(std::chrono::milliseconds(1));
  }
}
void ExecuteTitle(const Program& program, GuestContext& context, uint8_t* base,
                  uint32_t address, Safepoint safepoint) {
  Machine machine(program);
  machine.context = context;
  machine.Start(address);
  RunTitleMachine(machine, context, base, safepoint);
}
void ResumeTitle(const Program& program, GuestContext& context, uint8_t* base,
                 std::span<const uint8_t> image, Safepoint safepoint) {
  Machine machine(program);
  machine.Restore(image);
  RunTitleMachine(machine, context, base, safepoint);
}
void ResumeTitle(Machine& machine, GuestContext& context, uint8_t* base, Safepoint safepoint) {
  RunTitleMachine(machine, context, base, safepoint);
}
}  // namespace gh::guest::continuation
