# SPF Virtual Input API

The SPF Virtual Input API allows your plugin to create "virtual" input devices, such as gamepads or steering wheels. Your plugin can then programmatically simulate button presses and axis movements on these devices, and the game will recognize them as if they came from a physical piece of hardware.

This can be used for a wide range of applications, including:
*   Using a mobile phone's accelerometer to control steering.
*   Creating custom input hardware that communicates with your plugin.
*   Mapping data from external sources to in-game controls.

**Header:** `include/SPF/SPF_API/SPF_VirtInput_API.h`

## Core Concept: Device Types

When you create a virtual device, you must choose one of two types:

**`SPF_INPUT_DEVICE_TYPE_GENERIC`**
A generic device acts like a standard joystick or gamepad. It will appear in the game's "Controls" menu, where the user can see its buttons and axes and bind them to any game action they wish (e.g., binding "Button 1" to "Honk Horn"). This is the most flexible option for creating general-purpose virtual controllers.

**`SPF_INPUT_DEVICE_TYPE_SEMANTICAL`**
A semantical device's inputs are mapped directly to specific game functions by their name. For example, an axis with the programmatic name `"steer"` will directly control the truck's steering, and an axis named `"throttle"` will control the throttle. These devices do **not** appear in the game's controls menu for binding because their function is fixed. This is useful for plugins that want to directly control the game without requiring user configuration.

## Workflow

Creating and using a virtual device follows a clear, multi-step process:

1.  **Create Device:** Call `Virt_CreateDevice()` to create a new device and get its handle.
2.  **Add Inputs:** Call `Virt_AddButton()` and `Virt_AddAxis()` to define all the inputs your device will have. This must be done *before* registering the device.
3.  **Register Device:** Call `Virt_Register()` to finalize the device's configuration and make it visible to the game. No more inputs can be added after this point.
4.  **Simulate Events:** In your plugin's main loop (e.g., `OnUpdate`), call functions like `Virt_PressButton()`, `Virt_ReleaseButton()`, and `Virt_SetAxisValue()` to send input events to the game.

## Critical Lifecycle Rules

> [!CAUTION]
> **Virtual devices MUST be created during the `OnLoad` phase.**

The underlying game engine (SCS SDK) only allows virtual device registration during the initial input boot sequence. 

*   **Correct Placement**: Always call `Virt_CreateDevice` and `Virt_Register` inside your plugin's `OnLoad` function.
*   **The Late Enablement Issue**: If your plugin is disabled when the game starts, the framework will not load it during the boot sequence. If you then enable the plugin through the UI mid-session, its `OnLoad` will be called, but the game will reject the device registration because the "registration window" is already closed.
*   **Solution**: If you enable a virtual-input plugin mid-session, you **must restart the SPF SDK** (using the "Restart SDK" button in the framework UI) to re-trigger the boot sequence and make the device visible to the game.

## Getting the API

The Virtual Input API is provided as part of the main `SPF_Core_API` struct, where it is named `input`.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"

// In OnActivated:
void OnActivated(const SPF_Core_API* api) {
    // api->input can now be used
    // Its type is SPF_VirtInput_API*
}
```

## Function Reference

### Device Creation
---
**`SPF_VirtualDevice_Handle* Virt_CreateDevice(...)`**
Creates a new virtual device.
*   `pluginName`: Your plugin's name.
*   `deviceName`: A unique internal name for the device (e.g., `"my_virtual_gamepad"`).
*   `displayName`: The name shown in the game's UI (e.g., `"My Virtual Gamepad"`).
*   `type`: The device type (`GENERIC` or `SEMANTICAL`).
*   **Returns:** A handle to the device.

---
**`void Virt_AddButton(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName)`**
Adds a button to a device. Must be called before `Virt_Register()`.
*   `h`: The handle to the virtual device.
*   `inputName`: Programmatic name used to identify the button (e.g., `"action_a"`).
*   `displayName`: Name shown in the game's UI for binding (e.g., `"Action A"`).

---
**`void Virt_AddAxis(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName)`**
Adds an axis to a device. Must be called before `Virt_Register()`.
*   `h`: The handle to the virtual device.
*   `inputName`: Programmatic name for the axis (e.g., `"x_axis"`).
*   `displayName`: Name shown in the UI (e.g., `"X Axis"`).

---
**`bool Virt_Register(SPF_VirtualDevice_Handle* h)`**
Finalizes and registers the device with the game.

### Event Simulation
---
**`void Virt_PressButton(SPF_VirtualDevice_Handle* h, const char* inputName)`**
Simulates pressing and holding a button. The button remains pressed until `Virt_ReleaseButton` is called.

---
**`void Virt_ReleaseButton(SPF_VirtualDevice_Handle* h, const char* inputName)`**
Simulates releasing a button.

---
**`void Virt_SetAxisValue(SPF_VirtualDevice_Handle* h, const char* inputName, float value)`**
Sets the value of an axis. The value is typically in the range of -1.0 to 1.0.

## Complete Example

This example creates a generic virtual gamepad with one button and one axis.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"

static SPF_VirtualDevice_Handle* s_hGamepad = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* api) {
    if (api && api->input) {
        // 1. Create a generic device
        s_hGamepad = api->input->Virt_CreateDevice("MyPlugin", "my_gamepad", "My Virtual Gamepad", SPF_INPUT_DEVICE_TYPE_GENERIC);
        
        if (s_hGamepad) {
            // 2. Add inputs before registering
            api->input->Virt_AddButton(s_hGamepad, "honk_button", "Virtual Honk");
            api->input->Virt_AddAxis(s_hGamepad, "steer_axis", "Virtual Steering");
            
            // 3. Finalize the device
            api->input->Virt_Register(s_hGamepad);
        }
    }
}

void MyPlugin_OnUpdate() {
    if (!s_hGamepad) return;

    // 4. Simulate events
    if (ShouldHonk()) {
        g_coreApi->input->Virt_PressButton(s_hGamepad, "honk_button");
    } else {
        g_coreApi->input->Virt_ReleaseButton(s_hGamepad, "honk_button");
    }

    float steering = 0.5f; 
    g_coreApi->input->Virt_SetAxisValue(s_hGamepad, "steer_axis", steering);
}
```
After running this code, you can go into the game's controls menu, see "My Virtual Gamepad", and bind "Virtual Honk" and "Virtual Steering" to game actions.