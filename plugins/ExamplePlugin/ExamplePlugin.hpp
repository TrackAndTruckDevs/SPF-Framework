/**
 * @file ExamplePlugin.hpp
 * @brief Internal header for the ExamplePlugin.
 * @details This header defines the core structure and function prototypes for the plugin.
 * It is organized to provide a clear overview of the plugin's architecture for new developers,
 * with detailed comments explaining the purpose of each component.
 */
#pragma once

// =================================================================================================
// 1. SPF API Includes
// =================================================================================================
// These headers provide the definitions for the framework APIs that the plugin interacts with.
// Including them is necessary to use any framework functionality. Each header corresponds to a
// specific framework subsystem.
#include <SPF/SPF_API/SPF_Plugin.h>            // Defines the core plugin export structures (SPF_Plugin_Exports, SPF_Core_API) and lifecycle functions. This is mandatory.
#include <SPF/SPF_API/SPF_Manifest_API.h>      // For defining the plugin's metadata (name, version, required hooks, etc.) via GetManifestData.
#include <SPF/SPF_API/SPF_Logger_API.h>        // For logging messages to the framework's central logger.
#include <SPF/SPF_API/SPF_Config_API.h>        // For reading from and writing to the plugin's dedicated settings file.
#include <SPF/SPF_API/SPF_Localization_API.h>  // For handling multi-language strings from translation files.
#include <SPF/SPF_API/SPF_KeyBinds_API.h>      // For registering custom actions and binding them to keyboard/gamepad inputs.
#include <SPF/SPF_API/SPF_UI_API.h>            // For creating and managing user interface windows and widgets.
#include <SPF/SPF_API/SPF_Telemetry_API.h>     // For reading live game data (speed, RPM, job info, etc.).
#include <SPF/SPF_API/SPF_Hooks_API.h>         // For intercepting and modifying native game functions.
#include <SPF/SPF_API/SPF_GameConsole_API.h>   // For executing commands in the in-game developer console.
#include <SPF/SPF_API/SPF_VirtInput_API.h>     // For creating virtual input devices (like a virtual gamepad) to simulate input.
#include <SPF/SPF_API/SPF_Camera_API.h>        // For interacting with and controlling the various in-game cameras.
#include <SPF/SPF_API/SPF_GameLog_API.h>       // For subscribing to the game's internal log output.
#include <SPF/SPF_API/SPF_Formatting_API.h>    // For safe, cross-DLL string formatting to prevent crashes.
#include <SPF/SPF_API/SPF_JsonReader_API.h>    // For safely reading JSON data provided by the framework in callbacks.

// =================================================================================================
// 2. Standard Library Includes
// =================================================================================================
#include <cstdint>  // For fixed-width integer types like int32_t, which are useful for consistent data sizes.

// It's a strong best practice to wrap all your plugin's code in a unique namespace.
// This prevents naming conflicts with the framework or other plugins that might be loaded.
namespace ExamplePlugin {

// =================================================================================================
// 3. Core Plugin Architecture
// =================================================================================================
// This section defines the fundamental building blocks of the plugin's architecture. A good
// architecture is key to writing a maintainable and understandable plugin.

// --- Type-definitions ---

/**
 * @brief Defines the function signature for a game's internal string formatting function.
 * @details This type alias is crucial for the Hooks API. When you hook a function, you must
 * provide a "detour" function with the exact same signature as the original, and a "trampoline"
 * function pointer of the same type to call the original function. This `using` statement
 * creates a clear, readable type that can be used for both the detour and the trampoline,
 * ensuring type safety and preventing hard-to-debug crashes.
 *
 * @param pOutput A pointer to an opaque output string buffer structure used by the game.
 * @param ppInput A pointer-to-a-pointer to a `const char*`, which holds a game-specific string
 *                (e.g., "@@quit_game@@") to be processed.
 * @return A pointer to the resulting formatted string object, also an opaque structure.
 */

using GameStringFormatting_t = void* (*)(void* pOutput, const char** ppInput);

// --- Plugin Context ---

/**
 * @brief Encapsulates all global state for the plugin in a single object.
 *
 * @details This struct follows the "Context Object" design pattern. Because the framework
 * communicates with the plugin via C-style callbacks, you cannot use member functions
 * of a class directly. Instead of using many scattered global variables (which is bad practice),
 * all plugin-wide state (API pointers, cached handles, settings, runtime flags, etc.) is
 * consolidated into this single `PluginContext` object. A single global instance of this
 * struct (`g_ctx`) is then used throughout the plugin.
 *
 * This approach is the cornerstone of this example's architecture and offers several advantages:
 * - **Organization:** Keeps related data together, making the code easier to understand and navigate.
 * - **Reduces Global Namespace Pollution:** Only one global variable (`g_ctx`) is introduced for the
 *   entire plugin's state, minimizing the risk of naming collisions.
 * - **Maintainability:** Simplifies adding, removing, or finding state variables. All state is defined here.
 */
struct PluginContext {
  // --- Primary API Pointers ---
  // These are the main gateways to the framework's functionality. They are received during the
  // plugin's lifecycle and stored here for universal access across all plugin files.

  /**
   * @brief Pointer to the Load API, received in the `OnLoad` lifecycle function.
   * @details This API is available at the earliest stage of plugin loading. It provides access
   * to essential services that do not depend on the game being fully initialized, such as
   * the logger, config system, and localization. This pointer is valid until `OnUnload` completes.
   */
  const SPF_Load_API* loadAPI = nullptr;

  /**
   * @brief Pointer to the Core API, received in the `OnActivated` lifecycle function.
   * @details This API provides access to all framework services, including those that depend on the
   * game being fully loaded (e.g., telemetry, camera, hooks). This pointer is valid from the
   * moment `OnActivated` is called until `OnUnload` completes.
   */
  const SPF_Core_API* coreAPI = nullptr;

  // --- Cached Handles & Pointers ---
  // Pointers and handles that are frequently used can be cached here for convenience and performance.
  // This avoids having to repeatedly call getter functions.

  /**
   * @brief Cached pointer to the UI API, received in `OnRegisterUI`.
   * @details Caching this avoids needing to pass it through `user_data` pointers in every
   * render callback, simplifying the render function signatures.
   */
  SPF_UI_API* uiAPI = nullptr;

  /**
   * @brief Cached handle to the plugin's main window.
   * @details This handle is retrieved from the UI API in `OnRegisterUI` and is used to
   * programmatically control the window's visibility.
   */
  SPF_Window_Handle* mainWindowHandle = nullptr;

  /**
   * @brief Handle to our created virtual input device.
   * @details This handle is created in `InitializeVirtualDevice` and used in `RenderVirtInputTab`
   * to simulate input events like button presses and axis movements. It must be stored as
   * part of the plugin's state to be accessible in the update/render loops.
   */
  SPF_VirtualDevice_Handle* virtualDevice = nullptr;

  // --- Plugin State ---
  // Variables that represent the internal, mutable state of the plugin at runtime.

  /**
   * @brief A cached value for the 'some_number' setting from the plugin's config file.
   * @details This value is loaded from `settings.json` in `OnLoad` and can be modified at
   * runtime via the UI. It's cached here to avoid reading from the config API every frame.
   */
  int32_t someNumber = 0;

  /**
   * @brief A buffer to hold the command to be sent to the game console via the UI.
   * @details A fixed-size C-style array is used here because the ImGui `InputText` function
   * (which the UI API wraps) is a C-style API that operates on `char*` buffers.
   */
  char consoleCommand[256] = "g_traffic 1";

  /**
   * @brief A flag to control whether the game string formatting hook should modify the quit button color.
   * @details This is a simple boolean toggled by a checkbox in the UI. It is read by the
   * `Detour_GameStringFormatting` hook function to decide whether to apply its modification.
   */
  bool isModificationActive = false;

  // --- Hooking State ---

  /**
   * @brief Trampoline pointer to the original game string formatting function.
   * @details When we hook a function using `coreAPI->hooks->Register`, the framework finds the
   * original function and stores a pointer to a "trampoline" here. The trampoline is a small
   * piece of code that allows us to call the original, un-hooked function. Our detour function
   * *must* call this trampoline to ensure the original game logic is executed, otherwise the
   * game will likely crash or misbehave.
   */
  GameStringFormatting_t o_GameStringFormatting = nullptr;

  /**
   * @brief Handle for the registered GameLog callback.
   * @details This handle is returned by `g_ctx.coreAPI->gamelog->RegisterCallback` and
   * must be stored to keep the callback active. Its destruction (managed by the framework)
   * will automatically unregister the callback.
   */
  SPF_GameLog_Callback_Handle gameLogCallbackHandle = nullptr;
};

/**
 * @brief The single global instance of the plugin's context.
 * @details This is defined once in `ExamplePlugin.cpp` and declared `extern` here, making it
 * accessible throughout all of the plugin's source files. It serves as the bridge between the
 * C-style, callback-driven nature of the framework and a more organized, object-oriented
 * approach to state management.
 */
extern PluginContext g_ctx;

// =================================================================================================
// 4. Function Prototypes
// =================================================================================================
// Prototypes are organized by functionality to make the header file readable and serve as a
// table of contents for the plugin's features. This helps new developers quickly understand
// what the plugin does.

// --- Manifest ---

/**
 * @brief Fills the manifest structure with this plugin's metadata.
 * @details This function is called by the framework *before* the plugin is loaded to learn
 * about its name, version, default settings, and requirements.
 * @param[out] out_manifest A reference to the manifest structure to be filled by the function.
 */
void GetManifestData(SPF_ManifestData_C& out_manifest);

// --- Plugin Lifecycle ---
// These are the primary entry points called by the framework in a specific, guaranteed order.
// A plugin's core logic is built around these functions.

/**
 * @brief Called first when the plugin DLL is loaded into memory.
 * @details This is the earliest point for initialization. Use it for setup that does not
 * depend on the game being fully active. Only the `load_api` services (logger, config,
 * localization, formatting) are available here.
 * @param load_api A pointer to the Load API.
 */
void OnLoad(const SPF_Load_API* load_api);

/**
 * @brief Called when the plugin is activated by the framework.
 * @details This function is called after `OnLoad` and after the framework has processed the
 * plugin's manifest. At this point, the game is running and all framework services are
 * available via the `core_api`. This is the main initialization function where you should
 * register callbacks for keybinds, hooks, telemetry, etc.
 * @param core_api A pointer to the Core API, which contains pointers to all other APIs.
 */
void OnActivated(const SPF_Core_API* core_api);

/**
 * @brief (Optional) Called once after the game world has been loaded.
 * @details This function is the ideal place to initialize logic that depends on
 *          in-game objects being available (e.g., camera hooks, reading vehicle data).
 */
void OnGameWorldReady();

/**
 * @brief Called every frame while the plugin is active.
 * @details This function is tied to the rendering loop. Avoid doing heavy or blocking work
 * here as it can impact game performance. It's suitable for polling data or updating animations.
 * For frequent logging, use the throttled logger API.
 */
void OnUpdate();

/**
 * @brief Called last, just before the plugin is unloaded from memory.
 * @details Use this function to perform all necessary cleanup, such as freeing allocated
 * memory, saving any pending data, and nulling out pointers to prevent use-after-free errors.
 */
void OnUnload();

// --- Framework Callbacks ---
// These are functions that the plugin implements and registers with the framework. The framework
// then calls them in response to specific events.

/**
 * @brief Called when a setting is changed externally (e.g., via the main settings UI or by another plugin).
 * @param keyPath The dot-separated path of the setting that changed (e.g., "settings.some_number").
 * @param value_handle A handle to the new JSON value.
 * @param json_reader A pointer to the JSON reader API for safely extracting the value from the handle.
 */
void OnSettingChanged(const char* keyPath, const SPF_JsonValue_Handle* value_handle, const SPF_JsonReader_API* json_reader);

/**
 * @brief Called for each new line added to the in-game log.
 * @details This callback is registered with the Game Log API. It's useful for monitoring game
 * events that are only reported in the log, like hiring a driver or discovering a city.
 * @param log_line The content of the log line.
 * @param user_data A pointer to user-defined data passed during registration (not used here).
 */
void OnGameLogMessage(const char* log_line, void* user_data);

/**
 * @brief Callback executed when the 'ExamplePlugin.MainWindow.toggle' keybind is triggered by the user.
 */
void OnToggleMainWindow();

/**
 * @brief Callback executed when the 'ExamplePlugin.Camera.cycle' keybind is triggered.
 */
void OnCameraKeybind();

// --- UI Implementation ---

/**
 * @brief Called once to allow the plugin to register its UI rendering callbacks.
 * @details The framework calls this when the UI system is ready. In this function, you link
 * the window names from your manifest to the C++ functions that will draw their content.
 * @param ui_api A pointer to the UI API.
 */
void OnRegisterUI(SPF_UI_API* ui_api);

/**
 * @brief Renders the content of the plugin's main window and its tabs.
 * @details This function is registered as a callback and is called by the UI system every
 * frame that the window is visible. It uses the UI API to draw widgets.
 * @param ui A pointer to the UI API, used to draw widgets.
 * @param user_data A pointer to user-defined data passed during registration (not used here).
 */
void RenderMainWindow(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the content of the "Camera" tab within the main window.
 */
void RenderCameraTab(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the content of the "Telemetry" tab.
 */
void RenderTelemetryTab(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the content of the "Virtual Input" tab.
 */
void RenderVirtInputTab(SPF_UI_API* ui, void* user_data);

// --- Helper Functions ---
// These are internal functions that encapsulate specific logic for better organization.

/**
 * @brief Creates and registers the plugin's virtual input device.
 * @details This demonstrates the workflow for the Virtual Input API: create, add inputs, then register.
 */
void InitializeVirtualDevice();

/**
 * @brief Finds the target function in memory and installs the game string formatting hook.
 * @details This demonstrates the workflow for the Hooks API.
 */
void InstallGameStringFormattingHook();

// --- Hook Implementation ---

/**
 * @brief Our detour function that will be called instead of the original game string formatting function.
 * @details This function intercepts the call, checks if our modification is active, potentially
 * modifies the input, and then **must** call the original function via the trampoline.
 * @param pOutput The same output buffer pointer as the original function.
 * @param ppInput The same input string pointer as the original function.
 * @return The return value from the original function, called via the trampoline.
 */
void* Detour_GameStringFormatting(void* pOutput, const char** ppInput);

}  // namespace ExamplePlugin
