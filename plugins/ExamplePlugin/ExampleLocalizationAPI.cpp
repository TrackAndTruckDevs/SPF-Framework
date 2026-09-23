/**
 * @file ExampleLocalizationAPI.cpp
 * @brief Implementation of the SPF Localization API example.
 *
 * @details PATTERN SUMMARY:
 *   - Store translation KEYS in code; resolve at draw time into a stack buffer.
 *   - OnLanguageChanged: switch the plugin's active language when the interface language
 *     changes *and* we have a matching table.
 *   - Never concatenate pre-translated fragments if word order differs by language —
 *     pass placeholders into one key and let the translation supply the template.
 */
#include "ExampleLocalizationAPI.hpp"

#include "SPF/SPF_API/SPF_Localization_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

namespace ExamplePlugin {

void OnLanguageChanged(const char* langCode) {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->localization || !langCode) {
    return;
  }

  SPF_Localization_Handle* h = g_ctx.coreAPI->localization->Loc_GetContext(PLUGIN_NAME);

  // Check if the plugin actually has a translation for the new language.
  // If it doesn't, do NOTHING (stay on the current language) — a failed SetLanguage
  // would leave the UI half-switched or empty.
  if (g_ctx.coreAPI->localization->Loc_HasLanguage(h, langCode)) {
    if (g_ctx.coreAPI->localization->Loc_SetLanguage(h, langCode)) {
      char log_buffer[256];
      g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Plugin language synchronized to: %s", langCode);
      g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    }
  }
}

int LocalizationAPI_GetWelcomeText(char* out_buffer, int buffer_size) {
  if (!out_buffer || buffer_size <= 0) {
    return 0;
  }

  if (!g_ctx.loadAPI || !g_ctx.loadAPI->localization) {
    // Fail-open: stable English fallback rather than an empty window if the API
    // is missing (very early frame or headless harness).
    static const char kFallback[] = "Hello from the ExamplePlugin window!";
    int i = 0;
    for (; kFallback[i] && i + 1 < buffer_size; ++i) {
      out_buffer[i] = kFallback[i];
    }
    out_buffer[i] = '\0';
    return 1;
  }

  auto loc = g_ctx.loadAPI->localization;
  // Key must match an entry in the plugin's localization resources (localization/ folder).
  // A human sentence as the key works for demos but is awkward to reuse — prefer
  // dotted keys like "messages.welcome" in real plugins.
  return loc->Loc_GetString(loc->Loc_GetContext(PLUGIN_NAME), "messages.welcome", out_buffer, buffer_size);
}

void RenderLocalizationSection(SPF_UI_API* ui) {
  // Resolve once per frame into a stack buffer — language can change underneath us,
  // so do not cache the translated text across frames without listening to OnLanguageChanged.
  char welcome_msg[256];
  LocalizationAPI_GetWelcomeText(welcome_msg, sizeof(welcome_msg));
  ui->UI_Text(welcome_msg);

  // Separating "resolve string" from "draw widget" keeps the render function free of
  // language branching — the same Draw call works for every locale.
  ui->UI_Separator();
}

}  // namespace ExamplePlugin
