#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>  // For std::pair

SPF_NS_BEGIN
namespace Events::UI {
/**
 * @brief Fired by the WndProc proxy when the game window changes size.
 */
struct ResizeEvent {
  uint32_t width;
  uint32_t height;
};

/**
 * @brief Fired when a UI element requests to focus on a specific component's settings.
 */
struct FocusComponentInSettingsWindow {
  std::string componentName;
};

/**
 * @brief Fired when the user clicks the enable/disable toggle for a plugin.
 */
struct RequestPluginStateChange {
  std::string pluginName;
  bool enable;
};

/**
 * @brief Fired when a setting is changed in the UI and needs to be persisted.
 */
struct RequestSettingChange {
  std::string componentName;
  std::string keyPath;
  nlohmann::json newValue;
};

/**
 * @brief Fired by ConfigService after a setting has been successfully changed.
 */
struct OnSettingWasChanged {
  std::string systemName;
  std::string componentName;
  std::string keyPath;
  nlohmann::json newValue;
};

/**
 * @brief Fired from the UI to request the start of an input capture session.
 */
struct RequestInputCapture {
  std::string actionFullName;      // e.g., "framework.ui.main_window.toggle"
  nlohmann::json originalBinding;  // e.g., {"key": "KEY_DELETE", "type": "keyboard"}
};

/**
 * @brief Fired from the UI to request an update to a specific keybinding.
 */
struct RequestBindingUpdate {
  std::string actionFullName;
  nlohmann::json originalBinding;
  nlohmann::json newBinding;
  // Optional: if an input is reassigned, this holds the action and binding that should be cleared.
  std::optional<std::pair<std::string, nlohmann::json>> bindingToClear;
};

/**
 * @brief Fired from the UI to request the deletion of a specific binding.
 */
struct RequestDeleteBinding {
  std::string actionFullName;
  nlohmann::json bindingToDelete;
};

struct RequestExecuteCommand {
  std::string command;
};

/**
 * @brief Fired from the UI to request a change to a property of a binding (e.g., press_type).
 */
struct RequestBindingPropertyUpdate {
  std::string actionFullName;
  nlohmann::json originalBinding;
  std::string propertyName;
  nlohmann::json newValue;
};

/**
 * @brief Fired from the UI to request the cancellation of an input capture session.
 */
struct RequestInputCaptureCancel {};

/**
 * @brief Fired from the UI to request a check for framework updates.
 */
struct RequestUpdateCheck {};

/**
 * @brief Fired from the UI to request a fetch of the patrons list.
 */
struct RequestPatronsFetch {};
}  // namespace Events::UI
SPF_NS_END
