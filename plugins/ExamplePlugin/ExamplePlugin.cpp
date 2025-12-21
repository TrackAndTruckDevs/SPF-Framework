/**
 * @file ExamplePlugin.cpp
 * @brief The main implementation file for the ExamplePlugin.
 * @details This file contains the implementation for all functions declared in ExamplePlugin.hpp.
 * It is organized into logical sections to improve readability and maintainability, with
 * detailed comments explaining the purpose and logic of each part, serving as a comprehensive
 * guide for new developers.
 */

#include "ExamplePlugin.hpp"
#include <cstring> // For C-style string manipulation functions like strncpy_s, strcpy_s, strcmp, strstr.

namespace ExamplePlugin {

// =================================================================================================
// 1. Constants & Global State
// =================================================================================================

/**
 * @brief A constant for the plugin's name.
 * @details Using a constant avoids "magic strings" (hard-coded strings scattered in the code)
 * and makes it easy to rename the plugin in one place. It is used to identify the plugin
 * by the framework for logging, configuration, and other services.
 */
const char* PLUGIN_NAME = "ExamplePlugin";

/**
 * @brief The single, global instance of the plugin's context.
 * @details This is defined once here and declared as `extern` in the header, making it the central
 * point for accessing all plugin state. This pattern is crucial for managing state in a C-style,
 * callback-driven environment where you cannot easily pass class instances around.
 */
PluginContext g_ctx;

// =================================================================================================
// 2. Manifest Implementation
// =================================================================================================

void GetManifestData(SPF_ManifestData_C& out_manifest) {
    // This function is where you define all the metadata for your plugin. The framework calls this
    // function *before* loading your plugin DLL to understand what it is, what it needs, and how it
    // can be configured. This uses a C-style struct for maximum ABI (Application Binary Interface)
    // stability, ensuring compatibility even if the plugin and framework are built with different
    // compilers or settings.

    // --- 2.1. Plugin Information (SPF_InfoData_C) ---
    // This section provides the basic identity of your plugin.
    {
        auto& info = out_manifest.info;

        // `name`: The unique programmatic name of your plugin. No spaces or special characters.
        // This is used for internal identification, folder names, and config files.
        // CRITICAL: This MUST match the name used in `GetContext` calls for various APIs.
        strncpy_s(info.name, PLUGIN_NAME, sizeof(info.name));

        // `version`: The version of your plugin. It's a best practice to follow Semantic Versioning (semver.org).
        // Example: "1.0.0", "2.1.0-beta", etc.
        strncpy_s(info.version, "0.1.0-alpha", sizeof(info.version));

        // `author`: (Optional) Your name or your organization's name.
        strncpy_s(info.author, "Your Name", sizeof(info.author));

        //---Optional Social and Project Links ---
        strncpy_s(info.email, "mailto:your.email@example.com", sizeof(info.email));
        strncpy_s(info.discordUrl, "discordUrl", sizeof(info.discordUrl));
        strncpy_s(info.steamProfileUrl, "steamProfileUrl", sizeof(info.steamProfileUrl));
        strncpy_s(info.githubUrl, "githubUrl", sizeof(info.githubUrl));
        strncpy_s(info.youtubeUrl, "youtubeUrl", sizeof(info.youtubeUrl));
        strncpy_s(info.scsForumUrl, "scsForumUrl", sizeof(info.scsForumUrl));
        strncpy_s(info.patreonUrl, "patreonUrl", sizeof(info.patreonUrl));
        strncpy_s(info.websiteUrl, "websiteUrl", sizeof(info.websiteUrl));

        // `descriptionKey`: (Optional) A key for a localized description string. If you provide a key
        // (e.g., "plugin.description"), the framework will look it up in your translation files.
        // If you leave it empty, it will use `descriptionLiteral` instead.
        info.descriptionKey[0] = '\0';

        // `descriptionLiteral`: A fallback description used if `descriptionKey` is empty or not found.
        strncpy_s(info.descriptionLiteral, "A template plugin to demonstrate the SPF API.", sizeof(info.descriptionLiteral));
    }

    // --- 2.2. Configuration Policy (SPF_ConfigPolicyData_C) ---
    // This section defines how your plugin interacts with the framework's configuration system.
    {
        auto& policy = out_manifest.configPolicy;

        // `allowUserConfig`: If `true`, the framework will generate a `settings.json` file
        // inside the plugin's config folder (e.g., `/plugins/ExamplePlugin/config/settings.json`),
        // allowing users to override default settings.
        policy.allowUserConfig = true;

        // `userConfigurableSystems`: A list of framework systems that the user can configure for this
        // plugin via the main Settings UI. Common values are "settings", "logging", "localization", "ui".
        // Note: The "keybinds" system is always user-configurable and does not need to be listed here.
        policy.userConfigurableSystemsCount = 4;
        strcpy_s(policy.userConfigurableSystems[0], "settings");
        strcpy_s(policy.userConfigurableSystems[1], "logging");
        strcpy_s(policy.userConfigurableSystems[2], "localization");
        strcpy_s(policy.userConfigurableSystems[3], "ui");

        // `requiredHooks`: (Optional) A list of function hooks required for the plugin to work.
        // If a hook is listed here, the framework will ensure it is enabled whenever this plugin is
        // active, and the user will not be able to disable it from the UI.
        policy.requiredHooksCount = 2;
        strcpy_s(policy.requiredHooks[0], "GameConsole"); // We need this for the console command example.
        strcpy_s(policy.requiredHooks[1], "GameLogHook");   // We need this for the game log example.
    }

    // --- 2.3. Custom Settings (settingsJson) ---
    // This is a JSON string literal that defines the default values for your plugin's custom settings.
    // If `allowUserConfig` is true, the framework will create a `settings.json` file for the plugin,
    // and the JSON object you provide here will be inserted under a top-level key named "settings".
    out_manifest.settingsJson = R"json({
        "a_simple_number": 42,
        "a_slider_number": 50.5,
        "a_drag_number": 10,
        "a_dropdown_choice": "option_b",
        "a_radio_choice": 2,
        "a_color": [0.2, 0.8, 0.4],
        "a_text_note": "This is some default text.\nIt can span multiple lines.",
        "a_complex_object": { "mode": "alpha", "enabled": true, "targets": ["a", "b", "c"] }
    })json";

    // --- 2.4. Default Settings for Framework Systems ---
    // Here you can provide default configurations for various framework systems for your plugin.

    // --- Logging ---
    auto& logging = out_manifest.logging;
    // `level`: The default minimum log level. Can be "trace", "debug", "info", "warn", "error", "critical".
    strncpy_s(logging.level, "info", sizeof(logging.level));
    // `sinks.file`: If `true`, a dedicated log file for this plugin will be created (e.g., `ExamplePlugin/logs/ExamplePlugin.log`).
    // If `false`, this plugin's logs will go to the main framework log file.
    logging.sinks.file = true;

    // --- Localization ---
    auto& localization = out_manifest.localization;
    // `language`: The default language code (e.g., "en", "de", "uk"). This should match the name
    // of your translation file (e.g., `en.json`).
    strncpy_s(localization.language, "en", sizeof(localization.language));

    // --- Keybinds ---
    auto& keybinds = out_manifest.keybinds;
    keybinds.actionCount = 2; // We are defining two distinct actions.
    {
        // First action: Toggle the main window.
        auto& action = keybinds.actions[0];
        // `groupName`: A category for the action. Best practice is "{PluginName}.{Feature}".
        strncpy_s(action.groupName, "ExamplePlugin.MainWindow", sizeof(action.groupName));
        // `actionName`: The specific action, usually a verb.
        strncpy_s(action.actionName, "toggle", sizeof(action.actionName));
        // The full action name becomes "ExamplePlugin.MainWindow.toggle".

        action.definitionCount = 1; // This action has one default key definition.
        auto& def = action.definitions[0];
        strncpy_s(def.type, "keyboard", sizeof(def.type));
        strncpy_s(def.key, "KEY_F5", sizeof(def.key)); // See VirtualKeyMapping.cpp for all key names.
        strncpy_s(def.pressType, "short", sizeof(def.pressType)); // "short" or "long" press.
        def.pressThresholdMs = 300; // For "long" press, the time in ms to hold.
        // `consume`: When to prevent the game from receiving the input. "never", "on_ui_focus", "always".
        strncpy_s(def.consume, "always", sizeof(def.consume));
        // `behavior`: How the action is triggered. "toggle" (on/off), "hold" (while pressed).
        strncpy_s(def.behavior, "toggle", sizeof(def.behavior));
    }
    {
        // Second action: Cycle through camera views.
        auto& action = keybinds.actions[1];
        strncpy_s(action.groupName, "ExamplePlugin.Camera", sizeof(action.groupName));
        strncpy_s(action.actionName, "cycle", sizeof(action.actionName));
        action.definitionCount = 1;
        auto& def = action.definitions[0];
        strncpy_s(def.type, "keyboard", sizeof(def.type));
        strncpy_s(def.key, "KEY_F6", sizeof(def.key));
        strncpy_s(def.pressType, "short", sizeof(def.pressType));
        def.pressThresholdMs = 300;
        strncpy_s(def.consume, "always", sizeof(def.consume));
        strncpy_s(def.behavior, "press", sizeof(def.behavior));
    }

    // --- UI ---
    auto& ui = out_manifest.ui;
    ui.windowsCount = 1; // We are defining one window.
    {
        auto& window = ui.windows[0];
        // `name`: The unique ID for this window within the plugin.
        strncpy_s(window.name, "MainWindow", sizeof(window.name));
        window.isVisible = true;      // Default visibility state.
        window.isInteractive = true;  // If false, the window is see-through to mouse clicks.
        window.posX = 100;            // Default position on the screen.
        window.posY = 100;
        window.sizeW = 400;           // Default size.
        window.sizeH = 300;
        window.isCollapsed = false;   // Default collapsed state.
        window.autoScroll = false;    // If the window should auto-scroll to the bottom on new content.
    }

    // --- 2.5. Metadata for Localization and UI Hints ---
    // This section is optional. It allows you to provide translatable names and descriptions
    // for your settings, keybinds, and UI elements. You can also specify custom UI widgets.
    out_manifest.customSettingsMetadataCount = 8;

    // Example 1: A simple integer input (default behavior).
    // This setting uses the default ImGui::InputInt widget because no specific 'widget' type is provided.
    {
        auto& meta = out_manifest.customSettingsMetadata[0];
        strncpy_s(meta.keyPath, "a_simple_number", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.simple_number.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.simple_number.description", sizeof(meta.descriptionKey));
        meta.widget[0] = '\0'; // No specific widget type, framework will default based on data type.
    }

    // Example 2: A float slider with custom range and format.
    {
        auto& meta = out_manifest.customSettingsMetadata[1];
        strncpy_s(meta.keyPath, "a_slider_number", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.slider_number.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.slider_number.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be a "slider".
        strncpy_s(meta.widget, "slider", sizeof(meta.widget));
        
        // Fill in the corresponding union parameters for a slider.
        // These parameters define the range and display format of the slider.
        meta.widget_params.slider.min_val = 0.0f;
        meta.widget_params.slider.max_val = 100.0f;
        strncpy_s(meta.widget_params.slider.format, "%.1f %%", sizeof(meta.widget_params.slider.format));
    }

    // Example 3: An integer with a draggable input (drag widget).
    {
        auto& meta = out_manifest.customSettingsMetadata[2]; // Index 2 for the third setting
        strncpy_s(meta.keyPath, "a_drag_number", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.drag_number.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.drag_number.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be a "drag" control.
        strncpy_s(meta.widget, "drag", sizeof(meta.widget));
        
        // Fill in the corresponding union parameters for a drag widget.
        // These parameters define the speed of change and the value range.
        meta.widget_params.drag.speed = 0.5f; // How much value changes per pixel when dragging
        meta.widget_params.drag.min_val = -100.0f;
        meta.widget_params.drag.max_val = 100.0f;
        strncpy_s(meta.widget_params.drag.format, "%d units", sizeof(meta.widget_params.drag.format)); // Custom format string
    }

    // Example 4: A dropdown (combo box) for selecting a string option.
    {
        auto& meta = out_manifest.customSettingsMetadata[3]; // Index 3 for the fourth setting
        strncpy_s(meta.keyPath, "a_dropdown_choice", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.dropdown.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.dropdown.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be a "combo" box.
        strncpy_s(meta.widget, "combo", sizeof(meta.widget));
        
        // Provide the options as a JSON string. Each option has a 'value' and a 'labelKey'.
        // The 'value' can be a string or a number. The 'labelKey' is for localization or literal text.
        const char* options = R"json([
            { "value": "option_a", "labelKey": "options.a.title" },
            { "value": "option_b", "labelKey": "options.b.title" },
            { "value": "option_c", "labelKey": "This is a literal label" }
        ])json";
        strncpy_s(meta.widget_params.choice.options_json, options, sizeof(meta.widget_params.choice.options_json));
    }

    // Example 5: Radio buttons for selecting a numeric option.
    {
        auto& meta = out_manifest.customSettingsMetadata[4]; // Index 4 for the fifth setting
        strncpy_s(meta.keyPath, "a_radio_choice", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.radio.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.radio.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be "radio" buttons.
        strncpy_s(meta.widget, "radio", sizeof(meta.widget));
        
        // Radio buttons use the same choice parameters (`options_json`) as combo boxes.
        // Note that the 'value' can be numeric.
        const char* options = R"json([
            { "value": 1, "labelKey": "options.radio_one" },
            { "value": 2, "labelKey": "options.radio_two" },
            { "value": 3, "labelKey": "options.radio_three" }
        ])json";
        strncpy_s(meta.widget_params.choice.options_json, options, sizeof(meta.widget_params.choice.options_json));
    }

    // Example 6: An RGB Color Picker.
    {
        auto& meta = out_manifest.customSettingsMetadata[5]; // Index 5 for the sixth setting
        strncpy_s(meta.keyPath, "a_color", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.color.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.color.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be a "color3" picker (RGB).
        // The setting's value in settingsJson should be an array of 3 floats, e.g., [R, G, B].
        strncpy_s(meta.widget, "color3", sizeof(meta.widget));
        
        // Fill in parameters for a color picker. 'flags' allow for advanced customization.
        // Set to 0 for default color picker behavior.
        meta.widget_params.color.flags = 0;
    }

    // Example 7: A multiline text input field.
    {
        auto& meta = out_manifest.customSettingsMetadata[6]; // Index 6 for the seventh setting
        strncpy_s(meta.keyPath, "a_text_note", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.note.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.note.description", sizeof(meta.descriptionKey));

        // Specify the widget type to be a "multiline" text input.
        strncpy_s(meta.widget, "multiline", sizeof(meta.widget));
        meta.widget_params.multiline.height_in_lines = 4; // Set the text box height to be 4 lines tall.
    }

    // Example 8: A complex object (no widget, for programmatic access).
    {
        auto& meta = out_manifest.customSettingsMetadata[7];
        strncpy_s(meta.keyPath, "a_complex_object", sizeof(meta.keyPath));
        strncpy_s(meta.titleKey, "setting.complex_object.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "setting.complex_object.description", sizeof(meta.descriptionKey));
        meta.widget[0] = '\0'; // No widget, this setting is for internal logic.
    }

    // --- Keybinds Metadata ---
    out_manifest.keybindsMetadataCount = 2;
    {
        auto& meta = out_manifest.keybindsMetadata[0];
        strncpy_s(meta.groupName, "ExamplePlugin.MainWindow", sizeof(meta.groupName));
        strncpy_s(meta.actionName, "toggle", sizeof(meta.actionName));
        strncpy_s(meta.titleKey, "keybind.main_window_toggle.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "keybind.main_window_toggle.description", sizeof(meta.descriptionKey));
    }
    {
        auto& meta = out_manifest.keybindsMetadata[1];
        strncpy_s(meta.groupName, "ExamplePlugin.Camera", sizeof(meta.groupName));
        strncpy_s(meta.actionName, "cycle", sizeof(meta.actionName));
        strncpy_s(meta.titleKey, "keybind.camera_cycle.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "keybind.camera_cycle.description", sizeof(meta.descriptionKey));
    }

    // --- UI Metadata ---
    out_manifest.uiMetadataCount = 1;
    {
        auto& meta = out_manifest.uiMetadata[0];
        strncpy_s(meta.windowName, "MainWindow", sizeof(meta.windowName));
        strncpy_s(meta.titleKey, "ui.window.main_window.title", sizeof(meta.titleKey));
        strncpy_s(meta.descriptionKey, "ui.window.main_window.description", sizeof(meta.descriptionKey));
    }
}

// =================================================================================================
// 3. Plugin Lifecycle
// =================================================================================================
// The following functions are the core lifecycle events for the plugin. The framework calls them
// in a specific order: OnLoad -> OnActivated -> OnUpdate (every frame) -> OnUnload.

/**
 * @brief Called once when the plugin is first loaded into memory.
 * @param load_api A pointer to the `SPF_Load_API`, which provides essential, early-available
 *                 services like logging, configuration, and string formatting.
 * @details This is the ideal place for one-time setup that doesn't depend on other plugins.
 *            Key tasks include:
 *          - Caching the `load_api` pointer in the global context (`g_ctx`).
 *          - Getting a logger instance for the plugin.
 *          - Reading initial configuration values from `settings.json`.
 */
void OnLoad(const SPF_Load_API* load_api) {
    // Cache the provided API pointers in our global context for access in other functions.
    g_ctx.loadAPI = load_api;

    // It's crucial to check if the API pointers are valid before using them. This prevents
    // crashes if the framework fails to provide them for some reason.
    if (g_ctx.loadAPI && g_ctx.loadAPI->logger && g_ctx.loadAPI->config) {
        // Get a handle to our plugin's dedicated logger instance.
        auto logger = g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME);
        g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, "ExamplePlugin has been loaded!");

        // Read initial values from the config file. The `GetContext` call gets a handle
        // specific to our plugin, ensuring we don't conflict with other plugins' settings.
        auto config = g_ctx.loadAPI->config->GetContext(PLUGIN_NAME);
        g_ctx.someNumber = g_ctx.loadAPI->config->GetInt32(config, "settings.a_simple_number", 42);

        // Log the initial value. Use a local buffer for safe cross-DLL string formatting.
        char log_buffer[256];
        g_ctx.loadAPI->formatting->Format(log_buffer, sizeof(log_buffer), "Initial value for 'a_simple_number' is %d.", g_ctx.someNumber);
        g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
    }
}

/**
 * @brief Called once all plugins have been loaded and the framework is fully initialized.
 * @param core_api A pointer to the `SPF_Core_API`, which contains pointers to all other APIs
 *                 (telemetry, camera, input, hooks, etc.).
 * @details This is the main initialization function. It's called after `OnLoad` for all plugins
 *          has completed. Use this function to:
 *          - Cache the `core_api` pointer.
 *          - Register callbacks for keybinds, events, and hooks.
 *          - Initialize more complex features that require the full suite of APIs.
 */
void OnActivated(const SPF_Core_API* core_api) {
    g_ctx.coreAPI = core_api;
    auto logger = g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME);

    // Register callbacks for systems that require the core API.
    if (g_ctx.coreAPI && g_ctx.coreAPI->keybinds) {
        auto keybinds = g_ctx.coreAPI->keybinds->GetContext(PLUGIN_NAME);
        g_ctx.coreAPI->keybinds->Register(keybinds, "ExamplePlugin.MainWindow.toggle", OnToggleMainWindow);
        g_ctx.coreAPI->keybinds->Register(keybinds, "ExamplePlugin.Camera.cycle", OnCameraKeybind);
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Registered keybinds.");
    }

    if (g_ctx.coreAPI && g_ctx.coreAPI->gamelog) {
        g_ctx.gameLogCallbackHandle = g_ctx.coreAPI->gamelog->RegisterCallback(PLUGIN_NAME, OnGameLogMessage, nullptr);
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Registered game log callback.");
    }

    // Initialize more complex features that need the core API.
    InitializeVirtualDevice();
    InstallGameStringFormattingHook();

    // Parse the complex object on activation to demonstrate GetJsonValueHandle and JsonReaderApi.
    ParseComplexObject();

    // --- Telemetry Event Example ---
    // Get a handle for the telemetry API and register our callbacks.
    if (g_ctx.coreAPI && g_ctx.coreAPI->telemetry) {
        g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);
        if (g_ctx.telemetryHandle) {
            auto tel = g_ctx.coreAPI->telemetry;
            g_ctx.gameStateCallback = tel->RegisterForGameState(g_ctx.telemetryHandle, OnGameStateUpdate, &g_ctx);
            g_ctx.timestampsCallback = tel->RegisterForTimestamps(g_ctx.telemetryHandle, OnTimestampsUpdate, &g_ctx);
            g_ctx.commonDataCallback = tel->RegisterForCommonData(g_ctx.telemetryHandle, OnCommonDataUpdate, &g_ctx);
            g_ctx.truckConstantsCallback = tel->RegisterForTruckConstants(g_ctx.telemetryHandle, OnTruckConstantsUpdate, &g_ctx);
            g_ctx.trailerConstantsCallback = tel->RegisterForTrailerConstants(g_ctx.telemetryHandle, OnTrailerConstantsUpdate, &g_ctx);
            g_ctx.truckDataCallback = tel->RegisterForTruckData(g_ctx.telemetryHandle, OnTruckDataUpdate, &g_ctx);
            g_ctx.trailersCallback = tel->RegisterForTrailers(g_ctx.telemetryHandle, OnTrailersUpdate, &g_ctx);
            g_ctx.jobConstantsCallback = tel->RegisterForJobConstants(g_ctx.telemetryHandle, OnJobConstantsUpdate, &g_ctx);
            g_ctx.jobDataCallback = tel->RegisterForJobData(g_ctx.telemetryHandle, OnJobDataUpdate, &g_ctx);
            g_ctx.navigationDataCallback = tel->RegisterForNavigationData(g_ctx.telemetryHandle, OnNavigationDataUpdate, &g_ctx);
            g_ctx.controlsCallback = tel->RegisterForControls(g_ctx.telemetryHandle, OnControlsUpdate, &g_ctx);
            g_ctx.specialEventsCallback = tel->RegisterForSpecialEvents(g_ctx.telemetryHandle, OnSpecialEventsUpdate, &g_ctx);
            g_ctx.gameplayEventsCallback = tel->RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvent, &g_ctx);
            g_ctx.gearboxConstantsCallback = tel->RegisterForGearboxConstants(g_ctx.telemetryHandle, OnGearboxConstantsUpdate, &g_ctx);

            g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Registered all telemetry callbacks.");
        }
    }
}
/**
 * @brief (Optional) Called once when the game world has been fully loaded.
 * @details This is the ideal place to initialize features that require the game to be
 *          "in-game", such as camera hooks, interacting with vehicle data, etc.
 *          It provides a reliable signal that it's safe to access game world objects.
 */
void OnGameWorldReady() {
    if (g_ctx.coreAPI && g_ctx.coreAPI->logger) {
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO,
                                  "OnGameWorldReady called! Game world is loaded and ready.");
        
        // Example: Now would be a good time to find camera offsets or install
        // hooks that depend on game objects being in memory.
    }
}
/**
 * @brief Called every frame while the plugin is active.
 * @details This function is the main "tick" or "update" loop for the plugin. It's called
 *          continuously.
 * @warning Avoid performing heavy or blocking operations here, as it will directly impact
 *          game performance. For frequent logging, use `LogThrottled` to avoid spamming
 *          the log file.
 */
void OnUpdate() {
    // Example of throttled logging: This message will only be logged at most once every
    // 3000 milliseconds (2 seconds), even though OnUpdate is called every frame.
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->logger || !g_ctx.coreAPI->formatting) {
        return;
    }

    auto logger = g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME);
    auto format = g_ctx.coreAPI->formatting;

    char full_log_buffer[16384];
    full_log_buffer[0] = '\0';

    char temp_line_buffer[1024];

    auto strcat_safe = [&](const char* src) {
        strcat_s(full_log_buffer, sizeof(full_log_buffer), src);
    };

    strcat_safe("--- BEGIN EXHAUSTIVE EVENT CACHE LOG (Throttled) ---\n");

    // --- GameState ---
    {
        const auto& data = g_ctx.eventDataCache.gameState;
        strcat_safe("[GameState]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game ID: %s (%s)\n", data.game_id, data.game_name); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game Version: %u.%u\n", data.scs_game_version_major, data.scs_game_version_minor); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Telemetry Version: %u.%u\n", data.telemetry_game_version_major, data.telemetry_game_version_minor); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Paused: %s, Scale: %.2f, MP Time Offset: %d\n", data.paused ? "Yes" : "No", data.scale, data.multiplayer_time_offset); strcat_safe(temp_line_buffer);
    }

    // --- Timestamps ---
    {
        const auto& data = g_ctx.eventDataCache.timestamps;
        strcat_safe("[Timestamps]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Sim: %llu, Render: %llu, Paused Sim: %llu\n", data.simulation, data.render, data.paused_simulation); strcat_safe(temp_line_buffer);
    }

    // --- CommonData ---
    {
        const auto& data = g_ctx.eventDataCache.commonData;
        strcat_safe("[CommonData]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game Time: %u, Next Rest: %d min\n", data.game_time, data.next_rest_stop); strcat_safe(temp_line_buffer);
    }

    // --- TruckConstants ---
    {
        const auto& data = g_ctx.eventDataCache.truckConstants;
        strcat_safe("[TruckConstants]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Truck: %s %s (%s, %s)\n", data.brand, data.name, data.brand_id, data.id); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  License: %s (%s, %s)\n", data.license_plate, data.license_plate_country, data.license_plate_country_id); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Drivetrain: %u Fwd, %u Rev, RPM Limit: %.0f, Diff Ratio: %.2f\n", data.forward_gear_count, data.reverse_gear_count, data.rpm_limit, data.differential_ratio); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Capacities: Fuel: %.1f L, AdBlue: %.1f L\n", data.fuel_capacity, data.adblue_capacity); strcat_safe(temp_line_buffer);
        for (uint32_t i = 0; i < data.wheel_count; ++i) {
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Wheel %u: Radius=%.3f, Steerable=%d, Powered=%d, Liftable=%d\n", i, data.wheels[i].radius, data.wheels[i].steerable, data.wheels[i].powered, data.wheels[i].liftable); strcat_safe(temp_line_buffer);
        }
    }

    // --- TruckData ---
    {
        const auto& data = g_ctx.eventDataCache.truckData;
        strcat_safe("[TruckData]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  World Pos: (%.2f, %.2f, %.2f)\n", data.world_placement.position.x, data.world_placement.position.y, data.world_placement.position.z); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Speed: %.1f kph, RPM: %.0f\n", data.speed * 3.6f, data.engine_rpm); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Gear: %d (Displayed: %d), Cruise Control: %.1f kph\n", data.gear, data.displayed_gear, data.cruise_control_speed * 3.6f); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Brakes: Parking=%d, Motor=%d, Retarder=%u, Temp: %.1f C\n", data.parking_brake, data.motor_brake, data.retarder_level, data.brake_temperature); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Pressures: Air=%.1f psi, Oil=%.1f psi\n", data.air_pressure, data.oil_pressure); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Temps: Water=%.1f C, Oil=%.1f C\n", data.water_temperature, data.oil_temperature); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Fluids: Fuel=%.1f L, AdBlue=%.1f L\n", data.fuel_amount, data.adblue_amount); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Lights: L=%d R=%d, Park=%d, Low=%d, High=%d, Beacon=%d\n", data.lblinker, data.rblinker, data.light_parking, data.light_low_beam, data.light_high_beam, data.light_beacon); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Wear: Eng=%.3f, Trans=%.3f, Cab=%.3f, Chas=%.3f, Wheels=%.3f\n", data.wear_engine, data.wear_transmission, data.wear_cabin, data.wear_chassis, data.wear_wheels); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Odometer: %.1f km\n", data.odometer); strcat_safe(temp_line_buffer);
    }

    // --- Trailers ---
    {
        const auto& data = g_ctx.eventDataCache.trailers;
        strcat_safe("[Trailers]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Count: %zu\n", data.size()); strcat_safe(temp_line_buffer);
        for (size_t i = 0; i < data.size(); ++i) {
            const auto& trailer = data[i];
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Trailer %zu: %s (%s) Conn: %d\n", i, trailer.constants.name, trailer.constants.id, trailer.data.connected); strcat_safe(temp_line_buffer);
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "      Damage: Cargo=%.3f, Chassis=%.3f, Wheels=%.3f\n", trailer.data.cargo_damage, trailer.data.wear_chassis, trailer.data.wear_wheels); strcat_safe(temp_line_buffer);
            for (uint32_t j = 0; j < trailer.constants.wheel_count; ++j) {
                const auto& wheel_data = trailer.data.wheels[j];
                const auto& wheel_const = trailer.constants.wheels[j];
                format->Format(temp_line_buffer, sizeof(temp_line_buffer), "      Trailer Wheel %u: R=%.3f, Defl=%.3f, Ground=%d, Vel=%.2f\n", j, wheel_const.radius, wheel_data.suspension_deflection, wheel_data.on_ground, wheel_data.angular_velocity); strcat_safe(temp_line_buffer);
            }
        }
    }

    // --- Job ---
    {
        const auto& job_const = g_ctx.eventDataCache.jobConstants;
        const auto& job_data = g_ctx.eventDataCache.jobData;
        strcat_safe("[Job]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  On Job: %s\n", job_data.on_job ? "Yes" : "No"); strcat_safe(temp_line_buffer);
        if (job_data.on_job) {
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Cargo: %s (%s), Mass: %.0f kg\n", job_const.cargo_name, job_const.cargo_id, job_const.cargo_mass); strcat_safe(temp_line_buffer);
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Route: %s -> %s\n", job_const.source_city, job_const.destination_city); strcat_safe(temp_line_buffer);
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Company: %s -> %s\n", job_const.source_company, job_const.destination_company); strcat_safe(temp_line_buffer);
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Income: %llu, Market: %s\n", job_const.income, job_const.job_market); strcat_safe(temp_line_buffer);
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    Time Left: %u min, Cargo Dmg: %.3f\n", job_data.remaining_delivery_minutes, job_data.cargo_damage); strcat_safe(temp_line_buffer);
        }
    }

    // --- Navigation ---
    {
        const auto& data = g_ctx.eventDataCache.navigationData;
        strcat_safe("[Navigation]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Distance: %.0f m, Time: %.0f s (%.1f real s), Speed Limit: %.0f kph\n", data.navigation_distance, data.navigation_time, data.navigation_time_real_seconds, data.navigation_speed_limit * 3.6f); strcat_safe(temp_line_buffer);
    }

    // --- Controls ---
    {
        const auto& data = g_ctx.eventDataCache.controls;
        strcat_safe("[Controls]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  User: Thr=%.2f, Brk=%.2f, Steer=%.2f, Clutch=%.2f\n", data.userInput.throttle, data.userInput.brake, data.userInput.steering, data.userInput.clutch); strcat_safe(temp_line_buffer);
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Effective: Thr=%.2f, Brk=%.2f, Steer=%.2f, Clutch=%.2f\n", data.effectiveInput.throttle, data.effectiveInput.brake, data.effectiveInput.steering, data.effectiveInput.clutch); strcat_safe(temp_line_buffer);
    }

    // --- GameplayEvents ---
    {
        const auto& event_id = g_ctx.eventDataCache.lastGameplayEventId;
        const auto& data = g_ctx.eventDataCache.gameplayEvents;
        strcat_safe("[GameplayEvents]\n");
        format->Format(temp_line_buffer, sizeof(temp_line_buffer), "  Last Event ID: %s\n", event_id); strcat_safe(temp_line_buffer);
        if (strcmp(event_id, "player.fined") == 0) {
            format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Fine Details: Amount=%lld, Offence=%s\n", data.player_fined.fine_amount, data.player_fined.fine_offence); strcat_safe(temp_line_buffer);
        } else if (strcmp(event_id, "job.delivered") == 0) {
             format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Job Delivered: Revenue=%lld, XP=%d, Dist=%.1f km\n", data.job_delivered.revenue, data.job_delivered.earned_xp, data.job_delivered.distance_km); strcat_safe(temp_line_buffer);
        } else if (strcmp(event_id, "job.cancelled") == 0) {
             format->Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Job Cancelled: Penalty=%lld\n", data.job_cancelled.penalty); strcat_safe(temp_line_buffer);
        }
    }

    strcat_safe("--- END EXHAUSTIVE EVENT CACHE LOG ---\n");


    g_ctx.coreAPI->logger->LogThrottled(
        logger,
        SPF_LOG_INFO,
        "ExamplePlugin.full_event_cache.log",
        3000,
        full_log_buffer
    );
}

/**
 * @brief Called once when the plugin is about to be unloaded from memory.
 * @details This is the last chance to perform cleanup. Key tasks include:
 *          - Unregistering callbacks and hooks.
 *          - Freeing any allocated memory.
 *          - Nulling out cached API pointers to prevent use-after-free errors.
 */
void OnUnload() {
    if (g_ctx.loadAPI && g_ctx.loadAPI->logger) {
        g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, "ExamplePlugin is being unloaded.");
    }

    // It's good practice to null out all cached pointers on unload. This helps prevent
    // accidental use-after-free if another part of the code attempts to access them
    // after the plugin has been told to shut down.
    g_ctx.telemetryHandle = nullptr;
    g_ctx.gameStateCallback = nullptr;
    g_ctx.timestampsCallback = nullptr;
    g_ctx.commonDataCallback = nullptr;
    g_ctx.truckConstantsCallback = nullptr;
    g_ctx.trailerConstantsCallback = nullptr;
    g_ctx.truckDataCallback = nullptr;
    g_ctx.trailersCallback = nullptr;
    g_ctx.jobConstantsCallback = nullptr;
    g_ctx.jobDataCallback = nullptr;
    g_ctx.navigationDataCallback = nullptr;
    g_ctx.controlsCallback = nullptr;
    g_ctx.specialEventsCallback = nullptr;
    g_ctx.gameplayEventsCallback = nullptr;
    g_ctx.gearboxConstantsCallback = nullptr;

    g_ctx.mainWindowHandle = nullptr;
    g_ctx.virtualDevice = nullptr;
    g_ctx.uiAPI = nullptr;
    g_ctx.coreAPI = nullptr;
    g_ctx.loadAPI = nullptr;
}

// =================================================================================================
// 4. Framework Callbacks
// =================================================================================================
// These functions are callbacks that the plugin registers to be notified of specific events
// by the framework, such as a setting changing, a key being pressed, or a game event occurring.

/**
 * @brief Called by the framework when a setting relevant to this plugin is changed.
 * @param keyPath The full path of the setting that changed (e.g., "settings.a_simple_number").
 * @param value_handle A handle to the new JSON value of the setting.
 * @param json_reader A pointer to the JSON Reader API, used to extract the data from `value_handle`.
 * @details This function allows the plugin to react dynamically to configuration changes made by
 *          the user through the UI.
 */
void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
    // Check which setting has changed and react accordingly.
    if (strcmp(keyPath, "settings.a_simple_number") == 0) {
        // Update the cached value in our global context using the ConfigApi.
        g_ctx.someNumber = g_ctx.loadAPI->config->GetInt32(config_handle, "settings.a_simple_number", 42);

        // Log the change for debugging purposes.
        char log_buffer[256];
        g_ctx.loadAPI->formatting->Format(log_buffer, sizeof(log_buffer), "'a_simple_number' was changed externally. New value: %d", g_ctx.someNumber);
        g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    } else if (strcmp(keyPath, "settings.a_complex_object") == 0) {
        // The complex object setting has changed. Re-parse it.
        // This demonstrates the use of GetJsonValueHandle and JsonReaderApi.
        ParseComplexObject();
    }
}

/**
 * @brief Registered with the GameLogHook, this is called for each new line added to the game's log.
 * @param log_line The content of the log line.
 * @param user_data A custom pointer passed during registration (not used here).
 * @details This demonstrates how to listen to game events by monitoring the game's own logging.
 *          It's a powerful way to react to game state changes that don't have a dedicated API.
 * @warning This callback can be frequent. Avoid complex processing here.
 */
void OnGameLogMessage(const char* log_line, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->logger || !log_line) return;

    // Example: Log a message to our own plugin log if we see a specific message in the game log.
    if (strstr(log_line, "Loaded")) {
        char buffer[4096];
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Game Log contains 'Loaded': %s", log_line);
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, buffer);
    }
}

/**
 * @brief The callback function for the "ExamplePlugin.MainWindow.toggle" keybind action.
 * @details This function was registered with the Keybinds API in `OnActivated`. It is executed
 *          whenever the user presses the key combination assigned to this action (F5 by default).
 */
void OnToggleMainWindow() {
    if (g_ctx.uiAPI && g_ctx.mainWindowHandle) {
        // Read the current visibility state directly from the framework.
        const bool isCurrentlyVisible = g_ctx.uiAPI->IsVisible(g_ctx.mainWindowHandle);
        // Instruct the UI API to apply the inverse of the current state.
        g_ctx.uiAPI->SetVisibility(g_ctx.mainWindowHandle, !isCurrentlyVisible);

        char log_buffer[256];
        g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "Main window visibility toggled to: %s", !isCurrentlyVisible ? "visible" : "hidden");
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    }
}

/**
 * @brief The callback function for the "ExamplePlugin.Camera.cycle" keybind action.
 * @details This function was registered with the Keybinds API in `OnActivated`. It is executed
 *          whenever the user presses the key combination assigned to this action (F6 by default).
 */
void OnCameraKeybind() {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->camera) return;

    SPF_CameraType current_camera_type;
    if (g_ctx.coreAPI->camera->GetCurrentCamera(&current_camera_type)) {
        // Determine the next camera in the cycle.
        SPF_CameraType next_camera_type = (current_camera_type == SPF_CAMERA_INTERIOR) ? SPF_CAMERA_BEHIND :
                                          (current_camera_type == SPF_CAMERA_BEHIND) ? SPF_CAMERA_DEVELOPER_FREE :
                                          SPF_CAMERA_INTERIOR;
        // Switch to the next camera.
        g_ctx.coreAPI->camera->SwitchTo(next_camera_type);

        char log_buffer[256];
        g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "Switched camera from %d to %d via keybind.", current_camera_type, next_camera_type);
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    } else {
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_WARN, "Could not get current camera type to cycle.");
    }
}

// --- Telemetry Event Callbacks ---

/**
 * @brief Callback function for the GameState event.
 * @details This function is invoked by the framework whenever the game's state (e.g., paused status,
 * time scale) is updated. It receives the updated `SPF_GameState` data and a `user_data` pointer.
 * The `user_data` is used to access the plugin's global context (`g_ctx`), allowing the function
 * to store the latest game state data in `g_ctx.eventDataCache` for display in the UI.
 * @param data A pointer to the `SPF_GameState` structure containing the updated game state information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnGameStateUpdate(const SPF_GameState* data, void* user_data) {
    if (!data || !user_data) return; // Always check for null pointers to prevent crashes.
    auto* ctx = reinterpret_cast<PluginContext*>(user_data); // Cast user_data back to our context type.
    ctx->eventDataCache.gameState = *data; // Store the latest data in our cache.
}

/**
 * @brief Callback function for the Timestamps event.
 * @details This function is invoked by the framework whenever the game's timestamps data is updated.
 * It receives the updated `SPF_Timestamps` data and a `user_data` pointer.
 * The `user_data` is used to access the plugin's global context (`g_ctx`), allowing the function
 * to store the latest timestamp data in `g_ctx.eventDataCache` for display in the UI.
 * @param data A pointer to the `SPF_Timestamps` structure containing the updated timestamp information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnTimestampsUpdate(const SPF_Timestamps* data, void* user_data) {
    if (!data || !user_data) return; // Always check for null pointers to prevent crashes.
    auto* ctx = reinterpret_cast<PluginContext*>(user_data); // Cast user_data back to our context type.
    ctx->eventDataCache.timestamps = *data; // Store the latest data in our cache.
}

/**
 * @brief Callback function for the CommonData event.
 * @details This function is invoked by the framework whenever common game data (e.g., game time,
 * next rest stop) is updated. It receives the updated `SPF_CommonData` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest common data in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_CommonData` structure containing the updated common game information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnCommonDataUpdate(const SPF_CommonData* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.commonData = *data;
}

/**
 * @brief Callback function for the TruckConstants event.
 * @details This function is invoked by the framework when the truck's static configuration
 * (e.g., brand, model, wheel count) changes. It receives the updated `SPF_TruckConstants`
 * and a `user_data` pointer. The `user_data` allows access to `g_ctx` to store the latest
 * truck constants in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_TruckConstants` structure containing the updated truck
 *             configuration information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnTruckConstantsUpdate(const SPF_TruckConstants* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.truckConstants = *data;
}

/**
 * @brief Callback function for the TrailerConstants event.
 * @details This function is invoked by the framework when the static configuration of a trailer
 * (e.g., brand, model, wheel count) changes. It receives the updated `SPF_TrailerConstants`
 * and a `user_data` pointer. The `user_data` allows access to `g_ctx` to store the latest
 * trailer constants in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_TrailerConstants` structure containing the updated trailer
 *             configuration information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnTrailerConstantsUpdate(const SPF_TrailerConstants* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.trailerConstants = *data;
}

/**
 * @brief Callback function for the TruckData event.
 * @details This function is invoked by the framework frequently with updated dynamic truck data
 * (e.g., speed, RPM, fuel). It receives the updated `SPF_TruckData` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest truck data in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_TruckData` structure containing the updated dynamic truck information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnTruckDataUpdate(const SPF_TruckData* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.truckData = *data;
}

/**
 * @brief Callback function for the Trailers event.
 * @details This function is invoked by the framework when the list of attached trailers
 * or their dynamic data changes. It receives a pointer to an array of `SPF_Trailer`
 * structures and the count of trailers, along with a `user_data` pointer. The `user_data`
 * allows access to `g_ctx` to store the latest trailer data in `g_ctx.eventDataCache.trailers`.
 * The vector is cleared and re-populated to reflect the current state.
 * @param data A pointer to the array of `SPF_Trailer` structures containing the updated trailer data.
 * @param count The number of trailers in the `data` array.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnTrailersUpdate(const SPF_Trailer* data, uint32_t count, void* user_data) {
    if (!user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);

    ctx->eventDataCache.trailers.clear(); // Clear previous data.
    if (data && count > 0) {
        // Copy each trailer from the C-style array into the C++ vector.
        for (uint32_t i = 0; i < count; ++i) {
            ctx->eventDataCache.trailers.push_back(data[i]);
        }
    }
}

/**
 * @brief Callback function for the JobConstants event.
 * @details This function is invoked by the framework when the current job's static configuration
 * (e.g., cargo, destination, income) changes. It receives the updated `SPF_JobConstants`
 * and a `user_data` pointer. The `user_data` allows access to `g_ctx` to store the latest
 * job constants in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_JobConstants` structure containing the updated job
 *             configuration information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnJobConstantsUpdate(const SPF_JobConstants* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.jobConstants = *data;
}

/**
 * @brief Callback function for the JobData event.
 * @details This function is invoked by the framework when dynamic job data (e.g., cargo damage,
 * remaining delivery time) is updated. It receives the updated `SPF_JobData` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest job data in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_JobData` structure containing the updated dynamic job information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnJobDataUpdate(const SPF_JobData* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.jobData = *data;
}

/**
 * @brief Callback function for the NavigationData event.
 * @details This function is invoked by the framework when navigation data (e.g., remaining distance,
 * time to arrival, speed limit) is updated. It receives the updated `SPF_NavigationData` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest navigation data in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_NavigationData` structure containing the updated navigation information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnNavigationDataUpdate(const SPF_NavigationData* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.navigationData = *data;
}

/**
 * @brief Callback function for the Controls event.
 * @details This function is invoked by the framework when player control inputs (e.g., steering,
 * throttle, brake) are updated. It receives the updated `SPF_Controls` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest controls data in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_Controls` structure containing the updated control input information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnControlsUpdate(const SPF_Controls* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.controls = *data;
}

/**
 * @brief Callback function for the SpecialEvents event.
 * @details This function is invoked by the framework when single-frame events like fines,
 * tollgates, or job completion occur. It receives the updated `SPF_SpecialEvents` and a `user_data` pointer.
 * The `user_data` allows access to `g_ctx` to store the latest event flags in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_SpecialEvents` structure containing the updated event flags.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnSpecialEventsUpdate(const SPF_SpecialEvents* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.specialEvents = *data;
}

/**
 * @brief Callback function for the GameplayEvents event.
 * @details This function is invoked by the framework when a specific gameplay event occurs (e.g.,
 * a fine is issued, a job is delivered). It receives a string ID for the event, a data payload
 * with event-specific details, and a `user_data` pointer. The function stores both the event ID
 * and the data payload in the `g_ctx.eventDataCache`.
 * @param event_id A string identifying the type of gameplay event (e.g., "player.fined").
 * @param data A pointer to the `SPF_GameplayEvents` structure containing the data for the event.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnGameplayEvent(const char* event_id, const SPF_GameplayEvents* data, void* user_data) {
    if (!event_id || !data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);

    // Copy the event data payload.
    ctx->eventDataCache.gameplayEvents = *data;
    // Copy the event ID string safely into our cache.
    strncpy_s(ctx->eventDataCache.lastGameplayEventId, event_id, sizeof(ctx->eventDataCache.lastGameplayEventId));
}

/**
 * @brief Callback function for the GearboxConstants event.
 * @details This function is invoked by the framework when the truck's gearbox configuration
 * (e.g., shifter type, slot layout) changes. It receives the updated `SPF_GearboxConstants`
 * and a `user_data` pointer. The `user_data` allows access to `g_ctx` to store the latest
 * gearbox constants in `g_ctx.eventDataCache`.
 * @param data A pointer to the `SPF_GearboxConstants` structure containing the updated
 *             gearbox configuration information.
 * @param user_data A pointer to the plugin's global `PluginContext` (g_ctx), allowing state updates.
 */
void OnGearboxConstantsUpdate(const SPF_GearboxConstants* data, void* user_data) {
    if (!data || !user_data) return;
    auto* ctx = reinterpret_cast<PluginContext*>(user_data);
    ctx->eventDataCache.gearboxConstants = *data;
}

// =================================================================================================
// 5. UI Implementation
// =================================================================================================
// This section contains all functions related to the plugin's user interface.

/**
 * @brief Called once by the framework to allow the plugin to register its UI elements.
 * @param ui_api A pointer to the `SPF_UI_API`, used for registering draw callbacks and
 *               interacting with the UI system.
 * @details This is the entry point for all UI-related setup. Here, you should:
 *          - Cache the `ui_api` pointer.
 *          - Register a draw callback for each window your plugin owns.
 *          - Get and cache handles to your windows for later manipulation (e.g., toggling visibility).
 */
void OnRegisterUI(SPF_UI_API* ui_api) {
    if (ui_api) {
        g_ctx.uiAPI = ui_api;
        // Register our main rendering function (`RenderMainWindow`) to be called for the window
        // identified by `PLUGIN_NAME` and "MainWindow".
        ui_api->RegisterDrawCallback(PLUGIN_NAME, "MainWindow", RenderMainWindow, nullptr);

        // Get and cache the handle to our window for efficient access later.
        g_ctx.mainWindowHandle = g_ctx.uiAPI->GetWindowHandle(PLUGIN_NAME, "MainWindow");
    }
}

/**
 * @brief The main rendering callback for the plugin's primary window.
 * @param ui A pointer to the `SPF_UI_API`, used to draw ImGui widgets.
 * @param user_data A custom pointer passed during registration (not used here).
 * @details This function is called every frame that the window is visible. It uses the
 *          provided `ui` pointer, which wraps ImGui functions, to draw the window's contents.
 *          The window's Begin/End calls are handled by the framework.
 */
void RenderMainWindow(SPF_UI_API* ui, void* user_data) {
    // The window title is handled automatically by the framework. It looks for a localization key
    // in the format `{window_name}.title` (e.g., "MainWindow.title"). If not found, it defaults
    // to the window name itself.

    // A tab bar is a good way to organize a complex UI.
    if (ui->BeginTabBar("##MainWindowTabs")) {
        if (ui->BeginTabItem("General")) {
            ui->Text("Hello from the ExamplePlugin window!");

            // Example of getting and displaying a translated string.
            char welcome_msg[256];
            g_ctx.loadAPI->localization->GetString(g_ctx.loadAPI->localization->GetContext(PLUGIN_NAME), "messages.welcome", welcome_msg, sizeof(welcome_msg));
            ui->Text(welcome_msg);
            ui->Separator();

            // --- Config UI Example ---
            ui->Text("This slider modifies a value in settings.json.");
            if (ui->SliderInt("Some Number", &g_ctx.someNumber, 0, 100, "%d")) {
                // If the slider is moved, update the configuration file.
                g_ctx.loadAPI->config->SetInt32(g_ctx.loadAPI->config->GetContext(PLUGIN_NAME), "settings.a_simple_number", g_ctx.someNumber);
                g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, "User changed 'a_simple_number' via UI.");
            }
            ui->Separator();

            // --- Game Console Example ---
            ui->Text("Enter a command to execute in the in-game console:");
            ui->InputText("##ConsoleCommand", g_ctx.consoleCommand, sizeof(g_ctx.consoleCommand));
            ui->SameLine(0, 0);
            if (ui->Button("Execute", 0, 0)) {
                if (g_ctx.coreAPI && g_ctx.coreAPI->console && g_ctx.consoleCommand[0] != '\0') {
                    g_ctx.coreAPI->console->ExecuteCommand(g_ctx.consoleCommand);
                    char log_buffer[512];
                    g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "Executed console command: '%s'", g_ctx.consoleCommand);
                    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
                }
            }
            ui->Separator();

            // --- Hook Example ---
            ui->Text("This checkbox controls a function hook:");
            ui->Checkbox("Make 'Quit' button red", &g_ctx.isModificationActive);
            ui->EndTabItem();
        }
        // Render the content of other tabs by calling their respective functions.
        if (ui->BeginTabItem("Camera")) { RenderCameraTab(ui, user_data); ui->EndTabItem(); }
        if (ui->BeginTabItem("Telemetry")) { RenderTelemetryTab(ui, user_data); ui->EndTabItem(); }
        if (ui->BeginTabItem("Events")) { RenderEventsTab(ui, user_data); ui->EndTabItem(); }
        if (ui->BeginTabItem("Virtual Input")) { RenderVirtInputTab(ui, user_data); ui->EndTabItem(); }
        ui->EndTabBar();
    }
}

/**
 * @brief Renders the content for the "Camera" tab in the main window.
 */
void RenderCameraTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->camera || !ui) {
        ui->Text("Camera API is not available.");
        return;
    }
    ui->Text("Use this tab to interact with the game's camera system.");
    ui->Text("You can also press F6 to cycle through the cameras.");
    ui->Separator();

    // Get and display the current camera type.
    SPF_CameraType current_camera;
    if (g_ctx.coreAPI->camera->GetCurrentCamera(&current_camera)) {
        char buffer[256];
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Current Camera Type: %d", current_camera);
        ui->Text(buffer);
    } else {
        ui->Text("Could not retrieve current camera type.");
    }
    ui->Separator();

    // Add buttons to switch to a specific camera.
    ui->Text("Switch to a specific camera:");
    if (ui->Button("Interior", 0, 0)) g_ctx.coreAPI->camera->SwitchTo(SPF_CAMERA_INTERIOR);
    ui->SameLine(0, 5);
    if (ui->Button("Behind", 0, 0)) g_ctx.coreAPI->camera->SwitchTo(SPF_CAMERA_BEHIND);
    ui->SameLine(0, 5);
    if (ui->Button("Developer Free", 0, 0)) g_ctx.coreAPI->camera->SwitchTo(SPF_CAMERA_DEVELOPER_FREE);
    ui->Separator();

    // Get and display the camera's world coordinates.
    float x, y, z;
    if (g_ctx.coreAPI->camera->GetCameraWorldCoordinates(&x, &y, &z)) {
        char buffer[256];
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "X: %.2f, Y: %.2f, Z: %.2f", x, y, z);
        ui->Text("Current Camera Position:");
        ui->Text(buffer);
    } else {
        ui->Text("Could not get camera world coordinates.");
    }
}

/**
 * @brief Renders the content for the "Telemetry" tab in the main window.
 */
void RenderTelemetryTab(SPF_UI_API* ui, void* user_data) {
    // --- Telemetry Polling vs. Event-Driven ---
    // This tab demonstrates direct polling of telemetry data using Get...() functions.
    // While this works, for high-frequency data updates (like per-frame rendering),
    // it is generally more efficient to use the event-driven callback mechanism
    // (as shown in OnActivated where callbacks are registered for OnTruckDataUpdate, etc., which update g_ctx.eventDataCache).
    // The event-driven approach means your plugin only reacts when data actually changes,
    // rather than constantly asking for it.
    // Use Get...() for infrequent snapshots or specific UI displays, but prefer callbacks
    // for continuous, performance-critical data handling.
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->telemetry || !ui) {
        ui->Text("Telemetry API is not available.");
        return;
    }
    ui->Text("This tab displays live data from the Telemetry API.");
    ui->Separator();

    char buffer[256];
    auto telemetry = g_ctx.coreAPI->telemetry->GetContext(PLUGIN_NAME);

    // Display truck data.
    SPF_TruckData truck_data;
    g_ctx.coreAPI->telemetry->GetTruckData(telemetry, &truck_data);
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Speed: %.0f kph", truck_data.speed * 3.6f);
    ui->Text(buffer);
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Engine RPM: %.0f", truck_data.engine_rpm);
    ui->Text(buffer);
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Gear: %d", truck_data.displayed_gear);
    ui->Text(buffer);
    ui->Separator();

    // Display job data.
    SPF_JobConstants job_constants;
    g_ctx.coreAPI->telemetry->GetJobConstants(telemetry, &job_constants);
    SPF_JobData job_data;
    g_ctx.coreAPI->telemetry->GetJobData(telemetry, &job_data);
    if (job_data.on_job) {
        ui->Text("Currently on a job!");
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Cargo: %s", job_constants.cargo_name);
        ui->Text(buffer);
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Destination: %s, %s", job_constants.destination_company, job_constants.destination_city);
        ui->Text(buffer);
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Cargo Damage: %.1f%%", job_data.cargo_damage * 100.0f);
        ui->Text(buffer);
    } else {
        ui->Text("Not currently on a job.");
    }
}

void RenderEventsTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !ui) {
        ui->Text("Core API not available.");
        return;
    }
    ui->Text("This tab displays the last data received from event callbacks.");
    ui->Separator();

    char buffer[512];

    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "Last Gameplay Event: %s", g_ctx.eventDataCache.lastGameplayEventId);
    ui->Text(buffer);
    ui->Separator();

    ui->Text("Game State:");
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "  Paused: %s", g_ctx.eventDataCache.gameState.paused ? "Yes" : "No");
    ui->Text(buffer);
    ui->Separator();

    ui->Text("Truck Data:");
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "  Speed: %.0f kph", g_ctx.eventDataCache.truckData.speed * 3.6f);
    ui->Text(buffer);
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "  Engine RPM: %.0f", g_ctx.eventDataCache.truckData.engine_rpm);
    ui->Text(buffer);
    ui->Separator();

    ui->Text("Trailer Info:");
    g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "  Attached Trailers: %zu", g_ctx.eventDataCache.trailers.size());
    ui->Text(buffer);
    if (!g_ctx.eventDataCache.trailers.empty()) {
        g_ctx.coreAPI->formatting->Format(buffer, sizeof(buffer), "  Trailer 1 Brand: %s", g_ctx.eventDataCache.trailers[0].constants.brand);
        ui->Text(buffer);
    }
}

/**
 * @brief Renders the content for the "Virtual Input" tab in the main window.
 */
void RenderVirtInputTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->input || !g_ctx.virtualDevice || !ui) {
        ui->Text("Virtual Input API not available or device not initialized.");
        return;
    }
    ui->Text("Use the controls below to simulate input.");
    ui->Text("You must bind 'Virtual Honk' and 'Virtual Throttle' in the game's keybinding menu.");
    ui->Separator();

    // Example of a virtual button.
    ui->Text("Virtual Honk Button:");
    ui->Button("Hold to Honk", 0, 0); // The button itself is just for show.
    if (ui->IsItemActive()) {
        // While the ImGui button is held down, press the virtual button.
        g_ctx.coreAPI->input->PressButton(g_ctx.virtualDevice, "virt_honk");
    } else {
        // When the ImGui button is released, release the virtual button.
        g_ctx.coreAPI->input->ReleaseButton(g_ctx.virtualDevice, "virt_honk");
    }
    ui->Separator();

    // Example of a virtual axis.
    static float throttle_value = 0.0f;
    ui->Text("Virtual Throttle Axis:");
    if (ui->SliderFloat("Throttle", &throttle_value, 0.0f, 1.0f, "%.2f")) {
        // When the slider value changes, update the virtual axis value.
        g_ctx.coreAPI->input->SetAxisValue(g_ctx.virtualDevice, "virt_throttle", throttle_value);
    }
}

// =================================================================================================
// 6. Helper Functions
// =================================================================================================
// This section contains internal helper functions called from the main lifecycle events or callbacks.

/**
 * @brief Creates and registers a virtual input device.
 * @details This function demonstrates how to use the Input API to create a virtual controller.
 *          Once registered, the buttons and axes added here ("virt_honk", "virt_throttle")
 *          will appear in the game's keybinding menu, allowing the user to assign them to
 *          physical hardware. The plugin can then programmatically control them.
 */
void InitializeVirtualDevice() {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->input) return;

    auto logger = g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME);
    g_ctx.virtualDevice = g_ctx.coreAPI->input->CreateDevice(
        PLUGIN_NAME,
        "Example_virtual_device",
        "ExamplePlugin Virtual Controller",
        SPF_INPUT_DEVICE_TYPE_GENERIC
    );

    if (!g_ctx.virtualDevice) {
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_ERROR, "Failed to create virtual device.");
        return;
    }

    // Add a button and an axis to the virtual device.
    g_ctx.coreAPI->input->AddButton(g_ctx.virtualDevice, "virt_honk", "Virtual Honk");
    g_ctx.coreAPI->input->AddAxis(g_ctx.virtualDevice, "virt_throttle", "Virtual Throttle");

    // Register the device with the framework to make it active.
    if (g_ctx.coreAPI->input->Register(g_ctx.virtualDevice)) {
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Successfully registered virtual device.");
    } else {
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_ERROR, "Failed to register virtual device.");
    }
}

/**
 * @brief Parses the `a_complex_object` setting to demonstrate `GetJsonValueHandle` and `JsonReaderApi`.
 * @details This function retrieves a complex JSON object from the config using `ConfigApi::GetJsonValueHandle`
 *          and then uses the `JsonReaderApi` to extract nested values. It showcases how to handle
 *          advanced configuration structures within a plugin.
 */
void ParseComplexObject() {
    // Ensure all required APIs are available. The JsonReader is retrieved from the Core API.
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->config || !g_ctx.coreAPI->json_reader) {
        return;
    }

    auto logger = g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME);
    auto config = g_ctx.coreAPI->config;
    auto config_handle = config->GetContext(PLUGIN_NAME);
    const auto* json_reader = g_ctx.coreAPI->json_reader;

    char log_buffer[512]; // Increased buffer size for potentially long strings

    // 1. Get the handle to the complex JSON object from the Config API.
    const SPF_JsonValue_Handle* object_handle = config->GetJsonValueHandle(config_handle, "settings.a_complex_object");

    if (object_handle) {
        g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "Parsing complex object 'settings.a_complex_object':");
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

        // 2. Use the JsonReader API to check for and get the 'mode' member.
        if (json_reader->HasMember(object_handle, "mode")) {
            const SPF_JsonValue_Handle* mode_handle = json_reader->GetMember(object_handle, "mode");
            if (mode_handle && json_reader->GetType(mode_handle) == SPF_JSON_TYPE_STRING) {
                char mode_str[64];
                json_reader->GetString(mode_handle, mode_str, sizeof(mode_str));
                g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "  -> Mode: %s", mode_str);
                g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
            }
        }

        // 3. Get the 'enabled' member.
        const SPF_JsonValue_Handle* enabled_handle = json_reader->GetMember(object_handle, "enabled");
        if (enabled_handle && json_reader->GetType(enabled_handle) == SPF_JSON_TYPE_BOOLEAN) {
            bool enabled_val = json_reader->GetBool(enabled_handle, false);
            g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "  -> Enabled: %s", enabled_val ? "true" : "false");
            g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
        }

        // 4. Get the 'targets' array and iterate through it.
        const SPF_JsonValue_Handle* targets_handle = json_reader->GetMember(object_handle, "targets");
        if (targets_handle && json_reader->GetType(targets_handle) == SPF_JSON_TYPE_ARRAY) {
            int array_size = json_reader->GetArraySize(targets_handle);
            g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "  -> Found 'targets' array with %d elements:", array_size);
            g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

            for (int i = 0; i < array_size; ++i) {
                const SPF_JsonValue_Handle* item_handle = json_reader->GetArrayItem(targets_handle, i);
                if (item_handle && json_reader->GetType(item_handle) == SPF_JSON_TYPE_STRING) {
                    char item_str[64];
                    json_reader->GetString(item_handle, item_str, sizeof(item_str));
                    g_ctx.coreAPI->formatting->Format(log_buffer, sizeof(log_buffer), "    - Target[%d]: %s", i, item_str);
                    g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
                }
            }
        }
    } else {
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_WARN, "Failed to get handle for 'settings.a_complex_object'.");
    }
}


/**
 * @brief Finds a game function via signature scanning and installs the game string formatting hook.
 * @details This demonstrates the Hooks API. It finds a function in the game's code that matches
 *          the provided byte signature and redirects it to our custom `Detour_GameStringFormatting`
 *          function. The original function's address is stored in a "trampoline" pointer
 *          (`g_ctx.o_GameStringFormatting`) so we can call it from our detour.
 */
void InstallGameStringFormattingHook() {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->hooks) return;

    // This is a byte signature of the target function in memory.
    const char* signature = "48 89 5C 24 08 48 89 6C 24 18 48 89 74 24 20 57 41 54 41 55 41 56 41 57 B8 70 88 00 00 ? ? ? ? ? 48 2B E0 48";

    g_ctx.coreAPI->hooks->Register(
        PLUGIN_NAME,
        "GameStringFormattingHook", // Renamed hook ID for consistency
        "Game String Formatting Hook", // Renamed hook description for consistency
        reinterpret_cast<void*>(Detour_GameStringFormatting), // Our detour function
        reinterpret_cast<void**>(&g_ctx.o_GameStringFormatting), // Pointer to store the original
        signature,
        true // Enable the hook immediately
    );
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, "Registered 'GameStringFormatting' hook.");
}

// =================================================================================================
// 7. Hook Implementations
// =================================================================================================
// This section contains the actual implementation of our detour functions.

/**
 * @brief Our detour function that will be called instead of the original game string formatting function.
 * @details This function intercepts the call, checks if our modification is active, potentially
 * modifies the input, and then **must** call the original function via the trampoline.
 * @param pOutput The same output buffer pointer as the original function.
 * @param ppInput The same input string pointer as the original function (e.g., a game localization key).
 * @return The return value from the original function, called via the trampoline.
 */
void* Detour_GameStringFormatting(void* pOutput, const char** ppInput) {
    // Check our global flag to see if the modification should be active.
    if (g_ctx.isModificationActive) {
        const char* inputKey = *ppInput;
        // Check if the game string (localization key) is the one for the quit button.
        if (inputKey && strstr(inputKey, ">@@quit_game@@</font>")) {
            // If it is, we replace the pointer to the input string with our own custom string.
            // This custom string uses the game's UI markup to make the button red.
            static const char* modifiedQuitButton = "<img src=/material/ui/white.mat xscale=stretch yscale=stretch color=@@clr_list_item_bg_s@@><ret><align hstyle=center vstyle=center><font face=/font/normal_bold.font xscale=1.4 yscale=1.4><color value=FF0000FF>@@quit_game@@</font></align>";
            *ppInput = modifiedQuitButton;

            if (g_ctx.loadAPI && g_ctx.loadAPI->logger) {
                g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->GetLogger(PLUGIN_NAME), SPF_LOG_INFO, "Overriding 'quit_game' button color.");
            }
        }
    }

    // CRITICAL: Always call the original function via the trampoline pointer.
    // Failure to do so will break the game's functionality and likely cause a crash.
    return g_ctx.o_GameStringFormatting(pOutput, ppInput);
}

// =================================================================================================
// 8. Plugin Exports
// =================================================================================================
// These are the two mandatory, C-style functions that the plugin DLL must export. The framework
// uses them as the entry points to load and interact with the plugin. The `extern "C"` block
// is essential to prevent C++ name mangling, ensuring the framework can find them by name.

extern "C" {

/**
 * @brief Exports the manifest API to the framework.
 * @param[out] out_api A pointer to a structure that this function must fill with a pointer
 *                     to the plugin's `GetManifestData` function.
 * @return `true` on success, `false` on failure.
 * @details This is the very first function the framework calls. It allows the framework to get
 *          the plugin's manifest *before* the plugin is fully loaded.
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
 * @param[out] exports A pointer to a structure that this function must fill with
 *                     pointers to the plugin's `OnLoad`, `OnUpdate`, etc., functions.
 * @return `true` on success, `false` on failure.
 * @details After reading the manifest, the framework calls this function to get the pointers
 *          to the actual implementation of the plugin.
 */
SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports) {
    if (exports) {
        // Connect the internal C++ functions to the C-style export struct.
        exports->OnLoad = OnLoad;
        exports->OnActivated = OnActivated;
        exports->OnUnload = OnUnload;
        exports->OnUpdate = OnUpdate;

        // --- Optional, Game World Dependent Initialization ---
        // If your plugin needs to interact with game world objects (e.g., cameras, vehicle data)
        // or install hooks that depend on the game being fully loaded, this is the function to use.
        // It's called only once per session when the player loads into the game world.
        exports->OnGameWorldReady = OnGameWorldReady;

        exports->OnRegisterUI = OnRegisterUI;
        exports->OnSettingChanged = OnSettingChanged;
        return true;
    }
    return false;
}

} // extern "C"

} // namespace ExamplePlugin