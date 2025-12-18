# The SPF Plugin Structure

This document describes the fundamental structure and lifecycle of any plugin built for the SPF-Framework. The entire interface is defined in the mandatory `SPF_Plugin.h` header file, which provides a stable C-style ABI (Application Binary Interface).

## The Main Entry Point: `SPF_GetPlugin`

Every plugin DLL **must** export a C-function with the following signature:

```c
#if defined(_WIN32)
#define SPF_PLUGIN_EXPORT __declspec(dllexport)
#else
#define SPF_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
    SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports);
}
```
When the framework loads your plugin, it looks for this function by name. Your implementation's only job is to fill the provided `SPF_Plugin_Exports` struct with pointers to your plugin's lifecycle functions.

**Example:**
```c
#include "SPF/SPF_API/SPF_Plugin.h"

// Forward-declarations of our lifecycle functions
void MyPlugin_OnLoad(const SPF_Load_API* load_api);
void MyPlugin_OnActivated(const SPF_Core_API* core_api);
void MyPlugin_OnUnload();

// Main entry point implementation
extern "C" {
    SPF_PLUGIN_EXPORT bool SPF_GetPlugin(SPF_Plugin_Exports* exports) {
        if (exports) {
            exports->OnLoad = MyPlugin_OnLoad;
            exports->OnActivated = MyPlugin_OnActivated;
            exports->OnUnload = MyPlugin_OnUnload;
            // Assign other optional functions if you use them, otherwise set to NULL
            exports->OnGameWorldReady = NULL; // or MyPlugin_OnGameWorldReady
            exports->OnUpdate = NULL;
            exports->OnRegisterUI = NULL;
            exports->OnSettingChanged = NULL;
            return true;
        }
        return false;
    }
}
```

## The Plugin Lifecycle

The framework initializes your plugin in stages, calling the functions you provided in `SPF_Plugin_Exports`. It's crucial to perform the right tasks at the right stage.

---
**1. `OnLoad(const SPF_Load_API* load_api)`**
*   **When:** Immediately after your DLL is loaded, before the manifest is fully processed.
*   **Purpose:** Perform essential, early setup. The primary task here is to acquire handles for core services like the logger and config system.
*   **Available API:** You receive the `SPF_Load_API` struct, which contains only a limited set of guaranteed services: `logger`, `config`, `localization`, and `formatting`. **You cannot register hooks or keybinds here.**

---
**2. `OnActivated(const SPF_Core_API* core_api)`**
*   **When:** After the framework has processed your manifest and activated the plugin.
*   **Purpose:** This is your main initialization function. Your plugin **must** store the `core_api` pointer it receives here for later use. This is the correct place to register hooks, keybind callbacks, and perform setup that relies on the full framework.
*   **Available API:** You receive the `SPF_Core_API` struct, which contains pointers to **all** framework services.

---
**3. `OnGameWorldReady()`** (Optional)
*   **When:** Called once per session, after `OnActivated`, at the moment the player loads into the game world (e.g., can drive the truck).
*   **Purpose:** This is the ideal place to initialize logic that depends on game-world objects being available (e.g., camera hooks, reading detailed vehicle data). It provides a reliable signal that the game is "in-game" and ready.
*   **Available API:** All services via the `core_api` pointer stored during `OnActivated`.

---
**4. `OnUpdate()`** (Optional)
*   **When:** Called on every frame of the game loop.
*   **Purpose:** For logic that needs to run continuously, like updating data or animations. For performance, avoid heavy computations in this function. If you don't need it, leave the function pointer `NULL` in `SPF_Plugin_Exports`.

---
**5. `OnRegisterUI(SPF_UI_API* ui_api)`** (Optional)
*   **When:** Called once when the framework's UI system is ready.
*   **Purpose:** If your plugin adds custom UI windows (as defined in your manifest), this is where you register their rendering functions with the UI system.

---
**6. `OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath)`** (Optional)
*   **When:** Called whenever a custom setting defined in your plugin's manifest is changed by the user or code.
*   **Purpose:** Allows you to react to configuration changes in real-time.
*   **Parameters:**
    *   `config_handle`: The configuration handle for your plugin.
    *   `keyPath`: The dot-separated key of the setting that changed.
*   **Workflow:** Use the provided `config_handle` and `keyPath` with the `SPF_Config_API` functions to get the new value. See the `SPF_Config_API` documentation for details.

---
**7. `OnUnload()`**
*   **When:** Called just before your plugin DLL is unloaded from memory.
*   **Purpose:** Perform all necessary cleanup. Free any memory you allocated, save any pending data, and ensure your plugin shuts down cleanly.

## API Gateway Structs

The framework provides two main structs to access its services, corresponding to the lifecycle stages.

### `SPF_Load_API`
Passed to `OnLoad`, this contains only the essential services that are available before the plugin is fully active:
*   `logger`
*   `config`
*   `localization`
*   `formatting`

### `SPF_Core_API`
Passed to `OnActivated`, this contains all available framework services. Your plugin should store this pointer and use it throughout its lifetime.
*   All services from `SPF_Load_API`, plus:
*   `keybinds`
*   `ui`
*   `telemetry`
*   `input`: The Virtual Input API, for simulating input events (e.g. key presses).
*   `hooks`
*   `camera`
*   `console`
*   `gamelog`
*   `json_reader`: (Advanced) The JSON Reader API, for parsing complex JSON data structures.

This staged approach ensures that services are only used after they have been properly initialized by the framework.
