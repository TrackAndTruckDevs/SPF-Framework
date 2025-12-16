#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_GameConsole_API
 * @brief Provides a C-style API for plugins to execute commands in the in-game console.
 *
 * @details This API allows a plugin to programmatically run any command that a user
 *          could type into the game's developer console. It is a one-way interface
 *          for sending commands to the game engine.
 *
 * @section Workflow
 * 1.  **Request Hook**: In your plugin's manifest (`GetManifestData`), you must
 *     add `"GameConsole"` to the `requiredHooks` array and increment
 *     `requiredHooksCount`. The framework will not provide the console API
 *     pointer if this hook is not requested.
 * 2.  **Get the API**: The `SPF_GameConsole_API` pointer is available within the
 *     main `SPF_Core_API` struct provided to your plugin's `OnLoad` function.
 * 3.  **Execute a Command**: Call the `ExecuteCommand` function with the desired
 *     command string at any point in your plugin's logic (e.g., in response to
 *     a button click or a keybind).
 *
 * @section Example
 * @code{.cpp}
 * // In your plugin's UI rendering function:
 * if (ui->Button("Enable Police", 0, 0))
 * {
 *     if (g_coreAPI && g_coreAPI->console)
 *     {
 *         g_coreAPI->console->ExecuteCommand("g_police 1");
 *     }
 * }
 * @endcode
 *
 * @note This API is for **executing** existing commands only. It does not provide
 *       a mechanism for plugins to **register** new custom commands with the console.
 */
typedef struct SPF_GameConsole_API {
    /**
     * @brief Executes a command string in the in-game console.
     * @param command The command string to execute (e.g., "g_traffic 1", "g_set_time 14 30").
     */
    void (*ExecuteCommand)(const char* command);

} SPF_GameConsole_API;

#ifdef __cplusplus
}
#endif
