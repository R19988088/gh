#include "gh_gpu_commands.h"

#include <array>
#include <cassert>
#include <span>
#include <unordered_map>
#include <vector>

namespace {
class Reader final : public gh::graphics::PacketReader {
 public:
  explicit Reader(std::span<const uint32_t> words) : words_(words) {}
  size_t RemainingWords() const override { return words_.size() - position_; }
  uint32_t ReadWord() override { assert(RemainingWords()); return words_[position_++]; }
  void SkipWords(uint32_t count) override { assert(RemainingWords() >= count); position_ += count; }
  const void* CurrentBytes() const override { return words_.data() + position_; }
 private:
  std::span<const uint32_t> words_;
  size_t position_ = 0;
};

class Target final : public gh::graphics::CommandTarget {
 public:
  uint32_t ReadRegister(uint32_t index) override { return registers[index]; }
  void WriteRegister(uint32_t index, uint32_t value) override { registers[index] = value; }
  uint32_t ReadPhysical(uint32_t address) override { return memory[address]; }
  void WritePhysical(uint32_t address, uint32_t value) override { memory[address] = value; }
  void MakeCoherent() override { registers[0xA31] = 0; ++coherent; }
  bool PollDelay(uint32_t interval) override {
    assert(interval == 0x100);
    ++polls;
    memory[0x100] = 0x78563412;
    return !cancel;
  }
  bool SubmitDraw(const gh::graphics::DrawCommand& command) override {
    draws.push_back(command);
    return draw_success;
  }
  void LoadShader(uint32_t type, uint32_t address, const void* code, uint32_t words) override {
    shader_type = type;
    shader_address = address;
    shader_words = words;
    inline_shader = code != nullptr;
  }
  std::unordered_map<uint32_t, uint32_t> registers, memory;
  std::vector<gh::graphics::DrawCommand> draws;
  uint32_t shader_type = 0, shader_address = 0, shader_words = 0;
  bool inline_shader = false, draw_success = true;
  uint32_t polls = 0, coherent = 0;
  bool cancel = false;
};

constexpr uint32_t Packet(uint32_t opcode, uint32_t count, bool predicate = false) {
  return 0xC0000000u | ((count - 1) << 16) | (opcode << 8) | uint32_t(predicate);
}
}  // namespace

int main() {
  using namespace gh::graphics;
  GuestCommandProcessor processor;
  Target target;
  auto execute = [&](uint32_t header, std::initializer_list<uint32_t> words) {
    Reader reader(words);
    const auto result = processor.Execute(header, reader, target);
    if (result == PacketResult::kExecuted) assert(reader.RemainingWords() == 0);
    return result;
  };
  assert(execute((2u << 16) | 0xA00, {1, 2, 3}) == PacketResult::kExecuted);
  assert(target.registers[0xA02] == 3);
  assert(execute((2u << 16) | 0x8000 | 0xA00, {4, 5, 6}) == PacketResult::kExecuted);
  assert(target.registers[0xA00] == 6 && target.registers[0xA01] == 2);
  assert(execute(0x40000000u | (0x37 << 11) | 0x25, {10, 20}) == PacketResult::kExecuted);
  assert(target.registers[0x25] == 10 && target.registers[0x37] == 20);
  assert(execute(0x40000000u, {1}) == PacketResult::kInvalid);

  assert(execute(Packet(0x50, 2), {0, 0}) == PacketResult::kExecuted);
  assert(execute(Packet(0x3D, 2, true), {0x100, 17}) == PacketResult::kExecuted);
  assert(target.memory.empty());
  assert(execute(Packet(0x60, 1), {0x80}) == PacketResult::kExecuted);
  assert(execute(Packet(0x62, 1), {0x80}) == PacketResult::kExecuted);
  assert(execute(Packet(0x61, 1), {0x1234}) == PacketResult::kExecuted);
  assert(processor.bin_mask() == 0x123400000080ull);
  assert(execute(Packet(0x64, 4, true), {0, 0, 0, 0}) == PacketResult::kExecuted);
  const std::array<uint32_t, 1> device_words{7};
  Reader device(device_words);
  assert(processor.Execute(Packet(0x40, 1), device, target) == PacketResult::kDeviceCommand);
  assert(device.RemainingWords() == 1);

  const std::array<uint32_t, 4> swapped{0x12345678, 0x34127856, 0x78563412, 0x56781234};
  for (uint32_t endian = 0; endian < 4; ++endian) {
    assert(execute(Packet(0x3D, 2), {0x100 | endian, 0x12345678}) == PacketResult::kExecuted);
    assert(target.memory[0x100] == swapped[endian]);
  }
  target.registers[1] = 0xFF;
  target.registers[2] = 0x30;
  target.registers[3] = 0x100;
  assert(execute(Packet(0x21, 3), {0xC0000001, 2, 3}) == PacketResult::kExecuted);
  assert(target.registers[1] == 0x130);
  assert(execute(Packet(0x3E, 2), {1, 0x102}) == PacketResult::kExecuted);
  assert(target.memory[0x100] == 0x30010000);
  assert(execute(Packet(0x45, 6), {0x113, 0x102, 0x130, 0xFFFFFFFF, 0x201, 0xAABBCCDD}) == PacketResult::kExecuted);
  assert(target.memory[0x200] == 0xBBAADDCC);

  target.memory[0x100] = 0;
  assert(execute(Packet(0x3C, 5), {0x13, 0x102, 0x12345678, 0xFFFFFFFF, 0x100}) == PacketResult::kExecuted);
  assert(target.polls == 1);
  target.registers[0xA31] = 0x80000000;
  assert(execute(Packet(0x3C, 5), {3, 0xA31, 0, 0xFFFFFFFF, 0x100}) == PacketResult::kExecuted);
  assert(target.coherent == 1 && target.polls == 1);
  target.cancel = true;
  target.memory[0x100] = 0;
  assert(execute(Packet(0x3C, 5), {0x13, 0x100, 1, 0xFFFFFFFF, 0x100}) == PacketResult::kInvalid);

  assert(execute(Packet(0x2D, 3), {(1 << 16) | 6, 99, 101}) == PacketResult::kExecuted);
  assert(target.registers[0x4806] == 99 && target.registers[0x4807] == 101);
  assert(execute(Packet(0x55, 2), {0x4000, 42}) == PacketResult::kExecuted);
  assert(target.registers[0x4000] == 42);
  target.memory[0x300] = 0x78563412;
  assert(execute(Packet(0x2F, 3), {0x300, 9, 1}) == PacketResult::kExecuted);
  assert(target.registers[0x4009] == 0x12345678);

  // Index DMA state must reach the backend with the original alignment,
  // width, endianness and explicit-major-mode rules.
  const uint32_t indexed = (7 << 16) | (1 << 11) | 0x11;
  assert(execute(Packet(0x22, 4), {0x100, indexed, 0x307, 0x80000009}) == PacketResult::kExecuted);
  const auto& draw = target.draws.back();
  assert(draw.indexed && draw.index_format == 1 && draw.index_endian == 2);
  assert(draw.index_address == 0x304 && draw.index_length == 36 && draw.index_count == 7);
  assert(draw.primitive == 0x11 && draw.explicit_major_mode);
  assert(target.registers[0x21FC] == indexed && target.registers[0x21FA] == 0x307);
  assert(target.registers[0x21FB] == 0x80000009);
  const uint32_t automatic = (3 << 16) | (2 << 6) | 4;
  assert(execute(Packet(0x36, 2), {automatic, 0x1234}) == PacketResult::kExecuted);
  assert(!target.draws.back().indexed && !target.draws.back().explicit_major_mode);
  const auto draw_count = target.draws.size();
  target.registers[0x2293] = 0x81;
  assert(execute(Packet(0x36, 1), {automatic}) == PacketResult::kExecuted);
  assert(target.draws.size() == draw_count);
  target.registers[0x2293] = 0;
  target.draw_success = false;
  assert(execute(Packet(0x36, 1), {automatic}) == PacketResult::kExecuted);
  assert(execute(Packet(0x22, 1), {0}) == PacketResult::kInvalid);
  assert(execute(Packet(0x36, 2), {indexed, 0x400}) == PacketResult::kInvalid);
  assert(target.registers[0x21FA] == 0x400);

  assert(execute(Packet(0x27, 2), {0x401, 3}) == PacketResult::kExecuted);
  assert(target.shader_type == 1 && target.shader_address == 0x400);
  assert(target.shader_words == 3 && !target.inline_shader);
  assert(execute(Packet(0x2B, 5), {0, 3, 0x100, 0x200, 0x300}) == PacketResult::kExecuted);
  assert(target.shader_type == 0 && target.shader_words == 3 && target.inline_shader);
  assert(execute(Packet(0x2B, 3), {0, 2, 0x100}) == PacketResult::kInvalid);
  assert(execute(Packet(0x27, 2), {0x402, 3}) == PacketResult::kInvalid);
}
