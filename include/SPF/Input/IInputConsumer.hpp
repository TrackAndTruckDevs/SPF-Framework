#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Input/InputEvents.hpp"

SPF_NS_BEGIN
namespace Input {
class IInputConsumer {
 public:
  virtual ~IInputConsumer() = default;

  virtual bool OnMouseMove(const MouseMoveEvent& event) { return false; }
  virtual bool OnMouseButton(const MouseButtonEvent& event) { return false; }
  virtual bool OnMouseWheel(const MouseWheelEvent& event) { return false; }
  virtual bool OnKeyPress(const KeyboardEvent& event) { return false; }
  virtual bool OnKeyRelease(const KeyboardEvent& event) { return false; }

  // Gamepad Events
  virtual bool OnGamepadButtonPress(const GamepadEvent& event) { return false; }
  virtual bool OnGamepadButtonRelease(const GamepadEvent& event) { return false; }
  virtual bool OnGamepadAxisMove(const GamepadEvent& event) { return false; }

  // Joystick Events
  virtual bool OnJoystickButtonPress(const JoystickEvent& event) { return false; }
  virtual bool OnJoystickButtonRelease(const JoystickEvent& event) { return false; }
};

}  // namespace Input
SPF_NS_END
