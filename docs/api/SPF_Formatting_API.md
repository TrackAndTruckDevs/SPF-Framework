# SPF Formatting API

The SPF Formatting API provides a single, essential utility: a safe way to format strings using `printf`-style syntax.

## The Problem It Solves

In C and C++, passing variadic arguments (the `...` in functions like `printf`) across DLL boundaries (e.g., from your plugin to the SPF framework) is not guaranteed to be safe. Different compilers or build settings can lead to mismatched expectations on the call stack, causing crashes and undefined behavior.

This API solves that problem by providing a stable function that your plugin can call to perform formatting operations within the framework's memory space, ensuring safety and reliability.

## Getting the API

To use the formatting API, you first need to get a pointer to the `SPF_Formatting_API` struct from the framework during your plugin's initialization.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"

// Global pointer to the Formatting API
SPF_Formatting_API* s_fmtAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_fmtAPI = (SPF_Formatting_API*)params->GetAPI(SPF_API_FORMATTING);
}
```

## Function Reference

The API consists of a single function.

---
**`int Format(char* buffer, size_t buffer_size, const char* format, ...)`**

Formats a string using `printf`-style arguments and stores it safely in a provided buffer. This function is a wrapper around `vsnprintf`.

*   **Parameters:**
    *   `buffer`: A pointer to the character buffer where the formatted string will be written.
    *   `buffer_size`: The total size of your `buffer`. The function will not write past this boundary.
    *   `format`: The `printf`-style format string (e.g., `"Value: %d"`).
    *   `...`: The variable arguments that correspond to the format specifiers in the `format` string.
*   **Returns:** The number of characters that *would have been written* if the buffer was large enough (not including the null terminator). If this value is greater than or equal to `buffer_size`, it means the output was truncated. A negative value indicates an error.

## Complete Example

A common use case is to format a string before logging it or displaying it in the UI.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Formatting_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"

SPF_Formatting_API* s_fmtAPI = NULL;
SPF_Logger_API* s_loggerAPI = NULL;
SPF_Logger_Handle* s_myLogger = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_fmtAPI = (SPF_Formatting_API*)params->GetAPI(SPF_API_FORMATTING);
    s_loggerAPI = (SPF_Logger_API*)params->GetAPI(SPF_API_LOGGER);

    if (s_loggerAPI) {
        s_myLogger = s_loggerAPI->GetContext("MyPlugin");
    }
}

void SomeFunction() {
    if (!s_fmtAPI || !s_loggerAPI || !s_myLogger) {
        return;
    }

    char message[256];
    int score = 100;
    const char* player_name = "Player";

    s_fmtAPI->Format(message, sizeof(message), "Player '%s' reached a score of %d!", player_name, score);

    s_loggerAPI->Log(s_myLogger, SPF_LOG_INFO, message);
    // This will log: "Player 'Player' reached a score of 100!"
}
```
