/**
 * @file SPF_Logger_API.h
 * @brief Defines the C-style API for the logging subsystem.
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
 * A plugin obtains this handle by calling `GetLogger` and then passes it to all
 * subsequent logging calls. This allows the framework to associate log messages
 * with the correct source plugin.
 */
typedef struct SPF_Logger_Handle SPF_Logger_Handle;

/**
 * @enum SPF_LogLevel
 * @brief Defines the severity levels for log messages.
 */
typedef enum {
    SPF_LOG_TRACE = 0,    // For fine-grained debugging information.
    SPF_LOG_DEBUG = 1,    // For messages useful during development.
    SPF_LOG_INFO = 2,     // For general informational messages.
    SPF_LOG_WARN = 3,     // For warnings about potential issues.
    SPF_LOG_ERROR = 4,    // For errors that occurred but don't stop the program.
    SPF_LOG_CRITICAL = 5  // For critical errors that may require shutdown.
} SPF_LogLevel;

// =================================================================================================
// 2. API STRUCTURE DEFINITION
// =================================================================================================

/**
 * @struct SPF_Logger_API
 * @brief C-style API for logging messages from a plugin.
 *
 * @details This API allows a plugin to log messages that will be processed by the
 *          framework's central logging system. This ensures that all plugin
 *          logs are consistently formatted, timestamped, and routed to the
 *          correct output (e.g., console, file).
 *
 * @section Important Note on Formatting
 * This API does **not** provide a printf-style formatting function (like `LogF`).
 * Passing variadic arguments (`...`) across DLL boundaries is inherently unsafe
 * and can lead to stack corruption and crashes, especially when plugins and the
 * framework are built with different compilers or standard library versions.
 *
 * **The plugin developer is responsible for formatting all strings *before*
 * calling the `Log` function.** Use a safe function like `snprintf` or,
 * preferably, the provided `SPF_Formatting_API` to prepare your log
 * messages in a local buffer first.
 *
 * @section Workflow
 * 1.  **Get Logger**: In `OnLoad`, call `GetLogger()` with your plugin's name
 *     to get a handle. Cache this handle for later use.
 * 2.  **Format Message**: Use `snprintf` to format your message into a buffer.
 * 3.  **Log Message**: Call `Log()` with the handle, log level, and the
 *     pre-formatted message.
 *
 * @section Example
 * @code{.cpp}
 * // In OnLoad:
 * g_logger = core->logger->GetLogger("MyPlugin");
 * const SPF_Formatting_API* fmt = core->formatting;
 *
 * // In a function:
 * char buffer[256];
 * int value = 42;
 * fmt->Format(buffer, sizeof(buffer), "The value is: %d", value);
 * core->logger->Log(g_logger, SPF_LOG_INFO, buffer);
 * @endcode
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
    SPF_Logger_Handle* (*GetLogger)(const char* pluginName);

    /**
     * @brief Logs a pre-formatted message string.
     *
     * @param handle The logger handle obtained from GetLogger.
     * @param level The severity level of the message.
     * @param message The message string to log.
     */
    void (*Log)(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* message);

    /**
     * @brief Sets the minimum level for messages to be processed by this logger.
     * Messages with a lower severity will be ignored.
     *
     * @param handle The logger handle.
     * @param level The new minimum log level.
     */
    void (*SetLevel)(SPF_Logger_Handle* handle, SPF_LogLevel level);

    /**
     * @brief Gets the current minimum log level for the logger.
     *
     * @param handle The logger handle.
     * @return The current log level.
     */
    SPF_LogLevel (*GetLevel)(SPF_Logger_Handle* handle);

    /**
     * @brief Logs a pre-formatted message, but only if a certain amount of time
     *        has passed since the last message with the same throttle_key.
     *
     * @param handle The logger handle.
     * @param level The severity level.
     * @param throttle_key A unique, persistent string literal that identifies this
     *                     specific log message. The framework uses this key to track
     *                     when the message was last logged.
     *                     Example: "myplugin.update.position_log"
     * @param throttle_ms The minimum time in milliseconds that must pass before
     *                    this message can be logged again.
     * @param message The pre-formatted string to log.
     */
    void (*LogThrottled)(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message);

} SPF_Logger_API;

// =================================================================================================
// 3. HELPER MACROS (C only)
// =================================================================================================

#ifndef __cplusplus

/**
 * @brief Helper macro for logging a trace message.
 */
#define Trace(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_TRACE, format, ##__VA_ARGS__)

/**
 * @brief Helper macro for logging a debug message.
 */
#define Debug(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)

/**
 * @brief Helper macro for logging an informational message.
 */
#define Info(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

/**
 * @brief Helper macro for logging a warning message.
 */
#define Warn(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_WARN, format, ##__VA_ARGS__)

/**
 * @brief Helper macro for logging an error message.
 */
#define Error(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

/**
 * @brief Helper macro for logging a critical message.
 */
#define Critical(api_ptr, handle_ptr, format, ...) (api_ptr)->LogF(handle_ptr, SPF_LOG_LEVEL_CRITICAL, format, ##__VA_ARGS__)

#endif // __cplusplus

#ifdef __cplusplus
}
#endif
