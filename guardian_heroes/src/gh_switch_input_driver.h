#pragma once

#include <array>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <rex/input/input_driver.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <switch/runtime/pad.h>
#ifdef __cplusplus
}
#endif

namespace rex::input {
class InputSystem;
}

namespace rex::input::switch_ {

// libnx Npad adapter for the ReXGlue XInput-shaped input interface.
//
// The driver deliberately owns only the Npad polling and vibration handles.
// InputSystem remains responsible for assigning the active physical devices to
// guest users. The source is kept separate from the desktop SDL driver so the
// Switch target never needs SDL or a window listener.
class SwitchInputDriver final : public InputDriver {
 public:
  explicit SwitchInputDriver(rex::ui::Window* window, size_t window_z_order);
  ~SwitchInputDriver() override;

  X_STATUS Setup() override;

  void EnumerateDevices(std::vector<DeviceInfo>& out) override;
  X_RESULT GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) override;
  X_RESULT GetDeviceCapabilities(DeviceId id, uint32_t flags,
                                 X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetDeviceKeystroke(DeviceId id, uint32_t flags,
                              X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  struct Device {
    DeviceId id = DeviceId::kInvalid;
    HidNpadIdType npad = HidNpadIdType_No1;
    bool handheld = false;
    u64 previous_buttons = 0;
    u64 current_buttons = 0;
    u64 sampling_number = 0;
    X_INPUT_STATE state{};
    std::deque<X_INPUT_KEYSTROKE> keystrokes;
    uint32_t packet_number = 0;
    bool has_state = false;
    std::array<HidVibrationDeviceHandle, 2> vibration{};
    int vibration_count = 0;
  };

  static DeviceId MakeDeviceId(HidNpadIdType id);
  static bool IsHandheld(DeviceId id);
  static HidNpadIdType ToNpadId(DeviceId id);
  static bool ReadCommonState(HidNpadIdType id, HidNpadCommonState& state);
  static void FillGamepad(const HidNpadCommonState& state, X_INPUT_GAMEPAD& out);
  static uint16_t KeyForButton(uint64_t button);

  Device* Find(DeviceId id);
  const Device* Find(DeviceId id) const;
  void RefreshState(Device& device);

  PadState pad_{};
  bool initialized_ = false;
  std::array<Device, 9> devices_{};
  std::mutex mutex_;
};

std::unique_ptr<rex::input::InputSystem> CreateSwitchInputSystem(bool tool_mode);

}  // namespace rex::input::switch_
