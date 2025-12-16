# SPF Game Log API

The SPF Game Log API provides a powerful event-based mechanism for your plugin to "listen" to the game's log (`game.log.txt`) in real-time.

## Why Use It?

Many in-game events do not have dedicated telemetry data channels but are written as plain text to the game's log. By subscribing to these log lines, your plugin can react to a much wider range of events, such as:
*   Hiring or firing a driver.
*   Discovering a new city.
*   Game saving events.
*   And many more.

## Workflow

1.  **Implement a Callback:** Create a function in your plugin that matches the `SPF_GameLog_Callback` signature. This function will be your event handler.
2.  **Register the Callback:** In your plugin's `OnLoad` function, call `RegisterCallback`, passing a pointer to your handler function.
3.  **Process Events:** Your callback will now be invoked every time the game writes a new line to its log, allowing you to parse the string and react accordingly.

## Getting the API

The Game Log API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnLoad` function.

```c
#include "SPF/SPF_API/SPF_Plugin.h"

// Global pointer to the Core API
const SPF_Core_API* s_coreAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
    
    // You can now access s_coreAPI->gamelog anywhere in your plugin
}
```

## Callback Definition

Your callback function must match the following signature:

---
**`void YourCallback(const char* log_line, void* user_data)`**

*   `log_line`: A pointer to a null-terminated string containing the raw log line from the game.
*   `user_data`: A user-defined pointer that you can optionally pass during registration. This is useful for providing context to your callback, such as a pointer to one of your plugin's C++ objects.

## Function Reference

The API consists of a single function, accessed via the `gamelog` member of your `SPF_Core_API` pointer.

---
**`SPF_GameLog_Callback_Handle RegisterCallback(const char* pluginName, SPF_GameLog_Callback callback, void* user_data)`**

Registers a callback function to be invoked for each new game log line.

*   **Parameters:**
    *   `pluginName`: Your plugin's name, which must match the manifest. This is used for internal tracking.
    *   `callback`: A function pointer to your handler.
    *   `user_data`: An optional pointer to your own data that will be passed to your callback.
*   **Returns:** An opaque `SPF_GameLog_Callback_Handle`. You should store this handle. The framework will automatically unregister the callback when the plugin is unloaded.

## Complete Example

This example demonstrates how to register a callback to detect when a new driver is hired.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include <string.h> // For strstr

// Global reference to the Core API
const SPF_Core_API* s_coreAPI = NULL;
// Handle for our callback registration
SPF_GameLog_Callback_Handle s_driverHiredCallbackHandle = NULL;

// 1. Implement the callback function
void OnGameLogMessage(const char* log_line, void* user_data) {
    // Check if the log line contains the substring "driver hired"
    if (strstr(log_line, "driver hired") != NULL) {
        // A new driver was hired! Trigger some logic.
        // For example, log a message using the Logger API.
        if (s_coreAPI && s_coreAPI->logger) {
            SPF_Logger_Handle* myLogger = s_coreAPI->logger->GetContext("MyPlugin");
            s_coreAPI->logger->Log(myLogger, SPF_LOG_INFO, "Detected a new driver was hired!");
        }
    }
}

// 2. Register the callback when the plugin loads
SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;

    if (s_coreAPI && s_coreAPI->gamelog) {
        // Register our function. We don't need user_data for this simple example, so we pass NULL.
        s_driverHiredCallbackHandle = s_coreAPI->gamelog->RegisterCallback("MyPlugin", OnGameLogMessage, NULL);
    }
}
```
