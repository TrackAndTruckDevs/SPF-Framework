/**                                                                                               
* @file SPF_Hooks_API.h                                                                          
* @brief API for memory pattern scanning and function hooking.
*                                                                                                 
* @details This API provides plugins with the power to intercept and modify the 
* behavior of the game engine. By using pattern scanning (signatures), hooks 
* remain stable across game updates. It relies on a high-performance detour 
* mechanism to redirect game logic to your custom C++ functions.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Signature Scanning**: Find target functions using unique byte patterns. This 
*    avoids hardcoded memory addresses that change every update. 
*    The pattern scanning syntax supports:
*    - **Exact Bytes**: Standard hex values (e.g., `48 89 5C`).
*    - **Wildcards**: Use `?` or `??` to match any byte (e.g., `48 8B ?? ?? ??`).
*    - **Ranges**: Use `[XX-YY]` to match a byte within a hex range (e.g., `[40-7F]`).
*    - **Variable Wildcards**: Use `[min-max?]` to match a sequence of any bytes with 
*      variable length (e.g., `[1-3?]` matches 1, 2, or 3 bytes). This is crucial for 
*      handling different compiler optimizations or minor code changes between updates.
*                                                                                                 
* 2. **Detours**: Your custom function that executes instead of the original. 
*    It MUST match the original signature exactly.
*                                                                                                 
* 3. **Trampolines**: A pointer to the original game code. Always call the 
*    trampoline from your detour unless you intentionally want to block game logic.
*                                                                                                 
* 4. **ABI Safety**: Hooks are registered via handles, ensuring that adding or 
*    removing hooks does not break compatibility with other plugins.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // 1. Original function pointer
* typedef void (*GameFunc_t)(int);
* GameFunc_t o_GameFunc = nullptr;
*
* // 2. Your detour
* void MyDetour(int val) {
*     Log("Intercepted: %d", val);
*     o_GameFunc(val); // Call original
* }
*
* // 3. Registration
* api->Hook_Register("MyPlugin", "HookID", "Friendly Name", &MyDetour, (void**)&o_GameFunc, "48 89...", true);
* @endcode                                                                                        
*/ 

#pragma once

#include <stdbool.h> // For bool
#include <stdint.h> // For uintptr_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to a registered hook instance.
 */
typedef struct SPF_Hook_Handle SPF_Hook_Handle;

/**
 * @brief Typedef for the function that finds a function by signature (or string reference) and installs a hook.
 *
 * @param pluginName The name of the calling plugin.
 * @param hookName A unique programmatic name for the hook (e.g., "MyPlugin_TrafficHook").
 * @param displayName A user-friendly name for display in UI menus.
 * @param pDetour A pointer to your detour function. This function will be called instead
 *                of the original. It must match the original function's signature.
 * @param ppOriginal A pointer to a function pointer variable. The framework will store the
 *                   address of the trampoline here. You must use this to call the
 *                   original function from your detour.
 * @param signature A string representing the byte pattern for the hook target.
 *                  Can also start with "str:" prefix to look up the function by a unique
 *                  string reference (e.g. "str:[msg] There were at least %u more nested messages...").
 * @param isEnabled The initial enabled state of the hook.
 * @return A handle to the hook, or NULL on failure. The framework manages the memory.
 */
typedef SPF_Hook_Handle* (*SPF_Hook_Register_t)(const char* pluginName, const char* hookName, const char* displayName, void* pDetour, void** ppOriginal, const char* signature, bool isEnabled);

/**
 * @brief Finds a byte pattern in the game's memory.
 *
 * @param signature A string representing the byte pattern.
 *                  Supported formats:
 *                  - `XX`: Exact hex byte (e.g., "48")
 *                  - `?` or `??`: Wildcard (matches any byte)
 *                  - `[XX-YY]`: Range (matches any byte from XX to YY, e.g., "[40-7F]")
 * @return The memory address where the pattern was found, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindPattern_t)(const char* signature);

/**
 * @brief Finds a byte pattern within a specific memory range.
 *
 * @param signature A string representing the byte pattern. Supports wildcards and ranges.
 * @param startAddress The base address to start searching from.
 * @param searchLength The size of the memory block to search.
 * @return The memory address where the pattern was found, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindPatternFrom_t)(const char* signature, uintptr_t startAddress, size_t searchLength);

/**
 * @brief Checks if a hook is currently enabled in the configuration.
 * @param h The handle to the hook instance.
 * @return True if the hook is enabled, false otherwise.
 */
typedef bool (*SPF_Hook_IsEnabled_t)(SPF_Hook_Handle* h);

/**
 * @brief Checks if a hook is currently installed (i.e., active in memory).
 * @param h The handle to the hook instance.
 * @return True if the hook is installed, false otherwise.
 */
typedef bool (*SPF_Hook_IsInstalled_t)(SPF_Hook_Handle* h);

/**
 * @brief Reads a 32-bit integer from a specific memory address.
 * @param address The absolute memory address to read from.
 * @return The 32-bit integer value, or 0 if the address is null.
 */
typedef int32_t (*SPF_Memory_ReadInt32_t)(uintptr_t address);

/**
 * @brief Reads an 8-bit integer (byte) from a specific memory address.
 * @param address The absolute memory address to read from.
 * @return The 8-bit integer value, or 0 if the address is null.
 */
typedef int8_t (*SPF_Memory_ReadInt8_t)(uintptr_t address);

/**
 * @brief Reads a 64-bit integer from a specific memory address.
 * @param address The absolute memory address to read from.
 * @return The 64-bit integer value, or 0 if the address is null.
 */
typedef int64_t (*SPF_Memory_ReadInt64_t)(uintptr_t address);

/**
 * @brief Reads a 32-bit floating-point value from a specific memory address.
 * @param address The absolute memory address to read from.
 * @return The float value, or 0.0f if the address is null.
 */
typedef float (*SPF_Memory_ReadFloat_t)(uintptr_t address);

/**
 * @brief Calculates an absolute address from a RIP-relative instruction (x64).
 *
 * @details In x64 architecture, many instructions use relative addressing from the 
 *          current Instruction Pointer (RIP). This function resolves the final 
 *          absolute address.
 *
 * @param instructionAddr The starting address of the instruction (e.g., found via pattern).
 * @param offsetPos The byte offset within the instruction where the 32-bit displacement is located.
 * @param instructionSize The total size of the instruction in bytes.
 * @return The resolved absolute address.
 *
 * @code
 * // Example: mov rax, [rip + 0x123456] -> instruction is 7 bytes, offset starts at byte 3
 * uintptr_t addr = api->Memory_GetRipAddress(patternAddr, 3, 7);
 * @endcode
 */
typedef uintptr_t (*SPF_Memory_GetRipAddress_t)(uintptr_t instructionAddr, int offsetPos, int instructionSize);

/**
 * @brief Finds a byte pattern by searching backwards from a starting address.
 * 
 * @param startAddress The address to start the backward search from.
 * @param searchRange The maximum number of bytes to search backwards.
 * @param signature The byte pattern to look for.
 * @return The memory address where the pattern starts, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindBackward_t)(uintptr_t startAddress, size_t searchRange, const char* signature);

/**
 * @brief Finds the address of a null-terminated string in the game module.
 * @param str The string to look for.
 * @return The memory address of the string, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindString_t)(const char* str);

/**
 * @brief Finds a function address based on a string it contains, with optional context.
 * @param str The string used by the target function.
 * @param findStart If true, will automatically backtrack to the function prologue.
 * @param contextSig Optional additional signature to match near the string reference to disambiguate.
 * @param contextRange The search window size (in bytes) for the context signature.
 * @return The function address, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindFunctionByString_t)(const char* str, bool findStart, const char* contextSig, size_t contextRange);

/**
 * @brief Finds the starting address of a function containing the given address.
 *
 * @details Uses Windows Runtime Function Tables (.pdata) for 100% accuracy on x64.
 *          This is the most reliable way to find function boundaries without heuristics.
 *
 * @param address Any address within the function (e.g., found via pattern).
 * @return The address of the function's first instruction, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_GetFunctionStart_t)(uintptr_t address);

/**
 * @brief Finds the end address of a function containing the given address.
 *
 * @details Uses Windows Runtime Function Tables (.pdata) for 100% accuracy on x64.
 *          Returns the address of the byte immediately following the last instruction.
 *
 * @param address Any address within the function (e.g., found via pattern).
 * @return The address immediately following the function, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_GetFunctionEnd_t)(uintptr_t address);

/**
 * @brief Finds a sequence of patterns that appear close to each other.
 *
 * @details Useful when the compiler inserts padding, NOPs, or minor logic (like log calls)
 *          between key instructions that you want to match.
 *
 * @param signatures An array of signature strings to find in order.
 * @param count The number of signatures in the array.
 * @param maxGap The maximum number of bytes allowed between each pattern match.
 * @param startAddress Optional address to start searching from. If 0, searches entire module.
 * @param searchRange Optional range limit. If 0 and startAddress is set, tries to detect function end.
 * @return The address where the FIRST pattern in the chain starts, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindChain_t)(const char** signatures, size_t count, size_t maxGap, uintptr_t startAddress, size_t searchRange);

/**
 * @brief Extracts a VTable address from an instruction that references it.
 * 
 * @param signature A signature that matches the instruction referencing the VTable (e.g., LEA RAX, [RIP+...]).
 * @param offsetPos The byte position of the 32-bit displacement within the instruction.
 * @param instructionSize The total size of the instruction in bytes.
 * @return The absolute address of the VTable, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindVTable_t)(const char* signature, int offsetPos, int instructionSize);

/**
 * @brief Gets a function address from a VTable by its index.
 * 
 * @param vtableAddr The absolute address of the VTable.
 * @param index The 0-based index of the function in the table.
 * @return The absolute address of the function, or 0 if invalid.
 */
typedef uintptr_t (*SPF_Hook_GetVTableFunction_t)(uintptr_t vtableAddr, int index);

/**
 * @brief Finds a function that references a specific 32-bit constant value.
 * 
 * @details Useful for finding math-heavy functions or those using unique "magic numbers".
 * 
 * @param constant The 32-bit value to look for (e.g., 0x3C888889).
 * @param findStart If true, the scanner will backtrack to the beginning of the function.
 * @return The function start or reference address, or 0 if not found.
 */
typedef uintptr_t (*SPF_Hook_FindFunctionByConstant_t)(uint32_t constant, bool findStart);

/**
 * @brief Finds an attribute offset for a class using SCS reflection.
 * 
 * @details This allows you to dynamically find member variables in SCS objects using 
 *          the same names you see in .sii files or class descriptors. 
 *          Requires the "Harvesting" system to be active in the framework.
 * 
 * @param className The name of the SCS class (e.g., "vehicle_interior_camera").
 * @param attributeName The name of the attribute (e.g., "head_offset").
 * @return The relative byte offset of the attribute, or 0 if not found.
 * 
 * @code
 * // Example: Find FOV offset in the camera manager
 * uintptr_t offset = api->Reflection_GetAttributeOffset("core_camera", "camera_fov");
 * if (offset > 0) {
 *     float currentFov = api->Memory_ReadFloat(cameraAddr + offset);
 * }
 * @endcode
 */
typedef uintptr_t (*SPF_Reflection_GetAttributeOffset_t)(const char* className, const char* attributeName);

/**
 * @brief Resolves an SCS smart_ptr to its raw underlying object address.
 * 
 * @details Many SCS game objects (like traffic vehicles or player trucks) are stored 
 *          using a custom smart_ptr implementation. This function handles the 
 *          dereferencing and safety checks to get you the real memory address.
 * 
 * @param address The address of the smart_ptr object in memory.
 * @return The absolute address of the underlying object, or 0 if null/invalid.
 */
typedef uintptr_t (*SPF_Reflection_ResolveSmartPtr_t)(uintptr_t address);

/**
 * @brief Writes a 32-bit floating-point value to memory.
 * @param address The absolute memory address to write to.
 * @param value The float value to write.
 */
typedef void (*SPF_Memory_WriteFloat_t)(uintptr_t address, float value);

/**
 * @brief Writes a 32-bit integer value to memory.
 * @param address The absolute memory address to write to.
 * @param value The 32-bit integer value to write.
 */
typedef void (*SPF_Memory_WriteInt32_t)(uintptr_t address, int32_t value);

/**
 * @brief Reads a 3-component vector (float3) from memory.
 * 
 * @details SCS games use float3 structures (X, Y, Z) for positions, rotations, and scales.
 * 
 * @param address The start address of the vector in memory.
 * @param outX Pointer to store the X component.
 * @param outY Pointer to store the Y component.
 * @param outZ Pointer to store the Z component.
 */
typedef void (*SPF_Memory_ReadVector3_t)(uintptr_t address, float* outX, float* outY, float* outZ);

/**
 * @brief Writes a 3-component vector (float3) to memory.
 * 
 * @param address The start address of the vector in memory.
 * @param x The X value to write.
 * @param y The Y value to write.
 * @param z The Z value to write.
 */
typedef void (*SPF_Memory_WriteVector3_t)(uintptr_t address, float x, float y, float z);


/**
 * @struct SPF_Hooks_API
 * @brief C-style API for finding memory patterns and hooking game functions.
 *
 * @details This API provides plugins with the ability to intercept and execute custom
 *          code before, after, or instead of a native game function. It uses
 *          pattern scanning to find functions in memory and relies on MinHook
 *          for creating the low-level hooks.
 *
 * @section Core Concepts
 * 1.  **Signature**: A unique sequence of bytes representing the beginning of a
 *     function in memory. This is used to find the function's address, which
 *     can change between game updates. Supports exact bytes, wildcards (`?` or `??`),
 *     and hex ranges (`[XX-YY]`) for advanced instruction matching.
 * 2.  **Detour**: Your C++ function that will be executed when the game tries to
 *     call the original function. It MUST have the exact same function signature
 *     (calling convention, parameters, and return type) as the function you are hooking.
 * 3.  **Trampoline (`ppOriginal`)**: A pointer to a small piece of executable code
 *     generated by the hooking library. This trampoline, when called, executes the
 *     original game function. You **must** call the original function via this
 *     pointer from your detour to ensure the game continues to function correctly.
 *
 * @section Workflow
 * 1.  **Find Signature**: Using a disassembler or memory scanner (like Ghidra, x64dbg,
 *     or Cheat Engine), find the target function and identify a unique byte
 *     pattern at its start.
 * 2.  **Define Function Type**: In your C++ code, define a type for a function
 *     pointer that matches the original function's signature.
 * 3.  **Implement Detour**: Write your detour function. It should accept the same
 *     parameters and return the same type as the original. Inside this function,
 *     you can add your custom logic.
 * 4.  **Call the Original**: Inside your detour, you must call the original function
 *     using the trampoline pointer provided by the `Hook_Register` function. This is
 *     critical to avoid crashing the game.
 * 5.  **Register Hook**: In your plugin's `OnActivated` function, call `Hook_Register`,
 *     providing the signature, pointers to your detour and trampoline, and other
 *     metadata. The framework will then find the function and install the hook.
 *
 * @section Example
 * @code{.cpp}
 * // 1. Define the original function's signature
 * using MyFunction_t = void(*)(int, bool);
 *
 * // 2. Create a global pointer for the trampoline
 * static MyFunction_t o_MyFunction = nullptr;
 *
 * // 3. Implement your detour
 * void Detour_MyFunction(int param1, bool param2) {
 *     // Your custom logic here...
 *     Log("MyFunction was called!");
 *
 *     // 4. Call the original function
 *     return o_MyFunction(param1, param2);
 * }
 *
 * // 5. In OnActivated:
 * void OnActivated(const SPF_Core_API* core) {
 *     core->hooks->Hook_Register(
 *         "MyPlugin",
 *         "MyFunctionHook",
 *         "My Function Hook",
 *         &Detour_MyFunction,
 *         (void**)&o_MyFunction,
 *         "48 89 5C 24 ? 57 48 83 EC 60",
 *         true
 *     );
 * }
 * @endcode
 */
// The API struct that the framework provides to the plugin.
typedef struct {
    /**
     * @brief Finds a function by its byte signature and installs a hook.
     *
     * @param pluginName The name of the calling plugin.
     * @param hookName A unique programmatic name for the hook (e.g., "MyPlugin_TrafficHook").
     * @param displayName A user-friendly name for display in UI menus.
     * @param pDetour A pointer to your detour function. This function will be called instead
     *                of the original. It must match the original function's signature.
     * @param ppOriginal A pointer to a function pointer variable. The framework will store the
     *                   address of the trampoline here. You must use this to call the
     *                   original function from your detour.
     * @param signature A string representing the byte pattern for the hook target.
     * @param isEnabled The initial enabled state of the hook.
     * @return A handle to the hook, or NULL on failure. The framework manages the memory.
     */
    SPF_Hook_Register_t Hook_Register;
    SPF_Hook_FindPattern_t Hook_FindPattern;

    /**
     * @brief Finds a byte pattern within a specific memory range.
     *
     * @param signature A string representing the byte pattern.
     * @param startAddress The base address to start searching from.
     * @param searchLength The size of the memory block to search.
     * @return The memory address where the pattern was found, or 0 if not found.
     */
    SPF_Hook_FindPatternFrom_t Hook_FindPatternFrom;

    SPF_Hook_IsEnabled_t Hook_IsEnabled;
    SPF_Hook_IsInstalled_t Hook_IsInstalled;

    /**
     * @brief Safe memory reading and RIP-relative address resolution.
     * @section Memory Access API
     */
    SPF_Memory_ReadInt32_t Memory_ReadInt32;
    SPF_Memory_ReadInt8_t Memory_ReadInt8;
    SPF_Memory_ReadInt64_t Memory_ReadInt64;
    SPF_Memory_ReadFloat_t Memory_ReadFloat;
    SPF_Memory_GetRipAddress_t Memory_GetRipAddress;

    /**
     * @brief Finds a byte pattern by searching backwards from a starting address.
     */
    SPF_Hook_FindBackward_t Hook_FindBackward;

    /**
     * @brief Finds the memory address of a null-terminated string in the module.
     * @param str The string to search for.
     * @return The absolute address of the string, or 0 if not found.
     */
    SPF_Hook_FindString_t Hook_FindString;

    /**
     * @brief Locates a function based on a string reference it contains.
     * @param str The string to look for inside the function.
     * @param findStart If true, the scanner will automatically backtrack to the function prologue.
     * @param contextSig Optional byte signature to match near the string reference for disambiguation.
     * @param contextRange The search window size (in bytes) for the context signature.
     * @return The start address of the function, or 0 if not found.
     */
    SPF_Hook_FindFunctionByString_t Hook_FindFunctionByString;

    /**
     * @brief Finds the starting address of a function containing the given address.
     * 
     * @details This is the most reliable way to find the beginning of a function in x64.
     *          It uses the Windows Runtime Function Tables (.pdata), which are 100% accurate
     *          as they are required for system stack unwinding.
     * 
     * @param address Any memory address within the function (e.g., from a pattern match).
     * @return The absolute address of the function's first instruction, or 0 if not found.
     */
    SPF_Hook_GetFunctionStart_t Hook_GetFunctionStart;

    /**
     * @brief Finds the end address of a function containing the given address.
     * 
     * @details Uses Windows Runtime Function Tables (.pdata) for 100% accuracy on x64.
     *          Returns the address of the byte immediately following the last instruction.
     * 
     * @param address Any memory address within the function.
     * @return The absolute address immediately following the function, or 0 if not found.
     */
    SPF_Hook_GetFunctionEnd_t Hook_GetFunctionEnd;

    /**
     * @brief Finds a sequence of patterns that appear close to each other in memory.
     * 
     * @details Use this to create "logical chains" of instructions. It's much more stable
     *          than a single long signature because it can skip variable-length gaps 
     *          inserted by the compiler (like padding, log calls, or minor logic changes).
     * 
     * @param signatures An array of C-strings containing the signatures to find in order.
     * @param count The number of signatures provided in the array.
     * @param maxGap The maximum number of bytes allowed between each successful pattern match.
     * @param startAddress Optional address to begin the search. If 0, the entire module is scanned.
     * @param searchRange Optional range limit. If 0 and startAddress is set, defaults to 4KB (covers most functions).
     * @return The memory address where the FIRST signature in the chain starts, or 0 if not found.
     */
    SPF_Hook_FindChain_t Hook_FindChain;

    /**
     * @brief Finds a VTable address by looking for an instruction that references it.
     * @param signature Byte pattern for the instruction (e.g., "48 8D 05").
     * @param offsetPos Byte offset within the instruction where the 32-bit displacement starts.
     * @param instructionSize Total length of the instruction in bytes.
     * @return The absolute address of the VTable, or 0 if not found.
     */
    SPF_Hook_FindVTable_t Hook_FindVTable;

    /**
     * @brief Gets a function address from a Virtual Function Table (VTable) by index.
     * @param vtableAddr Absolute address of the VTable.
     * @param index 0-based index of the function pointer.
     * @return The absolute address of the function, or 0 if not found.
     */
    SPF_Hook_GetVTableFunction_t Hook_GetVTableFunction;

    /**
     * @brief Locates a function that uses a specific 32-bit constant (magic number).
     * @param constant The 32-bit value to search for (e.g., 0x3C888889).
     * @param findStart If true, automatically returns the function prologue address.
     * @return The function or reference address, or 0 if not found.
     */
    SPF_Hook_FindFunctionByConstant_t Hook_FindFunctionByConstant;

    /**
     * @name Reflection API (v1.2)
     * @{
     */

    /**
     * @brief Dynamically finds the byte offset of a class member using SCS reflection.
     * @see SPF_Reflection_GetAttributeOffset_t
     */
    SPF_Reflection_GetAttributeOffset_t Reflection_GetAttributeOffset;

    /**
     * @brief Safely dereferences an SCS smart_ptr to get the underlying object address.
     * @see SPF_Reflection_ResolveSmartPtr_t
     */
    SPF_Reflection_ResolveSmartPtr_t Reflection_ResolveSmartPtr;

    /** @} */

    /**
     * @name Advanced Memory API (v1.2)
     * @{
     */

    /**
     * @brief Writes a float value to the specified memory address.
     */
    SPF_Memory_WriteFloat_t Memory_WriteFloat;

    /**
     * @brief Writes a 32-bit integer to the specified memory address.
     */
    SPF_Memory_WriteInt32_t Memory_WriteInt32;

    /**
     * @brief Reads a 3D vector (X, Y, Z) from memory into separate variables.
     */
    SPF_Memory_ReadVector3_t Memory_ReadVector3;

    /**
     * @brief Writes X, Y, Z components to a memory location as a 3D vector.
     */
    SPF_Memory_WriteVector3_t Memory_WriteVector3;

} SPF_Hooks_API;

#ifdef __cplusplus
}
#endif