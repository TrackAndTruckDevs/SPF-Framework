/**
 * @file ExampleKeybindsAPI.hpp
 * @brief Complete example of the SPF KeyBinds API — static actions, dynamic actions, input blocking.
 *
 * @details Keybinds let players bind *your* actions to hardware the same way the game
 * binds its own controls. Two registration styles:
 *   - Static (Kbind_Register): action id known at activation; void(void) callback when bound key fires.
 *   - Dynamic (Kbind_RegisterActionMetadata): add/remove actions at runtime with title/desc
 *     shown in the framework's bind UI — for features that appear/disappear with context.
 *
 * Input blocking (Kbind_SetBlockState / UI_SetMouseBlockState) suppresses a *specific*
 * bound action or mouse look so your modal UI can capture input without the truck reacting.
 *
 * DEVELOPER NOTE: Start here for "how do I make my feature rebindable".
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Registers built-in actions and restores previously bound dynamic actions.
 * @details Called from OnActivated after coreAPI->keybinds is valid.
 * Restoring dynamic actions every activation is required: metadata is re-created by code
 * each session, but the player's *bindings* survive in the framework — re-register with the
 * same action id so existing binds attach again automatically (Kbind_Register_Ex for the
 * shared dynamic callback + user_data tag).
 */
void KeybindsAPI_OnActivated();

/**
 * @brief Callback for dynamic actions (registration + restore path).
 * @param action_id Stable id the player bound (e.g. "MyCustomAction").
 * @param user_data Opaque pointer passed at registration — used here to tag restore vs fresh.
 *
 * Signature must match SPF_Keybind_Callback_Ex:
 *   void (*)(const char* action_id, void* userData)
 */
void OnDynamicActionTriggered(const char* action_id, void* user_data);

/**
 * @brief Bound callback for "MainWindow.toggle" — show/hide the plugin window (default F5).
 * @details Separated from tab layout so the keybind layer does not depend on widgets.
 * Needs uiAPI + mainWindowHandle; both valid after OnRegisterUI. Kbind_Register requires
 * a plain void(void) function pointer — lambdas without captures also work (see Demo.honk).
 */
void OnToggleMainWindow();

/**
 * @brief Renders the "Dynamic Keybinds" tab (register/unregister metadata at runtime).
 */
void RenderDynamicKeybindsTab(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the input-blocking demos under the General tab.
 * @details Two levels:
 *   - Kbind_SetBlockState: suppress one bound action (e.g. game horn) while a checkbox is on.
 *   - UI_SetMouseBlockState: suppress game mouse-look so dragging your window doesn't turn the camera.
 * Both require the corresponding "Plugin Managed" policy in framework settings for full effect.
 */
void RenderKeybindBlockingSection(SPF_UI_API* ui);

}  // namespace ExamplePlugin
