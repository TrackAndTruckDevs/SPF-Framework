#pragma once

#include <string>
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Events {
/**
 * @brief Event generated immediately before a plugin is loaded.
 * Listeners can use this event to prepare resources for the plugin.
 */
struct OnPluginWillBeLoaded {
  const std::string& pluginName;
};

/**
 * @brief Event generated immediately after a plugin has been successfully loaded
 * and its OnLoad() function has been called.
 */
struct OnPluginDidLoad {
  const std::string& pluginName;
};

/**
 * @brief Event generated immediately before a plugin is unloaded
 * and its resources are released.
 */
struct OnPluginWillBeUnloaded {
  const std::string& pluginName;
};

}  // namespace Events
SPF_NS_END
