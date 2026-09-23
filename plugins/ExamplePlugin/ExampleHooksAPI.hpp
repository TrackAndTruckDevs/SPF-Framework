/**
 * @file ExampleHooksAPI.hpp
 * @brief Complete example of the SPF Hooks API — intercepting native game functions.
 *
 * @details The Hooks API finds a game function by byte signature, redirects calls to your
 * detour, and gives you a trampoline to invoke the original. Use it when there is no
 * higher-level API for the behavior you need (string formatting, UI tweaks, etc.).
 *
 * DEVELOPER NOTE: Hooks are the lowest-level escape hatch — prefer dedicated APIs first.
 * Signature format is framework-specific pattern language (see Hook_Register docs).
 * The trampoline MUST be called from the detour or the game breaks.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Registers (and enables) the GameStringFormatting hook. Called from OnActivated.
 * @details Requires g_ctx.coreAPI->hooks. The trampoline is stored in g_ctx.o_GameStringFormatting
 * via Hook_Register's out-parameter so the detour can call the original.
 */
void HooksAPI_OnActivated();

/**
 * @brief Clears the trampoline pointer on unload so a late call cannot hit freed plugin memory.
 * @details Framework removes the hook itself; nulling our copy is defensive hygiene.
 */
void HooksAPI_OnUnload();

/**
 * @brief Detour installed in place of the game's string formatting function.
 * @param pOutput Opaque output buffer (same as original).
 * @param ppInput Pointer to the input key (e.g. localization token) — may be swapped.
 * @return Result of the original function via trampoline.
 *
 * @details CONTRACT: every exit path must call g_ctx.o_GameStringFormatting.
 * Returning without the trampoline skips the game's real formatter → broken UI/crash.
 */
void* Detour_GameStringFormatting(void* pOutput, const char** ppInput);

/**
 * @brief Renders the hook-toggle checkbox under the General tab.
 * @details Bound to g_ctx.isModificationActive — the detour reads this flag each call.
 */
void RenderHooksSection(SPF_UI_API* ui);

}  // namespace ExamplePlugin
