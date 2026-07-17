#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/System/GamepadButton.hpp"

#include <string>
#include <unordered_map>


SPF_NS_BEGIN

namespace System {
// Enum to identify the type of controller for providing correct button glyphs/names.
enum class DeviceType {
  Unknown,
  Keyboard,     // For completeness
  Mouse,        // For completeness
  Xbox,         // For XInput-compatible controllers
  PlayStation,  // For DualShock/DualSense controllers
  Joystick      // For other DirectInput or custom devices
};

class GamepadButtonMapping {
 public:
  static GamepadButtonMapping& GetInstance();

  // Gets the framework's enum from a string representation (e.g., "FACE_DOWN")
  GamepadButton GetButton(const std::string& buttonName) const;

  // Gets the canonical string name from a framework's enum (e.g., GamepadButton::FaceDown -> "FACE_DOWN")
  std::string GetButtonName(GamepadButton button) const;

  // Maps XInput button flags (e.g., XINPUT_GAMEPAD_A) to framework's GamepadButton
  GamepadButton FromXInput(unsigned short xinputFlags) const;

  // Maps DirectInput button offsets (e.g., DIJOFS_BUTTON0) to framework's GamepadButton
  GamepadButton FromDInput(unsigned long dinputOffset) const;

  // Gets the user-friendly display name based on the controller type
  // (e.g., GamepadButton::FaceDown -> "A" for Xbox, "Cross" for PlayStation)
  std::string GetButtonDisplayName(GamepadButton button, DeviceType deviceType) const;

 private:
  // Singleton pattern
  GamepadButtonMapping();
  ~GamepadButtonMapping() = default;
  GamepadButtonMapping(const GamepadButtonMapping&) = delete;
  GamepadButtonMapping& operator=(const GamepadButtonMapping&) = delete;
  GamepadButtonMapping(GamepadButtonMapping&&) = delete;
  GamepadButtonMapping& operator=(GamepadButtonMapping&&) = delete;

  void InitializeMapping();

  std::unordered_map<std::string, GamepadButton> m_stringToButton;
  std::unordered_map<GamepadButton, std::string> m_buttonToString;

  // Maps for display names
  std::unordered_map<GamepadButton, std::string> m_xboxNames;
  std::unordered_map<GamepadButton, std::string> m_playstationNames;
};
}  // namespace System

SPF_NS_END
