#pragma once

#include "SPF/UI/BaseWindow.hpp"

#include <string>

SPF_NS_BEGIN

// Forward declarations
namespace Events {
class EventManager;
}
namespace Hooks {
class HookManager;
}

namespace UI {
class UIManager;
}  // namespace UI

namespace UI {
/**
 * @class HooksWindow
 * @brief An ImGui window for managing and configuring feature hooks.
 */
class HooksWindow : public BaseWindow {
 public:
  HooksWindow(UIManager& uiManager, Events::EventManager& eventManager, const std::string& componentName, const std::string& windowId);

 protected:
  void RenderContent() override;
  const char* GetWindowTitle() const override;

 private:
  UIManager& m_uiManager;
  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;
};
}  // namespace UI

SPF_NS_END
