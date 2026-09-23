/**
 * @file ExampleCameraAPI.hpp
 * @brief Complete example of the SPF Camera API — reading/switching cameras + keybind.
 *
 * @details The Camera API exposes the game's camera modes (interior, chase, developer free)
 * and world coordinates. Pair it with KeyBinds to offer player-facing shortcuts.
 *
 * DEVELOPER NOTE: For "switch to interior on hotkey", see OnCameraKeybind below.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Registers the "Camera.cycle" static keybind. Called from OnActivated after KeybindsAPI.
 * @details KeybindsAPI_OnActivated must run first so g_ctx.keybindsHandle is valid.
 * Manifest default for Camera.cycle is KEY_F6 (see BuildManifest Defaults_AddKeybind).
 */
void CameraAPI_OnActivated();

/**
 * @brief Bound callback for the "Camera.cycle" action (default F6).
 * @details Cycles Interior → Behind → Developer Free → Interior.
 * Kbind_Register requires void(void) — no parameters.
 */
void OnCameraKeybind();

/**
 * @brief Renders the Camera tab: current mode, switch buttons, world position.
 */
void RenderCameraTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
