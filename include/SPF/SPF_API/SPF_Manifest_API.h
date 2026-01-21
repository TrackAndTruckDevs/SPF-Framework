/**
 * @file SPF_Manifest_API.h
 * @brief Defines the C-style API for a plugin to declare its metadata,
 * default settings, and requirements to the framework.
 *
 * The manifest is a contract between the plugin and the SPF core. It allows
 * the core to understand the plugin's identity, how it should be configured,
 * and what framework features it intends to use, all *before* the plugin is
 * fully loaded and its `OnLoad` function is called.
 *
 * This C-API is designed for stability across different compilers. The plugin
 * implements a function that fills these C-style structs, and the framework
 * internally converts this data into its more flexible C++ representations.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// Manifest Constants
// =================================================================================================

// Define maximum lengths for strings and array counts to be copied into C-style arrays.
// These ensure that the data passed from the plugin does not overflow the framework's buffers.
#define SPF_MANIFEST_MAX_STRING_LENGTH 256
#define SPF_MANIFEST_MAX_SYSTEMS 32
#define SPF_MANIFEST_MAX_HOOKS 32
#define SPF_MANIFEST_MAX_KEYBINDS_PER_ACTION 16
#define SPF_MANIFEST_MAX_ACTIONS_PER_GROUP 64
#define SPF_MANIFEST_MAX_GROUPS 64
#define SPF_MANIFEST_MAX_WINDOWS 32

// =================================================================================================
// Manifest Data Blocks (C-compatible)
// =================================================================================================

/**
 * @struct SPF_InfoData_C
 * @brief Contains essential metadata about the plugin.
 * This information is used for display in UI, for logging, and for identification.
 */
typedef struct SPF_InfoData_C {
    // (Optional) A unique name for the plugin (e.g. "MyPlugin"). MUST be unique. If not specified it will be taken from the name of your dll, possible conflicts
    char name[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) The plugin's version (e.g., "1.0.0").
    char version[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional, but recommended) The minimum version of the SPF Framework required for this plugin to function correctly (e.g., "1.0.6").
    // If the user's framework version is lower than this, the plugin will be disabled. This prevents crashes due to API changes.
    char min_framework_version[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) The name of the author or organization.
    char author[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) A key for a localized description string from your translation files.
    char descriptionKey[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) A fallback description if the localization key is not found.
    char descriptionLiteral[SPF_MANIFEST_MAX_STRING_LENGTH];

     // --- Social and Project Links ---
    // (Optional) Contact email for the author.
    char email[SPF_MANIFEST_MAX_STRING_LENGTH];    
    // (Optional) URL to a Discord server invite.
    char discordUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to a Steam profile.
    char steamProfileUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to the GitHub repository.
    char githubUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to a YouTube channel/video.
    char youtubeUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to an SCS Software forum thread.
    char scsForumUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to a Patreon page.
    char patreonUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
    // (Optional) URL to a personal or project website.
    char websiteUrl[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_InfoData_C;

/**
 * @struct SPF_ConfigPolicyData_C
 * @brief Defines the plugin's policies regarding configuration.
 */
typedef struct SPF_ConfigPolicyData_C {
    // If `true`, the framework will allow users to override the plugin's settings
    // via a `settings.json` file.
    bool allowUserConfig;

    // List of framework systems (e.g. "settings", "logging", "ui", "localization".... ) that the user can configure via the UI for this plugin.
    // Note: The "keybinds" system is always user-configurable and does not need to be listed here.
    char userConfigurableSystems[SPF_MANIFEST_MAX_SYSTEMS][SPF_MANIFEST_MAX_STRING_LENGTH];
    unsigned int userConfigurableSystemsCount;

    // List of functional hooks required for the plugin to work.
    // The framework guarantees that the hooks listed here will be enabled regardless of the settings
    char requiredHooks[SPF_MANIFEST_MAX_HOOKS][SPF_MANIFEST_MAX_STRING_LENGTH];
    unsigned int requiredHooksCount;
} SPF_ConfigPolicyData_C;

/**
 * @struct SPF_LoggingData_C
 * @brief Default logging settings for the plugin.
 */
typedef struct SPF_LoggingSinks_C {
    // If `true`, plugin logs will be written separately to a file in the MyPlagin/logs folder. If false, plugin logs can be found in the framework log file framework.log
    bool file;
} SPF_LoggingSinks_C;

typedef struct SPF_LoggingData_C {
    // Default log level. e.g., "trace", "debug", "info", "warn", "error", "critical". 
    // Logging level, for the sink file, the user interface has its own filter
    char level[SPF_MANIFEST_MAX_STRING_LENGTH];
    SPF_LoggingSinks_C sinks;
} SPF_LoggingData_C;

/**
 * @struct SPF_LocalizationData_C
 * @brief Default localization settings.
 */
typedef struct SPF_LocalizationData_C {
    // The default language for the plugin's strings (e.g., "en").
    char language[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_LocalizationData_C;

/**
 * @struct SPF_KeybindsData_C
 * @brief Defines the default keybinds for actions provided by the plugin.
 */
typedef struct SPF_KeybindDefinition_C {
    // The input device type. Valid values: "keyboard", "gamepad".
    char type[SPF_MANIFEST_MAX_STRING_LENGTH];

    // The specific key or button name.
    // For a list of valid keyboard keys, see `VirtualKeyMapping.cpp`.
    // For a list of valid gamepad buttons, see `GamepadButtonMapping.cpp`.
    char key[SPF_MANIFEST_MAX_STRING_LENGTH];

    // The type of press required. Valid values: "short", "long".
    char pressType[SPF_MANIFEST_MAX_STRING_LENGTH];

    // For "long" pressType, the time in milliseconds to hold the key.
    int pressThresholdMs;

    // When to consume the input, preventing the game from seeing it.
    // Valid values: "never", "on_ui_focus", "always".
    char consume[SPF_MANIFEST_MAX_STRING_LENGTH];

    // How the key behaves. Valid values: "toggle", "hold".
    char behavior[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_KeybindDefinition_C;

typedef struct SPF_KeybindAction_C {
    /**
     * @brief The primary identifier for the action's group. Can be any string.
     * @details The framework combines this with `actionName` to form the full action name.
     *          Best Practice: "{PluginName}.{Category}" or "{PluginName}.{Category}.{Noun}".
     *          Example: "MyPlugin.UI.MainWindow"
     */
    char groupName[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief The name of the specific action, typically a verb.
     *          Example: "log_message"
     *          Example: "toggle"
     */
    char actionName[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief The array of key combinations that can trigger this action.
     * @see SPF_KeybindActionMetadata_C to provide a translatable name and description for this action.
     */
    SPF_KeybindDefinition_C definitions[SPF_MANIFEST_MAX_KEYBINDS_PER_ACTION];
    unsigned int definitionCount;
} SPF_KeybindAction_C;

typedef struct SPF_KeybindsData_C {
    // A flat list of all actions and their keybinds.
    SPF_KeybindAction_C actions[SPF_MANIFEST_MAX_ACTIONS_PER_GROUP];
    unsigned int actionCount;
} SPF_KeybindsData_C;

/**
 * @struct SPF_UIData_C
 * @brief Defines the default state for any UI windows the plugin creates.
 * @see SPF_WindowMetadata_C to provide a translatable title and description for this window in the settings panel.
 */
typedef struct SPF_WindowData_C {
    // Unique identifier for the window. e.g., "my_plugin_main_window"
    char name[SPF_MANIFEST_MAX_STRING_LENGTH];
    // Default visibility state. 
    bool isVisible;
    // Whether the user can interact with the window. Whether the window opens in "transparency" mode (all input goes to the game, we don't intercept anything)
    bool isInteractive;
    // Default position on the screen.
    int posX, posY;
    // Default size.
    int sizeW, sizeH;
    // Default collapsed state.
    bool isCollapsed;
    // If the window should auto-scroll to the bottom on new content.
    bool autoScroll;
} SPF_WindowData_C;

typedef struct SPF_UIData_C {
    SPF_WindowData_C windows[SPF_MANIFEST_MAX_WINDOWS];
    unsigned int windowsCount;
} SPF_UIData_C;


// =================================================================================================
// Manifest Metadata Blocks (C-compatible)
// =================================================================================================

/**
 * @mainpage How to Use Metadata
 * 
 * @section metadata_what 1. What is Metadata?
 * The `...Metadata` arrays (e.g., `customSettingsMetadata`, `keybindsMetadata`, `loggingMetadata`) are used to provide human-readable names and descriptions for your settings, actions, and windows. This information is primarily used to build the user interface, such as the "Settings" window. By providing metadata, you allow the framework to display user-friendly labels and tooltips instead of raw technical keys.
 *
 * @section metadata_usage 2. Using `titleKey` and `descriptionKey` (Two Methods)
 * The `titleKey` and `descriptionKey` fields are flexible and can be used in two ways, depending on whether your plugin uses localization.
 *
 * @subsection metadata_usage_loc Method A: With Localization (Recommended)
 * Provide a translation key that corresponds to an entry in your plugin's localization files (e.g., `MyPlugin/localization/en.json`).
 * @code{.cpp}
 * // In manifest:
 * strncpy_s(out_manifest.customSettingsMetadata[0].titleKey, "settings.some_number.title", ...);
 * 
 * // In MyPlugin/localization/en.json:
 * {
 *     "settings": {
 *       "some_number": {
 *         "title": "My Awesome Number"
 *       }
 *     }
 * }
 * @endcode
 * The framework will automatically find and display "My Awesome Number".
 *
 * @subsection metadata_usage_lit Method B: Without Localization (Literal Text)
 * If your plugin does not have localization files, or for a very simple case, you can put the desired text directly into the `titleKey` and `descriptionKey` fields.
 * @code{.cpp}
 * // In manifest:
 * strncpy_s(out_manifest.customSettingsMetadata[0].titleKey, "My Awesome Number", ...);
 * strncpy_s(out_manifest.customSettingsMetadata[0].descriptionKey, "This is the description for my number.", ...);
 * @endcode
 * If the framework cannot find the string "My Awesome Number" as a key in any translation file, it will log a one-time warning and then fall back to using the string itself as the literal text to display. This allows for easy, non-localized UI text.
 */

/**
 * @struct SPF_CustomSettingMetadata_C
 * @brief Defines display metadata for a custom setting declared in `settingsJson`.
 * @details It is **strongly recommended** to provide metadata for all custom settings to ensure a user-friendly UI.
 *          See the "How to Use Metadata" guide at the top of this section for usage details.
 */
typedef struct SPF_CustomSettingMetadata_C {
    /**
     * @brief The key path of the setting this metadata applies to (e.g., "some_number", "feature_flags.alpha").
     */
    char keyPath[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief A localization key or literal text for the setting's display name (title).
     */
    char titleKey[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief A localization key or literal text for the setting's detailed description (tooltip).
     */
    char descriptionKey[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief (Optional) If set to `true`, this setting will not be displayed in the settings UI.
     * @details This is useful for storing internal plugin state that should persist in `settings.json`
     *          but should not be exposed to the user. Defaults to `false` (visible) if not specified,
     *          as C-structs are typically zero-initialized.
     */
    bool hide_in_ui;

    // ---------------------------------------------------------------------------------------------
    // Optional UI Rendering Hints
    // ---------------------------------------------------------------------------------------------
    // The following fields are optional and allow you to control how a setting is displayed in the UI.
    // If 'widget' is left empty, the framework will choose a default widget based on the setting's data type.
    // ---------------------------------------------------------------------------------------------

    /**
     * @brief (Optional) The type of UI widget to use for this setting.
     *
     * If this is empty or not specified, a default widget will be chosen based on the data type
     * (e.g., a checkbox for booleans, a simple input for numbers/strings).
     *
     * Valid Widget Types:
     * - "input":      Default text/number input. (ImGui::InputText, InputInt, InputFloat)
     * - "slider":     A slider for numbers. (ImGui::SliderFloat, SliderInt)
     * - "drag":       A draggable field for numbers. (ImGui::DragFloat, DragInt)
     * - "combo":      A dropdown/combobox for selecting one option.
     * - "radio":      A group of radio buttons for selecting one option.
     * - "color3":     A color picker for an RGB color. The setting value should be a JSON array of 3 floats (e.g., [1.0, 0.5, 0.2]).
     * - "color4":     A color picker for an RGBA color. The setting value should be a JSON array of 4 floats (e.g., [1.0, 0.5, 0.2, 1.0]).
     * - "multiline":  A multi-line text input field.
     */
    char widget[64];

    /**
     * @brief (Optional) A union of parameters specific to the chosen widget type.
     *
     * Based on the value of `widget`, you should fill in the corresponding struct within this union.
     * For example, if `widget` is "slider", you must initialize the `slider` struct.
     * It is crucial to only initialize the member that matches the widget type.
     */
    union {
        /**
         * @brief Parameters for "slider" widget.
         */
        struct {
            float min_val;      ///< The minimum value of the slider.
            float max_val;      ///< The maximum value of the slider.
            char format[32];    ///< The display format (e.g., "%.2f", "%d C"). If empty, a default is used.
        } slider;

        /**
         * @brief Parameters for "drag" widget.
         */
        struct {
            float speed;        ///< The speed/increment when dragging the value.
            float min_val;      ///< The minimum value.
            float max_val;      ///< The maximum value.
            char format[32];    ///< The display format (e.g., "%.2f", "%d C"). If empty, a default is used.
        } drag;

        /**
         * @brief Parameters for "combo" and "radio" widgets.
         */
        struct {
            /**
             * @brief A JSON string representing the available options.
             *
             * The JSON must be an array of objects, where each object has a "value" and a "labelKey".
             * - "value": The actual string or number to be stored in the settings when this option is chosen.
             * - "labelKey": A localization key or literal text for the option's display name.
             *
             * @b Example:
             * @code
             * R"json([
             *   { "value": "option_a", "labelKey": "options.a.title" },
             *   { "value": "option_b", "labelKey": "Use This Text Directly" },
             *   { "value": 10, "labelKey": "options.c.title" }
             * ])json"
             * @endcode
             */
            char options_json[1024];
        } choice;

        /**
         * @brief Parameters for "color3" and "color4" widgets.
         */
        struct {
            /**
             * @brief (Advanced) A bitmask of ImGuiColorEditFlags to customize the color picker's behavior.
             * See imgui.h for available flags (e.g., ImGuiColorEditFlags_NoInputs, ImGuiColorEditFlags_AlphaBar).
             * Set to 0 for default behavior.
             */
            int flags;
        } color;

        /**
         * @brief Parameters for "multiline" widget.
         */
        struct {
            /**
             * @brief The height of the text box, specified in number of text lines.
             */
            int height_in_lines;
        } multiline;
    } widget_params;
} SPF_CustomSettingMetadata_C;

/**
 * @struct SPF_KeybindActionMetadata_C
 * @brief Defines display metadata for a keybind action.
 * @details It is **strongly recommended** to provide metadata for all keybind actions to explain what they do.
 *          See the "How to Use Metadata" guide at the top of this section for usage details.
 */
typedef struct SPF_KeybindActionMetadata_C {
    /**
     * @brief The group name of the action (must match `groupName` in `SPF_KeybindAction_C`).
     */
    char groupName[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief The action name (must match `actionName` in `SPF_KeybindAction_C`).
     */
    char actionName[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief A localization key or literal text for the action's display name.
     */
    char titleKey[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief A localization key or literal text for the action's detailed description (tooltip).
     */
    char descriptionKey[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_KeybindActionMetadata_C;

/**
 * @struct SPF_StandardSettingMetadata_C
 * @brief Defines override metadata for a standard framework system setting (e.g., logging level).
 * @details You are **not required** to provide this metadata. If omitted, the framework automatically uses its own default titles and descriptions.
 *          Only provide this if you want to override the default text or add a new language translation.
 *          See the "How to Use Metadata" guide at the top of this section for usage details.
 */
typedef struct SPF_StandardSettingMetadata_C {
    /**
     * @brief The key of the setting this metadata applies to (e.g., "level", "language").
     */
    char key[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief Your custom localization key or literal text for the setting's display name.
     */
    char titleKey[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief Your custom localization key or literal text for the setting's detailed description.
     */
    char descriptionKey[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_StandardSettingMetadata_C;

/**
 * @struct SPF_WindowMetadata_C
 * @brief Defines override metadata for a UI window or its properties.
 * @details You are **not required** to provide this metadata. If omitted, the framework automatically uses its own default titles and descriptions.
 *          Only provide this if you want to override the default text.
 *          See the "How to Use Metadata" guide at the top of this section for usage details.
 */
typedef struct SPF_WindowMetadata_C {
    /**
     * @brief The name of the window or window property this metadata applies to.
     * @example "my_plugin_main_window" (for a window group)
     * @example "is_visible" (for a generic window property)
     */
    char windowName[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief Your custom localization key or literal text for the item's title in the settings panel.
     */
    char titleKey[SPF_MANIFEST_MAX_STRING_LENGTH];

    /**
     * @brief Your custom localization key or literal text for the item's description (tooltip).
     */
    char descriptionKey[SPF_MANIFEST_MAX_STRING_LENGTH];
} SPF_WindowMetadata_C;


/**
 * @struct SPF_ManifestData_C
 * @brief The top-level structure that aggregates all manifest information.
 * A plugin must populate this structure and pass it to the framework.
 */
typedef struct SPF_ManifestData_C {
    // --- Standard Metadata & Policies ---
    SPF_InfoData_C info;
    SPF_ConfigPolicyData_C configPolicy;

    // --- Custom Plugin Settings ---
    /**
     * @brief A JSON string literal containing default values for the plugin's custom settings.
     *
     * @details This is where a plugin defines its own configuration values that are not part of the
     *          standard framework systems (like logging, ui, etc.).
     *
     * @section Structure and Merging
     * If `configPolicy.allowUserConfig` is true, the framework generates a single `settings.json`
     * file for the plugin (e.g., in `plugins/MyPlugin/config/settings.json`).
     *
     * The JSON object you provide here will be inserted into that main `settings.json` file
     * under a top-level key named `"settings"`.
     *
     * @b Example:
     * If you define your `settingsJson` in the manifest like this:
     * @code{.cpp}
      * out_manifest.settingsJson = R"json(
     *     {
     *         "some_number": 42,
     *         "some_bool": false,
     *         "feature_flags": {
     *             "alpha": true,
     *             "beta": false
     *         }
     *     }
     * )json";
     * @endcode
     *
     * The resulting `settings.json` will contain a section like this:
     * @code{.json}
     * {
      *     "settings": {
     *         "some_number": 42,
     *         "some_bool": false,
     *         "feature_flags": {
     *             "alpha": true,
     *             "beta": false
     *         }
     *     }
     * }
     * @endcode
     *
     * To access `some_number` programmatically via the `SPF_Config_API`, you would use the
     * key `"settings.some_number"`.
     *
     * @section Localization
     * To provide translatable display names and descriptions for these settings, add corresponding
     * entries to the `customSettingsMetadata` array. If no metadata is provided for a setting,
     * the framework will use the setting's raw key as its display name.
     *
     * @code{.cpp}
     * // 1. Define the data in settingsJson
     * out_manifest.settingsJson = R"json({ "some_number": 42 })json";
     *
     * // 2. Provide metadata for localization
     * out_manifest.customSettingsMetadataCount = 1;
     * strncpy_s(out_manifest.customSettingsMetadata[0].keyPath, "some_number", ...);
     * strncpy_s(out_manifest.customSettingsMetadata[0].titleKey, "setting.some_number.title", ...);
     * @endcode
     */
    const char* settingsJson;

    // --- Default Settings for Framework Systems ---
    SPF_LoggingData_C logging;
    SPF_LocalizationData_C localization;
    SPF_KeybindsData_C keybinds;
    SPF_UIData_C ui;

    // --- METADATA FOR LOCALIZATION ---
    /**
     * @brief Optional arrays for providing translatable display names and descriptions for your settings, actions, and windows.
     *
     * @details This system separates the core definition of a setting/action from its presentation metadata.
     *          This keeps the primary definitions clean while allowing for rich, localized UI.
     *
     *          For each item you want to add a translated name/description to, you add an entry to the
     *          corresponding metadata array below (e.g., `customSettingsMetadata`, `keybindsMetadata`).
     *
     *          If you do not provide metadata for an item, the framework will fall back to using its
     *          raw key name as a label in the UI and will not show a description. This ensures that
     *          providing metadata is purely optional and additive.
     */
    SPF_CustomSettingMetadata_C customSettingsMetadata[128];
    unsigned int customSettingsMetadataCount;

    SPF_KeybindActionMetadata_C keybindsMetadata[128];
    unsigned int keybindsMetadataCount;

    SPF_StandardSettingMetadata_C loggingMetadata[16];
    unsigned int loggingMetadataCount;

    SPF_StandardSettingMetadata_C localizationMetadata[16];
    unsigned int localizationMetadataCount;

    SPF_WindowMetadata_C uiMetadata[SPF_MANIFEST_MAX_WINDOWS];
    unsigned int uiMetadataCount;
} SPF_ManifestData_C;

// =================================================================================================
// Plugin Exported API
// =================================================================================================

/**
 * @brief A function pointer that the plugin implements to fill the manifest data.
 * @param out_manifest A reference to the C-struct that the plugin must populate.
 */
typedef void (*SPF_GetManifestData_t)(SPF_ManifestData_C& out_manifest);

/**
 * @struct SPF_Manifest_API
 * @brief A struct containing function pointers related to the manifest.
 * The framework asks the plugin to fill this struct.
 */
typedef struct SPF_Manifest_API {
    SPF_GetManifestData_t GetManifestData;
} SPF_Manifest_API;

/**
 * @brief The main exported function the framework looks for to get the manifest API.
 *
 * A plugin MUST export a function named `SPF_GetManifestAPI` with this signature.
 * The framework calls it to get a pointer to the plugin's `GetManifestData` function.
 *
 * @section Implementation Note for C++ Plugins
 * When implementing this function in your `.cpp` file, you MUST wrap it in
 * `extern "C" { ... }` to prevent C++ name mangling. For example:
 * @code{.cpp}
 * extern "C" {
 *     SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
 *         // ... your implementation ...
 *     }
 * }
 * @endcode
 *
 * @param out_api A pointer to a structure that the plugin must fill with its
 *                `GetManifestData` function pointer.
 * @return `true` on success, `false` on failure.
 */
typedef bool (*SPF_GetManifestAPI_t)(SPF_Manifest_API* out_api);


#ifdef __cplusplus
}
#endif