/**
 * @file ExampleVirtInputAPI.hpp
 * @brief Complete example of the SPF VirtInput API — virtual gamepad + analog axis test.
 *
 * @details VirtInput injects synthetic device events (buttons, axes, POV hats) as if a
 * physical controller were pressed. Use for automation, accessibility remaps, or testing.
 *
 * DEVELOPER NOTE: Device must be created (OnLoad) before tab rendering can use it.
 * Axis output in Input Test is bound to KeyBinds actions (see ExampleKeybindsAPI).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Creates the virtual device (buttons/axes/hats) and registers it with the framework.
 * Called from OnLoad — requires g_ctx.loadAPI->virtinput.
 *
 * PATTERN: Create → AddButton/AddAxis/AddPOV → Register. Do not Register before all inputs exist.
 */
void VirtInputAPI_OnLoad();

/**
 * @brief Destroys/unregisters the virtual device on unload so the system stops accepting events.
 */
void VirtInputAPI_OnUnload();

/**
 * @brief Renders the Virtual Input tab: button presses, axis moves, POV hat.
 * @details Requires g_ctx.virtualDevice created in VirtInputAPI_OnLoad.
 */
void RenderVirtInputTab(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the Input Test tab: analog axis value sourced from KeyBinds actions.
 * @details Demonstrates reading Kbind axis state for remapping/debug UIs.
 */
void RenderInputTestTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
