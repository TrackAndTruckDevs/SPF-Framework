/**
 * @file SPF_GameConsole_API.h
 * @brief API for programmatically executing commands in the in-game developer console.
 *
 * @details This API provides a bridge between your plugin and the game engine's
 * built-in command processor. It allows you to run any command that a user could
 * manually type into the developer console (e.g., 'g_traffic', 'g_set_time').
 *
 * This is a one-way interface designed for automation and integration of existing
 * game features into your plugin's UI or logic.
 *
 * ================================================================================================
 * KEY CONCEPTS
 * ================================================================================================
 *
 * 1. **Mandatory Hook**: To use this API, your plugin MUST declare a dependency on the
 *    'GameConsole' hook in its manifest using 'api->Policy_AddRequiredHook(handle, "GameConsole")'.
 *
 * 2. **One-Way Execution**: This API only sends commands to the game. It does not
 *    receive feedback or return values from the console.
 *
 * 3. **No Command Registration**: This API is for execution only. It cannot be used to
 *    register new, custom console commands.
 *
 * ================================================================================================
 * USAGE EXAMPLE (C++)
 * ================================================================================================
 * @code
 * // In response to a UI button click:
 * if (ui->Button("Fast Forward Time", 0, 0)) {
 *     // Skip to afternoon
 *     api->GCon_ExecuteCommand("g_set_time 14 00");
 * }
 * @endcode
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_GameConsole_API
 * @brief Table of function pointers for interacting with the game's console.
 */
typedef struct SPF_GameConsole_API {
  /**
   * @brief Executes a command string in the in-game console.
   * @details The command is processed immediately by the game engine.
   *          Ensure the console system is active and the hook is requested.
   * @param command The command string to execute (e.g., "g_traffic 1", "quit").
   */
  void (*GCon_ExecuteCommand)(const char* command);

  /**
   * @brief Returns the total number of console commands the game supports.
   * @details The list is discovered once (one-time pattern scan of the game's
   *          three command sources: console commands, script commands and input
   *          constants) and cached; subsequent calls return the cached count.
   * @return Number of available commands, or 0 if discovery failed.
   */
  int (*GCon_GetCommandCount)(void);

  /**
   * @brief Copies the command name at 'index' into 'out_buffer'.
   * @details Use after GCon_GetCommandCount to iterate the full command list
   *          (e.g. for building an autocomplete UI in your plugin).
   * @param index Zero-based command index (0 .. GCon_GetCommandCount()-1).
   * @param out_buffer Destination buffer for the null-terminated command name.
   * @param buffer_size Size of 'out_buffer' in bytes. The name is truncated if needed.
   * @return 1 on success, 0 on failure (NULL buffer, buffer_size <= 0, or index out of range).
   */
  int (*GCon_GetCommandNameByIndex)(int index, char* out_buffer, int buffer_size);

} SPF_GameConsole_API;

#ifdef __cplusplus
}
#endif