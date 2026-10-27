#pragma once

#include <cstdint>

namespace gh::graphics::xenos {

struct Float4 {
  float r;
  float g;
  float b;
  float a;
};

// Decodes the Xenos 7e3 RGB channel representation used by 10:10:10:2 clears.
float Float7e3To32(uint32_t value) noexcept;
float PWLGammaToLinear(float gamma) noexcept;
float LinearToPWLGamma(float linear) noexcept;
uint32_t Float32To20e4(float value, bool round_to_nearest_even) noexcept;
float Float20e4To32(uint32_t value) noexcept;

// Decodes the floating-point 10:10:10:2 clear-value representation used by
// Xenos render targets. RGB channels are unsigned 7e3; alpha is UNorm2.
Float4 Float1010102To32(uint32_t value) noexcept;

}  // namespace gh::graphics::xenos
