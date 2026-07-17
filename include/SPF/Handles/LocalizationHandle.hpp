#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Handles/IHandle.hpp"

#include <string>
#include <utility>


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
