/**
 * @file SPF_Logger_API.h
 * @brief Defines the C-style API for the logging subsystem.
 *
 * This API allows plugins to log messages that will be processed by the
 * framework's central logging system. This ensures that all plugin
 * logs are consistently formatted, timestamped, and routed to the
 * correct output (e.g., console, file, UI).
 *
 * @section Important Note on Formatting
 * This API does **not** provide a printf-style formatting function.
 * Passing variadic arguments (`...`) across DLL boundaries is inherently unsafe
 * and can lead to stack corruption and crashes, especially when plugins and the
 * framework are built with different compilers or standard library versions.
 *
 * **The plugin developer is responsible for formatting all strings *before*
 * calling the `Log` function.** Use `snprintf` or, preferably, the provided
 * `SPF_Formatting_API` to prepare your log messages in a local buffer first.
 *
 * @section Workflow
 * 1.  **Get Context**: In `OnLoad`, call `Log_GetContext()` with your plugin's name
 *     to get a handle. Cache this handle for later use (e.g., in a static variable).
 * 2.  **Format Message**: Use `SPF_Formatting_API::Fmt_Format` to format your message into a buffer.
 * 3.  **Log Message**: Call `Log()` with the handle, log level, and the pre-formatted message.
 *
 * @section Usage Example
 * @code{.c}
 * // Global or static variables to hold API pointers and handles
 * static SPF_Logger_Handle* s_hLog = NULL;
 * static const SPF_Formatting_API* s_fmt = NULL;
 *
 * // In your plugin's OnLoad function:
 * SPF_BOOL OnLoad(SPF_Load_API* api) {
 *     // Get the logger context for this plugin
 *     s_hLog = api->logger->Log_GetContext("MyPlugin");
 *     s_fmt = api->formatting;
 *     return TRUE;
 * }
 *
 * // Elsewhere in your code:
 * void ProcessData(int value) {
 *     char buffer[256];
 *     
 *     // Format the string safely using the Framework's formatting API
 *     // Note: Use Fmt_Format, NOT just Format
 *     s_fmt->Fmt_Format(buffer, sizeof(buffer), "Processing data value: %d", value);
 *     
 *     // Send to the logger
 *     // s_log is the SPF_Logger_API* pointer usually available from OnLoad
 *     // s_hLog is the handle we obtained earlier
 *     s_loadApi->logger->Log(s_hLog, SPF_LOG_INFO, buffer);
 * }
 * @endcode
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// 1. FORWARD DECLARATIONS & ENUMS
// =================================================================================================

/**
 * @brief An opaque handle representing a specific logger instance.
 *
 * A plugin obtains this handle by calling `Log_GetContext` and then passes it to all
 * subsequent logging calls. This allows the framework to associate log messages
 * with the correct source plugin and apply plugin-specific settings (like log levels).
 */
typedef struct SPF_Logger_Handle SPF_Logger_Handle;

/**
 * @enum SPF_LogLevel
 * @brief Defines the severity levels for log messages.
 */
typedef enum {
    SPF_LOG_TRACE = 0,    // For fine-grained debugging information (verbose).
    SPF_LOG_DEBUG = 1,    // For messages useful during development and debugging.
    SPF_LOG_INFO = 2,     // For general informational messages about system state.
    SPF_LOG_WARN = 3,     // For warnings about potential issues that differ from expected behavior.
    SPF_LOG_ERROR = 4,    // For errors that occurred but don't stop the program execution.
    SPF_LOG_CRITICAL = 5  // For critical errors that may require immediate attention or shutdown.
} SPF_LogLevel;

// =================================================================================================
// 2. API STRUCTURE DEFINITION
// =================================================================================================

/**
 * @struct SPF_Logger_API
 * @brief Table of functions for interacting with the logging system.
 */
typedef struct SPF_Logger_API {
    /**
     * @brief Gets a logger handle for the calling plugin.
     *
     * This function should be called once during plugin initialization (e.g., in OnLoad).
     * The returned handle is valid for the entire lifetime of the plugin and should be stored.
     *
     * @param pluginName The unique name of the plugin (e.g., "MyPlugin"). This should match
     *                   the name used in the plugin's manifest and directory structure.
     * @return A handle to the logger instance. Returns NULL on failure.
     */
    SPF_Logger_Handle* (*Log_GetContext)(const char* pluginName);

    /**
     * @brief Logs a pre-formatted message string.
     *
     * This is the primary function for sending log messages to the framework.
     * The message will be routed to all active sinks (file, console, UI) based on
     * the current log level configuration.
     *
     * @param h The logger handle obtained from Log_GetContext.
     * @param level The severity level of the message.
     * @param message The null-terminated message string to log.
     */
    void (*Log)(SPF_Logger_Handle* h, SPF_LogLevel level, const char* message);

    /**
     * @brief Sets the minimum level for messages to be processed by this logger.
     * 
     * Messages with a severity lower than the specified level will be ignored.
     * For example, setting the level to SPF_LOG_INFO will filter out TRACE and DEBUG messages.
     *
     * @param h The logger handle.
     * @param level The new minimum log level.
     */
    void (*Log_SetLevel)(SPF_Logger_Handle* h, SPF_LogLevel level);

    /**
     * @brief Gets the current minimum log level for the logger.
     *
     * @param h The logger handle.
     * @return The current log level.
     */
    SPF_LogLevel (*Log_GetLevel)(SPF_Logger_Handle* h);

    /**
     * @brief Logs a pre-formatted message, but only if a certain amount of time
     *        has passed since the last message with the same throttle_key.
     *
     * This is useful for logging inside high-frequency loops (like OnUpdate) without
     * flooding the log file.
     *
     * @param h The logger handle.
     * @param level The severity level.
     * @param throttle_key A unique, persistent string literal that identifies this
     *                     specific log message. The framework uses this key to track
     *                     when the message was last logged.
     *                     Example: "myplugin.update.position_log"
     * @param throttle_ms The minimum time in milliseconds that must pass before
     *                    this message can be logged again.
     * @param message The pre-formatted string to log.
     */
    void (*LogThrottled)(SPF_Logger_Handle* h, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message);

} SPF_Logger_API;

#ifdef __cplusplus
}
#endif
