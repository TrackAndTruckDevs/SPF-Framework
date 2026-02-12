/**                                                                                               
* @file SPF_KeyBinds_API.h                                                                          
* @brief API for registering logical actions and linking them to user-defined keybinds.
*                                                                                                 
* @details This API allows plugins to define abstract "actions" (like 'toggle_menu' 
* or 'fire_laser') and bind them to physical inputs (keyboard, gamepad, joystick) 
* defined in the manifest. 
* 
* The system decoupling allows users to fully rebind any action via the framework's 
* UI without any changes to the plugin's code.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Logical Actions**: You register callbacks for action NAMES, not keys. The 
*    actual key assignment is handled by the framework and stored in the config.
*                                                                                                 
* 2. **Dot Notation**: Action names are formed as 'GroupName.ActionName' (e.g., 
*    'MyPlugin.UI.Toggle'). These must match the names declared in your manifest.
*                                                                                                 
* 3. **Automatic Cleanup**: Keybind handles are managed by the framework. All 
*    registrations are automatically cleaned up when the plugin is unloaded.
* 
* 4. **Dynamic Blocking**: If an action's 'consume' policy is set to 'manual', 
*    the plugin can programmatically block the physical key from the game 
*    using 'Kbind_SetBlockState()'.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* void MyActionCallback() {
*     Log("Keybind triggered!");
* }
*
* void OnActivated(const SPF_Core_API* api) {
*     SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
*     api->keybinds->Kbind_Register(h, "MyPlugin.General.DoWork", MyActionCallback);
*
*     // Check how many bindings are assigned and their properties
*     int count = api->keybinds->Kbind_GetBindingCount(h, "MyPlugin.General.DoWork");
*     for (int i = 0; i < count; i++) {
*         SPF_BindingType type = api->keybinds->Kbind_GetBindingType(h, "MyPlugin.General.DoWork", i);
*         SPF_ActivationBehavior behavior = api->keybinds->Kbind_GetBindingBehavior(h, "MyPlugin.General.DoWork", i);
*         
*         if (type == SPF_BINDING_KEYBOARD && behavior == SPF_BEHAVIOR_TOGGLE) {
*             Log("Action is currently bound to a keyboard key in Toggle mode.");
*         }
*     }
* }
* @endcode                                                                                        
*/ 

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum SPF_BindingType
 * @brief Identifies the physical source of an input binding.
 * @details Strictly matches the framework's internal 'InputType'.
 */
typedef enum {
    SPF_BINDING_UNKNOWN = 0,
    SPF_BINDING_KEYBOARD,
    SPF_BINDING_GAMEPAD,       /**< Digital gamepad buttons. */
    SPF_BINDING_MOUSE,         /**< Digital mouse buttons. */
    SPF_BINDING_JOYSTICK,      /**< Digital joystick buttons. */
    SPF_BINDING_CHORD,         /**< Key combinations (e.g., Ctrl+S). */
    SPF_BINDING_GAMEPAD_AXIS,  /**< Analog gamepad sticks or triggers. */
    SPF_BINDING_MOUSE_AXIS,    /**< Mouse movement or scroll wheel. */
    SPF_BINDING_JOYSTICK_AXIS  /**< Analog joystick/flight stick axes. */
} SPF_BindingType;

/**
 * @enum SPF_ActivationBehavior
 * @brief Describes how a digital action responds to user interaction.
 */
typedef enum {
    SPF_BEHAVIOR_HOLD = 0,
    SPF_BEHAVIOR_TOGGLE,
    SPF_BEHAVIOR_NA             /**< Not applicable (e.g., for analog axes). */
} SPF_ActivationBehavior;

/**
 * @enum SPF_PressType
 * @brief Distinguishes between short and long presses.
 */
typedef enum {
    SPF_PRESS_SHORT = 0,
    SPF_PRESS_LONG,
    SPF_PRESS_NA
} SPF_PressType;

/**
 * @enum SPF_InputMode
 * @brief Defines whether an axis is treated as a continuous range or a discrete button.
 */
typedef enum {
    SPF_MODE_ANALOG = 0,        /**< Axis returns smooth values (e.g., 0.0 to 1.0). */
    SPF_MODE_DIGITAL,           /**< Axis acts like a button, triggering at a threshold. */
    SPF_MODE_NA                 /**< Not applicable (e.g., for standard digital keys). */
} SPF_InputMode;

/**
 * @enum SPF_AxisSide
 * @brief For analog axes, defines which half of the range is used.
 */
typedef enum {
    SPF_SIDE_POSITIVE = 0,      /**< Only positive values (0.0 to 1.0). */
    SPF_SIDE_NEGATIVE,          /**< Only negative values (0.0 to -1.0). */
    SPF_SIDE_BOTH,              /**< Full range used (-1.0 to 1.0). */
    SPF_SIDE_NA                 /**< Not applicable (e.g., for standard digital keys). */
} SPF_AxisSide;

/**
 * @enum SPF_AccumulatorMode
 * @brief Defines whether axis input is absolute or cumulative.
 */
typedef enum {
    SPF_ACCUMULATOR_OFF = 0,    /**< Axis returns absolute position. */
    SPF_ACCUMULATOR_ON,         /**< Axis accumulates delta changes (Virtual Knob). */
    SPF_ACCUMULATOR_NA          /**< Not applicable (e.g., for digital buttons). */
} SPF_AccumulatorMode;

// Forward-declare the handle type to make it an opaque pointer for the C API
typedef struct SPF_KeyBinds_Handle SPF_KeyBinds_Handle;

/**
 * @struct SPF_KeyBinds_API
 * @brief API for registering callbacks for named actions that can be triggered by user-defined keybinds.
 *
 * @section Core Concepts
 * The keybind system is built around a simple and flexible mechanism for defining abstract "actions"
 * which are separated from their physical key assignments.
 *
 * 1.  **Action**: A named, logical operation. The final, full action name is a single string
 *     created by joining the `groupName` and `actionName` from the manifest with a period (`.`).
 *     Example: `groupName` = "MyPlugin.MainWindow", `actionName` = "toggle" -> Full Name = "MyPlugin.MainWindow.toggle".
 *
 * 2.  **Keybind**: A specific keyboard or gamepad input configuration that triggers an action.
 *     Default keybinds are defined in the manifest, but the user can always override
 *     them in the framework's settings UI.
 *
 * 3.  **Callback**: A C/C++ function within your plugin's code that gets executed when its
 *     registered full action name is triggered.
 *
 * @section Workflow
 * 1.  **Declare in Manifest**: In your `GetManifestData` function, populate the `SPF_KeybindsData_C`
 *     structure. For each action, you must provide:
 *     - `groupName`: A string to identify the action's group. It can be anything, but following a
 *       convention is recommended for clarity.
 *     - `actionName`: A string that describes the specific action, typically a verb.
 *     - One or more default `SPF_KeybindDefinition_C` structs for the key(s).
 *
 * 2.  **Register Callback**: In your `OnActivated` function, call `Kbind_Register()`, passing the **full action name**
 *     (i.e., the concatenated "groupName.actionName") and a pointer to your callback function.
 *     The framework will not find the action if this name does not exactly match.
 *
 * @section Naming Conventions (Best Practices)
 * While the system is flexible, following a convention is highly recommended.
 *
 * - **`actionName`**: Use a simple verb that describes what the action does.
 *   - Good: "toggle", "log_message", "increase_speed"
 *
 * - **`groupName`**: Use a structure that provides context and avoids collisions.
 *   - Good: `{PluginName}.{Noun}` (e.g., "MyPlugin.MainWindow")
 *   - Better (for complex plugins): `{PluginName}.{Category}.{Noun}` (e.g., "MyPlugin.UI.MainWindow")
 *
 * Combining these gives a clear, fully-qualified action name like "MyPlugin.UI.MainWindow.toggle".
 * 
 * @section UsageExample Usage Example (C++)
 * @code                                                                                           
 * void MyActionCallback() {
 *     Log("Keybind triggered!");
 * }
 *
 * void OnActivated(const SPF_Core_API* api) {
 *     SPF_KeyBinds_Handle* h = api->keybinds->Kbind_GetContext("MyPlugin");
 *     api->keybinds->Kbind_Register(h, "MyPlugin.General.DoWork", MyActionCallback);
 *
 *     // Inspecting binding properties
 *     int count = api->keybinds->Kbind_GetBindingCount(h, "MyPlugin.General.DoWork");
 *     for (int i = 0; i < count; i++) {
 *         SPF_BindingType type = api->keybinds->Kbind_GetBindingType(h, "MyPlugin.General.DoWork", i);
 *         if (type == SPF_BINDING_KEYBOARD) {
 *             char name[64];
 *             api->keybinds->Kbind_GetBindingName(h, "MyPlugin.General.DoWork", i, name, sizeof(name));
 *             // result could be "KEY_F5"
 *         }
 *     }
 * }
 * @endcode
 */
typedef struct SPF_KeyBinds_API {
    /**
     * @brief Gets a keybinds context handle for the plugin.
     *
     * @details This handle is the primary identifier for all subsequent keybind
     *          calls. The framework uses it to associate the calls with the correct
     *          plugin. The framework manages the memory of this handle; do not free it.
     *
     * @param pluginName The name of the plugin, which MUST match the `name`
     *                   field in your plugin's manifest.
     * @return A handle to the keybinds context, or `NULL` if the plugin
     *         could not be found.
     */
    SPF_KeyBinds_Handle* (*Kbind_GetContext)(const char* pluginName);

    /**
     * @brief Registers a callback function for a specific action defined in the manifest.
     *
     * @details This function creates the runtime link between the abstract action
     *          (e.g., "toggle_window") and the C++ code that should execute when
     *          that action is triggered by its assigned keybind.
     *
     * @param h The context handle obtained from `Kbind_GetContext`.
     * @param actionName The **full name** of the action, formed by joining the `groupName` and `actionName`
     *                   from the manifest with a period (e.g., "MyPlugin.MainWindow.toggle"). This
     *                   MUST exactly match the intended full action name.
     * @param callback The function pointer to be called when the action is triggered.
     *                 The callback function must have a `void(void)` signature.
     */
    void (*Kbind_Register)(SPF_KeyBinds_Handle* h, const char* actionName, void (*callback)(void));

    /**
     * @brief Unregisters all actions and callbacks associated with the handle.
     *
     * @details This is typically called automatically by the framework when a plugin
     *          is unloaded. You only need to call this manually if you want to
     *          disable all keybind handling for your plugin mid-session without
     *          a full unload/reload cycle.
     *
     * @param h The context handle.
     */
    void (*Kbind_UnregisterAll)(SPF_KeyBinds_Handle* h);

    /**
     * @brief Programmatically controls whether an action's physical input is blocked from the game.
     * 
     * @details This function is only effective if the action's 'consume' policy is set to 'manual' 
     *          in the framework settings. It allows the plugin to decide at runtime when a 
     *          shared key (e.g., WASD) should be captured by the plugin or passed to the game.
     * 
     * @param h The context handle.
     * @param actionName The **full name** of the action (e.g., "MyPlugin.Movement.Forward").
     * @param block If true, the framework will consume the input and block it from the game. 
     *              If false, the input will be passed through to the game.
     */
    void (*Kbind_SetBlockState)(SPF_KeyBinds_Handle* h, const char* actionName, bool block);

    /**
     * @brief Gets the current value of the input bound to the specified action.
     * 
     * @details This function provides a unified way to read both digital and analog inputs. 
     *          The returned float is processed through the framework's input pipeline 
     *          (including deadzones, curves, and normalization).
     * 
     *          ### IMPORTANT FOR DIGITAL ACTIONS:
     *          For actions bound to buttons (keyboard, gamepad buttons), this method returns 
     *          the **immediate physical state** (1.0 = pressed, 0.0 = released). 
     *          It **ignores** logical behaviors such as 'toggle', 'hold', or 'press_type' (short/long).
     *          If you need to react to these logical events, use `Kbind_Register` instead.
     * 
     *          ### Expected Value Ranges:
     *          - **Digital Buttons (Keyboard/Gamepad):** Returns either `0.0` (released) or `1.0` (pressed).
     *          - **Analog Triggers:** Returns `0.0` to `1.0`.
     *          - **Analog Sticks (Standard):** 
     *              - If Side is set to **"Both"**: Returns `-1.0` to `1.0`.
     *              - If Side is **"Positive"** or **"Negative"**: Returns `0.0` to `1.0` representing 
     *                the absolute activation level in that direction.
     *          - **Accumulator Mode (Knobs/Mouse Wheel):** Returns the current persistent state 
     *            of the virtual controller. By default, this is clamped to `[-1.0, 1.0]`, 
     *            but can be customized by the user. 
     * 
     *          **Normalization Logic for Sides:**
     *          To simplify plugin logic, when an action is mapped to a specific side of an axis, 
     *          the framework treats it as a 0..1 scale.
     *          - Bound to **"Positive Side"**: Physical `0.5` -> Returns `0.5`; Physical `-0.5` -> Returns `0.0`.
     *          - Bound to **"Negative Side"**: Physical `-0.8` -> Returns `0.8` (absolute magnitude).
     *          - Bound to **"Both"**: Physical `-0.8` -> Returns `-0.8` (full range preserved).
     * 
     * @param h The context handle.
     * @param actionName The **full name** of the action (e.g., "MyPlugin.Controls.Throttle").
     * @return The current processed value of the action.
     */
    float (*Kbind_GetActionValue)(SPF_KeyBinds_Handle* h, const char* actionName);

    /**
     * @brief Returns the number of physical bindings (keys/axes) assigned to a logical action.
     * 
     * @details A single logical action can be triggered by multiple independent physical inputs
     *          simultaneously (e.g., 'Space' on keyboard AND 'Button A' on gamepad).
     *          This function allows you to determine how many such bindings exist so you can
     *          iterate through them to inspect their individual properties.
     *                                                                                                  
     * @param h The context handle obtained from `Kbind_GetContext`.
     * @param actionName The full name of the action (e.g., "MyPlugin.General.Jump"). 
     * @return The number of physical bindings, or 0 if the action is not found or has no keys assigned.
     */
    int (*Kbind_GetBindingCount)(SPF_KeyBinds_Handle* h, const char* actionName);

    /**
     * @brief Gets the physical source type of a specific binding.
     * 
     * @details Use this to identify what kind of hardware the user has assigned to the action.
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return One of the following SPF_BindingType values:
     *         - SPF_BINDING_KEYBOARD: A standard keyboard key.
     *         - SPF_BINDING_GAMEPAD: A digital button on a gamepad.
     *         - SPF_BINDING_MOUSE: A digital mouse button.
     *         - SPF_BINDING_JOYSTICK: A digital button on.
     *         - SPF_BINDING_CHORD: A combination of multiple keys (e.g., Shift+G).
     *         - SPF_BINDING_GAMEPAD_AXIS: An analog stick or trigger.
     *         - SPF_BINDING_MOUSE_AXIS: Mouse wheel or cursor movement.
     *         - SPF_BINDING_JOYSTICK_AXIS: Analog axes on a joystick.
     *         - SPF_BINDING_UNKNOWN: If the index is out of bounds or the action doesn't exist.
     */
    SPF_BindingType (*Kbind_GetBindingType)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

    /**
     * @brief Gets the logical activation behavior (Hold/Toggle) for a specific binding.
     * 
     * @details This setting defines how the framework triggers the action's callback.
     * 
     *          ### Applicability:
     *          - **Digital Inputs:** Always applicable for `SPF_BINDING_KEYBOARD`, `SPF_BINDING_GAMEPAD`, 
     *            `SPF_BINDING_MOUSE`, `SPF_BINDING_JOYSTICK`, and `SPF_BINDING_CHORD`.
     *          - **Analog Axes:** Applicable **ONLY** if the axis is in `SPF_MODE_DIGITAL`.
     * 
     *          ### Behavior:
     *          - Returns `SPF_BEHAVIOR_HOLD` or `SPF_BEHAVIOR_TOGGLE` for applicable types.
     *          - Returns `SPF_BEHAVIOR_NA` for axes in `SPF_MODE_ANALOG`.
     * 
     *          ### IMPORTANT:
     *          This setting **ONLY** affects the timing and frequency of calls to the function 
     *          registered via `Kbind_Register`. It does **not** affect the value returned by 
     *          `Kbind_GetActionValue`, which always provides immediate physical state for buttons.
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return SPF_BEHAVIOR_HOLD or SPF_BEHAVIOR_TOGGLE. Returns SPF_BEHAVIOR_NA for axes.
     */
    SPF_ActivationBehavior (*Kbind_GetBindingBehavior)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

    /**
     * @brief Gets the physical press type (Short/Long) required to trigger this binding.
     * 
     * @details This setting defines whether the framework waits for a release or a timer 
     *          before triggering the action's callback.
     * 
     *          ### Applicability:
     *          - **Digital Inputs:** Always applicable for `SPF_BINDING_KEYBOARD`, `SPF_BINDING_GAMEPAD`, 
     *            `SPF_BINDING_MOUSE`, `SPF_BINDING_JOYSTICK`, and `SPF_BINDING_CHORD`.
     *          - **Analog Axes:** Applicable **ONLY** if the axis is in `SPF_MODE_DIGITAL`.
     * 
     *          ### Behavior:
     *          - Returns `SPF_PRESS_SHORT` or `SPF_PRESS_LONG` for applicable types.
     *          - Returns `SPF_PRESS_NA` for axes in `SPF_MODE_ANALOG`.
     * 
     *          ### IMPORTANT:
     *          This setting **ONLY** affects the timing of calls to the function registered 
     *          via `Kbind_Register`. It does **not** affect the value returned by 
     *          `Kbind_GetActionValue`.
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return SPF_PRESS_SHORT or SPF_PRESS_LONG. Returns SPF_PRESS_NA for axes
     */
    SPF_PressType (*Kbind_GetBindingPressType)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

    /**
     * @brief Gets the accumulator mode (absolute vs cumulative) for an axis-based binding.
     * 
     * @details In accumulator mode (Virtual Knob), the input value persists and increments/decrements 
     *          with physical movement (like a mouse wheel or a relative throttle).
     * 
     *          ### Applicability:
     *          - **Analog Axes:** Applicable for `SPF_BINDING_GAMEPAD_AXIS`, `SPF_BINDING_MOUSE_AXIS`, 
     *            and `SPF_BINDING_JOYSTICK_AXIS`. 
     *          - **Digital Inputs:** Always returns `SPF_ACCUMULATOR_NA`.
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return SPF_ACCUMULATOR_ON, SPF_ACCUMULATOR_OFF, or SPF_ACCUMULATOR_NA.
     */
    SPF_AccumulatorMode (*Kbind_GetBindingAccumulatorMode)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

    /**
     * @brief Gets the human-readable display name of the physical input (e.g., "Space", "Shift").
     * 
     * @details This function returns the formatted name used in the framework's UI. 
     *          It automatically handles device-specific naming (e.g., "Cross" for PlayStation 
     *          vs "A" for Xbox controllers).
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @param out_buffer Buffer to store the resulting string.
     * @param buffer_size Size of the output buffer in bytes.
     * @return The number of characters written.
     */
    int (*Kbind_GetBindingName)(SPF_KeyBinds_Handle* h, const char* actionName, int index, char* out_buffer, int buffer_size);

    /**
     * @brief Gets the input mode (Analog/Digital) for an axis-based binding.
     * 
     * @details Defines whether the framework treats the physical axis as a continuous range 
     *          of values or as a discrete button trigger.
     * 
     *          ### Applicability:
     *          - **Analog Axes:** Applicable for `SPF_BINDING_GAMEPAD_AXIS`, `SPF_BINDING_MOUSE_AXIS`, 
     *            and `SPF_BINDING_JOYSTICK_AXIS`.
     *          - **Digital Inputs:** Always returns `SPF_MODE_NA`.
     * 
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return SPF_MODE_ANALOG, SPF_MODE_DIGITAL, or SPF_MODE_NA.
     */
    SPF_InputMode (*Kbind_GetBindingMode)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

    /**
     * @brief For axis-based bindings, identifies which side of the axis range is being monitored.
     * 
     * @details Defines whether the framework listens to the full range of the axis 
     *          or only one specific side (positive or negative).
     * 
     *          ### Applicability:
     *          - **Analog Axes:** Applicable for `SPF_BINDING_GAMEPAD_AXIS`, `SPF_BINDING_MOUSE_AXIS`, 
     *            and `SPF_BINDING_JOYSTICK_AXIS`.
     *          - **Digital Inputs:** Always returns `SPF_SIDE_NA`.
     * 
     * @param h The context handle.
     * @param actionName The full name of the action.
     * @param index The zero-based index of the binding.
     * @return SPF_SIDE_POSITIVE, SPF_SIDE_NEGATIVE, SPF_SIDE_BOTH, or SPF_SIDE_NA.
     */
    SPF_AxisSide (*Kbind_GetBindingSide)(SPF_KeyBinds_Handle* h, const char* actionName, int index);

} SPF_KeyBinds_API;

#ifdef __cplusplus
}
#endif
