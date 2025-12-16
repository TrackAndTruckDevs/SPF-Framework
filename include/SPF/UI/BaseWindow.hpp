#pragma once

#include "SPF/UI/IWindow.hpp"
#include "SPF/Namespace.hpp"

#include <string>
#include <set>
#include <imgui.h>
#include <nlohmann/json.hpp>

SPF_NS_BEGIN

namespace UI {
class BaseWindow : public IWindow {
 public:
  BaseWindow(std::string componentName, std::string windowId);
  virtual ~BaseWindow() = default;

  // --- IWindow implementation ---
  void Render() final;
  const std::string& GetWindowId() const final;
  const std::string& GetComponentName() const final;
  bool IsVisible() const final;
  bool IsDocked() const;
  bool IsInteractive() const override;
  bool IsFocused() const override;
  void ApplySettings(const nlohmann::json& settings) override;
  nlohmann::json GetCurrentSettings() const override;
  void SetDrawCallback(SPF_DrawCallback) override {}
  void Focus() override;

  const char* GetWindowTitle() const override;

  // --- Custom Logic ---
  bool IsConfiguredAsDockable() const;

  // --- Programmatic Control ---
  void SetVisibility(bool isVisible);
  void SetDocked(bool isDocked);
  int GetDockPriority() const;
  void SetPosition(float x, float y);
  void SetSize(float width, float height);

 protected:
  virtual void RenderContent() = 0;
  virtual ImGuiWindowFlags GetExtraWindowFlags() const;

 private:
  void SyncStateWithImGui();

 protected:
  std::string m_componentName;
  std::string m_windowId;

  // --- Canonical State (Saved/Loaded) ---
  bool m_isVisible = false;
  bool m_isCollapsed = false;
  bool m_isInteractive = true;
  bool m_is_docked = false;
  bool m_autoScroll = false;
  float m_posX = 100.0f;
  float m_posY = 100.0f;
  float m_sizeW = 300.0f;
  float m_sizeH = 200.0f;
  int m_dockPriority = 0;

  // --- Previous Frame State (For change detection) ---
  bool m_wasVisibleLastFrame = false;
  bool m_wasCollapsedLastFrame = false;
  bool m_wasDockedLastFrame = true;
  ImVec2 m_lastPosition = {0, 0};
  ImVec2 m_lastSize = {0, 0};

  // --- Internal State ---
  bool m_isFocused = false;
  bool m_needsFocus = false;
  bool m_stateIsDirty = true;
  bool m_isConfiguredAsDockable = false;
  bool m_allowUndocking = false;
  std::set<std::string> m_validSettingKeys;
  std::string m_titleLocalizationKey;
  std::string m_defaultTitle;
};
}  // namespace UI

SPF_NS_END
