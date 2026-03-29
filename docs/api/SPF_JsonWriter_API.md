# SPF JSON Writer API

The SPF JSON Writer API allows plugins to build and modify complex JSON structures in memory. This is particularly useful for creating data that needs to be saved to a file or sent to the framework. All operations are performed using opaque handles to ensure ABI stability.

## Core Concepts

*   **Handles:** All JSON nodes are represented by `SPF_JsonValue_Handle`.
*   **Root Nodes:** You start by creating a root node (either an Object `{}` or an Array `[]`).
*   **Memory Management:** When you create a handle using `Json_CreateObject` or `Json_CreateArray`, you are responsible for calling `Json_DestroyHandle` when you are finished with it, unless you pass that handle to a function that takes ownership.

## Workflow

1.  **Create Root:** Create an empty object or array.
2.  **Add Data:** Use the "Set" or "Append" functions to populate the JSON.
3.  **Use & Destroy:** Once the JSON is built, you can save it to a file using the [JSON IO API](SPF_JsonIO_API.md) or convert it to a string. Finally, destroy the root handle.

**Example:**
```c
// Create: { "name": "Gemini", "version": 2, "features": ["AI", "Speed"] }
SPF_JsonValue_Handle* root = writer->Json_CreateObject();
writer->Json_SetString(root, "name", "Gemini");
writer->Json_SetInt(root, "version", 2);

SPF_JsonValue_Handle* features = writer->Json_CreateArray();
writer->Json_ArrayAppendString(features, "AI");
writer->Json_ArrayAppendString(features, "Speed");

// Attaching a child node (the array) to the parent object.
writer->Json_SetNode(root, "features", features);

// Save it using the IO API
io->Json_SaveToFile(root, "plugins/MyPlugin/data/info.json", true);

// Cleaning up the root handle also cleans up all child nodes.
writer->Json_DestroyHandle(root);
```

## Function Reference

### Creation & Cleanup

**`SPF_JsonValue_Handle* Json_CreateObject()`**
Creates an empty JSON object `{}`.

**`SPF_JsonValue_Handle* Json_CreateArray()`**
Creates an empty JSON array `[]`.

**`void Json_DestroyHandle(SPF_JsonValue_Handle* h)`**
Explicitly destroys a JSON handle and all its children, freeing the associated memory.

---
### Object Modification (Key-Value)

**`void Json_SetInt(SPF_JsonValue_Handle* h, const char* key, int64_t value)`**
Sets an integer value for a specific key.

**`void Json_SetDouble(SPF_JsonValue_Handle* h, const char* key, double value)`**
Sets a floating-point value.

**`void Json_SetBool(SPF_JsonValue_Handle* h, const char* key, bool value)`**
Sets a boolean value.

**`void Json_SetString(SPF_JsonValue_Handle* h, const char* key, const char* value)`**
Sets a string value.

**`void Json_SetNode(SPF_JsonValue_Handle* h, const char* key, SPF_JsonValue_Handle* nodeHandle)`**
Attaches another JSON handle (object or array) as a member of this object.

---
### Array Modification

**`void Json_ArrayAppendInt(SPF_JsonValue_Handle* h, int64_t value)`**
Appends an integer to the end of an array.

**`void Json_ArrayAppendDouble(SPF_JsonValue_Handle* h, double value)`**
Appends a floating-point value.

**`void Json_ArrayAppendBool(SPF_JsonValue_Handle* h, bool value)`**
Appends a boolean value.

**`void Json_ArrayAppendString(SPF_JsonValue_Handle* h, const char* value)`**
Appends a string.

**`void Json_ArrayAppendNode(SPF_JsonValue_Handle* h, SPF_JsonValue_Handle* nodeHandle)`**
Appends another JSON handle as a new item in the array.

---
### Utilities

**`void Json_RemoveMember(SPF_JsonValue_Handle* h, const char* key)`**
Removes a member from an object by its key.

**`void Json_RemoveArrayItem(SPF_JsonValue_Handle* h, int index)`**
Removes an item from an array by its index.

**`void Json_Clear(SPF_JsonValue_Handle* h)`**
Clears all members from an object or all items from an array.
