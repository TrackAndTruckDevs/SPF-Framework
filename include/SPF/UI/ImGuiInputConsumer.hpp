#pragma once

#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Namespace.hpp"

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
};
}  // namespace UI

SPF_NS_END
