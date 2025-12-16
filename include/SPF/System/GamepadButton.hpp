#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace System {
// Represents abstract gamepad buttons and axes, independent of the underlying API (XInput, DirectInput).
enum class GamepadButton {
  Unknown,

  // Face Buttons (based on Xbox layout)
  FaceDown,   // A on Xbox, Cross on PlayStation
  FaceRight,  // B on Xbox, Circle on PlayStation
  FaceLeft,   // X on Xbox, Square on PlayStation
  FaceUp,     // Y on Xbox, Triangle on PlayStation

  // D-Pad
  DPadUp,
  DPadDown,
  DPadLeft,
  DPadRight,

  // Shoulder Buttons
  LeftShoulder,   // LB
  RightShoulder,  // RB

  // Triggers (as buttons)
  LeftTrigger,   // LT
  RightTrigger,  // RT

  // Center Buttons
  SpecialLeft,   // Back/View on Xbox, Create/Share on PS
  SpecialRight,  // Start/Menu on Xbox, Options on PS

  // Stick Buttons
  LeftStick,
  RightStick,

  // Axes (will be handled specially, but good to have placeholders)
  LeftStickX,
  LeftStickY,
  RightStickX,
  RightStickY
};
}  // namespace System

SPF_NS_END
