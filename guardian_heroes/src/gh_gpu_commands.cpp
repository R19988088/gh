// PM4 execution derived from Xenia, Copyright 2022 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#include "gh_gpu_commands.h"

#include <bit>

namespace gh::graphics {
namespace {

uint32_t Swap(uint32_t value, uint32_t endian) {
  switch (endian & 3) {
    case 1: return ((value << 8) & 0xFF00FF00u) | ((value >> 8) & 0x00FF00FFu);
    case 2: return std::byteswap(value);
    case 3: return (value >> 16) | (value << 16);
    default: return value;
  }
}

bool Compare(uint32_t function, uint32_t value, uint32_t reference, uint32_t mask) {
  value &= mask;
  switch (function & 7) {
    case 0: return false;
    case 1: return value < reference;
    case 2: return value <= reference;
    case 3: return value == reference;
    case 4: return value != reference;
    case 5: return value >= reference;
    case 6: return value > reference;
    default: return true;
  }
}

uint32_t ConstantBase(uint32_t type) {
  constexpr uint32_t bases[] = {0x4000, 0x4800, 0x4900, 0x4908, 0x2000};
  return type < 5 ? bases[type] : 0;
}

}  // namespace

void CommandTarget::WriteRegisterRange(PacketReader& reader, uint32_t index, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) WriteRegister(index + i, reader.ReadWord());
}

void CommandTarget::LoadRegisterRange(uint32_t address, uint32_t index, uint32_t count) {
  for (uint32_t i = 0; i < count; ++i) {
    WriteRegister(index + i, std::byteswap(ReadPhysical(address + i * 4)));
  }
}

PacketResult GuestCommandProcessor::Execute(uint32_t header, PacketReader& reader,
                                             CommandTarget& target) {
  if (!header || (header >> 30) == 2) return PacketResult::kExecuted;
  const uint32_t type = header >> 30;
  const uint32_t count = type == 1 ? 2 : ((header >> 16) & 0x3FFF) + 1;
  if (reader.RemainingWords() < count) return PacketResult::kInvalid;
  if (type == 0) {
    const uint32_t base = header & 0x7FFF;
    for (uint32_t i = 0; i < count; ++i) {
      target.WriteRegister(base + ((header & 0x8000) ? 0 : i), reader.ReadWord());
    }
    return PacketResult::kExecuted;
  }
  if (type == 1) {
    const uint32_t first = reader.ReadWord();
    const uint32_t second = reader.ReadWord();
    target.WriteRegister(header & 0x7FF, first);
    target.WriteRegister((header >> 11) & 0x7FF, second);
    return PacketResult::kExecuted;
  }

  const uint32_t opcode = (header >> 8) & 0x7F;
  if ((header & 1) && (!(bin_select_ & bin_mask_) || opcode == 0x64)) {
    reader.SkipWords(count);
    return PacketResult::kExecuted;
  }
  switch (opcode) {
    case 0x10:  // NOP
      reader.SkipWords(count);
      break;
    case 0x48:  // ME_INIT
      microcode_.clear();
      microcode_.reserve(count);
      for (uint32_t i = 0; i < count; ++i) microcode_.push_back(reader.ReadWord());
      break;
    case 0x22:  // DRAW_INDX
    case 0x36:  // DRAW_INDX_2
      return Draw(opcode, count, reader, target);
    case 0x27:  // IM_LOAD
    case 0x2B: {  // IM_LOAD_IMMEDIATE
      if (count < 2) return PacketResult::kInvalid;
      const uint32_t source = reader.ReadWord();
      const uint32_t start_size = reader.ReadWord();
      const bool immediate = opcode == 0x2B;
      const uint32_t type = immediate ? source : source & 3;
      const uint32_t words = start_size & 0xFFFF;
      if (type > 1 || (start_size >> 16) ||
          (immediate && count - 2 < words)) return PacketResult::kInvalid;
      const auto* code = immediate ? reader.CurrentBytes() : nullptr;
      const uint32_t address = immediate
          ? static_cast<uint32_t>(reinterpret_cast<uintptr_t>(code)) : source & ~3u;
      target.LoadShader(type, address, code, words);
      if (immediate) reader.SkipWords(words);
      break;
    }
    case 0x50:  // SET_BIN_MASK
    case 0x51: {  // SET_BIN_SELECT
      if (count != 2) return PacketResult::kInvalid;
      const uint64_t high = reader.ReadWord();
      const uint64_t value = (high << 32) | reader.ReadWord();
      (opcode == 0x50 ? bin_mask_ : bin_select_) = value;
      break;
    }
    case 0x60: case 0x61: case 0x62: case 0x63: {
      if (count != 1) return PacketResult::kInvalid;
      uint64_t& value = opcode < 0x62 ? bin_mask_ : bin_select_;
      const uint64_t word = reader.ReadWord();
      value = (opcode & 1) ? (value & 0xFFFFFFFFull) | (word << 32)
                           : (value & 0xFFFFFFFF00000000ull) | word;
      break;
    }
    case 0x21: {  // REG_RMW
      if (count != 3) return PacketResult::kInvalid;
      const uint32_t info = reader.ReadWord();
      const uint32_t and_mask = reader.ReadWord();
      const uint32_t or_mask = reader.ReadWord();
      uint32_t value = target.ReadRegister(info & 0x1FFF);
      value &= (info & 0x80000000u) ? target.ReadRegister(and_mask & 0x1FFF) : and_mask;
      value |= (info & 0x40000000u) ? target.ReadRegister(or_mask & 0x1FFF) : or_mask;
      target.WriteRegister(info & 0x1FFF, value);
      break;
    }
    case 0x3E: {  // REG_TO_MEM
      if (count != 2) return PacketResult::kInvalid;
      const uint32_t index = reader.ReadWord();
      const uint32_t address = reader.ReadWord();
      target.WritePhysical(address & ~3u, Swap(target.ReadRegister(index), address));
      break;
    }
    case 0x3D: {  // MEM_WRITE
      uint32_t address = reader.ReadWord();
      for (uint32_t i = 1; i < count; ++i, address += 4) {
        target.WritePhysical(address & ~3u, Swap(reader.ReadWord(), address));
      }
      break;
    }
    case 0x45: {  // COND_WRITE
      if (count != 6) return PacketResult::kInvalid;
      const uint32_t info = reader.ReadWord();
      const uint32_t poll = reader.ReadWord();
      const uint32_t reference = reader.ReadWord();
      const uint32_t mask = reader.ReadWord();
      const uint32_t address = reader.ReadWord();
      const uint32_t data = reader.ReadWord();
      const uint32_t value = (info & 0x10)
          ? Swap(target.ReadPhysical(poll & ~3u), poll) : target.ReadRegister(poll);
      if (Compare(info, value, reference, mask)) {
        if (info & 0x100) target.WritePhysical(address & ~3u, Swap(data, address));
        else target.WriteRegister(address, data);
      }
      break;
    }
    case 0x3C: {  // WAIT_REG_MEM
      if (count != 5) return PacketResult::kInvalid;
      const uint32_t info = reader.ReadWord();
      const uint32_t poll = reader.ReadWord();
      const uint32_t reference = reader.ReadWord();
      const uint32_t mask = reader.ReadWord();
      const uint32_t interval = reader.ReadWord();
      for (;;) {
        uint32_t value;
        if (info & 0x10) value = Swap(target.ReadPhysical(poll & ~3u), poll);
        else {
          value = target.ReadRegister(poll);
          if (poll == 0x0A31) {
            target.MakeCoherent();
            value = target.ReadRegister(poll);
          }
        }
        if (Compare(info, value, reference, mask)) break;
        if (!target.PollDelay(interval)) return PacketResult::kInvalid;
      }
      break;
    }
    case 0x2D:  // SET_CONSTANT
    case 0x55:  // SET_CONSTANT2
    case 0x56: {  // SET_SHADER_CONSTANTS
      const uint32_t offset = reader.ReadWord();
      uint32_t index = offset & 0xFFFF;
      if (opcode == 0x2D) {
        const uint32_t base = ConstantBase((offset >> 16) & 0xFF);
        if (!base) {
          reader.SkipWords(count - 1);
          break;
        }
        index = base + (offset & 0x7FF);
      }
      target.WriteRegisterRange(reader, index, count - 1);
      break;
    }
    case 0x2F: {  // LOAD_ALU_CONSTANT
      if (count != 3) return PacketResult::kInvalid;
      uint32_t address = reader.ReadWord() & 0x3FFFFFFF;
      const uint32_t offset = reader.ReadWord();
      const uint32_t words = reader.ReadWord() & 0xFFF;
      const uint32_t base = ConstantBase((offset >> 16) & 0xFF);
      if (!base) break;
      target.LoadRegisterRange(address, base + (offset & 0x7FF), words);
      break;
    }
    default: return PacketResult::kDeviceCommand;
  }
  return PacketResult::kExecuted;
}

PacketResult GuestCommandProcessor::Draw(uint32_t opcode, uint32_t count,
                                         PacketReader& reader, CommandTarget& target) {
  if (opcode == 0x22) {
    // The existing renderer doesn't use the per-draw viz query token.
    reader.ReadWord();
    --count;
  }
  if (!count) return PacketResult::kInvalid;
  DrawCommand command;
  command.initiator = reader.ReadWord();
  --count;
  target.WriteRegister(0x21FC, command.initiator);
  command.primitive = command.initiator & 0x3F;
  command.index_count = command.initiator >> 16;
  command.index_format = (command.initiator >> 11) & 1;
  command.explicit_major_mode = ((command.initiator >> 8) & 3) != 0 ||
                                command.primitive >= 0x10;
  const uint32_t source = (command.initiator >> 6) & 3;
  if (source == 0) {
    command.indexed = true;
    if (!count) return PacketResult::kInvalid;
    const uint32_t base = reader.ReadWord();
    --count;
    target.WriteRegister(0x21FA, base);
    if (!count) return PacketResult::kInvalid;
    const uint32_t size = reader.ReadWord();
    --count;
    target.WriteRegister(0x21FB, size);
    const uint32_t bytes = command.index_format ? 4 : 2;
    command.index_address = base & ~(bytes - 1);
    command.index_endian = size >> 30;
    command.index_length = (size & 0xFFFFFF) * bytes;
  }
  reader.SkipWords(count);
  // Immediate indices and reserved source selection were already unsupported.
  // A dropped draw must not stop processing the rest of the command buffer.
  if ((source != 0 && source != 2) || (target.ReadRegister(0x2293) & 0x81) != 0x81) {
    target.SubmitDraw(command);
  }
  return PacketResult::kExecuted;
}

}  // namespace gh::graphics
