# SPF Logger API

The SPF Logger API provides a centralized and powerful system for logging messages from your plugin. By routing your logs through the framework, you ensure they are consistently timestamped, formatted, and sent to all configured outputs (such as the in-game console and a dedicated log file for your plugin).

## Important: Pre-Formatting Strings

This API does **not** provide a `printf`-style formatting function (e.g., `Log("Value: %d", my_value)`). Passing variadic arguments (`...`) across DLL boundaries is inherently unsafe and can cause crashes.

**You must format all strings in a local buffer *before* calling the `Log` function.**

It is highly recommended to use the `SPF_Formatting_API` for this purpose, as it is also designed to be safe for cross-DLL use.

## Getting the API

The Logger API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnLoad` function.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Logger_API.h"

const SPF_Core_API* s_coreAPI = NULL;
SPF_Logger_Handle* s_myLogger = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
    
    if (s_coreAPI && s_coreAPI->logger) {
        // Get the logger context for our plugin
        s_myLogger = s_coreAPI->logger->GetLogger("MyPlugin");
    }
}
```

## Log Levels (`SPF_LogLevel`)

You can specify a severity level for each message you log.

| Value | Level | Description |
|---|---|---|
| `SPF_LOG_TRACE` | 0 | Fine-grained debugging information. |
| `SPF_LOG_DEBUG` | 1 | Messages useful during development. |
| `SPF_LOG_INFO` | 2 | General informational messages. |
| `SPF_LOG_WARN` | 3 | Warnings about potential issues. |
| `SPF_LOG_ERROR` | 4 | Errors that occurred but don't stop the program. |
| `SPF_LOG_CRITICAL` | 5 | Critical errors that may require shutdown. |

## Function Reference

Functions are accessed via the `logger` member of your `SPF_Core_API` pointer.

---
**`SPF_Logger_Handle* GetLogger(const char* pluginName)`**
Gets a logger handle for your plugin. You should call this once and cache the handle for the lifetime of your plugin.
*   **pluginName:** Your plugin's name, which must match the manifest.

---
**`void Log(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* message)`**
Logs a pre-formatted message.
*   **handle:** The handle obtained from `GetLogger`.
*   **level:** The severity of the message (e.g., `SPF_LOG_INFO`).
*   **message:** The null-terminated string to log.

---
**`void LogThrottled(SPF_Logger_Handle* handle, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message)`**
Logs a message, but only if `throttle_ms` milliseconds have passed since the last time a message with the same `throttle_key` was logged. This is essential for messages in high-frequency code (like `OnUpdate`).
*   **throttle_key:** A unique, persistent string literal to identify this specific log point (e.g., `"myplugin.update.position_log"`).
*   **throttle_ms:** The cooldown period in milliseconds.

---
**`void SetLevel(SPF_Logger_Handle* handle, SPF_LogLevel level)`** and **`SPF_LogLevel GetLevel(SPF_Logger_Handle* handle)`**
Sets or gets the minimum log level for this logger instance. Messages below this level will be ignored. For example, if the level is `SPF_LOG_INFO`, `TRACE` and `DEBUG` messages will not be processed.

## Complete Example

```c
// Assumes s_coreAPI and s_myLogger are initialized as shown above.

void SomeFunction() {
    if (!s_coreAPI || !s_coreAPI->logger || !s_myLogger) return;

    char buffer[256];
    int value = 42;

    // Use the Formatting API to safely prepare the string
    s_coreAPI->formatting->Format(buffer, sizeof(buffer), "The value is: %d", value);
    
    // Log the pre-formatted string
    s_coreAPI->logger->Log(s_myLogger, SPF_LOG_INFO, buffer);
}

// Example for a function that runs every frame
void OnUpdate() {
    if (!s_coreAPI || !s_coreAPI->logger || !s_myLogger) return;

    // This message will only be logged at most once every 5 seconds (5000 ms)
    s_coreAPI->logger->LogThrottled(
        s_myLogger, 
        SPF_LOG_DEBUG, 
        "myplugin.onupdate.tick", // Unique key for this log point
        5000, 
        "OnUpdate is running..."
    );
}
```
