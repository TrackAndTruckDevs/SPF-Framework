# SPF KeyBinds API

The SPF KeyBinds API provides a powerful and flexible system for your plugin to define abstract "actions" that can be triggered by user-defined key combinations. This decouples your plugin's logic from hard-coded keys, allowing users to fully customize their controls.

## Core Concepts

Understanding the keybind system requires knowing three core concepts:

**1. Action**
An "action" is a named, logical operation within your plugin, like "toggle UI" or "increase value". Each action is defined in your plugin's manifest and has a unique name.

**2. Keybind**
A "keybind" is the specific keyboard or gamepad button combination that triggers an action. You provide default keybinds in your manifest, but the user can always override these in the framework's main Settings UI.

**3. Callback**
A "callback" is a C function within your plugin that the framework executes when a keybind triggers its associated action.

**4. Dynamic Blocking**
By default, actions can pass input to the game or consume it entirely based on settings. With "Manual" (Plugin Managed) policy, a plugin can decide at runtime whether a physical key press should be blocked from the game using the `Kbind_SetBlockState` function.

## Workflow

The process is simple and involves two main steps:

1.  **Declare in Manifest:** In your `GetManifestData` function, you define all your plugin's actions and their default keybinds. This makes the framework's UI aware of them.
2.  **Register Callback:** In your plugin's `OnLoad` function, you call the `Register` function to link a specific action name from your manifest to a C function in your code.

## Getting the API Context

To register callbacks, you must first get your plugin's unique `SPF_KeyBinds_Handle`.

```cpp
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_KeyBinds_API.h"

const SPF_KeyBinds_API* s_keybindsAPI = NULL;
SPF_KeyBinds_Handle* s_myPluginKeybinds = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* api) {
    s_keybindsAPI = api->keybinds;
    
    if (s_keybindsAPI) {
        s_myPluginKeybinds = s_keybindsAPI->Kbind_GetContext("MyPlugin");
    }
}
```

## Function Reference

---
**`SPF_KeyBinds_Handle* Kbind_GetContext(const char* pluginName)`**
Gets a keybinds context handle for your plugin.
*   **pluginName:** Your plugin's name, which must match the manifest.
*   **Returns:** A handle to the keybinds context, or `NULL`.

---
**`void Kbind_Register(SPF_KeyBinds_Handle* h, const char* actionName, void (*callback)(void))`**
Registers a callback function for a specific action defined in the manifest.
*   **h:** The context handle obtained from `Kbind_GetContext`.
*   **actionName:** The name of the action. This **must** exactly match an `actionName` you defined in your manifest.
*   **callback:** A pointer to a `void(void)` function that will be called when the action is triggered.

---
**`void Kbind_SetBlockState(SPF_KeyBinds_Handle* h, const char* actionName, bool block)`**
Programmatically controls whether an action's physical input is blocked from the game.
*   **h:** The context handle obtained from `Kbind_GetContext`.
*   **actionName:** The full name of the action (e.g., "MyPlugin.Movement.Forward").
*   **block:** If `true`, the framework will block the input from reaching the game. If `false`, the input will be passed through.
*   **Note**: This function is only effective if the action's `consume` policy is set to **"manual"** (Plugin Managed) in the settings.

---
**`void Kbind_UnregisterAll(SPF_KeyBinds_Handle* h)`**
Manually unregisters all actions and callbacks associated with your plugin's handle. This is normally handled automatically when the plugin unloads.

## Dynamic Blocking Example

This feature allows a plugin to "take over" a shared key (like WASD) only when a specific mode is active.

1. Set the action's `consume` policy to `"manual"` in your manifest or via the framework UI.
2. Call `Kbind_SetBlockState` when you need to start/stop intercepting the key.

```c
void OnWalkModeToggle(bool active) {
    SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
    
    // When walk mode is active, block the 'W' key from the game (gas pedal)
    // so we can use it for walking instead.
    api->keybinds->Kbind_SetBlockState(h, "MyPlugin.Movement.Forward", active);
}
```

## Complete Example

This example shows how to define an action to toggle a UI window and register a callback for it.

**1. Manifest Definition (`GetManifestData`)**
First, define the action and its default keybind in your manifest.

```c
// In GetManifestData()
g_manifest.keybinds.actionCount = 1;
g_manifest.keybinds.actions[0].groupName = "MyPlugin.UI";
g_manifest.keybinds.actions[0].actionName = "toggle_main_window";
g_manifest.keybinds.actions[0].keybind.keyCode = KEY_F5; // Default to F5
```

**2. C++ Implementation**
Next, register a callback for this action in your plugin's code.

```c
// Global state for our window
static bool s_isWindowVisible = false;

// The callback function that will be triggered
void ToggleMainWindow() {
    s_isWindowVisible = !s_isWindowVisible;
    
    // In a real plugin, you would use the UI API to show/hide the window
}

// In your activation function
void MyPlugin_OnActivated(const SPF_Core_API* api) {
    if (api->keybinds) {
        SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
        // Register the "MyPlugin.UI.toggle_main_window" action to our C function
        api->keybinds->Kbind_Register(h, "MyPlugin.UI.toggle_main_window", &ToggleMainWindow);
    }
}
```
Now, when the user presses F5 (or whatever key they rebind it to), the `ToggleMainWindow` function will be called.
