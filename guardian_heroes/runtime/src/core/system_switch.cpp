#include <cstdio>
#include <rex/system.h>

namespace rex {
void ShowSimpleMessageBox(SimpleMessageBoxType type, std::string_view message) {
  const char* level = type == SimpleMessageBoxType::Error ? "ERROR" :
      (type == SimpleMessageBoxType::Warning ? "WARNING" : "INFO");
  std::fprintf(stderr, "[%s] %.*s\n", level, static_cast<int>(message.size()), message.data());
}
}  // namespace rex
