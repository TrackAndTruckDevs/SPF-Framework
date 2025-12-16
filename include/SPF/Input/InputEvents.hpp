#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/System/GamepadButton.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

SPF_NS_BEGIN

namespace Modules {
class IBindableInput;
}
namespace Input {
enum class PressType { Short, Long };

struct MouseMoveEvent {
  long lLastX = 0;
  long lLastY = 0;
};

struct MouseButtonEvent {
  int iButton = 0;  // e.g., 0 for left, 1 for right
  bool bPressed = false;
  PressType pressType = PressType::Short;  // To distinguish short/long presses
};

struct MouseWheelEvent {
  float delta = 0.0f;  // Positive for wheel up, negative for wheel down
};

struct KeyboardEvent {
  System::Keyboard key;
  bool pressed;
  PressType pressType = PressType::Short;  // To distinguish short/long presses
};

struct GamepadEvent {
  int deviceID = 0;  // Index of the gamepad
  System::GamepadButton button;
  bool pressed = false;                    // For digital button presses
  float value = 0.0f;                      // For analog inputs like triggers and sticks
  PressType pressType = PressType::Short;  // To distinguish short/long presses
};

struct JoystickEvent {
    int buttonIndex = -1;
    bool pressed = false;
    PressType pressType = PressType::Short;
};

/**
 * @brief Fired by InputManager after a key or button has been successfully captured.
 */
struct InputCaptured {
  std::shared_ptr<Modules::IBindableInput> capturedInput;
  std::string actionFullName;
  nlohmann::json originalBinding;
};

/**
 * @brief Fired by InputManager after a gamepad button has been successfully captured.
 */
struct GamepadButtonCaptured {
  System::GamepadButton capturedButton;
  std::string actionFullName;
  nlohmann::json originalBinding;
};

/**
 * @brief Fired by InputManager if an input capture session is cancelled.
 */
struct InputCaptureCancelled {
  std::string actionFullName;
};

/**
 * @brief Fired by Core when a captured key is already bound to another action.
 */
struct InputCaptureConflict {
  std::string actionFullName;                              // The action we are trying to rebind
  std::shared_ptr<Modules::IBindableInput> capturedInput;  // The input that was activated
  std::string conflictingAction;                           // The action that already uses this input
  nlohmann::json originalBinding;                          // The original binding object we are editing
};

/**
 * @brief Fired by SCSInputService when the game activates or deactivates
 * a virtual input device.
 */
struct InputDeviceActivityChanged {
  std::string deviceName;
  bool isActive;
};

}  // namespace Input
SPF_NS_END
