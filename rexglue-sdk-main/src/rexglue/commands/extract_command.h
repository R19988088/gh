/**
 * @file        rexglue/commands/extract_command.h
 * @brief       STFS package extraction command
 *
 * @copyright   Copyright (c) 2026 Tom Clay
 * @license     BSD 3-Clause License
 */

#pragma once

#include "../cli_utils.h"

#include <string>

namespace CLI {
class App;
}

namespace rexglue::cli {

rex::Result<void> ExtractPackage(const std::string& package_path, const std::string& output_dir);

void RegisterExtract(CLI::App& parent, DeferredAction& pending);

}  // namespace rexglue::cli
