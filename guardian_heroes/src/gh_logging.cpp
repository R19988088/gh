#include "gh_logging.h"

#include "gh_clock.h"

#include <cstdio>
#include <mutex>

namespace gh::logging {
namespace {
std::mutex mutex;
FILE* file = nullptr;

const char* Name(Level level) noexcept {
  switch (level) {
    case Level::kWarning: return "WARN";
    case Level::kError: return "ERROR";
    default: return "INFO";
  }
}
}  // namespace

void Initialize(const std::filesystem::path& path) {
  std::lock_guard lock(mutex);
  if (file) return;
  if (!path.empty()) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    file = std::fopen(path.string().c_str(), "a");
  }
}

void Shutdown() noexcept {
  std::lock_guard lock(mutex);
  if (file) {
    std::fclose(file);
    file = nullptr;
  }
}

void Write(Level level, std::string_view message) noexcept {
  std::lock_guard lock(mutex);
  const auto timestamp = clock::HostSystemTime();
  std::fprintf(stderr, "[%llu] [%s] %.*s\n", static_cast<unsigned long long>(timestamp),
               Name(level), static_cast<int>(message.size()), message.data());
  if (file) {
    std::fprintf(file, "[%llu] [%s] %.*s\n", static_cast<unsigned long long>(timestamp),
                 Name(level), static_cast<int>(message.size()), message.data());
    std::fflush(file);
  }
}

}  // namespace gh::logging
