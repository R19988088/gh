#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

namespace gh {

class Exception : public std::runtime_error {
 public:
  explicit Exception(const char* message, uint32_t code = 0) : std::runtime_error(message), code_(code) {}
  explicit Exception(const std::string& message, uint32_t code = 0)
      : std::runtime_error(message), code_(code) {}
  uint32_t code() const noexcept { return code_; }

 private:
  uint32_t code_;
};

namespace exceptions {
void InstallTerminateHandler() noexcept;
}

}  // namespace gh
