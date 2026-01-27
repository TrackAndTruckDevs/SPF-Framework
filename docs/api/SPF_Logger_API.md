# SPF Logger API

The **SPF Logger API** provides a standardized way for plugins to log messages to the framework's central logging system. This ensures that logs are consistently formatted, timestamped, and routed to the correct output sinks (e.g., console, file, UI).

**Header:** `include/SPF/SPF_API/SPF_Logger_API.h`

## Key Concepts

1.  **Context-Based**: Logging operations require a context handle (`SPF_Logger_Handle`). This associates the log message with your specific plugin, allowing for per-plugin log levels and file separation.
2.  **No Variadic Arguments**: To ensure binary compatibility and safety across DLL boundaries, the Logger API does **not** support `printf`-style formatting directly. You must format your strings into a buffer *before* calling the log function.
3.  **Throttling**: The API provides a mechanism to limit the frequency of log messages, which is essential for logging inside high-frequency loops (like `OnUpdate`).

## API Reference

### Types

#### `SPF_Logger_Handle`
An opaque pointer representing a logger instance for a specific plugin. Do not access the contents of this struct directly.

#### `SPF_LogLevel`
An enumeration defining the severity of the log message.

| Level | Value | Description |
| :--- | :--- | :--- |
| `SPF_LOG_TRACE` | 0 | Fine-grained debugging information (verbose). |
| `SPF_LOG_DEBUG` | 1 | Messages useful during development. |
| `SPF_LOG_INFO` | 2 | General informational messages. |
| `SPF_LOG_WARN` | 3 | Warnings about potential issues. |
| `SPF_LOG_ERROR` | 4 | Errors that occurred but didn't stop execution. |
| `SPF_LOG_CRITICAL` | 5 | Critical errors requiring immediate attention. |

### Functions

#### `Log_GetContext`
```c
SPF_Logger_Handle* (*Log_GetContext)(const char* pluginName);
```
Gets the logger handle for your plugin. Call this once during initialization (e.g., in `OnLoad`) and cache the result.
*   **pluginName**: The unique name of your plugin (must match the manifest).

#### `Log`
```c
void (*Log)(SPF_Logger_Handle* h, SPF_LogLevel level, const char* message);
```
Logs a pre-formatted message.
*   **h**: The logger handle.
*   **level**: The severity level.
*   **message**: The null-terminated string to log.

#### `Log_SetLevel`
```c
void (*Log_SetLevel)(SPF_Logger_Handle* h, SPF_LogLevel level);
```
Sets the minimum log level for this plugin. Messages below this level will be ignored.
*   **h**: The logger handle.
*   **level**: The new minimum level.

#### `Log_GetLevel`
```c
SPF_LogLevel (*Log_GetLevel)(SPF_Logger_Handle* h);
```
Gets the current minimum log level.

#### `LogThrottled`
```c
void (*LogThrottled)(SPF_Logger_Handle* h, SPF_LogLevel level, const char* throttle_key, uint32_t throttle_ms, const char* message);
```
Logs a message only if a certain amount of time has passed since the last log with the same `throttle_key`.
*   **throttle_key**: A unique string ID for this log message (e.g., "myplugin.update.error").
*   **throttle_ms**: Minimum time in milliseconds between logs.

## Usage Example

```cpp
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"

// Global handles
static SPF_Logger_Handle* g_hLog = nullptr;
static const SPF_Formatting_API* g_fmt = nullptr;

// 1. Initialization (in OnLoad)
void OnLoad(const SPF_Load_API* api) {
    // Get the logger context
    g_hLog = api->logger->Log_GetContext("MyPlugin");
    
    // Cache the formatting API for convenience
    g_fmt = api->formatting;
    
    // Log a simple string
    api->logger->Log(g_hLog, SPF_LOG_INFO, "MyPlugin loaded successfully!");
}

// 2. Logging with Formatting
void LogSomeData(int value, float speed) {
    if (!g_hLog || !g_fmt) return;

    char buffer[256];
    
    // Use the framework's safe formatting API
    g_fmt->Fmt_Format(buffer, sizeof(buffer), "Value: %d, Speed: %.2f", value, speed);
    
    // Pass the formatted buffer to the logger
    // Note: The function is named 'Log', not 'Log_Log'
    g_loadApi->logger->Log(g_hLog, SPF_LOG_INFO, buffer);
}

// 3. Throttled Logging (in OnUpdate)
void OnUpdate() {
    // This will appear in the log at most once every 1000ms (1 second)
    g_loadApi->logger->LogThrottled(
        g_hLog, 
        SPF_LOG_WARN, 
        "myplugin.update_warning", // Unique Key
        1000,                      // Throttle Time
        "This is a frequent warning!"
    );
}
```