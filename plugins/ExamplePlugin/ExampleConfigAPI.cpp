/**
 * @file ExampleConfigAPI.cpp
 * @brief Implementation of the SPF Config API example — reads, writes, change callback.
 *
 * @details FLOW FOR A TYPICAL "save my setting" FEATURE:
 *   1. OnActivated: read the persisted value once so in-memory state matches last session.
 *   2. UI: Cfg_GetInt32 to show current value; Cfg_SetInt32 when the user changes it.
 *   3. OnSettingChanged: optional — react when *anything* writes that key (settings UI,
 *      hot-reload, external edit) without polling.
 *
 * Keys are namespaced by the framework under your plugin (PLUGIN_NAME).
 * Prefer explicit dotted paths ("settings.foo"): keys are forever once users have
 * saved data against them.
 */
#include "ExampleConfigAPI.hpp"

#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/SPF_API/SPF_JsonReader_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstring>  // strcmp — match key paths in OnSettingChanged

namespace ExamplePlugin {

// ------------------------------------------------------------------------------------------------
// Startup: pull persisted state into memory
// ------------------------------------------------------------------------------------------------

void ConfigAPI_OnActivated() {
  if (!g_ctx.loadAPI || !g_ctx.loadAPI->config) {
    return;
  }

  // Reading a primitive is a single call with a first-run default (not written back
  // automatically — only Cfg_SetInt32 when the user deliberately changes the value).
  auto config = g_ctx.loadAPI->config;
  g_ctx.someNumber = config->Cfg_GetInt32(config->Cfg_GetContext(PLUGIN_NAME), "settings.a_simple_number", 42);

  // Nested object: read once at activation, not every frame — avoids repeated path walks.
  ParseComplexObject();
}

// ------------------------------------------------------------------------------------------------
// Change notification (framework export)
// ------------------------------------------------------------------------------------------------

void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
  if (!g_ctx.loadAPI || !keyPath) {
    return;
  }
  auto logger = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);

  char log_buffer[256];
  g_ctx.loadAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "[Config] Configuration changed! Handle: %p, Key: %s", (void*)config_handle, keyPath);
  g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

  // Match only keys this module owns. Updating cache here means UI and logic always
  // see the same value without re-reading the file on every widget draw.
  if (strcmp(keyPath, "settings.a_simple_number") == 0) {
    g_ctx.someNumber = g_ctx.loadAPI->config->Cfg_GetInt32(config_handle, "settings.a_simple_number", 42);

    char buf[256];
    g_ctx.loadAPI->formatting->Fmt_Format(buf, sizeof(buf), "'a_simple_number' was changed externally. New value: %d", g_ctx.someNumber);
    g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, buf);
  } else if (strcmp(keyPath, "settings.a_complex_object") == 0) {
    // Object subtree changed → re-parse once here instead of every frame in Render.
    ParseComplexObject();
  }
}

// ------------------------------------------------------------------------------------------------
// UI: one slider that round-trips through the config file
// ------------------------------------------------------------------------------------------------

void RenderConfigSection(SPF_UI_API* ui) {
  if (!g_ctx.loadAPI || !g_ctx.loadAPI->config) {
    ui->UI_Text("Config API is not available.");
    return;
  }

  auto config = g_ctx.loadAPI->config;
  SPF_Config_Handle* h = config->Cfg_GetContext(PLUGIN_NAME);

  ui->UI_Text("This slider modifies a value in settings.json.");

  // Pattern: read-modify-write on the UI thread.
  // Get current value into a local/ctx field, let the slider mutate it, and only when
  // the slider returns true (user actually moved it) push back to config.
  // Writing every frame would churn the settings file and waste change callbacks.
  if (ui->UI_SliderInt("Some Number", &g_ctx.someNumber, 0, 100, "%d", SPF_SLIDER_FLAG_NONE)) {
    config->Cfg_SetInt32(h, "settings.a_simple_number", g_ctx.someNumber);
    g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "User changed 'a_simple_number' via UI.");
  }

  ui->UI_Separator();
}

// ------------------------------------------------------------------------------------------------
// Nested object walk — educational helper
// ------------------------------------------------------------------------------------------------

void ParseComplexObject() {
  // JsonReader rides along on the Core API; config is where the value handle comes from.
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->config || !g_ctx.coreAPI->json_reader) {
    return;
  }

  auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);
  auto config = g_ctx.coreAPI->config;
  auto config_handle = config->Cfg_GetContext(PLUGIN_NAME);
  const auto* json_reader = g_ctx.coreAPI->json_reader;

  char log_buffer[512];

  // 1. Handle to a nested JSON object inside the main settings file.
  //    The path must match what Settings_SetJson declared in BuildManifest (under "settings.").
  const SPF_JsonValue_Handle* object_h = config->Cfg_GetJsonValueHandle(config_handle, "settings.a_complex_object");

  if (object_h) {
    g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Parsing complex object 'settings.a_complex_object':");
    g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

    // 2. Optional member: check presence + type before get — user-edited JSON may omit fields.
    if (json_reader->Json_HasMember(object_h, "mode")) {
      const SPF_JsonValue_Handle* mode_h = json_reader->Json_GetMember(object_h, "mode");
      if (mode_h && json_reader->Json_GetType(mode_h) == SPF_JSON_TYPE_STRING) {
        char mode_str[64];
        json_reader->Json_GetString(mode_h, mode_str, sizeof(mode_str));
        g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "  -> Mode: %s", mode_str);
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
      }
    }

    // 3. Boolean member with an explicit default if missing.
    const SPF_JsonValue_Handle* enabled_h = json_reader->Json_GetMember(object_h, "enabled");
    if (enabled_h && json_reader->Json_GetType(enabled_h) == SPF_JSON_TYPE_BOOLEAN) {
      bool enabled_val = json_reader->Json_GetBool(enabled_h, false);
      g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "  -> Enabled: %s", enabled_val ? "true" : "false");
      g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
    }

    // 4. Array: size first, then index — each item typed-checked the same way as scalars.
    const SPF_JsonValue_Handle* targets_h = json_reader->Json_GetMember(object_h, "targets");
    if (targets_h && json_reader->Json_GetType(targets_h) == SPF_JSON_TYPE_ARRAY) {
      int array_size = json_reader->Json_GetArraySize(targets_h);
      g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "  -> Found 'targets' array with %d elements:", array_size);
      g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

      for (int i = 0; i < array_size; ++i) {
        const SPF_JsonValue_Handle* item_h = json_reader->Json_GetArrayItem(targets_h, i);
        if (item_h && json_reader->Json_GetType(item_h) == SPF_JSON_TYPE_STRING) {
          char item_str[64];
          json_reader->Json_GetString(item_h, item_str, sizeof(item_str));
          g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "    - Target[%d]: %s", i, item_str);
          g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
        }
      }
    }
  } else {
    g_ctx.coreAPI->logger->Log(logger, SPF_LOG_WARN, "Failed to get handle for 'settings.a_complex_object'.");
  }
}

}  // namespace ExamplePlugin
