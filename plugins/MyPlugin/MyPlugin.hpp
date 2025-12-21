/**
 * @file MyPlugin.hpp
 * @brief Internal header for the MyPlugin.
 * @details This header defines the core structure and function prototypes for the plugin.
 * It serves as a template for new plugins, providing a minimal yet functional structure
 * with clear explanations for optional features.
 */
#pragma once

// =================================================================================================
// 1. SPF API Includes - Core & Essential
// =================================================================================================
// These headers are fundamental for any SPF plugin to load, declare itself, log, and format strings.
// If a type from these APIs is used in PluginContext or a function prototype, its header MUST be included here.

// SPF_Plugin.h: Defines the core plugin export structures (SPF_Plugin_Exports, SPF_Core_API)
//               and lifecycle functions. This is mandatory for any plugin.
#include <SPF/SPF_API/SPF_Plugin.h>
// SPF_Manifest_API.h: Required for defining the plugin's metadata (name, version, etc.)
//                     via the GetManifestData function, which the framework calls first.
#include <SPF/SPF_API/SPF_Manifest_API.h>
// SPF_Logger_API.h: Essential for logging messages to the framework's central logger.
//                   PluginContext uses SPF_Logger_Handle.
#include <SPF/SPF_API/SPF_Logger_API.h>
// SPF_Formatting_API.h: Provides safe, cross-DLL string formatting functions.
//                       PluginContext uses SPF_Formatting_API.
#include <SPF/SPF_API/SPF_Formatting_API.h>

// =================================================================================================
// 1.1. SPF API Includes - Optional (Commented Out)
// =================================================================================================
// Uncomment these includes ONLY if your plugin needs to use types from them in PluginContext
// or in function prototypes declared in this .hpp file.
// If an API is only used in the .cpp implementation (e.g., for a commented-out function body),

// #include <SPF/SPF_API/SPF_Config_API.h>         // For SPF_Config_Handle
// #include <SPF/SPF_API/SPF_Localization_API.h>   // For SPF_Localization_Handle
// #include <SPF/SPF_API/SPF_KeyBinds_API.h>       // For SPF_KeyBinds_Handle
// #include <SPF/SPF_API/SPF_UI_API.h>             // For SPF_UI_API, SPF_Window_Handle
// #include <SPF/SPF_API/SPF_Telemetry_API.h>      // For SPF_Telemetry_Handle
// #include <SPF/SPF_API/SPF_Hooks_API.h>          // For SPF_Hooks_API, SPF_Hook_Handle
// #include <SPF/SPF_API/SPF_GameConsole_API.h>    // For SPF_GameConsole_API
// #include <SPF/SPF_API/SPF_VirtInput_API.h>      // For SPF_VirtualDevice_Handle
// #include <SPF/SPF_API/SPF_Camera_API.h>         // For SPF_Camera_API
// #include <SPF/SPF_API/SPF_GameLog_API.h>        // For SPF_GameLog_Callback_Handle
// #include <SPF/SPF_API/SPF_JsonReader_API.h>     // For SPF_JsonValue_Handle, SPF_JsonReader_API (often with OnSettingChanged)

// =================================================================================================
// 2. Standard Library Includes
// =================================================================================================
#include <cstdint>  // For fixed-width integer types like int32_t, useful for consistent data sizes.

// It's a strong best practice to wrap all your plugin's code in a unique namespace.
// This prevents naming conflicts with the framework or other plugins that might be loaded.
namespace MyPlugin {

// =================================================================================================
// 3. Core Plugin Architecture
// =================================================================================================

// --- Plugin Context ---

/**
 * @brief Encapsulates all global state for the plugin in a single object.
 *
 * @details This struct follows the "Context Object" design pattern. All plugin-wide state
 * (API pointers, cached handles, settings, runtime flags, etc.) is consolidated into this
 * single `PluginContext` object. A single global instance of this struct (`g_ctx`) is then
 * used throughout the plugin.
 */
struct PluginContext {
  // --- Primary API Pointers (Essential) ---
  // These are the main gateways to the framework's functionality, provided during
  // the plugin's lifecycle.
  const SPF_Load_API* loadAPI = nullptr;
  const SPF_Core_API* coreAPI = nullptr;

  // --- Cached Handles (Essential) ---
  // These handles are obtained from the loadAPI and are used frequently.
  SPF_Logger_Handle* loggerHandle = nullptr;
  const SPF_Formatting_API* formattingAPI = nullptr;

  // --- Cached Handles & Pointers (Optional - Uncomment if needed) ---
  // Uncomment these members if your plugin uses the corresponding API.
  // Remember to also uncomment the relevant #include directives in this .hpp file.

  // SPF_Config_Handle* configHandle = nullptr;         // Requires: SPF_Config_API.h
  // SPF_Localization_Handle* localizationHandle = nullptr; // Requires: SPF_Localization_API.h
  // SPF_KeyBinds_Handle* keybindsHandle = nullptr;     // Requires: SPF_KeyBinds_API.h
  // SPF_UI_API* uiAPI = nullptr;                       // Requires: SPF_UI_API.h
  // SPF_Window_Handle* mainWindowHandle = nullptr;     // Requires: SPF_UI_API.h (Example for a main UI window)
  // SPF_Telemetry_Handle* telemetryHandle = nullptr;   // Requires: SPF_Telemetry_API.h
  // SPF_Hooks_API* hooksAPI = nullptr;                 // Requires: SPF_Hooks_API.h
  // SPF_GameConsole_API* gameConsoleAPI = nullptr;     // Requires: SPF_GameConsole_API.h
  // SPF_VirtualDevice_Handle* virtualDeviceHandle = nullptr; // Requires: SPF_VirtInput_API.h
  // SPF_Camera_API* cameraAPI = nullptr;               // Requires: SPF_Camera_API.h
  // SPF_GameLog_Callback_Handle gameLogCallbackHandle = nullptr; // Requires: SPF_GameLog_API.h

  // --- Telemetry Callback Handles (Optional - Uncomment if needed) ---
  // These handles manage the lifetime of telemetry subscriptions. Storing them explicitly
  // is good practice, though their lifetime is automatically tied to 'telemetryHandle'.
  // Requires: SPF_Telemetry_API.h
  // SPF_Telemetry_Callback_Handle* gameStateSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* timestampsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* commonDataSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* truckConstantsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* trailerConstantsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* truckDataSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* trailersSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* jobConstantsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* jobDataSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* navigationDataSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* controlsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* specialEventsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* gameplayEventsSubscription = nullptr;
  // SPF_Telemetry_Callback_Handle* gearboxConstantsSubscription = nullptr;

  // --- Plugin State Variables (Optional - Uncomment/Add if needed) ---
  // Add any plugin-specific state variables here.

  // int32_t someNumberSetting = 0;   // Example for a configurable setting
};

/**
 * @brief The single global instance of the plugin's context.
 * @details This is defined once in `MyPlugin.cpp` and declared `extern` here, making it
 * accessible throughout all of the plugin's source files.
 */
extern PluginContext g_ctx;

// =================================================================================================
// 4. Function Prototypes - Core Lifecycle
// =================================================================================================
// These are the mandatory functions that the framework expects the plugin to implement.

/**
 * @brief Fills the manifest structure with this plugin's metadata.
 * @details This function is called by the framework *before* the plugin is loaded to learn
 * about its name, version, and other basic properties.
 * @param[out] out_manifest A reference to the manifest structure to be filled.
 */
void GetManifestData(SPF_ManifestData_C& out_manifest);

/**
 * @brief Called first when the plugin DLL is loaded into memory.
 * @details This is the earliest point for initialization. Only the `load_api` services
 * (logger, config, localization, formatting) are available here.
 * @param load_api A pointer to the Load API.
 */
void OnLoad(const SPF_Load_API* load_api);

/**
 * @brief Called when the plugin is activated by the framework.
 * @details This function is called after `OnLoad` and after the framework has processed the
 * plugin's manifest. At this point, the game is running and all framework services are
 * available via the `core_api`.
 * @param core_api A pointer to the Core API, which contains pointers to all other APIs.
 */
void OnActivated(const SPF_Core_API* core_api);

/**
 * @brief Called every frame while the plugin is active.
 * @details This function is tied to the rendering loop. Avoid doing heavy or blocking work
 * here as it can impact game performance.
 */
void OnUpdate();

/**
 * @brief Called last, just before the plugin is unloaded from memory.
 * @details Use this function to perform all necessary cleanup.
 */
void OnUnload();

// =================================================================================================
// 4.1. Function Prototypes - Optional Callbacks (Commented Out)
// =================================================================================================
// Uncomment and implement these functions if your plugin needs to react to specific events.
// Remember to also uncomment the relevant #include directives above and add corresponding
// members to the PluginContext struct.

/**
 * @brief Called once after the game world has been fully loaded.
 * @details This function is the ideal place to initialize logic that depends on
 *          in-game objects being available (e.g., camera hooks, reading vehicle data).
 */
// void OnGameWorldReady();

/**
 * @brief Called when a setting is changed externally (e.g., via the main settings UI or by another plugin).
 * @details Uncomment this if your plugin defines custom settings in its manifest and needs to react
 *          to changes made by the user or other parts of the framework.
 *          Requires: SPF_Config_API.h, SPF_JsonReader_API.h (for parameters)
 * @param keyPath The dot-separated path of the setting that changed (e.g., "settings.some_number").
 * @param value_handle A handle to the new JSON value.
 * @param json_reader A pointer to the JSON reader API for safely extracting the value from the handle.
 */
// void OnSettingChanged(const char* keyPath, const SPF_JsonValue_Handle* value_handle, const SPF_JsonReader_API* json_reader);

/**
 * @brief Called once to allow the plugin to register its UI rendering callbacks.
 * @details Uncomment this if your plugin defines UI windows in its manifest and needs to draw content.
 *          Requires: SPF_UI_API.h (for parameter)
 * @param ui_api A pointer to the UI API.
 */
// void OnRegisterUI(SPF_UI_API* ui_api);

/**
 * @brief Renders the content of the plugin's main window.
 * @details This function is registered as a callback and is called by the UI system every
 *          frame that the window is visible. Uncomment this if your plugin has a UI window.
 *          Requires: SPF_UI_API.h (for parameter)
 * @param ui A pointer to the UI API, used to draw widgets.
 * @param user_data A pointer to user-defined data passed during registration (e.g., `this` pointer).
 */
// void RenderMainWindow(SPF_UI_API* ui, void* user_data);

/**
 * @brief Callback executed when a keybind action is triggered by the user.
 * @details Uncomment this if your plugin defines keybinds in its manifest and needs to react
 *          to user input. You would typically register this callback in `OnActivated`.
 *          Requires: SPF_KeyBinds_API.h (for registration, but callback itself is void(void))
 */
// void OnKeybindAction();

/**
 * @brief Callback for game log messages.
 * @details Uncomment this if your plugin needs to monitor the game's internal log output.
 *          You would typically register this callback in `OnActivated`.
 *          Requires: SPF_GameLog_API.h (for registration, but callback itself is const char*, void*)
 * @param log_line The content of the log line.
 * @param user_data A pointer to user-defined data passed during registration.
 */
// void OnGameLogMessage(const char* log_line, void* user_data);

// =================================================================================================
// 4.2. Function Prototypes - Optional Helper Functions (Commented Out)
// =================================================================================================
// Add prototypes for any internal helper functions your plugin might need.
// Remember to also uncomment the relevant #include directives above and add corresponding
// members to the PluginContext struct.

// void InitializeVirtualDevice(); // Example for SPF_VirtInput_API
// void InstallGameHook();         // Example for SPF_Hooks_API

// =================================================================================================
// 4.3. Function Prototypes - Telemetry Callbacks (Optional - Commented Out)
// =================================================================================================
// Uncomment and implement these callbacks if your plugin subscribes to telemetry data.
// Requires: SPF_Telemetry_API.h

// void OnGameState(const SPF_GameState* data, void* user_data);
// void OnTimestamps(const SPF_Timestamps* data, void* user_data);
// void OnCommonData(const SPF_CommonData* data, void* user_data);
// void OnTruckConstants(const SPF_TruckConstants* data, void* user_data);
// void OnTrailerConstants(const SPF_TrailerConstants* data, void* user_data);
// void OnTruckData(const SPF_TruckData* data, void* user_data);
// void OnTrailers(const SPF_Trailer* trailers, uint32_t count, void* user_data);
// void OnJobConstants(const SPF_JobConstants* data, void* user_data);
// void OnJobData(const SPF_JobData* data, void* user_data);
// void OnNavigationData(const SPF_NavigationData* data, void* user_data);
// void OnControls(const SPF_Controls* data, void* user_data);
// void OnSpecialEvents(const SPF_SpecialEvents* data, void* user_data);
// void OnGameplayEvents(const char* event_id, const SPF_GameplayEvents* data, void* user_data);
// void OnGearboxConstants(const SPF_GearboxConstants* data, void* user_data);

}  // namespace MyPlugin
