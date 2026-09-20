#pragma once

#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIManager.hpp"

#include <string>

namespace SPF::UI {
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
}  // namespace SPF::UI
