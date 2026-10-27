// PM4 execution derived from Xenia, Copyright 2022 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gh::graphics {

class PacketReader {
 public:
  virtual ~PacketReader() = default;
  virtual std::size_t RemainingWords() const = 0;
  virtual uint32_t ReadWord() = 0;
  virtual void SkipWords(uint32_t count) = 0;
  virtual const void* CurrentBytes() const = 0;
};

struct DrawCommand {
  uint32_t initiator = 0;
  uint32_t primitive = 0;
  uint32_t index_count = 0;
  uint32_t index_address = 0;
  uint32_t index_length = 0;
  uint32_t index_endian = 0;
  uint32_t index_format = 0;
  bool indexed = false;
  bool explicit_major_mode = false;
};

// Backend-specific register side effects and physical mappings stay at the
// device boundary. Packet decoding and guest-visible memory operations do not.
class CommandTarget {
 public:
  virtual ~CommandTarget() = default;
  virtual uint32_t ReadRegister(uint32_t index) = 0;
  virtual void WriteRegister(uint32_t index, uint32_t value) = 0;
  virtual uint32_t ReadPhysical(uint32_t address) = 0;
  virtual void WritePhysical(uint32_t address, uint32_t value) = 0;
  virtual void WriteRegisterRange(PacketReader& reader, uint32_t index, uint32_t count);
  virtual void LoadRegisterRange(uint32_t address, uint32_t index, uint32_t count);
  virtual void MakeCoherent() = 0;
  virtual bool PollDelay(uint32_t interval) = 0;
  virtual bool SubmitDraw(const DrawCommand& command) = 0;
  // A null code pointer identifies guest physical memory; otherwise code is
  // the original big-endian packet storage, preserving shader cache identity.
  virtual void LoadShader(uint32_t type, uint32_t address, const void* code,
                          uint32_t words) = 0;
};

enum class PacketResult { kExecuted, kDeviceCommand, kInvalid };

class GuestCommandProcessor {
 public:
  // header has already been read. Device commands leave the payload untouched
  // for interrupt and presentation adapters.
  PacketResult Execute(uint32_t header, PacketReader& reader, CommandTarget& target);

  void RestoreBinState(uint64_t select, uint64_t mask) noexcept {
    bin_select_ = select;
    bin_mask_ = mask;
  }

  uint64_t bin_mask() const noexcept { return bin_mask_; }
  uint64_t bin_select() const noexcept { return bin_select_; }

 private:
  PacketResult Draw(uint32_t opcode, uint32_t count, PacketReader& reader,
                    CommandTarget& target);
  uint64_t bin_select_ = 0xFFFFFFFFull;
  uint64_t bin_mask_ = 0xFFFFFFFFull;
  std::vector<uint32_t> microcode_;
};

}  // namespace gh::graphics
