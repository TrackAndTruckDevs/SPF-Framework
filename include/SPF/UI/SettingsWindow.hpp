#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


SPF_NS_BEGIN

// Forward declarations
namespace Events {
class EventManager;
namespace UI {
struct FocusComponentInSettingsWindow;
}
namespace Config {
struct OnKeybindsModified;
}
}  // namespace Events
namespace Utils {
template <typename>
class Sink;
}

namespace UI {
class SettingsWindow : public BaseWindow {
 public:
  SettingsWindow(const std::string& componentName, const std::string& windowId, Config::IConfigService& configService, const std::vector<std::string>& logLevels,
                 Events::EventManager& eventManager);

 protected:


  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  void OnFocusComponent(const Events::UI::FocusComponentInSettingsWindow& e);
  void PopulateConfigurableComponents();
  void UpdateHardwareCodeUsageCount(const Events::Config::OnKeybindsModified& e);
  void RenderSettingsNode(const std::string& key, const nlohmann::ordered_json& node, const std::string& systemName, const std::string& currentPath, int depth);
  void RenderKeybindsSettings();
  void DrawSettingsRows(const nlohmann::ordered_json& settingsNode, const std::string& systemName, const std::string& parentPath);

  Config::IConfigService& m_configService;
  std::vector<std::string> m_logLevels;
  Events::EventManager& m_eventManager;

  std::unique_ptr<Utils::Sink<void(const Events::UI::FocusComponentInSettingsWindow&)>> m_onFocusComponentSink;
  std::unique_ptr<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>> m_onKeybindsModifiedSink;

  // Note: both the "press a key" capture/conflict popup (UI::KeyCapturePopup) and the
  // "binding details" gear-icon popup (UI::BindingDetailsPopup) are now shared components
  // owned by UIManager, so they can also be triggered by plugins via SPF_KeyBinds_API's
  // Kbind_OpenRebindPopup / Kbind_OpenBindingDetailsPopup. See UIManager::GetKeyCapturePopup()
  // and UIManager::GetBindingDetailsPopup().
  std::map<uint32_t, int> m_hardwareCodeUsageCount;

  std::vector<std::string> m_configurableComponents;
  std::string m_currentComponent = "framework";

  // Drawer state
  std::string m_keybindsDrawerTitleKey;
  std::string m_keybindsActionHeaderKey;
  std::string m_keybindsKeyHeaderKey;

  std::string m_keybindsUnassignedTextKey;

  std::string m_noConfigurableComponentsKey;
  std::string m_componentInfoErrorKey;
  std::string m_noConfigurableSystemsKey;
  std::string m_keybindsNotAvailableKey;
  std::string m_settingHeaderKey;
  std::string m_valueHeaderKey;
  std::string m_nullValueFormatKey;

  float m_keybindsDrawerHeight = 0.0f;
  float m_keybindsDrawerMinHeight = 35.0f;
  float m_keybindsDrawerMaxHeight = 0.0f;
  bool m_keybindsDrawerExpanded = false;
  bool m_keybindsDrawerDragging = false;
};
}  // namespace UI

SPF_NS_END
