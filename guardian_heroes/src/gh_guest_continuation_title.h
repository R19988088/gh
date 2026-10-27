#pragma once

#include "gh_guest_continuation.h"
#include <chrono>
#include <memory>
#include <cstring>
#include <type_traits>

namespace gh::guest::continuation {
// Stops only guest execution at resumable frame boundaries. Guest memory,
// kernel queues and device workers require their own coordinated snapshot.
class TitleCoordinator {
  struct State;
 public:
  struct ThreadImage {
    uint32_t thread_id;
    uint64_t incarnation;
    std::vector<uint8_t> execution;
  };
  class Registration {
   public:
    Registration(TitleCoordinator&, uint32_t thread_id, bool (*can_pause)());
    ~Registration();
    Registration(const Registration&) = delete;
    Registration& operator=(const Registration&) = delete;
   private:
    std::shared_ptr<State> state_;
    uint32_t thread_id_;
  };
  class Stop {
   public:
    ~Stop();
    Stop(const Stop&) = delete;
    Stop& operator=(const Stop&) = delete;
    const std::vector<ThreadImage>& threads() const { return threads_; }
    bool IsActiveFor(const TitleCoordinator&) const;
    // In-place frame restore, while this stop still owns the same live threads.
    // The caller must restore guest memory and other state at this same stop.
    void Restore(std::span<const ThreadImage> threads);
   private:
    friend class TitleCoordinator;
    Stop(std::shared_ptr<State>, uint64_t, std::vector<ThreadImage>);
    std::shared_ptr<State> state_;
    uint64_t generation_;
    std::vector<ThreadImage> threads_;
  };
  struct PauseResult {
    std::unique_ptr<Stop> stop;
    std::string reason;
  };
  TitleCoordinator();
  ~TitleCoordinator();
  TitleCoordinator(const TitleCoordinator&) = delete;
  TitleCoordinator& operator=(const TitleCoordinator&) = delete;
  // Include every unfinished guest thread, including created/suspended ones.
  // Missing machines, atomic host waits and unsafe lock scopes time out.
  PauseResult Pause(std::span<const uint32_t> expected_threads,
                    std::chrono::milliseconds timeout);
  void Cancel();
 private:
  friend void RunTitleMachine(Machine&, GuestContext&, uint8_t*, void (*)());
  static void RestoreStopped(const std::shared_ptr<State>&, uint64_t,
                             std::span<const ThreadImage>);
  std::shared_ptr<State> state_;
};

// Configure before starting title threads. The immutable program must outlive
// every executing thread. Null keeps the original compiled execution path.
void InstallTitleProgram(const Program* program);
const Program* TitleProgram();
// Defined by gh_guest_continuation_title_codegen.py output when that output is linked.
Program CreateTitleContinuationProgram();
Machine* CurrentTitleExecution();
using Safepoint = void (*)();
void ExecuteTitle(const Program&, GuestContext&, uint8_t*, uint32_t, Safepoint);
// Starts on a fresh native thread; the image carries the entire guest stack.
void ResumeTitle(const Program&, GuestContext&, uint8_t*, std::span<const uint8_t>, Safepoint);
void ResumeTitle(Machine&, GuestContext&, uint8_t*, Safepoint);

template <class Context>
bool TryExecuteTitle(Context& context, uint8_t* base, uint32_t address,
                     Safepoint safepoint = nullptr) {
  const auto* program = TitleProgram();
  // Nested callbacks from an atomic host entry retain their original C++
  // execution. They cannot become an independently resumable guest stack.
  if (!program || CurrentTitleExecution()) return false;
  static_assert(std::is_trivially_copyable_v<Context>);
  static_assert(sizeof(Context) == sizeof(GuestContext));
  static_assert(alignof(Context) == alignof(GuestContext));
  GuestContext guest_context;
  std::memcpy(&guest_context, &context, sizeof(guest_context));
  struct CopyBack {
    Context& context;
    GuestContext& guest;
    ~CopyBack() { std::memcpy(&context, &guest, sizeof(guest)); }
  } copy_back{context, guest_context};
  ExecuteTitle(*program, guest_context, base, address, safepoint);
  return true;
}
}  // namespace gh::guest::continuation
