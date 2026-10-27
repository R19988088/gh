#include "gh_guest_continuation.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

namespace gh::guest::continuation {
bool Program::Entry::ContainsPC(uint32_t pc) const {
  if (contiguous_pcs)
    return pc >= pcs.front() && pc <= pcs.back() && ((pc - pcs.front()) & 3u) == 0;
  return std::binary_search(pcs.begin(), pcs.end(), pc);
}
void Program::AddCompiled(uint32_t address, std::vector<uint32_t> pcs, Step step) {
  std::sort(pcs.begin(), pcs.end());
  if (!step || !std::binary_search(pcs.begin(), pcs.end(), address) ||
      entries_.contains(address)) throw std::invalid_argument("invalid continuation entry");
  const bool contiguous = std::adjacent_find(pcs.begin(), pcs.end(),
      [](uint32_t left, uint32_t right) { return right - left != 4; }) == pcs.end();
  entries_.emplace(address, Entry{std::move(pcs), step, nullptr, {}, nullptr, contiguous});
}
void Program::AddAtomicHost(uint32_t address, Host host) {
  if (!host) throw std::invalid_argument("invalid host entry");
  entries_.insert_or_assign(address, Entry{{address}, nullptr, host, {}});
}
void Program::AddQueuedHost(uint32_t address, BeginHost begin) {
  if (!begin) throw std::invalid_argument("invalid queued host entry");
  entries_.insert_or_assign(address, Entry{{address}, nullptr, nullptr, {}, begin});
}
void Program::AddUnsupported(uint32_t address, std::string reason) {
  if (reason.empty() || entries_.contains(address))
    throw std::invalid_argument("invalid unsupported continuation entry");
  entries_.emplace(address, Entry{{address}, nullptr, nullptr, std::move(reason)});
}
const Program::Entry* Program::Find(uint32_t address) const {
  auto it = entries_.find(address);
  return it == entries_.end() ? nullptr : &it->second;
}
bool Machine::Valid(const Frame& frame) const {
  auto* entry = program_.Find(frame.function);
  return entry && (entry->step || entry->host || entry->begin) &&
         entry->ContainsPC(frame.pc);
}
void Machine::Start(uint32_t entry) {
  if (running_ || !frames_.empty()) throw std::logic_error("continuation already active");
  if (!Valid({entry, entry})) {
    const auto* target = program_.Find(entry);
    throw std::invalid_argument(target && !target->unsupported.empty() ? target->unsupported :
                                "unsupported continuation entry");
  }
  frames_.push_back({entry, entry});
  faulted_ = false;
}
RunResult Machine::Run(uint8_t* memory, size_t budget) {
  if (running_ || faulted_) throw std::logic_error("continuation is not resumable");
  running_ = true;
  const auto host_fpscr = GuestFPSCRRegister::getHostCSR();
  GuestFPSCRRegister::setHostCSR(context.fpscr.csr);
  struct Scope {
    Machine& machine; uint32_t fpscr;
    ~Scope() {
      machine.context.fpscr.csr = GuestFPSCRRegister::getHostCSR();
      GuestFPSCRRegister::setHostCSR(fpscr);
      machine.running_ = false;
    }
  } scope{*this, host_fpscr};
  size_t steps = 0;
  const Program::Entry* entry = nullptr;
  uint32_t current_function = 0;
  try {
    while (!frames_.empty() && steps < budget) {
      const auto function = frames_.back().function;
      if (!entry || function != current_function) {
        entry = program_.Find(function);
        current_function = function;
      }
      if (!entry || (!entry->host && !entry->step && !entry->begin) ||
          !entry->ContainsPC(frames_.back().pc))
        throw std::runtime_error("invalid continuation PC");
      if (entry->begin) {
        PreparePendingHost(memory);
        if (!pending_->Poll(context, memory)) return {Status::Blocked, steps, {}};
        pending_.reset(); saved_pending_.clear(); frames_.pop_back();
      } else if (entry->host) {
        entry->host(context, memory);
        frames_.pop_back();
      } else {
        const auto transition = entry->step(context, memory, frames_.back());
        switch (transition.kind) {
          case Transition::Kind::Next:
            frames_.back().pc = transition.pc;
            if (!entry->ContainsPC(transition.pc))
              throw std::runtime_error("invalid next PC");
            break;
          case Transition::Kind::Call:
          case Transition::Kind::TailCall: {
            if (!Valid({transition.target, transition.target})) {
              // The step may already have set LR, matching the original call.
              // Its host/guest target has not executed and cannot be replayed.
              faulted_ = true;
              const auto* target = program_.Find(transition.target);
              return {Status::Unsupported, steps, target && !target->unsupported.empty() ?
                      target->unsupported : "unregistered guest, hook or import target"};
            }
            if (transition.kind == Transition::Kind::Call) {
              frames_.back().pc = transition.pc;
              if (!entry->ContainsPC(transition.pc))
                throw std::runtime_error("invalid return PC");
            } else {
              frames_.pop_back();
            }
            frames_.push_back({transition.target, transition.target});
            break;
          }
          case Transition::Kind::Return: frames_.pop_back(); break;
        }
      }
      ++steps;
    }
    return {frames_.empty() ? Status::Complete : Status::Paused, steps, {}};
  } catch (...) {
    faulted_ = true;
    throw;
  }
}
void Machine::PreparePendingHost(uint8_t* memory, bool saved_only) {
  if (pending_ || frames_.empty() || (saved_only && saved_pending_.empty())) return;
  const auto* entry = program_.Find(frames_.back().function);
  if (!entry || !entry->begin) return;
  pending_ = entry->begin(context, memory, saved_pending_);
  if (!pending_) throw std::runtime_error("queued host did not create operation");
  saved_pending_.clear();
}
void Machine::WaitForWake(std::chrono::milliseconds maximum) {
  if (pending_) pending_->WaitForWake(maximum);
}
namespace {
void Write32(std::vector<uint8_t>& out, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) out.push_back(uint8_t(value >> (i * 8)));
}
uint32_t Read32(std::span<const uint8_t> bytes, size_t& offset) {
  if (bytes.size() - offset < 4) throw std::invalid_argument("truncated continuation");
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i) value |= uint32_t(bytes[offset++]) << (i * 8);
  return value;
}
}
std::vector<uint8_t> Machine::Save() const {
  if (running_ || faulted_) throw std::logic_error("snapshot requires a guest safepoint");
  static_assert(std::is_trivially_copyable_v<GuestContext>);
  if (frames_.size() > UINT32_MAX) throw std::length_error("too many guest frames");
  std::vector<uint8_t> bytes;
  Write32(bytes, 0x33434847);  // GHC3 adds a queued import record.
  Write32(bytes, sizeof(GuestContext));
  Write32(bytes, uint32_t(frames_.size()));
  auto* raw = reinterpret_cast<const uint8_t*>(&context);
  bytes.insert(bytes.end(), raw, raw + sizeof(context));
  for (const auto frame : frames_) {
    if (!Valid(frame)) throw std::logic_error("unsupported snapshot frame");
    Write32(bytes, frame.function); Write32(bytes, frame.pc);
    Write32(bytes, frame.ea);
    Write32(bytes, uint32_t(frame.temp.u64)); Write32(bytes, uint32_t(frame.temp.u64 >> 32));
    bytes.insert(bytes.end(), frame.vTemp.u8, frame.vTemp.u8 + 16);
  }
  const auto pending = pending_ ? pending_->Save() : saved_pending_;
  Write32(bytes, uint32_t(pending.size()));
  bytes.insert(bytes.end(), pending.begin(), pending.end());
  return bytes;
}
void Machine::Restore(std::span<const uint8_t> bytes) {
  if (running_) throw std::logic_error("restore requires a guest safepoint");
  size_t offset = 0;
  const auto signature = Read32(bytes, offset);
  if ((signature != 0x32434847 && signature != 0x33434847) || Read32(bytes, offset) != sizeof(GuestContext))
    throw std::invalid_argument("incompatible continuation ABI");
  const auto count = Read32(bytes, offset);
  if (bytes.size() - offset < sizeof(GuestContext) ||
      (bytes.size() - offset - sizeof(GuestContext)) / 36 < count)
    throw std::invalid_argument("invalid continuation size");
  GuestContext restored{};
  std::memcpy(&restored, bytes.data() + offset, sizeof(restored));
  offset += sizeof(restored);
  std::vector<Frame> frames;
  frames.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    Frame frame{Read32(bytes, offset), Read32(bytes, offset)};
    frame.ea = Read32(bytes, offset);
    frame.temp.u64 = Read32(bytes, offset);
    frame.temp.u64 |= uint64_t(Read32(bytes, offset)) << 32;
    std::memcpy(frame.vTemp.u8, bytes.data() + offset, 16);
    offset += 16;
    if (!Valid(frame)) throw std::invalid_argument("unsupported continuation PC");
    frames.push_back(frame);
  }
  std::vector<uint8_t> pending;
  if (signature == 0x33434847) {
    const auto size = Read32(bytes, offset);
    if (size > bytes.size() - offset) throw std::invalid_argument("truncated queued import");
    pending.assign(bytes.begin() + offset, bytes.begin() + offset + size); offset += size;
    if (size && (frames.empty() || !program_.Find(frames.back().function)->begin))
      throw std::invalid_argument("queued state has no queued import frame");
  }
  if (offset != bytes.size()) throw std::invalid_argument("invalid continuation size");
  pending_.reset(); saved_pending_ = std::move(pending);
  context = restored;
  frames_ = std::move(frames);
  faulted_ = false;
}
}  // namespace gh::guest::continuation
