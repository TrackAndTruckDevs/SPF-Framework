#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "nlohmann/json_fwd.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
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

 public:
  void OnInputCaptured(const Input::InputCaptured& e);
  void OnInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void OnInputCaptureUpdate(const Input::InputCaptureUpdate& e);
  void OnInputCaptureConflict(const Input::InputCaptureConflict& e);

 private:
  void OnFocusComponent(const Events::UI::FocusComponentInSettingsWindow& e);
  void PopulateConfigurableComponents();
  void UpdateHardwareCodeUsageCount(const Events::Config::OnKeybindsModified& e);
  void RenderSettingsNode(const std::string& key, const nlohmann::ordered_json& node, const std::string& systemName, const std::string& currentPath, int depth);
  void RenderKeybindsSettings();
  void DrawSettingsRows(const nlohmann::ordered_json& settingsNode, const std::string& systemName, const std::string& parentPath);
  std::string GetTranslatedActionName(const std::string& fullActionName) const;

  Config::IConfigService& m_configService;
  std::vector<std::string> m_logLevels;
  Events::EventManager& m_eventManager;

  std::unique_ptr<Utils::Sink<void(const Events::UI::FocusComponentInSettingsWindow&)>> m_onFocusComponentSink;
  std::unique_ptr<Utils::Sink<void(const Input::InputCaptureUpdate&)>> m_onInputCaptureUpdateSink;
  std::unique_ptr<Utils::Sink<void(const Events::Config::OnKeybindsModified&)>> m_onKeybindsModifiedSink;

  // State for keybinding editor
  std::optional<std::string> m_actionBeingEdited;
  std::vector<std::shared_ptr<Modules::IBindableInput>> m_currentChordInputs;
  nlohmann::ordered_json m_editingBindingObject;
  std::optional<std::string> m_editingBindingAction;              // For the details popup
  std::optional<nlohmann::ordered_json> m_editingBindingDetails;  // For the details popup
  nlohmann::ordered_json m_originalBindingCopy;                   // Copy of the binding when popup opened
  int m_currentPressThreshold = 500;                              // Buffer for the slider
  std::optional<Input::InputCaptured> m_bufferedInputInfo;
  std::optional<Input::InputCaptureConflict> m_conflictInfo;
  std::map<uint32_t, int> m_hardwareCodeUsageCount;

  // For press-type swap conflict resolution in the details popup
  std::optional<std::pair<std::string, nlohmann::ordered_json>> m_pressTypeSwapConflict;
  std::optional<std::string> m_pressTypeSwapNewValue;
  bool m_shouldOpenBindingDetailsPopup = false;

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
  std::string m_keyCaptureConflictTextDetailedKey;
  std::string m_keyCaptureReassignShortPressButtonKey;
  std::string m_keyCaptureReassignLongPressButtonKey;
  std::string m_keyCaptureReassignPositiveSideButtonKey;
  std::string m_keyCaptureReassignNegativeSideButtonKey;
  std::string m_keyCaptureAddShortPressButtonKey;
  std::string m_keyCaptureAddLongPressButtonKey;
  std::string m_keyCaptureAddPositiveSideButtonKey;
  std::string m_keyCaptureAddNegativeSideButtonKey;
  std::string m_keyCaptureReassignEntireAxisButtonKey;
  std::string m_keyCaptureActionListFormatKey;

  // Binding details popup
  std::string m_bindingDetailsPopupTitleKey;
  std::string m_bindingDetailsPressTypeLabelKey;
  std::string m_bindingDetailsBehaviorLabelKey;
  std::string m_bindingDetailsBehaviorToggleKey;
  std::string m_bindingDetailsBehaviorHoldKey;
  std::string m_bindingDetailsConsumeLabelKey;
  std::string m_bindingDetailsThresholdLabelKey;
  std::string m_bindingDetailsCloseButtonKey;

  std::string m_bindingDetailsModeLabelKey;
  std::string m_bindingDetailsModeAnalogKey;
  std::string m_bindingDetailsModeDigitalKey;
  std::string m_bindingDetailsDeadzoneLabelKey;
  std::string m_bindingDetailsSaturationLabelKey;
  std::string m_bindingDetailsSensitivityLabelKey;
  std::string m_bindingDetailsCurveLabelKey;
  std::string m_bindingDetailsSmoothingLabelKey;
  std::string m_bindingDetailsSideLabelKey;
  std::string m_bindingDetailsSideBothKey;
  std::string m_bindingDetailsSidePositiveKey;
  std::string m_bindingDetailsSideNegativeKey;
  std::string m_bindingDetailsRangeMinLabelKey;
  std::string m_bindingDetailsRangeMaxLabelKey;
  std::string m_bindingDetailsAccumulatorModeLabelKey;
  std::string m_bindingDetailsInvertLabelKey;

  // Cached localization strings for inline conflict resolution
  std::string m_conflictPressTypeMessage;
  std::string m_conflictSwapQuestion;
  std::string m_conflictYesSwapButton;
  std::string m_conflictCancelButton;

  // Cached localization strings for enums
  std::string m_enumPressTypeShortKey;
  std::string m_enumPressTypeLongKey;
  std::string m_enumSideBothKey;
  std::string m_enumSidePositiveKey;
  std::string m_enumSideNegativeKey;

  std::string m_keybindsUnassignedTextKey;

  std::string m_noConfigurableComponentsKey;
  std::string m_componentInfoErrorKey;
  std::string m_noConfigurableSystemsKey;
  std::string m_keybindsNotAvailableKey;
  std::string m_nullValueFormatKey;

  float m_keybindsDrawerHeight = 0.0f;
  float m_keybindsDrawerMinHeight = 35.0f;
  float m_keybindsDrawerMaxHeight = 0.0f;
  bool m_keybindsDrawerExpanded = false;
  bool m_keybindsDrawerDragging = false;
};
}  // namespace UI

SPF_NS_END