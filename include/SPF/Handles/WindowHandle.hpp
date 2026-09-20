#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/UI/IWindow.hpp"

namespace SPF::Handles {
/**
 * @brief A handle for the UI API, representing a single window.
 *
 * This handle holds a pointer to the C++ IWindow object, allowing the C-API
 * to manipulate the window state programmatically.
 */
struct WindowHandle : IHandle {
  UI::IWindow* window = nullptr;

  WindowHandle(UI::IWindow* window) : window(window) {}
};
}  // namespace SPF::Handles
