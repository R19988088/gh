#pragma once

#include "gh_controller_assignment.h"

#include <rex/input/device_assignment.h>

namespace gh::input {

// Thin SDK adapter: all assignment policy lives in ControllerAssignmentLogic.
class ControllerAssignment final : public rex::input::DeviceAssignment {
 public:
  void OnDevicesChanged(
      const std::vector<rex::input::DeviceInfo>& devices) override;
  void DevicesForUser(uint32_t user_index,
                      std::vector<rex::input::DeviceId>& out) const override;

 private:
  ControllerAssignmentLogic logic_;
};

}  // namespace gh::input
