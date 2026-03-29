# SPF Config API

The SPF Config API provides a simple and robust key-value store for your plugin to save and load settings. It abstracts away the file I/O and JSON parsing, allowing you to interact with your configuration through simple, type-safe functions.

All settings for a plugin are stored in a dedicated JSON file located at `[Game Plugins]/spf/plugins/[YourPluginName]/config/settings.json`.

## Programmatic vs. UI-Based Configuration

It's important to understand when to use this API.

Most simple settings can be automatically handled by the framework's UI. If you define a setting in your plugin's manifest builder and call `api->SetAllowUserConfig(h, true);`, users will be able to edit this value in the main Settings window.

This `SPF_Config_API` is intended for cases where you need to manage settings **programmatically** from your C++ code. For example:
*   Saving state that isn't exposed to the user in the UI.
*   Changing a setting in response to an in-game event.
*   Reading complex configuration structures that go beyond simple key-value pairs.

## Workflow

Using the Config API follows a simple pattern:

1.  **Declare in Manifest:** In your plugin's manifest builder function, enable the configuration system by calling `api->SetAllowUserConfig(handle, true)`. You can also provide a default JSON structure via `api->SetSettingsJson(handle, json)`.
2.  **Get Context:** In your `OnLoad` function, call `Cfg_GetContext()` with your plugin's name to get a configuration handle. This handle is your gateway to all other config functions.
3.  **Get/Set Values:** Use the getter (`Cfg_GetInt`, `Cfg_GetString`, etc.) and setter (`Cfg_SetInt`, `Cfg_SetString`, etc.) functions to read and write values. It is good practice to always provide a default value when reading.
4.  **React to Changes (Optional):** Implement the `OnSettingChanged` callback in your plugin's exports. The framework will call this function whenever a setting is changed, providing your plugin's `SPF_Config_Handle*` and the `keyPath` of the modified setting. You can then use the regular `Cfg_Get...` functions from this API to retrieve the new value.

    **Example `OnSettingChanged` implementation:**
    ```c
    void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
        if (strcmp(keyPath, "settings.some_bool") == 0) {
            // A specific boolean setting changed, get its new value.
            bool newValue = s_configAPI->Cfg_GetBool(config_handle, keyPath, false);
            // ... react to the change ...
        }
    }
    ```

## Simple vs. Advanced Usage

The Config API is designed with two levels of complexity:

*   **Simple Usage (Recommended for most cases):** Use the standard `Cfg_GetInt`, `Cfg_GetString`, `Cfg_GetBool`, etc. functions. These are easy to use and cover 95% of use cases for reading and writing simple configuration values.

*   **Advanced Usage (For complex data):** If your configuration contains nested JSON objects or arrays, the simple getters are not sufficient. In this scenario, you should use the "Advanced" workflow:
    1.  Use `Cfg_GetJsonValueHandle()` to retrieve an opaque handle to your complex JSON object.
    2.  Use the functions provided by the [`SPF_JsonReader_API`](SPF_JsonReader_API.md) to navigate and parse the data within that handle.

## Getting the API Context

To use the Config API, you must first get your plugin's unique `SPF_Config_Handle`. This handle tells the API which `settings.json` file to operate on.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Config_API.h"

// Global pointers to the APIs
SPF_Config_API* s_configAPI = NULL;
SPF_Config_Handle* s_myPluginConfig = NULL;

// In OnLoad...
void MyPlugin_OnLoad(const SPF_Load_API* load_api) {
    s_configAPI = load_api->config;
    
    if (s_configAPI) {
        // Get the context for our plugin, using the name from our manifest
        s_myPluginConfig = s_configAPI->Cfg_GetContext("MyPlugin");
    }
}
```

## Function Reference

### `SPF_Config_Handle* Cfg_GetContext(const char* pluginName)`
Gets a configuration context handle for your plugin. This is the most important function and must be called before any others.
*   **pluginName:** The name of your plugin. This **must** match the name declared in your manifest.
*   **Returns:** A handle to the configuration context, or `NULL` if not found.

---
### Value Getters

These functions retrieve values from your configuration file. If a key is not found, the provided `defaultValue` is returned.

**`int64_t Cfg_GetInt(SPF_Config_Handle* h, const char* key, int64_t defaultValue)`**
Retrieves a 64-bit integer value.
*   **h:** The context handle.
*   **key:** A dot-separated path to the value (e.g., `"settings.my_value"`).

**Example:**
```c
int64_t some_value = s_configAPI->Cfg_GetInt(s_myPluginConfig, "settings.some_number", 42);
```

---
**`int32_t Cfg_GetInt32(SPF_Config_Handle* h, const char* key, int32_t defaultValue)`**
Retrieves a 32-bit integer value. This is a convenience function for interoperability with APIs like ImGui that use `int`.

---
**`double Cfg_GetFloat(SPF_Config_Handle* h, const char* key, double defaultValue)`**
Retrieves a floating-point value.

---
**`bool Cfg_GetBool(SPF_Config_Handle* h, const char* key, bool defaultValue)`**
Retrieves a boolean value.

---
**`int Cfg_GetString(SPF_Config_Handle* h, const char* key, const char* defaultValue, char* out_buffer, int buffer_size)`**
Retrieves a string value.
*   `h`: The context handle.
*   `out_buffer`: A character buffer to copy the string into.
*   `buffer_size`: The size of `out_buffer` in bytes.
*   **Returns:** The number of characters written. If the return value is >= `buffer_size`, the string was truncated.

**Example:**
```c
char user_name[64];
s_configAPI->Cfg_GetString(s_myPluginConfig, "settings.user.name", "Player", user_name, sizeof(user_name));
```

---
**`SPF_JsonValue_Handle* Cfg_GetJsonValueHandle(SPF_Config_Handle* h, const char* key)`**
(Advanced) Retrieves a handle to a raw JSON value for a given key.

This function is for advanced use cases where a setting is a complex object or array. The returned handle is designed to be used with the [`SPF_JsonReader_API`](SPF_JsonReader_API.md) to parse the complex data structure.

*   **h:** The context handle.
*   **key:** The dot-separated path to the value (e.g., `"settings.my_complex_object"`).
*   **Returns:** An opaque handle to the JSON value, or `NULL` if not found. The lifetime of this handle is managed by the framework.

**Example:**
```c
// In OnActivated, after getting the core_api...
const SPF_JsonReader_API* json_reader = core_api->json_reader;
const SPF_JsonValue_Handle* obj_handle = s_configAPI->Cfg_GetJsonValueHandle(s_myPluginConfig, "settings.my_complex_object");

if (obj_handle && json_reader->Json_HasMember(obj_handle, "enabled")) {
    const SPF_JsonValue_Handle* enabled_handle = json_reader->Json_GetMember(obj_handle, "enabled");
    bool is_enabled = json_reader->Json_GetBool(enabled_handle, false);
    // ...
}
```


---
### Value Setters

These functions set values in your configuration. The changes are stored in memory and will be persisted to the `settings.json` file automatically on game shutdown or when settings are saved in the UI.

**`void Cfg_SetInt(SPF_Config_Handle* h, const char* key, int64_t value)`**
Sets a 64-bit integer value.

**Example:**
```c
s_configAPI->Cfg_SetInt(s_myPluginConfig, "settings.some_number", 123);
```
---
**`void Cfg_SetInt32(SPF_Config_Handle* h, const char* key, int32_t value)`**
Sets a 32-bit integer value.

---
**`void Cfg_SetFloat(SPF_Config_Handle* h, const char* key, double value)`**
Sets a floating-point value.

---
**`void Cfg_SetBool(SPF_Config_Handle* h, const char* key, bool value)`**
Sets a boolean value.

---
**`void Cfg_SetString(SPF_Config_Handle* h, const char* key, const char* value)`**
Sets a string value.

**Example:**
```c
s_configAPI->Cfg_SetString(s_myPluginConfig, "settings.user.name", "NewPlayerName");
```

---
**`void Cfg_SetJsonString(SPF_Config_Handle* h, const char* key, const char* json_literal)`**
Sets a raw JSON string as the value for a key. This allows writing complex structures (arrays, objects) in one call.
*   `json_literal`: A valid JSON string (e.g., `"[1, 2, 3]"` or `{\"a\": 1}`).

**Example:**
```c
s_configAPI->Cfg_SetJsonString(s_myPluginConfig, "settings.commands", "{\"cmd1\": {\"enabled\": true}}");
```

---
### Management & Inspection

These functions provide direct control over the configuration life-cycle and structure.

**`bool Cfg_HasKey(SPF_Config_Handle* h, const char* key)`**
Checks if a specific key path exists in the configuration.
*   **Returns:** `true` if the key exists, `false` otherwise.

---
**`void Cfg_Save(SPF_Config_Handle* h)`**
Force-saves the current configuration state from memory to `settings.json` immediately.
*   **Note:** Use this sparingly, as the framework normally saves changes on exit or when the user saves settings in the UI.

---
**`void Cfg_RemoveKey(SPF_Config_Handle* h, const char* key)`**
Removes a key or an entire path (including nested objects) from the configuration.
*   **Use Case:** Clearing old lists or nodes before writing new data.

---
**`void Cfg_Reload(SPF_Config_Handle* h)`**
Discards all unsaved in-memory changes and reloads the configuration from the disk.

---
**`SPF_Config_Handle* Cfg_CreateCustomContext(const char* filePath)`**
Creates a configuration context for an arbitrary JSON file on disk. This allows you to use the standard `Cfg_Get/Set` functions for any JSON file, not just the default `settings.json`.

*   **filePath:** The full physical path to the JSON file. You can use the `SPF_Environment_API` to resolve paths dynamically.
*   **Returns:** A handle to the configuration context, or `NULL` if the file couldn't be opened or parsed.

**Example:**
```c
char dataDir[512];
envAPI->Env_GetPluginDataDir(envHandle, dataDir, sizeof(dataDir));

char fullPath[1024];
fmtAPI->Fmt_Format(fullPath, sizeof(fullPath), "%s\\my_custom_config.json", dataDir);

SPF_Config_Handle* customCfg = configAPI->Cfg_CreateCustomContext(fullPath);
if (customCfg) {
    int value = configAPI->Cfg_GetInt32(customCfg, "ui.some_value", 0);
}
```

---
**`void Cfg_SetAutoSave(SPF_Config_Handle* h, bool enabled)`**
Enables or disables automatic saving to disk when values are changed in this context.

*   **h:** The context handle (plugin default or custom).
*   **enabled:** If `true` (default), changes made via `Cfg_Set...` are immediately synced to disk. If `false`, you must call `Cfg_Save` manually.

---
**`int Cfg_GetJsonString(SPF_Config_Handle* h, const char* key, char* out_buffer, int buffer_size)`**
Retrieves a raw JSON string representation of a configuration node.
*   **Returns:** Number of characters written. Truncated if >= `buffer_size`.