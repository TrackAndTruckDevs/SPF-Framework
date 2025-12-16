#include "SPF/UI/ImGuiInputConsumer.hpp"

#include <imgui.h>

#include "SPF/System/Keyboard.hpp"

SPF_NS_BEGIN

namespace UI {
// Helper function to convert our enum to ImGuiKey,
// ported from ImGuiEventProxy.cpp
ImGuiKey TranslateOurKeyToImGuiKey(System::Keyboard key) {
  switch (key) {
    case System::Keyboard::A:
      return ImGuiKey_A;
    case System::Keyboard::B:
      return ImGuiKey_B;
    case System::Keyboard::C:
      return ImGuiKey_C;
    case System::Keyboard::D:
      return ImGuiKey_D;
    case System::Keyboard::E:
      return ImGuiKey_E;
    case System::Keyboard::F:
      return ImGuiKey_F;
    case System::Keyboard::G:
      return ImGuiKey_G;
    case System::Keyboard::H:
      return ImGuiKey_H;
    case System::Keyboard::I:
      return ImGuiKey_I;
    case System::Keyboard::J:
      return ImGuiKey_J;
    case System::Keyboard::K:
      return ImGuiKey_K;
    case System::Keyboard::L:
      return ImGuiKey_L;
    case System::Keyboard::M:
      return ImGuiKey_M;
    case System::Keyboard::N:
      return ImGuiKey_N;
    case System::Keyboard::O:
      return ImGuiKey_O;
    case System::Keyboard::P:
      return ImGuiKey_P;
    case System::Keyboard::Q:
      return ImGuiKey_Q;
    case System::Keyboard::R:
      return ImGuiKey_R;
    case System::Keyboard::S:
      return ImGuiKey_S;
    case System::Keyboard::T:
      return ImGuiKey_T;
    case System::Keyboard::U:
      return ImGuiKey_U;
    case System::Keyboard::V:
      return ImGuiKey_V;
    case System::Keyboard::W:
      return ImGuiKey_W;
    case System::Keyboard::X:
      return ImGuiKey_X;
    case System::Keyboard::Y:
      return ImGuiKey_Y;
    case System::Keyboard::Z:
      return ImGuiKey_Z;
    case System::Keyboard::Num0:
      return ImGuiKey_0;
    case System::Keyboard::Num1:
      return ImGuiKey_1;
    case System::Keyboard::Num2:
      return ImGuiKey_2;
    case System::Keyboard::Num3:
      return ImGuiKey_3;
    case System::Keyboard::Num4:
      return ImGuiKey_4;
    case System::Keyboard::Num5:
      return ImGuiKey_5;
    case System::Keyboard::Num6:
      return ImGuiKey_6;
    case System::Keyboard::Num7:
      return ImGuiKey_7;
    case System::Keyboard::Num8:
      return ImGuiKey_8;
    case System::Keyboard::Num9:
      return ImGuiKey_9;
    case System::Keyboard::Escape:
      return ImGuiKey_Escape;
    case System::Keyboard::LControl:
      return ImGuiKey_LeftCtrl;
    case System::Keyboard::LShift:
      return ImGuiKey_LeftShift;
    case System::Keyboard::LAlt:
      return ImGuiKey_LeftAlt;
    case System::Keyboard::LSystem:
      return ImGuiKey_LeftSuper;
    case System::Keyboard::RControl:
      return ImGuiKey_RightCtrl;
    case System::Keyboard::RShift:
      return ImGuiKey_RightShift;
    case System::Keyboard::RAlt:
      return ImGuiKey_RightAlt;
    case System::Keyboard::RSystem:
      return ImGuiKey_RightSuper;
    case System::Keyboard::Menu:
      return ImGuiKey_Menu;
    case System::Keyboard::LBracket:
      return ImGuiKey_LeftBracket;
    case System::Keyboard::RBracket:
      return ImGuiKey_RightBracket;
    case System::Keyboard::Semicolon:
      return ImGuiKey_Semicolon;
    case System::Keyboard::Comma:
      return ImGuiKey_Comma;
    case System::Keyboard::Period:
      return ImGuiKey_Period;
    case System::Keyboard::Apostrophe:
      return ImGuiKey_Apostrophe;
    case System::Keyboard::Slash:
      return ImGuiKey_Slash;
    case System::Keyboard::Backslash:
      return ImGuiKey_Backslash;
    case System::Keyboard::Grave:
      return ImGuiKey_GraveAccent;
    case System::Keyboard::Equal:
      return ImGuiKey_Equal;
    case System::Keyboard::Hyphen:
      return ImGuiKey_Minus;
    case System::Keyboard::Space:
      return ImGuiKey_Space;
    case System::Keyboard::Enter:
      return ImGuiKey_Enter;
    case System::Keyboard::Backspace:
      return ImGuiKey_Backspace;
    case System::Keyboard::Tab:
      return ImGuiKey_Tab;
    case System::Keyboard::PageUp:
      return ImGuiKey_PageUp;
    case System::Keyboard::PageDown:
      return ImGuiKey_PageDown;
    case System::Keyboard::End:
      return ImGuiKey_End;
    case System::Keyboard::Home:
      return ImGuiKey_Home;
    case System::Keyboard::Insert:
      return ImGuiKey_Insert;
    case System::Keyboard::Delete:
      return ImGuiKey_Delete;
    case System::Keyboard::Add:
      return ImGuiKey_KeypadAdd;
    case System::Keyboard::Subtract:
      return ImGuiKey_KeypadSubtract;
    case System::Keyboard::Multiply:
      return ImGuiKey_KeypadMultiply;
    case System::Keyboard::Divide:
      return ImGuiKey_KeypadDivide;
    case System::Keyboard::Left:
      return ImGuiKey_LeftArrow;
    case System::Keyboard::Right:
      return ImGuiKey_RightArrow;
    case System::Keyboard::Up:
      return ImGuiKey_UpArrow;
    case System::Keyboard::Down:
      return ImGuiKey_DownArrow;
    case System::Keyboard::Numpad0:
      return ImGuiKey_Keypad0;
    case System::Keyboard::Numpad1:
      return ImGuiKey_Keypad1;
    case System::Keyboard::Numpad2:
      return ImGuiKey_Keypad2;
    case System::Keyboard::Numpad3:
      return ImGuiKey_Keypad3;
    case System::Keyboard::Numpad4:
      return ImGuiKey_Keypad4;
    case System::Keyboard::Numpad5:
      return ImGuiKey_Keypad5;
    case System::Keyboard::Numpad6:
      return ImGuiKey_Keypad6;
    case System::Keyboard::Numpad7:
      return ImGuiKey_Keypad7;
    case System::Keyboard::Numpad8:
      return ImGuiKey_Keypad8;
    case System::Keyboard::Numpad9:
      return ImGuiKey_Keypad9;
    case System::Keyboard::F1:
      return ImGuiKey_F1;
    case System::Keyboard::F2:
      return ImGuiKey_F2;
    case System::Keyboard::F3:
      return ImGuiKey_F3;
    case System::Keyboard::F4:
      return ImGuiKey_F4;
    case System::Keyboard::F5:
      return ImGuiKey_F5;
    case System::Keyboard::F6:
      return ImGuiKey_F6;
    case System::Keyboard::F7:
      return ImGuiKey_F7;
    case System::Keyboard::F8:
      return ImGuiKey_F8;
    case System::Keyboard::F9:
      return ImGuiKey_F9;
    case System::Keyboard::F10:
      return ImGuiKey_F10;
    case System::Keyboard::F11:
      return ImGuiKey_F11;
    case System::Keyboard::F12:
      return ImGuiKey_F12;
    case System::Keyboard::F13:
      return ImGuiKey_F13;
    case System::Keyboard::F14:
      return ImGuiKey_F14;
    case System::Keyboard::F15:
      return ImGuiKey_F15;
    case System::Keyboard::Pause:
      return ImGuiKey_Pause;
    default:
      return ImGuiKey_None;
  }
}

bool ImGuiInputConsumer::OnKeyPress(const Input::KeyboardEvent& event) {
  ImGuiIO& io = ImGui::GetIO();
  ImGuiKey im_key = TranslateOurKeyToImGuiKey(event.key);
  if (im_key != ImGuiKey_None) {
    io.AddKeyEvent(im_key, true);
  }
  return io.WantCaptureKeyboard;
}

bool ImGuiInputConsumer::OnKeyRelease(const Input::KeyboardEvent& event) {
  ImGuiIO& io = ImGui::GetIO();
  ImGuiKey im_key = TranslateOurKeyToImGuiKey(event.key);
  if (im_key != ImGuiKey_None) {
    io.AddKeyEvent(im_key, false);
  }
  return io.WantCaptureKeyboard;
}

bool ImGuiInputConsumer::OnMouseButton(const Input::MouseButtonEvent& event) {
  ImGuiIO& io = ImGui::GetIO();
  if (event.iButton >= 0 && event.iButton < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(event.iButton, event.bPressed);
  }
  return io.WantCaptureMouse;
}

bool ImGuiInputConsumer::OnMouseMove(const Input::MouseMoveEvent& event) {
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent(io.MousePos.x + event.lLastX, io.MousePos.y + event.lLastY);
  return io.WantCaptureMouse;
}

bool ImGuiInputConsumer::OnMouseWheel(const Input::MouseWheelEvent& event) {
  ImGuiIO& io = ImGui::GetIO();
  io.AddMouseWheelEvent(0.0f, event.delta);
  return io.WantCaptureMouse;
}
}  // namespace UI

SPF_NS_END
