/**
 * @file ExampleGameLogAPI.cpp
 * @brief Implementation of the SPF GameLog API example.
 *
 * @details REGISTRATION LIFECYCLE:
 *   OnActivated  → GLog_GetContext + GLog_RegisterCallback → store handle on g_ctx
 *   (runtime)    → OnGameLogMessage for each line
 *   OnUnload     → drop handles so the framework stops targeting a dying DLL
 *
 * Matching policy (substring filter) is an application choice — the API delivers lines;
 * you decide what is interesting. Keep filters in one place so behavior stays predictable.
 */
#include "ExampleGameLogAPI.hpp"

#include "SPF/SPF_API/SPF_GameLog_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstring>  // strstr — substring filter example

namespace ExamplePlugin {

// Last matching line kept for the UI section. Static (file-scope) presentational state
// does not need PluginContext — keep g_ctx for data shared across modules only.
static char s_lastGameLogMessage[512] = "(no matching game log messages yet)";

void GameLogAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->gamelog) {
    return;
  }

  // Context binds this plugin's name so unregisters and multi-plugin coexistence
  // do not collide. Always pair GLog_GetContext with the same PLUGIN_NAME string.
  SPF_GameLog_Handle* glog_h = g_ctx.coreAPI->gamelog->GLog_GetContext(PLUGIN_NAME);
  if (!glog_h) {
    return;
  }

  // Register: free/static callback matching SPF_GameLog_Callback_t + optional user pointer.
  // The framework keeps the function pointer for the life of the handle — do not let
  // the callback live in a DLL that can unload without unregistering first.
  g_ctx.gameLogCallbackHandle = g_ctx.coreAPI->gamelog->GLog_RegisterCallback(glog_h, OnGameLogMessage, nullptr);

  g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Registered game log callback.");
}

void GameLogAPI_OnUnload() {
  // Framework may auto-drop callbacks with the context; nulling our copies still matters
  // so a late event cannot reach partially-torn plugin state through a stale handle.
  g_ctx.gameLogCallbackHandle = nullptr;
}

void OnGameLogMessage(const char* log_line, void* user_data) {
  (void)user_data;  // Reserved for per-registration context; demo uses file-scope buffer.

  // Defensive: never assume non-null from a game/mod interop boundary.
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->logger || !log_line) {
    return;
  }

  // Cheap filter: only remember lines that look relevant. Prefer stable substrings the
  // game is known to emit — full sentences change with game updates and translations.
  if (strstr(log_line, "running")) {
    char buffer[4096];
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Game Log contains 'running': %s", log_line);
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, buffer);

    // Mirror a short form into the UI buffer — bounded copy, no heap, no lifetime issues.
    size_t i = 0;
    for (; log_line[i] && i + 1 < sizeof(s_lastGameLogMessage); ++i) {
      s_lastGameLogMessage[i] = log_line[i];
    }
    s_lastGameLogMessage[i] = '\0';
  }
}

void RenderGameLogSection(SPF_UI_API* ui) {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->gamelog) {
    ui->UI_Text("GameLog API is not available.");
    return;
  }

  ui->UI_Text("GameLog API — last matching in-game message:");
  ui->UI_TextWrapped(s_lastGameLogMessage);
  ui->UI_Separator();
}

}  // namespace ExamplePlugin
