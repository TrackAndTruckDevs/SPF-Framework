#pragma once

#include "SPF/Namespace.hpp"

#include <cstdint>


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

  /**
   * @brief Sends a virtual Key Up event to the game window.
   *
   * This is used to "reset" the game's internal state when a key is physically held
   * but should be logically ignored (e.g., when it becomes part of a blocking chord).
   *
   * @param hardwareCode The 32-bit hardware code of the key (as used in InputManager).
   */
  static void SendVirtualKeyRelease(uint32_t hardwareCode);
};
}  // namespace Hooks

SPF_NS_END
