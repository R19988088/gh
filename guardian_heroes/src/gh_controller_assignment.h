#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace gh::input {

using ControllerDeviceId = uint64_t;

// The project-owned device description keeps assignment policy independent of
// the input driver and its SDK-specific device handle type.
struct ControllerDevice {
  ControllerDeviceId id = 0;
  uint32_t ordinal = 0;
  bool synthetic = false;
};

class ControllerAssignmentLogic final {
 public:
  static constexpr uint32_t kMaxGuestUsers = 4;

  void OnDevicesChanged(const std::vector<ControllerDevice>& devices);
  void DevicesForUser(uint32_t user_index,
                      std::vector<ControllerDeviceId>& out) const;

 private:
  std::array<std::vector<ControllerDeviceId>, kMaxGuestUsers> users_;
};

}  // namespace gh::input
