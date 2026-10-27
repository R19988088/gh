#include "gh_xenos_math.h"

#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <array>
#include <algorithm>

namespace {

// Kept as the pre-extraction oracle from rex::graphics::xenos::Float7e3To32.
float ReXFloat7e3To32(uint32_t f10) {
  f10 &= 0x3FF;
  if (!f10) {
    return 0.0f;
  }
  uint32_t mantissa = f10 & 0x7F;
  uint32_t exponent = f10 >> 7;
  if (!exponent) {
    // Match the old rex::lzcnt(uint32_t) primitive independently of the
    // project's new std::countl_zero implementation.
    const uint32_t mantissa_leading_zero_count =
        static_cast<uint32_t>(__builtin_clz(mantissa)) - (32u - 8u);
    exponent = static_cast<uint32_t>(1 - static_cast<int32_t>(mantissa_leading_zero_count));
    mantissa = (mantissa << mantissa_leading_zero_count) & 0x7F;
  }
  return std::bit_cast<float>(((exponent + 124) << 23) | (mantissa << 16));
}

float ReXPWLGammaToLinear(float gamma) {
  gamma = std::isgreater(gamma, 0.0f) ? gamma : 0.0f;
  gamma = std::isless(gamma, 1.0f) ? gamma : 1.0f;
  float scale, offset;
  if (gamma >= 96.0f / 255.0f) {
    if (gamma >= 192.0f / 255.0f) { scale = 8.0f / 1024.0f; offset = -1024.0f; }
    else { scale = 4.0f / 1024.0f; offset = -256.0f; }
  } else if (gamma >= 64.0f / 255.0f) { scale = 2.0f / 1024.0f; offset = -64.0f; }
  else { scale = 1.0f / 1024.0f; offset = 0.0f; }
  float linear = gamma * ((255.0f * 1024.0f) * scale) + offset;
  linear += std::trunc(linear * scale);
  return linear * (1.0f / 1023.0f);
}

float ReXLinearToPWLGamma(float linear) {
  linear = std::isgreater(linear, 0.0f) ? linear : 0.0f;
  linear = std::isless(linear, 1.0f) ? linear : 1.0f;
  float scale, offset;
  if (linear >= 128.0f / 1023.0f) {
    if (linear >= 512.0f / 1023.0f) { scale = 1023.0f / 8.0f; offset = 128.0f / 255.0f; }
    else { scale = 1023.0f / 4.0f; offset = 64.0f / 255.0f; }
  } else if (linear >= 64.0f / 1023.0f) { scale = 1023.0f / 2.0f; offset = 32.0f / 255.0f; }
  else { scale = 1023.0f; offset = 0.0f; }
  return std::trunc(linear * scale) * (1.0f / 255.0f) + offset;
}

uint32_t ReXFloat32To20e4(float value, bool round_to_nearest_even) {
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

float ReXFloat20e4To32(uint32_t f24) {
  f24 &= 0xFFFFFF;
  if (!f24) return 0.0f;
  uint32_t mantissa = f24 & 0xFFFFF;
  uint32_t exponent = f24 >> 20;
  if (!exponent) {
    const uint32_t leading = static_cast<uint32_t>(__builtin_clz(mantissa)) - (32u - 21u);
    exponent = static_cast<uint32_t>(1 - static_cast<int32_t>(leading));
    mantissa = (mantissa << leading) & 0xFFFFF;
  }
  return std::bit_cast<float>(((exponent + 112) << 23) | (mantissa << 3));
}

}  // namespace

int main() {
  for (uint32_t input = 0; input < 0x400; ++input) {
    const uint32_t expected = std::bit_cast<uint32_t>(ReXFloat7e3To32(input));
    const uint32_t actual =
        std::bit_cast<uint32_t>(gh::graphics::xenos::Float7e3To32(input));
    assert(actual == expected);
  }

  // The packed floating clear-value decoder must retain the per-channel
  // conversions used by the existing Xenos D3D12 and Vulkan paths.
  for (uint32_t input = 0; input < 0x400; ++input) {
    const uint32_t packed = input | (input << 10) | (input << 20);
    const auto actual = gh::graphics::xenos::Float1010102To32(packed);
    const float expected = ReXFloat7e3To32(input);
    assert(std::bit_cast<uint32_t>(actual.r) == std::bit_cast<uint32_t>(expected));
    assert(std::bit_cast<uint32_t>(actual.g) == std::bit_cast<uint32_t>(expected));
    assert(std::bit_cast<uint32_t>(actual.b) == std::bit_cast<uint32_t>(expected));
  }
  for (uint32_t alpha = 0; alpha < 4; ++alpha) {
    const auto actual = gh::graphics::xenos::Float1010102To32(alpha << 30);
    const float expected = alpha * (1.0f / 3.0f);
    assert(std::bit_cast<uint32_t>(actual.a) == std::bit_cast<uint32_t>(expected));
  }

  // The hardware field is ten bits; verify the wrapper masks higher bits too.
  for (uint32_t input = 0; input < 0x400; ++input) {
    const uint32_t decorated = input | 0xA5C00000;
    assert(std::bit_cast<uint32_t>(gh::graphics::xenos::Float7e3To32(decorated)) ==
           std::bit_cast<uint32_t>(ReXFloat7e3To32(input)));
  }

  const std::array<float, 20> gamma_values = {
      -1.0f, -0.0f, 0.0f, 1.0f / 255.0f, 64.0f / 255.0f,
      96.0f / 255.0f, 192.0f / 255.0f, 1.0f, 1.1f, 0.33333334f,
      0.5f, 0.75f, 0.99999994f, INFINITY, -INFINITY, NAN,
      std::nextafter(64.0f / 255.0f, 0.0f), std::nextafter(96.0f / 255.0f, 1.0f),
      std::nextafter(192.0f / 255.0f, 0.0f), std::nextafter(1.0f, 0.0f)};
  for (float value : gamma_values) {
    assert(std::bit_cast<uint32_t>(gh::graphics::xenos::PWLGammaToLinear(value)) ==
           std::bit_cast<uint32_t>(ReXPWLGammaToLinear(value)));
    assert(std::bit_cast<uint32_t>(gh::graphics::xenos::LinearToPWLGamma(value)) ==
           std::bit_cast<uint32_t>(ReXLinearToPWLGamma(value)));
  }

  constexpr std::array<float, 12> float_values = {
      -1.0f, -0.0f, 0.0f, 0.0000001f, 0.0001f, 0.5f,
      1.0f, 1.5f, 1.9999999f, 2.0f, 255.0f, 512.0f};
  for (float value : float_values) {
    for (bool round : {false, true}) {
      assert(gh::graphics::xenos::Float32To20e4(value, round) ==
             ReXFloat32To20e4(value, round));
    }
  }
  constexpr std::array<uint32_t, 8> mantissas = {
      0u, 1u, 2u, 0x3FFFFFu, 0x400000u, 0x7FFFFDu, 0x7FFFFEu, 0x7FFFFFu};
  for (uint32_t exponent = 0; exponent < 256; ++exponent) {
    for (uint32_t mantissa : mantissas) {
      for (uint32_t sign : {0u, 0x80000000u}) {
        const float value = std::bit_cast<float>(sign | (exponent << 23) | mantissa);
        for (bool round : {false, true}) {
          assert(gh::graphics::xenos::Float32To20e4(value, round) ==
                 ReXFloat32To20e4(value, round));
        }
      }
    }
  }
  for (uint32_t input = 0; input <= 0xFFFFFFu; ++input) {
    assert(std::bit_cast<uint32_t>(gh::graphics::xenos::Float20e4To32(input)) ==
           std::bit_cast<uint32_t>(ReXFloat20e4To32(input)));
  }
  for (uint32_t input : {0u, 1u, 2u, 0x7FFFFFu, 0x800000u, 0xFFFFFFu,
                         0x1000000u, 0xABCDEFu}) {
    assert(std::bit_cast<uint32_t>(gh::graphics::xenos::Float20e4To32(input)) ==
           std::bit_cast<uint32_t>(ReXFloat20e4To32(input)));
  }
}
