# SPF Game Log API

The SPF Game Log API provides a real-time stream of every message written to the game's internal log file (`game.log.txt`). This event-driven mechanism allows plugins to react to a wide range of in-game events that are not available through standard telemetry channels.

## Why Use It?

Many critical game events, such as economic transactions, asset loading, or script notifications, are only reported via the game log. By subscribing to these log lines, your plugin can monitor:
*   Driver hiring and management events.
*   Discovery of new cities and dealerships.
*   Game saving and loading sequences.
*   Internal engine warnings or specific script triggers.

## Workflow

1.  **Define a Callback:** Implement a function matching the `SPF_GameLog_Callback_t` signature.
2.  **Get Context:** Call `GLog_GetContext()` once during initialization to obtain your plugin's log monitoring handle.
3.  **Register Subscription:** Use `GLog_RegisterCallback()` to link your function to the log stream.
4.  **Parse and React:** Use standard string manipulation (like `strstr`) within your callback to detect and handle specific log patterns.

## Getting the API

The Game Log API is provided as part of the main `SPF_Core_API` struct received in your plugin's `OnActivated` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_GameLog_API.h"

const SPF_GameLog_API* s_logAPI = NULL;
SPF_GameLog_Handle* s_logCtx = NULL;
SPF_GameLog_Callback_Handle* s_subscription = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_logAPI = core_api->gamelog;
    
    if (s_logAPI) {
        // 1. Get our context
        s_logCtx = s_logAPI->GLog_GetContext("MyPlugin");
        
        // 2. Register the monitor
        s_subscription = s_logAPI->GLog_RegisterCallback(s_logCtx, OnLogLine, NULL);
    }
}
```

## Function Reference

### `SPF_GameLog_Handle* GLog_GetContext(const char* pluginName)`
Retrieves the monitoring context for your plugin.
*   **pluginName:** The unique programmatic name of your plugin.
*   **Returns:** A handle to the context, or NULL on failure.

---
### `SPF_GameLog_Callback_Handle* GLog_RegisterCallback(SPF_GameLog_Handle* h, SPF_GameLog_Callback_t callback, void* userData)`
Subscribes to the live game log stream.
*   **h:** The context handle obtained from `GLog_GetContext`.
*   **callback:** The function to be called for every new log line.
*   **userData:** An optional pointer passed back to your callback for context.
*   **Returns:** A handle to the specific subscription.

**Lifecycle Note:** You do not need to manually unregister. All subscriptions are automatically cleaned up when the plugin is unloaded and its handles are destroyed.

---

## Callback Signature

Your handler must match this definition:

**`void OnLogLine(const char* message, void* userData)`**
*   `message`: The raw text of the log line.
*   `userData`: The pointer provided during registration.

---

## Complete Example

This example demonstrates how to detect when the game starts saving.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include <string.h>

void OnLogLine(const char* message, void* userData) {
    // Detect the start of a save operation
    if (strstr(message, "Game has been saved")) {
        // ... perform logic here ...
    }
}

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    if (core_api->gamelog) {
        SPF_GameLog_Handle* h = core_api->gamelog->GLog_GetContext("MyPlugin");
        core_api->gamelog->GLog_RegisterCallback(h, OnLogLine, NULL);
    }
}
```