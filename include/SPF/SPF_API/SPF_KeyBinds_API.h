#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
 * 2.  **Register Callback**: In your `OnLoad` function, call `Register()`, passing the **full action name**
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
    SPF_KeyBinds_Handle* (*GetContext)(const char* pluginName);

    /**
     * @brief Registers a callback function for a specific action defined in the manifest.
     *
     * @details This function creates the runtime link between the abstract action
     *          (e.g., "toggle_window") and the C++ code that should execute when
     *          that action is triggered by its assigned keybind.
     *
     * @param handle The context handle obtained from `GetContext`.
     * @param actionName The **full name** of the action, formed by joining the `groupName` and `actionName`
     *                   from the manifest with a period (e.g., "MyPlugin.MainWindow.toggle"). This
     *                   MUST exactly match the intended full action name.
     * @param callback The function pointer to be called when the action is triggered.
     *                 The callback function must have a `void(void)` signature.
     */
    void (*Register)(SPF_KeyBinds_Handle* handle, const char* actionName, void (*callback)(void));

    /**
     * @brief Unregisters all actions and callbacks associated with the handle.
     *
     * @details This is typically called automatically by the framework when a plugin
     *          is unloaded. You only need to call this manually if you want to
     *          disable all keybind handling for your plugin mid-session without
     *          a full unload/reload cycle.
     *
     * @param handle The context handle.
     */
    void (*UnregisterAll)(SPF_KeyBinds_Handle* handle);

} SPF_KeyBinds_API;

#ifdef __cplusplus
}
#endif
