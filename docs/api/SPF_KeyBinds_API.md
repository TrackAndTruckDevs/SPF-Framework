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

## Data Types (Enums)

### `SPF_BindingType`
Identifies the physical source of an input binding.
- `SPF_BINDING_UNKNOWN`: Unknown or invalid type.
- `SPF_BINDING_KEYBOARD`: Standard keyboard key.
- `SPF_BINDING_GAMEPAD`: Digital button on a gamepad.
- `SPF_BINDING_MOUSE`: Digital button on a mouse.
- `SPF_BINDING_JOYSTICK`: Digital button on a flight stick/joystick.
- `SPF_BINDING_CHORD`: Key combination (e.g., Shift+G).
- `SPF_BINDING_GAMEPAD_AXIS`: Analog stick or trigger on a gamepad.
- `SPF_BINDING_MOUSE_AXIS`: Mouse wheel or cursor movement.
- `SPF_BINDING_JOYSTICK_AXIS`: Analog axis on a joystick.

### `SPF_ActivationBehavior`
Describes how a digital action responds to user interaction (affects callbacks only).
- `SPF_BEHAVIOR_HOLD`: Action is active as long as the button is physically held.
- `SPF_BEHAVIOR_TOGGLE`: Action toggles state (ON/OFF) with each press.
- `SPF_BEHAVIOR_NA`: Not applicable (e.g., for raw analog axes).

### `SPF_PressType`
Distinguishes between short and long presses.
- `SPF_PRESS_SHORT`: Standard quick press.
- `SPF_PRESS_LONG`: Button must be held for a specific duration.
- `SPF_PRESS_NA`: Not applicable.

### `SPF_InputMode`
- `SPF_MODE_ANALOG`: Axis returns smooth range values.
- `SPF_MODE_DIGITAL`: Axis acts like a button (triggers at threshold).
- `SPF_MODE_NA`: Not applicable.

### `SPF_AxisSide`
- `SPF_SIDE_POSITIVE`: Only positive values (0.0 to 1.0).
- `SPF_SIDE_NEGATIVE`: Only negative values (0.0 to -1.0).
- `SPF_SIDE_BOTH`: Full range used.
- `SPF_SIDE_NA`: Not applicable.

### `SPF_AccumulatorMode`
- `SPF_ACCUMULATOR_OFF`: Normal absolute input.
- `SPF_ACCUMULATOR_ON`: Virtual Knob (accumulates deltas).
- `SPF_ACCUMULATOR_NA`: Not applicable.

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
**`float Kbind_GetActionValue(SPF_KeyBinds_Handle* h, const char* actionName)`**
Gets the current value of the input bound to the specified action. This function provides a unified way to read both digital and analog inputs.

> **IMPORTANT FOR DIGITAL ACTIONS:**
> For actions bound to buttons, this method returns the **immediate physical state** (1.0 = pressed, 0.0 = released). It **ignores** logical behaviors such as 'toggle', 'hold', or 'press_type'. If you need to react to these logical events, use `Kbind_Register` instead.

*   **h:** The context handle obtained from `Kbind_GetContext`.
*   **actionName:** The full name of the action (e.g., "MyPlugin.Controls.Throttle").
*   **Returns:** A `float` value representing the current processed state:
    *   **Digital Buttons (Keyboard/Gamepad):** Returns `1.0` if pressed, `0.0` otherwise.
    *   **Analog Triggers:** Returns `0.0` to `1.0`.
    *   **Analog Sticks (Standard):**
        *   Bound to **Both** sides: returns raw value `-1.0` to `1.0`.
        *   Bound to **Positive Side**: returns `0.0` to `1.0`.
        *   Bound to **Negative Side**: returns `0.0` to `1.0` (Normalized absolute magnitude). This is useful for splitting one axis into two logical actions (e.g., Brake/Throttle) without manual math.
    *   **Accumulator Mode (Knobs/Mouse Wheel):** Returns the current persistent state of the virtual controller. By default, this is clamped to `[-1.0, 1.0]`, but the limits can be customized by the user in the Settings UI.

**Normalization Logic for Sides:**
To simplify plugin development, when an action is mapped to a specific side of an axis, the framework treats it as a 0..1 scale representing "how much" the action is active:
- **Positive Side:** Physical `0.5` -> Returns `0.5`; Physical `-0.5` -> Returns `0.0`.
- **Negative Side:** Physical `-0.8` -> Returns `0.8`.
- **Both Sides:** Physical `-0.8` -> Returns `-0.8`.

---
**`int Kbind_GetBindingCount(SPF_KeyBinds_Handle* h, const char* actionName)`**
Returns the number of physical bindings (keys/axes) assigned to a logical action.

---
**`SPF_BindingType Kbind_GetBindingType(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Gets the source type of a specific binding (Keyboard, Gamepad, etc.).

---
**`SPF_ActivationBehavior Kbind_GetBindingBehavior(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Gets the behavior (Hold/Toggle) for a binding.

---
**`SPF_PressType Kbind_GetBindingPressType(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Gets the press type (Short/Long) for a binding.

---
**`SPF_InputMode Kbind_GetBindingMode(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Gets the input mode (Analog/Digital) for an axis.

---
**`SPF_AxisSide Kbind_GetBindingSide(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Identifies which side of the axis range is monitored.

---
**`SPF_AccumulatorMode Kbind_GetBindingAccumulatorMode(SPF_KeyBinds_Handle* h, const char* actionName, int index)`**
Gets the accumulator mode for an axis.

---
**`int Kbind_GetBindingName(SPF_KeyBinds_Handle* h, const char* actionName, int index, char* out_buffer, int buffer_size)`**
Gets the human-readable display name of the input (e.g., "Space", "Cross").

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

## Advanced Usage Example: Inspecting Bindings

This example shows how to inspect all physical bindings assigned to an action.

```cpp
void OnActivated(const SPF_Core_API* api) {
    SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
    const char* action = "MyPlugin.General.Jump";

    int count = api->keybinds->Kbind_GetBindingCount(h, action);
    for (int i = 0; i < count; i++) {
        SPF_BindingType type = api->keybinds->Kbind_GetBindingType(h, action, i);
        
        char name[64];
        api->keybinds->Kbind_GetBindingName(h, action, i, name, sizeof(name));
        
        if (type == SPF_BINDING_GAMEPAD_AXIS) {
            SPF_InputMode mode = api->keybinds->Kbind_GetBindingMode(h, action, i);
            // Adapt logic if user is using a trigger as a digital button
        }
    }
}
```
