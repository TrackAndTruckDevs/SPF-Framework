#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Input/InputEvents.hpp"

#include "nlohmann/json_fwd.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

SPF_NS_BEGIN

namespace Events {
class EventManager;
}
namespace Config {
struct IConfigService;
}
namespace Modules {
class IBindableInput;
}
namespace Utils {
template <typename>
class Sink;
}

namespace UI {

/**
 * @brief Reusable "press a key" capture popup for (re)binding a single keybind action,
 *        including conflict detection and resolution.
 *
 * @details This used to be logic private to SettingsWindow. It has been extracted into
 *          its own component, owned as a single shared instance by UIManager, so that
 *          the exact same capture/conflict/commit pipeline can be triggered either from
 *          the native Settings window OR from a plugin's own custom ImGui menu (via the
 *          public SPF_KeyBinds_API's Kbind_OpenRebindPopup). Because both paths go through
 *          this one instance and write through the same RequestBindingUpdate event, the
 *          config, the runtime KeyBindsManager state, and the native Settings window all
 *          stay perfectly in sync no matter which UI initiated the rebind.
 */
class KeyCapturePopup {
 public:
  KeyCapturePopup(Events::EventManager& eventManager, Config::IConfigService& configService);

  void RefreshLocalization();

  /**
   * @brief Starts an input-capture session for the given action.
   * @param actionFullName Fully-qualified action name (e.g. "MyPlugin.UI.toggle").
   * @param originalBinding The existing binding JSON to reassign, or an empty object to add a new binding.
   */
  void Open(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding);

  bool IsOpen() const { return m_actionBeingEdited.has_value(); }

  /**
   * @brief Draws the popup, if a capture session is active. Must be called exactly once
   *        per frame; it is safe to call from any render location (it only ever draws
   *        anything while a session is open) and will appear on top of whichever window
   *        (native Settings, or a plugin's own window) triggered the session.
   */
  void Render();

  void OnInputCaptured(const Input::InputCaptured& e);
  void OnInputCaptureCancelled(const Input::InputCaptureCancelled& e);
  void OnInputCaptureUpdate(const Input::InputCaptureUpdate& e);
  void OnInputCaptureConflict(const Input::InputCaptureConflict& e);

 private:
  void RefreshLocalizationOnLanguageChange(const std::string& componentName);

  Events::EventManager& m_eventManager;
  Config::IConfigService& m_configService;

  // Unlike OnInputCaptured/Cancelled/Conflict (routed through UIManager by Core),
  // OnInputCaptureUpdate is subscribed to directly, same as the framework's other UI code.
  std::unique_ptr<Utils::Sink<void(const Input::InputCaptureUpdate&)>> m_onInputCaptureUpdateSink;
  std::unique_ptr<Utils::Sink<void(const std::string&)>> m_onLanguageChangedSink;

  // State for the active capture session
  std::optional<std::string> m_actionBeingEdited;
  std::vector<std::shared_ptr<Modules::IBindableInput>> m_currentChordInputs;
  nlohmann::ordered_json m_editingBindingObject;
  std::optional<Input::InputCaptured> m_bufferedInputInfo;
  std::optional<Input::InputCaptureConflict> m_conflictInfo;

  // Localization keys
  std::string m_popupTitleKey;
  std::string m_pressKeyTextKey;
  std::string m_deleteButtonKey;
  std::string m_cancelButtonKey;
  std::string m_conflictTitleKey;
  std::string m_conflictTextDetailedKey;
  std::string m_reassignShortPressButtonKey;
  std::string m_reassignLongPressButtonKey;
  std::string m_reassignPositiveSideButtonKey;
  std::string m_reassignNegativeSideButtonKey;
  std::string m_addShortPressButtonKey;
  std::string m_addLongPressButtonKey;
  std::string m_addPositiveSideButtonKey;
  std::string m_addNegativeSideButtonKey;
  std::string m_reassignEntireAxisButtonKey;
  std::string m_actionListFormatKey;

  std::string m_enumPressTypeShortKey;
  std::string m_enumPressTypeLongKey;
  std::string m_enumSideBothKey;
  std::string m_enumSidePositiveKey;
  std::string m_enumSideNegativeKey;
};

}  // namespace UI
SPF_NS_END
