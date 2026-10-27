#pragma once

#include <filesystem>
#include <string_view>

namespace gh::logging {

enum class Level { kInfo, kWarning, kError };

void Initialize(const std::filesystem::path& file = {});
void Shutdown() noexcept;
void Write(Level level, std::string_view message) noexcept;
inline void Info(std::string_view message) noexcept { Write(Level::kInfo, message); }
inline void Warning(std::string_view message) noexcept { Write(Level::kWarning, message); }
inline void Error(std::string_view message) noexcept { Write(Level::kError, message); }

}  // namespace gh::logging
