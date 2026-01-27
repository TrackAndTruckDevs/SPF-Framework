/**
 * @file SPF_VirtInput_API.h
 * @brief API for creating and controlling virtual input devices.
 *
 * @details This API allows plugins to create virtual input devices (like gamepads)
 *          and simulate button presses and axis movements programmatically.
 *          These virtual inputs are recognized by the game engine as if they came
 *          from a physical controller.
 *
 * @section Workflow
 * 1.  **Create Device**: In your `OnLoad` function, call `Virt_CreateDevice()`
 *     to get a handle for your new virtual device.
 * 2.  **Add Inputs**: Call `Virt_AddButton()` and `Virt_AddAxis()` for each input
 *     you want to support. This defines the device's capabilities.
 * 3.  **Register Device**: Once configured, call `Virt_Register()` to make the device
 *     live and visible to the game.
 * 4.  **Simulate Events**: During `OnUpdate`, use `Virt_PressButton()`, `Virt_ReleaseButton()`,
 *     and `Virt_SetAxisValue()` to send input to the game.
 *
 * @section Lifecycle Rules (CRITICAL)
 * *   **Timing**: Virtual devices **MUST** be created and configured during the `OnLoad` lifecycle stage.
 * *   **SDK Limitation**: The underlying game engine only allows device registration during the 
 *     initial input boot phase. 
 * *   **Late Enablement**: If your plugin is disabled in the framework settings when the game starts, 
 *     the framework will NOT call your `OnLoad` during the boot phase. If the user enables your 
 *     plugin mid-session, your `OnLoad` will be called, but `Virt_Register` will fail because 
 *     the game's registration window is already closed. 
 * *   **Solution**: In such cases, the user must restart the SPF SDK (via the UI) to re-trigger 
 *     the registration phase.
 *
 * @section Usage Example * @code{.c}
 * // Global handle
 * static SPF_VirtualDevice_Handle* s_hDevice = NULL;
 *
 * void OnActivated(const SPF_Core_API* api) {
 *     // 1. Create a generic device
 *     s_hDevice = api->input->Virt_CreateDevice("MyPlugin", "v_pad", "My Virtual Pad", SPF_INPUT_DEVICE_TYPE_GENERIC);
 *     
 *     if (s_hDevice) {
 *         // 2. Add inputs (before registering)
 *         api->input->Virt_AddButton(s_hDevice, "btn_honk", "Honk Horn");
 *         api->input->Virt_AddAxis(s_hDevice, "axis_throttle", "Throttle");
 *         
 *         // 3. Register
 *         api->input->Virt_Register(s_hDevice);
 *     }
 * }
 *
 * void OnUpdate() {
 *     if (!s_hDevice) return;
 *
 *     // 4. Simulate input based on some logic
 *     if (ShouldHonk()) {
 *         g_coreApi->input->Virt_PressButton(s_hDevice, "btn_honk");
 *     } else {
 *         g_coreApi->input->Virt_ReleaseButton(s_hDevice, "btn_honk");
 *     }
 *     
 *     // Set axis value (-1.0 to 1.0)
 *     g_coreApi->input->Virt_SetAxisValue(s_hDevice, "axis_throttle", 0.75f);
 * }
 * @endcode
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Handles ---

/**
 * @brief An opaque handle to a virtual input device created by a plugin.
 *        This handle is used in all subsequent calls to identify the device.
 */
typedef struct SPF_VirtualDevice_Handle SPF_VirtualDevice_Handle;


// --- Enums and Constants ---

/**
 * @enum SPF_InputDeviceType
 * @brief Defines the type of virtual device to create.
 */
typedef enum {
    /**
     * @brief A generic device with buttons and axes.
     * @details This type of device will appear in the game's controls menu, allowing the user
     *          to freely bind its buttons and axes to any game action (e.g., binding "Button 1"
     *          to "Honk Horn"). This is useful for creating general-purpose virtual controllers.
     */
    SPF_INPUT_DEVICE_TYPE_GENERIC = 1,

    /**
     * @brief A device whose inputs map directly to specific game actions.
     * @details This type of device is "semantical," meaning its inputs have a predefined
     *          meaning (e.g., an axis named "steer" will directly control steering). These devices
     *          do not appear in the game's controls UI for binding, as their function is fixed.
     *          This is useful for plugins that want to directly control game functions like
     *          throttle or steering without user configuration.
     */
    SPF_INPUT_DEVICE_TYPE_SEMANTICAL = 2,
} SPF_InputDeviceType;


// --- API Structure ---

/**
 * @struct SPF_VirtInput_API
 * @brief API for creating and controlling virtual input devices.
 */
typedef struct SPF_VirtInput_API {
    /**
     * @brief Creates a new virtual input device for the calling plugin.
     *
     * @param pluginName The name of the calling plugin (must match the manifest).
     * @param deviceName A unique internal name for the device (e.g., "my_plugin_gamepad").
     * @param displayName The name shown in the game's UI (e.g., "My Plugin Virtual Gamepad").
     * @param type The type of the device (generic or semantical).
     * @return A handle to the new virtual device, or NULL on failure. The handle must be stored by the plugin.
     */
    SPF_VirtualDevice_Handle* (*Virt_CreateDevice)(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type);

    /**
     * @brief Adds a button to a virtual device.
     * @details This must be called *before* `Virt_Register()`. The `inputName` is used to identify the
     *          button when simulating press/release events.
     *
     * @param h The handle to the virtual device.
     * @param inputName A unique programmatic name for the button (e.g., "button_a", "action1").
     * @param displayName The name shown in the game's UI for binding (e.g., "Button A", "Primary Action").
     */
    void (*Virt_AddButton)(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName);

    /**
     * @brief Adds an axis to a virtual device.
     * @details This must be called *before* `Virt_Register()`. The `inputName` is used to identify the
     *          axis when setting its value.
     *
     * @param h The handle to the virtual device.
     * @param inputName A unique programmatic name for the axis (e.g., "axis_x", "throttle_axis").
     * @param displayName The name shown in the game's UI for binding (e.g., "X Axis", "Throttle").
     */
    void (*Virt_AddAxis)(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName);

    /**
     * @brief Finalizes the device configuration and registers it with the game.
     * @details After this call, the device and its inputs become visible to the game's input system.
     *          No more inputs can be added to the device after it is registered.
     *
     * @param h The handle to the virtual device.
     * @return True on successful registration, false otherwise.
     */
    bool (*Virt_Register)(SPF_VirtualDevice_Handle* h);

    // --- Event Simulation ---

    /**
     * @brief Pushes a button press event to the device's queue.
     * @details The game will process this event on its next input update. The button is considered
     *          held down until `Virt_ReleaseButton()` is called for the same input.
     *
     * @param h The handle to the virtual device.
     * @param inputName The programmatic name of the button to press (must match the name used in `Virt_AddButton`).
     */
    void (*Virt_PressButton)(SPF_VirtualDevice_Handle* h, const char* inputName);

    /**
     * @brief Pushes a button release event to the device's queue.
     * @param h The handle to the virtual device.
     * @param inputName The programmatic name of the button to release.
     */
    void (*Virt_ReleaseButton)(SPF_VirtualDevice_Handle* h, const char* inputName);

    /**
     * @brief Pushes an axis change event to the device's queue.
     * @details The axis will maintain this value until a new value is set.
     *
     * @param h The handle to the virtual device.
     * @param inputName The programmatic name of the axis to change.
     * @param value The new value of the axis, typically between -1.0 and 1.0.
     */
    void (*Virt_SetAxisValue)(SPF_VirtualDevice_Handle* h, const char* inputName, float value);

} SPF_VirtInput_API;

#ifdef __cplusplus
}
#endif