#pragma once

#include "SPF/Namespace.hpp"

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
  HooksWindow(const std::string& componentName, const std::string& windowId, UIManager& uiManager, Events::EventManager& eventManager);

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;


 private:
  UIManager& m_uiManager;
  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;

  std::string m_cachedNoHooksText;
  std::string m_cachedEnabledCheckbox;
};
}  // namespace UI

SPF_NS_END
