#pragma once

#include <cstdint>

namespace gh::filesystem {

uint32_t FlushGuestFile(uint8_t* base, uint32_t io_status_block) noexcept;

}  // namespace gh::filesystem
