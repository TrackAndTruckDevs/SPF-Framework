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

void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // This function is where you define all the metadata for your plugin. The framework calls this
    // function *before* loading your plugin DLL to understand what it is, what it needs, and how it
    // can be configured. This uses a Helper/Builder API for maximum ABI (Application Binary Interface)
    // stability, ensuring compatibility even if the plugin and framework are built with different
    // compilers or settings.

    // --- 2.1. Plugin Information ---
    // This section provides the basic identity of your plugin.
    {
        // `name`: The unique programmatic name of your plugin. No spaces or special characters.
        // This is used for internal identification, folder names, and config files.
        // CRITICAL: This MUST match the name used in `Cfg_GetContext` calls for various APIs.
        api->Info_SetName(h, PLUGIN_NAME);

        // `version`: The version of your plugin. It's a best practice to follow Semantic Versioning (semver.org).
        // Example: "1.0.0", "2.1.0-beta", etc.
        api->Info_SetVersion(h, "0.1.0-alpha");

        // Recommended to fill in
        // The minimum SPF Framework version required for this plugin to work correctly (e.g. "1.0.6").
        // If the user's framework version is lower than this, the plugin will be disabled. And a warning will be shown
        // This prevents crashes due to API changes.
        api->Info_SetMinFrameworkVersion(h, "1.0.6");

        // `author`: (Optional) Your name or your organization's name.
        api->Info_SetAuthor(h, "Your Name");

        //---Optional Social and Project Links ---
        api->Info_SetEmail(h, "mailto:your.email@example.com");
        api->Info_SetDiscordUrl(h, "discordUrl");
        api->Info_SetSteamProfileUrl(h, "steamProfileUrl");
        api->Info_SetGithubUrl(h, "githubUrl");
        api->Info_SetYoutubeUrl(h, "youtubeUrl");
        api->Info_SetScsForumUrl(h, "scsForumUrl");
        api->Info_SetPatreonUrl(h, "patreonUrl");
        api->Info_SetWebsiteUrl(h, "websiteUrl");

        // `descriptionKey`: (Optional) A key for a localized description string. If you provide a key
        // (e.g., "plugin.description"), the framework will look it up in your translation files.
        // If you leave it empty, it will use `descriptionLiteral` instead.
        api->Info_SetDescriptionKey(h, "");

        // `descriptionLiteral`: A fallback description used if `descriptionKey` is empty or not found.
        api->Info_SetDescriptionLiteral(h, "A template plugin to demonstrate the SPF API.");
    }

    // --- 2.2. Configuration Policy ---
    // This section defines how your plugin interacts with the framework's configuration system.
    {
        // `allowUserConfig`: If `true`, the framework will generate a `settings.json` file
        // inside the plugin's config folder (e.g., `/plugins/ExamplePlugin/config/settings.json`),
        // allowing users to override default settings.
        api->Policy_SetAllowUserConfig(h, true);

        // `userConfigurableSystems`: A list of framework systems that the user can configure for this
        // plugin via the main Settings UI. Common values are "settings", "logging", "localization", "ui".
        // Note: The "keybinds" system is always user-configurable and does not need to be listed here.
        api->Policy_AddConfigurableSystem(h, "settings");
        api->Policy_AddConfigurableSystem(h, "logging");
        api->Policy_AddConfigurableSystem(h, "localization");
        api->Policy_AddConfigurableSystem(h, "ui");

        // `requiredHooks`: (Optional) A list of function hooks required for the plugin to work.
        // If a hook is listed here, the framework will ensure it is enabled whenever this plugin is
        // active, and the user will not be able to disable it from the UI.
        api->Policy_AddRequiredHook(h, "GameConsole"); // We need this for the console command example.
        api->Policy_AddRequiredHook(h, "GameLogHook");   // We need this for the game log example.
    }

    // --- 2.3. Custom Settings (settingsJson) ---
    // This is a JSON string literal that defines the default values for your plugin's custom settings.
    // If `allowUserConfig` is true, the framework will create a `settings.json` file for the plugin,
    // and the JSON object you provide here will be inserted under a top-level key named "settings".
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

    // --- 2.4. Default Settings for Framework Systems ---
    // Here you can provide default configurations for various framework systems for your plugin.

    // --- Logging ---
    // `level`: The default minimum log level. Can be "trace", "debug", "info", "warn", "error", "critical".
    // `sinks.file`: If `true`, a dedicated log file for this plugin will be created (e.g., `ExamplePlugin/logs/ExamplePlugin.log`).
    api->Defaults_SetLogging(h, "info", true);

    // --- Localization ---
    // `language`: The default language code (e.g., "en", "de", "uk"). This should match the name
    // of your translation file (e.g., `en.json`).
    api->Defaults_SetLocalization(h, "en");

    // --- Keybinds ---
    {
        // First action: Toggle the main window.
        // `groupName`: A category for the action. Best practice is "{PluginName}.{Feature}".
        // `actionName`: The specific action, usually a verb.
        // The full action name becomes "ExamplePlugin.MainWindow.toggle".
        // `type`: "keyboard", "gamepad", "gamepad_axis", etc. 
        // `key`: "KEY_F5", "BTN_A", "LEFT_STICK_X".
        // `consume`: "never", "on_ui_focus", "always", "manual".
        // Note: press_type, behavior, and axis settings are defaulted automatically.
        api->Defaults_AddKeybind(h, "ExamplePlugin.MainWindow", "toggle", "keyboard", "KEY_F5", "always");
        api->Defaults_AddKeybind(h, "ExamplePlugin.MainWindow", "toggle", "chord", "keyboard:KEY_LCONTROL+keyboard:KEY_F5", "always");

        // Second action: Cycle through camera views.
        api->Defaults_AddKeybind(h, "ExamplePlugin.Camera", "cycle", "keyboard", "KEY_F6", "always");

        // Third action: Demonstrate programmatic blocking (using 'manual' consume policy).
        api->Defaults_AddKeybind(h, "ExamplePlugin.Demo", "honk", "keyboard", "KEY_H", "manual");

        // --- NEW: Analog Test Action ---
        // This action can be triggered by either Space key or Right Trigger.
        api->Defaults_AddKeybind(h, "ExamplePlugin.Test", "Axis", "keyboard", "KEY_SPACE", "never");
        api->Defaults_AddKeybind(h, "ExamplePlugin.Test", "Axis", "gamepad_axis", "RIGHT_TRIGGER_AXIS", "never");
    }

    // --- UI ---
    {
        // `name`: The unique ID for this window within the plugin.
        // `isVisible`: Default visibility. `isInteractive`: transparent to clicks if false.
        // `posX/Y`: Position. `sizeW/H`: Dimensions. `isCollapsed`: state. `autoScroll`: scroll to bottom.
        api->Defaults_AddWindow(h, "MainWindow", true, true, 100, 100, 400, 300, false, false);
    }

    // --- 2.5. Metadata for Localization and UI Hints ---
    // This section is optional. It allows you to provide translatable names and descriptions
    // for your settings, keybinds, and UI elements. You can also specify custom UI widgets.

    // Example 1: A simple integer input (default behavior).
    // This setting uses the default ImGui::InputInt widget because no specific 'widget' type is provided.
    api->Meta_AddCustomSetting(h, "a_simple_number", "setting.simple_number.title", "setting.simple_number.description", nullptr, nullptr, false);

    // Example 2: A float slider with custom range and format.
    // Specify the widget type to be a "slider".
    api->Meta_AddCustomSetting(h, "a_slider_number", "setting.slider_number.title", "setting.slider_number.description", "slider", "{ \"min\": 0.0, \"max\": 100.0, \"format\": \"%.1f %%\" }", false);

    // Example 3: An integer with a draggable input (drag widget).
    // Specify the widget type to be a "drag" control.
    api->Meta_AddCustomSetting(h, "a_drag_number", "setting.drag_number.title", "setting.drag_number.description", "drag", "{ \"speed\": 0.5, \"min\": -100.0, \"max\": 100.0, \"format\": \"%d units\" }", false);

    // Example 4: A dropdown (combo box) for selecting a string option.
    // Specify the widget type to be a "combo" box.
    const char* combo_options = R"json({ "options": [
        { "value": "option_a", "labelKey": "options.a.title" },
        { "value": "option_b", "labelKey": "options.b.title" },
        { "value": "option_c", "labelKey": "This is a literal label" }
    ]})json";
    api->Meta_AddCustomSetting(h, "a_dropdown_choice", "setting.dropdown.title", "setting.dropdown.description", "combo", combo_options, false);

    // Example 5: Radio buttons for selecting a numeric option.
    // Specify the widget type to be "radio" buttons.
    const char* radio_options = R"json({ "options": [
        { "value": 1, "labelKey": "options.radio_one" },
        { "value": 2, "labelKey": "options.radio_two" },
        { "value": 3, "labelKey": "options.radio_three" }
    ]})json";
    api->Meta_AddCustomSetting(h, "a_radio_choice", "setting.radio.title", "setting.radio.description", "radio", radio_options, false);

    // Example 6: An RGB Color Picker.
    // Specify the widget type to be a "color3" picker (RGB).
    api->Meta_AddCustomSetting(h, "a_color", "setting.color.title", "setting.color.description", "color3", "{ \"flags\": 0 }", false);

    // Example 7: A multiline text input field.
    // Specify the widget type to be a "multiline" text input.
    api->Meta_AddCustomSetting(h, "a_text_note", "setting.note.title", "setting.note.description", "multiline", "{ \"height_in_lines\": 4 }", false);

    // Example 8: A complex object (no widget, for programmatic access).
    api->Meta_AddCustomSetting(h, "a_complex_object", "setting.complex_object.title", "setting.complex_object.description", nullptr, nullptr, true);

    // Example 9: A simple float input (default behavior, now with +/- buttons).
    api->Meta_AddCustomSetting(h, "a_float_input", "setting.float_input.title", "setting.float_input.description", nullptr, nullptr, false);

    // Example 10: A double input with custom step.
    api->Meta_AddCustomSetting(h, "a_double_input", "setting.double_input.title", "setting.double_input.description", "input_double", "{ \"step\": 0.005, \"format\": \"%.4f\" }", false);

    // Example 11: A vertical float slider.
    api->Meta_AddCustomSetting(h, "a_vslider_float", "setting.vslider_float.title", "setting.vslider_float.description", "vslider", "{ \"min\": -1.0, \"max\": 1.0, \"width\": 30.0, \"height\": 100.0, \"format\": \"%.2f\" }", false);

    // Example 12: An input field with a hint.
    api->Meta_AddCustomSetting(h, "a_hinted_input", "setting.hinted_input.title", "setting.hinted_input.description", "input_with_hint", "{ \"hint\": \"Enter your username\" }", false);

    // Example 13: A logarithmic slider.
    api->Meta_AddCustomSetting(h, "a_log_slider", "setting.log_slider.title", "setting.log_slider.description", "slider", "{ \"min\": 0.1, \"max\": 1000.0, \"is_logarithmic\": true }", false);

    // --- Keybinds Metadata ---
    {
        api->Meta_AddKeybind(h, "ExamplePlugin.MainWindow", "toggle", "keybind.main_window_toggle.title", "keybind.main_window_toggle.description");
        api->Meta_AddKeybind(h, "ExamplePlugin.Camera", "cycle", "keybind.camera_cycle.title", "keybind.camera_cycle.description");
    }

    // --- UI Metadata ---
    {
        api->Meta_AddWindow(h, "MainWindow", "ui.window.main_window.title", "ui.window.main_window.description");
    }

    /*
    // --- TIP: Mass Metadata Registration ---
    // If your plugin has many similar settings (e.g., dozens of sliders for camera positions),
    // do not call api->Meta_AddCustomSetting manually every time. Use helper lambdas to
    // reduce boilerplate and ensure consistent formatting.
    //
    // NOTE: This approach requires '#include <string>' in your file.

    auto AddSliderHelper = [&](const char* key, const char* titleKey, float min, float max, const char* fmt) {
        std::string params = "{ \"min\": " + std::to_string(min) + 
                             ", \"max\": " + std::to_string(max) + 
                             ", \"format\": \"" + fmt + "\" }";
        api->Meta_AddCustomSetting(h, key, titleKey, nullptr, "slider", params.c_str(), false);
    };

    // Now you can register multiple sliders in a single line each:
    AddSliderHelper("rendering.fov", "setting.fov.title", 60.0f, 120.0f, "%.0f deg");
    AddSliderHelper("audio.volume", "setting.volume.title", 0.0f, 1.0f, "%.2f");
    */
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
    if (g_ctx.loadAPI && g_ctx.loadAPI->logger && g_ctx.loadAPI->config && g_ctx.loadAPI->input) {
        // Cache environment API and get context
        g_ctx.environmentAPI = g_ctx.loadAPI->environment;
        if (g_ctx.environmentAPI) {
            g_ctx.environmentHandle = g_ctx.environmentAPI->Env_GetContext(PLUGIN_NAME);
        }

        // Get a handle to our plugin's dedicated logger instance.
        auto logger = g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME);
        g_ctx.loadAPI->logger->Log(logger, SPF_LOG_INFO, "ExamplePlugin has been loaded!");

        // Read initial values from the config file.
        auto config = g_ctx.loadAPI->config->Cfg_GetContext(PLUGIN_NAME);
        g_ctx.someNumber = g_ctx.loadAPI->config->Cfg_GetInt32(config, "settings.a_simple_number", 42);

        // Initialize virtual devices early (in OnLoad) so they can be registered by the game SDK
        // during its own input initialization phase.
        InitializeVirtualDevice(g_ctx.loadAPI->input, g_ctx.loadAPI->logger);

        // Log the initial value. Use a local buffer for safe cross-DLL string formatting.
        char log_buffer[256];
        g_ctx.loadAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Initial value for 'a_simple_number' is %d.", g_ctx.someNumber);
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
    auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);

    // Get pointers to additional framework services.
    if (g_ctx.coreAPI) {
        g_ctx.vehicleAPI = g_ctx.coreAPI->vehicle;
        g_ctx.environmentAPI = g_ctx.coreAPI->environment;
    }

    // Register callbacks for systems that require the core API.
    if (g_ctx.coreAPI && g_ctx.coreAPI->keybinds) {
        g_ctx.keybindsHandle = g_ctx.coreAPI->keybinds->Kbind_GetContext(PLUGIN_NAME);
        g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "ExamplePlugin.MainWindow.toggle", OnToggleMainWindow);
        g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "ExamplePlugin.Camera.cycle", OnCameraKeybind);
        g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "ExamplePlugin.Demo.honk", []() {
            auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);
            g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "BEEP! (Honk action triggered in plugin)");
        });
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Registered keybinds.");
    }

    if (g_ctx.coreAPI && g_ctx.coreAPI->gamelog) {
        SPF_GameLog_Handle* glog_h = g_ctx.coreAPI->gamelog->GLog_GetContext(PLUGIN_NAME);
        g_ctx.gameLogCallbackHandle = g_ctx.coreAPI->gamelog->GLog_RegisterCallback(glog_h, OnGameLogMessage, nullptr);
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "Registered game log callback.");
    }

    // Initialize more complex features that need the core API.
    InstallGameStringFormattingHook();

    // Parse the complex object on activation to demonstrate GetJsonValueHandle and JsonReaderApi.
    ParseComplexObject();

    // --- Telemetry Event Example ---
    // Get a handle for the telemetry API and register our callbacks.
    if (g_ctx.coreAPI && g_ctx.coreAPI->telemetry) {
        g_ctx.telemetryHandle = g_ctx.coreAPI->telemetry->Tel_GetContext(PLUGIN_NAME);
        if (g_ctx.telemetryHandle) {
            auto tel = g_ctx.coreAPI->telemetry;
            g_ctx.gameStateCallback = tel->Tel_RegisterForGameState(g_ctx.telemetryHandle, OnGameStateUpdate, &g_ctx);
            g_ctx.timestampsCallback = tel->Tel_RegisterForTimestamps(g_ctx.telemetryHandle, OnTimestampsUpdate, &g_ctx);
            g_ctx.commonDataCallback = tel->Tel_RegisterForCommonData(g_ctx.telemetryHandle, OnCommonDataUpdate, &g_ctx);
            g_ctx.truckConstantsCallback = tel->Tel_RegisterForTruckConstants(g_ctx.telemetryHandle, OnTruckConstantsUpdate, &g_ctx);
            g_ctx.trailerConstantsCallback = tel->Tel_RegisterForTrailerConstants(g_ctx.telemetryHandle, OnTrailerConstantsUpdate, &g_ctx);
            g_ctx.truckDataCallback = tel->Tel_RegisterForTruckData(g_ctx.telemetryHandle, OnTruckDataUpdate, &g_ctx);
            g_ctx.trailersCallback = tel->Tel_RegisterForTrailers(g_ctx.telemetryHandle, OnTrailersUpdate, &g_ctx);
            g_ctx.jobConstantsCallback = tel->Tel_RegisterForJobConstants(g_ctx.telemetryHandle, OnJobConstantsUpdate, &g_ctx);
            g_ctx.jobDataCallback = tel->Tel_RegisterForJobData(g_ctx.telemetryHandle, OnJobDataUpdate, &g_ctx);
            g_ctx.navigationDataCallback = tel->Tel_RegisterForNavigationData(g_ctx.telemetryHandle, OnNavigationDataUpdate, &g_ctx);
            g_ctx.controlsCallback = tel->Tel_RegisterForControls(g_ctx.telemetryHandle, OnControlsUpdate, &g_ctx);
            g_ctx.specialEventsCallback = tel->Tel_RegisterForSpecialEvents(g_ctx.telemetryHandle, OnSpecialEventsUpdate, &g_ctx);
            g_ctx.gameplayEventsCallback = tel->Tel_RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvent, &g_ctx);
            g_ctx.gearboxConstantsCallback = tel->Tel_RegisterForGearboxConstants(g_ctx.telemetryHandle, OnGearboxConstantsUpdate, &g_ctx);

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
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
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

    auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);
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
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game ID: %s (%s)\n", data.game_id, data.game_name); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game Version: %u.%u\n", data.scs_game_version_major, data.scs_game_version_minor); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Telemetry Version: %u.%u\n", data.telemetry_game_version_major, data.telemetry_game_version_minor); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Paused: %s, Scale: %.2f, MP Time Offset: %d\n", data.paused ? "Yes" : "No", data.scale, data.multiplayer_time_offset); strcat_safe(temp_line_buffer);
    }

    // --- Timestamps ---
    {
        const auto& data = g_ctx.eventDataCache.timestamps;
        strcat_safe("[Timestamps]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Sim: %llu, Render: %llu, Paused Sim: %llu\n", data.simulation, data.render, data.paused_simulation); strcat_safe(temp_line_buffer);
    }

    // --- CommonData ---
    {
        const auto& data = g_ctx.eventDataCache.commonData;
        strcat_safe("[CommonData]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Game Time: %u, Next Rest: %d min\n", data.game_time, data.next_rest_stop); strcat_safe(temp_line_buffer);
    }

    // --- TruckConstants ---
    {
        const auto& data = g_ctx.eventDataCache.truckConstants;
        strcat_safe("[TruckConstants]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Truck: %s %s (%s, %s)\n", data.brand, data.name, data.brand_id, data.id); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  License: %s (%s, %s)\n", data.license_plate, data.license_plate_country, data.license_plate_country_id); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Drivetrain: %u Fwd, %u Rev, RPM Limit: %.0f, Diff Ratio: %.2f\n", data.forward_gear_count, data.reverse_gear_count, data.rpm_limit, data.differential_ratio); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Capacities: Fuel: %.1f L, AdBlue: %.1f L\n", data.fuel_capacity, data.adblue_capacity); strcat_safe(temp_line_buffer);
        for (uint32_t i = 0; i < data.wheel_count; ++i) {
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Wheel %u: Radius=%.3f, Steerable=%d, Powered=%d, Liftable=%d\n", i, data.wheels[i].radius, data.wheels[i].steerable, data.wheels[i].powered, data.wheels[i].liftable); strcat_safe(temp_line_buffer);
        }
    }

    // --- TruckData ---
    {
        const auto& data = g_ctx.eventDataCache.truckData;
        strcat_safe("[TruckData]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  World Pos: (%.2f, %.2f, %.2f)\n", data.world_placement.position.x, data.world_placement.position.y, data.world_placement.position.z); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Speed: %.1f kph, RPM: %.0f\n", data.speed * 3.6f, data.engine_rpm); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Gear: %d (Displayed: %d), Cruise Control: %.1f kph\n", data.gear, data.displayed_gear, data.cruise_control_speed * 3.6f); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Brakes: Parking=%d, Motor=%d, Retarder=%u, Temp: %.1f C\n", data.parking_brake, data.motor_brake, data.retarder_level, data.brake_temperature); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Pressures: Air=%.1f psi, Oil=%.1f psi\n", data.air_pressure, data.oil_pressure); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Temps: Water=%.1f C, Oil=%.1f C\n", data.water_temperature, data.oil_temperature); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Fluids: Fuel=%.1f L, AdBlue=%.1f L\n", data.fuel_amount, data.adblue_amount); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Lights: L=%d R=%d, Park=%d, Low=%d, High=%d, Beacon=%d\n", data.lblinker, data.rblinker, data.light_parking, data.light_low_beam, data.light_high_beam, data.light_beacon); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Wear: Eng=%.3f, Trans=%.3f, Cab=%.3f, Chas=%.3f, Wheels=%.3f\n", data.wear_engine, data.wear_transmission, data.wear_cabin, data.wear_chassis, data.wear_wheels); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Odometer: %.1f km\n", data.odometer); strcat_safe(temp_line_buffer);
    }

    // --- Trailers ---
    {
        const auto& data = g_ctx.eventDataCache.trailers;
        strcat_safe("[Trailers]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Count: %zu\n", data.size()); strcat_safe(temp_line_buffer);
        for (size_t i = 0; i < data.size(); ++i) {
            const auto& trailer = data[i];
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Trailer %zu: %s (%s) Conn: %d\n", i, trailer.constants.name, trailer.constants.id, trailer.data.connected); strcat_safe(temp_line_buffer);
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "      Damage: Cargo=%.3f, Chassis=%.3f, Wheels=%.3f\n", trailer.data.cargo_damage, trailer.data.wear_chassis, trailer.data.wear_wheels); strcat_safe(temp_line_buffer);
            for (uint32_t j = 0; j < trailer.constants.wheel_count; ++j) {
                const auto& wheel_data = trailer.data.wheels[j];
                const auto& wheel_const = trailer.constants.wheels[j];
                format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "      Trailer Wheel %u: R=%.3f, Defl=%.3f, Ground=%d, Vel=%.2f\n", j, wheel_const.radius, wheel_data.suspension_deflection, wheel_data.on_ground, wheel_data.angular_velocity); strcat_safe(temp_line_buffer);
            }
        }
    }

    // --- Job ---
    {
        const auto& job_const = g_ctx.eventDataCache.jobConstants;
        const auto& job_data = g_ctx.eventDataCache.jobData;
        strcat_safe("[Job]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  On Job: %s\n", job_data.on_job ? "Yes" : "No"); strcat_safe(temp_line_buffer);
        if (job_data.on_job) {
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Cargo: %s (%s), Mass: %.0f kg\n", job_const.cargo_name, job_const.cargo_id, job_const.cargo_mass); strcat_safe(temp_line_buffer);
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Route: %s -> %s\n", job_const.source_city, job_const.destination_city); strcat_safe(temp_line_buffer);
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Company: %s -> %s\n", job_const.source_company, job_const.destination_company); strcat_safe(temp_line_buffer);
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Income: %llu, Market: %s\n", job_const.income, job_const.job_market); strcat_safe(temp_line_buffer);
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    Time Left: %u min, Cargo Dmg: %.3f\n", job_data.remaining_delivery_minutes, job_data.cargo_damage); strcat_safe(temp_line_buffer);
        }
    }

    // --- Navigation ---
    {
        const auto& data = g_ctx.eventDataCache.navigationData;
        strcat_safe("[Navigation]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Distance: %.0f m, Time: %.0f s (%.1f real s), Speed Limit: %.0f kph\n", data.navigation_distance, data.navigation_time, data.navigation_time_real_seconds, data.navigation_speed_limit * 3.6f); strcat_safe(temp_line_buffer);
    }

    // --- Controls ---
    {
        const auto& data = g_ctx.eventDataCache.controls;
        strcat_safe("[Controls]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  User: Thr=%.2f, Brk=%.2f, Steer=%.2f, Clutch=%.2f\n", data.userInput.throttle, data.userInput.brake, data.userInput.steering, data.userInput.clutch); strcat_safe(temp_line_buffer);
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Effective: Thr=%.2f, Brk=%.2f, Steer=%.2f, Clutch=%.2f\n", data.effectiveInput.throttle, data.effectiveInput.brake, data.effectiveInput.steering, data.effectiveInput.clutch); strcat_safe(temp_line_buffer);
    }

    // --- GameplayEvents ---
    {
        const auto& event_id = g_ctx.eventDataCache.lastGameplayEventId;
        const auto& data = g_ctx.eventDataCache.gameplayEvents;
        strcat_safe("[GameplayEvents]\n");
        format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "  Last Event ID: %s\n", event_id); strcat_safe(temp_line_buffer);
        if (strcmp(event_id, "player.fined") == 0) {
            format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Fine Details: Amount=%lld, Offence=%s\n", data.player_fined.fine_amount, data.player_fined.fine_offence); strcat_safe(temp_line_buffer);
        } else if (strcmp(event_id, "job.delivered") == 0) {
             format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Job Delivered: Revenue=%lld, XP=%d, Dist=%.1f km\n", data.job_delivered.revenue, data.job_delivered.earned_xp, data.job_delivered.distance_km); strcat_safe(temp_line_buffer);
        } else if (strcmp(event_id, "job.cancelled") == 0) {
             format->Fmt_Format(temp_line_buffer, sizeof(temp_line_buffer), "    -> Job Cancelled: Penalty=%lld\n", data.job_cancelled.penalty); strcat_safe(temp_line_buffer);
        }
    }

    strcat_safe("--- END EXHAUSTIVE EVENT CACHE LOG ---\n");


    // g_ctx.coreAPI->logger->LogThrottled(
    //     logger,
    //     SPF_LOG_INFO,
    //     "ExamplePlugin.full_event_cache.log",
    //     3000,
    //     full_log_buffer
    // );
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
        g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "ExamplePlugin is being unloaded.");
    }

    // It's good practice to null out all cached pointers on unload. This helps prevent
    // accidental use-after-free if another part of the code attempts to access them
    // after the plugin has been told to shut down.

    // 1. Unregister callbacks & Subscriptions (Handles managed by framework will auto-cleanup)
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
    g_ctx.gameLogCallbackHandle = nullptr;

    // 2. Clear API Context Handles
    g_ctx.telemetryHandle = nullptr;
    g_ctx.keybindsHandle = nullptr;
    g_ctx.mainWindowHandle = nullptr;
    g_ctx.virtualDevice = nullptr;

    // 3. Clear Internal Plugin State
    g_ctx.selectedVehicle = nullptr;
    g_ctx.vehicleHandles.clear();
    g_ctx.o_GameStringFormatting = nullptr;

    // 4. Null out core API pointers
    g_ctx.uiAPI = nullptr;
    g_ctx.vehicleAPI = nullptr;
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
        g_ctx.someNumber = g_ctx.loadAPI->config->Cfg_GetInt32(config_handle, "settings.a_simple_number", 42);

        // Log the change for debugging purposes.
        char log_buffer[256];
        g_ctx.loadAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "'a_simple_number' was changed externally. New value: %d", g_ctx.someNumber);
        g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    } else if (strcmp(keyPath, "settings.a_complex_object") == 0) {
        // The complex object setting has changed. Re-parse it.
        // This demonstrates the use of GetJsonValueHandle and JsonReaderApi.
        ParseComplexObject();
    }
}

/**
 * @brief Called by the framework when the global interface language is changed.
 * @param langCode The new language code (e.g., "en", "uk").
 * @details This allows the plugin to automatically synchronize its language with the framework.
 */
void OnLanguageChanged(const char* langCode) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->localization || !langCode) return;

    SPF_Localization_Handle* h = g_ctx.coreAPI->localization->Loc_GetContext(PLUGIN_NAME);
    
    // Check if the plugin actually has a translation for the new language.
    // If it doesn't, we do NOTHING (stay on the current language).
    if (g_ctx.coreAPI->localization->Loc_HasLanguage(h, langCode)) {
        if (g_ctx.coreAPI->localization->Loc_SetLanguage(h, langCode)) {
            char log_buffer[256];
            g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Plugin language synchronized to: %s", langCode);
            g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
        }
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
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Game Log contains 'Loaded': %s", log_line);
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, buffer);
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
        const bool isCurrentlyVisible = g_ctx.uiAPI->UI_IsVisible(g_ctx.mainWindowHandle);
        // Instruct the UI API to apply the inverse of the current state.
        g_ctx.uiAPI->UI_SetVisibility(g_ctx.mainWindowHandle, !isCurrentlyVisible);

        char log_buffer[256];
        g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Main window visibility toggled to: %s", !isCurrentlyVisible ? "visible" : "hidden");
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
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
    if (g_ctx.coreAPI->camera->Cam_GetCurrentCamera(&current_camera_type)) {
        // Determine the next camera in the cycle.
        SPF_CameraType next_camera_type = (current_camera_type == SPF_CAMERA_INTERIOR) ? SPF_CAMERA_BEHIND :
                                          (current_camera_type == SPF_CAMERA_BEHIND) ? SPF_CAMERA_DEVELOPER_FREE :
                                          SPF_CAMERA_INTERIOR;
        // Switch to the next camera.
        g_ctx.coreAPI->camera->Cam_SwitchTo(next_camera_type);

        char log_buffer[256];
        g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Switched camera from %d to %d via keybind.", current_camera_type, next_camera_type);
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
    } else {
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Could not get current camera type to cycle.");
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
        ui_api->UI_RegisterDrawCallback(PLUGIN_NAME, "MainWindow", RenderMainWindow, nullptr);

        // Get and cache the handle to our window for efficient access later.
        g_ctx.mainWindowHandle = g_ctx.uiAPI->UI_GetWindowHandle(PLUGIN_NAME, "MainWindow");
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
    if (ui->UI_BeginTabBar("##MainWindowTabs")) {
        if (ui->UI_BeginTabItem("General")) {
            ui->UI_Text("Hello from the ExamplePlugin window!");

            // Example of getting and displaying a translated string.
            char welcome_msg[256];
            g_ctx.loadAPI->localization->Loc_GetString(g_ctx.loadAPI->localization->Loc_GetContext(PLUGIN_NAME), "messages.welcome", welcome_msg, sizeof(welcome_msg));
            ui->UI_Text(welcome_msg);
            ui->UI_Separator();

            // --- Config UI Example ---
            ui->UI_Text("This slider modifies a value in settings.json.");
            if (ui->UI_SliderInt("Some Number", &g_ctx.someNumber, 0, 100, "%d")) {
                // If the slider is moved, update the configuration file.
                g_ctx.loadAPI->config->Cfg_SetInt32(g_ctx.loadAPI->config->Cfg_GetContext(PLUGIN_NAME), "settings.a_simple_number", g_ctx.someNumber);
                g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "User changed 'a_simple_number' via UI.");
            }
            ui->UI_Separator();

            // --- Game Console Example ---
            ui->UI_Text("Enter a command to execute in the in-game console:");
            ui->UI_InputText("##ConsoleCommand", g_ctx.consoleCommand, sizeof(g_ctx.consoleCommand));
            ui->UI_SameLine(0, 0);
            if (ui->UI_Button("Execute", 0, 0)) {
                if (g_ctx.coreAPI && g_ctx.coreAPI->console && g_ctx.consoleCommand[0] != '\0') {
                    g_ctx.coreAPI->console->GCon_ExecuteCommand(g_ctx.consoleCommand);
                    char log_buffer[512];
                    g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Executed console command: '%s'", g_ctx.consoleCommand);
                    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
                }
            }
            ui->UI_Separator();

            // --- Dynamic Blocking Example ---
            ui->UI_Text("Dynamic Input Blocking (requires 'Plugin Managed' in settings):");
            if (ui->UI_Checkbox("Block Game Horn (H key)", &g_ctx.isHonkIntercepted)) {
                if (g_ctx.coreAPI && g_ctx.coreAPI->keybinds) {
                    auto h = g_ctx.coreAPI->keybinds->Kbind_GetContext(PLUGIN_NAME);
                    g_ctx.coreAPI->keybinds->Kbind_SetBlockState(h, "ExamplePlugin.Demo.honk", g_ctx.isHonkIntercepted);
                }
            }

            static bool blockMouse = false;
            if (ui->UI_Checkbox("Block Game Mouse Look", &blockMouse)) {
                ui->UI_SetMouseBlockState(blockMouse, false, false);
            }
            ui->UI_Separator();

            // --- Hook Example ---
            ui->UI_Text("This checkbox controls a function hook:");
            ui->UI_Checkbox("Make 'Quit' button red", &g_ctx.isModificationActive);
            ui->UI_EndTabItem();
        }
        // Render the content of other tabs by calling their respective functions.
        if (ui->UI_BeginTabItem("Traffic Inspector")) { RenderVehicleTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Camera")) { RenderCameraTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Telemetry")) { RenderTelemetryTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Events")) { RenderEventsTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Virtual Input")) { RenderVirtInputTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Styling API")) { RenderStylingTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Environment")) { RenderEnvironmentTab(ui, user_data); ui->UI_EndTabItem(); }
        if (ui->UI_BeginTabItem("Input Test")) { RenderInputTestTab(ui, user_data); ui->UI_EndTabItem(); }
        ui->UI_EndTabBar();
    }
}

void RenderEnvironmentTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.environmentAPI || !g_ctx.environmentHandle) {
        ui->UI_Text("Environment API is not available.");
        return;
    }

    auto env = g_ctx.environmentAPI;
    auto h = g_ctx.environmentHandle;
    char buffer[512];

    ui->UI_TextWrapped("This tab demonstrates the Environment API, which provides details about the game, framework, and system.");
    ui->UI_Separator();

    // --- Section 1: Game & Profile ---
    if (ui->UI_TreeNode(ICON_FA_TRUCK " Game & Profile")) {
        env->Env_GetGameName(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Game Name", buffer);

        env->Env_GetGameVersion(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Game Version", buffer);

        env->Env_GetActiveProfileName(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Active Profile", buffer);

        ui->UI_TreePop();
    }

    // --- Section 2: Filesystem Paths ---
    if (ui->UI_TreeNode(ICON_FA_FOLDER_OPEN " Resolved Paths")) {
        env->Env_GetSCSUserDir(h, buffer, sizeof(buffer));
        ui->UI_LabelText("User Dir (/home)", buffer);

        env->Env_GetCurrentProfilePath(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Profile Path", buffer);

        env->Env_GetSCSMusicDir(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Music Dir", buffer);

        ui->UI_TreePop();
    }

    // --- Section 3: Runtime Status ---
    if (ui->UI_TreeNode(ICON_FA_CHART_LINE " Runtime Status")) {
        ui->UI_LabelText("VR Active", env->Env_IsVRActive(h) ? "Yes" : "No");
        ui->UI_LabelText("Tobii DLL", env->Env_IsTobiiDllLoaded(h) ? "Loaded" : "Not Loaded");
        ui->UI_LabelText("Steam Overlay", env->Env_IsSteamOverlayDllLoaded(h) ? "Loaded" : "Not Loaded");

        env->Env_GetMultiplayerStatus(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Multiplayer", buffer);

        env->Env_GetRendererName(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Renderer", buffer);

        ui->UI_TreePop();
    }

    // --- Section 4: System Info ---
    if (ui->UI_TreeNode(ICON_FA_GEAR " System Info")) {
        env->Env_GetOSName(h, buffer, sizeof(buffer));
        ui->UI_LabelText("OS Version", buffer);

        env->Env_GetSystemLocale(h, buffer, sizeof(buffer));
        ui->UI_LabelText("Locale", buffer);

        ui->UI_TreePop();
    }
}

/**
 * @brief Renders the content for the "Camera" tab in the main window.
 */
void RenderCameraTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->camera || !ui) {
        ui->UI_Text("Camera API is not available.");
        return;
    }
    ui->UI_Text("Use this tab to interact with the game's camera system.");
    ui->UI_Text("You can also press F6 to cycle through the cameras.");
    ui->UI_Separator();

    // Get and display the current camera type.
    SPF_CameraType current_camera;
    if (g_ctx.coreAPI->camera->Cam_GetCurrentCamera(&current_camera)) {
        char buffer[256];
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Current Camera Type: %d", current_camera);
        ui->UI_Text(buffer);
    } else {
        ui->UI_Text("Could not retrieve current camera type.");
    }
    ui->UI_Separator();

    // Add buttons to switch to a specific camera.
    ui->UI_Text("Switch to a specific camera:");
    if (ui->UI_Button("Interior", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_INTERIOR);
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Behind", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_BEHIND);
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Developer Free", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_DEVELOPER_FREE);
    ui->UI_Separator();

    // Get and display the camera's world coordinates.
    float x, y, z;
    if (g_ctx.coreAPI->camera->Cam_GetCameraWorldCoordinates(&x, &y, &z)) {
        char buffer[256];
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "X: %.2f, Y: %.2f, Z: %.2f", x, y, z);
        ui->UI_Text("Current Camera Position:");
        ui->UI_Text(buffer);
    } else {
        ui->UI_Text("Could not get camera world coordinates.");
    }
}

/**
 * @brief Renders the content for the "Vehicle" tab in the main window.
 */
void RenderVehicleTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.vehicleAPI) {
        ui->UI_Text("Vehicle API is not available.");
        return;
    }

    // 1. Update vehicle list
    uint32_t count = g_ctx.vehicleAPI->Veh_GetCount();
    
    // Resize vector if needed (with buffer)
    if (g_ctx.vehicleHandles.size() < count + 10) {
        g_ctx.vehicleHandles.resize(count + 50);
    }

    // Get handles
    uint32_t actualCount = g_ctx.vehicleAPI->Veh_GetAllHandles(g_ctx.vehicleHandles.data(), (uint32_t)g_ctx.vehicleHandles.size());

    // 2. Prepare preview for combo box
    char previewText[64] = "Select Vehicle...";
    if (g_ctx.selectedVehicle) {
        // Check if selected vehicle still exists
        bool stillExists = false;
        for (uint32_t i = 0; i < actualCount; ++i) {
            if (g_ctx.vehicleHandles[i] == g_ctx.selectedVehicle) {
                stillExists = true;
                break;
            }
        }

        if (stillExists) {
            int32_t id = g_ctx.vehicleAPI->Veh_GetId(g_ctx.selectedVehicle);
            g_ctx.coreAPI->formatting->Fmt_Format(previewText, sizeof(previewText), "Vehicle ID: %d", id);
        } else {
            g_ctx.selectedVehicle = nullptr; // Vehicle disappeared
            g_ctx.coreAPI->formatting->Fmt_Format(previewText, sizeof(previewText), "Select Vehicle...");
        }
    }

    // 3. Draw Combo Box
    if (ui->UI_BeginCombo("Target Vehicle", previewText)) {
        for (uint32_t i = 0; i < actualCount; ++i) {
            SPF_VehicleHandle h = g_ctx.vehicleHandles[i];
            int32_t id = g_ctx.vehicleAPI->Veh_GetId(h);
            
            char itemLabel[64];
            g_ctx.coreAPI->formatting->Fmt_Format(itemLabel, sizeof(itemLabel), "Vehicle #%d", id);

            bool isSelected = (g_ctx.selectedVehicle == h);
            if (ui->UI_Selectable(itemLabel, isSelected)) {
                g_ctx.selectedVehicle = h;
            }
        }
        ui->UI_EndCombo();
    }

    ui->UI_Separator();

    // 4. Display info for selected vehicle
    if (g_ctx.selectedVehicle) {
        SPF_VehicleHandle h = g_ctx.selectedVehicle;
        
        // Retrieve data
        float speed = g_ctx.vehicleAPI->Veh_GetCurrentSpeed(h);
        float accel = g_ctx.vehicleAPI->Veh_GetAcceleration(h);
        float targetSpeed = g_ctx.vehicleAPI->Veh_GetTargetSpeed(h);
        float speedLimit = g_ctx.vehicleAPI->Veh_GetSpeedLimit(h);
        float patience = g_ctx.vehicleAPI->Veh_GetPatience(h);
        float safety = g_ctx.vehicleAPI->Veh_GetSafety(h);
        
        char buffer[64];

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s (%.0f km/h)", speed, speed * 3.6f);
        ui->UI_LabelText("Speed", buffer);

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s^2", accel);
        ui->UI_LabelText("Acceleration", buffer);

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s", targetSpeed);
        ui->UI_LabelText("Target Speed", buffer);

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s", speedLimit);
        ui->UI_LabelText("Speed Limit", buffer);

        ui->UI_Separator();

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f", patience);
        ui->UI_LabelText("AI Patience", buffer);

        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f", safety);
        ui->UI_LabelText("AI Safety", buffer);
        
        uintptr_t addr = g_ctx.vehicleAPI->Veh_GetRawAddress(h);
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "0x%llX", (unsigned long long)addr);
        ui->UI_LabelText("Address", buffer);
    } else {
        ui->UI_Text("Please select a vehicle to inspect.");
        ui->UI_Text("Note: Use the 'Traffic' debug camera mode to see vehicle IDs.");
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
        ui->UI_Text("Telemetry API is not available.");
        return;
    }
    ui->UI_Text("This tab displays live data from the Telemetry API.");
    ui->UI_Separator();

    char buffer[256];
    auto telemetry = g_ctx.coreAPI->telemetry->Tel_GetContext(PLUGIN_NAME);

    // Display truck data.
    SPF_TruckData truck_data;
    g_ctx.coreAPI->telemetry->Tel_GetTruckData(telemetry, &truck_data, sizeof(SPF_TruckData));
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Speed: %.0f kph", truck_data.speed * 3.6f);
    ui->UI_Text(buffer);
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Engine RPM: %.0f", truck_data.engine_rpm);
    ui->UI_Text(buffer);
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Gear: %d", truck_data.displayed_gear);
    ui->UI_Text(buffer);
    ui->UI_Separator();

    // Display job data.
    SPF_JobConstants job_constants;
    g_ctx.coreAPI->telemetry->Tel_GetJobConstants(telemetry, &job_constants, sizeof(SPF_JobConstants));
    SPF_JobData job_data;
    g_ctx.coreAPI->telemetry->Tel_GetJobData(telemetry, &job_data, sizeof(SPF_JobData));
    if (job_data.on_job) {
        ui->UI_Text("Currently on a job!");
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Cargo: %s", job_constants.cargo_name);
        ui->UI_Text(buffer);
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Destination: %s, %s", job_constants.destination_company, job_constants.destination_city);
        ui->UI_Text(buffer);
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Cargo Damage: %.1f%%", job_data.cargo_damage * 100.0f);
        ui->UI_Text(buffer);
    } else {
        ui->UI_Text("Not currently on a job.");
    }
}

void RenderEventsTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !ui) {
        ui->UI_Text("Core API not available.");
        return;
    }
    ui->UI_Text("This tab displays the last data received from event callbacks.");
    ui->UI_Separator();

    char buffer[512];

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Last Gameplay Event: %s", g_ctx.eventDataCache.lastGameplayEventId);
    ui->UI_Text(buffer);
    ui->UI_Separator();

    ui->UI_Text("Game State:");
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "  Paused: %s", g_ctx.eventDataCache.gameState.paused ? "Yes" : "No");
    ui->UI_Text(buffer);
    ui->UI_Separator();

    ui->UI_Text("Truck Data:");
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "  Speed: %.0f kph", g_ctx.eventDataCache.truckData.speed * 3.6f);
    ui->UI_Text(buffer);
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "  Engine RPM: %.0f", g_ctx.eventDataCache.truckData.engine_rpm);
    ui->UI_Text(buffer);
    ui->UI_Separator();

    ui->UI_Text("Trailer Info:");
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "  Attached Trailers: %zu", g_ctx.eventDataCache.trailers.size());
    ui->UI_Text(buffer);
    if (!g_ctx.eventDataCache.trailers.empty()) {
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "  Trailer 1 Brand: %s", g_ctx.eventDataCache.trailers[0].constants.brand);
        ui->UI_Text(buffer);
    }
}

/**
 * @brief Renders the content for the "Virtual Input" tab in the main window.
 */
void RenderVirtInputTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->input || !g_ctx.virtualDevice || !ui) {
        ui->UI_Text("Virtual Input API not available or device not initialized.");
        return;
    }
    ui->UI_Text("Use the controls below to simulate input.");
    ui->UI_Text("You must bind 'Virtual Honk' and 'Virtual Throttle' in the game's keybinding menu.");
    ui->UI_Separator();

    // Example of a virtual button.
    ui->UI_Text("Virtual Honk Button:");
    ui->UI_Button("Hold to Honk", 0, 0); // The button itself is just for show.
    if (ui->UI_IsItemActive()) {
        // While the ImGui button is held down, press the virtual button.
        g_ctx.coreAPI->input->Virt_PressButton(g_ctx.virtualDevice, "virt_honk");
    } else {
        // When the ImGui button is released, release the virtual button.
        g_ctx.coreAPI->input->Virt_ReleaseButton(g_ctx.virtualDevice, "virt_honk");
    }
    ui->UI_Separator();

    // Example of a virtual axis.
    static float throttle_value = 0.0f;
    ui->UI_Text("Virtual Throttle Axis:");
    if (ui->UI_SliderFloat("Throttle", &throttle_value, 0.0f, 1.0f, "%.2f")) {
        // When the slider value changes, update the virtual axis value.
        g_ctx.coreAPI->input->Virt_SetAxisValue(g_ctx.virtualDevice, "virt_throttle", throttle_value);
    }
}

void RenderInputTestTab(SPF_UI_API* ui, void* user_data) {
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds || !g_ctx.keybindsHandle || !ui) {
        ui->UI_Text("Keybinds API not available.");
        return;
    }

    ui->UI_Text("Use this tab to test analog axis bindings and view detailed binding info.");
    ui->UI_Text("Assign any axis to 'ExamplePlugin.Test.Axis' in settings.");
    ui->UI_Separator();

    const char* actionName = "ExamplePlugin.Test.Axis";
    auto keybinds = g_ctx.coreAPI->keybinds;
    auto format = g_ctx.coreAPI->formatting;

    float val = keybinds->Kbind_GetActionValue(g_ctx.keybindsHandle, actionName);

    char val_buf[64];
    format->Fmt_Format(val_buf, sizeof(val_buf), "Raw Action Value: %.4f", val);
    ui->UI_Text(val_buf);

    // Visualize 0.0 to 1.0 (e.g. Triggers)
    ui->UI_Text("Unipolar (0..1):");
    float uni_fraction = (val < 0.0f) ? 0.0f : val;
    ui->UI_ProgressBar(uni_fraction, -1, 0, "");

    ui->UI_Spacing();

    // Visualize -1.0 to 1.0 (e.g. Sticks)
    ui->UI_Text("Bipolar (-1..1):");
    float bi_fraction = (val + 1.0f) / 2.0f;
    ui->UI_ProgressBar(bi_fraction, -1, 0, "");
    
    ui->UI_Separator();
    ui->UI_Text("Active Bindings Information:");

    int bindingCount = keybinds->Kbind_GetBindingCount(g_ctx.keybindsHandle, actionName);
    if (bindingCount == 0) {
        ui->UI_TextColored(1.0f, 0.5f, 0.5f, 1.0f, "No bindings assigned to this action.");
    } else {
        for (int i = 0; i < bindingCount; ++i) {
            char line_buf[256];
            char name_buf[128];
            keybinds->Kbind_GetBindingName(g_ctx.keybindsHandle, actionName, i, name_buf, sizeof(name_buf));

            format->Fmt_Format(line_buf, sizeof(line_buf), "[Binding %d] Name: %s", i + 1, name_buf);
            if (ui->UI_TreeNode(line_buf)) {
                // 1. Type
                SPF_BindingType type = keybinds->Kbind_GetBindingType(g_ctx.keybindsHandle, actionName, i);
                const char* typeStr = (type == SPF_BINDING_KEYBOARD) ? "Keyboard" :
                                      (type == SPF_BINDING_GAMEPAD) ? "Gamepad Button" :
                                      (type == SPF_BINDING_MOUSE) ? "Mouse Button" :
                                      (type == SPF_BINDING_JOYSTICK) ? "Joystick Button" :
                                      (type == SPF_BINDING_CHORD) ? "Chord" :
                                      (type == SPF_BINDING_GAMEPAD_AXIS) ? "Gamepad Axis" :
                                      (type == SPF_BINDING_MOUSE_AXIS) ? "Mouse Axis" :
                                      (type == SPF_BINDING_JOYSTICK_AXIS) ? "Joystick Axis" : "Unknown";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Type: %s", typeStr);
                ui->UI_BulletText(line_buf);

                // 2. Behavior
                SPF_ActivationBehavior behavior = keybinds->Kbind_GetBindingBehavior(g_ctx.keybindsHandle, actionName, i);
                const char* behaviorStr = (behavior == SPF_BEHAVIOR_HOLD) ? "Hold" :
                                          (behavior == SPF_BEHAVIOR_TOGGLE) ? "Toggle" : "N/A";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Behavior: %s", behaviorStr);
                ui->UI_BulletText(line_buf);

                // 3. Press Type
                SPF_PressType press = keybinds->Kbind_GetBindingPressType(g_ctx.keybindsHandle, actionName, i);
                const char* pressStr = (press == SPF_PRESS_SHORT) ? "Short" :
                                       (press == SPF_PRESS_LONG) ? "Long" : "N/A";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Press Type: %s", pressStr);
                ui->UI_BulletText(line_buf);

                // 4. Mode
                SPF_InputMode mode = keybinds->Kbind_GetBindingMode(g_ctx.keybindsHandle, actionName, i);
                const char* modeStr = (mode == SPF_MODE_ANALOG) ? "Analog" :
                                      (mode == SPF_MODE_DIGITAL) ? "Digital" : "N/A";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Mode: %s", modeStr);
                ui->UI_BulletText(line_buf);

                // 5. Side
                SPF_AxisSide side = keybinds->Kbind_GetBindingSide(g_ctx.keybindsHandle, actionName, i);
                const char* sideStr = (side == SPF_SIDE_POSITIVE) ? "Positive" :
                                      (side == SPF_SIDE_NEGATIVE) ? "Negative" :
                                      (side == SPF_SIDE_BOTH) ? "Both" : "N/A";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Side: %s", sideStr);
                ui->UI_BulletText(line_buf);

                // 6. Accumulator
                SPF_AccumulatorMode acc = keybinds->Kbind_GetBindingAccumulatorMode(g_ctx.keybindsHandle, actionName, i);
                const char* accStr = (acc == SPF_ACCUMULATOR_ON) ? "ON" :
                                     (acc == SPF_ACCUMULATOR_OFF) ? "OFF" : "N/A";
                format->Fmt_Format(line_buf, sizeof(line_buf), "Accumulator: %s", accStr);
                ui->UI_BulletText(line_buf);

                ui->UI_TreePop();
            }
        }
    }
}

void RenderStylingTab(SPF_UI_API* ui, void* user_data) {
    if (!ui->UI_Style_Create) {
        ui->UI_Text("Styling API not available in this version of the framework.");
        return;
    }

    ui->UI_TextWrapped("This tab demonstrates the features of the new Text Styling and Markdown API.");
    ui->UI_Separator();

    // 1. Create style handles
    SPF_TextStyle_Handle h1_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle centered_text_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle separator_style = ui->UI_Style_Create();
    SPF_TextStyle_Handle markdown_base_style = ui->UI_Style_Create();

    // 2. Configure the styles
    ui->UI_Style_SetFont(h1_style, SPF_FONT_H1);
    ui->UI_Style_SetColor(h1_style, 1.0f, 0.84f, 0.0f, 1.0f); // Gold color
    ui->UI_Style_SetAlign(h1_style, SPF_TEXT_ALIGN_CENTER);

    ui->UI_Style_SetAlign(centered_text_style, SPF_TEXT_ALIGN_CENTER);
    ui->UI_Style_SetWrap(centered_text_style, true);
    ui->UI_Style_SetPadding(centered_text_style, 0.f, 10.f);

    ui->UI_Style_SetSeparator(separator_style, true);
    ui->UI_Style_SetColor(separator_style, 0.6f, 0.6f, 0.6f, 1.0f); // Gray

    // 3. Use the styles to render UI
    ui->UI_TextStyled(h1_style, ICON_FA_FONT_AWESOME " Welcome to the Styling API! " ICON_FA_FONT_AWESOME);

    ui->UI_TextStyled(centered_text_style, "This text is centered and will wrap if it becomes too long to fit in the available space. This demonstrates alignment, wrapping, and vertical padding.");

    ui->UI_Spacing();

    // Demonstrate Icons with Styling
    ui->UI_TextStyled(separator_style, ICON_FA_ICONS " Icon Integration Demo");
    ui->UI_Text("You can now use FontAwesome 7 icons directly in your UI!");
    
    // Example row of icons
    ui->UI_Text(ICON_FA_PLAY " Play  " ICON_FA_PAUSE " Pause  " ICON_FA_STOP " Stop  " ICON_FA_FORWARD " Forward");
    
    // Example colored brand icons
    ui->UI_TextColored(0.35f, 0.39f, 0.98f, 1.0f, ICON_FA_DISCORD " Discord");
    ui->UI_SameLine(0, 10);
    ui->UI_TextColored(1.0f, 0.0f, 0.0f, 1.0f, ICON_FA_YOUTUBE " YouTube");
    ui->UI_SameLine(0, 10);
    ui->UI_TextColored(0.1f, 1.0f, 0.1f, 1.0f, ICON_FA_GITHUB " GitHub");

    ui->UI_Spacing();

    ui->UI_TextStyled(separator_style, "Markdown Demo");

    const char* markdown =
        "# Enhanced Markdown Test\n"
        "This is a demonstration of the **SPF v1.1.5** markdown engine.\n\n"
        "--- \n"
        "### 1. Custom Colors & Formatting\n"
        "You can now use {#ff4444}custom RGB colors{/} directly in your text. \n"
        "This is {#44ff44}green text{/}, and this is {#ffcc00}gold text{/}.\n"
        "You can even combine them: **{#ff4444}Bold Red{/}** and *{#44ff44}Italic Green{/}*.\n\n"
        "--- \n\n"
        "Find plugin demonstrations, tutorials, and project updates on our YouTube Channel at [Track'n'Truck](https://www.youtube.com/@TrackAndTruck).\n\n"
        "--- \n"
        "### 2. Lists & Task Lists\n"
        "* Regular bullet point\n"
        "    * Nested point\n\n"
        "* [x] This is a completed task\n"
        "* [ ] This is a pending task\n\n\n"
        "1. First numbered item\n"
        "2. Second numbered item\n\n"
        "--- \n"
        "### 3. Blockquotes & Code\n"
        "> \"The best way to predict the future is to invent it.\"\n"
        "> — Alan Kay\n\n"
        "```cpp\n"
        "// New colors work everywhere!\n"
        "ui->UI_ShowNotification(..., \"{#00ff00}Success!{/}\");\n"
        "```\n"
        "And `inline code` is still here.\n\n"
        "| Header 1 | Header 2 |\n"
        "|----------|----------|\n"
        "| Cell 1   | Cell 2   |\n";
    
    // The base style for markdown can have padding, but the renderer will handle fonts/colors.
    ui->UI_Style_SetPadding(markdown_base_style, 10.0f, 5.0f);
    ui->UI_RenderMarkdown(markdown, markdown_base_style);

    ui->UI_Spacing();
    ui->UI_TextStyled(separator_style, "Notification System Test (v1.1.5)");
    ui->UI_TextWrapped("Testing different display modes and categories.");

    // --- TOP Mode (Standard) ---
    ui->UI_TextDisabled("Top Mode (Replaces existing)");
    if (ui->UI_Button(ICON_FA_CIRCLE_INFO " Info", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_INFO, "This is a **top** notification. It replaces any other top notification.", SPF_NOTIF_MODE_TOP);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button(ICON_FA_CIRCLE_CHECK " Success", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_SUCCESS, "Operation completed! " ICON_FA_THUMBS_UP, SPF_NOTIF_MODE_TOP);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button(ICON_FA_TRIANGLE_EXCLAMATION " Warning", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_WARNING, "Attention! High speed detected.", SPF_NOTIF_MODE_TOP);
    }

    // --- STACK Mode (Bottom Right) ---
    ui->UI_Spacing();
    ui->UI_TextDisabled("Stack Mode (Bottom-Right, Stacks upwards)");
    if (ui->UI_Button(ICON_FA_LAYER_GROUP " Add Stacked Notif", 0, 0)) {
        static int notif_count = 0;
        char buf[128];
        sprintf(buf, "Stacked message #%d\nThis will push older ones up.", ++notif_count);
        ui->UI_ShowNotification(SPF_NOTIFICATION_INFO, buf, SPF_NOTIF_MODE_STACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button(ICON_FA_CIRCLE_XMARK " Stack Error", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_ERROR, "A stacked error occurred!", SPF_NOTIF_MODE_STACK);
    }

    // --- STICKY Mode ---
    ui->UI_Spacing();
    ui->UI_TextDisabled("Sticky Mode (At cursor, no timeout)");
    if (ui->UI_Button(ICON_FA_THUMBTACK " Toggle Sticky Help", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_HINT, 
            "**Sticky Help Tip**\n\n"
            "This window has no timeout. It will stay here until:\n"
            "1. You click the button again (Toggle).\n"
            "2. You click anywhere outside this notification.\n\n"
            "Useful for explaining complex UI elements!", 
            SPF_NOTIF_MODE_STICKY);
    }

    ui->UI_Spacing();
    if (ui->UI_Button("Test Long Text (Top)", 0, 0)) {
        ui->UI_ShowNotification(SPF_NOTIFICATION_INFO, 
            "This is a very long notification message designed to test the \n*automatic text wrapping* and **dynamic height** \nadjustment of the ***notification window***. "
            "It should handle multiple lines of text gracefully without cutting off the content or expanding beyond reasonable bounds.", 
            SPF_NOTIF_MODE_TOP);
    }

    ui->UI_Spacing();
    ui->UI_TextStyled(separator_style, "Window Management Test (v1.1.5)");
    ui->UI_TextWrapped("Use the button below to center the window and resize it to fit all tabs.");
    if (ui->UI_Button("Center and Fit Window", 0, 0)) {
        const char* tabs[] = {
            "General", "Traffic Inspector", "Camera", "Telemetry", 
            "Events", "Virtual Input", "Styling API", "Environment", "Input Test"
        };
        
        float total_width = 0;
        SPF_Style_Handle* style = ui->UI_GetStyle();
        float frame_padding_x, frame_padding_y;
        ui->UI_Style_GetFramePadding(style, &frame_padding_x, &frame_padding_y);
        
        // Tab bars in ImGui have specific spacing. We'll approximate.
        // Usually it's text_width + frame_padding.x * 2 for each tab.
        for (const char* tab : tabs) {
            float w, h;
            // Using a typical default font size of 18.0f for the framework
            ui->UI_CalcTextSizeWithFont(SPF_FONT_REGULAR, 18.0f, tab, &w, &h);
            total_width += w + (frame_padding_x * 2.0f) + 4.0f; // 4.0f is a small extra margin between tabs
        }
        
        // Add window padding and some extra space for the close button/decorations
        float win_padding_x, win_padding_y;
        ui->UI_Style_GetWindowPadding(style, &win_padding_x, &win_padding_y);
        total_width += (win_padding_x * 2.0f) + 20.0f; 

        float v_w, v_h;
        ui->UI_GetViewportSize(&v_w, &v_h);
        
        float win_h = 450.0f; // Desired height
        float pos_x = (v_w - total_width) * 0.5f;
        float pos_y = (v_h - win_h) * 0.5f;

        ui->UI_SetWindowPos(pos_x, pos_y, SPF_COND_ALWAYS);
        ui->UI_SetWindowSize(total_width, win_h, SPF_COND_ALWAYS);
        
        ui->UI_ShowNotification(SPF_NOTIFICATION_SUCCESS, "Window centered and resized to fit all tabs!", SPF_NOTIF_MODE_TOP);
    }

    ui->UI_Spacing();
    ui->UI_TextStyled(separator_style, "Custom Gradient API Test (v1.1.5)");
    ui->UI_TextWrapped("Demonstrating multi-color primitives for advanced custom widgets.");

    float canvas_x, canvas_y;
    ui->UI_GetCursorScreenPos(&canvas_x, &canvas_y);
    SPF_DrawList_Handle dl = ui->UI_GetWindowDrawList();

    // 1. Multi-Color Triangle
    uint32_t col_r = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.0f, 0.0f, 1.0f);
    uint32_t col_g = ui->UI_ColorConvertFloat4ToU32(0.0f, 1.0f, 0.0f, 1.0f);
    uint32_t col_b = ui->UI_ColorConvertFloat4ToU32(0.0f, 0.0f, 1.0f, 1.0f);
    ui->UI_DrawList_AddTriangleFilledMultiColor(dl, canvas_x + 50, canvas_y + 10, canvas_x + 10, canvas_y + 90, canvas_x + 90, canvas_y + 90, col_r, col_g, col_b);

    // 2. Radial Gradient Circle
    uint32_t col_white = ui->UI_ColorConvertFloat4ToU32(1.0f, 1.0f, 1.0f, 1.0f);
    uint32_t col_gold = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.84f, 0.0f, 1.0f);
    ui->UI_DrawList_AddCircleFilledMultiColor(dl, canvas_x + 150, canvas_y + 50, 40.0f, col_white, col_gold, 32);

    // 3. Multi-Color Rect (Linear Gradient)
    uint32_t col_tl = ui->UI_ColorConvertFloat4ToU32(1.0f, 0.0f, 1.0f, 1.0f); // Magenta
    uint32_t col_tr = ui->UI_ColorConvertFloat4ToU32(0.0f, 1.0f, 1.0f, 1.0f); // Cyan
    uint32_t col_br = ui->UI_ColorConvertFloat4ToU32(1.0f, 1.0f, 0.0f, 1.0f); // Yellow
    uint32_t col_bl = ui->UI_ColorConvertFloat4ToU32(0.0f, 0.0f, 0.0f, 1.0f); // Black
    ui->UI_DrawList_AddRectFilledMultiColor(dl, canvas_x + 220, canvas_y + 10, canvas_x + 350, canvas_y + 90, col_tl, col_tr, col_br, col_bl);

    ui->UI_Dummy(360, 100); // Reserve space for custom drawing

    ui->UI_Spacing();
    ui->UI_TextStyled(separator_style, "Screen Transition API Test");
    ui->UI_TextWrapped("Test the cinematic screen transitions implemented in the framework.");

    if (ui->UI_Button("Fade To Black (2s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_FADE, 2.0f, false, SPF_TRANS_COLOR_BLACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Fade From Black (2s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_FADE, 2.0f, true, SPF_TRANS_COLOR_BLACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Cross Black (3s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_CROSS, 3.0f, false, SPF_TRANS_COLOR_BLACK);
    }

    if (ui->UI_Button("Fade To White (1s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_FADE, 1.0f, false, SPF_TRANS_COLOR_WHITE);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Flash White (0.5s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_FLASH, 0.5f, false, SPF_TRANS_COLOR_WHITE);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Cross White (2s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_CROSS, 2.0f, false, SPF_TRANS_COLOR_WHITE);
    }

    if (ui->UI_Button("Letterbox IN (1.5s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_LETTERBOX, 1.5f, false, SPF_TRANS_COLOR_BLACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Letterbox OUT (1.5s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_LETTERBOX, 1.5f, true, SPF_TRANS_COLOR_BLACK);
    }

    if (ui->UI_Button("Wipe Right (Gray, 1s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_WIPE_RIGHT, 1.0f, false, SPF_TRANS_COLOR_GRAY);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Wipe Down (Sepia, 1s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_WIPE_BOTTOM, 1.0f, false, SPF_TRANS_COLOR_SEPIA);
    }

    if (ui->UI_Button("Shutter H (2s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_SHUTTER_H, 2.0f, false, SPF_TRANS_COLOR_BLACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Shutter V (2s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_SHUTTER_V, 2.0f, false, SPF_TRANS_COLOR_BLACK);
    }
    ui->UI_SameLine(0, 5);
    if (ui->UI_Button("Radial (3s)", 0, 0)) {
        ui->UI_PlayTransition(SPF_TRANS_RADIAL, 3.0f, false, SPF_TRANS_COLOR_BLACK);
    }
    
    // 4. Clean up the style handles
    ui->UI_Style_Destroy(h1_style);
    ui->UI_Style_Destroy(centered_text_style);
    ui->UI_Style_Destroy(separator_style);
    ui->UI_Style_Destroy(markdown_base_style);
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
void InitializeVirtualDevice(SPF_VirtInput_API* input_api, SPF_Logger_API* logger_api) {
    if (!input_api || !logger_api) return;

    auto logger = logger_api->Log_GetContext(PLUGIN_NAME);
    g_ctx.virtualDevice = input_api->Virt_CreateDevice(
        PLUGIN_NAME,
        "Example_virtual_device",
        "ExamplePlugin Virtual Controller",
        SPF_INPUT_DEVICE_TYPE_GENERIC
    );

    if (!g_ctx.virtualDevice) {
        logger_api->Log(logger, SPF_LOG_ERROR, "Failed to create virtual device.");
        return;
    }

    // Add a button and an axis to the virtual device.
    input_api->Virt_AddButton(g_ctx.virtualDevice, "virt_honk", "Virtual Honk");
    input_api->Virt_AddAxis(g_ctx.virtualDevice, "virt_throttle", "Virtual Throttle");

    // Register the device with the framework to make it active.
    if (input_api->Virt_Register(g_ctx.virtualDevice)) {
        logger_api->Log(logger, SPF_LOG_INFO, "Successfully registered virtual device.");
    } else {
        logger_api->Log(logger, SPF_LOG_ERROR, "Failed to register virtual device.");
    }
}

/**
 * @brief Parses the `a_complex_object` setting to demonstrate `Cfg_GetJsonValueHandle` and `JsonReaderApi`.
 * @details This function retrieves a complex JSON object from the config using `Cfg_GetJsonValueHandle`
 *          and then uses the `JsonReaderApi` to extract nested values. It showcases how to handle
 *          advanced configuration structures within a plugin.
 */
void ParseComplexObject() {
    // Ensure all required APIs are available. The JsonReader is retrieved from the Core API.
    if (!g_ctx.coreAPI || !g_ctx.coreAPI->config || !g_ctx.coreAPI->json_reader) {
        return;
    }

    auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);
    auto config = g_ctx.coreAPI->config;
    auto config_handle = config->Cfg_GetContext(PLUGIN_NAME);
    const auto* json_reader = g_ctx.coreAPI->json_reader;

    char log_buffer[512]; // Increased buffer size for potentially long strings

    // 1. Get the handle to the complex JSON object from the Config API.
    const SPF_JsonValue_Handle* object_h = config->Cfg_GetJsonValueHandle(config_handle, "settings.a_complex_object");

    if (object_h) {
        g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Parsing complex object 'settings.a_complex_object':");
        g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);

        // 2. Use the JsonReader API to check for and get the 'mode' member.
        if (json_reader->Json_HasMember(object_h, "mode")) {
            const SPF_JsonValue_Handle* mode_h = json_reader->Json_GetMember(object_h, "mode");
            if (mode_h && json_reader->Json_GetType(mode_h) == SPF_JSON_TYPE_STRING) {
                char mode_str[64];
                json_reader->Json_GetString(mode_h, mode_str, sizeof(mode_str));
                g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "  -> Mode: %s", mode_str);
                g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
            }
        }

        // 3. Get the 'enabled' member.
        const SPF_JsonValue_Handle* enabled_h = json_reader->Json_GetMember(object_h, "enabled");
        if (enabled_h && json_reader->Json_GetType(enabled_h) == SPF_JSON_TYPE_BOOLEAN) {
            bool enabled_val = json_reader->Json_GetBool(enabled_h, false);
            g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "  -> Enabled: %s", enabled_val ? "true" : "false");
            g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buffer);
        }

        // 4. Get the 'targets' array and iterate through it.
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

    g_ctx.coreAPI->hooks->Hook_Register(
        PLUGIN_NAME,
        "GameStringFormattingHook", // Renamed hook ID for consistency
        "Game String Formatting Hook", // Renamed hook description for consistency
        reinterpret_cast<void*>(Detour_GameStringFormatting), // Our detour function
        reinterpret_cast<void**>(&g_ctx.o_GameStringFormatting), // Pointer to store the original
        signature,
        true // Enable the hook immediately
    );
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Registered 'GameStringFormatting' hook.");
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
                g_ctx.loadAPI->logger->Log(g_ctx.loadAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Overriding 'quit_game' button color.");
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
 *                     to the plugin's `BuildManifest` function.
 * @return `true` on success, `false` on failure.
 * @details This is the very first function the framework calls. It allows the framework to get
 *          the plugin's manifest *before* the plugin is fully loaded.
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
        exports->OnLanguageChanged = OnLanguageChanged; // Export the new language sync callback
        return true;
    }
    return false;
}

} // extern "C"

} // namespace ExamplePlugin