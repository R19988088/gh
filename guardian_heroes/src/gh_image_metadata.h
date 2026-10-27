#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gh::image {
struct OptionalHeader {
  uint32_t value = 0;
  uint32_t offset = 0;  // Position within the XEX header; inline fields point to their value.
  uint32_t size = 0;
};
bool FindOptionalHeader(std::span<const uint8_t> header, uint32_t key, OptionalHeader& result);

struct PageDescriptor { uint32_t type = 0, count = 0; };
struct SecurityMetadata {
  std::array<uint8_t, 256> signature{};
  std::array<uint8_t, 16> key{};
  uint32_t image_size = 0, flags = 0, export_table = 0, load_address = 0;
  uint32_t page_table_offset = 0;
  std::vector<PageDescriptor> pages;
};
struct ImportFunction { uint32_t ordinal = 0, value_address = 0, thunk_address = 0; };
struct ImportVersion { uint32_t value = 0; };
struct ImportLibrary {
  std::string name;
  uint32_t id = 0;
  ImportVersion version, min_version;
  std::vector<ImportFunction> imports;
  // Preserve every type-zero slot in source order, including repeated ordinals.
  std::vector<ImportFunction> variable_slots;
};

// Owns the original ABI header bytes and parsed security metadata. Delta patch
// writers mutate HeaderBytes(), then Refresh() before using parsed values.
class XexMetadata {
 public:
  bool Load(std::span<const uint8_t> file);
  bool Refresh();
  std::vector<uint8_t>& HeaderBytes() { return header_; }
  const std::vector<uint8_t>& HeaderBytes() const { return header_; }
  const SecurityMetadata& Security() const { return security_; }
  bool IsXex1() const { return xex1_; }
  uint32_t ImageSize(uint32_t page_size) const;
  bool AlternateTitleIds(std::vector<uint32_t>& ids) const;
  bool ParseImports(std::span<const uint8_t> image, uint32_t base,
                    std::vector<ImportLibrary>& libraries) const;
 private:
  std::vector<uint8_t> header_;
  SecurityMetadata security_;
  bool xex1_ = false;
};

struct PeSection {
  char name[9]{};
  uint32_t raw_address = 0, raw_size = 0, address = 0, size = 0, flags = 0;
};
struct PeMetadata {
  std::vector<PeSection> sections;
  uint32_t timestamp = 0, exception_rva = 0, exception_size = 0;
};
bool ParsePe(std::span<const uint8_t> image, uint32_t base, PeMetadata& metadata);
uint32_t FindXexExport(std::span<const uint8_t> table, uint16_t ordinal);
uint32_t FindPeExport(std::span<const uint8_t> image, uint32_t directory_rva,
                      uint32_t base, uint16_t ordinal);
uint32_t FindPeExport(std::span<const uint8_t> image, uint32_t directory_rva,
                      uint32_t base, std::string_view name);
}  // namespace gh::image
