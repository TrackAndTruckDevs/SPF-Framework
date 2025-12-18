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
#include "SPF_Hooks_API.h"

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// 0. FORWARD DECLARATIONS
// =================================================================================================
// All framework structures are forward-declared here. This allows pointers to these
// types to be used throughout the API headers (e.g., in function signatures and
// struct definitions) without needing to include their full definitions, which
// prevents circular dependency issues and resolves compilation order problems.

// --- Main API Structs (Passed to plugin lifecycle functions) ---
typedef struct SPF_Core_API SPF_Core_API;
typedef struct SPF_Load_API SPF_Load_API;
typedef struct SPF_UI_API SPF_UI_API;

// --- Opaque Handles (Pointers to internal framework objects) ---
// The plugin interacts with these handles without knowing their internal structure,
// ensuring ABI stability and separation of concerns.
typedef struct SPF_Logger_Handle SPF_Logger_Handle;
typedef struct SPF_Localization_Handle SPF_Localization_Handle;
typedef struct SPF_Config_Handle SPF_Config_Handle;
typedef struct SPF_KeyBinds_Handle SPF_KeyBinds_Handle;
typedef struct SPF_Telemetry_Handle SPF_Telemetry_Handle;

// --- Sub-System API Structs (Accessed via SPF_Load_API or SPF_Core_API) ---
typedef struct SPF_Logger_API SPF_Logger_API;
typedef struct SPF_Localization_API SPF_Localization_API;
typedef struct SPF_Config_API SPF_Config_API;
typedef struct SPF_KeyBinds_API SPF_KeyBinds_API;
typedef struct SPF_Telemetry_API SPF_Telemetry_API;
typedef struct SPF_Input_API SPF_Input_API;
typedef struct SPF_Camera_API SPF_Camera_API;
typedef struct SPF_GameConsole_API SPF_GameConsole_API;
typedef struct SPF_Formatting_API SPF_Formatting_API;
typedef struct SPF_GameLog_API SPF_GameLog_API;
typedef struct SPF_JsonReader_API SPF_JsonReader_API;


// =================================================================================================
// 1. PLUGIN EXPORTS (FUNCTIONS PROVIDED BY THE PLUGIN TO THE CORE)
// =================================================================================================

/**
 * @page PluginLifecycle Plugin Lifecycle & API Availability
 * @brief Explains the stages of plugin initialization and API access.
 *
 * The framework initializes plugins in stages. The API is passed to each
 * lifecycle function, but its services become available progressively.
 * Attempting to use a service before it is available will result in a runtime error.
 *
 * @section InitializationSequence Initialization Sequence:
 * 1.  @ref OnLoad()
 * 2.  @ref OnActivated()
 * 3.  @ref OnRegisterUI()
 *
 * @subsection OnLoadDetails 1. OnLoad()
 * -   **When:** Immediately after the plugin DLL is loaded.
 * -   **API State:** Only CORE services are available in the `load_api` struct:
 *     -   `logger`, `config`, `localization`, `formatting`
 * -   **Purpose:** Essential setup using core services. The plugin should store
 *     any required handles (e.g., from `logger->GetHandle()`) for later use.
 *
 * @subsection OnActivatedDetails 2. OnActivated()
 * -   **When:** After the framework has processed the manifest and activated the plugin.
 * -   **API State:** ALL services in the `core_api` struct are now available.
 * -   **Purpose:** Main initialization. The plugin MUST store the `core_api` pointer.
 *     Registering callbacks for keybinds, hooks, and other interactive services
 *     MUST be done here.
 *
 * @subsection OnRegisterUIDetails 3. OnRegisterUI()
 * -   **When:** When the UI system is ready.
 * -   **API State:** All services are available via the previously stored `core_api` pointer.
 * -   **Purpose:** UI-specific setup.
 */



/**
 * @struct SPF_Plugin_Exports
 * @brief Contains function pointers to the plugin's core lifecycle entry points and callbacks.
 *
 * @details A plugin MUST implement the mandatory functions (`OnLoad`, `OnUnload`, `OnActivated`)
 *          and MAY implement the optional ones (`OnUpdate`, `OnRegisterUI`, `OnSettingChanged`,
 *          `OnGameWorldReady`). The plugin fills this structure, and the framework uses these
 *          pointers to communicate with the plugin at various stages of its operation.
 */
typedef struct {
  /**
   * @brief (Mandatory) Called once when the plugin DLL is loaded into memory.
   *
   * @details This is the first function called by the framework after the plugin library
   *          is successfully loaded. Use it for essential, early-stage setup that does not
   *          depend on the game being fully active.
   *
   * @param load_api A pointer to the `SPF_Load_API` structure, which provides access
   *                 to essential core services (Logger, Config, Localization, Formatting)
   *                 that are guaranteed to be available at this early stage.
   */
  void (*OnLoad)(const SPF_Load_API* load_api);

  /**
   * @brief (Mandatory) Called once just before the plugin is unloaded from memory.
   *
   * @details This is the last function called by the framework before the plugin DLL is
   *          unloaded. Use it to free all acquired resources, save any pending data,
   *          and perform a clean shutdown. All API pointers (core_api, load_api)
   *          should be considered invalid after this function returns.
   */
  void (*OnUnload)();

  /**
   * @brief (Optional) Called every frame while the plugin is active.
   *
   * @details This function is tied to the game's rendering loop. Avoid performing
   *          heavy or blocking operations here, as it will directly impact game performance.
   *          It is suitable for polling data, updating animations, or other logic that needs
   *          to run continuously. If the plugin does not require per-frame updates,
   *          this pointer should be set to `NULL`.
   */
  void (*OnUpdate)();

  /**
   * @brief (Optional) Called once to register UI components and windows.
   *
   * @details This function is called by the framework when the UI system is ready.
   *          If the plugin adds its own custom windows to the settings menu or
   *          any other UI elements, it should implement this function to register
   *          them with the provided UI API.
   *
   * @param ui_api A pointer to the `SPF_UI_API`, which provides functions for
   *                 registering draw callbacks and interacting with the UI system.
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
   * @param config_handle The configuration context handle for the plugin, the same
   *                      handle returned by `SPF_Config_API.GetContext()`.
   * @param keyPath The full path to the setting that changed (e.g., "settings.some_bool").
   */
  void (*OnSettingChanged)(SPF_Config_Handle* config_handle, const char* keyPath);

  /**
   * @brief (Mandatory) Called after the plugin is fully loaded and activated.
   *
   * @details This function is called by the framework after `OnLoad` has completed
   *          for all plugins and the framework has fully processed the plugin's manifest.
   *          At this point, the game is running, and ALL framework services are available
   *          via the `core_api`. The plugin MUST store the `core_api` pointer passed here
   *          for later use. Registering callbacks for keybinds, hooks, telemetry, and
   *          other interactive services SHOULD be done in this function.
   *
   * @param core_api A pointer to the full `SPF_Core_API` structure, which provides
   *                 access to all framework subsystems.
   */
  void (*OnActivated)(const SPF_Core_API* core_api);

  /**
   * @brief (Optional) Called once after the game world has been fully loaded.
   *
   * @details This function is called by the framework after the game world is
   *          fully loaded and all in-game objects are available. It is the ideal
   *          place to initialize logic that depends on these in-game objects
   *          (e.g., installing game-specific hooks, reading vehicle data, etc.).
   */
  void (*OnGameWorldReady)();

} SPF_Plugin_Exports;

// =================================================================================================
// 2. API STRUCTURES (PROVIDED BY THE CORE TO THE PLUGIN)
// =================================================================================================



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

  /**
   * @brief (Advanced) JSON Reader API. Provides functions to safely parse raw JSON
   *          data structures received from the framework.
   * @details This API is essential for working with complex JSON values, such as
   *          objects or arrays, that cannot be directly retrieved using the simple
   *          getters in the `SPF_Config_API`. It should be used in conjunction with
   *          `SPF_Config_API.GetJsonValueHandle` to parse configuration values, or
   *          for any future event payloads provided in JSON format.
   *          The plugin retrieves a handle to the JSON value (e.g., `SPF_JsonValue_Handle*`)
   *          and then uses the functions within this API (e.g., `GetType`, `GetString`)
   *          to navigate and extract information from that JSON structure.
   */
  SPF_JsonReader_API* json_reader;
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
