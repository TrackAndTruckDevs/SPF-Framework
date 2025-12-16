#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/Localization/LocalizationManager.hpp"  // Include the new icons header

#include "SPF/Logging/LoggerFactory.hpp"

#include <cmath>  // For fabs

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Logging;
using namespace SPF::Localization;

BaseWindow::BaseWindow(std::string componentName, std::string windowId)
    : m_componentName(std::move(componentName)),
      m_windowId(std::move(windowId)),
      m_defaultTitle(m_windowId),
      m_titleLocalizationKey(m_windowId + ".title") {}

const std::string& BaseWindow::GetWindowId() const { return m_windowId; }

const std::string& BaseWindow::GetComponentName() const { return m_componentName; }

bool BaseWindow::IsVisible() const { return m_isVisible; }

bool BaseWindow::IsInteractive() const { return m_isInteractive; }

bool BaseWindow::IsFocused() const { return m_isFocused; }

bool BaseWindow::IsDocked() const { return m_is_docked; }

void BaseWindow::SetVisibility(bool isVisible) {
  if (m_isVisible != isVisible) {
    m_isVisible = isVisible;
    m_stateIsDirty = true;
  }
}

void BaseWindow::SetDocked(bool isDocked) {
    if (m_isConfiguredAsDockable && m_is_docked != isDocked) {
        m_is_docked = isDocked;
        m_stateIsDirty = true;
    }
}

void BaseWindow::Focus() { m_needsFocus = true; }

int BaseWindow::GetDockPriority() const { return m_dockPriority; }

void BaseWindow::SetPosition(float x, float y) {
  m_posX = x;
  m_posY = y;
  m_stateIsDirty = true;
}

void BaseWindow::SetSize(float width, float height) {
  m_sizeW = width;
  m_sizeH = height;
  m_stateIsDirty = true;
}

const char* BaseWindow::GetWindowTitle() const {
  const auto& localizedTitle =
      LocalizationManager::GetInstance().Get(m_componentName, m_titleLocalizationKey);

  if (localizedTitle == m_titleLocalizationKey) {
    return m_defaultTitle.c_str();
  }

  return localizedTitle.c_str();
}

bool BaseWindow::IsConfiguredAsDockable() const { return m_isConfiguredAsDockable; }

void BaseWindow::ApplySettings(const nlohmann::json& settings) {
  m_validSettingKeys.clear();

  m_isConfiguredAsDockable = settings.contains("is_docked");

  if (m_isConfiguredAsDockable) {
    m_is_docked = settings.value("is_docked", m_is_docked);
    m_validSettingKeys.insert("is_docked");
  }

  if (settings.contains("is_visible")) {
    m_isVisible = settings.value("is_visible", m_isVisible);
    m_validSettingKeys.insert("is_visible");
  }
  if (settings.contains("is_collapsed")) {
    m_isCollapsed = settings.value("is_collapsed", m_isCollapsed);
    m_validSettingKeys.insert("is_collapsed");
  }
  if (settings.contains("is_interactive")) {
    m_isInteractive = settings.value("is_interactive", m_isInteractive);
    m_validSettingKeys.insert("is_interactive");
  }
  if (settings.contains("auto_scroll")) {
    m_autoScroll = settings.value("auto_scroll", m_autoScroll);
    m_validSettingKeys.insert("auto_scroll");
  }
  if (settings.contains("pos_x")) {
    m_posX = settings.value("pos_x", m_posX);
    m_validSettingKeys.insert("pos_x");
  }
  if (settings.contains("pos_y")) {
    m_posY = settings.value("pos_y", m_posY);
    m_validSettingKeys.insert("pos_y");
  }
  if (settings.contains("size_w")) {
    m_sizeW = settings.value("size_w", m_sizeW);
    m_validSettingKeys.insert("size_w");
  }
  if (settings.contains("size_h")) {
    m_sizeH = settings.value("size_h", m_sizeH);
    m_validSettingKeys.insert("size_h");
  }
  if (settings.contains("dock_priority")) {
    m_dockPriority = settings.value("dock_priority", m_dockPriority);
    m_validSettingKeys.insert("dock_priority");
  }
  if (settings.contains("allow_undocking")) {
    m_allowUndocking = settings.value("allow_undocking", m_allowUndocking);
    m_validSettingKeys.insert("allow_undocking");
  }

  m_wasVisibleLastFrame = m_isVisible;
  m_wasCollapsedLastFrame = m_isCollapsed;
  m_wasDockedLastFrame = m_is_docked;
  m_lastPosition = ImVec2(m_posX, m_posY);
  m_lastSize = ImVec2(m_sizeW, m_sizeH);

  m_stateIsDirty = true;
}

nlohmann::json BaseWindow::GetCurrentSettings() const {
  nlohmann::json settings;
  settings["is_visible"] = m_isVisible;
  settings["is_interactive"] = m_isInteractive;

  if (m_isConfiguredAsDockable) {
    settings["is_docked"] = m_is_docked;
  }

  if (m_validSettingKeys.count("is_collapsed")) {
    settings["is_collapsed"] = m_isCollapsed;
  }

  if (m_validSettingKeys.count("dock_priority")) {
    settings["dock_priority"] = m_dockPriority;
  }

  if (m_validSettingKeys.count("auto_scroll")) {
    settings["auto_scroll"] = m_autoScroll;
  }

  if (!m_is_docked) {
    settings["pos_x"] = m_posX;
    settings["pos_y"] = m_posY;
    settings["size_w"] = m_sizeW;
    settings["size_h"] = m_sizeH;
  }
  return settings;
}

ImGuiWindowFlags BaseWindow::GetExtraWindowFlags() const {
  return 0;  // Base implementation returns no extra flags
}

void BaseWindow::Render() {
  if (!m_isVisible) {
    return;
  }

  if (m_needsFocus) {
    ImGui::SetNextWindowFocus();
    m_needsFocus = false;
  }

  bool isApplyingForcedState = m_stateIsDirty;
  ImGuiCond condition = m_stateIsDirty ? ImGuiCond_Always : ImGuiCond_Once;
  ImGui::SetNextWindowPos(ImVec2(m_posX, m_posY), condition);
  ImGui::SetNextWindowSize(ImVec2(m_sizeW, m_sizeH), condition);
  ImGui::SetNextWindowCollapsed(m_isCollapsed, condition);

  m_stateIsDirty = false;

  bool* p_open = m_isConfiguredAsDockable ? nullptr : &m_isVisible;
  ImGuiWindowFlags flags = GetExtraWindowFlags();

  // Hide the collapse button if the window is not configured to be collapsible
  if (m_validSettingKeys.find("is_collapsed") == m_validSettingKeys.end()) {
    flags |= ImGuiWindowFlags_NoCollapse;
  }

  if (m_isConfiguredAsDockable && !m_is_docked) {
    flags |= ImGuiWindowFlags_NoDocking;
  }

  std::string stableId = "###" + m_componentName + "_" + m_windowId;
  std::string fullTitle = std::string(GetWindowTitle()) + stableId;

  if (ImGui::Begin(fullTitle.c_str(), p_open, flags)) {
    m_isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_None);

    if (m_isConfiguredAsDockable && m_allowUndocking) {
      const char* buttonText_key = m_is_docked ? "ui.actions.undock" : "ui.actions.dock";
      std::string buttonText = LocalizationManager::GetInstance().Get(m_componentName, buttonText_key);
      if (buttonText == buttonText_key) {  // Fallback if key not found
        buttonText = LocalizationManager::GetInstance().Get("framework", buttonText_key);
      }

      float windowWidth = ImGui::GetWindowWidth();
      float buttonWidth = ImGui::CalcTextSize(buttonText.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;

      ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
      if (ImGui::Button(buttonText.c_str())) {
        bool was_docked = m_is_docked;
        m_is_docked = !m_is_docked;
        m_stateIsDirty = true;

        if (was_docked && !m_is_docked) {
          ImVec2 current_pos = ImGui::GetWindowPos();
          SetPosition(current_pos.x + 20, current_pos.y + 20);
        }
      }
      ImGui::Separator();
    }

    if (!isApplyingForcedState) {
      SyncStateWithImGui();
    }

    RenderContent();
  }
  ImGui::End();

  if (m_isVisible != m_wasVisibleLastFrame) {
    m_wasVisibleLastFrame = m_isVisible;
  }
}

void BaseWindow::SyncStateWithImGui() {
  if (ImGui::GetCurrentContext() == nullptr || ImGui::IsWindowAppearing()) {
    return;
  }

  // Sync docked state
  bool isDockedNow = ImGui::IsWindowDocked();
  if (isDockedNow != m_wasDockedLastFrame) {
    m_is_docked = isDockedNow;
    m_wasDockedLastFrame = isDockedNow;
  }

  // Sync collapsed state
  bool isCollapsedNow = ImGui::IsWindowCollapsed();
  if (isCollapsedNow != m_wasCollapsedLastFrame) {
    m_isCollapsed = isCollapsedNow;
    m_wasCollapsedLastFrame = isCollapsedNow;
  }

  // Sync position
  ImVec2 currentPos = ImGui::GetWindowPos();
  if (std::fabs(currentPos.x - m_lastPosition.x) > 0.1f || std::fabs(currentPos.y - m_lastPosition.y) > 0.1f) {
    m_posX = currentPos.x;
    m_posY = currentPos.y;
    m_lastPosition = currentPos;
  }

  // Sync size (only if not collapsed)
  if (!isCollapsedNow) {
    ImVec2 currentSize = ImGui::GetWindowSize();
    if (std::fabs(currentSize.x - m_lastSize.x) > 0.1f || std::fabs(currentSize.y - m_lastSize.y) > 0.1f) {
      m_sizeW = currentSize.x;
      m_sizeH = currentSize.y;
      m_lastSize = currentSize;
    }
  }
}

}  // namespace UI
SPF_NS_END
