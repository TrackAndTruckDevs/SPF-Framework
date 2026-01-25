# SPF Game Console API

The SPF Game Console API provides a simple, one-way interface for your plugin to execute commands in the game's built-in developer console. This allows you to programmatically change game variables, trigger events, or perform any other action that is possible through console commands.

## Prerequisites: Requesting the Hook

To ensure the framework can safely hook into the game's console, your plugin **must** declare its dependency. In your plugin's manifest builder function, you must add the "GameConsole" hook.

**Example Manifest Snippet:**
```c
void BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // ...
    api->Policy_AddRequiredHook(h, "GameConsole");
}
```
If you do not request this hook, the framework will not provide a pointer to the Game Console API, and it will be `NULL`.

## Getting the API

The Game Console API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnActivated` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_GameConsole_API.h"

// Global pointer to the Core API
const SPF_Core_API* s_coreAPI = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
    
    // You can now access s_coreAPI->console anywhere in your plugin
}
```

## Function Reference

The API consists of a single function, accessed via the `console` member of your `SPF_Core_API` pointer.

---
### `void GCon_ExecuteCommand(const char* command)`

Executes a command string in the in-game console as if a user had typed it.

*   **Parameters:**
    *   `command`: The full command string to execute (e.g., `"g_traffic 1.0"`, `"g_set_time 14 30"`).

**Important Note:** This API is for **executing** existing game commands only. It does not provide a way for plugins to **register** new custom commands with the console.

## Complete Example

This example shows how you might create a button in your plugin's UI to toggle the in-game police presence.

```c
// Assumes s_coreAPI is initialized as shown above.
// Assumes 'ui' is a pointer to the SPF_UI_API.

static bool police_enabled = true;

void RenderMyUI(const SPF_UI_API* ui) {
    if (s_coreAPI && s_coreAPI->console && ui->Button("Toggle Police", 0, 0)) {
        police_enabled = !police_enabled;
        
        const char* cmd = police_enabled ? "g_police 1" : "g_police 0";
        s_coreAPI->console->GCon_ExecuteCommand(cmd);
    }
}
```