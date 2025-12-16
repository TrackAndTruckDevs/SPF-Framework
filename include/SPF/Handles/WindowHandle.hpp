#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace UI {
class IWindow;
}  // namespace UI

namespace Handles {
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
}  // namespace Handles
SPF_NS_END
