#include "gh_switch_input_driver.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>
#include <utility>

#include <rex/input/device_assignment.h>
#include <rex/input/flags.h>
#include <rex/input/input_system.h>
#include <rex/input/nop/nop_input_driver.h>
#include <rex/logging.h>
#include <rex/ui/virtual_key.h>

namespace rex::input::switch_ {
namespace {

constexpr uint64_t kDevicePrefix = 0x5357490000000000ull;  // "SWI"
constexpr uint64_t kHandheldBit = 0x100ull;
constexpr uint64_t kButtonMap[][2] = {
    {HidNpadButton_Up, X_INPUT_GAMEPAD_DPAD_UP},
    {HidNpadButton_Down, X_INPUT_GAMEPAD_DPAD_DOWN},
    {HidNpadButton_Left, X_INPUT_GAMEPAD_DPAD_LEFT},
    {HidNpadButton_Right, X_INPUT_GAMEPAD_DPAD_RIGHT},
    {HidNpadButton_Plus, X_INPUT_GAMEPAD_START},
    {HidNpadButton_Minus, X_INPUT_GAMEPAD_BACK},
    {HidNpadButton_StickL, X_INPUT_GAMEPAD_LEFT_THUMB},
    {HidNpadButton_StickR, X_INPUT_GAMEPAD_RIGHT_THUMB},
    {HidNpadButton_L, X_INPUT_GAMEPAD_LEFT_SHOULDER},
    {HidNpadButton_R, X_INPUT_GAMEPAD_RIGHT_SHOULDER},
    // Preserve the face-button geometry of an Xbox pad on Nintendo's layout.
    {HidNpadButton_A, X_INPUT_GAMEPAD_B},
    {HidNpadButton_B, X_INPUT_GAMEPAD_A},
    {HidNpadButton_X, X_INPUT_GAMEPAD_X},
    {HidNpadButton_Y, X_INPUT_GAMEPAD_Y},
};

constexpr std::array<uint16_t, 16> kButtonKeys = {
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadDpadUp),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadDpadDown),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadDpadLeft),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadDpadRight),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadStart),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadBack),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadLThumbPress),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadRThumbPress),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadLShoulder),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadRShoulder),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadB),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadA),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadX),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadY),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadLTrigger),
    static_cast<uint16_t>(rex::ui::VirtualKey::kXInputPadRTrigger),
};

int16_t ClampStick(int32_t value) {
  return static_cast<int16_t>(std::clamp(value, -32768, 32767));
}

uint8_t Trigger(bool pressed) { return pressed ? 0xff : 0; }

}  // namespace

SwitchInputDriver::SwitchInputDriver(rex::ui::Window* window, size_t window_z_order)
    : InputDriver(window, window_z_order) {
  for (size_t i = 0; i < 8; ++i) {
    devices_[i].id = MakeDeviceId(static_cast<HidNpadIdType>(HidNpadIdType_No1 + i));
    devices_[i].npad = static_cast<HidNpadIdType>(HidNpadIdType_No1 + i);
  }
  devices_[8].id = DeviceId{kDevicePrefix | kHandheldBit};
  devices_[8].npad = HidNpadIdType_Handheld;
  devices_[8].handheld = true;
}

SwitchInputDriver::~SwitchInputDriver() = default;

X_STATUS SwitchInputDriver::Setup() {
  std::lock_guard lock(mutex_);
  if (initialized_) return X_STATUS_SUCCESS;

  // padConfigureInput/padInitializeWithMask are libnx's Npad setup path. It
  // does not own the global hid session, which is initialized by libnx startup
  // and remains owned by the application lifecycle.
  padConfigureInput(8, HidNpadStyleSet_NpadStandard);
  u64 mask = 1ull << HidNpadIdType_Handheld;
  for (unsigned i = 0; i < 8; ++i) mask |= 1ull << (HidNpadIdType_No1 + i);
  padInitializeWithMask(&pad_, mask);
  initialized_ = true;
  return X_STATUS_SUCCESS;
}

void SwitchInputDriver::EnumerateDevices(std::vector<DeviceInfo>& out) {
  std::lock_guard lock(mutex_);
  if (!initialized_) return;
  padUpdate(&pad_);
  for (auto& device : devices_) {
    if (!padIsNpadActive(&pad_, device.npad)) {
      if (device.has_state) device.keystrokes.clear();
      device.has_state = false;
      continue;
    }
    RefreshState(device);
    DeviceInfo info;
    info.id = device.id;
    info.name = device.handheld ? "Switch handheld" : "Switch Npad " +
                                                       std::to_string(static_cast<unsigned>(device.npad) + 1);
    info.guid = "switch-npad-" + std::to_string(static_cast<unsigned>(device.npad));
    info.synthetic = false;
    out.push_back(std::move(info));
  }
}

bool SwitchInputDriver::ReadCommonState(HidNpadIdType id, HidNpadCommonState& state) {
  std::memset(&state, 0, sizeof(state));
  if (id == HidNpadIdType_Handheld) {
    HidNpadHandheldState value{};
    if (hidGetNpadStatesHandheld(id, &value, 1) == 1) {
      state = value;
      return true;
    }
    return false;
  }
  const u32 styles = hidGetNpadStyleSet(id);
  if (styles & HidNpadStyleTag_NpadFullKey) {
    HidNpadFullKeyState value{};
    if (hidGetNpadStatesFullKey(id, &value, 1) == 1) { state = value; return true; }
  }
  if (styles & HidNpadStyleTag_NpadHandheld) {
    HidNpadHandheldState value{};
    if (hidGetNpadStatesHandheld(id, &value, 1) == 1) { state = value; return true; }
  }
  if (styles & HidNpadStyleTag_NpadJoyDual) {
    HidNpadJoyDualState value{};
    if (hidGetNpadStatesJoyDual(id, &value, 1) == 1) { state = value; return true; }
  }
  if (styles & HidNpadStyleTag_NpadJoyLeft) {
    HidNpadJoyLeftState value{};
    if (hidGetNpadStatesJoyLeft(id, &value, 1) == 1) { state = value; return true; }
  }
  if (styles & HidNpadStyleTag_NpadJoyRight) {
    HidNpadJoyRightState value{};
    if (hidGetNpadStatesJoyRight(id, &value, 1) == 1) { state = value; return true; }
  }
  return false;
}

void SwitchInputDriver::RefreshState(Device& device) {
  HidNpadCommonState state{};
  if (!ReadCommonState(device.npad, state)) {
    device.has_state = false;
    return;
  }
  X_INPUT_GAMEPAD gamepad{};
  FillGamepad(state, gamepad);
  const bool changed = std::memcmp(&gamepad, &device.state.gamepad, sizeof(gamepad)) != 0;
  if (changed) ++device.packet_number;
  device.state.packet_number = device.packet_number;
  device.state.gamepad = gamepad;

  constexpr size_t kMaximumPendingKeystrokes = 64;
  for (const auto& mapping : kButtonMap) {
    const auto hid_button = mapping[0];
    const bool was_pressed = (device.current_buttons & hid_button) != 0;
    const bool is_pressed = (state.buttons & hid_button) != 0;
    if (was_pressed == is_pressed) continue;
    X_INPUT_KEYSTROKE event{};
    event.virtual_key = KeyForButton(hid_button);
    event.flags = is_pressed ? X_INPUT_KEYSTROKE_KEYDOWN : X_INPUT_KEYSTROKE_KEYUP;
    device.keystrokes.push_back(event);
    if (device.keystrokes.size() > kMaximumPendingKeystrokes) device.keystrokes.pop_front();
  }
  for (const auto [hid_button, trigger_key] : {
           std::pair<uint64_t, uint16_t>{HidNpadButton_ZL, kButtonKeys[14]},
           std::pair<uint64_t, uint16_t>{HidNpadButton_ZR, kButtonKeys[15]}}) {
    const bool was_pressed = (device.current_buttons & hid_button) != 0;
    const bool is_pressed = (state.buttons & hid_button) != 0;
    if (was_pressed == is_pressed) continue;
    X_INPUT_KEYSTROKE event{};
    event.virtual_key = trigger_key;
    event.flags = is_pressed ? X_INPUT_KEYSTROKE_KEYDOWN : X_INPUT_KEYSTROKE_KEYUP;
    device.keystrokes.push_back(event);
    if (device.keystrokes.size() > kMaximumPendingKeystrokes) device.keystrokes.pop_front();
  }
  device.previous_buttons = device.current_buttons;
  device.current_buttons = state.buttons;
  device.sampling_number = state.sampling_number;
  device.has_state = true;
}

void SwitchInputDriver::FillGamepad(const HidNpadCommonState& state, X_INPUT_GAMEPAD& out) {
  std::memset(&out, 0, sizeof(out));
  uint16_t buttons = 0;
  for (const auto& mapping : kButtonMap) {
    if (state.buttons & mapping[0]) buttons |= static_cast<uint16_t>(mapping[1]);
  }
  out.buttons = buttons;
  out.left_trigger = Trigger(state.buttons & HidNpadButton_ZL);
  out.right_trigger = Trigger(state.buttons & HidNpadButton_ZR);
  out.thumb_lx = ClampStick(state.analog_stick_l.x);
  out.thumb_ly = ClampStick(state.analog_stick_l.y);
  out.thumb_rx = ClampStick(state.analog_stick_r.x);
  out.thumb_ry = ClampStick(state.analog_stick_r.y);
}

X_RESULT SwitchInputDriver::GetDeviceState(DeviceId id, X_INPUT_STATE* out_state) {
  std::lock_guard lock(mutex_);
  if (!out_state) return X_ERROR_BAD_ARGUMENTS;
  auto* device = Find(id);
  if (!device || !device->has_state || !padIsNpadActive(&pad_, device->npad))
    return X_ERROR_DEVICE_NOT_CONNECTED;
  *out_state = device->state;
  return X_ERROR_SUCCESS;
}

X_RESULT SwitchInputDriver::GetDeviceCapabilities(DeviceId id, uint32_t /*flags*/,
                                                  X_INPUT_CAPABILITIES* out_caps) {
  std::lock_guard lock(mutex_);
  if (!out_caps) return X_ERROR_BAD_ARGUMENTS;
  if (!Find(id)) return X_ERROR_DEVICE_NOT_CONNECTED;
  std::memset(out_caps, 0, sizeof(*out_caps));
  out_caps->type = XINPUT_DEVTYPE_GAMEPAD;
  out_caps->sub_type = 0x01;  // XINPUT_DEVSUBTYPE_GAMEPAD.
  out_caps->flags = X_INPUT_CAPS_FFB_SUPPORTED;
  out_caps->gamepad.buttons = 0xffff;
  out_caps->gamepad.left_trigger = 0xff;
  out_caps->gamepad.right_trigger = 0xff;
  out_caps->gamepad.thumb_lx = 0x7fff;
  out_caps->gamepad.thumb_ly = 0x7fff;
  out_caps->gamepad.thumb_rx = 0x7fff;
  out_caps->gamepad.thumb_ry = 0x7fff;
  out_caps->vibration.left_motor_speed = 0xffff;
  out_caps->vibration.right_motor_speed = 0xffff;
  return X_ERROR_SUCCESS;
}

X_RESULT SwitchInputDriver::SetDeviceVibration(DeviceId id, X_INPUT_VIBRATION* vibration) {
  std::lock_guard lock(mutex_);
  auto* device = Find(id);
  if (!device || !vibration || !device->has_state) return X_ERROR_DEVICE_NOT_CONNECTED;

  if (device->vibration_count == 0) {
    const u32 style = hidGetNpadStyleSet(device->npad);
    HidNpadStyleTag selected = HidNpadStyleTag_NpadFullKey;
    if (style & HidNpadStyleTag_NpadHandheld) selected = HidNpadStyleTag_NpadHandheld;
    else if (style & HidNpadStyleTag_NpadJoyDual) selected = HidNpadStyleTag_NpadJoyDual;
    if (R_FAILED(hidInitializeVibrationDevices(device->vibration.data(), 2, device->npad,
                                                selected))) {
      device->vibration_count = -1;
      return X_ERROR_FUNCTION_FAILED;
    }
    device->vibration_count = 2;
  }
  std::array<HidVibrationValue, 2> values{};
  const float low = static_cast<uint16_t>(vibration->left_motor_speed) / 65535.0f;
  const float high = static_cast<uint16_t>(vibration->right_motor_speed) / 65535.0f;
  values[0] = {low, 160.0f, high, 320.0f};
  values[1] = values[0];
  return R_SUCCEEDED(hidSendVibrationValues(device->vibration.data(), values.data(), 2))
             ? X_ERROR_SUCCESS
             : X_ERROR_FUNCTION_FAILED;
}

X_RESULT SwitchInputDriver::GetDeviceKeystroke(DeviceId id, uint32_t /*flags*/,
                                               X_INPUT_KEYSTROKE* out_keystroke) {
  std::lock_guard lock(mutex_);
  auto* device = Find(id);
  if (!device || !device->has_state) return X_ERROR_DEVICE_NOT_CONNECTED;
  if (!out_keystroke) return X_ERROR_BAD_ARGUMENTS;
  if (!device->keystrokes.empty()) {
    *out_keystroke = device->keystrokes.front();
    device->keystrokes.pop_front();
    return X_ERROR_SUCCESS;
  }
  return X_ERROR_EMPTY;
}

uint16_t SwitchInputDriver::KeyForButton(uint64_t button) {
  for (size_t i = 0; i < std::size(kButtonMap); ++i)
    if (kButtonMap[i][0] == button) return kButtonKeys[i];
  if (button == HidNpadButton_ZL) return kButtonKeys[14];
  if (button == HidNpadButton_ZR) return kButtonKeys[15];
  return 0;
}

DeviceId SwitchInputDriver::MakeDeviceId(HidNpadIdType id) {
  return DeviceId{kDevicePrefix | static_cast<uint64_t>(id)};
}

bool SwitchInputDriver::IsHandheld(DeviceId id) {
  return (static_cast<uint64_t>(id) & kHandheldBit) != 0;
}

HidNpadIdType SwitchInputDriver::ToNpadId(DeviceId id) {
  return IsHandheld(id) ? HidNpadIdType_Handheld
                        : static_cast<HidNpadIdType>(static_cast<uint64_t>(id) & 0xff);
}

SwitchInputDriver::Device* SwitchInputDriver::Find(DeviceId id) {
  for (auto& device : devices_)
    if (device.id == id) return &device;
  return nullptr;
}

const SwitchInputDriver::Device* SwitchInputDriver::Find(DeviceId id) const {
  for (const auto& device : devices_)
    if (device.id == id) return &device;
  return nullptr;
}

std::unique_ptr<rex::input::InputSystem> CreateSwitchInputSystem(bool tool_mode) {
  auto input = std::make_unique<rex::input::InputSystem>(nullptr);
  if (tool_mode) {
    auto nop = std::make_unique<rex::input::nop::NopInputDriver>(nullptr, 0);
    if (nop->Setup() == X_STATUS_SUCCESS) input->AddDriver(std::move(nop));
  } else {
    auto driver = std::make_unique<SwitchInputDriver>(nullptr, 0);
    if (driver->Setup() == X_STATUS_SUCCESS) input->AddDriver(std::move(driver));
  }
  input->SetDeviceAssignment(std::make_unique<rex::input::SlotAssignment>());
  return input;
}

}  // namespace rex::input::switch_
