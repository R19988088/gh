#include "gh_xenos_math.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace gh::graphics::xenos {

float Float7e3To32(uint32_t value) noexcept {
  value &= 0x3FF;
  if (!value) {
    return 0.0f;
  }
  uint32_t mantissa = value & 0x7F;
  uint32_t exponent = value >> 7;
  if (!exponent) {
    const uint32_t leading_zero_count =
        static_cast<uint32_t>(std::countl_zero(mantissa));
    const uint32_t mantissa_leading_zero_count = leading_zero_count - (32u - 8u);
    exponent = static_cast<uint32_t>(1 - static_cast<int32_t>(mantissa_leading_zero_count));
    mantissa = (mantissa << mantissa_leading_zero_count) & 0x7F;
  }
  return std::bit_cast<float>(((exponent + 124) << 23) | (mantissa << 16));
}

float PWLGammaToLinear(float gamma) noexcept {
  gamma = std::isgreater(gamma, 0.0f) ? gamma : 0.0f;
  gamma = std::isless(gamma, 1.0f) ? gamma : 1.0f;
  float scale, offset;
  if (gamma >= 96.0f / 255.0f) {
    if (gamma >= 192.0f / 255.0f) {
      scale = 8.0f / 1024.0f;
      offset = -1024.0f;
    } else {
      scale = 4.0f / 1024.0f;
      offset = -256.0f;
    }
  } else if (gamma >= 64.0f / 255.0f) {
    scale = 2.0f / 1024.0f;
    offset = -64.0f;
  } else {
    scale = 1.0f / 1024.0f;
    offset = 0.0f;
  }
  float linear = gamma * ((255.0f * 1024.0f) * scale) + offset;
  linear += std::trunc(linear * scale);
  return linear * (1.0f / 1023.0f);
}

float LinearToPWLGamma(float linear) noexcept {
  linear = std::isgreater(linear, 0.0f) ? linear : 0.0f;
  linear = std::isless(linear, 1.0f) ? linear : 1.0f;
  float scale, offset;
  if (linear >= 128.0f / 1023.0f) {
    if (linear >= 512.0f / 1023.0f) {
      scale = 1023.0f / 8.0f;
      offset = 128.0f / 255.0f;
    } else {
      scale = 1023.0f / 4.0f;
      offset = 64.0f / 255.0f;
    }
  } else if (linear >= 64.0f / 1023.0f) {
    scale = 1023.0f / 2.0f;
    offset = 32.0f / 255.0f;
  } else {
    scale = 1023.0f;
    offset = 0.0f;
  }
  return std::trunc(linear * scale) * (1.0f / 255.0f) + offset;
}

uint32_t Float32To20e4(float value, bool round_to_nearest_even) noexcept {
  if (!(value > 0.0f)) return 0;
  uint32_t bits = std::bit_cast<uint32_t>(value);
  if (bits >= 0x3FFFFFF8) return 0xFFFFFF;
  if (bits < 0x38800000) {
    const uint32_t shift = std::min(uint32_t(113 - (bits >> 23)), uint32_t(24));
    bits = (0x800000 | (bits & 0x7FFFFF)) >> shift;
  } else {
    bits += 0xC8000000u;
  }
  if (round_to_nearest_even) bits += 3 + ((bits >> 3) & 1);
  return (bits >> 3) & 0xFFFFFF;
}

float Float20e4To32(uint32_t value) noexcept {
  value &= 0xFFFFFF;
  if (!value) return 0.0f;
  uint32_t mantissa = value & 0xFFFFF;
  uint32_t exponent = value >> 20;
  if (!exponent) {
    const uint32_t leading = static_cast<uint32_t>(std::countl_zero(mantissa)) - (32u - 21u);
    exponent = static_cast<uint32_t>(1 - static_cast<int32_t>(leading));
    mantissa = (mantissa << leading) & 0xFFFFF;
  }
  return std::bit_cast<float>(((exponent + 112) << 23) | (mantissa << 3));
}

Float4 Float1010102To32(uint32_t value) noexcept {
  constexpr float kInvAlphaMax = 1.0f / 3.0f;
  return {
      Float7e3To32(value),
      Float7e3To32(value >> 10),
      Float7e3To32(value >> 20),
      (value >> 30) * kInvAlphaMax,
  };
}

}  // namespace gh::graphics::xenos
