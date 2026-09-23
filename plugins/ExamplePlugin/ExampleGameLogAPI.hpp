/**
 * @file ExampleGameLogAPI.hpp
 * @brief Complete example of the SPF GameLog API — reacting to in-game log lines.
 *
 * @details The game (and other mods) emit log lines (job offers, damage, chat, …).
 * Registering a callback lets your plugin observe those lines without polling
 * or reading log files from disk.
 *
 * DEVELOPER NOTE: GameLog is *observational*. It does not replace Telemetry events
 * for numeric state — prefer Telemetry for positions/speeds, GameLog for textual
 * narrative the game already writes (and that has no dedicated event yet).
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Registers the game-log callback. Called from OnActivated once coreAPI exists.
 * @details GLog_GetContext(PLUGIN_NAME) groups this plugin's subscriptions; the returned
 * SPF_GameLog_Callback_Handle* is stored on g_ctx so unload can drop the association.
 * Handle lifetime is framework-owned; null it on unload (see GameLogAPI_OnUnload).
 */
void GameLogAPI_OnActivated();

/**
 * @brief Clears GameLog handles on plugin unload so late lines cannot call into a dying DLL.
 */
void GameLogAPI_OnUnload();

/**
 * @brief Callback invoked for each new game log line (see SPF_GameLog_Callback_t).
 * @param log_line The content of the log line (UTF-8). Treat as read-only and short-lived —
 *                 copy before storing if you need it after the call returns.
 * @param user_data Opaque pointer from registration (unused in this demo; useful for
 *                  binding a specific filter or ring buffer without globals).
 *
 * @details WHY keep this handler cheap:
 * The game can burst many lines. Heavy parsing here can hitch the emitter.
 * Enqueue work for OnUpdate if you need to parse off this path.
 *
 * Signature must match SPF_GameLog_Callback_t exactly:
 *   void (*)(const char* message, void* userData)
 */
void OnGameLogMessage(const char* log_line, void* user_data);

/**
 * @brief Renders the GameLog demo section (last matching message) under the General tab.
 */
void RenderGameLogSection(SPF_UI_API* ui);

}  // namespace ExamplePlugin
