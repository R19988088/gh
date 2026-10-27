#include "gh_controller_assignment.h"

#include <cassert>
#include <cstdint>
#include <vector>

int main() {
  using gh::input::ControllerAssignmentLogic;
  using gh::input::ControllerDevice;
  using gh::input::ControllerDeviceId;

  ControllerAssignmentLogic assignment;
  assignment.OnDevicesChanged({
      {.id = ControllerDeviceId{1}, .ordinal = 0, .synthetic = false},
      {.id = ControllerDeviceId{2}, .ordinal = 1, .synthetic = false},
      {.id = ControllerDeviceId{3}, .ordinal = 2, .synthetic = false},
      {.id = ControllerDeviceId{4}, .ordinal = 3, .synthetic = false},
      {.id = ControllerDeviceId{5}, .ordinal = 4, .synthetic = false},
      {.id = ControllerDeviceId{6}, .ordinal = 99, .synthetic = true},
  });

  std::vector<ControllerDeviceId> out;
  assignment.DevicesForUser(0, out);
  assert((out == std::vector<ControllerDeviceId>{1, 6}));
  assignment.DevicesForUser(1, out);
  assert((out == std::vector<ControllerDeviceId>{2}));
  assignment.DevicesForUser(2, out);
  assert((out == std::vector<ControllerDeviceId>{3}));
  assignment.DevicesForUser(3, out);
  assert((out == std::vector<ControllerDeviceId>{4}));
  assignment.DevicesForUser(ControllerAssignmentLogic::kMaxGuestUsers, out);
  assert(out.empty());

  assignment.OnDevicesChanged({{.id = ControllerDeviceId{2},
                                .ordinal = 1,
                                .synthetic = false}});
  assignment.DevicesForUser(0, out);
  assert(out.empty());
  assignment.DevicesForUser(1, out);
  assert((out == std::vector<ControllerDeviceId>{2}));
}
