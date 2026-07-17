#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"  // For Error logging
#include "SPF/Modules/PluginManager.hpp"  // For GetInstance
#include "SPF/SPF_API/SPF_UI_API.h"       // For SPF_DrawCallback and SPF_Window_Flags
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIElements.hpp"   // For Button
#include "SPF/Utils/SEHGuard.hpp"  // For InvokeSafe across platforms

#include "imgui.h"

#include <minwindef.h>
#include <string>

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
    m_hasCrashed = false;  // Reset crash state when a new callback is set
  }

  /**
   * @brief Sets the behavior flags for this window.
   * @param flags A bitmask of SPF_WindowFlags.
   */
  void SetWindowFlags(SPF_WindowFlags flags) { m_spfFlags = flags; }

 protected:
  void RenderContent() override {
    if (m_hasCrashed) {
      ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "CRITICAL: Window rendering disabled.");
      ImGui::TextWrapped("The plugin '%s' caused an access violation (crash) during UI execution. This usually happens due to ABI mismatch or uninitialized pointers.",
                         m_componentName.c_str());
      ImGui::Spacing();
      if (Button("Try to Reload Rendering")) {
        m_hasCrashed = false;
      }
      return;
    }

    if (m_drawCallback) {
      SPF_UI_API* builder = Modules::PluginManager::GetInstance().GetUIApi();

      DWORD exceptionCode = 0;
      if (!InvokeSafe(builder, &exceptionCode)) {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UIManager");
        if (logger) {
          logger->Error(
            "Plugin '{}' crashed during UI rendering (Exception: 0x{:08X}). The framework intercepted the crash to prevent game instability.", m_componentName, exceptionCode);
        }
        m_hasCrashed = true;
      }
    }
  }

  ImGuiWindowFlags GetExtraWindowFlags() const override {
    ImGuiWindowFlags imFlags = BaseWindow::GetExtraWindowFlags();

    if (m_spfFlags & SPF_WINDOW_FLAG_NO_TITLE_BAR) imFlags |= ImGuiWindowFlags_NoTitleBar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_RESIZE) imFlags |= ImGuiWindowFlags_NoResize;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_MOVE) imFlags |= ImGuiWindowFlags_NoMove;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_SCROLLBAR) imFlags |= ImGuiWindowFlags_NoScrollbar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_SCROLL_WITH_MOUSE) imFlags |= ImGuiWindowFlags_NoScrollWithMouse;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_COLLAPSE) imFlags |= ImGuiWindowFlags_NoCollapse;
    if (m_spfFlags & SPF_WINDOW_FLAG_ALWAYS_AUTO_RESIZE) imFlags |= ImGuiWindowFlags_AlwaysAutoResize;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_BACKGROUND) imFlags |= ImGuiWindowFlags_NoBackground;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_SAVED_SETTINGS) imFlags |= ImGuiWindowFlags_NoSavedSettings;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_MOUSE_INPUTS) imFlags |= ImGuiWindowFlags_NoMouseInputs;
    if (m_spfFlags & SPF_WINDOW_FLAG_MENU_BAR) imFlags |= ImGuiWindowFlags_MenuBar;
    if (m_spfFlags & SPF_WINDOW_FLAG_HORIZONTAL_SCROLLBAR) imFlags |= ImGuiWindowFlags_HorizontalScrollbar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_FOCUS_ON_APPEARING) imFlags |= ImGuiWindowFlags_NoFocusOnAppearing;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_BRING_TO_FRONT_ON_FOCUS) imFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (m_spfFlags & SPF_WINDOW_FLAG_ALWAYS_VERTICAL_SCROLLBAR) imFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
    if (m_spfFlags & SPF_WINDOW_FLAG_ALWAYS_HORIZONTAL_SCROLLBAR) imFlags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_NAV_INPUTS) imFlags |= ImGuiWindowFlags_NoNavInputs;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_NAV_FOCUS) imFlags |= ImGuiWindowFlags_NoNavFocus;
    if (m_spfFlags & SPF_WINDOW_FLAG_UNSAVED_DOCUMENT) imFlags |= ImGuiWindowFlags_UnsavedDocument;
    if (m_spfFlags & SPF_WINDOW_FLAG_NO_DOCKING) imFlags |= ImGuiWindowFlags_NoDocking;

    // Internal flags mapping
    if (m_spfFlags & SPF_WINDOW_FLAG_CHILD_WINDOW) imFlags |= ImGuiWindowFlags_ChildWindow;
    if (m_spfFlags & SPF_WINDOW_FLAG_TOOLTIP) imFlags |= ImGuiWindowFlags_Tooltip;
    if (m_spfFlags & SPF_WINDOW_FLAG_POPUP) imFlags |= ImGuiWindowFlags_Popup;
    if (m_spfFlags & SPF_WINDOW_FLAG_MODAL) imFlags |= ImGuiWindowFlags_Modal;
    if (m_spfFlags & SPF_WINDOW_FLAG_CHILD_MENU) imFlags |= ImGuiWindowFlags_ChildMenu;

    return imFlags;
  }

 private:
  bool InvokeSafe(SPF_UI_API* builder, DWORD* outExceptionCode) {
    return Utils::InvokeSafe([this, builder]() { m_drawCallback(builder, m_userData); }, outExceptionCode);
  }

  SPF_DrawCallback m_drawCallback = nullptr;
  void* m_userData = nullptr;
  SPF_WindowFlags m_spfFlags = SPF_WINDOW_FLAG_NONE;
  bool m_hasCrashed = false;
};
}  // namespace UI
SPF_NS_END
