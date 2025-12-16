#include "SPF/System/VirtualKeyMapping.hpp"

#include <stdexcept>

SPF_NS_BEGIN

namespace System {
VirtualKeyMapping& VirtualKeyMapping::GetInstance() {
  static VirtualKeyMapping instance;
  return instance;
}

Keyboard VirtualKeyMapping::GetKey(const std::string& keyName) const {
  auto it = m_stringToKey.find(keyName);
  if (it != m_stringToKey.end()) {
    return it->second;
  }
  return Keyboard::Unknown;
}

std::string VirtualKeyMapping::GetKeyName(Keyboard key) const {
  auto it = m_keyToString.find(key);
  if (it != m_keyToString.end()) {
    return it->second;
  }
  return "KEY_UNKNOWN";
}

std::string VirtualKeyMapping::GetKeyDisplayName(Keyboard key) const {
  auto it = m_keyToDisplayName.find(key);
  if (it != m_keyToDisplayName.end()) {
    return it->second;
  }
  // Fallback to the internal name if a display name is not found
  return GetKeyName(key);
}

VirtualKeyMapping::VirtualKeyMapping() { InitializeMapping(); }

void VirtualKeyMapping::InitializeMapping() {
  // Helper lambda to add a mapping in both directions
  auto addMapping = [this](Keyboard key, const std::string& name) {
    m_keyToString[key] = name;
    m_stringToKey[name] = key;
  };

  addMapping(Keyboard::Unknown, "KEY_UNKNOWN");
  addMapping(Keyboard::A, "KEY_A");
  addMapping(Keyboard::B, "KEY_B");
  addMapping(Keyboard::C, "KEY_C");
  addMapping(Keyboard::D, "KEY_D");
  addMapping(Keyboard::E, "KEY_E");
  addMapping(Keyboard::F, "KEY_F");
  addMapping(Keyboard::G, "KEY_G");
  addMapping(Keyboard::H, "KEY_H");
  addMapping(Keyboard::I, "KEY_I");
  addMapping(Keyboard::J, "KEY_J");
  addMapping(Keyboard::K, "KEY_K");
  addMapping(Keyboard::L, "KEY_L");
  addMapping(Keyboard::M, "KEY_M");
  addMapping(Keyboard::N, "KEY_N");
  addMapping(Keyboard::O, "KEY_O");
  addMapping(Keyboard::P, "KEY_P");
  addMapping(Keyboard::Q, "KEY_Q");
  addMapping(Keyboard::R, "KEY_R");
  addMapping(Keyboard::S, "KEY_S");
  addMapping(Keyboard::T, "KEY_T");
  addMapping(Keyboard::U, "KEY_U");
  addMapping(Keyboard::V, "KEY_V");
  addMapping(Keyboard::W, "KEY_W");
  addMapping(Keyboard::X, "KEY_X");
  addMapping(Keyboard::Y, "KEY_Y");
  addMapping(Keyboard::Z, "KEY_Z");
  addMapping(Keyboard::Num0, "KEY_0");
  addMapping(Keyboard::Num1, "KEY_1");
  addMapping(Keyboard::Num2, "KEY_2");
  addMapping(Keyboard::Num3, "KEY_3");
  addMapping(Keyboard::Num4, "KEY_4");
  addMapping(Keyboard::Num5, "KEY_5");
  addMapping(Keyboard::Num6, "KEY_6");
  addMapping(Keyboard::Num7, "KEY_7");
  addMapping(Keyboard::Num8, "KEY_8");
  addMapping(Keyboard::Num9, "KEY_9");
  addMapping(Keyboard::Escape, "KEY_ESCAPE");
  addMapping(Keyboard::LControl, "KEY_LCONTROL");
  addMapping(Keyboard::LShift, "KEY_LSHIFT");
  addMapping(Keyboard::LAlt, "KEY_LALT");
  addMapping(Keyboard::LSystem, "KEY_LSYSTEM");
  addMapping(Keyboard::RControl, "KEY_RCONTROL");
  addMapping(Keyboard::RShift, "KEY_RSHIFT");
  addMapping(Keyboard::RAlt, "KEY_RALT");
  addMapping(Keyboard::RSystem, "KEY_RSYSTEM");
  addMapping(Keyboard::Menu, "KEY_MENU");
  addMapping(Keyboard::LBracket, "KEY_LBRACKET");
  addMapping(Keyboard::RBracket, "KEY_RBRACKET");
  addMapping(Keyboard::Semicolon, "KEY_SEMICOLON");
  addMapping(Keyboard::Comma, "KEY_COMMA");
  addMapping(Keyboard::Period, "KEY_PERIOD");
  addMapping(Keyboard::Apostrophe, "KEY_APOSTROPHE");
  addMapping(Keyboard::Slash, "KEY_SLASH");
  addMapping(Keyboard::Backslash, "KEY_BACKSLASH");
  addMapping(Keyboard::Grave, "KEY_GRAVE");
  addMapping(Keyboard::Equal, "KEY_EQUAL");
  addMapping(Keyboard::Hyphen, "KEY_HYPHEN");
  addMapping(Keyboard::Space, "KEY_SPACE");
  addMapping(Keyboard::Enter, "KEY_ENTER");
  addMapping(Keyboard::Backspace, "KEY_BACKSPACE");
  addMapping(Keyboard::Tab, "KEY_TAB");
  addMapping(Keyboard::PageUp, "KEY_PAGEUP");
  addMapping(Keyboard::PageDown, "KEY_PAGEDOWN");
  addMapping(Keyboard::End, "KEY_END");
  addMapping(Keyboard::Home, "KEY_HOME");
  addMapping(Keyboard::Insert, "KEY_INSERT");
  addMapping(Keyboard::Delete, "KEY_DELETE");
  addMapping(Keyboard::Add, "KEY_ADD");
  addMapping(Keyboard::Subtract, "KEY_SUBTRACT");
  addMapping(Keyboard::Multiply, "KEY_MULTIPLY");
  addMapping(Keyboard::Divide, "KEY_DIVIDE");
  addMapping(Keyboard::Left, "KEY_LEFT");
  addMapping(Keyboard::Right, "KEY_RIGHT");
  addMapping(Keyboard::Up, "KEY_UP");
  addMapping(Keyboard::Down, "KEY_DOWN");
  addMapping(Keyboard::Numpad0, "KEY_NUMPAD0");
  addMapping(Keyboard::Numpad1, "KEY_NUMPAD1");
  addMapping(Keyboard::Numpad2, "KEY_NUMPAD2");
  addMapping(Keyboard::Numpad3, "KEY_NUMPAD3");
  addMapping(Keyboard::Numpad4, "KEY_NUMPAD4");
  addMapping(Keyboard::Numpad5, "KEY_NUMPAD5");
  addMapping(Keyboard::Numpad6, "KEY_NUMPAD6");
  addMapping(Keyboard::Numpad7, "KEY_NUMPAD7");
  addMapping(Keyboard::Numpad8, "KEY_NUMPAD8");
  addMapping(Keyboard::Numpad9, "KEY_NUMPAD9");
  addMapping(Keyboard::F1, "KEY_F1");
  addMapping(Keyboard::F2, "KEY_F2");
  addMapping(Keyboard::F3, "KEY_F3");
  addMapping(Keyboard::F4, "KEY_F4");
  addMapping(Keyboard::F5, "KEY_F5");
  addMapping(Keyboard::F6, "KEY_F6");
  addMapping(Keyboard::F7, "KEY_F7");
  addMapping(Keyboard::F8, "KEY_F8");
  addMapping(Keyboard::F9, "KEY_F9");
  addMapping(Keyboard::F10, "KEY_F10");
  addMapping(Keyboard::F11, "KEY_F11");
  addMapping(Keyboard::F12, "KEY_F12");
  addMapping(Keyboard::F13, "KEY_F13");
  addMapping(Keyboard::F14, "KEY_F14");
  addMapping(Keyboard::F15, "KEY_F15");
  addMapping(Keyboard::Pause, "KEY_PAUSE");

  // Initialize Display Names
  m_keyToDisplayName[Keyboard::Unknown] = "Unknown";
  m_keyToDisplayName[Keyboard::A] = "A";
  m_keyToDisplayName[Keyboard::B] = "B";
  m_keyToDisplayName[Keyboard::C] = "C";
  m_keyToDisplayName[Keyboard::D] = "D";
  m_keyToDisplayName[Keyboard::E] = "E";
  m_keyToDisplayName[Keyboard::F] = "F";
  m_keyToDisplayName[Keyboard::G] = "G";
  m_keyToDisplayName[Keyboard::H] = "H";
  m_keyToDisplayName[Keyboard::I] = "I";
  m_keyToDisplayName[Keyboard::J] = "J";
  m_keyToDisplayName[Keyboard::K] = "K";
  m_keyToDisplayName[Keyboard::L] = "L";
  m_keyToDisplayName[Keyboard::M] = "M";
  m_keyToDisplayName[Keyboard::N] = "N";
  m_keyToDisplayName[Keyboard::O] = "O";
  m_keyToDisplayName[Keyboard::P] = "P";
  m_keyToDisplayName[Keyboard::Q] = "Q";
  m_keyToDisplayName[Keyboard::R] = "R";
  m_keyToDisplayName[Keyboard::S] = "S";
  m_keyToDisplayName[Keyboard::T] = "T";
  m_keyToDisplayName[Keyboard::U] = "U";
  m_keyToDisplayName[Keyboard::V] = "V";
  m_keyToDisplayName[Keyboard::W] = "W";
  m_keyToDisplayName[Keyboard::X] = "X";
  m_keyToDisplayName[Keyboard::Y] = "Y";
  m_keyToDisplayName[Keyboard::Z] = "Z";
  m_keyToDisplayName[Keyboard::Num0] = "0";
  m_keyToDisplayName[Keyboard::Num1] = "1";
  m_keyToDisplayName[Keyboard::Num2] = "2";
  m_keyToDisplayName[Keyboard::Num3] = "3";
  m_keyToDisplayName[Keyboard::Num4] = "4";
  m_keyToDisplayName[Keyboard::Num5] = "5";
  m_keyToDisplayName[Keyboard::Num6] = "6";
  m_keyToDisplayName[Keyboard::Num7] = "7";
  m_keyToDisplayName[Keyboard::Num8] = "8";
  m_keyToDisplayName[Keyboard::Num9] = "9";
  m_keyToDisplayName[Keyboard::Escape] = "Escape";
  m_keyToDisplayName[Keyboard::LControl] = "Left Control";
  m_keyToDisplayName[Keyboard::LShift] = "Left Shift";
  m_keyToDisplayName[Keyboard::LAlt] = "Left Alt";
  m_keyToDisplayName[Keyboard::LSystem] = "Left Super";
  m_keyToDisplayName[Keyboard::RControl] = "Right Control";
  m_keyToDisplayName[Keyboard::RShift] = "Right Shift";
  m_keyToDisplayName[Keyboard::RAlt] = "Right Alt";
  m_keyToDisplayName[Keyboard::RSystem] = "Right Super";
  m_keyToDisplayName[Keyboard::Menu] = "Menu";
  m_keyToDisplayName[Keyboard::LBracket] = "[";
  m_keyToDisplayName[Keyboard::RBracket] = "]";
  m_keyToDisplayName[Keyboard::Semicolon] = ";";
  m_keyToDisplayName[Keyboard::Comma] = ",";
  m_keyToDisplayName[Keyboard::Period] = ".";
  m_keyToDisplayName[Keyboard::Apostrophe] = "'";
  m_keyToDisplayName[Keyboard::Slash] = "/";
  m_keyToDisplayName[Keyboard::Backslash] = "\\";
  m_keyToDisplayName[Keyboard::Grave] = "`";
  m_keyToDisplayName[Keyboard::Equal] = "=";
  m_keyToDisplayName[Keyboard::Hyphen] = "-";
  m_keyToDisplayName[Keyboard::Space] = "Space";
  m_keyToDisplayName[Keyboard::Enter] = "Enter";
  m_keyToDisplayName[Keyboard::Backspace] = "Backspace";
  m_keyToDisplayName[Keyboard::Tab] = "Tab";
  m_keyToDisplayName[Keyboard::PageUp] = "Page Up";
  m_keyToDisplayName[Keyboard::PageDown] = "Page Down";
  m_keyToDisplayName[Keyboard::End] = "End";
  m_keyToDisplayName[Keyboard::Home] = "Home";
  m_keyToDisplayName[Keyboard::Insert] = "Insert";
  m_keyToDisplayName[Keyboard::Delete] = "Delete";
  m_keyToDisplayName[Keyboard::Add] = "Numpad +";
  m_keyToDisplayName[Keyboard::Subtract] = "Numpad -";
  m_keyToDisplayName[Keyboard::Multiply] = "Numpad *";
  m_keyToDisplayName[Keyboard::Divide] = "Numpad /";
  m_keyToDisplayName[Keyboard::Left] = "Left Arrow";
  m_keyToDisplayName[Keyboard::Right] = "Right Arrow";
  m_keyToDisplayName[Keyboard::Up] = "Up Arrow";
  m_keyToDisplayName[Keyboard::Down] = "Down Arrow";
  m_keyToDisplayName[Keyboard::Numpad0] = "Numpad 0";
  m_keyToDisplayName[Keyboard::Numpad1] = "Numpad 1";
  m_keyToDisplayName[Keyboard::Numpad2] = "Numpad 2";
  m_keyToDisplayName[Keyboard::Numpad3] = "Numpad 3";
  m_keyToDisplayName[Keyboard::Numpad4] = "Numpad 4";
  m_keyToDisplayName[Keyboard::Numpad5] = "Numpad 5";
  m_keyToDisplayName[Keyboard::Numpad6] = "Numpad 6";
  m_keyToDisplayName[Keyboard::Numpad7] = "Numpad 7";
  m_keyToDisplayName[Keyboard::Numpad8] = "Numpad 8";
  m_keyToDisplayName[Keyboard::Numpad9] = "Numpad 9";
  m_keyToDisplayName[Keyboard::F1] = "F1";
  m_keyToDisplayName[Keyboard::F2] = "F2";
  m_keyToDisplayName[Keyboard::F3] = "F3";
  m_keyToDisplayName[Keyboard::F4] = "F4";
  m_keyToDisplayName[Keyboard::F5] = "F5";
  m_keyToDisplayName[Keyboard::F6] = "F6";
  m_keyToDisplayName[Keyboard::F7] = "F7";
  m_keyToDisplayName[Keyboard::F8] = "F8";
  m_keyToDisplayName[Keyboard::F9] = "F9";
  m_keyToDisplayName[Keyboard::F10] = "F10";
  m_keyToDisplayName[Keyboard::F11] = "F11";
  m_keyToDisplayName[Keyboard::F12] = "F12";
  m_keyToDisplayName[Keyboard::F13] = "F13";
  m_keyToDisplayName[Keyboard::F14] = "F14";
  m_keyToDisplayName[Keyboard::F15] = "F15";
  m_keyToDisplayName[Keyboard::Pause] = "Pause";
}

Keyboard VirtualKeyMapping::FromWinAPI(WPARAM wParam) const {
  switch (wParam) {
    case 'A':
      return System::Keyboard::A;
    case 'B':
      return System::Keyboard::B;
    case 'C':
      return System::Keyboard::C;
    case 'D':
      return System::Keyboard::D;
    case 'E':
      return System::Keyboard::E;
    case 'F':
      return System::Keyboard::F;
    case 'G':
      return System::Keyboard::G;
    case 'H':
      return System::Keyboard::H;
    case 'I':
      return System::Keyboard::I;
    case 'J':
      return System::Keyboard::J;
    case 'K':
      return System::Keyboard::K;
    case 'L':
      return System::Keyboard::L;
    case 'M':
      return System::Keyboard::M;
    case 'N':
      return System::Keyboard::N;
    case 'O':
      return System::Keyboard::O;
    case 'P':
      return System::Keyboard::P;
    case 'Q':
      return System::Keyboard::Q;
    case 'R':
      return System::Keyboard::R;
    case 'S':
      return System::Keyboard::S;
    case 'T':
      return System::Keyboard::T;
    case 'U':
      return System::Keyboard::U;
    case 'V':
      return System::Keyboard::V;
    case 'W':
      return System::Keyboard::W;
    case 'X':
      return System::Keyboard::X;
    case 'Y':
      return System::Keyboard::Y;
    case 'Z':
      return System::Keyboard::Z;
    case '0':
      return System::Keyboard::Num0;
    case '1':
      return System::Keyboard::Num1;
    case '2':
      return System::Keyboard::Num2;
    case '3':
      return System::Keyboard::Num3;
    case '4':
      return System::Keyboard::Num4;
    case '5':
      return System::Keyboard::Num5;
    case '6':
      return System::Keyboard::Num6;
    case '7':
      return System::Keyboard::Num7;
    case '8':
      return System::Keyboard::Num8;
    case '9':
      return System::Keyboard::Num9;
    case VK_ESCAPE:
      return System::Keyboard::Escape;
    case VK_LCONTROL:
      return System::Keyboard::LControl;
    case VK_LSHIFT:
      return System::Keyboard::LShift;
    case VK_LMENU:
      return System::Keyboard::LAlt;
    case VK_LWIN:
      return System::Keyboard::LSystem;
    case VK_RCONTROL:
      return System::Keyboard::RControl;
    case VK_RSHIFT:
      return System::Keyboard::RShift;
    case VK_RMENU:
      return System::Keyboard::RAlt;
    case VK_RWIN:
      return System::Keyboard::RSystem;
    case VK_MENU:
      return System::Keyboard::Menu;
    case VK_OEM_4:
      return System::Keyboard::LBracket;
    case VK_OEM_6:
      return System::Keyboard::RBracket;
    case VK_OEM_1:
      return System::Keyboard::Semicolon;
    case VK_OEM_COMMA:
      return System::Keyboard::Comma;
    case VK_OEM_PERIOD:
      return System::Keyboard::Period;
    case VK_OEM_7:
      return System::Keyboard::Apostrophe;
    case VK_OEM_2:
      return System::Keyboard::Slash;
    case VK_OEM_5:
      return System::Keyboard::Backslash;
    case VK_OEM_3:
      return System::Keyboard::Grave;
    case VK_OEM_PLUS:
      return System::Keyboard::Equal;
    case VK_OEM_MINUS:
      return System::Keyboard::Hyphen;
    case VK_SPACE:
      return System::Keyboard::Space;
    case VK_RETURN:
      return System::Keyboard::Enter;
    case VK_BACK:
      return System::Keyboard::Backspace;
    case VK_TAB:
      return System::Keyboard::Tab;
    case VK_PRIOR:
      return System::Keyboard::PageUp;
    case VK_NEXT:
      return System::Keyboard::PageDown;
    case VK_END:
      return System::Keyboard::End;
    case VK_HOME:
      return System::Keyboard::Home;
    case VK_INSERT:
      return System::Keyboard::Insert;
    case VK_DELETE:
      return System::Keyboard::Delete;
    case VK_ADD:
      return System::Keyboard::Add;
    case VK_SUBTRACT:
      return System::Keyboard::Subtract;
    case VK_MULTIPLY:
      return System::Keyboard::Multiply;
    case VK_DIVIDE:
      return System::Keyboard::Divide;
    case VK_LEFT:
      return System::Keyboard::Left;
    case VK_RIGHT:
      return System::Keyboard::Right;
    case VK_UP:
      return System::Keyboard::Up;
    case VK_DOWN:
      return System::Keyboard::Down;
    case VK_NUMPAD0:
      return System::Keyboard::Numpad0;
    case VK_NUMPAD1:
      return System::Keyboard::Numpad1;
    case VK_NUMPAD2:
      return System::Keyboard::Numpad2;
    case VK_NUMPAD3:
      return System::Keyboard::Numpad3;
    case VK_NUMPAD4:
      return System::Keyboard::Numpad4;
    case VK_NUMPAD5:
      return System::Keyboard::Numpad5;
    case VK_NUMPAD6:
      return System::Keyboard::Numpad6;
    case VK_NUMPAD7:
      return System::Keyboard::Numpad7;
    case VK_NUMPAD8:
      return System::Keyboard::Numpad8;
    case VK_NUMPAD9:
      return System::Keyboard::Numpad9;
    case VK_F1:
      return System::Keyboard::F1;
    case VK_F2:
      return System::Keyboard::F2;
    case VK_F3:
      return System::Keyboard::F3;
    case VK_F4:
      return System::Keyboard::F4;
    case VK_F5:
      return System::Keyboard::F5;
    case VK_F6:
      return System::Keyboard::F6;
    case VK_F7:
      return System::Keyboard::F7;
    case VK_F8:
      return System::Keyboard::F8;
    case VK_F9:
      return System::Keyboard::F9;
    case VK_F10:
      return System::Keyboard::F10;
    case VK_F11:
      return System::Keyboard::F11;
    case VK_F12:
      return System::Keyboard::F12;
    case VK_F13:
      return System::Keyboard::F13;
    case VK_F14:
      return System::Keyboard::F14;
    case VK_F15:
      return System::Keyboard::F15;
    case VK_PAUSE:
      return System::Keyboard::Pause;
    default:
      return System::Keyboard::Unknown;
  }
}
}  // namespace System

SPF_NS_END
