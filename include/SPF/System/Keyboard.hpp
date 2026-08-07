#pragma once

#include "SPF/Namespace.hpp"

#include <string>


SPF_NS_BEGIN

namespace System {
enum class Keyboard {
  Unknown = -1,
  A = 0,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  Escape,
  LControl,
  LShift,
  LAlt,
  LSystem,
  RControl,
  RShift,
  RAlt,
  RSystem,
  Menu,
  LBracket,
  RBracket,
  Semicolon,
  Comma,
  Period,
  Apostrophe,
  Slash,
  Backslash,
  Grave,
  Equal,
  Hyphen,
  Space,
  Enter,
  Backspace,
  Tab,
  PageUp,
  PageDown,
  End,
  Home,
  Insert,
  Delete,
  Add,
  Subtract,
  Multiply,
  Divide,
  Left,
  Right,
  Up,
  Down,
  Numpad0,
  Numpad1,
  Numpad2,
  Numpad3,
  Numpad4,
  Numpad5,
  Numpad6,
  Numpad7,
  Numpad8,
  Numpad9,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  Pause,

  // System / lock keys
  CapsLock,
  NumLock,
  ScrollLock,
  PrintScreen,

  // Numpad extras
  NumpadDecimal,

  // OEM special keys
  Oem8,
  Oem102,

  // Extended function keys
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,

  // Media keys
  VolumeMute,
  VolumeDown,
  VolumeUp,
  MediaNextTrack,
  MediaPrevTrack,
  MediaStop,
  MediaPlayPause,

  // Browser keys
  BrowserBack,
  BrowserForward,
  BrowserRefresh,
  BrowserStop,
  BrowserSearch,
  BrowserFavorites,
  BrowserHome,

  // Application / launch keys
  LaunchMail,
  LaunchMediaSelect,
  LaunchApp1,
  LaunchApp2,

  Count
};

// Functions to convert between string and enum
std::string ToString(Keyboard key);
Keyboard FromString(const std::string& str);
}  // namespace System

SPF_NS_END
