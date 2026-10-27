#include "gh_controller_assignment.h"

namespace gh::input {

void ControllerAssignmentLogic::OnDevicesChanged(
    const std::vector<ControllerDevice>& devices) {
  for (auto& user : users_) {
    user.clear();
  }
  for (const auto& device : devices) {
    if (device.synthetic) {
      users_[0].push_back(device.id);
    } else if (device.ordinal < kMaxGuestUsers) {
      users_[device.ordinal].push_back(device.id);
    }
  }
}

void ControllerAssignmentLogic::DevicesForUser(
    uint32_t user_index, std::vector<ControllerDeviceId>& out) const {
  out.clear();
  if (user_index < kMaxGuestUsers) {
    out = users_[user_index];
  }
}

}  // namespace gh::input
