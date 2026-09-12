#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Modules/IBindableInput.hpp"

#include <string>

SPF_NS_BEGIN
namespace Config {
struct IConfigService;
}
namespace Modules {

/**
 * @brief Maps a physical input type to the FontAwesome glyph the native UI
 *        uses for it (keyboard/gamepad/mouse icon). Returns an empty string
 *        for chords and unknown types — a chord's icon is per-constituent,
 *        see GetDisplayNameWithIcon().
 */
const char* GetIconForInputType(InputType type);

/**
 * @brief Builds the exact "icon + name" display string the native Settings
 *        UI shows for a binding (e.g. the keyboard glyph followed by "W", or
 *        for a chord, one icon per constituent device joined with " + ").
 *        This is the single source of truth for that formatting so plugins,
 *        the Settings window, and the keybind capture popup never drift.
 */
std::string GetDisplayNameWithIcon(const IBindableInput& input);

/**
 * @brief Resolves a fully-qualified keybind action name (e.g. "MyPlugin.UI.toggle")
 *        to its translated display title, using the same manifest metadata (`_meta.titleKey`)
 *        lookup the native Settings UI uses. Falls back to the raw action name if no
 *        title is found.
 */
std::string GetTranslatedActionName(Config::IConfigService& configService, const std::string& fullActionName);

}  // namespace Modules
SPF_NS_END
