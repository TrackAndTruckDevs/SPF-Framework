#pragma once

#include <Windows.h>
#include <Xinput.h>

#include "SPF/Utils/Signal.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Hooks {
/**
 * @class XInputHook
 * @brief Manages the hooking of XInputGetState to intercept gamepad input.
 */
struct XInputHook {
  // Called after the original XInputGetState is called.
  // Provides the user index and the state structure pointer.
  inline static Utils::Signal<void(DWORD, XINPUT_STATE*)> OnStateGet;

  /**
   * @brief Installs or re-enables the hook for XInputGetState.
   * @return True if successful, false otherwise.
   */
  static bool Install();

  /**
   * @brief Disables the hook for a framework reload.
   */
  static void Uninstall();

  /**
   * @brief Completely removes the hook on shutdown.
   */
  static void Remove();
};
}  // namespace Hooks

SPF_NS_END
