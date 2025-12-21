#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp> // Required for arbitrary settings
#include <optional>

SPF_NS_BEGIN
namespace Config {

// --- Info Block ---
/**
 * @brief Contains general information about the plugin/framework.
 */
struct InfoData {
    std::optional<std::string> name;         ///< @brief Display name.
    std::optional<std::string> version;      ///< @brief Version string (e.g., "1.0.0").
    std::optional<std::string> author;       ///< @brief Author's name.
    std::optional<std::string> descriptionKey; ///< @brief Localization key for description.
    std::optional<std::string> descriptionLiteral; ///< @brief Literal description if no localization key is provided.
    std::optional<std::string> email;        ///< @brief Author's contact email.
    std::optional<std::string> discordUrl;   ///< @brief URL to a Discord server invite.  
    std::optional<std::string> steamProfileUrl;    ///< @brief URL to a Steam profile.      
    std::optional<std::string> githubUrl;    ///< @brief URL to the GitHub repository.
    std::optional<std::string> youtubeUrl;   ///< @brief URL to a YouTube channel/video.
    std::optional<std::string> scsForumUrl;  ///< @brief URL to an SCS Software forum thread.
    std::optional<std::string> patreonUrl;   ///< @brief URL to a Patreon page.   
    std::optional<std::string> websiteUrl;   ///< @brief URL to a personal or project website.
};

// --- Config Policy Block ---
/**
 * @brief Defines policies for how configuration should be handled.
 */
struct ConfigPolicyData {
    std::optional<bool> allowUserConfig;        ///< @brief True if users can modify this component's settings via UI.
    std::vector<std::string> userConfigurableSystems; ///< @brief List of system names (e.g., "keybinds", "settings") that users can configure.
    std::vector<std::string> requiredHooks;     ///< @brief List of hooks required by this component for its features to work.
};

// --- Settings Block (Arbitrary JSON content for settings.config.settings) ---
// This represents the content of the "settings": { "config": { "settings": { ... } } } block
// The user clarified that this should be arbitrary JSON for both framework and plugins.
// So, the top-level ManifestData will have a nlohmann::json member for this.

// --- Logging Settings Block ---
/**
 * @brief Defines default logging settings for a component.
 */
struct LoggingData {
    std::optional<std::string> level; ///< @brief Default logging level (e.g., "info", "warn", "debug").

    /**
     * @brief Defines which logging sinks are enabled by default.
     */
    struct Sinks {
        std::optional<bool> file; ///< @brief True to enable the file sink.
        std::optional<bool> ui;   ///< @brief True to enable the UI logger window sink.
    } sinks;
};

// --- Localization Settings Block ---
/**
 * @brief Defines default localization settings for a component.
 */
struct LocalizationData {
    std::optional<std::string> language; ///< @brief Default language for the component (e.g., "en_US").
};

// --- Keybinds Settings Block ---
/**
 * @brief Defines a single keybinding for an action.
 */
struct KeybindDefinition {
    std::optional<std::string> type;    ///< @brief Type of input device (e.g., "keyboard", "gamepad").
    std::optional<std::string> key;     ///< @brief The specific key or button name (e.g., "v", "dpad_up").
    std::optional<std::string> pressType; ///< @brief The type of press required (e.g., "short", "long").
    std::optional<int> pressThresholdMs; ///< @brief Time in milliseconds to qualify as a long press.
    std::optional<std::string> consume; ///< @brief Input consumption policy (e.g., "never", "on_ui_focus").
    std::optional<std::string> behavior; ///< @brief Reserved for future use (e.g., for toggle/hold behavior).
};

/**
 * @brief Container for all keybind action definitions for a component.
 */
struct KeybindsData {
    /// @brief Map of action groups to actions to their binding definitions.
    /// Format: { "groupName": { "actionName": [ ...bindings... ] } }
    std::map<std::string, std::map<std::string, std::vector<KeybindDefinition>>> actions;
};

// --- UI Settings Block ---
/**
 * @brief Defines the default state and properties of a UI window.
 */
struct WindowData {
    std::optional<bool> isVisible;      ///< @brief Default visibility state.
    std::optional<bool> isInteractive;  ///< @brief Whether the window can receive user input.
    std::optional<int> posX;            ///< @brief Default X position.
    std::optional<int> posY;            ///< @brief Default Y position.
    std::optional<int> sizeW;           ///< @brief Default width.
    std::optional<int> sizeH;           ///< @brief Default height.
    std::optional<bool> isCollapsed;    ///< @brief Default collapsed state.
    std::optional<bool> isDocked;       ///< @brief Default docked state.
    std::optional<int> dockPriority;    ///< @brief Priority for ordering within a dock space.
    std::optional<bool> allowUndocking; ///< @brief Whether the user is allowed to undock the window.
    std::optional<bool> autoScroll;     ///< @brief Whether the window content should auto-scroll by default.
};

/**
 * @brief Container for all UI window definitions for a component.
 */
struct UIData {
    /// @brief Map of window names to their default state definitions.
    /// Format: { "windowName": { ...window_data... } }
    std::map<std::string, WindowData> windows;
};

// --- Metadata Structs ---

/**
 * @brief Metadata for a user-configurable custom setting.
 */
struct CustomSettingMetadata {
    std::string keyPath; ///< @brief Full JSON path to the setting.
    std::optional<std::string> titleKey; ///< @brief Localization key for the setting's title.
    std::optional<std::string> descriptionKey; ///< @brief Localization key for the setting's description.
    bool hide_in_ui = false; ///< @brief If true, this setting will not be displayed in the UI. Defaults to false (visible).

    // ---------------------------------------------------------------------------------------------
    // Optional UI Rendering Hints (parsed from SPF_CustomSettingMetadata_C)
    // These fields allow controlling how a setting is displayed in the UI.
    // If 'widget' is empty, a default widget will be chosen based on the setting's data type.
    // ---------------------------------------------------------------------------------------------
    std::optional<std::string> widget; ///< @brief The type of UI widget to use (e.g., "slider", "drag", "color3").
    /**
     * @brief Parameters specific to the chosen widget type.
     * This JSON object will contain key-value pairs like "min": 0.0, "max": 100.0, "format": "%.2f",
     * or "options": [...] for combo/radio widgets.
     */
    nlohmann::json widget_params;
};

/**
 * @brief Metadata for a user-configurable keybind action.
 */
struct KeybindActionMetadata {
    std::string groupName; ///< @brief The group this action belongs to.
    std::string actionName; ///< @brief The name of the action.
    std::optional<std::string> titleKey; ///< @brief Localization key for the action's title.
    std::optional<std::string> descriptionKey; ///< @brief Localization key for the action's description.
};

/**
 * @brief Metadata for a user-configurable standard setting (e.g., logging level).
 */
struct StandardSettingMetadata {
    std::string key; ///< @brief The key for the setting.
    std::optional<std::string> titleKey; ///< @brief Localization key for the setting's title.
    std::optional<std::string> descriptionKey; ///< @brief Localization key for the setting's description.
};

/**
 * @brief Metadata for a user-configurable UI window.
 */
struct WindowMetadata {
    std::string windowName; ///< @brief The name of the window.
    std::optional<std::string> titleKey; ///< @brief Localization key for the window's title.
    std::optional<std::string> descriptionKey; ///< @brief Localization key for the window's description.
};


// --- Top-level Manifest Data ---
/**
 * @brief The top-level structure representing a complete plugin or framework manifest.
 *
 * This combines various configuration and metadata blocks into a single cohesive structure.
 */
struct ManifestData {
    InfoData info;                  ///< @brief General information block.
    ConfigPolicyData configPolicy;  ///< @brief Configuration policy block.
    // The entire "settings" block, including "config" and nested "settings", as arbitrary JSON
    nlohmann::json settings;

    LoggingData logging;            ///< @brief Default logging configuration.
    LocalizationData localization;  ///< @brief Default localization configuration.
    KeybindsData keybinds;          ///< @brief Default keybind definitions.
    UIData ui;                      ///< @brief Default UI element configurations.

    // --- Metadata ---
    std::vector<CustomSettingMetadata> customSettingsMetadata; ///< @brief Metadata for custom settings defined in the 'settings' block.
    std::vector<KeybindActionMetadata> keybindsMetadata;     ///< @brief Metadata for keybind actions.
    std::vector<StandardSettingMetadata> loggingMetadata;    ///< @brief Metadata for standard logging settings.
    std::vector<StandardSettingMetadata> localizationMetadata; ///< @brief Metadata for standard localization settings.
    std::vector<WindowMetadata> uiMetadata;                 ///< @brief Metadata for UI windows.
};

} // namespace Config
SPF_NS_END
