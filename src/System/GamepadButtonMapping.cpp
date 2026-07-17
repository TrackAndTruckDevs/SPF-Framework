#include "SPF/System/GamepadButtonMapping.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/System/GamepadButton.hpp"

#include <string>


SPF_NS_BEGIN

namespace System {
GamepadButtonMapping& GamepadButtonMapping::GetInstance() {
  static GamepadButtonMapping instance;
  return instance;
}

GamepadButtonMapping::GamepadButtonMapping() { InitializeMapping(); }

GamepadButton GamepadButtonMapping::GetButton(const std::string& buttonName) const {
  auto it = m_stringToButton.find(buttonName);
  if (it != m_stringToButton.end()) {
    return it->second;
  }
  return GamepadButton::Unknown;
}

std::string GamepadButtonMapping::GetButtonName(GamepadButton button) const {
  auto it = m_buttonToString.find(button);
  if (it != m_buttonToString.end()) {
    return it->second;
  }
  return "UNKNOWN_BUTTON";
}

GamepadButton GamepadButtonMapping::FromXInput(unsigned short xinputFlags) const {
  // XInput button constants from XInput.h
  if (xinputFlags & 0x1000) return GamepadButton::FaceDown;       // XINPUT_GAMEPAD_A
  if (xinputFlags & 0x2000) return GamepadButton::FaceRight;      // XINPUT_GAMEPAD_B
  if (xinputFlags & 0x4000) return GamepadButton::FaceLeft;       // XINPUT_GAMEPAD_X
  if (xinputFlags & 0x8000) return GamepadButton::FaceUp;         // XINPUT_GAMEPAD_Y
  if (xinputFlags & 0x0001) return GamepadButton::DPadUp;         // XINPUT_GAMEPAD_DPAD_UP
  if (xinputFlags & 0x0002) return GamepadButton::DPadDown;       // XINPUT_GAMEPAD_DPAD_DOWN
  if (xinputFlags & 0x0004) return GamepadButton::DPadLeft;       // XINPUT_GAMEPAD_DPAD_LEFT
  if (xinputFlags & 0x0008) return GamepadButton::DPadRight;      // XINPUT_GAMEPAD_DPAD_RIGHT
  if (xinputFlags & 0x0010) return GamepadButton::SpecialRight;   // XINPUT_GAMEPAD_START
  if (xinputFlags & 0x0020) return GamepadButton::SpecialLeft;    // XINPUT_GAMEPAD_BACK
  if (xinputFlags & 0x0040) return GamepadButton::LeftStick;      // XINPUT_GAMEPAD_LEFT_THUMB
  if (xinputFlags & 0x0080) return GamepadButton::RightStick;     // XINPUT_GAMEPAD_RIGHT_THUMB
  if (xinputFlags & 0x0100) return GamepadButton::LeftShoulder;   // XINPUT_GAMEPAD_LEFT_SHOULDER
  if (xinputFlags & 0x0200) return GamepadButton::RightShoulder;  // XINPUT_GAMEPAD_RIGHT_SHOULDER

  return GamepadButton::Unknown;
}

GamepadButton GamepadButtonMapping::FromDInput(unsigned long dinputOffset) const {
  // DirectInput offsets from dinput.h (DIJOFS_BUTTON0 = 48)
  switch (dinputOffset) {
    case 48:
      return GamepadButton::FaceDown;  // Button 0
    case 49:
      return GamepadButton::FaceRight;  // Button 1
    case 50:
      return GamepadButton::FaceLeft;  // Button 2
    case 51:
      return GamepadButton::FaceUp;  // Button 3
    case 52:
      return GamepadButton::LeftShoulder;  // Button 4
    case 53:
      return GamepadButton::RightShoulder;  // Button 5
    case 54:
      return GamepadButton::SpecialLeft;  // Button 6
    case 55:
      return GamepadButton::SpecialRight;  // Button 7
    case 56:
      return GamepadButton::LeftStick;  // Button 8
    case 57:
      return GamepadButton::RightStick;  // Button 9
    default:
      return GamepadButton::Unknown;
  }
}

std::string GamepadButtonMapping::GetButtonDisplayName(GamepadButton button, DeviceType deviceType) const {
  const auto* mapToUse = &m_xboxNames;  // Default to Xbox names
  if (deviceType == DeviceType::PlayStation) {
    mapToUse = &m_playstationNames;
  }

  auto it = mapToUse->find(button);
  if (it != mapToUse->end()) {
    return it->second;
  }

  // Fallback to canonical name if no specific display name is found
  return GetButtonName(button);
}

void GamepadButtonMapping::InitializeMapping() {
  // Helper lambda to add a mapping in both directions
  auto addMapping = [this](GamepadButton button, const std::string& name) {
    m_buttonToString[button] = name;
    m_stringToButton[name] = button;
  };

  addMapping(GamepadButton::Unknown, "UNKNOWN_BUTTON");
  addMapping(GamepadButton::FaceDown, "FACE_DOWN");
  addMapping(GamepadButton::FaceRight, "FACE_RIGHT");
  addMapping(GamepadButton::FaceLeft, "FACE_LEFT");
  addMapping(GamepadButton::FaceUp, "FACE_UP");
  addMapping(GamepadButton::DPadUp, "DPAD_UP");
  addMapping(GamepadButton::DPadDown, "DPAD_DOWN");
  addMapping(GamepadButton::DPadLeft, "DPAD_LEFT");
  addMapping(GamepadButton::DPadRight, "DPAD_RIGHT");
  addMapping(GamepadButton::LeftShoulder, "LEFT_SHOULDER");
  addMapping(GamepadButton::RightShoulder, "RIGHT_SHOULDER");
  addMapping(GamepadButton::LeftTrigger, "LEFT_TRIGGER");
  addMapping(GamepadButton::RightTrigger, "RIGHT_TRIGGER");
  addMapping(GamepadButton::SpecialLeft, "SPECIAL_LEFT");
  addMapping(GamepadButton::SpecialRight, "SPECIAL_RIGHT");
  addMapping(GamepadButton::LeftStick, "LEFT_STICK");
  addMapping(GamepadButton::RightStick, "RIGHT_STICK");
  addMapping(GamepadButton::LeftStickX, "LEFT_STICK_X");
  addMapping(GamepadButton::LeftStickY, "LEFT_STICK_Y");
  addMapping(GamepadButton::RightStickX, "RIGHT_STICK_X");
  addMapping(GamepadButton::RightStickY, "RIGHT_STICK_Y");
  addMapping(GamepadButton::LeftTriggerAxis, "LEFT_TRIGGER_AXIS");
  addMapping(GamepadButton::RightTriggerAxis, "RIGHT_TRIGGER_AXIS");

  // Additional Axes
  addMapping(GamepadButton::AxisRx, "AXIS_RX");
  addMapping(GamepadButton::AxisRy, "AXIS_RY");
  addMapping(GamepadButton::AxisRz, "AXIS_RZ");
  addMapping(GamepadButton::Slider0, "SLIDER_0");
  addMapping(GamepadButton::Slider1, "SLIDER_1");

  // Additional POVs
  addMapping(GamepadButton::POV1Up, "POV1_UP");
  addMapping(GamepadButton::POV1Down, "POV1_DOWN");
  addMapping(GamepadButton::POV1Left, "POV1_LEFT");
  addMapping(GamepadButton::POV1Right, "POV1_RIGHT");

  addMapping(GamepadButton::POV2Up, "POV2_UP");
  addMapping(GamepadButton::POV2Down, "POV2_DOWN");
  addMapping(GamepadButton::POV2Left, "POV2_LEFT");
  addMapping(GamepadButton::POV2Right, "POV2_RIGHT");

  addMapping(GamepadButton::POV3Up, "POV3_UP");
  addMapping(GamepadButton::POV3Down, "POV3_DOWN");
  addMapping(GamepadButton::POV3Left, "POV3_LEFT");
  addMapping(GamepadButton::POV3Right, "POV3_RIGHT");

  // Initialize Xbox Display Names
  m_xboxNames[GamepadButton::FaceDown] = "A";
  m_xboxNames[GamepadButton::FaceRight] = "B";
  m_xboxNames[GamepadButton::FaceLeft] = "X";
  m_xboxNames[GamepadButton::FaceUp] = "Y";
  m_xboxNames[GamepadButton::DPadUp] = "D-Pad Up";
  m_xboxNames[GamepadButton::DPadDown] = "D-Pad Down";
  m_xboxNames[GamepadButton::DPadLeft] = "D-Pad Left";
  m_xboxNames[GamepadButton::DPadRight] = "D-Pad Right";
  m_xboxNames[GamepadButton::LeftShoulder] = "LB";
  m_xboxNames[GamepadButton::RightShoulder] = "RB";
  m_xboxNames[GamepadButton::LeftTrigger] = "LT";
  m_xboxNames[GamepadButton::RightTrigger] = "RT";
  m_xboxNames[GamepadButton::SpecialLeft] = "View";
  m_xboxNames[GamepadButton::SpecialRight] = "Menu";
  m_xboxNames[GamepadButton::LeftStick] = "LS";
  m_xboxNames[GamepadButton::RightStick] = "RS";
  m_xboxNames[GamepadButton::LeftStickX] = "LS X";
  m_xboxNames[GamepadButton::LeftStickY] = "LS Y";
  m_xboxNames[GamepadButton::RightStickX] = "RS X";
  m_xboxNames[GamepadButton::RightStickY] = "RS Y";
  m_xboxNames[GamepadButton::LeftTriggerAxis] = "LT Axis";
  m_xboxNames[GamepadButton::RightTriggerAxis] = "RT Axis";

  m_xboxNames[GamepadButton::AxisRx] = "Axis Rx";
  m_xboxNames[GamepadButton::AxisRy] = "Axis Ry";
  m_xboxNames[GamepadButton::AxisRz] = "Axis Rz";
  m_xboxNames[GamepadButton::Slider0] = "Slider 0";
  m_xboxNames[GamepadButton::Slider1] = "Slider 1";

  m_xboxNames[GamepadButton::POV1Up] = "POV 1 Up";
  m_xboxNames[GamepadButton::POV1Down] = "POV 1 Down";
  m_xboxNames[GamepadButton::POV1Left] = "POV 1 Left";
  m_xboxNames[GamepadButton::POV1Right] = "POV 1 Right";

  m_xboxNames[GamepadButton::POV2Up] = "POV 2 Up";
  m_xboxNames[GamepadButton::POV2Down] = "POV 2 Down";
  m_xboxNames[GamepadButton::POV2Left] = "POV 2 Left";
  m_xboxNames[GamepadButton::POV2Right] = "POV 2 Right";

  m_xboxNames[GamepadButton::POV3Up] = "POV 3 Up";
  m_xboxNames[GamepadButton::POV3Down] = "POV 3 Down";
  m_xboxNames[GamepadButton::POV3Left] = "POV 3 Left";
  m_xboxNames[GamepadButton::POV3Right] = "POV 3 Right";

  // Initialize PlayStation Display Names
  m_playstationNames[GamepadButton::FaceDown] = "Cross";
  m_playstationNames[GamepadButton::FaceRight] = "Circle";
  m_playstationNames[GamepadButton::FaceLeft] = "Square";
  m_playstationNames[GamepadButton::FaceUp] = "Triangle";
  m_playstationNames[GamepadButton::DPadUp] = "D-Pad Up";
  m_playstationNames[GamepadButton::DPadDown] = "D-Pad Down";
  m_playstationNames[GamepadButton::DPadLeft] = "D-Pad Left";
  m_playstationNames[GamepadButton::DPadRight] = "D-Pad Right";
  m_playstationNames[GamepadButton::LeftShoulder] = "L1";
  m_playstationNames[GamepadButton::RightShoulder] = "R1";
  m_playstationNames[GamepadButton::LeftTrigger] = "L2";
  m_playstationNames[GamepadButton::RightTrigger] = "R2";
  m_playstationNames[GamepadButton::SpecialLeft] = "Share";
  m_playstationNames[GamepadButton::SpecialRight] = "Options";
  m_playstationNames[GamepadButton::LeftStick] = "L3";
  m_playstationNames[GamepadButton::RightStick] = "R3";
  m_playstationNames[GamepadButton::LeftStickX] = "L3 X";
  m_playstationNames[GamepadButton::LeftStickY] = "L3 Y";
  m_playstationNames[GamepadButton::RightStickX] = "R3 X";
  m_playstationNames[GamepadButton::RightStickY] = "R3 Y";
  m_playstationNames[GamepadButton::LeftTriggerAxis] = "L2 Axis";
  m_playstationNames[GamepadButton::RightTriggerAxis] = "R2 Axis";

  m_playstationNames[GamepadButton::AxisRx] = "Axis Rx";
  m_playstationNames[GamepadButton::AxisRy] = "Axis Ry";
  m_playstationNames[GamepadButton::AxisRz] = "Axis Rz";
  m_playstationNames[GamepadButton::Slider0] = "Slider 0";
  m_playstationNames[GamepadButton::Slider1] = "Slider 1";

  m_playstationNames[GamepadButton::POV1Up] = "POV 1 Up";
  m_playstationNames[GamepadButton::POV1Down] = "POV 1 Down";
  m_playstationNames[GamepadButton::POV1Left] = "POV 1 Left";
  m_playstationNames[GamepadButton::POV1Right] = "POV 1 Right";

  m_playstationNames[GamepadButton::POV2Up] = "POV 2 Up";
  m_playstationNames[GamepadButton::POV2Down] = "POV 2 Down";
  m_playstationNames[GamepadButton::POV2Left] = "POV 2 Left";
  m_playstationNames[GamepadButton::POV2Right] = "POV 2 Right";

  m_playstationNames[GamepadButton::POV3Up] = "POV 3 Up";
  m_playstationNames[GamepadButton::POV3Down] = "POV 3 Down";
  m_playstationNames[GamepadButton::POV3Left] = "POV 3 Left";
  m_playstationNames[GamepadButton::POV3Right] = "POV 3 Right";
}
}  // namespace System

SPF_NS_END
