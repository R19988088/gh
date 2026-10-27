/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include "gh_system_lzx.h"

#include "gh_image.h"

int lzx_decompress(const void* lzx_data, size_t lzx_len, void* dest, size_t dest_len,
                   uint32_t window_size, void* window_data, size_t window_data_len) {
  return gh::image::DecompressLzx(
      {static_cast<const uint8_t*>(lzx_data), lzx_len},
      {static_cast<uint8_t*>(dest), dest_len}, window_size,
      {static_cast<const uint8_t*>(window_data), window_data_len});
}

