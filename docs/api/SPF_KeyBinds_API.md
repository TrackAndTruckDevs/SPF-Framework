# SPF KeyBinds API

The SPF KeyBinds API provides a powerful and flexible system for your plugin to define abstract "actions" that can be triggered by user-defined key combinations. This decouples your plugin's logic from hard-coded keys, allowing users to fully customize their controls.

## Core Concepts

Understanding the keybind system requires knowing three core concepts:

**1. Action**
An "action" is a named, logical operation within your plugin, like "toggle UI" or "increase value". Each action is defined in your plugin's manifest or registered dynamically at runtime using `Kbind_RegisterActionMetadata`. Starting from v1.2.0, the API handles plugin namespaces automatically (Smart Naming).

**2. Keybind**
A "keybind" is the specific keyboard or gamepad button combination that triggers an action. You provide default keybinds in your manifest, but the user can always override these in the framework's main Settings UI.

**3. Callback**
A "callback" is a C function within your plugin that the framework executes when a keybind triggers its associated action.

**4. Dynamic Blocking**
By default, actions can pass input to the game or consume it entirely based on settings. With "Manual" (Plugin Managed) policy, a plugin can decide at runtime whether a physical key press should be blocked from the game using the `Kbind_SetBlockState` function.

## Workflow

The process is simple and involves two main steps:

1.  **Declare Actions:** Define your actions statically in the `BuildManifest` function or dynamically at runtime using `Kbind_RegisterActionMetadata`.
2.  **Register Callback:** In your plugin's `OnActivated` function, you call the `Register` function to link a specific action name to a C function in your code.

## Smart Naming

Starting from v1.2.0, the API automatically handles plugin-specific namespaces. You no longer need to manually prepend your Plugin ID to every action name.

### How it works:
- If you provide a name like `"toggle"`, the API automatically converts it to `"{PluginID}.toggle"`.
- If you provide a group like `"UI.open"`, it becomes `"{PluginID}.UI.open"`.
- Names that already start with your Plugin ID remain unchanged.

| Input Name          | Internal Full Key (Result)      |
|---------------------|---------------------------------|
| "honk"              | "MyPlugin.honk"                 |
| "Camera.cycle"      | "MyPlugin.Camera.cycle"         |
| "MyPlugin.test"     | "MyPlugin.test" (no change)     |

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
Registers a standard callback function for a specific logical action.
*   **h:** The context handle obtained from `Kbind_GetContext`.
*   **actionName:** The name of the action. **Smart Naming** is applied (e.g., `"General.DoWork"` becomes `"MyPlugin.General.DoWork"`).
*   **callback:** A pointer to a `void(void)` function that will be called when the action is triggered.

---
**`typedef void (*SPF_Keybind_Callback_Ex)(const char* action_id, void* user_data)`**
Advanced callback type that receives context information.
*   **action_id:** The full internal name of the action that was triggered (e.g., `"MyPlugin.UI.toggle"`).
*   **user_data:** The custom pointer that was passed during registration.

---
**`void Kbind_Register_Ex(SPF_KeyBinds_Handle* h, const char* actionName, SPF_Keybind_Callback_Ex callback, void* user_data)`**
Registers an extended callback function with a context pointer and action ID support.
*   **h:** The context handle.
*   **actionName:** The logical name of the action. **Smart Naming** is applied.
*   **callback:** The function pointer (receives `action_id` and `user_data`).
*   **user_data:** An arbitrary pointer that will be passed back to the callback. This is useful for passing class instances or specific context objects.

---
**`float Kbind_GetActionValue(SPF_KeyBinds_Handle* h, const char* actionName)`**
Gets the current value of the input bound to the specified action. This function provides a unified way to read both digital and analog inputs.

> **IMPORTANT FOR DIGITAL ACTIONS:**
> For actions bound to buttons, this method returns the **immediate physical state** (1.0 = pressed, 0.0 = released). It **ignores** logical behaviors such as 'toggle', 'hold', or 'press_type'. If you need to react to these logical events, use `Kbind_Register` instead.

*   **h:** The context handle obtained from `Kbind_GetContext`.
*   **actionName:** The logical name of the action (e.g., `"Controls.Throttle"`). **Smart Naming** is applied.
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
*   **actionName:** The logical name of the action (e.g., `"Movement.Forward"`). **Smart Naming** is applied.
*   **block:** If `true`, the framework will block the input from reaching the game. If `false`, the input will be passed through.
*   **Note**: This function is only effective if the action's `consume` policy is set to **"manual"** (Plugin Managed) in the settings.

---
**`void Kbind_RegisterActionMetadata(SPF_KeyBinds_Handle* h, const char* actionName, const char* titleKey, const char* descKey, SPF_Keybind_Callback_Ex callback, void* user_data)`**
Dynamically registers a new logical action at runtime with an optional extended callback. This allows creating new actions "on the fly" and assigning logic in a single call.
*   **h:** The context handle.
*   **actionName:** The internal ID for the action. **Smart Naming** is applied.
*   **titleKey:** Localization key (or literal) for the display name in the menu.
*   **descKey:** (Optional) Localization key (or literal) for the tooltip description.
*   **callback:** (Optional) An extended callback function. If provided, it will be automatically registered for this action.
*   **user_data:** (Optional) Data pointer for the callback.

---
**`void Kbind_UnregisterActionMetadata(SPF_KeyBinds_Handle* h, const char* actionName)`**
Removes a dynamically registered action from the UI and configuration.
*   **actionName:** The name of the action to remove. **Smart Naming** is applied.

---
**`int Kbind_GetActionCount(SPF_KeyBinds_Handle* h)`**
Returns the total number of actions currently owned by the plugin (both static and dynamic).

---
**`int Kbind_GetActionNameByIndex(SPF_KeyBinds_Handle* h, int index, char* out_buffer, int buffer_size)`**
Gets the full name of an action by its zero-based index. Useful for recovering dynamic actions during plugin activation to re-register their callbacks.

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

## Complete Example (Static & Dynamic)

This example shows how to define actions and register callbacks using the modern Smart Naming approach.

**1. Manifest Definition (`BuildManifest`)**
Define static actions and their default keybinds.

```c
void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // SMART NAMING: "UI" group will be automatically prefixed with "MyPlugin."
    api->Defaults_AddKeybind(h, "UI", "toggle_window", "keyboard", "KEY_F5", "always");
    api->Meta_AddKeybind(h, "UI", "toggle_window", "Toggle Menu", "Press F5 to open UI");
}
```

**2. C++ Implementation & Dynamic Actions**
Register callbacks and add new actions on the fly.

```c
void ToggleMainWindow() { /* ... */ }

// Extended callback can identify which of many actions was triggered
void OnDynamicAction(const char* action_id, void* user_data) {
    Log("Action %s triggered with context %s", action_id, (const char*)user_data);
}

void MyPlugin_OnActivated(const SPF_Core_API* api) {
    if (api->keybinds) {
        SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
        
        // A. Register static action (short names work!)
        api->keybinds->Kbind_Register(h, "UI.toggle_window", &ToggleMainWindow);

        // B. Add a dynamic action with logic in ONE call
        api->keybinds->Kbind_RegisterActionMetadata(h, "Dynamic.Cmd1", "Manual Action", "Created at runtime", &OnDynamicAction, (void*)"Cmd1Context");

        // C. Use Register_Ex for actions defined in manifest (e.g. to use one function for multiple buttons)
        api->keybinds->Kbind_Register_Ex(h, "Camera.cycle", &OnDynamicAction, (void*)"CameraContext");

        // D. Recover actions (e.g. after game restart)
        int count = api->keybinds->Kbind_GetActionCount(h);
        for (int i = 0; i < count; i++) {
            char name[128];
            api->keybinds->Kbind_GetActionNameByIndex(h, i, name, sizeof(name));
            // Re-register universal callback for dynamic actions
            if (strstr(name, "Dynamic.")) {
                api->keybinds->Kbind_Register_Ex(h, name, &OnDynamicAction, (void*)"Restored");
            }
        }
    }
}
```
Now, when the user presses F5 (or whatever key they rebind it to), the `ToggleMainWindow` function will be called.

## Advanced Usage Example: Inspecting Bindings

This example shows how to inspect all physical bindings assigned to an action using short names.

```cpp
void OnActivated(const SPF_Core_API* api) {
    SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
    const char* action = "General.Jump"; // Smart Naming will resolve to "MyPlugin.General.Jump"

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
