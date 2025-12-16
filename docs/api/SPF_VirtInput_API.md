# SPF Virtual Input API

The SPF Virtual Input API is a powerful feature that allows your plugin to create "virtual" input devices, such as gamepads or steering wheels. Your plugin can then programmatically simulate button presses and axis movements on these devices, and the game will recognize them as if they came from a physical piece of hardware.

This can be used for a wide range of applications, including:
*   Using a mobile phone's accelerometer to control steering.
*   Creating custom input hardware that communicates with your plugin.
*   Mapping data from external sources to in-game controls.

## Core Concept: Device Types

When you create a virtual device, you must choose one of two types:

**`SPF_INPUT_DEVICE_TYPE_GENERIC`**
A generic device acts like a standard joystick or gamepad. It will appear in the game's "Controls" menu, where the user can see its buttons and axes and bind them to any game action they wish (e.g., binding "Button 1" to "Honk Horn"). This is the most flexible option for creating general-purpose virtual controllers.

**`SPF_INPUT_DEVICE_TYPE_SEMANTICAL`**
A semantical device's inputs are mapped directly to specific game functions by their name. For example, an axis with the programmatic name `"steer"` will directly control the truck's steering, and an axis named `"throttle"` will control the throttle. These devices do **not** appear in the game's controls menu for binding because their function is fixed. This is useful for plugins that want to directly control the game without requiring user configuration.

## Workflow

Creating and using a virtual device follows a clear, multi-step process:

1.  **Create Device:** Call `CreateDevice()` to create a new device and get its handle.
2.  **Add Inputs:** Call `AddButton()` and `AddAxis()` to define all the inputs your device will have. This must be done *before* registering the device.
3.  **Register Device:** Call `Register()` to finalize the device's configuration and make it visible to the game. No more inputs can be added after this point.
4.  **Simulate Events:** In your plugin's main loop (e.g., `OnUpdate`), call functions like `PressButton()`, `ReleaseButton()`, and `SetAxisValue()` to send input events to the game.

## Getting the API

The Virtual Input API is provided as part of the main `SPF_Core_API` struct, where it is named `input`.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"

const SPF_Core_API* s_coreAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
    // s_coreAPI->input can now be used
}
```
*Note: The API is defined in `SPF_VirtInput_API.h` but the struct member in `SPF_Core_API` is named `input`.*

## Function Reference

### Device Creation
---
**`SPF_VirtualDevice_Handle* CreateDevice(...)`**
Creates a new virtual device.
*   `pluginName`: Your plugin's name.
*   `deviceName`: A unique internal name for the device (e.g., `"my_virtual_gamepad"`).
*   `displayName`: The name shown in the game's UI (e.g., `"My Virtual Gamepad"`).
*   `type`: The device type (`GENERIC` or `SEMANTICAL`).
*   **Returns:** A handle to the device.

---
**`void AddButton(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName)`**
Adds a button to a device. Must be called before `Register()`.
*   `inputName`: Programmatic name used to identify the button (e.g., `"action_a"`).
*   `displayName`: Name shown in the game's UI (e.g., `"Action A"`).

---
**`void AddAxis(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName)`**
Adds an axis to a device. Must be called before `Register()`.
*   `inputName`: Programmatic name for the axis (e.g., `"x_axis"`).
*   `displayName`: Name shown in the UI (e.g., `"X Axis"`).

---
**`bool Register(SPF_VirtualDevice_Handle* handle)`**
Finalizes and registers the device with the game.

### Event Simulation
---
**`void PressButton(SPF_VirtualDevice_Handle* handle, const char* inputName)`**
Simulates pressing and holding a button. The button remains pressed until `ReleaseButton` is called.

---
**`void ReleaseButton(SPF_VirtualDevice_Handle* handle, const char* inputName)`**
Simulates releasing a button.

---
**`void SetAxisValue(SPF_VirtualDevice_Handle* handle, const char* inputName, float value)`**
Sets the value of an axis. The value is typically in the range of -1.0 to 1.0.

## Complete Example

This example creates a generic virtual gamepad with one button and one axis.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"
#include "SPF/SPF_API/SPF_Telemetry_API.h" // For getting truck data

const SPF_Core_API* s_coreAPI = NULL;
SPF_VirtualDevice_Handle* s_myGamepad = NULL;

// 1. Create and register the device on load
SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;

    if (s_coreAPI && s_coreAPI->input) {
        // Create a generic device
        s_myGamepad = s_coreAPI->input->CreateDevice("MyPlugin", "my_gamepad", "My Virtual Gamepad", SPF_INPUT_DEVICE_TYPE_GENERIC);
        
        if (s_myGamepad) {
            // Add inputs before registering
            s_coreAPI->input->AddButton(s_myGamepad, "honk_button", "Virtual Honk");
            s_coreAPI->input->AddAxis(s_myGamepad, "steer_axis", "Virtual Steering");
            
            // Finalize the device
            s_coreAPI->input->Register(s_myGamepad);
        }
    }
}

// 2. Simulate events in the update loop
void MyPlugin_OnUpdate() {
    if (!s_coreAPI || !s_coreAPI->telemetry || !s_myGamepad) return;

    // Example: Press the button if the truck speed is over 20 m/s
    SPF_TruckData truck_data;
    s_coreAPI->telemetry->GetTruckData(s_coreAPI->telemetry->GetContext("MyPlugin"), &truck_data);

    if (truck_data.speed > 20.0f) {
        s_coreAPI->input->PressButton(s_myGamepad, "honk_button");
    } else {
        s_coreAPI->input->ReleaseButton(s_myGamepad, "honk_button");
    }

    // Example: Map some other value to the steering axis
    float some_value = -0.5f; // This could come from anywhere (e.g., phone sensor)
    s_coreAPI->input->SetAxisValue(s_myGamepad, "steer_axis", some_value);
}
```
After running this code, you can go into the game's controls menu, see "My Virtual Gamepad", and bind "Virtual Honk" and "Virtual Steering" to game actions.
