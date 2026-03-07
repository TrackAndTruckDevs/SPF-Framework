#pragma once

#include "SPF/UI/IWindow.hpp"
#include "SPF/UI/MarkdownRenderer.hpp"
#include "SPF/Renderer/ITexture.hpp"
#include <memory>
#include <string>

SPF_NS_BEGIN

namespace UI {

class WelcomeWindow : public IWindow {
 public:
  WelcomeWindow(const std::string& componentName, const std::string& windowId);
  virtual ~WelcomeWindow() = default;

  // IWindow implementation
  void Render() override;
  const std::string& GetWindowId() const override { return m_windowId; }
  const std::string& GetComponentName() const override { return m_componentName; }
  bool IsVisible() const override { return m_isVisible; }
  bool IsInteractive() const override { return true; }
  bool IsFocused() const override { return m_isFocused; }
  void Focus() override { m_isFocused = true; }
  const char* GetWindowTitle() const override { return "Welcome"; }
  
  void ApplySettings(const nlohmann::ordered_json& settings) override;
  void SetDrawCallback(SPF_DrawCallback callback) override {}
  nlohmann::ordered_json GetCurrentSettings() const override;

  // Helpers
  void SetVisibility(bool visible) { m_isVisible = visible; }

 private:
  void InitializeResources();
  void UpdateLocalization();
  void RenderContent();
  void RenderMarkdownContent();

  std::string m_componentName;
  std::string m_windowId;
  bool m_isVisible = false;
  bool m_isFocused = false;
  bool m_wasVisibleLastFrame = false;

  std::unique_ptr<Rendering::ITexture> m_logoTexture;
  MarkdownRenderer m_markdownRenderer;
  bool m_resourcesInitialized = false;

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
};

}  // namespace UI

SPF_NS_END
