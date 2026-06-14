# SPF Hooks API

The SPF Hooks API is a powerful, low-level interface that allows plugins to intercept and execute custom code in place of a native game function. This technique, known as "hooking" or "detouring," is the primary method for modifying or extending core game behaviors.

The API uses pattern scanning to locate functions in memory, making your hooks resilient to game updates that would normally break address-based pointers.

## Core Concepts

Before using this API, it is critical to understand three concepts:

**1. Signature**
A signature is a unique sequence of bytes that represents the beginning of a function in the game's compiled code. Instead of relying on a fixed memory address (which changes with every game update), we find the function by searching for this unique "fingerprint". 

The SPF pattern scanner supports advanced syntax:
- **Exact Bytes**: Hex values (e.g., `48 89`).
- **Wildcards**: `?` or `??` matches any single byte.
- **Ranges**: `[XX-YY]` matches any byte within the specified hex range (e.g., `[40-7F]`).

### Signature Syntax

The pattern scanner is designed to be both flexible and precise. Using ranges is especially powerful for x64 assembly where you want to match instructions that target any general-purpose register but avoid RIP-relative addressing.

| Syntax | Description | Example |
| :--- | :--- | :--- |
| `XX` | Exact hex byte | `48`, `8B`, `E8` |
| `?` or `??` | Wildcard (matches anything) | `48 8B ?? ??` |
| `[XX-YY]` | Hex range (inclusive) | `[40-7F]` |
| `[min-max?]` | Variable wildcard (matches any sequence of length between min and max) | `[1-4?]`, `[0-1?]` |

### String-Based Pattern Scanning
The most stable way to find functions is by referencing unique strings they contain (like error messages or log entries). Unlike byte patterns at the start of a function, these strings rarely change between game updates.

---
### `uintptr_t Hook_FindString(const char* str)`
Finds the memory address of a null-terminated string.
*   **Note**: A "null-terminated" string is a standard C-string that ends with a `0x00` byte.

---
### `uintptr_t Hook_FindFunctionByString(const char* str, bool findStart, const char* contextSig, size_t contextRange)`
Locates a function based on a string reference (Xref) found within its code.

*   **Parameters:**
    *   `str`: The string to search for.
    *   `findStart`: If `true`, the scanner will automatically backtrack from the string reference to find the function's prologue (`PUSH RBX`, `SUB RSP`, etc.).
    *   `contextSig`: (Optional) An additional byte signature to match near the string reference. This is used to disambiguate if multiple functions use the same string.
    *   `contextRange`: The search window size (in bytes) for the context signature. Default is 512.

### String Search Example
If a string `"[cmd] Unknown queue id: %u"` is used in three different functions, you can find the exact one by providing a short context signature and a tight search range.

```cpp
// Target: Find FUN_1401dc890 which uses a specific error log
const char* targetStr = "[cmd] Unknown queue id: %u";
const char* targetContext = "e8 ?? ?? ?? ?? 32 c0"; // CALL log + XOR AL, AL (exit block)
size_t range = 32; // Only look within 32 bytes of the string reference

uintptr_t funcAddr = api->hooks->Hook_FindFunctionByString(targetStr, true, targetContext, range);

if (funcAddr) {
    Log("Function found at: 0x%p", funcAddr);
}
```

---
### `uintptr_t Hook_GetFunctionStart(uintptr_t address)`
Finds the starting address of a function containing the given address.

*   **Details**: Uses Windows Runtime Function Tables (`.pdata`) for 100% accuracy on x64. This is the most reliable way to find function boundaries without using heuristics like backtracking for `PUSH RBP` or `CC` padding.
*   **Parameters:**
    *   `address`: Any valid memory address within the target function (e.g., an address found via pattern scanning).
*   **Returns**: The address of the function's first instruction, or `0` if not found.

### Function Start Example
If you find a unique instruction in the middle of a large function, you can instantly get the function's entry point to install a hook.

```cpp
// Find a unique instruction inside UpdateSimulationTime
uintptr_t midAddr = api->hooks->Hook_FindPattern("B8 6D C1 16 6C"); // Math constant

if (midAddr) {
    // Get the actual function start (prologue)
    uintptr_t funcStart = api->hooks->Hook_GetFunctionStart(midAddr);
    
    // Install hook at the start
    api->hooks->Hook_Register(..., funcStart, ...);
}
```

---
### `uintptr_t Hook_FindChain(const char** signatures, size_t count, size_t maxGap, uintptr_t startAddress, size_t searchRange)`
Finds a sequence of patterns that appear close to each other in memory.

*   **Details**: Useful when the compiler inserts padding, NOPs, or minor logic (like log calls) between key instructions. This creates a "logical chain" that is much harder to break than a single long byte pattern.
*   **Parameters:**
    *   `signatures`: An array of signature strings.
    *   `count`: Number of elements in the array.
    *   `maxGap`: Maximum number of bytes allowed between each matched pattern.
    * `startAddress`: Optional address to start searching from. If `0`, scans the entire module.
    * `searchRange`: Optional range limit. If `0` and `startAddress` is set, it defaults to **4KB** (which covers 99% of game functions). Use a manual value only for exceptionally large functions.
    *   **Returns**: The address where the **FIRST** pattern in the chain starts, or `0` if the full chain is not found.

### Instruction Chaining Example
Instead of one fragile 30-byte signature, use three stable 5-byte "anchors".

```cpp
const char* chain[] = {
    "F3 0F 58 [80-BF] ?? ?? ?? ??", // ADDSS (Anchor 1)
    "0F 11 [80-BF] ?? ?? ?? ??",    // MOVSS (Anchor 2)
    "73 05"                         // JNC   (Anchor 3)
};

// Search for this sequence with max 12 bytes gap between anchors
// searchRange is 0, so it will auto-detect the function end if startAddress is provided
uintptr_t addr = api->hooks->Hook_FindChain(chain, 3, 12, funcStartAddr, 0);

if (addr) {
    Log("Stable sequence found at: 0x%p", addr);
}
```

---
### `uintptr_t Hook_FindVTable(const char* signature, int offsetPos, int instructionSize)`
Extracts a VTable address from an instruction that references it (usually a constructor or static initializer).

*   **Parameters:**
    *   `signature`: The byte pattern for the instruction (e.g., `48 8D 05 ?? ?? ?? ??` for `LEA RAX, [RIP+...]`).
    *   `offsetPos`: The byte offset within the instruction where the 32-bit displacement is located.
    *   `instructionSize`: The total size of the instruction in bytes.

---
### `uintptr_t Hook_GetVTableFunction(uintptr_t vtableAddr, int index)`
Gets a function address from a Virtual Function Table (VTable) by its index. This is the most stable way to hook class methods as their positions in the VTable rarely change.

*   **Parameters:**
    *   `vtableAddr`: The absolute address of the VTable (found via `Hook_FindVTable`).
    *   `index`: The 0-based index of the virtual function.

### VTable Hooking Example
```cpp
// 1. Find the VTable address from a known instruction
uintptr_t vtable = api->hooks->Hook_FindVTable("48 8D 05 ?? ?? ?? ??", 3, 7);

if (vtable) {
    // 2. Get the 'Update' function (e.g., at index 5 in the VTable)
    uintptr_t updateFn = api->hooks->Hook_GetVTableFunction(vtable, 5);
    
    // 3. Install hook at the resolved address
    api->hooks->Hook_Register(..., updateFn, ...);
}
```

---
### `uintptr_t Hook_FindFunctionByConstant(uint32_t constant, bool findStart)`
Locates a function that uses a specific 32-bit constant (magic number, math constant, or bitmask).

*   **Details**: The scanner finds the constant in the data section, locates RIP-relative references to it, and returns the function address.
*   **Parameters:**
    *   `constant`: The 32-bit value to search for.
    *   `findStart`: If `true`, returns the function prologue address using `.pdata`.

### Constant Search Example
```cpp
// Find the function that uses the magic constant for time scaling (0x3C888889)
uintptr_t timeScaleFn = api->hooks->Hook_FindFunctionByConstant(0x3C888889, true);

if (timeScaleFn) {
    Log("Time scaling function found at: 0x%p", timeScaleFn);
}
```

### Multi-Version Matching
One of the most powerful features of the SPF pattern scanner is the ability to create a single signature that works across multiple game versions, even if the compiler added new instructions or changed stack allocations.

**Scenario:**
* **Version 1:** `40 56 48 83 ec 60` (Standard push and 1-byte stack sub)
* **Version 2:** `40 55 56 48 81 ec a8 00 00 00` (Added `PUSH RBP` and 4-byte stack sub)

**Unified Signature:**
`40 [0-1?] 56 48 [81-83] ec [1-4?]`

*   `[0-1?]` handles the optional `PUSH RBP`.
*   `[81-83]` matches both types of `SUB RSP` instructions.
*   `[1-4?]` handles the difference between a 1-byte and 4-byte stack size operand.

**Example: Advanced Matching**
To find an instruction like `LEA REG, [REG + disp8]` while ignoring `LEA REG, [RIP + disp32]`, you can use:
`48 8D [40-7F]`

In this case, `[40-7F]` matches the ModR/M byte for register+offset addressing, effectively skipping RIP-relative variants (which use bytes like `05`, `0D`, `15`, etc. which are outside the `40-7F` range).

**2. Detour**
A detour is your C++ function that the framework will execute *instead of* the original game function. For this to work safely, your detour function **must** have the exact same signature as the function you are hooking: the same calling convention, the same return type, and the same parameters in the same order.

**3. Trampoline**
When the framework installs a hook, it creates a "trampoline" – a small piece of code that saves the original function's starting bytes before they are overwritten. The framework gives you a pointer to this trampoline. From your detour function, you **must** call the original function via this trampoline pointer to ensure the game continues to operate correctly. Failing to do so will almost certainly crash the game.

## Workflow

1.  **Find Signature:** Using a disassembler or memory scanner (like Ghidra, x64dbg, or Cheat Engine), find the target function in the game and identify a unique byte pattern at its start.
2.  **Define Function Type:** In your C++ code, define a `using` or `typedef` for a function pointer that matches the original function's signature.
3.  **Implement Detour:** Write your detour function, matching the signature from the previous step.
4.  **Call the Original:** Inside your detour, call the original function using the trampoline pointer.
5.  **Register Hook:** In your plugin's `OnActivated` function, call `Hook_Register`, providing the signature, pointers to your detour and trampoline, and other metadata.

## Getting the API

The Hooks API is provided as part of the main `SPF_Core_API` struct that your plugin receives in its `OnActivated` lifecycle event.

```c
#include "SPF/SPF_API/SPF_Plugin.h"

const SPF_Core_API* s_coreAPI = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;
}
```

## Function Reference

Functions are accessed via the `hooks` member of your `SPF_Core_API` pointer.

---
### `SPF_Hook_Handle* Hook_Register(...)`
Finds a function by its byte signature and installs a hook.

*   **Parameters:**
    *   `pluginName`: Your plugin's name from the manifest.
    *   `hookName`: A unique programmatic name for the hook (e.g., `"MyPlugin_TrafficHook"`).
    *   `displayName`: A user-friendly name for display in UI menus.
    *   `pDetour`: A pointer to your detour function.
    *   `ppOriginal`: A pointer to your trampoline function pointer variable (e.g., `(void**)&o_MyFunction`). The framework will write the trampoline's address here.
    *   `signature`: A string representing the byte pattern. Supports exact bytes, wildcards (`??`), and ranges (`[XX-YY]`).
    *   `isEnabled`: The initial enabled state of the hook.
*   **Returns:** An opaque handle to the hook, or `NULL` on failure.

---
### `uintptr_t Hook_FindPattern(const char* signature)`
Finds a byte pattern in the game's memory and returns the address. Supports the full signature syntax including ranges and wildcards.

---
### `uintptr_t Hook_FindBackward(uintptr_t startAddress, size_t searchRange, const char* signature)`
Finds a byte pattern by searching backwards from a starting address.

*   **Parameters:**
    *   `startAddress`: The memory address to start the backward search from.
    *   `searchRange`: The maximum number of bytes to search backwards.
    *   `signature`: The byte pattern to look for.
*   **Returns**: The memory address where the pattern starts, or `0` if not found.

---
### `bool Hook_IsEnabled(SPF_Hook_Handle* h)`
Checks if a hook is currently enabled in the configuration.

---
### `bool Hook_IsInstalled(SPF_Hook_Handle* h)`
Checks if a hook is currently active in memory (i.e., successfully found and installed).

---

## Reflection API (v1.2)

The Reflection API provides high-level access to game object data by leveraging the engine's internal "descriptors". This is the most robust way to access fields because symbolic names (like those in `.sii` files) are used instead of hardcoded offsets.

---
### `uintptr_t Reflection_GetAttributeOffset(const char* className, const char* attributeName)`
Dynamically resolves the byte offset of a class member variable.

*   **Parameters:**
    *   `className`: The internal SCS name of the class (e.g., `"vehicle_interior_camera"`).
    *   `attributeName`: The engine name of the attribute (e.g., `"head_offset"`).
*   **Returns**: The relative offset from the object's base address, or `0` if not found.

---
### `uintptr_t Reflection_ResolveSmartPtr(uintptr_t address)`
Safely dereferences an SCS `smart_ptr` to get the raw underlying object address.

*   **Details**: SCS uses a custom reference-counting smart pointer for many objects (traffic, player truck, etc.). This function handles the dereference logic and validation to safely retrieve the target object's address.

---

## Memory Access

The Hooks API also provides utility functions for safe memory reading, writing, and resolving relative addresses. This is essential for plugins that need to extract data from game structures or global variables found via pattern scanning.

---
### `int32_t Memory_ReadInt32(uintptr_t address)`
Reads a 32-bit signed integer from the specified memory address.

---
### `int8_t Memory_ReadInt8(uintptr_t address)`
Reads an 8-bit signed integer (byte) from the specified memory address.

---
### `int64_t Memory_ReadInt64(uintptr_t address)`
Reads a 64-bit signed integer from the specified memory address. Useful for reading pointers in x64.

---
### `float Memory_ReadFloat(uintptr_t address)`
Reads a 32-bit floating-point value from the specified memory address.

---
### `void Memory_WriteFloat(uintptr_t address, float value)`
Writes a 32-bit floating-point value directly to the specified memory address.

---
### `void Memory_WriteInt32(uintptr_t address, int32_t value)`
Writes a 32-bit signed integer directly to the specified memory address.

---
### `void Memory_ReadVector3(uintptr_t address, float* outX, float* outY, float* outZ)`
Reads three consecutive 32-bit floats from memory into the provided variables. This is the standard format for positions and rotations in the SCS engine.

---
### `void Memory_WriteVector3(uintptr_t address, float x, float y, float z)`
Writes three 32-bit floats to the specified memory address as a 3D vector.

---
### `uintptr_t Memory_GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize)`
Calculates an absolute memory address from an x64 RIP-relative instruction.

*   **Parameters:**
    *   `instructionAddr`: The base address of the instruction (usually returned by `Hook_FindPattern`).
    *   `offsetPos`: The position of the 32-bit displacement value within the instruction bytes.
    *   `instructionSize`: The total length of the instruction in bytes.

---

## Memory Access Example

This example demonstrates how to find a global game variable using a pattern and read its value.

```cpp
// 1. Find the instruction that accesses a global variable
// Example instruction: movss xmm0, [rip + 0x1234]  (Size: 8 bytes, Offset at byte 4)
uintptr_t patternAddr = api->hooks->Hook_FindPattern("F3 0F 10 05 ? ? ? ?");

if (patternAddr != 0) {
    // 2. Resolve the absolute address of the variable
    uintptr_t globalVarAddr = api->hooks->Memory_GetRipAddress(patternAddr, 4, 8);

    // 3. Read the value (e.g., a float)
    float value = api->hooks->Memory_ReadFloat(globalVarAddr);
    Log("Global value is: %f", value);
}
```

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
    // ... logic ...

    // 5. Call the original function using the trampoline. This is CRITICAL.
    o_SomeGameFunction(param1, param2);

    // 6. Your custom logic can also run after the original function returns
}

// 7. Register the hook when the plugin activates
void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_coreAPI = core_api;

    if (s_coreAPI && s_coreAPI->hooks) {
        s_coreAPI->hooks->Hook_Register(
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