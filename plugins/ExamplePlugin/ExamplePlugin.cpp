/**
 * @file ExamplePlugin.cpp
 * @brief Thin lifecycle orchestrator: BuildManifest, OnLoad/OnActivated/OnUpdate/OnUnload,
 *        OnRegisterUI, RenderMainWindow (tab bar), SPF_GetManifestAPI / SPF_GetPlugin exports.
 *
 * @details Per-API logic lives in Example*API.*. This file must stay small:
 * it only sequences module lifecycle hooks in dependency order and wires framework
 * exports to the owning module's callback symbols.
 *
 * LIFECYCLE ORDER (do not reorder casually):
 *   OnLoad:        loadAPI → Environment → VirtInput (early, before game input init)
 *   OnActivated:   coreAPI cache → Config → Keybinds (handle) → Camera (uses handle)
 *                  → GameLog → Hooks → Styling → Telemetry
 *   OnUpdate:      Climate → Sound → Telemetry housekeeping
 *   OnUnload:      modules (reverse-ish) → null core/load pointers
 */
#include "ExamplePlugin.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_Manifest_API.h"
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExampleCameraAPI.hpp"
#include "ExampleClimateAPI.hpp"
#include "ExampleConfigAPI.hpp"
#include "ExampleEnvironmentAPI.hpp"
#include "ExampleGameLogAPI.hpp"
#include "ExampleGameWorldAPI.hpp"
#include "ExampleGeneralAPI.hpp"
#include "ExampleHooksAPI.hpp"
#include "ExampleJsonAPI.hpp"
#include "ExampleKeybindsAPI.hpp"
#include "ExampleLocalizationAPI.hpp"
#include "ExampleSoundAPI.hpp"
#include "ExampleStylingAPI.hpp"
#include "ExampleTelemetryAPI.hpp"
#include "ExampleVehicleAPI.hpp"
#include "ExampleVirtInputAPI.hpp"

namespace ExamplePlugin {

// =================================================================================================
// Shared state (single global — declared extern in ExamplePlugin.hpp)
// =================================================================================================

PluginContext g_ctx;

// =================================================================================================
// Manifest
// =================================================================================================

void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
  // Identity — PLUGIN_NAME / PLUGIN_VERSION / PLUGIN_AUTHOR come from CMake target_compile_definitions.
  {
    api->Info_SetName(h, PLUGIN_NAME);
    api->Info_SetVersion(h, PLUGIN_VERSION);
    // Minimum framework version: plugins with older APIs are disabled with a user-visible warning.
    api->Info_SetMinFrameworkVersion(h, PLUGIN_VERSION);
    api->Info_SetAuthor(h, PLUGIN_AUTHOR);

    api->Info_SetEmail(h, "mailto:your.email@example.com");
    api->Info_SetDiscordUrl(h, "discordUrl");
    api->Info_SetSteamProfileUrl(h, "steamProfileUrl");
    api->Info_SetGithubUrl(h, "githubUrl");
    api->Info_SetYoutubeUrl(h, "youtubeUrl");
    api->Info_SetScsForumUrl(h, "scsForumUrl");
    api->Info_SetPatreonUrl(h, "patreonUrl");
    api->Info_SetWebsiteUrl(h, "websiteUrl");

    api->Info_SetDescriptionKey(h, "");
    api->Info_SetDescriptionLiteral(h, "A template plugin to demonstrate the SPF API.");
  }

  // Configuration policy — user-editable systems + hooks the framework must keep on for us.
  {
    api->Policy_SetAllowUserConfig(h, true);
    api->Policy_AddConfigurableSystem(h, "settings");
    api->Policy_AddConfigurableSystem(h, "logging");
    api->Policy_AddConfigurableSystem(h, "localization");
    api->Policy_AddConfigurableSystem(h, "ui");
    // Required hooks: GameConsole (ExampleConsoleAPI), GameLogHook (ExampleGameLogAPI).
    api->Policy_AddRequiredHook(h, "GameConsole");
    api->Policy_AddRequiredHook(h, "GameLogHook");
  }

  // Default settings.json content (under top-level "settings." when allowUserConfig).
  api->Settings_SetJson(h, R"json({
        "a_simple_number": 42,
        "a_slider_number": 50.5,
        "a_drag_number": 10,
        "a_dropdown_choice": "option_b",
        "a_radio_choice": 2,
        "a_color": [0.2, 0.8, 0.4],
        "a_text_note": "This is some default text.\nIt can span multiple lines.",
        "a_complex_object": { "mode": "alpha", "enabled": true, "targets": ["a", "b", "c"] },
        "a_float_input": 123.45,
        "a_double_input": 12345.6789,
        "a_vslider_float": 0.5,
        "a_hinted_input": "",
        "a_log_slider": 10.0
    })json");

  // Framework system defaults for this plugin.
  api->Defaults_SetLogging(h, "info", true);
  api->Defaults_SetLocalization(h, "en");

  // Keybind defaults — group "Action" style: group + action → "Group.action".
  {
    api->Defaults_AddKeybind(h, "MainWindow", "toggle", "keyboard", "KEY_F5", "always");
    api->Defaults_AddKeybind(h, "MainWindow", "toggle", "chord", "keyboard:KEY_LCONTROL+keyboard:KEY_F5", "always");
    api->Defaults_AddKeybind(h, "Camera", "cycle", "keyboard", "KEY_F6", "always");
    // Demo.honk: manual consume policy so ExampleKeybindsAPI can demonstrate blocking.
    api->Defaults_AddKeybind(h, "Demo", "honk", "keyboard", "KEY_H", "manual");
    // Analog test axis: keyboard space OR gamepad trigger (ExampleVirtInputAPI Input Test).
    api->Defaults_AddKeybind(h, "Test", "Axis", "keyboard", "KEY_SPACE", "never");
    api->Defaults_AddKeybind(h, "Test", "Axis", "gamepad_axis", "RIGHT_TRIGGER_AXIS", "never");
  }

  // Window defaults for MainWindow (name must match OnRegisterUI registration).
  {
    api->Defaults_AddWindow(h, "MainWindow", true, true, 100, 100, 400, 300, false, false);
  }

  // Setting widget metadata — keys must exist under Settings_SetJson "settings.".
  {
    api->Meta_AddCustomSetting(h, "a_simple_number", "setting.simple_number.title", "setting.simple_number.description", nullptr, nullptr, false);
    api->Meta_AddCustomSetting(h, "a_slider_number", "setting.slider_number.title", "setting.slider_number.description", "slider", "{ \"min\": 0.0, \"max\": 100.0, \"format\": \"%.1f %%\" }", false);
    api->Meta_AddCustomSetting(h, "a_drag_number", "setting.drag_number.title", "setting.drag_number.description", "drag", "{ \"speed\": 0.5, \"min\": -100.0, \"max\": 100.0, \"format\": \"%d units\" }", false);

    const char* combo_options = R"json({ "options": [
        { "value": "option_a", "labelKey": "options.a.title" },
        { "value": "option_b", "labelKey": "options.b.title" },
        { "value": "option_c", "labelKey": "This is a literal label" }
    ]})json";
    api->Meta_AddCustomSetting(h, "a_dropdown_choice", "setting.dropdown.title", "setting.dropdown.description", "combo", combo_options, false);

    const char* radio_options = R"json({ "options": [
        { "value": 1, "labelKey": "options.radio_one" },
        { "value": 2, "labelKey": "options.radio_two" },
        { "value": 3, "labelKey": "options.radio_three" }
    ]})json";
    api->Meta_AddCustomSetting(h, "a_radio_choice", "setting.radio.title", "setting.radio.description", "radio", radio_options, false);

    api->Meta_AddCustomSetting(h, "a_color", "setting.color.title", "setting.color.description", "color3", "{ \"flags\": 0 }", false);
    api->Meta_AddCustomSetting(h, "a_text_note", "setting.note.title", "setting.note.description", "multiline", "{ \"height_in_lines\": 4 }", false);
    api->Meta_AddCustomSetting(h, "a_complex_object", "setting.complex_object.title", "setting.complex_object.description", nullptr, nullptr, true);
    api->Meta_AddCustomSetting(h, "a_float_input", "setting.float_input.title", "setting.float_input.description", nullptr, nullptr, false);
    api->Meta_AddCustomSetting(h, "a_double_input", "setting.double_input.title", "setting.double_input.description", "input_double", "{ \"step\": 0.005, \"format\": \"%.4f\" }", false);
    api->Meta_AddCustomSetting(h, "a_vslider_float", "setting.vslider_float.title", "setting.vslider_float.description", "vslider", "{ \"min\": -1.0, \"max\": 1.0, \"width\": 30.0, \"height\": 100.0, \"format\": \"%.2f\" }", false);
    api->Meta_AddCustomSetting(h, "a_hinted_input", "setting.hinted_input.title", "setting.hinted_input.description", "input_with_hint", "{ \"hint\": \"Enter your username\" }", false);
    api->Meta_AddCustomSetting(h, "a_log_slider", "setting.log_slider.title", "setting.log_slider.description", "slider", "{ \"min\": 0.1, \"max\": 1000.0, \"is_logarithmic\": true }", false);
  }

  // Keybind + window localization keys (titles/tooltips in the framework settings UI).
  {
    api->Meta_AddKeybind(h, "MainWindow", "toggle", "keybind.main_window_toggle.title", "keybind.main_window_toggle.description");
    api->Meta_AddKeybind(h, "Camera", "cycle", "keybind.camera_cycle.title", "keybind.camera_cycle.description");
    api->Meta_AddWindow(h, "MainWindow", "ui.window.main_window.title", "ui.window.main_window.description");
  }
}

// =================================================================================================
// Lifecycle
// =================================================================================================

void OnLoad(const SPF_Load_API* load_api) {
  g_ctx.loadAPI = load_api;
  if (!g_ctx.loadAPI || !g_ctx.loadAPI->logger || !g_ctx.loadAPI->config || !g_ctx.loadAPI->input) {
    return;
  }

  // Early service pointers available at load stage.
  g_ctx.jsonWriterAPI = g_ctx.loadAPI->json_writer;
  g_ctx.jsonIOAPI = g_ctx.loadAPI->json_io;

  // Environment first: other modules may need Env_GetPluginDataDir (Sound bank path, JSON paths).
  EnvironmentAPI_OnLoad();

  // VirtInput before the game finishes its own input init so the virtual device is enumerable.
  VirtInputAPI_OnLoad();

  auto logger = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);
  g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, "ExamplePlugin has been loaded!");
}

void OnActivated(const SPF_Core_API* core_api) {
  g_ctx.coreAPI = core_api;
  if (!g_ctx.coreAPI) {
    return;
  }

  // Cache service pointers every module reads from g_ctx.
  g_ctx.vehicleAPI = g_ctx.coreAPI->vehicle;
  g_ctx.gameworldAPI = g_ctx.coreAPI->gameworld;
  g_ctx.climateAPI = g_ctx.coreAPI->climate;
  g_ctx.environmentAPI = g_ctx.coreAPI->environment;
  g_ctx.uiAPI = g_ctx.coreAPI->ui;
  g_ctx.soundAPI = g_ctx.coreAPI->sound;
  g_ctx.jsonWriterAPI = g_ctx.coreAPI->json_writer;
  g_ctx.jsonIOAPI = g_ctx.coreAPI->json_io;

  // Module activation — ORDER MATTERS:
  ConfigAPI_OnActivated();     // reads settings + ParseComplexObject
  KeybindsAPI_OnActivated();   // creates keybindsHandle, registers statics, restores dynamics
  CameraAPI_OnActivated();     // registers Camera.cycle (needs keybindsHandle from above)
  GameLogAPI_OnActivated();    // GLog_RegisterCallback
  HooksAPI_OnActivated();      // Hook_Register GameStringFormatting
  StylingAPI_OnActivated();    // load textures + fonts once
  TelemetryAPI_OnActivated();  // parent handle + all Tel_RegisterFor*
}

void OnGameWorldReady() {
  if (g_ctx.coreAPI && g_ctx.coreAPI->logger) {
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "OnGameWorldReady called! Game world is loaded and ready.");
  }
}

void OnUpdate() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->logger || !g_ctx.coreAPI->formatting) {
    return;
  }

  // Module per-frame work — each early-outs when its feature is off / not ready.
  ClimateAPI_OnUpdate();    // weather auto-toggle (~2s frame-count delay)
  SoundAPI_OnUpdate();      // horn→bell detection + playback only while replaceHornEnabled
  TelemetryAPI_OnUpdate();  // optional throttled diagnostics (no-op by default)
}

void OnUnload() {
  if (g_ctx.loadAPI && g_ctx.loadAPI->logger) {
    g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "ExamplePlugin is being unloaded.");
  }

  // Module teardown before nulling shared API pointers (modules may still need uiAPI/logger).
  SoundAPI_Shutdown();        // stop/release bell, unload bank
  StylingAPI_OnUnload();      // destroy textures (UI context must still be alive)
  TelemetryAPI_OnUnload();    // drop parent + callback handle references
  HooksAPI_OnUnload();        // null trampoline
  GameLogAPI_OnUnload();      // null gameLogCallbackHandle
  JsonAPI_OnUnload();         // destroy customConfigHandle if open
  VirtInputAPI_OnUnload();    // destroy virtual device
  EnvironmentAPI_OnUnload();  // null environmentHandle

  // Vehicle selection state (handles die with world/unload).
  g_ctx.selectedVehicle = nullptr;
  g_ctx.vehicleHandles.clear();
  g_ctx.keybindsHandle = nullptr;
  g_ctx.mainWindowHandle = nullptr;
  g_ctx.telemetryHandle = nullptr;

  // Null primary API pointers last — use-after-free guard for any late callback.
  g_ctx.uiAPI = nullptr;
  g_ctx.vehicleAPI = nullptr;
  g_ctx.jsonWriterAPI = nullptr;
  g_ctx.jsonIOAPI = nullptr;
  g_ctx.coreAPI = nullptr;
  g_ctx.loadAPI = nullptr;
}

// =================================================================================================
// UI entry + tab bar
// =================================================================================================

void OnRegisterUI(SPF_UI_API* ui_api) {
  if (!ui_api) {
    return;
  }
  g_ctx.uiAPI = ui_api;
  // Draw callback: framework calls RenderMainWindow while MainWindow is visible.
  // Handle cached for KeybindsAPI OnToggleMainWindow (UI_IsVisible / UI_SetVisibility).
  ui_api->UI_RegisterDrawCallback(PLUGIN_NAME, "MainWindow", RenderMainWindow, nullptr);
  g_ctx.mainWindowHandle = g_ctx.uiAPI->UI_GetWindowHandle(PLUGIN_NAME, "MainWindow");
}

void RenderMainWindow(SPF_UI_API* ui, void* user_data) {
  // Window Begin/End is framework-owned; this only draws tab content.
  // Tab titles are localization keys via "{Window}.tab.{name}" (see localization files).
  if (!ui->UI_BeginTabBar("##MainWindowTabs", SPF_TAB_BAR_FLAG_NONE)) {
    return;
  }

  if (ui->UI_BeginTabItem("General", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderGeneralTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Traffic Inspector", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderVehicleTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Game World", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderGameWorldTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Camera", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderCameraTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Climate", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderClimateTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Telemetry", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderTelemetryTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Events", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderEventsTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Virtual Input", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderVirtInputTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Styling API", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderStylingTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Environment", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderEnvironmentTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Input Test", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderInputTestTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Dynamic Keybinds", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderDynamicKeybindsTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Custom JSON", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderCustomJsonTab(ui, user_data);
    ui->UI_EndTabItem();
  }
  if (ui->UI_BeginTabItem("Sound", nullptr, SPF_TAB_ITEM_FLAG_NONE)) {
    RenderSoundTab(ui, user_data);
    ui->UI_EndTabItem();
  }

  ui->UI_EndTabBar();
}

}  // namespace ExamplePlugin

// =================================================================================================
// Plugin exports (extern "C" — no name mangling; framework resolves these by symbol)
// =================================================================================================

extern "C" {

/**
 * @brief Framework reads BuildManifest before fully loading the DLL.
 * @param out_api Filled with the plugin's BuildManifest function pointer.
 * @return true on success.
 */
SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
  if (out_api) {
    out_api->BuildManifest = ExamplePlugin::BuildManifest;
    return true;
  }
  return false;
}

/**
 * @brief Framework fetches lifecycle + optional callback pointers after the manifest.
 * @param exports Filled with OnLoad/OnActivated/OnUpdate/OnUnload and optional hooks.
 * @return true on success.
 *
 * @details Callbacks are owned by their Example*API modules — included above so we can
 * assign the real symbols without re-implementing them here:
 *   OnSettingChanged   → ExampleConfigAPI
 *   OnLanguageChanged  → ExampleLocalizationAPI
 */
SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports) {
  if (!exports) {
    return false;
  }

  exports->OnLoad = ExamplePlugin::OnLoad;
  exports->OnActivated = ExamplePlugin::OnActivated;
  exports->OnUnload = ExamplePlugin::OnUnload;
  exports->OnUpdate = ExamplePlugin::OnUpdate;
  // Optional: one-shot when the player loads into the game world (world-dependent setup).
  exports->OnGameWorldReady = ExamplePlugin::OnGameWorldReady;
  exports->OnRegisterUI = ExamplePlugin::OnRegisterUI;
  exports->OnSettingChanged = ExamplePlugin::OnSettingChanged;
  exports->OnLanguageChanged = ExamplePlugin::OnLanguageChanged;
  return true;
}

}  // extern "C"
