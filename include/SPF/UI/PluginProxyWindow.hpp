#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/SPF_API/SPF_Plugin.h"    // For SPF_DrawCallback
#include "SPF/Modules/PluginManager.hpp"  // For GetInstance
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace UI {
/**
 * @brief A window implementation used as a proxy for windows declared by plugins.
 *
 * This window is created by the UIManager based on a plugin's manifest.
 * The plugin later provides the drawing logic via a callback.
 */
class PluginProxyWindow : public BaseWindow {
 public:
  PluginProxyWindow(const std::string& componentName, const std::string& windowId) : BaseWindow(componentName, windowId) {}

  /**
   * @brief Sets the function pointer that will be called to render this window's content.
   * @param callback The C-style function pointer for drawing.
   * @param user_data The user-provided data to pass to the callback.
   */
  void SetDrawCallback(SPF_DrawCallback callback, void* user_data) {
    m_drawCallback = callback;
    m_userData = user_data;
  }

 protected:
  void RenderContent() override {
    if (m_drawCallback) {
      auto* builder = Modules::PluginManager::GetInstance().GetUIApi();
      m_drawCallback(builder, m_userData);
    }
  }

  ImGuiWindowFlags GetExtraWindowFlags() const override { return ImGuiWindowFlags_NoDocking; }

 private:
  SPF_DrawCallback m_drawCallback = nullptr;
  void* m_userData = nullptr;
};
}  // namespace UI
SPF_NS_END
