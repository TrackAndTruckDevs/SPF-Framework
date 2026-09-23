/**
 * @file ExampleLocalizationAPI.hpp
 * @brief Complete example of the SPF Localization API — user-facing language changes.
 *
 * @details Use this module whenever UI text must follow the player's language choice.
 * The framework loads translation tables; you request strings by stable keys (not literals),
 * so adding a language never requires recompiling plugin logic that only passes keys around.
 *
 * DEVELOPER NOTE: For "react when the user switches language", see OnLanguageChanged below.
 * Loc_GetString always writes into YOUR buffer (int return = success/length semantics per header).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Framework export: called when the active interface language changes.
 * @param langCode New language code (e.g. "en", "uk"). May be nullptr — check it.
 *
 * @details The handler should:
 *   - Loc_HasLanguage for *this plugin* first; if we have no table for langCode, do nothing
 *     (stay on the current language instead of blanking all strings).
 *   - Loc_SetLanguage only when we actually have the translation.
 *   - Invalidate any UI that cached translated strings.
 *
 * Fresh Loc_GetString lookups after a successful SetLanguage pick up the new table;
 * *cached* pointers/strings do not — that is why this export exists.
 *
 * Wired from SPF_GetPlugin in ExamplePlugin.cpp (same pattern as OnSettingChanged).
 * Signature must match SPF_Plugin_Exports::OnLanguageChanged.
 */
void OnLanguageChanged(const char* langCode);

/**
 * @brief Resolves the General-tab welcome string into @p out_buffer.
 * @param out_buffer Destination buffer (caller owns lifetime — do not return internal pointers).
 * @param buffer_size Size of @p out_buffer in bytes.
 * @return Result of Loc_GetString (per API header: non-negative on success / 0 on failure —
 *         check the header for your framework version; always bound writes by buffer_size).
 *
 * @details API contract: the translation subsystem never returns a raw pointer you must free;
 * it copies into your buffer. That keeps ownership obvious across the C ABI boundary.
 */
int LocalizationAPI_GetWelcomeText(char* out_buffer, int buffer_size);

/**
 * @brief Renders the Localization portion of the General tab.
 */
void RenderLocalizationSection(SPF_UI_API* ui);

}  // namespace ExamplePlugin
