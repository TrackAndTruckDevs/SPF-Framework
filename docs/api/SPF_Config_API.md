# SPF Config API

The SPF Config API provides a simple and robust key-value store for your plugin to save and load settings. It abstracts away the file I/O and JSON parsing, allowing you to interact with your configuration through simple, type-safe functions.

All settings for a plugin are stored in a dedicated JSON file located at `[Game Plugins]/spf/plugins/[YourPluginName]/config/settings.json`.

## Programmatic vs. UI-Based Configuration

It's important to understand when to use this API.

Most simple settings can be automatically handled by the framework's UI. If you define a setting in your plugin's manifest and set `configPolicy.allowUserConfig = true;`, users will be able to edit this value in the main Settings window.

This `SPF_Config_API` is intended for cases where you need to manage settings **programmatically** from your C++ code. For example:
*   Saving state that isn't exposed to the user in the UI.
*   Changing a setting in response to an in-game event.
*   Reading complex configuration structures that go beyond simple key-value pairs.

## Workflow

Using the Config API follows a simple pattern:

1.  **Declare in Manifest:** In your plugin's `GetManifestData` function, enable the configuration system by setting `configPolicy.allowUserConfig = true;`. You can also provide a default JSON structure for your settings.
2.  **Get Context:** In your `OnLoad` or `Init` function, call `GetContext()` with your plugin's name to get a configuration handle. This handle is your gateway to all other config functions.
3.  **Get/Set Values:** Use the getter (`GetInt`, `GetString`, etc.) and setter (`SetInt`, `SetString`, etc.) functions to read and write values. It is good practice to always provide a default value when reading.
4.  **React to Changes (Optional):** Implement the `OnSettingChanged` callback in your plugin's exports. The framework will call this function whenever a setting is changed, providing your plugin's `SPF_Config_Handle*` and the `keyPath` of the modified setting. You can then use the regular `Get...` functions from this API to retrieve the new value.

    **Example `OnSettingChanged` implementation:**
    ```c
    void OnSettingChanged(SPF_Config_Handle* config_handle, const char* keyPath) {
        if (strcmp(keyPath, "settings.some_bool") == 0) {
            // A specific boolean setting changed, get its new value.
            bool newValue = s_configAPI->GetBool(config_handle, keyPath, false);
            // ... react to the change ...
        }
    }
    ```

## Simple vs. Advanced Usage

The Config API is designed with two levels of complexity:

*   **Simple Usage (Recommended for most cases):** Use the standard `GetInt`, `GetString`, `GetBool`, etc. functions. These are easy to use and cover 95% of use cases for reading and writing simple configuration values.

*   **Advanced Usage (For complex data):** If your configuration contains nested JSON objects or arrays, the simple getters are not sufficient. In this scenario, you should use the "Advanced" workflow:
    1.  Use `GetJsonValueHandle()` to retrieve an opaque handle to your complex JSON object.
    2.  Use the functions provided by the [`SPF_JsonReader_API`](SPF_JsonReader_API.md) to navigate and parse the data within that handle.

## Getting the API Context

To use the Config API, you must first get your plugin's unique `SPF_Config_Handle`. This handle tells the API which `settings.json` file to operate on.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Config_API.h"

// Global pointers to the APIs
SPF_Config_API* s_configAPI = NULL;
SPF_Config_Handle* s_myPluginConfig = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_configAPI = (SPF_Config_API*)params->GetAPI(SPF_API_CONFIG);
    
    if (s_configAPI) {
        // Get the context for our plugin, using the name from our manifest
        s_myPluginConfig = s_configAPI->GetContext("MyPlugin");
    }
}
```

## Function Reference

### `SPF_Config_Handle* GetContext(const char* pluginName)`
Gets a configuration context handle for your plugin. This is the most important function and must be called before any others.
*   **pluginName:** The name of your plugin. This **must** match the `name` field in your manifest.
*   **Returns:** A handle to the configuration context, or `NULL` if not found.

---
### Value Getters

These functions retrieve values from your configuration file. If a key is not found, the provided `defaultValue` is returned.

**`int64_t GetInt(SPF_Config_Handle* handle, const char* key, int64_t defaultValue)`**
Retrieves a 64-bit integer value.
*   **key:** A dot-separated path to the value (e.g., `"settings.my_value"`).

**Example:**
```c
int64_t some_value = s_configAPI->GetInt(s_myPluginConfig, "settings.some_number", 42);
```

---
**`int32_t GetInt32(SPF_Config_Handle* handle, const char* key, int32_t defaultValue)`**
Retrieves a 32-bit integer value. This is a convenience function for interoperability with APIs like ImGui that use `int`.

---
**`double GetFloat(SPF_Config_Handle* handle, const char* key, double defaultValue)`**
Retrieves a floating-point value.

---
**`bool GetBool(SPF_Config_Handle* handle, const char* key, bool defaultValue)`**
Retrieves a boolean value.

---
**`int GetString(SPF_Config_Handle* handle, const char* key, const char* defaultValue, char* out_buffer, int buffer_size)`**
Retrieves a string value.
*   `out_buffer`: A character buffer to copy the string into.
*   `buffer_size`: The size of `out_buffer`.
*   **Returns:** The number of characters written. If the return value is >= `buffer_size`, the string was truncated.

**Example:**
```c
char user_name[64];
s_configAPI->GetString(s_myPluginConfig, "settings.user.name", "Player", user_name, sizeof(user_name));
```

---
**`SPF_JsonValue_Handle* GetJsonValueHandle(SPF_Config_Handle* handle, const char* key)`**
(Advanced) Retrieves a handle to a raw JSON value for a given key.

This function is for advanced use cases where a setting is a complex object or array. The returned handle is designed to be used with the [`SPF_JsonReader_API`](SPF_JsonReader_API.md) to parse the complex data structure.

*   **key:** The dot-separated path to the value (e.g., `"settings.my_complex_object"`).
*   **Returns:** An opaque handle to the JSON value, or `NULL` if not found. The lifetime of this handle is managed by the framework.

**Example:**
```c
// In OnActivated, after getting the core_api...
const SPF_JsonReader_API* json_reader = core_api->json_reader;
SPF_Config_Handle* config_handle = s_configAPI->GetContext("MyPlugin");

const SPF_JsonValue_Handle* obj_handle = s_configAPI->GetJsonValueHandle(config_handle, "settings.my_complex_object");

if (obj_handle && json_reader->HasMember(obj_handle, "enabled")) {
    const SPF_JsonValue_Handle* enabled_handle = json_reader->GetMember(obj_handle, "enabled");
    bool is_enabled = json_reader->GetBool(enabled_handle, false);
    // ...
}
```


---
### Value Setters

These functions set values in your configuration. The changes are stored in memory and will be persisted to the `settings.json` file automatically on game shutdown or when settings are saved in the UI.

**`void SetInt(SPF_Config_Handle* handle, const char* key, int64_t value)`**
Sets a 64-bit integer value.

**Example:**
```c
s_configAPI->SetInt(s_myPluginConfig, "settings.some_number", 123);
```
---
**`void SetInt32(SPF_Config_Handle* handle, const char* key, int32_t value)`**
Sets a 32-bit integer value.

---
**`void SetFloat(SPF_Config_Handle* handle, const char* key, double value)`**
Sets a floating-point value.

---
**`void SetBool(SPF_Config_Handle* handle, const char* key, bool value)`**
Sets a boolean value.

---
**`void SetString(SPF_Config_Handle* handle, const char* key, const char* value)`**
Sets a string value.

**Example:**
```c
s_configAPI->SetString(s_myPluginConfig, "settings.user.name", "NewPlayerName");
```
