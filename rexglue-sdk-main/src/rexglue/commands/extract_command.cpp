/**
 * @file        rexglue/commands/extract_command.cpp
 * @brief       STFS package extraction command
 *
 * @copyright   Copyright (c) 2026 Tom Clay
 * @license     BSD 3-Clause License
 */

#include "extract_command.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/format.h>

#include <rex/filesystem/devices/stfs_container_device.h>
#include <rex/filesystem/entry.h>
#include <rex/filesystem/file.h>
#include <rex/logging.h>

namespace fs = std::filesystem;

namespace rexglue::cli {

using rex::Err;
using rex::ErrorCategory;
using rex::Ok;
using rex::Result;
using rex::X_STATUS;

namespace {

Result<void> ExtractChildren(rex::filesystem::Entry& directory, const fs::path& output_dir,
                             std::span<uint8_t> buffer, size_t& file_count,
                             uint64_t& total_bytes) {
  for (const auto& child : directory.children()) {
    const auto& name = child->name();
    if (name.empty() || name == "." || name == ".." ||
        name.find_first_of("/\\") != std::string::npos) {
      return Err(ErrorCategory::Format, "Invalid package entry name: " + name);
    }
    const fs::path destination = output_dir / rex::to_path(name);
    std::error_code ec;
    if (fs::exists(destination, ec)) {
      return Err(ErrorCategory::IO, "Output entry already exists: " + destination.string());
    }
    if (ec) {
      return Err(ErrorCategory::IO,
                 "Cannot inspect output entry '" + destination.string() + "': " + ec.message());
    }

    if (child->attributes() & rex::filesystem::kFileAttributeDirectory) {
      fs::create_directory(destination, ec);
      if (ec) {
        return Err(ErrorCategory::IO,
                   "Cannot create directory '" + destination.string() + "': " + ec.message());
      }
      auto result = ExtractChildren(*child, destination, buffer, file_count, total_bytes);
      if (!result) {
        return result;
      }
      continue;
    }

    rex::filesystem::File* raw_file = nullptr;
    auto status = child->Open(rex::filesystem::FileAccess::kFileReadData, &raw_file);
    const auto destroy_file = [](rex::filesystem::File* file) { file->Destroy(); };
    std::unique_ptr<rex::filesystem::File, decltype(destroy_file)> input(raw_file, destroy_file);
    if (status != X_STATUS_SUCCESS || !input) {
      return Err(ErrorCategory::IO,
                 fmt::format("Cannot open package entry '{}' (status 0x{:08X})", child->path(),
                             status));
    }
    std::ofstream output(destination, std::ios::binary);
    if (!output) {
      return Err(ErrorCategory::IO, "Cannot open output file: " + destination.string());
    }

    for (size_t offset = 0; offset < child->size();) {
      const size_t requested = std::min(buffer.size(), child->size() - offset);
      size_t bytes_read = 0;
      status = input->ReadSync(buffer.first(requested), offset, &bytes_read);
      if (status != X_STATUS_SUCCESS || bytes_read != requested) {
        return Err(ErrorCategory::IO,
                   fmt::format("Cannot read package entry '{}' at {}: status 0x{:08X}, "
                               "read {} of {} bytes",
                               child->path(), offset, status, bytes_read, requested));
      }
      output.write(reinterpret_cast<const char*>(buffer.data()),
                   static_cast<std::streamsize>(bytes_read));
      if (!output) {
        return Err(ErrorCategory::IO, "Cannot write output file: " + destination.string());
      }
      offset += bytes_read;
    }
    output.close();
    if (!output) {
      return Err(ErrorCategory::IO, "Cannot close output file: " + destination.string());
    }
    ++file_count;
    total_bytes += child->size();
  }
  return Ok();
}

}  // namespace

Result<void> ExtractPackage(const std::string& package_path, const std::string& output_dir) {
  std::error_code ec;
  const auto source = fs::canonical(rex::to_path(package_path), ec);
  if (ec) {
    return Err(ErrorCategory::IO, "Cannot resolve package '" + package_path + "': " + ec.message());
  }
  if (!fs::is_regular_file(source, ec) || ec) {
    return Err(ErrorCategory::IO, "Package is not a regular file: " + source.string());
  }
  const auto absolute_output = fs::absolute(rex::to_path(output_dir), ec);
  if (ec) {
    return Err(ErrorCategory::IO,
               "Cannot resolve output directory '" + output_dir + "': " + ec.message());
  }
  const auto destination = fs::weakly_canonical(absolute_output, ec);
  if (ec) {
    return Err(ErrorCategory::IO,
               "Cannot resolve output directory '" + output_dir + "': " + ec.message());
  }
  if (source == destination) {
    return Err(ErrorCategory::IO, "Output directory must not overwrite the package");
  }
  const bool output_exists = fs::exists(destination, ec);
  if (ec) {
    return Err(ErrorCategory::IO, "Cannot inspect output directory: " + ec.message());
  }
  if (output_exists) {
    if (!fs::is_directory(destination, ec) || ec) {
      return Err(ErrorCategory::IO, "Output path is not a directory: " + destination.string());
    }
    if (!fs::is_empty(destination, ec) || ec) {
      return Err(ErrorCategory::IO, "Output directory must be empty: " + destination.string());
    }
  }

  rex::filesystem::StfsContainerDevice device("", source);
  if (!device.Initialize()) {
    return Err(ErrorCategory::Format, "Cannot initialize STFS package: " + source.string());
  }
  auto* root = device.ResolvePath("");
  if (!root || !(root->attributes() & rex::filesystem::kFileAttributeDirectory)) {
    return Err(ErrorCategory::Format, "Package has no root directory: " + source.string());
  }
  fs::create_directories(destination, ec);
  if (ec) {
    return Err(ErrorCategory::IO, "Cannot create output directory: " + ec.message());
  }

  std::vector<uint8_t> buffer(1024 * 1024);
  size_t file_count = 0;
  uint64_t total_bytes = 0;
  auto result = ExtractChildren(*root, destination, buffer, file_count, total_bytes);
  if (!result) {
    return result;
  }
  REXLOG_INFO("Extracted {} files ({} bytes) to {}", file_count, total_bytes, destination.string());
  return Ok();
}

void RegisterExtract(CLI::App& parent, DeferredAction& pending) {
  auto* extract = parent.add_subcommand("extract", "Extract an STFS package to an empty directory")
                      ->fallthrough();
  auto package_path = std::make_shared<std::string>();
  auto output_dir = std::make_shared<std::string>();
  extract->add_option("package", *package_path, "Path to the STFS package")->required();
  extract->add_option("output-directory", *output_dir, "New or empty output directory")->required();
  extract->callback([package_path, output_dir, &pending]() {
    pending = [package_path, output_dir]() { return ExtractPackage(*package_path, *output_dir); };
  });
}

}  // namespace rexglue::cli
