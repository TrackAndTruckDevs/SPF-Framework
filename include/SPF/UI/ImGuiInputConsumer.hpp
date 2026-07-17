#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Input/InputEvents.hpp"


SPF_NS_BEGIN

namespace UI {
class ImGuiInputConsumer : public Input::IInputConsumer {
 public:
  // Keyboard events
  bool OnKeyPress(const Input::KeyboardEvent& event) override;
  bool OnKeyRelease(const Input::KeyboardEvent& event) override;

  // Mouse events
  bool OnMouseButton(const Input::MouseButtonEvent& event) override;
  bool OnMouseMove(const Input::MouseMoveEvent& event) override;
  bool OnMouseWheel(const Input::MouseWheelEvent& event) override;

  bool IsCapturingKeyboard() override;
  bool IsCapturingMouse() override;
};
}  // namespace UI

SPF_NS_END
