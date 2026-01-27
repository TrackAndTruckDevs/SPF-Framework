/**
 * @file MyPlugin.cpp
 * @brief The main implementation file for the MyPlugin.
 * @details This file contains the minimal implementation for a plugin to be loaded
 * and recognized by the SPF framework. It serves as a basic template for new plugins,
 * with clear explanations and commented-out sections for optional features.
 */

#include "MyPlugin.hpp" // Always include your own header first
#include <cstring>      // For C-style string manipulation functions like strncpy_s.

namespace MyPlugin {

// =================================================================================================
// 1. Constants & Global State
// =================================================================================================

/**
 * @brief A constant for the plugin's name.
 * @details This MUST match the name used in `Cfg_GetContext` calls for various APIs
 * and the plugin's directory name.
 */
const char* PLUGIN_NAME = "MyPlugin";

/**
 * @brief The single, global instance of the plugin's context.
 * @details This is the central point for accessing all plugin state.
 */
PluginContext g_ctx;

// =================================================================================================
// 2. Manifest Implementation
// =================================================================================================

void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // This function defines all the metadata for your plugin. The framework calls this
    // function *before* loading your plugin DLL to understand what it is.

    // --- 2.1. Plugin Information ---
    // This section provides the basic identity of your plugin.
    {
        // `name`: (Optional) A unique name for the plugin (e.g., "MyPlugin").
        // If not specified, the framework will use the name of your DLL file, but specifying it
        // here is recommended to avoid potential conflicts.
        api->Info_SetName(h, PLUGIN_NAME);

        // `version`: (Optional) The plugin's version string (e.g., "1.0.0").
        api->Info_SetVersion(h, "0.1.0");

        // Recommended to fill in
        // The minimum SPF Framework version required for this plugin to work correctly (e.g. "1.0.0").
        // If the user's framework version is lower than this, the plugin will be disabled. And a warning will be shown
        // This prevents crashes due to API changes.
        api->Info_SetMinFrameworkVersion(h, "1.0.0");

        // `author`: (Optional) The name of the author or organization.
        api->Info_SetAuthor(h, "Your Name/Organization");

        // `descriptionLiteral`: (Optional) A simple, hardcoded description for your plugin.
        // This is used as a fallback if the localized description key is not found.
        api->Info_SetDescriptionLiteral(h, "A minimal template plugin for the SPF API.");

        // `descriptionKey`: (Optional) A key for a localized description string.
        // This requires using the Localization API and having corresponding translation files.
        // api->Info_SetDescriptionKey(h, "plugin.description");

        // --- Optional Social and Project Links ---
        // Uncomment any of the following lines to provide contact or project URLs.
        // These will be displayed in the plugin's information panel in the UI.

        // api->Info_SetEmail(h, "your.email@example.com");
        // api->Info_SetDiscordUrl(h, "https://discord.gg/your_invite_code");
        // api->Info_SetSteamProfileUrl(h, "https://steamcommunity.com/id/your_profile");
        // api->Info_SetGithubUrl(h, "https://github.com/your_username/your_repo");
        // api->Info_SetYoutubeUrl(h, "https://www.youtube.com/your_channel");
        // api->Info_SetScsForumUrl(h, "https://forum.scssoft.com/viewtopic.php?f=your_topic");
        // api->Info_SetPatreonUrl(h, "https://www.patreon.com/your_creator_name");
        // api->Info_SetWebsiteUrl(h, "https://your.website.com");
    }

    // --- 2.2. Configuration Policy ---
    // This section defines how your plugin interacts with the framework's configuration system.
    {
        // `allowUserConfig`: Set to `true` if you want a `settings.json` file to be created
        // for your plugin, allowing users (or the framework UI) to override default settings.
        api->Policy_SetAllowUserConfig(h, false);

        // To enable configurable systems, uncomment the calls below.
        // api->Policy_AddConfigurableSystem(h, "logging");
        // api->Policy_AddConfigurableSystem(h, "settings");
        // api->Policy_AddConfigurableSystem(h, "localization");
        // api->Policy_AddConfigurableSystem(h, "ui");

        // To enable required hooks, uncomment the line below.
        // api->Policy_AddRequiredHook(h, "GameConsole"); // Example: Requires GameConsole hook
    }

    // --- 2.3. Custom Settings (settingsJson) ---
    // A JSON string literal that defines the default values for your plugin's custom settings.
    // If `Policy_SetAllowUserConfig` is true, the framework creates a `settings.json` file.
    // The JSON object you provide here will be inserted under a top-level key named "settings".

    // Example: Define some default custom settings.
    /*
    api->Settings_SetJson(h, R"json(
        {
            "some_number": 42,
            "some_bool": false,
            "some_string": "hello",
            "feature_flags": {
                "alpha": true,
                "beta": false
            }
        }
    )json");
    */

    // --- 2.4. Default Settings for Framework Systems ---
    // Here you can provide default configurations for various framework systems.

    // --- Logging ---
    // Requires: SPF_Logger_API.h
    {
        // `level`: Default minimum log level for this plugin (e.g., "trace", "debug", "info", "warn", "error", "critical").
        api->Defaults_SetLogging(h, "info", false);
    }

    // --- Localization ---
    // Requires: SPF_Localization_API.h
    // Uncomment if your plugin uses localized strings.
    /*
    {
        // `language`: Default language code (e.g., "en", "de", "uk").
        api->Defaults_SetLocalization(h, "en");
    }
    */

    // --- Keybinds ---
    // Requires: SPF_KeyBinds_API.h
    // Uncomment and configure if your plugin needs custom keybinds.
    /*
    {
        // --- Action 0: A sample keybind to toggle a window ---
        // `groupName`: Logical grouping for actions, used to avoid name collisions.
        // Best practice: "{PluginName}.{Feature}".
        // `actionName`: Specific action (e.g., "toggle", "activate").
        // Define one or more default key combinations for this action.
        // `type`: "keyboard" or "gamepad".
        // `key`: Key name (see VirtualKeyMapping.cpp or GamepadButtonMapping.cpp).
        // `pressType`: "short" (tap) or "long" (hold).
        // `pressThresholdMs`: For "long" press, time in ms to hold.
        // `consume`: When to consume input: "never", "on_ui_focus", "always".
        // `behavior`: How action behaves. Valid values: "toggle" (on/off), "hold" (while pressed).
        api->Defaults_AddKeybind(h, "MyPlugin.MainWindow", "toggle", "keyboard", "KEY_F5", "short", 300, "always", "toggle");
    }
    */

    // --- UI ---
    // Requires: SPF_UI_API.h
    // Uncomment and configure if your plugin needs GUI windows.
    /*
    {
        // --- Window 0: The main window for the plugin ---
        // `name`: Unique ID for this window within the plugin.
        // `isVisible`: Default visibility state.
        // `isInteractive`: If false, mouse clicks pass through the window to the game.
        // Default position and size on screen.
        // `isCollapsed`: Default collapsed state.
        // `autoScroll`: If the window should auto-scroll to the bottom on new content.
        api->Defaults_AddWindow(h, "MainWindow", true, true, 100, 100, 400, 300, false, false);
    }
    */

    // =============================================================================================
    // 2.5. Metadata for UI Display (Optional)
    // =============================================================================================
    // These sections are used to provide human-readable names and descriptions for your
    // settings, keybinds, and UI windows in the framework's settings panel.
    // If you don't provide metadata for an item, the framework will use its raw key as a label.
    //==============================================================================================

    // --- Custom Settings Metadata ---
    // Provide titles and descriptions for the settings defined in `settingsJson`.
    
    /*
    {
        //--- Metadata for "some_number" ---
        // Can be a localization key or literal text.
        // Optional: Specify a UI widget (e.g., "slider") and its parameters.
        api->Meta_AddCustomSetting(h, "some_number", "My Awesome Number", "This is the description for the awesome number.", "slider", "{ \"min\": 0, \"max\": 100, \"format\": \"%d\" }", false);

        //--- Example of a hidden setting ---
        // No title or description needed, as it won't be shown
        api->Meta_AddCustomSetting(h, "internal_coordinates", nullptr, nullptr, nullptr, nullptr, true);
    }
    */

    // --- Keybinds Metadata ---
    // Provide titles and descriptions for the actions defined in `keybinds`.

    /*
    {
        // `groupName`: Must match the action's groupName.
        // `actionName`: Must match the action's actionName.
        api->Meta_AddKeybind(h, "MyPlugin.MainWindow", "toggle", "Toggle Main Window", "Opens or closes the main window of MyPlugin.");
    }
    */

    // --- Standard Settings Metadata (Logging, Localization, UI) ---
    // You can override the default titles/descriptions for standard framework settings.
    /*
    {
        // Example: Override the description for the "level" setting in the logging section.
        api->Meta_AddStandardSetting(h, "logging", "level", "Log Level (MyPlugin)", "Sets the minimum level for messages to be logged by MyPlugin.");
    }
    */

    /*
    // --- TIP: Mass Metadata Registration ---
    // For plugins with many settings, use helper lambdas to avoid repetition.
    // NOTE: Requires '#include <string>'.

    auto AddSliderHelper = [&](const char* key, const char* title, float min, float max, const char* fmt) {
        std::string p = "{ \"min\": " + std::to_string(min) + ", \"max\": " + std::to_string(max) + ", \"format\": \"" + fmt + "\" }";
        api->Meta_AddCustomSetting(h, key, title, nullptr, "slider", p.c_str(), false);
    };

    AddSliderHelper("my_feature.fov", "Field of View", 60.0f, 120.0f, "%.0f");
    */
}

// =================================================================================================
// 3. Plugin Lifecycle Implementations
// =================================================================================================
// The following functions are the core lifecycle events for the plugin.

void OnLoad(const SPF_Load_API* load_api) {
    // Cache the provided API pointers in our global context.
    g_ctx.loadAPI = load_api;

    // --- Essential API Initialization ---
    // Get and cache the logger and formatting API handles.
    if (g_ctx.loadAPI) {
        g_ctx.loggerHandle = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);
        g_ctx.formattingAPI = g_ctx.loadAPI->formatting;

        if (g_ctx.loggerHandle && g_ctx.formattingAPI) {
            char log_buffer[256];
            g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
            g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
        }
    }

    // --- Optional API Initialization (Uncomment if needed) ---
    // Remember to also uncomment the relevant #include directives in MyPlugin.hpp
    // and add corresponding members to the PluginContext struct.

    /*
    // Config API
    // Requires: SPF_Config_API.h
    */

    /*
    // Localization API
    // Requires: SPF_Localization_API.h
    */

    /*
    // Virtual Input API
    // Requires: SPF_VirtInput_API.h
    */
}

void OnActivated(const SPF_Core_API* core_api) {
    g_ctx.coreAPI = core_api;

    if (g_ctx.loggerHandle && g_ctx.formattingAPI) {
        char log_buffer[256];
        g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s has been activated!", PLUGIN_NAME);
        g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
    }

    // --- Optional API Initialization & Callback Registration (Uncomment if needed) ---
    // Remember to also uncomment the relevant #include directives in MyPlugin.hpp
    // and add corresponding members to the PluginContext struct.

    /*
    // Keybinds API
    // Requires: SPF_KeyBinds_API.h
    */

    /*
    // Game Log API
    // Requires: SPF_GameLog_API.h
    */

    /*
    // Telemetry API
    // Requires: SPF_Telemetry_API.h, SPF_TelemetryData.h (uncomment in MyPlugin.hpp)
    // 1. Check if the telemetry API is available.
    if (g_ctx.coreAPI && g_ctx.coreAPI->telemetry) {
        // 2. Get the telemetry context handle for this plugin. This is the main handle
        //    that manages all telemetry subscriptions for this plugin.
        g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);

        // 3. Register callbacks for the telemetry data you need.
        //    The returned handles are managed by the framework and will be automatically
        //    cleaned up when the plugin unloads. Storing them in g_ctx is optional but good practice.
        //    The final parameter (e.g., &g_ctx) is user data passed to the callback.
        if (g_ctx.telemetryHandle) {
            const auto tel = g_ctx.coreAPI->telemetry; // Shortcut for Telemetry API
            g_ctx.gameStateSubscription = tel->RegisterForGameState(g_ctx.telemetryHandle, OnGameState, &g_ctx);
            g_ctx.timestampsSubscription = tel->RegisterForTimestamps(g_ctx.telemetryHandle, OnTimestamps, &g_ctx);
            g_ctx.commonDataSubscription = tel->RegisterForCommonData(g_ctx.telemetryHandle, OnCommonData, &g_ctx);
            g_ctx.truckConstantsSubscription = tel->RegisterForTruckConstants(g_ctx.telemetryHandle, OnTruckConstants, &g_ctx);
            g_ctx.trailerConstantsSubscription = tel->RegisterForTrailerConstants(g_ctx.telemetryHandle, OnTrailerConstants, &g_ctx);
            g_ctx.truckDataSubscription = tel->RegisterForTruckData(g_ctx.telemetryHandle, OnTruckData, &g_ctx);
            g_ctx.trailersSubscription = tel->RegisterForTrailers(g_ctx.telemetryHandle, OnTrailers, &g_ctx);
            g_ctx.jobConstantsSubscription = tel->RegisterForJobConstants(g_ctx.telemetryHandle, OnJobConstants, &g_ctx);
            g_ctx.jobDataSubscription = tel->RegisterForJobData(g_ctx.telemetryHandle, OnJobData, &g_ctx);
            g_ctx.navigationDataSubscription = tel->RegisterForNavigationData(g_ctx.telemetryHandle, OnNavigationData, &g_ctx);
            g_ctx.controlsSubscription = tel->RegisterForControls(g_ctx.telemetryHandle, OnControls, &g_ctx);
            g_ctx.specialEventsSubscription = tel->RegisterForSpecialEvents(g_ctx.telemetryHandle, OnSpecialEvents, &g_ctx);
            g_ctx.gameplayEventsSubscription = tel->RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvents, &g_ctx);
            g_ctx.gearboxConstantsSubscription = tel->RegisterForGearboxConstants(g_ctx.telemetryHandle, OnGearboxConstants, &g_ctx);
        }
    }
    */

    /*
    // Hooks API
    // Requires: SPF_Hooks_API.h
    */

    /*
    // Game Console API
    // Requires: SPF_GameConsole_API.h
    */

    /*
    // Camera API
    // Requires: SPF_Camera_API.h
    */
}

void OnUpdate() {
    // This function is called every frame while the plugin is active.
    // Avoid performing heavy or blocking operations here, as it will directly impact game performance.

    // --- Optional API Usage (Uncomment if needed) ---
    // Remember to also uncomment the relevant #include directives in MyPlugin.hpp/MyPlugin.cpp
    // and add corresponding members to the PluginContext struct.

    /*
    // Example: Polling Telemetry data
    // Requires: SPF_Telemetry_API.h (and corresponding types in PluginContext)
    */

    /*
    // Example: Simulating Virtual Input (e.g., holding a button)
    // Requires: SPF_VirtInput_API.h (and corresponding types in PluginContext)
    */
}

void OnUnload() {
    // Perform cleanup. Nullify cached API pointers to prevent use-after-free
    // and ensure a clean shutdown. This is the last chance for cleanup.

    if (g_ctx.loadAPI && g_ctx.loggerHandle && g_ctx.formattingAPI) {
        char log_buffer[256];
        g_ctx.formattingAPI->Fmt_Format(log_buffer, sizeof(log_buffer), "%s is being unloaded.", PLUGIN_NAME);
        g_ctx.loadAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, log_buffer);
    }

    // --- Optional API Cleanup (Uncomment if needed) ---
    // Example: Unregistering keybinds (often handled by framework, but good practice if explicitly registered).
    // Requires: SPF_KeyBinds_API.h

    // Nullify all cached API pointers and handles.
    g_ctx.coreAPI = nullptr;
    g_ctx.loadAPI = nullptr;
    g_ctx.loggerHandle = nullptr;
    g_ctx.formattingAPI = nullptr;

    // --- Optional Handles (Nullify if used) ---
    // g_ctx.configHandle = nullptr;
    // g_ctx.localizationHandle = nullptr;
    // g_ctx.keybindsHandle = nullptr;
    // g_ctx.uiAPI = nullptr;
    // g_ctx.mainWindowHandle = nullptr;
    // g_ctx.telemetryHandle = nullptr;
    // g_ctx.hooksAPI = nullptr;
    // g_ctx.gameConsoleAPI = nullptr;
    // g_ctx.virtualDeviceHandle = nullptr;
    // g_ctx.cameraAPI = nullptr;
    // g_ctx.gameLogCallbackHandle = nullptr;
    //
    // // Telemetry Subscriptions (Nullify if used)
    // g_ctx.gameStateSubscription = nullptr;
    // g_ctx.timestampsSubscription = nullptr;
    // g_ctx.commonDataSubscription = nullptr;
    // g_ctx.truckConstantsSubscription = nullptr;
    // g_ctx.trailerConstantsSubscription = nullptr;
    // g_ctx.truckDataSubscription = nullptr;
    // g_ctx.trailersSubscription = nullptr;
    // g_ctx.jobConstantsSubscription = nullptr;
    // g_ctx.jobDataSubscription = nullptr;
    // g_ctx.navigationDataSubscription = nullptr;
    // g_ctx.controlsSubscription = nullptr;
    // g_ctx.specialEventsSubscription = nullptr;
    // g_ctx.gameplayEventsSubscription = nullptr;
    // g_ctx.gearboxConstantsSubscription = nullptr;
}

// =================================================================================================
// 4. Optional Callback Implementations (Commented Out)
// =================================================================================================
// Implement these functions if your plugin needs to react to specific events.
// Remember to also uncomment their prototypes in MyPlugin.hpp and register them
// in OnActivated or OnRegisterUI as appropriate.

/*
// --- OnSettingChanged Callback ---
// Requires: SPF_Config_API.h
void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
    // A setting has changed. Check the keyPath and use the config_handle
    // with the Config API to get the new value.
    // Example:
    // if (strcmp(keyPath, "settings.some_bool") == 0) {
    //     bool newValue = g_ctx.loadAPI->config->Cfg_GetBool(config_handle, keyPath, false);
    //     // ... react to the new value ...
    // }
}
*/

/*
// --- OnLanguageChanged Callback ---
// Requires: SPF_Localization_API.h
// This callback allows your plugin to automatically match the framework's language.
void OnLanguageChanged(const char* langCode) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->localization || !langCode) return;

    SPF_Localization_Handle* h = g_ctx.coreAPI->localization->Loc_GetContext(PLUGIN_NAME);
    
    // Loc_GetFrameworkLanguage(): Returns the current language code used by the framework.
    // Loc_HasLanguage(h, code): Checks if your plugin actually has a translation for that code.
    
    // Example: Smart synchronization
    if (g_ctx.coreAPI->localization->Loc_HasLanguage(h, langCode)) {
        g_ctx.coreAPI->localization->Loc_SetLanguage(h, langCode);
        // Log sync event...
    }
}
*/

/*
// --- OnRegisterUI Callback ---
// Requires: SPF_UI_API.h
*/

/*
// --- RenderMainWindow Callback (for UI) ---
// Requires: SPF_UI_API.h
// This function name should match what you passed to RegisterDrawCallback.
*/

/*
// --- OnKeybindAction Callback ---
// Requires: SPF_KeyBinds_API.h
// This function name should match what you passed to SPF_KeyBinds_API.Register.
*/

/*
// --- OnGameLogMessage Callback ---
// Requires: SPF_GameLog_API.h
// This function name should match what you passed to SPF_GameLog_API.RegisterCallback.
*/

/*
// --- OnGameWorldReady Callback ---
// Called once when the game world has been fully loaded. Ideal for initializing
// hooks or logic that depends on game objects being in memory.
void OnGameWorldReady() {
    // if (g_ctx.coreAPI && g_ctx.coreAPI->logger) {
    //     g_ctx.coreAPI->logger->Log(g_ctx.loggerHandle, SPF_LOG_INFO, "Game world is ready!");
    // }
}
*/

// =================================================================================================
// 5. Optional Helper Function Implementations (Commented Out)
// =================================================================================================
// Implement these functions if your plugin needs internal helper logic.
// Remember to also uncomment their prototypes in MyPlugin.hpp.

/*
// --- InitializeVirtualDevice Helper ---
// Requires: SPF_VirtInput_API.h
// This function could be called from OnLoad.
*/

/*
// --- Game Hook Implementation ---
// Requires: SPF_Hooks_API.h
*/

// =================================================================================================
// 5.1. Optional Telemetry Callback Implementations (Commented Out)
// =================================================================================================
// Implement these functions if your plugin subscribes to telemetry data.
// Remember to also uncomment their prototypes in MyPlugin.hpp and register them in OnActivated.
// Requires: SPF_Telemetry_API.h, SPF_TelemetryData.h (for data structures, uncomment in MyPlugin.hpp)

/*
void OnGameState(const SPF_GameState* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest game state data
    // // ctx->telemetryDataCache.gameState = *data;
}
*/

/*
void OnTimestamps(const SPF_Timestamps* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest timestamp data
    // // ctx->telemetryDataCache.timestamps = *data;
}
*/

/*
void OnCommonData(const SPF_CommonData* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest common data
    // // ctx->telemetryDataCache.commonData = *data;
}
*/

/*
void OnTruckConstants(const SPF_TruckConstants* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest truck constants
    // // ctx->telemetryDataCache.truckConstants = *data;
}
*/

/*
void OnTrailerConstants(const SPF_TrailerConstants* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest trailer constants
    // // ctx->telemetryDataCache.trailerConstants = *data;
}
*/

/*
void OnTruckData(const SPF_TruckData* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest truck data
    // // ctx->telemetryDataCache.truckData = *data;
}
*/

/*
void OnTrailers(const SPF_Trailer* trailers, uint32_t count, void* user_data) {
    // if (!user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest trailers data
    // // ctx->telemetryDataCache.trailers.clear();
    // // if (trailers && count > 0) {
    // //     for (uint32_t i = 0; i < count; ++i) {
    // //         ctx->telemetryDataCache.trailers.push_back(trailers[i]);
    // //     }
    // // }
}
*/

/*
void OnJobConstants(const SPF_JobConstants* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest job constants
    // // ctx->telemetryDataCache.jobConstants = *data;
}
*/

/*
void OnJobData(const SPF_JobData* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest job data
    // // ctx->telemetryDataCache.jobData = *data;
}
*/

/*
void OnNavigationData(const SPF_NavigationData* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest navigation data
    // // ctx->telemetryDataCache.navigationData = *data;
}
*/

/*
void OnControls(const SPF_Controls* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest controls data
    // // ctx->telemetryDataCache.controls = *data;
}
*/

/*
void OnSpecialEvents(const SPF_SpecialEvents* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest special events data
    // // ctx->telemetryDataCache.specialEvents = *data;
}
*/

/*
void OnGameplayEvents(const char* event_id, const SPF_GameplayEvents* data, void* user_data) {
    // if (!event_id || !data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest gameplay event data
    // // ctx->telemetryDataCache.gameplayEvents = *data;
    // // strncpy_s(ctx->telemetryDataCache.lastGameplayEventId, event_id, sizeof(ctx->telemetryDataCache.lastGameplayEventId));
}
*/

/*
void OnGearboxConstants(const SPF_GearboxConstants* data, void* user_data) {
    // if (!data || !user_data) return;
    // MyPlugin::PluginContext* ctx = reinterpret_cast<MyPlugin::PluginContext*>(user_data);
    // // Example: Cache the latest gearbox constants
    // // ctx->telemetryDataCache.gearboxConstants = *data;
}
*/

// =================================================================================================
// 6. Plugin Exports
// =================================================================================================
// These are the two mandatory, C-style functions that the plugin DLL must export.
// The `extern "C"` block is essential to prevent C++ name mangling, ensuring the framework
// can find them by name.

extern "C" {

/**
 * @brief Exports the manifest API to the framework.
 * @details This function is mandatory for the framework to properly identify and configure the plugin.
 */
SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
    if (out_api) {
        out_api->BuildManifest = BuildManifest;
        return true;
    }
    return false;
}

/**
 * @brief Exports the plugin's main lifecycle and callback functions to the framework.
 * @details This function is mandatory for the framework to interact with the plugin's lifecycle.
 */
SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports) {
    if (exports) {
        // Connect the internal C++ functions to the C-style export struct.
        exports->OnLoad = OnLoad;
        exports->OnActivated = OnActivated;
        exports->OnUnload = OnUnload;
        exports->OnUpdate = OnUpdate;

        // Optional callbacks are set to nullptr by default.
        // Uncomment and assign your implementation if you use them.
        // exports->OnGameWorldReady = OnGameWorldReady; // Assign your OnGameWorldReady function for game-world-dependent logic.
        // exports->OnRegisterUI = OnRegisterUI;         // Assign your OnRegisterUI function if you have UI windows.
        // exports->OnSettingChanged = OnSettingChanged; // Assign your OnSettingChanged function if you implement it.
        // exports->OnLanguageChanged = OnLanguageChanged; // Assign your OnLanguageChanged function for auto-sync.
        return true;
    }
    return false;
}

} // extern "C"

} // namespace MyPlugin
