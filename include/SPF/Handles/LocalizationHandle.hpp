#pragma once

#include <string>

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Handles {
/**
 * @brief A handle for the Localization API.
 *
 * Stores the plugin name as context for localization lookups.
 */
struct LocalizationHandle : IHandle {
  const std::string pluginName;

  LocalizationHandle(std::string pluginName) : pluginName(std::move(pluginName)) {}
};
}  // namespace Handles
SPF_NS_END
