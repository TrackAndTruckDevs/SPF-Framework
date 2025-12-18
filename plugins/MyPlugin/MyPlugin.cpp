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
 * @details This MUST match the name used in `GetContext` calls for various APIs
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

void GetManifestData(SPF_ManifestData_C& out_manifest) {
    // This function defines all the metadata for your plugin. The framework calls this
    // function *before* loading your plugin DLL to understand what it is.

    // --- 2.1. Plugin Information (SPF_InfoData_C) ---
    // This section provides the basic identity of your plugin.
    {
        auto& info = out_manifest.info;

        // `name`: (Optional) A unique name for the plugin (e.g., "MyPlugin").
        // If not specified, the framework will use the name of your DLL file, but specifying it
        // here is recommended to avoid potential conflicts.
        strncpy_s(info.name, PLUGIN_NAME, sizeof(info.name));

        // `version`: (Optional) The plugin's version string (e.g., "1.0.0").
        strncpy_s(info.version, "0.1.0", sizeof(info.version));

        // `author`: (Optional) The name of the author or organization.
        strncpy_s(info.author, "Your Name/Organization", sizeof(info.author));

        // `descriptionLiteral`: (Optional) A simple, hardcoded description for your plugin.
        // This is used as a fallback if the localized description key is not found.
        strncpy_s(info.descriptionLiteral, "A minimal template plugin for the SPF API.", sizeof(info.descriptionLiteral));

        // `descriptionKey`: (Optional) A key for a localized description string.
        // This requires using the Localization API and having corresponding translation files.
        // strncpy_s(info.descriptionKey, "plugin.description", sizeof(info.descriptionKey));

        // --- Optional Social and Project Links ---
        // Uncomment any of the following lines to provide contact or project URLs.
        // These will be displayed in the plugin's information panel in the UI.

        // `email`: (Optional) A contact email.
        // strncpy_s(info.email, "your.email@example.com", sizeof(info.email));
        // `discordUrl`: (Optional) A URL to a Discord server.
        // strncpy_s(info.discordUrl, "https://discord.gg/your_invite_code", sizeof(info.discordUrl));
        // `steamProfileUrl`: (Optional) A URL to a Steam profile.
        // strncpy_s(info.steamProfileUrl, "https://steamcommunity.com/id/your_profile", sizeof(info.steamProfileUrl));
        // `githubUrl`: (Optional) A URL to the GitHub repository for this plugin.
        // strncpy_s(info.githubUrl, "https://github.com/your_username/your_repo", sizeof(info.githubUrl));
        // `youtubeUrl`: (Optional) A URL to a YouTube channel or video.
        // strncpy_s(info.youtubeUrl, "https://www.youtube.com/your_channel", sizeof(info.youtubeUrl));
        // `scsForumUrl`: (Optional) A URL to a thread on the SCS Software forums.
        // strncpy_s(info.scsForumUrl, "https://forum.scssoft.com/viewtopic.php?f=your_topic", sizeof(info.scsForumUrl));
        // `patreonUrl`: (Optional) A URL to a Patreon page.
        // strncpy_s(info.patreonUrl, "https://www.patreon.com/your_creator_name", sizeof(info.patreonUrl));
        // `websiteUrl`: (Optional) A URL to a personal or project website.
        // strncpy_s(info.websiteUrl, "https://your.website.com", sizeof(info.websiteUrl));
    }

    // --- 2.2. Configuration Policy (SPF_ConfigPolicyData_C) ---
    // This section defines how your plugin interacts with the framework's configuration system.
    {
        auto& policy = out_manifest.configPolicy;

        // `allowUserConfig`: Set to `true` if you want a `settings.json` file to be created
        // for your plugin, allowing users (or the framework UI) to override default settings.
        policy.allowUserConfig = false;

        // `userConfigurableSystemsCount`: The number of framework systems (e.g., "settings", "logging", "localization", "ui")
        // that should have a configuration section generated in the settings UI for your plugin.
        // IMPORTANT: Always initialize this to 0 if you are not listing any systems to avoid errors.
        policy.userConfigurableSystemsCount = 0; //To enable configurable systems, uncomment the block below and set the count accordingly
        // strncpy_s(policy.userConfigurableSystems[0], "logging", sizeof(policy.userConfigurableSystems[0]));
        // strncpy_s(policy.userConfigurableSystems[1], "settings", sizeof(policy.userConfigurableSystems[1]));
        // strncpy_s(policy.userConfigurableSystems[1], "localization", sizeof(policy.userConfigurableSystems[1]));
        // strncpy_s(policy.userConfigurableSystems[1], "ui", sizeof(policy.userConfigurableSystems[1]));

        // `requiredHooksCount`: List any game hooks your plugin absolutely requires to function.
        // The framework will ensure these hooks are enabled whenever your plugin is active,
        // regardless of user settings.
        // IMPORTANT: Always initialize this to 0 if you are not listing any hooks to avoid errors.
        policy.requiredHooksCount = 0; // To enable required hooks, uncomment the lines below and set the count accordingly.
        // strncpy_s(policy.requiredHooks[0], "GameConsole", sizeof(policy.requiredHooks[0])); // Example: Requires GameConsole hook
    }

    // --- 2.3. Custom Settings (settingsJson) ---
    // A JSON string literal that defines the default values for your plugin's custom settings.
    // If `policy.allowUserConfig` is true, the framework creates a `settings.json` file.
    // The JSON object you provide here will be inserted under a top-level key named "settings".
    out_manifest.settingsJson = nullptr;
    // Example: Define some default custom settings.
    // To provide user-friendly names and descriptions, see `customSettingsMetadata` at the end.
    /*
    out_manifest.settingsJson = R"json(
        {
            "some_number": 42,
            "some_bool": false,
            "some_string": "hello",
            "feature_flags": {
                "alpha": true,
                "beta": false
            }
        }
    )json";
    */

    // --- 2.4. Default Settings for Framework Systems ---
    // Here you can provide default configurations for various framework systems.

    // --- Logging ---
    // Requires: SPF_Logger_API.h
    {
        auto& logging = out_manifest.logging;
        // `level`: Default minimum log level for this plugin (e.g., "trace", "debug", "info", "warn", "error", "critical").
        strncpy_s(logging.level, "info", sizeof(logging.level));
        // `sinks.file`: If true, logs from this plugin will be written to a dedicated file
        // (e.g., `MyPlugin/logs/MyPlugin.log`) in addition to the main framework log.
        logging.sinks.file = false;
    }

    // --- Localization ---
    // Requires: SPF_Localization_API.h
    // Uncomment if your plugin uses localized strings.
    /*
    {
        auto& localization = out_manifest.localization;
        // `language`: Default language code (e.g., "en", "de", "uk").
        strncpy_s(localization.language, "en", sizeof(localization.language));
    }
    */

    // --- Keybinds ---
    // Requires: SPF_KeyBinds_API.h
    // Uncomment and configure if your plugin needs custom keybinds.
    // auto& keybinds = out_manifest.keybinds;
    // keybinds.actionCount = 1; // Number of distinct actions defined by your plugin.
    // {
        // --- Action 0: A sample keybind to toggle a window ---
        // auto& action = keybinds.actions[0];
        // `groupName`: Logical grouping for actions, used to avoid name collisions.
        // Best practice: "{PluginName}.{Feature}".
        // strncpy_s(action.groupName, "MyPlugin.MainWindow", sizeof(action.groupName));
        // `actionName`: Specific action (e.g., "toggle", "activate").
        // strncpy_s(action.actionName, "toggle", sizeof(action.actionName));

        // Define one or more default key combinations for this action.
        // action.definitionCount = 1;
        // {
            // --- Definition 0 ---
            // auto& def = action.definitions[0];
            // `type`: "keyboard" or "gamepad".
            // strncpy_s(def.type, "keyboard", sizeof(def.type));
            // `key`: Key name (see VirtualKeyMapping.cpp or GamepadButtonMapping.cpp).
            // strncpy_s(def.key, "KEY_F5", sizeof(def.key));
            // `pressType`: "short" (tap) or "long" (hold).
            // strncpy_s(def.pressType, "short", sizeof(def.pressType));
            // `pressThresholdMs`: For "long" press, time in ms to hold.
            // def.pressThresholdMs = 300;
            // `consume`: When to consume input: "never", "on_ui_focus", "always".
            // strncpy_s(def.consume, "always", sizeof(def.consume));
            // `behavior`: How action behaves. Valid values: "toggle" (on/off), "hold" (while pressed).
            // strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
        // }
    // }

    // --- UI ---
    // Requires: SPF_UI_API.h
    // Uncomment and configure if your plugin needs GUI windows.
    // auto& ui = out_manifest.ui;
    // ui.windowsCount = 1; // Number of UI windows defined by your plugin.
    // {
        // --- Window 0: The main window for the plugin ---
        // auto& window = ui.windows[0];
        // `name`: Unique ID for this window within the plugin.
        // strncpy_s(window.name, "MainWindow", sizeof(window.name));
        // `isVisible`: Default visibility state.
        // window.isVisible = true;
        // `isInteractive`: If false, mouse clicks pass through the window to the game.
        // window.isInteractive = true;
        // Default position and size on screen.
        // window.posX = 100;
        // window.posY = 100;
        // window.sizeW = 400;
        // window.sizeH = 300;
        // `isCollapsed`: Default collapsed state.
        // window.isCollapsed = false;
        // `autoScroll`: If the window should auto-scroll to the bottom on new content.
        // window.autoScroll = false;
    // }

    // =============================================================================================
    // 2.5. Metadata for UI Display (Optional)
    // =============================================================================================
    // These sections are used to provide human-readable names and descriptions for your
    // settings, keybinds, and UI windows in the framework's settings panel.
    // If you don't provide metadata for an item, the framework will use its raw key as a label.
    //==============================================================================================

    // --- Custom Settings Metadata ---
    // Provide titles and descriptions for the settings defined in `settingsJson`.
    
    /*out_manifest.customSettingsMetadataCount = 0; // To enable custom settings metadata, uncomment the lines below and set the count accordingly.
    {
        //--- Metadata for "some_number" ---
        auto& meta = out_manifest.customSettingsMetadata[0];
        strncpy_s(meta.keyPath, "some_number", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "My Awesome Number", sizeof(meta.titleKey)); // Can be a localization key or literal text
        strncpy_s(meta.descriptionKey, "This is the description for the awesome number.", sizeof(meta.descriptionKey)); // Can be a localization key or literal text

        //Optional: Specify a UI widget (e.g., "slider") and its parameters.
        strncpy_s(meta.widget, "slider", sizeof(meta.widget));
        meta.widget_params.slider.min_val = 0;
        meta.widget_params.slider.max_val = 100;
        strncpy_s(meta.widget_params.slider.format, "%d", sizeof(meta.widget_params.slider.format));
    }*/

    // --- Keybinds Metadata ---
    // Provide titles and descriptions for the actions defined in `keybinds`.

    /*out_manifest.keybindsMetadataCount = 0; // To enable keybinds metadata, uncomment the lines below and set the count accordingly.
    {
        auto& meta = out_manifest.keybindsMetadata[0];
        strncpy_s(meta.groupName, "MyPlugin.MainWindow", sizeof(meta.groupName)); // Must match the action's groupName
        strncpy_s(meta.actionName, "toggle", sizeof(meta.actionName));           // Must match the action's actionName
        strncpy_s(meta.titleKey, "Toggle Main Window", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "Opens or closes the main window of MyPlugin.", sizeof(meta.descriptionKey));
    }*/

    // --- Standard Settings Metadata (Logging, Localization, UI) ---
    // You can override the default titles/descriptions for standard framework settings.
    /*
    out_manifest.loggingMetadataCount = 0; // To enable logging metadata, uncomment the lines below and set the count accordingly.
    out_manifest.localizationMetadataCount = 0; // To enable localization metadata, uncomment the lines below and set the count accordingly.
    out_manifest.uiMetadataCount = 0; // To enable UI metadata, uncomment the lines below and set the count accordingly.
    
    // {
        // Example: Override the description for the "level" setting in the logging section.
        // auto& meta = out_manifest.loggingMetadata[0];
        // strncpy_s(meta.key, "level", sizeof(meta.key));
        // strncpy_s(meta.titleKey, "Log Level (MyPlugin)", sizeof(meta.titleKey)); // Override title
        // strncpy_s(meta.descriptionKey, "Sets the minimum level for messages to be logged by MyPlugin.", sizeof(meta.descriptionKey));
    // }
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
        g_ctx.loggerHandle = g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME);
        g_ctx.formattingAPI = g_ctx.loadAPI->formatting;

        if (g_ctx.loggerHandle && g_ctx.formattingAPI) {
            char log_buffer[256];
            g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s has been loaded!", PLUGIN_NAME);
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
}

void OnActivated(const SPF_Core_API* core_api) {
    g_ctx.coreAPI = core_api;

    if (g_ctx.loggerHandle && g_ctx.formattingAPI) {
        char log_buffer[256];
        g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s has been activated!", PLUGIN_NAME);
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
    // Requires: SPF_Telemetry_API.h
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
    // Virtual Input API
    // Requires: SPF_VirtInput_API.h
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
        g_ctx.formattingAPI->Format(log_buffer, sizeof(log_buffer), "%s is being unloaded.", PLUGIN_NAME);
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
}

// =================================================================================================
// 4. Optional Callback Implementations (Commented Out)
// =================================================================================================
// Implement these functions if your plugin needs to react to specific events.
// Remember to also uncomment their prototypes in MyPlugin.hpp and register them
// in OnActivated or OnRegisterUI as appropriate.

/*
// --- OnSettingChanged Callback ---
// Requires: SPF_Config_API.h, SPF_JsonReader_API.h
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
// This function could be called from OnActivated.
*/

/*
// --- Game Hook Implementation ---
// Requires: SPF_Hooks_API.h
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
        out_api->GetManifestData = GetManifestData;
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
        // exports->OnSettingChanged = OnSettingChanged; // Assign your OnSettingChanged function if you have custom settings.
        return true;
    }
    return false;
}

} // extern "C"

} // namespace MyPlugin
