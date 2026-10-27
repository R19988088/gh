#pragma once

#include "gh_image.h"
#include "gh_image_metadata.h"

#include <functional>

namespace gh::image {
struct ResolvedImport {
  bool variable = false;
  bool implemented = false;
  uint32_t ordinal = 0;
  uint32_t address = 0;
  std::string_view name;
};
struct ImportBindingTarget {
  std::function<ResolvedImport(std::string_view, uint16_t)> resolve;
  std::function<void(uint32_t, uint32_t)> write;
  std::function<void(const ImportLibrary&, const ImportFunction&, const ResolvedImport&, bool)> bound;
};
void BindImports(const ImportLibrary& library, const ImportBindingTarget& target);

// Records address the same mutable image: copy records may overlap, and LZX
// records use the original range as the dictionary before replacing the target.
int ApplyDeltaRecords(std::span<const uint8_t> records, uint32_t window_size,
                       std::span<uint8_t> image);

struct DeltaPatchMemory {
  std::function<bool(uint32_t, uint32_t)> grow;
  std::function<void(uint32_t, uint32_t)> shrink;
  std::function<std::span<uint8_t>(uint32_t)> image;
  std::function<void(bool, std::string_view)> log;  // true for a warning
};
struct PatchVersions { uint32_t source = 0, target = 0; };
int ApplyDeltaPatch(const XexMetadata& patch, std::span<const uint8_t> payload,
                    XexMetadata& base, bool devkit, uint32_t page_size,
                    Key& base_session_key, Key& patch_session_key,
                    const DeltaPatchMemory& memory, PatchVersions& versions);
}  // namespace gh::image
