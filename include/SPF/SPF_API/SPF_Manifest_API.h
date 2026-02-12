/**
 * @file SPF_Manifest_API.h
 * @brief API for defining plugin identity, configuration defaults, and UI layout.
 *
 * @details The Manifest API is the standard interface for plugin initialization.
 * During the boot phase, the framework calls the plugin's `BuildManifest` function
 * and provides a function table. The plugin calls these functions to register its
 * metadata, default settings, and UI requirements.
 * 
 * =================================================================================================
 * COMPREHENSIVE USAGE EXAMPLE
 * =================================================================================================
 * @code
 * // Recommended: Use a constant for your plugin name.
 * const char* PLUGIN_NAME = "MySuperPlugin";
 *
 * void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
 *
 *     // --- 1. Basic Identity ---
 *     api->Info_SetName(h, PLUGIN_NAME);               // Internal unique ID (No spaces)
 *     api->Info_SetVersion(h, "1.2.0");                // Semantic version (e.g. "1.0.0")
 *     api->Info_SetAuthor(h, "John Doe");              // Displayed author name
 *     api->Info_SetMinFrameworkVersion(h, "1.0.6");    // Prevents load on incompatible framework
 *     api->Info_SetDescriptionLiteral(h, "Main Description."); // Fallback if no translation found
 *     api->Info_SetWebsiteUrl(h, "https://example.com");
 *
 *     // --- 2. Configuration Policy ---
 *     api->Policy_SetAllowUserConfig(h, true);           // Enables 'settings.json' and UI menu
 *     api->Policy_AddConfigurableSystem(h, "settings");  // Shows 'Custom Settings' tab in UI
 *     api->Policy_AddConfigurableSystem(h, "logging");   // Shows 'Logging' tab in UI
 *     api->Policy_AddRequiredHook(h, "GameConsole");     // Enforces a mandatory hook dependency
 *
 *     // --- 3. Custom Settings Defaults ---
 *     // This JSON defines default values ONLY for the "settings" section of 'settings.json'.
 *     // These variables are specific to your plugin's internal logic.
 *     const char* defaults = R"json({
 *         "rendering": {
 *             "fov": 75.0,
 *             "mode": "high"
 *         },
 *         "theme_color": [1.0, 0.5, 0.0]
 *     })json";
 *     api->Settings_SetJson(h, defaults);
 *
 *     // --- 4. Standard System Defaults ---
 *     
 *     // Logging: Level ("trace", "debug", "info", "warn", "error", "critical"), FileSink (bool)
 *     api->Defaults_SetLogging(h, "info", true);
 *
 *     // Keybinds (Universal): Group, Action, Type, Key, Consume
 *
 *     // Example 1: Digital keyboard binding (F5 to toggle menu)
 *     api->Defaults_AddKeybind(h, "MyPlugin.Main", "toggle", "keyboard", "KEY_F5", "always");
 *
 *     // Example 2: Analog gamepad binding (Right Trigger for throttle)
 *     // Note: Use "_AXIS" suffix for gamepad triggers and sticks to enable analog mode.
 *     api->Defaults_AddKeybind(h, "MyPlugin.Vehicle", "throttle", "gamepad_axis", "RIGHT_TRIGGER_AXIS", "never");
 *
 *     // Example 3: Mouse wheel binding (Axis index 2)
 *     api->Defaults_AddKeybind(h, "MyPlugin.UI", "zoom", "mouse_axis", "2", "on_ui_focus");
 *
 *     // Windows: Name, Visible (bool), Interactive (bool), X, Y, W, H, Collapsed (bool), AutoScroll (bool)
 *     api->Defaults_AddWindow(h, "MainWindow", true, true, 100, 100, 400, 300, false, false);
 *
 *     // --- 5. UI Metadata (Widgets & Hints) ---
 *     // Links keys from step 3 to specific UI widgets and provides descriptions.
 *     
 *     // Slider for 'rendering.fov'
 *     api->Meta_AddCustomSetting(h, "rendering.fov", "Field of View", "Set preferred FOV",
 *                                "slider", "{ \"min\": 60.0, \"max\": 120.0, \"format\": \"%.0f deg\" }", false);
 *
 *     // Dropdown for 'rendering.mode'
 *     const char* modeOpts = "{ \"options\": [ { \"value\": \"low\", \"labelKey\": \"Low\" }, { \"value\": \"high\", \"labelKey\": \"High\" } ] }";
 *     api->Meta_AddCustomSetting(h, "rendering.mode", "Quality", nullptr, "combo", modeOpts, false);
 *
 *     // UI Metadata for standard elements
 *     api->Meta_AddKeybind(h, "MyPlugin.Main", "toggle", "Toggle Menu", "Opens the main UI.");
 *     api->Meta_AddWindow(h, "MainWindow", "Main Plugin UI", "Primary interaction window.");
 * }
 *
 * // Standard export function required by the framework
 * extern "C" SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
 *     if (!out_api) return false;
 *     out_api->BuildManifest = MyPlugin_BuildManifest;
 *     return true;
 * }
 * @endcode
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// Opaque Handle Definition
// =================================================================================================

/**
 * @brief Opaque handle representing the internal Manifest Builder object.
 * @details The plugin holds this handle but cannot access its memory directly.
 * All manipulations must be done via the function pointers provided in `SPF_Manifest_Builder_API`.
 */
typedef struct SPF_Manifest_Builder_Handle SPF_Manifest_Builder_Handle;

// =================================================================================================
// Manifest Builder API (Function Table)
// =================================================================================================

/**
 * @struct SPF_Manifest_Builder_API
 * @brief A table of function pointers provided by the framework to the plugin.
 * @details The plugin uses these functions to populate the manifest data during the initialization phase.
 */
typedef struct SPF_Manifest_Builder_API {
    
    // -------------------------------------------------------------------------
    // 1. Plugin Information (InfoData)
    // -------------------------------------------------------------------------
    // These functions define the identity of the plugin.
    
    /**
     * @brief Sets the internal programmatic name of the plugin.
     * @param h The builder handle.
     * @param name The unique name (e.g., "MyPlugin"). MUST match the name used in `GetContext` calls.
     *             No spaces or special characters allowed.
     */
    void (*Info_SetName)(SPF_Manifest_Builder_Handle* h, const char* name);

    /**
     * @brief Sets the display version of the plugin.
     * @param h The builder handle.
     * @param version The version string (recommended: Semantic Versioning, e.g., "1.0.0").
     */
    void (*Info_SetVersion)(SPF_Manifest_Builder_Handle* h, const char* version);

    /**
     * @brief Sets the minimum required version of the SPF Framework.
     * @details If the running framework version is lower than this, the plugin will fail to load
     *          and an error will be displayed to the user.
     * @param h The builder handle.
     * @param version The version string (e.g., "1.0.6").
     */
    void (*Info_SetMinFrameworkVersion)(SPF_Manifest_Builder_Handle* h, const char* version);

    /**
     * @brief Sets the author's name.
     * @param h The builder handle.
     * @param author The author's name or organization.
     */
    void (*Info_SetAuthor)(SPF_Manifest_Builder_Handle* h, const char* author);

    /**
     * @brief Sets the localization key for the plugin's description.
     * @details The framework will look up this key in the plugin's locale files.
     * @param h The builder handle.
     * @param key The localization key (e.g., "plugin.description").
     */
    void (*Info_SetDescriptionKey)(SPF_Manifest_Builder_Handle* h, const char* key);

    /**
     * @brief Sets a literal description string (fallback if key is missing).
     * @param h The builder handle.
     * @param desc The description text.
     */
    void (*Info_SetDescriptionLiteral)(SPF_Manifest_Builder_Handle* h, const char* desc);

    /**
     * @brief Sets the author's contact email.
     * @param h The builder handle.
     * @param email The email address (e.g., "mailto:dev@example.com").
     */
    void (*Info_SetEmail)(SPF_Manifest_Builder_Handle* h, const char* email);

    // --- Social Media & Project Links ---
    // These URLs will be displayed in the plugin's "About" section in the UI.
    
    void (*Info_SetDiscordUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetSteamProfileUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetGithubUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetYoutubeUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetScsForumUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetPatreonUrl)(SPF_Manifest_Builder_Handle* h, const char* url);
    void (*Info_SetWebsiteUrl)(SPF_Manifest_Builder_Handle* h, const char* url);

    // -------------------------------------------------------------------------
    // 2. Configuration Policy
    // -------------------------------------------------------------------------
    // Defines how the plugin interacts with the framework's configuration system
    // and declares its mandatory dependencies.

    /**
     * @brief Enables or disables the framework-managed configuration for the plugin.
     * @details If set to 'true':
     *          1. The framework automatically creates and manages a 'settings.json' file 
     *             for the plugin (located in /plugins/PluginName/config/).
     *          2. The plugin will appear in the main "Settings" menu of the framework.
     *          3. User changes in the UI will be automatically saved to disk.
     * 
     * @param h The builder handle.
     * @param allow Set to 'true' to enable the automatic settings system.
     */
    void (*Policy_SetAllowUserConfig)(SPF_Manifest_Builder_Handle* h, bool allow);

    /**
     * @brief Registers a standard framework system to be visible in the plugin's settings UI.
     * @details Only systems added via this function will have a corresponding tab in the 
     *          plugin's configuration window.
     * 
     * @param h The builder handle.
     * @param systemName One of the following predefined system identifiers:
     *                   - "settings"     : Shows the "Custom Settings" tab (variables from Settings_SetJson).
     *                   - "logging"      : Shows the "Logging" tab (log levels and file output).
     *                   - "localization" : Shows the "Language" tab (language selection).
     *                   - "ui"           : Shows the "UI / Windows" tab (window positions and visibility).
     * 
     * @note The "keybinds" system is always enabled by default if any keybinds are defined 
     *       and does not need to be manually added here.
     */
    void (*Policy_AddConfigurableSystem)(SPF_Manifest_Builder_Handle* h, const char* systemName);

    /**
     * @brief Declares a mandatory dependency on a framework-level function hook.
     * @details Use this to ensure that a specific hook (e.g., "GameConsole") is active 
     *          whenever your plugin is loaded. The framework will enforce this and 
     *          prevent the user from disabling the required hook via the UI.
     * 
     * @param h The builder handle.
     * @param hookName The internal name of the hook (e.g., "GameConsole", "GameLogHook").
     */
    void (*Policy_AddRequiredHook)(SPF_Manifest_Builder_Handle* h, const char* hookName);

    // -------------------------------------------------------------------------
    // 3. Custom Settings Defaults
    // -------------------------------------------------------------------------
    // Use this section to define variables that are unique to your plugin.

    /**
     * @brief Sets the default JSON structure for the plugin's custom variables.
     * @details This function defines the initial state of the "settings": { ... } 
     *          section within the plugin's 'settings.json'. 
     * 
     * @important Do NOT include "logging", "ui", or "keybinds" keys in this JSON. 
     *            Those sections are managed automatically by the 'Defaults_...' functions.
     * 
     * @param h The builder handle.
     * @param json A string containing a valid JSON object.
     *             Example: 
     *             R"json({
     *                 "enable_visuals": true,
     *                 "update_interval_ms": 100,
     *                 "colors": { "primary": [1.0, 1.0, 1.0] }
     *             })json"
     */
    void (*Settings_SetJson)(SPF_Manifest_Builder_Handle* h, const char* json);

    // -------------------------------------------------------------------------
    // 4. Default Configurations for Framework Systems
    // -------------------------------------------------------------------------

    /**
     * @brief Configures default logging behavior (populates "logging" section).
     * @param h The builder handle.
     * @param level Default log level: "trace", "debug", "info", "warn", "error", or "critical".
     * @param fileSink Enable writing to [Plugin]/logs/[Plugin].log.
     */
    void (*Defaults_SetLogging)(SPF_Manifest_Builder_Handle* h, const char* level, bool fileSink);

    /**
     * @brief Sets the default language code.
     * @param h The builder handle.
     * @param langCode The ISO language code (e.g., "en", "ua", "de"). Matches the translation file name.
     */
    void (*Defaults_SetLocalization)(SPF_Manifest_Builder_Handle* h, const char* langCode);

    /**
     * @brief Registers a default binding (key, button, or axis) for a logical action.
     * 
     * @details This is a high-level, universal function used to define default input assignments 
     *          in the plugin's manifest. The framework handles all low-level configuration and 
     *          initialization based on the 'type' parameter.
     * 
     *          ### 1. Automatic Defaulting & Internal Logic
     * 
     *          When this function is called, the framework populates a complex internal binding 
     *          structure with standard defaults. These defaults vary significantly between 
     *          Digital (buttons) and Analog (axes) inputs:
     * 
     *          #### A. Digital Inputs ("keyboard", "gamepad")
     *          Optimized for binary on/off actions (e.g., "Open Menu", "Honk").
     *          - @b press_type: Defaults to "short" (triggers immediately on press).
     *          - @b press_threshold_ms: Defaults to 500ms (relevant if user later switches to "long" press in UI).
     *          - @b behavior: Defaults to "toggle" (standard one-time trigger).
     *          - @b value: Internally produces 0.0 (released) or 1.0 (pressed).
     * 
     *          #### B. Analog Inputs ("gamepad_axis", "mouse_axis", "joystick_axis")
     *          Optimized for continuous movement (e.g., "Throttle", "Steering", "Scroll").
     *          - @b mode: Defaults to "analog" (provides a smooth float value).
     *          - @b deadzone: Defaults to 0.0 (raw input passed initially).
     *          - @b saturation: Defaults to 1.0 (reaches max value at full physical deflection).
     *          - @b sensitivity: Defaults to 1.0 (linear scale multiplier).
     *          - @b curve: Defaults to "linear" (no transformation applied).
     *          - @b side: Defaults to "both" (uses the full range of the axis [-1, 1]). Can also be "positive" (0 to 1) or "negative" (0 to -1, normalized to positive).
     *          - @b smoothing: Defaults to 0.0 (no temporal interpolation).
     *          - @b invert: Defaults to false.
     *          - @b range_min/max: Defaults to -1.0 and 1.0 (standard for sticks/joysticks).
     *          - @b threshold: Defaults to 0.5 (used if the user later switches mode to "digital" in UI).
     * 
     * @param h          The manifest builder handle.
     * @param groupName  Action group identifier (e.g., "MyPlugin.Movement"). Used for UI grouping.
     * @param actionName The logical action name (e.g., "forward"). 
     *                   The full logical action key is formed as "groupName.actionName".
     * @param type       Input category. Supported values:
     *                   - "keyboard"      : Standard physical keys.
     *                   - "mouse"         : Mouse buttons.
     *                   - "gamepad"       : Digital gamepad buttons (FACE_DOWN, FACE_RIGHT, SPECIAL_RIGHT...).
     *                   - "gamepad_axis"  : Analog sticks and triggers (LEFT_STICK_X, RIGHT_TRIGGER_AXIS...).
     *                   - "mouse_axis"    : Mouse wheel (Index 2).
     *                   - "joystick_axis" : Generic joystick/pedal axes (Index 0, 1, 2...).
     * @param key        Physical input identifier:
     *                   - For @b keyboard: Use Virtual Key names ("KEY_W", "KEY_SPACE", "KEY_ESCAPE", "KEY_F1"...).
     *                   - For @b mouse: Use Button names ("MOUSE_LEFT", "MOUSE_RIGHT", "MOUSE_MIDDLE", "MOUSE_X1"...).
     *                   - For @b gamepad: Use Button names ("FACE_DOWN", "FACE_RIGHT", "SPECIAL_RIGHT", "LEFT_SHOULDER"...).
     *                   - For @b gamepad_axis: Use Axis names ("LEFT_STICK_X", "LEFT_STICK_Y", "RIGHT_TRIGGER_AXIS"...).
     *                   - For @b mouse_axis: Use "2" for the vertical scroll wheel.
     *                   - For @b joystick: Use "BUTTON_" prefix with 1-based index (e.g., "BUTTON_1", "BUTTON_12").
     *                   - For @b joystick_axis: Use the 0-based index of the axis as a string (e.g., "0", "5").
     * @param consume    Input consumption policy:
     *                   - "never"       : The game always receives the input.
     *                   - "always"      : Input is captured by the framework and completely hidden from the game.
     *                   - "on_ui_focus" : Input is blocked only when a framework window is focused.
     *                   - "manual"      : The plugin controls the blocking state via Kbind_SetBlockState().
     */
    void (*Defaults_AddKeybind)(SPF_Manifest_Builder_Handle* h, 
                                const char* groupName, 
                                const char* actionName, 
                                const char* type, 
                                const char* key, 
                                const char* consume);

    /**
     * @brief Adds a default UI window configuration.
     * @param h The builder handle.
     * @param windowName Unique identifier for the window.
     * @param isVisible Default visibility state.
     * @param isInteractive If true, receives mouse/keyboard input.
     * @param x Default X position.
     * @param y Default Y position.
     * @param w Default Width.
     * @param h Default Height.
     * @param isCollapsed Default collapsed state.
     * @param autoScroll Default auto-scroll state.
     */
    void (*Defaults_AddWindow)(SPF_Manifest_Builder_Handle* h, 
                               const char* windowName, 
                               bool isVisible, 
                               bool isInteractive, 
                               int x, int y, int w, int height, 
                               bool isCollapsed, 
                               bool autoScroll);

    // -------------------------------------------------------------------------
    // 5. Metadata (UI Definitions)
    // -------------------------------------------------------------------------
    // These functions describe how the settings and elements defined above should be displayed in the UI.

    /**
     * @brief Adds metadata for a custom setting to control its UI representation.
      * 
     * @param h The builder handle.
     * @param keyPath The JSON path to the setting (e.g., "my_feature.speed").
     * @param titleKey Localization key for the setting title.
     * @param descKey Localization key for the setting description.
     * @param widgetType (Optional) The type of widget to use. Pass NULL for auto-detection.
     *                   Valid types:
     *                   - "input": Simple text/number input (default).
     *                   - "input_double": Number input for doubles.
     *                   - "input_with_hint": Text input with a placeholder hint.
     *                   - "slider": Horizontal slider (float/int).
     *                   - "vslider": Vertical slider.
     *                   - "drag": Draggable number field.
     *                   - "combo": Dropdown list.
     *                   - "radio": Radio button group.
     *                   - "color3": RGB color picker.
     *                   - "multiline": Multi-line text area.
     * 
     * @param widgetParamsJson (Optional) JSON string defining widget parameters. Pass NULL if none.
     *                         
     *                         **Widget Parameter Formats:**
     *                         
     *                         **slider / drag / vslider:**
     *                         {
     *                           "min": 0.0,       // Minimum value
     *                           "max": 100.0,     // Maximum value
     *                           "format": "%.1f", // Display format (printf style)
     *                           "step": 1.0,      // (Drag only) Speed/Step
     *                           "width": 30.0,    // (VSlider only) Width in pixels
     *                           "height": 100.0,  // (VSlider only) Height in pixels
     *                           "logarithmic": false // (Slider only) Logarithmic scale
     *                         }
     *
     *                         **combo / radio:**
     *                         {
     *                           "options": [
     *                             { "value": "val1", "labelKey": "opt.one" },
     *                             { "value": 2, "labelKey": "opt.two" }
     *                           ]
     *                         }
     *
     *                         **input_double:**
     *                         { "step": 0.01, "format": "%.4f" }
     *
     *                         **input_with_hint:**
     *                         { "hint": "Type here..." }
     *
     *                         **color3:**
     *                         { "flags": 0 } // ImGuiColorEditFlags
     *
     *                         **multiline:**
     *                         { "height_in_lines": 4 }
     *
     * @param hideInUI If true, this setting will not appear in the Settings UI.
     */
    void (*Meta_AddCustomSetting)(SPF_Manifest_Builder_Handle* h, 
                                  const char* keyPath, 
                                  const char* titleKey, 
                                  const char* descKey, 
                                  const char* widgetType, 
                                  const char* widgetParamsJson, 
                                  bool hideInUI);

    /**
     * @brief Adds metadata for a keybind action.
     * @param h The builder handle.
     * @param groupName The group name (must match `Defaults_AddKeybind`).
     * @param actionName The action name (must match `Defaults_AddKeybind`).
     * @param titleKey Localization key for the title.
     * @param descKey Localization key for the description.
     */
    void (*Meta_AddKeybind)(SPF_Manifest_Builder_Handle* h, const char* groupName, const char* actionName, const char* titleKey, const char* descKey);

    /**
     * @brief Adds metadata for a UI window.
     * @param h The builder handle.
     * @param windowName The window name (must match `Defaults_AddWindow`).
     * @param titleKey Localization key for the title.
     * @param descKey Localization key for the description.
     */
    void (*Meta_AddWindow)(SPF_Manifest_Builder_Handle* h, const char* windowName, const char* titleKey, const char* descKey);

    /**
     * @brief Adds metadata for a standard framework setting (logging/localization).
     * @param h The builder handle.
     * @param system System name: "logging" or "localization".
     * @param key The specific setting key (e.g., "level", "language").
     * @param titleKey Localization key for the title.
     * @param descKey Localization key for the description.
     */
    void (*Meta_AddStandardSetting)(SPF_Manifest_Builder_Handle* h, const char* system, const char* key, const char* titleKey, const char* descKey);

} SPF_Manifest_Builder_API;

// =================================================================================================
// Plugin Export Definition
// =================================================================================================

/**
 * @typedef SPF_BuildManifest_t
 * @brief Function pointer type for the plugin's manifest builder function.
 * @details The plugin must implement a function matching this signature.
 */
typedef void (*SPF_BuildManifest_t)(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api);

/**
 * @struct SPF_Manifest_API
 * @brief Structure used to pass the builder function pointer from the plugin to the framework.
 */
typedef struct SPF_Manifest_API {
    /**
     * @brief Pointer to the plugin's implementation of the manifest builder.
     * @details The framework will call this function, providing a handle and the API table.
     */
    SPF_BuildManifest_t BuildManifest;
} SPF_Manifest_API;


/**
 * @brief The name of the export function the plugin must implement to provide manifest info.
 * Signature: bool SPF_GetManifestAPI(SPF_Manifest_API* out_api);
 */
#define SPF_GetManifestAPI_ExportName "SPF_GetManifestAPI"

// Typedef for the export function itself (for creating the export definition)
typedef bool (*SPF_GetManifestAPI_Func)(SPF_Manifest_API* out_api);


#ifdef __cplusplus
}
#endif
