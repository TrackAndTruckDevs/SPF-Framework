/**
 * @file ExampleGeneralAPI.hpp
 * @brief Composition of the General tab — short demos that link out to full API modules.
 *
 * @details The General tab is an overview: one snippet per framework subsystem.
 * Each snippet lives in its owning Example*API module; this file only sequences them.
 *
 * DEVELOPER NOTE: For the full example of a subsystem, open the corresponding
 * Example*API file (e.g. ExampleConfigAPI for settings, ExampleKeybindsAPI for binds).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Renders the General tab by composing section helpers from each API module.
 * @details Order: welcome → config → console → keybind blocking → hooks.
 * Each section is independent — a missing subsystem shows its own "not available" text.
 */
void RenderGeneralTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
