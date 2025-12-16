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
        strncpy_s(info.name, PLUGIN_NAME, sizeof(info.name));                     // A unique name for the plugin (e.g. "MyPlugin"). MUST be unique. If not specified it will be taken from the name of your dll, possible conflicts
        strncpy_s(info.version, "0.1.0", sizeof(info.version));                   //Optional: Plugin version.
        strncpy_s(info.author, "Your Name/Organization", sizeof(info.author));    //Optional: Author/organization.
        strncpy_s(info.descriptionLiteral, "A minimal template plugin for the SPF API.", sizeof(info.descriptionLiteral)); // Optional Simple description.
        //strncpy_s(info.descriptionKey, "plugin.description", sizeof(info.descriptionKey)); // Optional: A key for a localized description string (requires Localization API).

        // ---Optional Social and Project Links ---
        // strncpy_s(info.email, "MyEmail", sizeof(info.email));
        // strncpy_s(info.discordUrl, "discordUrl", sizeof(info.discordUrl));
        // strncpy_s(info.steamProfileUrl, "steamProfileUrl", sizeof(info.steamProfileUrl));
        // strncpy_s(info.githubUrl, "githubUrl", sizeof(info.githubUrl));
        // strncpy_s(info.youtubeUrl, "youtubeUrl", sizeof(info.youtubeUrl));
        // strncpy_s(info.scsForumUrl, "scsForumUrl", sizeof(info.scsForumUrl));
        // strncpy_s(info.patreonUrl, "patreonUrl", sizeof(info.patreonUrl));
        // strncpy_s(info.websiteUrl, "websiteUrl", sizeof(info.websiteUrl));
    }

    // --- 2.2. Configuration Policy (SPF_ConfigPolicyData_C) ---
    // This section defines how your plugin interacts with the framework's configuration system.
    {
        auto& policy = out_manifest.configPolicy;
        policy.allowUserConfig = false; // Set to true if you want a settings.json file for your plugin, managed by the framework.
        policy.userConfigurableSystemsCount = 0; // List systems like "settings", "logging", "localization", "ui" here.
                                                 // These will appear in the framework's settings for your plugin.
        // strcpy_s(policy.userConfigurableSystems[0], "settings"); // Example: Enables custom settings UI
        // strcpy_s(policy.userConfigurableSystems[1], "logging"); // Example: Enables logging settings UI
        // strcpy_s(policy.userConfigurableSystems[2], "localization"); // Example: Enables localization settings UI
        // strcpy_s(policy.userConfigurableSystems[3], "ui"); // Example: Enables UI window settings UI

        policy.requiredHooksCount = 0; // List any hooks your plugin absolutely requires to function.
                                       // The framework will ensure these are enabled whenever your plugin is active.
        // strcpy_s(policy.requiredHooks[0], "GameConsole"); // Example: Requires GameConsole hook
    }

    // --- 2.3. Custom Settings (settingsJson) ---
    // This is a JSON string literal that defines the default values for your plugin's custom settings.
    // If `policy.allowUserConfig` is true, the framework creates a `settings.json` file.
    // The JSON object you provide here will be inserted under a top-level key named "settings".
    out_manifest.settingsJson = nullptr; // Example: R"json({ "some_number": 42, "some_bool": false })json"

    // --- 2.4. Default Settings for Framework Systems ---
    // Here you can provide default configurations for various framework systems for your plugin.

    // --- Logging ---
    // Requires: SPF_Logger_API.h
    {
        auto& logging = out_manifest.logging;
        strncpy_s(logging.level, "info", sizeof(logging.level)); // Default minimum log level (e.g., "trace", "debug", "info").
        logging.sinks.file = false; // If true, logs will also be written to a dedicated file (e.g., MyPlugin/logs/MyPlugin.log).
    }

    // --- Localization ---
    // Requires: SPF_Localization_API.h
    /*
    {
        auto& localization = out_manifest.localization;
        strncpy_s(localization.language, "en", sizeof(localization.language)); // Default language code (e.g., "en", "uk").
    }
    */

    // --- Keybinds ---
    // Requires: SPF_KeyBinds_API.h
    // Uncomment and configure if your plugin needs custom keybinds.
    /*
    {
        auto& keybinds = out_manifest.keybinds;
        keybinds.actionCount = 1; // Number of distinct actions defined by your plugin.
        {
            auto& action = keybinds.actions[0];
            strncpy_s(action.groupName, "MyPlugin.MainWindow", sizeof(action.groupName)); // Logical grouping (e.g., "{PluginName}.{Feature}").
            strncpy_s(action.actionName, "toggle", sizeof(action.actionName));          // Specific action (e.g., "toggle", "activate").
            action.definitionCount = 1; // Number of default key combinations for this action.
            auto& def = action.definitions[0];
            strncpy_s(def.type, "keyboard", sizeof(def.type));      // Input type: "keyboard" or "gamepad".
            strncpy_s(def.key, "KEY_F5", sizeof(def.key));          // Key name (see VirtualKeyMapping.cpp).
            strncpy_s(def.pressType, "short", sizeof(def.pressType)); // Press type: "short" (tap) or "long" (hold).
            def.pressThresholdMs = 300;                             // For "long" press, time in ms to hold.
            strncpy_s(def.consume, "always", sizeof(def.consume));  // When to consume input: "never", "on_ui_focus", "always".
            strncpy_s(def.behavior, "toggle", sizeof(def.behavior)); // How action behaves: "toggle" (on/off), "hold" (while pressed), "press" (fire once).
        }
    }
    */

    // --- UI ---
    // Requires: SPF_UI_API.h
    // Uncomment and configure if your plugin needs GUI windows.
    /*
    {
        auto& ui = out_manifest.ui;
        ui.windowsCount = 1; // Number of UI windows defined by your plugin.
        {
            auto& window = ui.windows[0];
            strncpy_s(window.name, "MainWindow", sizeof(window.name));           // Unique ID for this window within the plugin.
            strncpy_s(window.description, "Main window for MyPlugin", sizeof(window.description)); // Tooltip shown in the UI.
            window.isVisible = true;      // Default visibility state.
            window.isInteractive = true;  // If false, mouse clicks pass through to the game.
            window.posX = 100;            // Default position on screen.
            window.posY = 100;
            window.sizeW = 400;           // Default size.
            window.sizeH = 300;
            window.isCollapsed = false;   // Default collapsed state.
            window.autoScroll = false;    // If the window should auto-scroll to the bottom on new content.
        }
    }
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
