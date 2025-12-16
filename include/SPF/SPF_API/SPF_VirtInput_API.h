#pragma once

/**
 * @file SPF_VirtInput_API.h
 * @brief C-API for interacting with the SPF Virtual Input system.
 * @details This API allows plugins to create virtual input devices (like gamepads or steering wheels)
 *          and simulate button presses and axis movements, which the game will recognize as if
 *          they came from a physical device.
 */

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
 * @struct SPF_Input_API
 * @brief API for creating and controlling virtual input devices.
 *
 * @section Workflow
 * 1.  **Create Device**: In your `OnLoad` or a similar one-time initialization function,
 *     call `CreateDevice()` to get a handle for your new virtual device.
 * 2.  **Add Inputs**: Call `AddButton()` and `AddAxis()` for each input you want your
 *     device to have. Give them unique, programmatic names (e.g., "button_a", "axis_lx")
 *     and user-friendly display names for the UI. This must be done *before* registering.
 * 3.  **Register Device**: Once all inputs are added, call `Register()` to finalize the
 *     device and make it visible to the game. After this point, no more inputs can be added.
 * 4.  **Simulate Events**: During your plugin's update loop (`OnUpdate`), call `PressButton()`,
 *     `ReleaseButton()`, and `SetAxisValue()` to send input events to the game. These events
 *     are queued and processed by the game on its next input tick.
 *
 * @section Example
 * @code{.cpp}
 * // In OnLoad:
 * SPF_Input_API* input_api = core->input;
 * SPF_VirtualDevice_Handle* my_gamepad = input_api->CreateDevice("MyPlugin", "my_gamepad", "My Virtual Gamepad", SPF_INPUT_DEVICE_TYPE_GENERIC);
 * input_api->AddButton(my_gamepad, "honk_button", "Honk");
 * input_api->AddAxis(my_gamepad, "steer_axis", "Steering");
 * input_api->Register(my_gamepad);
 *
 * // In OnUpdate:
 * if (some_condition) {
 *     input_api->PressButton(my_gamepad, "honk_button");
 * } else {
 *     input_api->ReleaseButton(my_gamepad, "honk_button");
 * }
 *
 * float steering_value = get_steering_from_somewhere(); // e.g., from a mobile phone tilt sensor
 * input_api->SetAxisValue(my_gamepad, "steer_axis", steering_value);
 * @endcode
 */
typedef struct SPF_Input_API {
    /**
     * @brief Creates a new virtual input device for the calling plugin.
     *
     * @param pluginName The name of the calling plugin (must match the manifest).
     * @param deviceName A unique internal name for the device (e.g., "my_plugin_gamepad").
     * @param displayName The name shown in the game's UI (e.g., "My Plugin Virtual Gamepad").
     * @param type The type of the device (generic or semantical).
     * @return A handle to the new virtual device, or NULL on failure. The handle must be stored by the plugin.
     */
    SPF_VirtualDevice_Handle* (*CreateDevice)(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type);

    /**
     * @brief Adds a button to a virtual device.
     * @details This must be called *before* `Register()`. The `inputName` is used to identify the
     *          button when simulating press/release events.
     *
     * @param handle The handle to the virtual device.
     * @param inputName A unique programmatic name for the button (e.g., "button_a", "action1").
     * @param displayName The name shown in the game's UI for binding (e.g., "Button A", "Primary Action").
     */
    void (*AddButton)(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName);

    /**
     * @brief Adds an axis to a virtual device.
     * @details This must be called *before* `Register()`. The `inputName` is used to identify the
     *          axis when setting its value.
     *
     * @param handle The handle to the virtual device.
     * @param inputName A unique programmatic name for the axis (e.g., "axis_x", "throttle_axis").
     * @param displayName The name shown in the game's UI for binding (e.g., "X Axis", "Throttle").
     */
    void (*AddAxis)(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName);

    /**
     * @brief Finalizes the device configuration and registers it with the game.
     * @details After this call, the device and its inputs become visible to the game's input system.
     *          No more inputs can be added to the device after it is registered.
     *
     * @param handle The handle to the virtual device.
     * @return True on successful registration, false otherwise.
     */
    bool (*Register)(SPF_VirtualDevice_Handle* handle);

    // --- Event Simulation ---

    /**
     * @brief Pushes a button press event to the device's queue.
     * @details The game will process this event on its next input update. The button is considered
     *          held down until `ReleaseButton()` is called for the same input.
     *
     * @param handle The handle to the virtual device.
     * @param inputName The programmatic name of the button to press (must match the name used in `AddButton`).
     */
    void (*PressButton)(SPF_VirtualDevice_Handle* handle, const char* inputName);

    /**
     * @brief Pushes a button release event to the device's queue.
     * @param handle The handle to the virtual device.
     * @param inputName The programmatic name of the button to release.
     */
    void (*ReleaseButton)(SPF_VirtualDevice_Handle* handle, const char* inputName);

    /**
     * @brief Pushes an axis change event to the device's queue.
     * @details The axis will maintain this value until a new value is set.
     *
     * @param handle The handle to the virtual device.
     * @param inputName The programmatic name of the axis to change.
     * @param value The new value of the axis, typically between -1.0 and 1.0.
     */
    void (*SetAxisValue)(SPF_VirtualDevice_Handle* handle, const char* inputName, float value);

} SPF_Input_API;

#ifdef __cplusplus
}
#endif
