#include <rex/platform/env.h>
#include <optional>
#include <string>

namespace rex::platform::env {
std::optional<std::string> get(std::string_view) { return std::nullopt; }
bool set(std::string_view, std::string_view) { return false; }
bool unset(std::string_view) { return false; }
}  // namespace rex::platform::env
