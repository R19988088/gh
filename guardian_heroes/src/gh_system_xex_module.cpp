/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include "gh_image.h"
#include "gh_image_metadata.h"
#include "gh_image_loader.h"

#include <algorithm>

#include <fmt/format.h>

#include <rex/logging.h>
#include <rex/math.h>
#include <rex/memory.h>
#include <rex/runtime.h>
#include <rex/system/export_resolver.h>
#include <rex/system/flags.h>
#include <rex/system/kernel_state.h>
#include <rex/system/function_dispatcher.h>
#include "gh_system_xex_module.h"
#include <rex/system/xmodule.h>
#include <rex/types.h>

namespace rex::runtime {

using rex::system::KernelState;
static_assert(offsetof(xex1_security_info, page_descriptors) == 0x168);
static_assert(offsetof(xex2_security_info, page_descriptors) == 0x184);
static_assert(sizeof(xex2_page_descriptor) == 24);

XexModule::XexModule(FunctionDispatcher* function_dispatcher, KernelState* kernel_state)
    : Module(function_dispatcher), kernel_state_(kernel_state),
      metadata_(std::make_unique<gh::image::XexMetadata>()) {}

XexModule::~XexModule() {}

const xex2_header* XexModule::xex_header() const {
  return reinterpret_cast<const xex2_header*>(metadata_->HeaderBytes().data());
}

uint32_t XexModule::image_size() const {
  return metadata_->ImageSize(memory()->LookupHeap(base_address_)->page_size());
}

bool XexModule::GetOptHeader(const xex2_header* header, xex2_header_keys key, void** out_ptr) {
  if (!header || !out_ptr) return false;
  gh::image::OptionalHeader field;
  const auto bytes = std::span(reinterpret_cast<const uint8_t*>(header), size_t(header->header_size));
  if (!gh::image::FindOptionalHeader(bytes, key, field)) return false;
  if (!(key & 255)) *reinterpret_cast<uint32_t*>(out_ptr) = field.value;
  else *out_ptr = const_cast<uint8_t*>(bytes.data() + field.offset);
  return true;
}

bool XexModule::GetOptHeader(xex2_header_keys key, void** out_ptr) const {
  return XexModule::GetOptHeader(xex_header(), key, out_ptr);
}

const void* XexModule::GetSecurityInfo(const xex2_header* header) {
  return reinterpret_cast<const uint8_t*>(header) + header->security_offset;
}

const PESection* XexModule::GetPESection(const char* name) {
  for (std::vector<PESection>::iterator it = pe_sections_.begin(); it != pe_sections_.end(); ++it) {
    if (!strcmp(it->name, name)) {
      return &(*it);
    }
  }
  return nullptr;
}

uint32_t XexModule::GetProcAddress(uint16_t ordinal) const {
  if (xex_security_info()->export_table) {
    const uint32_t address = xex_security_info()->export_table;
    const uint32_t size = image_size();
    if (address < base_address_ || address - base_address_ > size) return 0;
    return gh::image::FindXexExport(
        {memory()->TranslateVirtual(address), size - (address - base_address_)}, ordinal);
  }
  xex2_opt_data_directory* directory = nullptr;
  if (!GetOptHeader(XEX_HEADER_EXPORTS_BY_NAME, &directory)) return 0;
  return gh::image::FindPeExport(
      {memory()->TranslateVirtual(base_address_), image_size()}, directory->offset,
      base_address_, ordinal);
}

uint32_t XexModule::GetProcAddress(const std::string_view name) const {
  xex2_opt_data_directory* directory = nullptr;
  if (!GetOptHeader(XEX_HEADER_EXPORTS_BY_NAME, &directory)) return 0;
  return gh::image::FindPeExport(
      {memory()->TranslateVirtual(base_address_), image_size()}, directory->offset,
      base_address_, name);
}

int XexModule::ApplyPatch(XexModule* module) {
  if (!is_patch()) return 1;
  gh::image::Key base_key, patch_key{};
  std::copy_n(module->session_key_, 16, base_key.begin());
  gh::image::DeltaPatchMemory target;
  target.grow = [this, module](uint32_t old_size, uint32_t new_size) {
    const uint32_t address = module->base_address_ + old_size;
    const uint32_t size = new_size - old_size;
    const bool allocated = memory()->LookupHeap(address)->AllocFixed(address, size, 4096,
        memory::kMemoryAllocationReserve | memory::kMemoryAllocationCommit,
        memory::kMemoryProtectRead | memory::kMemoryProtectWrite);
    if (!allocated) {
      REXLOG_ERROR("Unable to allocate XEX memory at {:08X}-{:08X}.", address, size);
      assert_always();
    }
    return allocated;
  };
  target.shrink = [this, module](uint32_t old_size, uint32_t new_size) {
    const uint32_t address = module->base_address_ + new_size;
    const uint32_t size = old_size - new_size;
    if (!memory()->LookupHeap(address)->Decommit(address, size)) {
      REXLOG_ERROR("Unable to decommit XEX memory at {:08X}-{:08X}.", address, size);
      assert_always();
    }
  };
  target.image = [this, module](uint32_t size) {
    return std::span(memory()->TranslateVirtual(module->base_address_), size);
  };
  target.log = [](bool warning, std::string_view message) {
    if (warning) REXLOG_WARN("{}", message);
    else REXLOG_ERROR("{}", message);
  };
  gh::image::PatchVersions versions;
  const int result = gh::image::ApplyDeltaPatch(*metadata_, xexp_data_mem_,
      *module->metadata_, module->is_dev_kit_,
      memory()->LookupHeap(module->base_address_)->page_size(), base_key, patch_key,
      target, versions);
  std::copy(base_key.begin(), base_key.end(), module->session_key_);
  std::copy(patch_key.begin(), patch_key.end(), session_key_);
  // The project owns patched header storage; refresh the exported ABI pointers.
  const bool refreshed = module->ReadSecurityInfo();
  if (!result && !refreshed) return 1;
  if (!result) {
    const xex2_version source{versions.source}, destination{versions.target};
    REXLOG_INFO("XEX patch applied successfully: base version: {}.{}.{}.{}, new version: {}.{}.{}.{}",
        uint32_t(source.major), uint32_t(source.minor), uint32_t(source.build), uint32_t(source.qfe),
        uint32_t(destination.major), uint32_t(destination.minor), uint32_t(destination.build),
        uint32_t(destination.qfe));
  }
  return result;
}

int XexModule::ReadImage(const void* xex_addr, size_t xex_length, bool use_dev_key) {
  const auto* format = opt_file_format_info();
  const size_t header_size = xex_header()->header_size;
  if (!format || header_size > xex_length) return 1;
  is_dev_kit_ = use_dev_key;
  const auto payload = std::span(static_cast<const uint8_t*>(xex_addr), xex_length)
                           .subspan(header_size);
  if (is_patch()) {
    xexp_data_mem_.assign(payload.begin(), payload.end());
    return 0;
  }

  const auto format_offset = reinterpret_cast<uintptr_t>(format) -
                             reinterpret_cast<uintptr_t>(xex_header());
  if (format_offset > metadata_->HeaderBytes().size() ||
      metadata_->HeaderBytes().size() - format_offset < 8 ||
      format->info_size > metadata_->HeaderBytes().size() - format_offset) return 1;

  auto* heap = memory()->LookupHeap(base_address_);
  heap->Reset();
  const auto key = gh::image::SessionKey(
      std::span<const uint8_t, 16>(reinterpret_cast<const uint8_t*>(xex_security_info()->aes_key), 16), use_dev_key);
  std::copy(key.begin(), key.end(), session_key_);

  const uint32_t allocation_size = image_size();
  std::vector<uint8_t> image;
  const int result = gh::image::DecodePayload(payload,
      std::span(reinterpret_cast<const uint8_t*>(format), size_t(format->info_size)),
      key, allocation_size, image);
  if (result) return result;
  if (!heap->AllocFixed(base_address_, image.size(), 4096,
          rex::memory::kMemoryAllocationReserve | rex::memory::kMemoryAllocationCommit,
          rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite)) {
    REXLOG_ERROR("Unable to allocate XEX memory at {:08X}-{:08X}.", base_address_, image.size());
    return 3;
  }
  std::memcpy(memory()->TranslateVirtual(base_address_), image.data(), image.size());
  return is_valid_executable() ? 0 : 3;
}

int XexModule::ReadPEHeaders() {
  gh::image::PeMetadata parsed;
  if (!gh::image::ParsePe({memory()->TranslateVirtual(base_address_), image_size()},
                          base_address_, parsed)) return 1;
  pe_sections_.clear();
  for (const auto& source : parsed.sections) {
    PESection section;
    std::copy_n(source.name, sizeof(section.name), section.name);
    section.raw_address = source.raw_address;
    section.raw_size = source.raw_size;
    section.address = source.address;
    section.size = source.size;
    section.flags = source.flags;
    pe_sections_.push_back(section);
  }
  pe_time_date_stamp_ = parsed.timestamp;
  exception_dir_rva_ = parsed.exception_rva;
  exception_dir_size_ = parsed.exception_size;
  return 0;
}

bool XexModule::ReadSecurityInfo() {
  if (!metadata_->Refresh()) return false;
  const auto& security = metadata_->Security();
  security_info_.rsa_signature = reinterpret_cast<const char*>(security.signature.data());
  security_info_.aes_key = reinterpret_cast<const char*>(security.key.data());
  security_info_.image_size = security.image_size;
  security_info_.image_flags = security.flags;
  security_info_.export_table = security.export_table;
  security_info_.load_address = security.load_address;
  security_info_.page_descriptor_count = uint32_t(security.pages.size());
  security_info_.page_descriptors = reinterpret_cast<const xex2_page_descriptor*>(
      metadata_->HeaderBytes().data() + security.page_table_offset);
  return true;
}

bool XexModule::Load(const std::string_view name, const std::string_view path, const void* xex_addr,
                     size_t xex_length) {
  assert_false(loaded_);
  if (!metadata_->Load({static_cast<const uint8_t*>(xex_addr), xex_length}) ||
      !ReadSecurityInfo()) return false;
  xex_format_ = metadata_->IsXex1() ? kFormatXex1 : kFormatXex2;
  loaded_ = true;
  base_address_ = metadata_->Security().load_address;
  rex::be<uint32_t>* base_addr_opt = nullptr;
  if (GetOptHeader(XEX_HEADER_IMAGE_BASE_ADDRESS, &base_addr_opt)) base_address_ = *base_addr_opt;

  // Setup debug info.
  name_ = name;
  path_ = path;

  // Load in the XEX basefile
  // We'll try using both XEX2 keys to see if any give a valid PE
  int result_code = ReadImage(xex_addr, xex_length, false);
  if (result_code) {
    REXLOG_WARN("XEX load failed with code {}, trying with devkit encryption key...", result_code);

    result_code = ReadImage(xex_addr, xex_length, true);
    if (result_code) {
      REXLOG_ERROR("XEX load failed with code {}, tried both encryption keys", result_code);
      return false;
    }
  }

  // Note: caller will have to call LoadContinue once it's determined whether a
  // patch file exists or not!
  return true;
}

bool XexModule::LoadContinue() {
  // Second part of image load
  // Split from Load() so that we can patch the XEX before loading this data
  assert_false(finished_load_);
  if (finished_load_) {
    return true;
  }

  finished_load_ = true;

  if (ReadPEHeaders()) {
    REXLOG_ERROR("Failed to load XEX PE headers!");
    return false;
  }

  if (!metadata_->AlternateTitleIds(opt_alternate_title_ids_)) return false;
  auto* heap = memory()->LookupHeap(base_address_);
  const auto page_size = heap->page_size();
  low_address_ = UINT_MAX;
  high_address_ = 0;
  uint32_t page = 0;
  for (const auto descriptor : metadata_->Security().pages) {
    const auto start = base_address_ + page * page_size;
    const auto end = start + descriptor.count * page_size;
    if (descriptor.type == XEX_SECTION_CODE) {
      low_address_ = std::min(low_address_, start);
      high_address_ = std::max(high_address_, end);
    }
    page += descriptor.count;
  }

  std::vector<gh::image::ImportLibrary> libraries;
  if (!metadata_->ParseImports({memory()->TranslateVirtual(base_address_), image_size()},
                               base_address_, libraries)) return false;
  for (const auto& library : libraries) {
    if (!SetupLibraryImports(library)) return false;
  }

  // Guest protection remains an SDK heap operation over the parsed page table.
  page = 0;
  for (const auto descriptor : metadata_->Security().pages) {
    const auto address = base_address_ + page * page_size;
    const auto size = descriptor.count * page_size;
    switch (descriptor.type) {
      case XEX_SECTION_CODE:
      case XEX_SECTION_READONLY_DATA:
        heap->Protect(address, size, memory::kMemoryProtectRead);
        break;
      case XEX_SECTION_DATA:
        heap->Protect(address, size, memory::kMemoryProtectRead | memory::kMemoryProtectWrite);
        break;
    }
    page += descriptor.count;
  }

  // Populate binary introspection data
  PopulateBinaryData();

  return true;
}

bool XexModule::Unload() {
  if (!loaded_) {
    return true;
  }
  loaded_ = false;

  // If this isn't a patch, just deallocate the memory occupied by the exe
  if (!is_patch()) {
    assert_not_zero(base_address_);

    memory()->LookupHeap(base_address_)->Release(base_address_);
  }

  metadata_->HeaderBytes().resize(0);

  return true;
}

bool XexModule::SetupLibraryImports(const gh::image::ImportLibrary& library) {
  auto* resolver = kernel_state_->emulator()->export_resolver();
  gh::image::ImportBindingTarget target;
  target.resolve = [resolver](std::string_view name, uint16_t ordinal) {
    auto* entry = resolver ? resolver->GetExportByOrdinal(name, ordinal) : nullptr;
    if (!entry) return gh::image::ResolvedImport{};
    const bool variable = entry->type == runtime::Export::Type::kVariable;
    return gh::image::ResolvedImport{variable, entry->is_implemented(), entry->ordinal,
        variable ? entry->variable_ptr : 0, entry->name};
  };
  target.write = [this](uint32_t address, uint32_t value) {
    *memory()->TranslateVirtual<rex::be<uint32_t>*>(address) = value;
  };
  target.bound = [](const gh::image::ImportLibrary& source, const gh::image::ImportFunction& slot,
                    const gh::image::ResolvedImport& entry, bool implemented) {
    if (implemented) {
      REXLOG_DEBUG("Patched variable import {}:{:#x} ({}) -> {:#x}", source.name,
                   slot.ordinal, entry.name, entry.address);
    } else {
      REXLOG_WARN("Variable import {}:{:#x} ({}) not implemented", source.name,
                  slot.ordinal, entry.name);
    }
  };
  gh::image::BindImports(library, target);
  ImportLibrary imported;
  imported.name = library.name;
  imported.id = library.id;
  imported.version.value = library.version.value;
  imported.min_version.value = library.min_version.value;
  for (const auto& entry : library.imports) {
    imported.imports.push_back({entry.ordinal, entry.value_address, entry.thunk_address});
  }
  REXLOG_DEBUG("created symbols for import library {} with {} imports", imported.name,
               imported.imports.size());
  import_libs_.push_back(std::move(imported));
  return true;
}

bool XexModule::ContainsAddress(uint32_t address) {
  return address >= low_address_ && address < high_address_;
}

// Binary introspection implementation
void XexModule::PopulateBinaryData() {
  binary_sections_.clear();
  binary_symbols_.clear();

  // Populate sections from existing PE sections
  for (const auto& pe_sec : pe_sections_) {
    BinarySection sec;
    sec.name = pe_sec.name;
    sec.virtual_address = pe_sec.address;
    sec.virtual_size = pe_sec.size;
    sec.host_data = memory()->TranslateVirtual<uint8_t*>(pe_sec.address);
    sec.executable = (pe_sec.flags & kXEPESectionMemoryExecute) != 0;
    sec.writable = (pe_sec.flags & kXEPESectionMemoryWrite) != 0;
    binary_sections_.push_back(std::move(sec));
  }

  // Populate symbols from import libraries
  for (const auto& lib : import_libs_) {
    for (const auto& import : lib.imports) {
      BinarySymbol sym;
      sym.name = fmt::format("{}@{}", lib.name, import.ordinal);
      sym.address = import.thunk_address;
      sym.size = 16;  // Thunk size: 2 ordinal words + mtctr + bctr
      sym.type = BinarySymbolType::Import;
      binary_symbols_.push_back(std::move(sym));
    }
  }
}

std::span<const BinarySection> XexModule::binary_sections() const {
  return binary_sections_;
}

const BinarySection* XexModule::FindSectionByName(std::string_view name) const {
  for (const auto& sec : binary_sections_) {
    if (sec.name == name)
      return &sec;
  }
  return nullptr;
}

std::span<const BinarySymbol> XexModule::binary_symbols() const {
  return binary_symbols_;
}

}  // namespace rex::runtime
