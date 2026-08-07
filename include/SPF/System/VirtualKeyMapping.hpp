#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/System/Keyboard.hpp"

#include <minwindef.h>
#include <string>
#include <unordered_map>

SPF_NS_BEGIN

namespace System {
class VirtualKeyMapping {
 public:
  // Returns the single instance of the class
  static VirtualKeyMapping& GetInstance();

  // Gets the framework's key enum from a string representation
  Keyboard GetKey(const std::string& keyName) const;

  // Gets the string representation from a framework's key enum
  std::string GetKeyName(Keyboard key) const;

  // Gets the human-readable display name for a key
  std::string GetKeyDisplayName(Keyboard key) const;

  // Converts a Windows virtual-key code to the framework's key enum
  Keyboard FromWinAPI(WPARAM wParam) const;

  // Converts a Windows virtual-key code to the framework's key enum, resolving
  // the left/right side of generic modifier codes (VK_CONTROL/VK_MENU) from the
  // extended-key flag (WM_KEYDOWN lParam bit 24).
  Keyboard FromWinAPI(WPARAM wParam, bool isExtended) const;

 private:
  // Private constructor and destructor for singleton pattern
  VirtualKeyMapping();
  ~VirtualKeyMapping() = default;

  // Delete copy and move operations
  VirtualKeyMapping(const VirtualKeyMapping&) = delete;
  VirtualKeyMapping& operator=(const VirtualKeyMapping&) = delete;
  VirtualKeyMapping(VirtualKeyMapping&&) = delete;
  VirtualKeyMapping& operator=(VirtualKeyMapping&&) = delete;

  void InitializeMapping();

  std::unordered_map<std::string, Keyboard> m_stringToKey;
  std::unordered_map<Keyboard, std::string> m_keyToString;
  std::unordered_map<Keyboard, std::string> m_keyToDisplayName;
};
}  // namespace System

SPF_NS_END
