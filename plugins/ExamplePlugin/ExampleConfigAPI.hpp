/**
 * @file ExampleConfigAPI.hpp
 * @brief Complete example of the SPF Config API — persistent plugin settings.
 *
 * @details The Config API is how your plugin stores and reads values that must survive
 * game restarts (user preferences, feature toggles, last-used state). The framework
 * owns the underlying file; you only get/set keys on your plugin's own context.
 *
 * DEVELOPER NOTE: If you only need to read/write a settings value, start here.
 * Nested JSON / custom contexts are covered in ExampleJsonAPI.
 */
#pragma once

#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Called once from OnActivated, after g_ctx.loadAPI / g_ctx.coreAPI are valid.
 * @details Reads the persisted value at startup and walks a nested object (ParseComplexObject).
 * WHY OnActivated and not OnLoad?
 * OnLoad provides SPF_Load_API (logging, paths, config surface for early reads).
 * Full Core API (config via coreAPI->config, json_reader) appears one lifecycle stage later.
 * Reading once here avoids re-parsing the same JSON subtree every frame.
 */
void ConfigAPI_OnActivated();

/**
 * @brief Framework export: fires when a config key changes (user UI, file edit, another plugin write).
 * @param config_handle Config context that changed (main plugin context or a custom one).
 * @param keyPath       Dotted path of the key, e.g. "settings.a_simple_number".
 *
 * @details WHY this lives in the Config module:
 * A developer looking for "how do I react to a setting change" should open this file and
 * see the callback, the keys it cares about, and how the UI writes those keys —
 * without hunting through the main plugin file.
 *
 * Wired from SPF_GetPlugin exports (ExamplePlugin.cpp), not a register call —
 * the framework stores the function pointer when the plugin is activated.
 * Signature must match SPF_Plugin_Exports::OnSettingChanged exactly.
 */
void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath);

/**
 * @brief Renders the Config slider section shown under the General tab.
 * @param ui UI API for widgets.
 *
 * @details Kept separate from RenderMainWindow so the tab bar stays a pure layout concern.
 * Custom-JSON / custom-context demos live in ExampleJsonAPI — this is only the
 * main plugin settings surface (get/set a value the user can tweak).
 */
void RenderConfigSection(SPF_UI_API* ui);

/**
 * @brief Reads settings.a_complex_object and logs its fields via the JsonReader API.
 * @details Demonstrates Cfg_GetJsonValueHandle + Json_HasMember / Json_GetMember /
 * Json_GetType / Json_GetString / Json_GetBool / Json_GetArraySize / Json_GetArrayItem.
 * Called from ConfigAPI_OnActivated and again from OnSettingChanged when that object changes —
 * re-parse on change instead of polling the file.
 */
void ParseComplexObject();

}  // namespace ExamplePlugin
