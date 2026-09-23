/**
 * @file ExampleStylingAPI.hpp
 * @brief Complete example of the SPF Styling / UI assets API — styles, markdown, fonts, textures, transitions.
 *
 * @details The Styling tab is the showcase for Text styles, Markdown, notifications,
 * dynamic fonts, textures, and screen transitions. Asset loading happens once in
 * StylingAPI_OnActivated (not every frame).
 *
 * DEVELOPER NOTE: For custom fonts/textures or markdown blocks, start with
 * StyAPI_OnActivated (load) + RenderStylingTab (usage).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Loads demo textures (memory + file) and registers demo fonts. Called from OnActivated.
 * @details UI_CreateTextureFromMemory / UI_LoadFontFrom* may defer actual GPU/upload work
 * to the next frame — handles are available immediately, pixels ready shortly after.
 */
void StylingAPI_OnActivated();

/**
 * @brief Releases plugin textures/fonts on unload. Call only when the UI context is still alive.
 */
void StylingAPI_OnUnload();

/**
 * @brief Renders the Styling API tab (styles, icons, markdown, notifications, fonts, textures, transitions).
 */
void RenderStylingTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
