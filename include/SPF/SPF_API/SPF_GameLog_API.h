#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file SPF_GameLog_API.h
 * @brief C-API for subscribing to game log events.
 */

/**
 * @brief A callback function pointer type for receiving game log lines.
 *
 * @param log_line The raw, formatted log line from the game.
 * @param user_data A pointer to user-defined data, passed during registration.
 */
typedef void (*SPF_GameLog_Callback)(const char* log_line, void* user_data);

/**
 * @brief Opaque handle for a registered GameLog callback.
 *
 * Plugins should store this handle and ensure its lifetime matches the desired
 * registration period. When the handle is destroyed, the callback is unregistered.
 */
typedef void* SPF_GameLog_Callback_Handle;

/**
 * @struct SPF_GameLog_API
 * @brief API for subscribing to game log events.
 *
 * @section Workflow
 * 1.  Implement a callback function in your plugin that matches the
 *     `SPF_GameLog_Callback` signature.
 * 2.  In your plugin's `OnLoad` function, call `RegisterCallback`, passing
 *     a pointer to your callback function.
 * 3.  Your callback will now be invoked every time the game writes a new
 *     line to its log.
 *
 * @section Example
 * @code{.cpp}
 * // 1. Implement the callback
 * void MyLogCallback(const char* log_line, void* user_data) {
 *     if (strstr(log_line, "driver hired")) {
 *         // A new driver was hired, do something!
 *     }
 * }
 *
 * // 2. Register it in OnLoad
 * void OnLoad(const SPF_Core_API* core) {
 *     // Store the handle to ensure the callback remains registered.
 *     // The handle will be automatically cleaned up by the framework
 *     // when the plugin unloads.
 *     SPF_GameLog_Callback_Handle my_handle = core->gamelog->RegisterCallback(MyLogCallback, nullptr);
 * }
 * @endcode
 */
typedef struct SPF_GameLog_API {
    /**
     * @brief Registers a callback function to receive game log lines.
     *
     * @param callback A function pointer that will be called for each log line.
     * @param user_data An optional, user-defined pointer that will be passed
     *                  to the callback function.
     * @return An opaque handle representing the registered callback. This handle
     *         must be stored by the plugin. When the handle is destroyed (e.g.,
     *         by the framework when the plugin unloads), the callback is
     *         automatically unregistered.
     */
    SPF_GameLog_Callback_Handle (*RegisterCallback)(const char* pluginName, SPF_GameLog_Callback callback, void* user_data);

} SPF_GameLog_API;

#ifdef __cplusplus
}
#endif
