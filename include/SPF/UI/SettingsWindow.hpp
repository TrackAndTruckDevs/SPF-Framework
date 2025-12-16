#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/Config/IConfigService.hpp"

#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>
#include <memory>

SPF_NS_BEGIN

// Forward declarations
namespace Events {
class EventManager;
namespace UI {
struct FocusComponentInSettingsWindow;
}
}  // namespace Events
namespace Utils {
template <typename>
class Sink;
}

namespace UI {
class SettingsWindow : public BaseWindow {
 public:
  SettingsWindow(Config::IConfigService& configService, const std::vector<std::string>& logLevels, Events::EventManager& eventManager, const std::string& componentName,
                 const std::string& windowId);

 protected:
  const char* GetWindowTitle() const override;
  void RenderContent() override;

 public:
  void OnInputCaptured(const Input::InputCaptured& e);
  void OnInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void OnInputCaptureConflict(const Input::InputCaptureConflict& e);

 private:
  void OnFocusComponent(const Events::UI::FocusComponentInSettingsWindow& e);
  void PopulateConfigurableComponents();
  void RenderSettingsNode(const std::string& key, const nlohmann::json& node, const std::string& systemName, const std::string& currentPath);
  void RenderKeybindsSettings();

  Config::IConfigService& m_configService;
  std::vector<std::string> m_logLevels;
  Events::EventManager& m_eventManager;

  std::unique_ptr<Utils::Sink<void(const Events::UI::FocusComponentInSettingsWindow&)>> m_onFocusComponentSink;

  // State for keybinding editor
  std::optional<std::string> m_actionBeingEdited;
  nlohmann::json m_editingBindingObject;
  std::optional<std::string> m_editingBindingAction;      // For the details popup
  std::optional<nlohmann::json> m_editingBindingDetails;  // For the details popup
  nlohmann::json m_originalBindingCopy;                   // Copy of the binding when popup opened
  int m_currentPressThreshold = 500;                      // Buffer for the slider
  std::optional<Input::InputCaptured> m_bufferedInputInfo;
  std::optional<Input::InputCaptureConflict> m_conflictInfo;

  std::vector<std::string> m_configurableComponents;
  std::string m_currentComponent = "framework";

  // Drawer state
  std::string m_keybindsDrawerTitleKey;
  std::string m_keybindsActionHeaderKey;
  std::string m_keybindsKeyHeaderKey;

  // Key capture popup localization keys
  std::string m_keyCapturePopupTitleKey;
  std::string m_keyCapturePressKeyTextKey;
  std::string m_keyCaptureDeleteButtonKey;
  std::string m_keyCaptureCancelButtonKey;
  std::string m_keyCaptureConflictTitleKey;
  std::string m_keyCaptureConflictTextKey;
  std::string m_keyCaptureConflictReassignQuestionKey;
  std::string m_keyCaptureConflictYesButtonKey;
  std::string m_keyCaptureConflictNoButtonKey;

  // Binding details popup
  std::string m_bindingDetailsPopupTitleKey;
  std::string m_bindingDetailsPressTypeLabelKey;
  std::string m_bindingDetailsBehaviorLabelKey;
  std::string m_bindingDetailsBehaviorToggleKey;
  std::string m_bindingDetailsBehaviorHoldKey;
  std::string m_bindingDetailsConsumeLabelKey;
  std::string m_bindingDetailsThresholdLabelKey;
  std::string m_bindingDetailsCloseButtonKey;

  std::string m_keybindsUnassignedTextKey;

  float m_keybindsDrawerHeight = 0.0f;
  float m_keybindsDrawerMinHeight = 35.0f;
  float m_keybindsDrawerMaxHeight = 0.0f;
  bool m_keybindsDrawerExpanded = false;
  bool m_keybindsDrawerDragging = false;
};
}  // namespace UI

SPF_NS_END