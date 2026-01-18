#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/SPF_API/SPF_UI_API.h"    // For SPF_DrawCallback and SPF_Window_Flags
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

  /**
   * @brief Sets the behavior flags for this window.
   * @param flags A bitmask of SPF_Window_Flags.
   */
  void SetWindowFlags(SPF_Window_Flags flags) {
    m_spfFlags = flags;
  }

 protected:
  void RenderContent() override {
    if (m_drawCallback) {
      auto* builder = Modules::PluginManager::GetInstance().GetUIApi();
      m_drawCallback(builder, m_userData);
    }
  }

  ImGuiWindowFlags GetExtraWindowFlags() const override {
    ImGuiWindowFlags imFlags = BaseWindow::GetExtraWindowFlags(); // Call base implementation

    if (m_spfFlags & SPF_WINDOW_FLAG_NO_TITLE) imFlags |= ImGuiWindowFlags_NoTitleBar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_RESIZE) imFlags |= ImGuiWindowFlags_NoResize;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_MOVE) imFlags |= ImGuiWindowFlags_NoMove;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_SCROLLBAR) imFlags |= ImGuiWindowFlags_NoScrollbar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_COLLAPSE) imFlags |= ImGuiWindowFlags_NoCollapse;
    if (m_spfFlags & SPF_WINDOW_FLAG_ALWAYS_AUTO_RESIZE) imFlags |= ImGuiWindowFlags_AlwaysAutoResize;
    if (m_spfFlags & SPF_WINDOW_FLAG_MENU_BAR) imFlags |= ImGuiWindowFlags_MenuBar;
    if (m_spfFlags & SPF_WINDOW_FLAG_HORIZONTAL_SCROLLBAR) imFlags |= ImGuiWindowFlags_HorizontalScrollbar;

    return imFlags;
  }

 private:
  SPF_DrawCallback m_drawCallback = nullptr;
  void* m_userData = nullptr;
  SPF_Window_Flags m_spfFlags = SPF_WINDOW_FLAG_NONE;
};
}  // namespace UI
SPF_NS_END
