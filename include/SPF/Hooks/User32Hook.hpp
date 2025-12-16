#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Hooks {
/**
 * @brief Manages hooks for functions in user32.dll.
 *
 * This class is responsible for intercepting WinAPI calls like SetCursorPos,
 * ShowCursor, and SetCursor to prevent the game from interfering with the UI cursor.
 * It supports being temporarily uninstalled (disabled) for framework reloads
 * and completely removed on shutdown.
 */
class User32Hook {
 public:
  /**
   * @brief Installs or re-enables hooks for SetCursorPos, GetKeyboardState, etc.
   * @return True if installation was successful, false otherwise.
   */
  static bool Install();

  /**
   * @brief Disables all hooks managed by this class for a framework reload.
   */
  static void Uninstall();

  /**
   * @brief Completely removes all hooks managed by this class on shutdown.
   */
  static void Remove();
};
}  // namespace Hooks

SPF_NS_END
