#pragma once
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Events::Config {
/**
 * @brief Fired by ConfigService after keybindings have been modified
 *        to signal other services to reload the configuration.
 */
struct OnKeybindsModified {};
}  // namespace Events::Config
SPF_NS_END
