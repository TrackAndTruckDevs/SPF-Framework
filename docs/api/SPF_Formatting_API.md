# SPF Formatting API

The SPF Formatting API provides a single, essential utility: a safe way to format strings using `printf`-style syntax across DLL boundaries.

## The Problem It Solves

In C and C++, passing variadic arguments (the `...` in functions like `printf`) across DLL boundaries (e.g., from your plugin to the SPF framework) is not guaranteed to be safe. Different compilers or build settings can lead to mismatched expectations on how the call stack is managed, often causing crashes and undefined behavior.

This API solves that problem by providing a stable function that your plugin can call to perform formatting operations within the framework's memory space, ensuring safety and reliability.

## Getting the API

To use the formatting API, you first obtain a pointer to the `SPF_Formatting_API` struct from the `SPF_Load_API` during your plugin's `OnLoad` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"

// Global pointer to the Formatting API
const SPF_Formatting_API* s_fmtAPI = NULL;

void MyPlugin_OnLoad(const SPF_Load_API* load_api) {
    s_fmtAPI = load_api->formatting;
}
```

## Function Reference

The API consists of a single function.

---
### `int Fmt_Format(char* buffer, size_t buffer_size, const char* format, ...)`

Formats a string using `printf`-style arguments and stores it safely in a provided buffer. This function is a safe wrapper around `vsnprintf`.

*   **Parameters:**
    *   `buffer`: A pointer to the character buffer where the formatted string will be written.
    *   `buffer_size`: The total size of your `buffer` in bytes. The function will not write past this boundary.
    *   `format`: The `printf`-style format string (e.g., `"Value: %d"`).
    *   `...`: The variable arguments that correspond to the format specifiers in the `format` string.
*   **Returns:** The number of characters that *would have been written* if the buffer was large enough (not including the null terminator). If this value is greater than or equal to `buffer_size`, it means the output was truncated. A negative value indicates a formatting error.

## Complete Example

A common use case is to format a string before logging it.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"

const SPF_Formatting_API* s_fmtAPI = NULL;
const SPF_Logger_API* s_loggerAPI = NULL;
SPF_Logger_Handle* s_myLogger = NULL;

void MyPlugin_OnLoad(const SPF_Load_API* load_api) {
    s_fmtAPI = load_api->formatting;
    s_loggerAPI = load_api->logger;

    if (s_loggerAPI) {
        s_myLogger = s_loggerAPI->GetLogger("MyPlugin");
    }
}

void LogPlayerScore(const char* name, int score) {
    if (!s_fmtAPI || !s_loggerAPI || !s_myLogger) return;

    char message[256];
    s_fmtAPI->Fmt_Format(message, sizeof(message), "Player '%s' reached a score of %d!", name, score);

    s_loggerAPI->Log(s_myLogger, SPF_LOG_INFO, message);
}
```