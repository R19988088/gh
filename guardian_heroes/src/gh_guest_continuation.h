#pragma once

#include "guest_context.h"
#include <cstdint>
#include <chrono>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace gh::guest::continuation {
struct Frame {
  uint32_t function, pc;
  uint32_t ea = 0;
  GuestRegister temp{};
  GuestVectorRegister vTemp{};
};
struct Transition {
  enum class Kind { Next, Call, TailCall, Return } kind;
  uint32_t pc = 0, target = 0;
  static Transition Next(uint32_t pc) { return {Kind::Next, pc}; }
  static Transition Call(uint32_t target, uint32_t resume) { return {Kind::Call, resume, target}; }
  static Transition TailCall(uint32_t target) { return {Kind::TailCall, 0, target}; }
  static Transition Return() { return {Kind::Return}; }
};
using Step = Transition (*)(GuestContext&, uint8_t*, Frame&);
using Host = void (*)(GuestContext&, uint8_t*);
class PendingHost {
 public:
  virtual ~PendingHost() = default;
  virtual bool Poll(GuestContext&, uint8_t*) = 0;
  virtual std::vector<uint8_t> Save() const = 0;
  virtual void WaitForWake(std::chrono::milliseconds) = 0;
};
using BeginHost = std::unique_ptr<PendingHost> (*)(GuestContext&, uint8_t*, std::span<const uint8_t>);
class Program {
 public:
  struct Entry {
    std::vector<uint32_t> pcs;
    Step step = nullptr;
    Host host = nullptr;
    std::string unsupported;
    BeginHost begin = nullptr;
    bool contiguous_pcs = false;
    bool ContainsPC(uint32_t pc) const;
  };
  void AddCompiled(uint32_t address, std::vector<uint32_t> pcs, Step step);
  // A hook/import may run atomically, but must finish before a snapshot. Its
  // nested guest calls still use the original path and are not continuations.
  // Registration replaces a compiled entry when that guest address is hooked.
  void AddAtomicHost(uint32_t address, Host host);
  void AddQueuedHost(uint32_t address, BeginHost begin);
  void AddUnsupported(uint32_t address, std::string reason);
  const Entry* Find(uint32_t address) const;
 private:
  std::unordered_map<uint32_t, Entry> entries_;
};
enum class Status { Paused, Complete, Unsupported, Faulted, Blocked };
struct RunResult { Status status; size_t steps; std::string reason; };
class Machine {
 public:
  explicit Machine(const Program& program) : program_(program) {}
  GuestContext context{};
  void Start(uint32_t entry);
  RunResult Run(uint8_t* memory, size_t instruction_budget);
  // Same executable/GuestContext ABI only. The caller captures guest memory,
  // other threads, wait queues and devices separately at the same safepoint.
  std::vector<uint8_t> Save() const;
  void Restore(std::span<const uint8_t> bytes);
  // Restore binding runs on the new native owner before the kernel start gate.
  void PreparePendingHost(uint8_t* memory, bool saved_only = false);
  void WaitForWake(std::chrono::milliseconds maximum);
  std::span<const Frame> frames() const { return frames_; }
 private:
  friend class TitleCoordinator;
  bool Valid(const Frame&) const;
  const Program& program_;
  std::vector<Frame> frames_;
  std::unique_ptr<PendingHost> pending_;
  std::vector<uint8_t> saved_pending_;
  bool running_ = false, faulted_ = false;
};
}  // namespace gh::guest::continuation
