#include "gh_controller_assignment_adapter.h"

#include <cassert>

int main() {
  using namespace rex::input;
  gh::input::ControllerAssignment assignment;
  const std::vector<DeviceInfo> devices = {
      {.id = DeviceId{1}, .ordinal = 0},
      {.id = DeviceId{2}, .ordinal = 1},
      {.id = DeviceId{6}, .ordinal = 99, .synthetic = true},
      {.id = DeviceId{7}, .ordinal = 99},
  };
  assignment.OnDevicesChanged(devices);

  std::vector<DeviceId> out;
  assignment.DevicesForUser(0, out);
  assert((out == std::vector<DeviceId>{DeviceId{1}, DeviceId{6}}));
  assignment.DevicesForUser(1, out);
  assert((out == std::vector<DeviceId>{DeviceId{2}}));
  assignment.DevicesForUser(kMaxGuestUsers, out);
  assert(out.empty());

  assignment.OnDevicesChanged({devices[1]});
  assignment.DevicesForUser(0, out);
  assert(out.empty());
  assignment.DevicesForUser(1, out);
  assert((out == std::vector<DeviceId>{DeviceId{2}}));
}
