# SPF JSON Reader API

The SPF JSON Reader API is a specialized, **read-only** interface for safely inspecting JSON values. Its primary purpose is to allow plugins to read data passed to them by the framework in callbacks without creating a dependency on the framework's internal JSON library (`nlohmann/json`). This guarantees ABI (Application Binary Interface) stability.

**You should not need to request this API.** Instead, a pointer to it is provided directly as an argument in callbacks where it is needed.

## Primary Use Case: `OnSettingChanged`

The most common use case for this API is within the `OnSettingChanged` plugin callback. When a user (or your code via the `SPF_Config_API`) changes a setting, the framework invokes this callback, providing you with:
*   The key of the setting that changed.
*   A handle to the new JSON value (`SPF_JsonValue_Handle`).
*   A pointer to this `SPF_JsonReader_API`.

This allows your plugin to react to the change in real-time by safely reading the new value.

## Workflow

1.  In a callback like `OnSettingChanged`, receive the `SPF_JsonValue_Handle` and the `SPF_JsonReader_API`.
2.  Use the `reader->GetType(value_handle)` function to determine the data type of the new value.
3.  Based on the returned `SPF_JsonType`, call the appropriate getter function (e.g., `reader->GetBool(...)`) to read the value.

## Data Types

### `SPF_JsonType` (enum)
Represents the data type of a JSON value held by a `SPF_JsonValue_Handle`.

| Value | Description |
|---|---|
| `SPF_JSON_TYPE_NULL` | The value is null. |
| `SPF_JSON_TYPE_OBJECT` | The value is a JSON object. |
| `SPF_JSON_TYPE_ARRAY` | The value is a JSON array. |
| `SPF_JSON_TYPE_STRING` | The value is a string. |
| `SPF_JSON_TYPE_BOOLEAN` | The value is a boolean (`true`/`false`). |
| `SPF_JSON_TYPE_NUMBER_INTEGER`| The value is a signed integer. |
| `SPF_JSON_TYPE_NUMBER_UNSIGNED`| The value is an unsigned integer. |
| `SPF_JSON_TYPE_NUMBER_FLOAT` | The value is a floating-point number. |

## Function Reference

All functions are called via the `SPF_JsonReader_API*` pointer provided to your callback.

---
**`SPF_JsonType GetType(const SPF_JsonValue_Handle* handle)`**
Gets the data type of the JSON value represented by the handle. This should always be your first call.

---
**`bool GetBool(const SPF_JsonValue_Handle* handle, bool default_value)`**
Reads the value as a boolean. Returns `default_value` if the handle is invalid or the type does not match.

---
**`int64_t GetInt(const SPF_JsonValue_Handle* handle, int64_t default_value)`**
Reads the value as a 64-bit signed integer.

---
**`uint64_t GetUint(const SPF_JsonValue_Handle* handle, uint64_t default_value)`**
Reads the value as a 64-bit unsigned integer.

---
**`double GetFloat(const SPF_JsonValue_Handle* handle, double default_value)`**
Reads the value as a double-precision float.

---
**`int GetString(const SPF_JsonValue_Handle* handle, char* out_buffer, int buffer_size)`**
Reads the value as a string. Returns the number of characters written (or that would have been written), similar to `snprintf`.

## Complete Example

Here is a complete implementation of an `OnSettingChanged` callback that uses the JSON Reader API.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_JsonReader_API.h"
#include <string.h> // For strcmp

// This function is exported by the plugin's SPF_Plugin_Exports struct
SPF_PLUGIN_ENTRY void MyPlugin_OnSettingChanged(
    const char* keyPath, 
    const SPF_JsonValue_Handle* newValue, 
    const SPF_JsonReader_API* reader) 
{
    if (strcmp(keyPath, "settings.enable_feature_x") == 0) {
        // The setting for our feature has changed.
        
        // 1. Check the type
        if (reader->GetType(newValue) == SPF_JSON_TYPE_BOOLEAN) {
            // 2. Get the value
            bool is_enabled = reader->GetBool(newValue, false);

            // 3. React to the change
            if (is_enabled) {
                // Enable our feature...
            } else {
                // Disable our feature...
            }
        }
    }
}
```
