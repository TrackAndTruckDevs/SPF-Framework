# SPF Hooks API

The SPF Hooks API is a powerful, low-level interface that allows plugins to intercept and execute custom code in place of a native game function. This technique, known as "hooking" or "detouring," is the primary method for modifying or extending core game behaviors.

The API uses pattern scanning to locate functions in memory, making your hooks resilient to game updates that would normally break address-based pointers.

## Core Concepts

Before using this API, it is critical to understand three concepts:

**1. Signature**
A signature is a unique sequence of bytes that represents the beginning of a function in the game's compiled code. Instead of relying on a fixed memory address (which changes with every game update), we find the function by searching for this unique "fingerprint". Wildcards (`?`) can be used for bytes that might change.

**2. Detour**
A detour is your C++ function that the framework will execute *instead of* the original game function. For this to work safely, your detour function **must** have the exact same signature as the function you are hooking: the same calling convention, the same return type, and the same parameters in the same order.

**3. Trampoline**
When the framework installs a hook, it creates a "trampoline" – a small piece of code that saves the original function's starting bytes before they are overwritten. The framework gives you a pointer to this trampoline. From your detour function, you **must** call the original function via this trampoline pointer to ensure the game continues to operate correctly. Failing to do so will almost certainly crash the game.

## Workflow

1.  **Find Signature:** Using a disassembler or memory scanner (like Ghidra, x64dbg, or Cheat Engine), find the target function in the game and identify a unique byte pattern at its start.
2.  **Define Function Type:** In your C++ code, define a `using` or `typedef` for a function pointer that matches the original function's signature.
3.  **Implement Detour:** Write your detour function, matching the signature from the previous step.
4.  **Call the Original:** Inside your detour, call the original function using the trampoline pointer.
5.  **Register Hook:** In your plugin's `OnLoad` function, call `Register`, providing the signature, pointers to your detour and trampoline, and other metadata.

## Getting the API

The Hooks API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnLoad` function.

```c
#include "SPF/SPF_API/SPF_Plugin.h"

const SPF_Core_API* s_coreAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
}
```

## Function Reference

Functions are accessed via the `hooks` member of your `SPF_Core_API` pointer.

---
**`SPF_Hook_Handle* Register(...)`**
Finds a function by its byte signature and installs a hook.

*   **Parameters:**
    *   `pluginName`: Your plugin's name from the manifest.
    *   `hookName`: A unique programmatic name for the hook (e.g., `"MyPlugin_TrafficHook"`).
    *   `displayName`: A user-friendly name for display in UI menus.
    *   `pDetour`: A pointer to your detour function.
    *   `ppOriginal`: A pointer to your trampoline function pointer variable (e.g., `(void**)&o_MyFunction`). The framework will write the trampoline's address here.
    *   `signature`: A string representing the byte pattern (e.g., `"48 89 5C 24 ? 57 48 83"`).
    *   `isEnabled`: The initial enabled state of the hook.
*   **Returns:** An opaque handle to the hook, or `NULL` on failure.

---
**`uintptr_t FindPattern(const char* signature)`**
Finds a byte pattern in the game's memory and returns the address. This is useful for reading data or for more complex hooking scenarios.

---
**`bool IsEnabled(SPF_Hook_Handle* handle)`**
Checks if a hook is currently enabled in the configuration.

---
**`bool IsInstalled(SPF_Hook_Handle* handle)`**
Checks if a hook is currently active in memory (i.e., successfully found and installed).

## Complete Example

This example shows the full process for hooking a hypothetical game function `void SomeGameFunction(int param1, bool param2)`.

```c
#include "SPF/SPF_API/SPF_Plugin.h"

// Global API pointer
const SPF_Core_API* s_coreAPI = NULL;

// 1. Define the original function's signature as a type
using SomeGameFunction_t = void(*)(int, bool);

// 2. Create a global pointer for the trampoline.
// This will be filled by the framework upon registration.
static SomeGameFunction_t o_SomeGameFunction = NULL;

// 3. Implement your detour function with the matching signature
void Detour_SomeGameFunction(int param1, bool param2) {
    // 4. Your custom logic can run before the original function
    if (s_coreAPI && s_coreAPI->logger) {
        SPF_Logger_Handle* myLogger = s_coreAPI->logger->GetContext("MyPlugin");
        s_coreAPI->logger->Log(myLogger, SPF_LOG_INFO, "SomeGameFunction was hooked!");
    }

    // 5. Call the original function using the trampoline. This is CRITICAL.
    o_SomeGameFunction(param1, param2);

    // 6. Your custom logic can also run after the original function returns
}

// 7. Register the hook when the plugin loads
SPF_PLUGIN_ENTRY void MyPlugin_OnLoad(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;

    if (s_coreAPI && s_coreAPI->hooks) {
        s_coreAPI->hooks->Register(
            "MyPlugin",                             // Plugin name
            "SomeGameFunctionHook",                 // Unique hook name
            "My Test Hook",                         // Display name for UI
            &Detour_SomeGameFunction,               // Pointer to our detour
            (void**)&o_SomeGameFunction,            // Pointer to our trampoline variable
            "48 89 5C 24 ? 57 48 83 EC 60",         // The byte signature to find
            true                                    // Enable the hook by default
        );
    }
}
```
