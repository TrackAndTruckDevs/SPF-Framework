#pragma once

#include "SPF/Namespace.hpp"

#include "nlohmann/json.hpp"  // IWYU pragma: keep

#include <memory>
#include <optional>
#include <string>
#include <utility>

SPF_NS_BEGIN

namespace Events {
class EventManager;
}
namespace Config {
struct IConfigService;
}
namespace Utils {
template <typename>
class Sink;
}

namespace UI {

/**
 * @brief Reusable "binding details" (gear icon) popup: press-type / behavior / consume
 *        policy / press-threshold for digital bindings, and the full analog axis tuning
 *        UI (mode, deadzone, saturation, sensitivity, curve, smoothing, range, invert,
 *        side, live value graph) for axis bindings.
 *
 * @details Extracted out of SettingsWindow for the same reason as KeyCapturePopup: a
 *          single shared instance (owned by UIManager) lets both the native Settings
 *          window and a plugin's own custom menu (via SPF_KeyBinds_API's
 *          Kbind_OpenBindingDetailsPopup) open the exact same UI and write through the
 *          same RequestBindingPropertyUpdate pipeline, so nothing can drift out of sync.
 */
class BindingDetailsPopup {
 public:
  BindingDetailsPopup(Events::EventManager& eventManager, Config::IConfigService& configService);

  void RefreshLocalization();

  /**
   * @brief Opens the details popup for a specific binding.
   * @param actionFullName Fully-qualified action name (e.g. "MyPlugin.UI.toggle").
   * @param bindingJson The binding's current JSON representation (as returned by the
   *                     binding's ToJson(), or read back from config).
   */
  void Open(const std::string& actionFullName, const nlohmann::ordered_json& bindingJson);

  /**
   * @brief Draws the popup, if open. Must be called once per frame from a top-level
   *        render location, same as KeyCapturePopup::Render().
   */
  void Render();

 private:
  void RefreshLocalizationOnLanguageChange(const std::string& componentName);

  Events::EventManager& m_eventManager;
  Config::IConfigService& m_configService;

  std::unique_ptr<Utils::Sink<void(const std::string&)>> m_onLanguageChangedSink;

  bool m_shouldOpen = false;
  std::optional<std::string> m_editingBindingAction;
  std::optional<nlohmann::ordered_json> m_editingBindingDetails;
  nlohmann::ordered_json m_originalBindingCopy;
  int m_currentPressThreshold = 500;

  // For press-type swap conflict resolution.
  std::optional<std::pair<std::string, nlohmann::ordered_json>> m_pressTypeSwapConflict;
  std::optional<std::string> m_pressTypeSwapNewValue;

  std::string m_popupTitleKey;
  std::string m_pressTypeLabelKey;
  std::string m_behaviorLabelKey;
  std::string m_behaviorToggleKey;
  std::string m_behaviorHoldKey;
  std::string m_consumeLabelKey;
  std::string m_thresholdLabelKey;
  std::string m_closeButtonKey;

  std::string m_modeLabelKey;
  std::string m_modeAnalogKey;
  std::string m_modeDigitalKey;
  std::string m_deadzoneLabelKey;
  std::string m_saturationLabelKey;
  std::string m_sensitivityLabelKey;
  std::string m_curveLabelKey;
  std::string m_smoothingLabelKey;
  std::string m_sideLabelKey;
  std::string m_sideBothKey;
  std::string m_sidePositiveKey;
  std::string m_sideNegativeKey;
  std::string m_rangeMinLabelKey;
  std::string m_rangeMaxLabelKey;
  std::string m_accumulatorModeLabelKey;
  std::string m_invertLabelKey;

  std::string m_conflictPressTypeMessage;
  std::string m_conflictSwapQuestion;
  std::string m_conflictYesSwapButton;
  std::string m_conflictCancelButton;
};

}  // namespace UI
SPF_NS_END
