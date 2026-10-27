// XEX/PE layout and import semantics derived from Xenia/ReXGlue (BSD license).
#include "gh_image_metadata.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>

namespace gh::image {
namespace {
bool Contains(std::span<const uint8_t> data, size_t offset, size_t size) {
  return offset <= data.size() && size <= data.size() - offset;
}
uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
uint16_t Be16(const uint8_t* p) { return uint16_t(p[0]) << 8 | p[1]; }
uint32_t Le32(const uint8_t* p) {
  return uint32_t(p[3]) << 24 | uint32_t(p[2]) << 16 | uint32_t(p[1]) << 8 | p[0];
}
uint16_t Le16(const uint8_t* p) { return uint16_t(p[1]) << 8 | p[0]; }
bool String(std::span<const uint8_t> data, size_t offset, std::string& value) {
  if (offset >= data.size()) return false;
  const auto* first = data.data() + offset;
  const auto* end = static_cast<const uint8_t*>(std::memchr(first, 0, data.size() - offset));
  if (!end) return false;
  value.assign(reinterpret_cast<const char*>(first), size_t(end - first));
  return true;
}
}  // namespace

bool FindOptionalHeader(std::span<const uint8_t> header, uint32_t key, OptionalHeader& result) {
  if (header.size() < 24) return false;
  const uint32_t count = Be32(header.data() + 20);
  if (count > (header.size() - 24) / 8) return false;
  for (uint32_t i = 0; i < count; ++i) {
    const size_t entry = 24 + size_t(i) * 8;
    if (Be32(header.data() + entry) != key) continue;
    result.value = Be32(header.data() + entry + 4);
    const uint32_t words = key & 255;
    if (!words) { result.offset = uint32_t(entry + 4); result.size = 4; return true; }
    result.offset = words == 1 ? uint32_t(entry + 4) : result.value;
    if (!Contains(header, result.offset, 4)) return false;
    result.size = words == 255 ? Be32(header.data() + result.offset) : words * 4;
    return Contains(header, result.offset, result.size);
  }
  return false;
}

bool XexMetadata::Load(std::span<const uint8_t> file) {
  if (file.size() < 24) return false;
  const size_t size = Be32(file.data() + 8);
  if (size < 24 || size > file.size()) return false;
  header_.assign(file.begin(), file.begin() + size);
  return Refresh();
}

bool XexMetadata::Refresh() {
  const std::span<const uint8_t> header(header_);
  if (header.size() < 24) return false;
  const uint32_t magic = Be32(header.data());
  if (magic != 0x58455831 && magic != 0x58455832) return false;
  if (Be32(header.data() + 20) > (header.size() - 24) / 8) return false;
  xex1_ = magic == 0x58455831;
  const uint32_t sec = Be32(header.data() + 16);
  const size_t page_offset = xex1_ ? 0x168 : 0x184;
  if (!Contains(header, sec, page_offset)) return false;
  const auto* p = header.data() + sec;
  SecurityMetadata security;
  security.image_size = Be32(p + 4);
  std::copy_n(p + 8, 256, security.signature.begin());
  security.load_address = Be32(p + (xex1_ ? 0x130 : 0x110));
  std::copy_n(p + (xex1_ ? 0x134 : 0x150), 16, security.key.begin());
  security.flags = Be32(p + (xex1_ ? 0x158 : 0x10C));
  security.export_table = Be32(p + (xex1_ ? 0x15C : 0x160));
  const uint32_t page_count = Be32(p + page_offset - 4);
  if (page_count > (header.size() - sec - page_offset) / 24) return false;
  security.page_table_offset = sec + uint32_t(page_offset);
  for (uint32_t i = 0; i < page_count; ++i) {
    const uint32_t descriptor = Be32(p + page_offset + size_t(i) * 24);
    security.pages.push_back({descriptor & 15, descriptor >> 4});
  }
  security_ = std::move(security);
  return true;
}

uint32_t XexMetadata::ImageSize(uint32_t page_size) const {
  uint64_t size = 0;
  for (const auto page : security_.pages) size += uint64_t(page.count) * page_size;
  return size <= std::numeric_limits<uint32_t>::max() ? uint32_t(size) : 0;
}

bool XexMetadata::AlternateTitleIds(std::vector<uint32_t>& ids) const {
  ids.clear();
  OptionalHeader field;
  if (!FindOptionalHeader(header_, 0x407FF, field)) return true;
  if (field.size < 4 || field.size % 4) return false;
  for (size_t i = 4; i < field.size; i += 4)
    ids.push_back(Be32(header_.data() + field.offset + i));
  return true;
}

bool XexMetadata::ParseImports(std::span<const uint8_t> image, uint32_t base,
                               std::vector<ImportLibrary>& libraries) const {
  libraries.clear();
  OptionalHeader field;
  if (!FindOptionalHeader(header_, 0x103FF, field)) return true;
  const auto data = std::span(header_).subspan(field.offset, field.size);
  if (data.size() < 12) return false;
  const uint32_t strings_size = Be32(data.data() + 4);
  const uint32_t strings_count = Be32(data.data() + 8);
  if (!Contains(data, 12, strings_size)) return false;
  const auto strings_data = data.subspan(12, strings_size);
  std::vector<std::string> strings;
  size_t offset = 0;
  for (uint32_t i = 0; i < strings_count; ++i) {
    std::string name;
    if (!String(strings_data, offset, name)) return false;
    offset = (offset + name.size() + 1 + 3) & ~size_t(3);
    strings.push_back(std::move(name));
  }
  offset = size_t(strings_size) + 12;
  while (offset < data.size()) {
    if (!Contains(data, offset, 4)) return false;
    const uint32_t size = Be32(data.data() + offset);
    if (!size) break;
    if (size < 40 || !Contains(data, offset, size)) return false;
    const auto* library = data.data() + offset;
    const uint32_t name_index = Be16(library + 36) & 255;
    const uint32_t count = Be16(library + 38);
    if (name_index >= strings.size() || count > (size - 40) / 4) return false;
    ImportLibrary parsed;
    parsed.name = strings[name_index];
    const auto separator = parsed.name.find_last_of("/\\");
    if (separator != std::string::npos) parsed.name.erase(0, separator + 1);
    parsed.id = Be32(library + 24);
    parsed.version.value = Be32(library + 28);
    parsed.min_version.value = Be32(library + 32);
    std::map<uint16_t, ImportFunction> imports;
    for (uint32_t i = 0; i < count; ++i) {
      const uint32_t address = Be32(library + 40 + size_t(i) * 4);
      if (!address) continue;
      if (address < base || !Contains(image, address - base, 4)) return false;
      const uint32_t value = Be32(image.data() + (address - base));
      const uint16_t ordinal = uint16_t(value);
      auto& entry = imports[ordinal];
      entry.ordinal = ordinal;
      if (value >> 24 == 0) {
        entry.value_address = address;
        parsed.variable_slots.push_back({ordinal, address, 0});
      } else if (value >> 24 == 1) entry.thunk_address = address;
    }
    for (const auto& [ordinal, entry] : imports) parsed.imports.push_back(entry);
    libraries.push_back(std::move(parsed));
    offset += size;
  }
  return true;
}

bool ParsePe(std::span<const uint8_t> image, uint32_t base, PeMetadata& metadata) {
  if (!Contains(image, 0, 64) || Le16(image.data()) != 0x5A4D) return false;
  const uint32_t nt = Le32(image.data() + 60);
  if (!Contains(image, nt, 248)) return false;
  const auto* p = image.data() + nt;
  if (Le32(p) != 0x4550 || Le16(p + 4) != 0x1F2 ||
      !(Le16(p + 22) & 0x100) || Le16(p + 20) != 224 ||
      Le16(p + 24) != 0x10B || Le16(p + 24 + 68) != 14) return false;
  const uint16_t count = Le16(p + 6);
  if (!Contains(image, size_t(nt) + 248, size_t(count) * 40)) return false;
  PeMetadata parsed;
  parsed.timestamp = Le32(p + 8);
  parsed.exception_rva = Le32(p + 24 + 96 + 3 * 8);
  parsed.exception_size = Le32(p + 24 + 96 + 3 * 8 + 4);
  for (uint32_t i = 0; i < count; ++i) {
    const auto* section = p + 248 + size_t(i) * 40;
    PeSection entry;
    std::memcpy(entry.name, section, 8);
    entry.raw_address = Le32(section + 20);
    entry.raw_size = Le32(section + 16);
    entry.address = base + Le32(section + 12);
    entry.size = Le32(section + 8);
    entry.flags = Le32(section + 36);
    parsed.sections.push_back(entry);
  }
  metadata = std::move(parsed);
  return true;
}

uint32_t FindXexExport(std::span<const uint8_t> table, uint16_t ordinal) {
  if (table.size() < 44) return 0;
  // The guest ABI truncates ordinal subtraction to 16 bits.
  ordinal = uint16_t(ordinal - Be32(table.data() + 40));
  const uint32_t count = Be32(table.data() + 36);
  if (ordinal >= count || !Contains(table, 44 + size_t(ordinal) * 4, 4)) return 0;
  return Be32(table.data() + 44 + size_t(ordinal) * 4) + (Be32(table.data() + 32) << 16);
}

uint32_t FindPeExport(std::span<const uint8_t> image, uint32_t directory_rva,
                      uint32_t base, uint16_t ordinal) {
  if (!Contains(image, directory_rva, 40)) return 0;
  const auto* directory = image.data() + directory_rva;
  const uint32_t functions = Le32(directory + 28);
  if (ordinal >= Le32(directory + 20) ||
      !Contains(image, uint64_t(directory_rva) + functions + size_t(ordinal) * 4, 4)) return 0;
  return base + Le32(directory + functions + size_t(ordinal) * 4);
}

uint32_t FindPeExport(std::span<const uint8_t> image, uint32_t directory_rva,
                      uint32_t base, std::string_view name) {
  if (!Contains(image, directory_rva, 40)) return 0;
  const auto* directory = image.data() + directory_rva;
  const uint32_t count = Le32(directory + 24);
  const uint32_t names = Le32(directory + 32), ordinals = Le32(directory + 36);
  if (!Contains(image, uint64_t(directory_rva) + names, uint64_t(count) * 4) ||
      !Contains(image, uint64_t(directory_rva) + ordinals, uint64_t(count) * 2)) return 0;
  for (uint32_t i = 0; i < count; ++i) {
    const uint32_t offset = Le32(directory + names + size_t(i) * 4);
    std::string candidate;
    if (!String(image, uint64_t(directory_rva) + offset, candidate)) return 0;
    if (candidate == name) return FindPeExport(image, directory_rva, base,
                                              Le16(directory + ordinals + size_t(i) * 2));
  }
  return 0;
}
}  // namespace gh::image
