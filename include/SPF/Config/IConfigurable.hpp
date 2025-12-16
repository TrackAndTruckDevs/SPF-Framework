#pragma once

#include "SPF/Namespace.hpp"
#include <nlohmann/json.hpp>
#include <string>

SPF_NS_BEGIN

namespace Config {
/**
 * @brief Interface for components that can have their settings
 *        reconfigured at runtime.
 */
struct IConfigurable {
  virtual ~IConfigurable() = default;

  /**
   * @brief Called when a setting relevant to this component has changed.
   *
   * The component should inspect the parameters and update its internal
   * state accordingly.
   *
   * @param systemName The name of the system the setting belongs to (e.g., "logging", "ui").
   * @param componentName The name of the component whose setting changed (e.g., "framework", "TestPlugin").
   * @param keyPath The path to the setting within the system (e.g., "level", "windows.main_window.is_visible").
   * @param newValue The new value of the setting.
   * @return True if the component handled the setting change, false otherwise.
   */
  virtual bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) = 0;
};

}  // namespace Config

SPF_NS_END
