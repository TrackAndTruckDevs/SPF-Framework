#pragma once

/**
 * @file SPF_Plugin.h
 * @brief The single, mandatory header file for any SPF plugin.
 *
 * This file defines the stable C-style ABI (Application Binary Interface)
 * that all plugins must use to communicate with the SPF core. It ensures
 * that plugins remain compatible even if the core is compiled with a
 * different compiler version.
 *
 * To create a plugin, you must:
 * 1. Include this header file.
 * 2. Implement the functions defined in SPF_Plugin_Exports.
 * 3. Export a single function, SPF_GetPlugin, which fills the provided
 *    SPF_Plugin_Exports structure with pointers to your implementations.
 * 4. (Recommended) Export the SPF_GetManifestAPI function to provide
 *    plugin metadata to the core before it is fully loaded.
 */

#include <stdbool.h>

// Include dependent C-API definitions
#include "SPF_JsonReader_API.h"
#include "SPF_Hooks_API.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// 1. PLUGIN LIFECYCLE & API AVAILABILITY
// =================================================================================================

/*
 * @section Plugin Lifecycle & API Availability
 * The framework initializes the plugin in stages. The API is passed to each
 * lifecycle function, but its services become available progressively. Attempting
 * to use a service before it is available is a compile-time error.
 *
 * Initialization Sequence:
 * 1. OnLoad()
 * 2. OnActivated()
 * 3. OnRegisterUI()
 *
 * ---
 *
 * 1. void (*OnLoad)(const SPF_Load_API* load_api);
 *    - **When:** Immediately after the DLL is loaded.
 *    - **API State:** Only CORE services are available in the `load_api` struct:
 *      - `logger`, `config`, `localization`, `formatting`
 *    - **Purpose:** Essential setup using core services. The plugin should store
 *      any required handles (e.g., from `logger->GetHandle()`) for later use.
 *
 * 2. void (*OnActivated)(const SPF_Core_API* core_api);
 *    - **When:** After the framework has processed the manifest and activated the plugin.
 *    - **API State:** ALL services in the `core_api` struct are now available.
 *    - **Purpose:** Main initialization. The plugin MUST store the `core_api` pointer.
 *      Registering callbacks for keybinds, hooks, and other interactive services
 *      MUST be done here.
 *
 * 3. void (*OnRegisterUI)(...);
 *    - **When:** When the UI system is ready.
 *    - **API State:** All services are available via the previously stored `core_api` pointer.
 *    - **Purpose:** UI-specific setup.
 */

// =================================================================================================
// 2. STRUCTURES PROVIDED BY THE PLUGIN TO THE CORE
// =================================================================================================

// Forward-declare the main core API struct
typedef struct SPF_Core_API SPF_Core_API;
typedef struct SPF_Load_API SPF_Load_API;

/**
 * @struct SPF_Plugin_Exports
 * @brief Contains function pointers to the plugin's lifecycle entry points.
 *
 * The plugin must fill this structure to tell the core which functions to call
 * at different stages of its operation.
 */
typedef struct {
  /**
   * @brief Called once when the plugin is loaded.
   *
   * This is the first function called after the library is successfully loaded.
   * Use it for essential setup using only the core services provided.
   *
   * @param load_api A pointer to the "Load-time" API structure, which contains
   *                 only the guaranteed-available core services.
   */
  void (*OnLoad)(const SPF_Load_API* load_api);

  /**
   * @brief Called once just before the plugin is unloaded.
   *
   * Use this function to free all acquired resources, save data, and perform
   * a clean shutdown.
   */
  void (*OnUnload)();

  /**
   * @brief (Optional) Called on every frame of the game loop.
   *
   * Use for logic that needs to run continuously (e.g., updating data, animations).
   * If the plugin does not require per-frame updates, leave this pointer as NULL.
   */
  void (*OnUpdate)();

  /**
   * @brief (Optional) Called once to register UI components.
   *
   * If the plugin adds its own windows to the settings menu, it should
   * implement this function. It is called after the framework's UI is initialized.
   *
   * @param ui_api A pointer to the UI API.
   */
  void (*OnRegisterUI)(struct SPF_UI_API* ui_api);

  /**
   * @brief (Optional) Called when a setting specific to this plugin is changed
   * from an external source (e.g., the framework's settings window).
   *
   * The framework will NOT call this for system settings that it manages
   * itself (e.g., `logging`, `keybinds`, `ui`). The call will only occur
   * for custom configuration blocks defined by the plugin in its manifest.
   *
   * @param keyPath The full path to the setting that changed (e.g., "my_settings.some_bool").
   * @param value_handle An opaque handle to the new JSON value.
   * @param json_reader An API provided by the framework to safely read data
   *                    from the `value_handle`.
   */
  void (*OnSettingChanged)(const char* keyPath, const SPF_JsonValue_Handle* value_handle, const SPF_JsonReader_API* json_reader);

  /**
   * @brief (Optional) Called after the plugin is fully loaded and activated.
   *
   * Use this function for main initialization tasks. The plugin MUST store the
   * `core_api` pointer passed here for later use. Registering keybinds, hooks,
   * and other interactive services should be done in this function.
   *
   * @param core_api A pointer to the full core API structure.
   */
  void (*OnActivated)(const SPF_Core_API* core_api);

  /**
   * @brief (Optional) Called once after the game world has been loaded.
   *
   * This function is the ideal place to initialize logic that depends on
   * in-game objects being available (e.g., camera hooks, reading vehicle data).
   */
  void (*OnGameWorldReady)();

} SPF_Plugin_Exports;

// =================================================================================================
// 3. STRUCTURES PROVIDED BY THE CORE TO THE PLUGIN
// =================================================================================================

// --- Opaque Handles ---
// The plugin interacts with these without knowing their internal structure.
typedef struct SPF_Logger_Handle SPF_Logger_Handle;
typedef struct SPF_Localization_Handle SPF_Localization_Handle;
typedef struct SPF_Config_Handle SPF_Config_Handle;
typedef struct SPF_KeyBinds_Handle SPF_KeyBinds_Handle;
typedef struct SPF_Telemetry_Handle SPF_Telemetry_Handle;

// --- Forward-declarations of API structs ---
typedef struct SPF_Logger_API SPF_Logger_API;
typedef struct SPF_Localization_API SPF_Localization_API;
typedef struct SPF_Config_API SPF_Config_API;
typedef struct SPF_KeyBinds_API SPF_KeyBinds_API;
typedef struct SPF_UI_API SPF_UI_API;
typedef struct SPF_Telemetry_API SPF_Telemetry_API;
typedef struct SPF_Input_API SPF_Input_API;
typedef struct SPF_Camera_API SPF_Camera_API;
typedef struct SPF_GameConsole_API SPF_GameConsole_API;
typedef struct SPF_Formatting_API SPF_Formatting_API;
typedef struct SPF_GameLog_API SPF_GameLog_API;

/**
 * @struct SPF_Load_API
 * @brief Provides access to essential core services available at load time.
 *
 * This structure is passed to the `OnLoad` function and contains only services
 * that are guaranteed to be available immediately when the plugin is loaded.
 */
struct SPF_Load_API {
  /**
   * @brief Logging API. Allows writing messages to log files and the in-game
   * console. Each plugin gets its own logger instance.
   */
  SPF_Logger_API* logger;

  /**
   * @brief Localization API. Allows registering translation files and retrieving
   * localized strings by key.
   */
  SPF_Localization_API* localization;

  /**
   * @brief Configuration API. Allows plugins to save and load their own settings.
   */
  SPF_Config_API* config;

  /**
   * @brief Formatting API. Provides safe, cross-DLL string formatting.
   */
  SPF_Formatting_API* formatting;
};

/**
 * @struct SPF_Core_API
 * @brief The gateway to all framework functionality available to plugins.
 *
 * This structure is the main entry point to all framework subsystems.
 * A pointer to it is provided in `OnActivated`, and the plugin must save it.
 */
struct SPF_Core_API {
  /**
   * @brief Logging API. Allows writing messages to log files and the in-game
   * console. Each plugin gets its own logger instance.
   */
  SPF_Logger_API* logger;

  /**
   * @brief Localization API. Allows registering translation files and retrieving
   * localized strings by key.
   */
  SPF_Localization_API* localization;

  /**
   * @brief Configuration API. Allows plugins to save and load their own settings.
   */
  SPF_Config_API* config;

  /**
   * @brief Key Binds API. Allows registering custom actions and binding them to keys.
   */
  SPF_KeyBinds_API* keybinds;

  /**
   * @brief User Interface API. Allows registering custom windows in the settings menu.
   */
  SPF_UI_API* ui;

  /**
   * @brief Telemetry API. Provides access to game telemetry data (speed, RPM,
   * cargo status, etc.).
   */
  SPF_Telemetry_API* telemetry;

  /**
   * @brief Input API. Allows simulating input (key presses, mouse movements).
   */
  SPF_Input_API* input;

  /**
   * @brief Hooks API. Allows intercepting game functions (hooking) to modify
   * or extend game behavior.
   */
  SPF_Hooks_API* hooks;

  /**
   * @brief Camera API. For controlling the in-game camera.
   */
  SPF_Camera_API* camera;

  /**
   * @brief Game Console API. Allows interacting with the in-game console,
   * such as registering custom console commands.
   */
  SPF_GameConsole_API* console;

  /**
   * @brief Formatting API. Provides safe, cross-DLL string formatting.
   */
  SPF_Formatting_API* formatting;

  /**
   * @brief Game Log API. Allows subscribing to game log events.
   */
  SPF_GameLog_API* gamelog;
};

// =================================================================================================
// 3. THE PLUGIN'S MAIN EXPORTED FUNCTION
// =================================================================================================

/**
 * @brief The main function that the SPF core looks for in the plugin DLL upon loading.
 *
 * The plugin MUST implement this function to provide the core with pointers
 * to its lifecycle functions.
 *
 * @param[out] exports A pointer to a structure that the plugin must fill with
 *                     pointers to its functions (OnLoad, OnUnload, etc.).
 * @return `true` on success, `false` on failure (which will cause the
 *         plugin to fail to load).
 */
#if defined(_WIN32)
#define SPF_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SPF_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports);

#ifdef __cplusplus
}
#endif
