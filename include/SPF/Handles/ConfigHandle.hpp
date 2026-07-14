#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Handles/IHandle.hpp"

#include <string>
#include <utility>


SPF_NS_BEGIN
namespace Handles {
/**
 * @brief A handle for the Config API.
 *
 * This handle holds the plugin name as context for the ConfigService.
 * It does not own any resources itself.
 */
struct ConfigHandle : IHandle {
  const std::string pluginName;

  ConfigHandle(std::string pluginName) : pluginName(std::move(pluginName)) {}
};
}  // namespace Handles
SPF_NS_END
