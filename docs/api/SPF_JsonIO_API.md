# SPF JSON IO API

The SPF JSON IO API provides functions for loading, saving, and parsing JSON data. This is the bridge between JSON data in memory (handled via the [Writer](SPF_JsonWriter_API.md) and [Reader](SPF_JsonReader_API.md) APIs) and external sources like files or strings.

## Core Concepts

*   **Parsing:** Converts a string into a handle.
*   **Loading:** Reads a file from disk into a handle.
*   **Serialization:** Converts a handle back into a string or saves it to a file.

## Workflow

1.  **Load:** Start by loading a file (`Json_LoadFromFile`) or parsing a string (`Json_ParseString`).
2.  **Read/Modify:** Use the Reader or Writer APIs to work with the resulting `SPF_JsonValue_Handle`.
3.  **Save:** Save your changes back to disk (`Json_SaveToFile`).

**Example:**
```c
// Load a file
SPF_JsonValue_Handle* h = io->Json_LoadFromFile("plugins/MyPlugin/data/config.json");

if (h) {
    // Read something using Reader API
    const SPF_JsonValue_Handle* val = reader->Json_GetMember(h, "settings.active");
    bool isActive = reader->Json_GetBool(val, false);

    // Save changes back (if modified)
    io->Json_SaveToFile(h, "plugins/MyPlugin/data/config_backup.json", true);

    // Cleanup
    writer->Json_DestroyHandle(h);
}
```

## Function Reference

### Deserialization

**`SPF_JsonValue_Handle* Json_ParseString(const char* jsonString)`**
Parses a JSON string and returns a handle.
*   **Returns:** A handle to the JSON root, or `NULL` if parsing failed.

**`SPF_JsonValue_Handle* Json_LoadFromFile(const char* filePath)`**
Loads and parses a JSON file from disk.
*   **Returns:** A handle to the JSON root, or `NULL` if the file couldn't be opened or parsed.

---
### Serialization

**`int Json_ToString(const SPF_JsonValue_Handle* h, bool prettyPrint, char* out_buffer, int buffer_size)`**
Converts a JSON handle into a string representation.
*   `prettyPrint`: If `true`, the output string will be formatted with indentation (4 spaces).
*   **Returns:** Number of characters written. Truncated if >= `buffer_size`.

**`bool Json_SaveToFile(const SPF_JsonValue_Handle* h, const char* filePath, bool prettyPrint)`**
Saves a JSON handle's data to a file on disk.
*   **Returns:** `true` if saved successfully, `false` otherwise.

---
### Utilities

**`bool Json_IsValid(const char* jsonString)`**
Checks if the provided string is valid JSON without creating a full handle.
*   **Returns:** `true` if valid, `false` otherwise.
