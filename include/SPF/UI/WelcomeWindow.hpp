#pragma once

#include "SPF/Renderer/ITexture.hpp"
#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/UI/IWindow.hpp"
#include "SPF/UI/MarkdownRenderer.hpp"
#include "SPF/Utils/Signal.hpp"

#include "nlohmann/json_fwd.hpp"

#include <memory>
#include <string>

namespace SPF::UI {

enum class WelcomeMode { FirstInstall, FrameworkUpdate };

class WelcomeWindow : public IWindow {
 public:
  WelcomeWindow(const std::string& componentName, const std::string& windowId);
  virtual ~WelcomeWindow() = default;

  // IWindow implementation
  void Render() override;
  const std::string& GetWindowId() const override { return m_windowId; }
  const std::string& GetComponentName() const override { return m_componentName; }
  bool IsVisible() const override { return m_isVisible; }
  bool IsDeveloperOnly() const override { return false; }
  bool IsPersistent() const override { return false; }
  bool IsInteractive() const override { return true; }
  bool IsFocused() const override { return m_isFocused; }
  void Focus() override { m_isFocused = true; }
  const char* GetWindowTitle() const override { return "Welcome"; }

  void ApplySettings(const nlohmann::ordered_json& settings) override;
  void SetDrawCallback(SPF_DrawCallback callback) override {}
  nlohmann::ordered_json GetCurrentSettings() const override;

  // Helpers
  void SetVisibility(bool visible) { m_isVisible = visible; }
  void SetMode(WelcomeMode mode) { m_mode = mode; }
  void SetUpdateContent(const std::string& changelogMarkdown);

 private:
  void InitializeResources();
  void UpdateLocalization();
  void OnFrameworkLanguageChanged(const std::string& componentName);
  void RenderContent();
  void RenderStackedLayout();
  void RenderTwoColumnLayout();
  void RenderFirstInstallCards(float contentX, float contentWidth, bool twoColumnLayout);
  void RenderUpdateCard(float contentX, float contentWidth, float maxNotesHeight);
  void RenderPluginsPathRow(const std::string& path);
  void RenderGoButton(float buttonX);
  void RenderFooter(float x, float width, bool centered);
  std::string GetVersionLabel() const;

  std::string m_componentName;
  std::string m_windowId;
  bool m_isVisible = false;
  bool m_isFocused = false;
  bool m_wasVisibleLastFrame = false;

  WelcomeMode m_mode = WelcomeMode::FirstInstall;
  std::string m_updateChangelog;  // Server release notes, restyled for the update card

  std::unique_ptr<Rendering::ITexture> m_logoTexture;
  MarkdownRenderer m_markdownRenderer;
  bool m_resourcesInitialized = false;

  // --- Layout state measured on the previous frame ---
  float m_lastLayoutHeight = 0.0f;   // Total height of logo/footer, used to center vertically
  float m_gridCardHeights[4] = {};   // Content height of each grid card, used to equalize cards sharing a row
  float m_introWidth = 0.0f;         // Rendered width of the intro paragraph, used to center it
  float m_footerWidth = 0.0f;        // Rendered width of the footer, used to center it
  float m_leftColumnHeight = 0.0f;   // Two-column layout: height of the logo/title/button column
  float m_rightColumnHeight = 0.0f;  // Two-column layout: height of the cards column
  float m_menuKeyTextHeight = 0.0f;  // Height of the menu-key card text, used to center it on the keycap

  // --- Localized Strings (Cached) ---
  std::string m_locTitle;
  std::string m_locIntroTitle;
  std::string m_locIntroText;
  std::string m_locSection1Title;
  std::string m_locSection1Text;
  std::string m_locSection2Title;
  std::string m_locSection2Text;
  std::string m_locSection3Title;
  std::string m_locSection3Text;
  std::string m_locSection4Title;
  std::string m_locSection4Text;
  std::string m_locSection5Title;
  std::string m_locSection5Text;
  std::string m_locFooter;
  std::string m_locBtnGo;
  std::string m_locMenuKey;
  std::string m_locOpenFolder;
  std::string m_locUpdateTitle;
  std::string m_locUpdateText;
  std::string m_locUpdateNotesTitle;
  std::string m_locUpdateButton;

  // Refreshes the cached strings above when the framework language changes while the window is open.
  Utils::Sink<void(const std::string&)> m_langSink;
};

}  // namespace SPF::UI
