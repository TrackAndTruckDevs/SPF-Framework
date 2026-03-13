# SPF JSON Reader API

The SPF JSON Reader API is a specialized, **read-only** interface for safely inspecting JSON values. Its primary purpose is to allow plugins to read data passed to them by the framework in callbacks without creating a dependency on the framework's internal JSON library (`nlohmann/json`). This guarantees ABI (Application Binary Interface) stability.

**You should not need to request this API.** Instead, a pointer to it is provided directly as an argument in callbacks where it is needed, or can be accessed via `core_api->json_reader`.

## Primary Use Case: Parsing Complex Configuration

The primary use case for this API is to parse complex JSON objects or arrays retrieved from your plugin's configuration.

While the [`SPF_Config_API`](SPF_Config_API.md) is excellent for simple values (numbers, booleans, strings), it cannot directly return a nested object. This is where the `SPF_JsonReader_API` comes in.

## Workflow

The typical workflow for reading a complex setting is:
1.  In your `OnActivated` function, get and store the `core_api` pointer. The `SPF_JsonReader_API` is available via `core_api->json_reader`.
2.  Use the `SPF_Config_API`'s `Cfg_GetJsonValueHandle()` function to get a handle to your complex setting node.
3.  Use the functions in this `SPF_JsonReader_API` (like `Json_GetMember`, `Json_GetArraySize`, etc.) to navigate and extract data from the handle.

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

All functions are called via the `SPF_JsonReader_API*` pointer.

---
**`SPF_JsonType Json_GetType(const SPF_JsonValue_Handle* h)`**
Gets the data type of the JSON value represented by the handle. This should always be your first call.

---
**`bool Json_GetBool(const SPF_JsonValue_Handle* h, bool default_value)`**
Reads the value as a boolean. Returns `default_value` if the handle is invalid or the type does not match.

---
**`int64_t Json_GetInt(const SPF_JsonValue_Handle* h, int64_t default_value)`**
Reads the value as a 64-bit signed integer.

---
**`uint64_t Json_GetUint(const SPF_JsonValue_Handle* h, uint64_t default_value)`**
Reads the value as a 64-bit unsigned integer.

---
**`double Json_GetFloat(const SPF_JsonValue_Handle* h, double default_value)`**
Reads the value as a double-precision float.

---
**`int Json_GetString(const SPF_JsonValue_Handle* h, char* out_buffer, int buffer_size)`**
Reads the value as a string. Returns the number of characters written.

---
### Object & Array Navigation

These functions are used to navigate inside a JSON object or array handle.

---
**`bool Json_HasMember(const SPF_JsonValue_Handle* h, const char* memberName)`**
Checks if the JSON object represented by the handle has a specific member.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_OBJECT`.
*   **memberName:** The name of the member to check for.
*   **Returns:** `true` if the handle is a valid object and contains the member, `false` otherwise.

---
**`SPF_JsonValue_Handle* Json_GetMember(const SPF_JsonValue_Handle* h, const char* memberName)`**
Retrieves a handle to a member of a JSON object.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_OBJECT`.
*   **memberName:** The name of the member to retrieve.
*   **Returns:** A new handle to the member's value if found, otherwise `NULL`.

---
**`int Json_GetArraySize(const SPF_JsonValue_Handle* h)`**
Gets the number of elements in a JSON array.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_ARRAY`.
*   **Returns:** The number of elements, or `0` if the handle is not a valid array.

---
**`SPF_JsonValue_Handle* Json_GetArrayItem(const SPF_JsonValue_Handle* h, int index)`**
Retrieves a handle to an element at a specific index in a JSON array.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_ARRAY`.
*   **index:** The zero-based index of the element to retrieve.
*   **Returns:** A new handle to the element's value if the index is valid, otherwise `NULL`.

---
**`int Json_GetObjectSize(const SPF_JsonValue_Handle* h)`**
Gets the number of members (keys) in a JSON object.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_OBJECT`.
*   **Returns:** The number of members in the object.

---
**`int Json_GetMemberName(const SPF_JsonValue_Handle* h, int index, char* out_buffer, int buffer_size)`**
Retrieves the name of a member (key) at a specific index in a JSON object. This is the primary function for iterating over an object's keys.
*   **h:** A handle to a JSON value, which must be of type `SPF_JSON_TYPE_OBJECT`.
*   **index:** The zero-based index of the member.
*   **Returns:** The number of characters written. Truncated if >= `buffer_size`.

---
**`SPF_JsonValue_Handle* Json_GetMemberValueByIndex(const SPF_JsonValue_Handle* h, int index)`**
Retrieves a handle to a member's value at a specific index in a JSON object. 
*   **Use Case:** Optimized iteration when you need both the key (via `Json_GetMemberName`) and the value without doing a name-based lookup.
*   **Returns:** A new handle to the value, or `NULL` if the index is invalid.


## Complete Example: Iterating Over an Object

This example shows how to iterate over all members of an object without knowing their names beforehand.

```c
const SPF_JsonValue_Handle* obj_h = config->Cfg_GetJsonValueHandle(config_h, "settings.dynamic_list");

if (obj_h && json->Json_GetType(obj_h) == SPF_JSON_TYPE_OBJECT) {
    int size = json->Json_GetObjectSize(obj_h);
    for (int i = 0; i < size; ++i) {
        char key_name[64];
        json->Json_GetMemberName(obj_h, i, key_name, sizeof(key_name));
        
        const SPF_JsonValue_Handle* val_h = json->Json_GetMemberValueByIndex(obj_h, i);
        // ... process key_name and val_h ...
    }
}
```

## Complete Example

Here is a complete example of a function that reads a complex object from the configuration and parses it using this API.

**Assumed `settings.json`:**
```json
"settings": {
    "a_complex_object": { 
        "mode": "alpha", 
        "enabled": true, 
        "targets": ["a", "b", "c"] 
    }
}
```

**Example C++ Code:**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Config_API.h"
#include "SPF/SPF_API/SPF_JsonReader_API.h"

void ParseMyComplexObject(const SPF_Core_API* core_api) {
    if (!core_api || !core_api->config || !core_api->json_reader) return;

    const SPF_Config_API* config = core_api->config;
    const SPF_JsonReader_API* json = core_api->json_reader;
    
    SPF_Config_Handle* config_h = config->Cfg_GetContext("MyPlugin");

    // 1. Get a handle to the complex object from Config API
    const SPF_JsonValue_Handle* obj_h = config->Cfg_GetJsonValueHandle(config_h, "settings.a_complex_object");

    if (obj_h && json->Json_GetType(obj_h) == SPF_JSON_TYPE_OBJECT) {
        // 2. Get the "mode" string member.
        const SPF_JsonValue_Handle* mode_h = json->Json_GetMember(obj_h, "mode");
        if (mode_h && json->Json_GetType(mode_h) == SPF_JSON_TYPE_STRING) {
            char mode_str[64];
            json->Json_GetString(mode_h, mode_str, sizeof(mode_str));
            // Now mode_str contains "alpha"
        }

        // 3. Get the "targets" array member.
        const SPF_JsonValue_Handle* arr_h = json->Json_GetMember(obj_h, "targets");
        if (arr_h && json->Json_GetType(arr_h) == SPF_JSON_TYPE_ARRAY) {
            int size = json->Json_GetArraySize(arr_h); // Returns 3
            for (int i = 0; i < size; ++i) {
                const SPF_JsonValue_Handle* item_h = json->Json_GetArrayItem(arr_h, i);
                // ... process each item handle ...
            }
        }
    }
}
```