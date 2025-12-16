#include "SPF/System/GamepadButtonMapping.hpp"

#include <stdexcept>

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
}
}  // namespace System

SPF_NS_END
