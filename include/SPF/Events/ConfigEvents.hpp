#pragma once

namespace SPF::Events::Config {
/**
 * @brief Fired by ConfigService after keybindings have been modified
 *        to signal other services to reload the configuration.
 */
struct OnKeybindsModified {};
}  // namespace SPF::Events::Config
