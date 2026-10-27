#include "gh_controller_assignment_adapter.h"

namespace gh::input {

void ControllerAssignment::OnDevicesChanged(
    const std::vector<rex::input::DeviceInfo>& devices) {
  std::vector<ControllerDevice> project_devices;
  project_devices.reserve(devices.size());
  for (const auto& device : devices) {
    project_devices.push_back({static_cast<ControllerDeviceId>(device.id),
                               device.ordinal, device.synthetic});
  }
  logic_.OnDevicesChanged(project_devices);
}

void ControllerAssignment::DevicesForUser(
    uint32_t user_index, std::vector<rex::input::DeviceId>& out) const {
  std::vector<ControllerDeviceId> project_ids;
  logic_.DevicesForUser(user_index, project_ids);
  out.clear();
  out.reserve(project_ids.size());
  for (ControllerDeviceId id : project_ids) {
    out.push_back(static_cast<rex::input::DeviceId>(id));
  }
}

}  // namespace gh::input
