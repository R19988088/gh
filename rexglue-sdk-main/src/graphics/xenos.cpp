/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cmath>

#include <rex/graphics/xenos.h>
#include <rex/math.h>
#include <rex/memory.h>

#include "gh_xenos_math.h"

namespace rex::graphics {
namespace xenos {

// Based on X360GammaToLinear and X360LinearToGamma from the Source Engine, with
// additional logic from Direct3D 9 code in game executable disassembly, located
// via the floating-point constants involved.
// https://github.com/ValveSoftware/source-sdk-2013/blob/master/mp/src/mathlib/color_conversion.cpp#L329
// These are provided here in part as a reference for shader translators.

float PWLGammaToLinear(float gamma) {
  return gh::graphics::xenos::PWLGammaToLinear(gamma);
}

float LinearToPWLGamma(float linear) {
  return gh::graphics::xenos::LinearToPWLGamma(linear);
}

// https://github.com/Microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexConvert.cpp

float Float7e3To32(uint32_t f10) {
  return gh::graphics::xenos::Float7e3To32(f10);
}

// Based on CFloat24 from d3dref9.dll and the 6e4 code from:
// https://github.com/Microsoft/DirectXTex/blob/master/DirectXTex/DirectXTexConvert.cpp
// 6e4 has a different exponent bias allowing [0,512) values, 20e4 allows [0,2).

uint32_t Float32To20e4(float f32, bool round_to_nearest_even) {
  return gh::graphics::xenos::Float32To20e4(f32, round_to_nearest_even);
}

float Float20e4To32(uint32_t f24) {
  return gh::graphics::xenos::Float20e4To32(f24);
}

const char* GetColorRenderTargetFormatName(ColorRenderTargetFormat format) {
  switch (format) {
    case ColorRenderTargetFormat::k_8_8_8_8:
      return "k_8_8_8_8";
    case ColorRenderTargetFormat::k_8_8_8_8_GAMMA:
      return "k_8_8_8_8_GAMMA";
    case ColorRenderTargetFormat::k_2_10_10_10:
      return "k_2_10_10_10";
    case ColorRenderTargetFormat::k_2_10_10_10_FLOAT:
      return "k_2_10_10_10_FLOAT";
    case ColorRenderTargetFormat::k_16_16:
      return "k_16_16";
    case ColorRenderTargetFormat::k_16_16_16_16:
      return "k_16_16_16_16";
    case ColorRenderTargetFormat::k_16_16_FLOAT:
      return "k_16_16_FLOAT";
    case ColorRenderTargetFormat::k_16_16_16_16_FLOAT:
      return "k_16_16_16_16_FLOAT";
    case ColorRenderTargetFormat::k_2_10_10_10_AS_10_10_10_10:
      return "k_2_10_10_10_AS_10_10_10_10";
    case ColorRenderTargetFormat::k_2_10_10_10_FLOAT_AS_16_16_16_16:
      return "k_2_10_10_10_FLOAT_AS_16_16_16_16";
    case ColorRenderTargetFormat::k_32_FLOAT:
      return "k_32_FLOAT";
    case ColorRenderTargetFormat::k_32_32_FLOAT:
      return "k_32_32_FLOAT";
    default:
      return "kUnknown";
  }
}

const char* GetDepthRenderTargetFormatName(DepthRenderTargetFormat format) {
  switch (format) {
    case DepthRenderTargetFormat::kD24S8:
      return "kD24S8";
    case DepthRenderTargetFormat::kD24FS8:
      return "kD24FS8";
    default:
      return "kUnknown";
  }
}

Float1010102 DecodeFloat1010102(uint32_t value) {
  const auto decoded = gh::graphics::xenos::Float1010102To32(value);
  return {decoded.r, decoded.g, decoded.b, decoded.a};
}

}  // namespace xenos
}  // namespace rex::graphics
