/**                                                                                               
* @file SPF_JsonReader_API.h                                                                          
* @brief API for safely reading data from opaque JSON handles.
*                                                                                                 
* @details This API provides a stable, read-only interface for inspecting JSON data
* provided by the framework (e.g., in the 'OnSettingChanged' callback or advanced 
* config lookups). It ensures absolute ABI compatibility by preventing plugins from 
* needing to link against specific JSON libraries or accessing raw internal structures.
*                                                                                                 
* All JSON handles are managed by the framework. Plugins should use this API
* to traverse objects, iterate arrays, and retrieve primitive values safely.                                       
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Opaque Handles**: Data is accessed via 'SPF_JsonValue_Handle'. These handles
*    are references to internal framework objects. Do not attempt to free them.
*                                                                                                 
* 2. **Type-Driven Access**: Always check the node type using 'Json_GetType()' before
*    calling specific getters to avoid receiving default fallback values.
*                                                                                                 
* 3. **Deep Navigation**: Use 'Json_GetMember()' for objects or 'Json_GetArrayItem()'
*    for arrays to obtain handles to nested nodes and traverse complex structures.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // Example: Reading a complex setting like { "color": "red", "intensity": 0.8 }
* void OnSettingChanged(SPF_Config_Handle* config_h, const char* keyPath) {
*     // 1. Get handle from Config API
*     const SPF_JsonValue_Handle* node_h = cfgApi->Cfg_GetJsonValueHandle(config_h, keyPath);
*
*     // 2. Check if it's an object and has required member
*     if (node_h && jsonApi->Json_HasMember(node_h, "intensity")) {
*         const SPF_JsonValue_Handle* val_h = jsonApi->Json_GetMember(node_h, "intensity");
*         
*         // 3. Read the primitive value
*         double intensity = jsonApi->Json_GetFloat(val_h, 1.0);
*     }
* }
* @endcode                                                                                        
*/ 

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief An opaque handle representing a reference to a JSON value.
 * @details The plugin receives this handle from the framework and uses it with the
 *          functions in `SPF_JsonReader_API` to read the underlying data.
 *          The plugin should never attempt to modify or free this handle.
 */
typedef struct SPF_JsonValue_Handle SPF_JsonValue_Handle;

/**
 * @enum SPF_JsonType
 * @brief Represents the data type of a JSON value.
 */
typedef enum {
    SPF_JSON_TYPE_NULL,
    SPF_JSON_TYPE_OBJECT,
    SPF_JSON_TYPE_ARRAY,
    SPF_JSON_TYPE_STRING,
    SPF_JSON_TYPE_BOOLEAN,
    SPF_JSON_TYPE_NUMBER_INTEGER,  // Represents any signed integer type
    SPF_JSON_TYPE_NUMBER_UNSIGNED, // Represents any unsigned integer type
    SPF_JSON_TYPE_NUMBER_FLOAT,    // Represents any floating-point type
    SPF_JSON_TYPE_UNKNOWN
} SPF_JsonType;

/**
 * @struct SPF_JsonReader_API
 * @brief A set of C-style functions for reading values from a JSON object handle.
 *
 * @details This structure contains function pointers that allow a plugin to safely
 *          query the type and value of a JSON object that is owned by the core
 *          framework. This is essential for maintaining ABI stability, as the
 *          plugin does not need to link against the framework's JSON library
 *          or know about its internal data structures (like `nlohmann::ordered_json`).
 *
 * @section Workflow
 * 1. Receive this API as an argument in a callback (e.g., `OnSettingChanged`).
 * 2. When in a callback, check the accompanying `keyPath` parameter to identify the setting.
 * 3. Use `Json_GetType()` to determine the data type of the provided `SPF_JsonValue_Handle`.
 * 4. Call the appropriate getter function (`Json_GetBool`, `Json_GetInt`, etc.) based on the type.
 */
typedef struct SPF_JsonReader_API {
    /**
     * @brief Gets the data type of the JSON value.
     * @param h The handle to the JSON value.
     * @return The SPF_JsonType enum value, allowing you to know which
     *         getter function to call.
     */
    SPF_JsonType (*Json_GetType)(const SPF_JsonValue_Handle* h);

    /**
     * @brief Gets the value as a boolean.
     * @param h The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not boolean.
     * @return The boolean value.
     */
    bool (*Json_GetBool)(const SPF_JsonValue_Handle* h, bool default_value);

    /**
     * @brief Gets the value as a 64-bit signed integer.
     * @param h The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The integer value.
     */
    int64_t (*Json_GetInt)(const SPF_JsonValue_Handle* h, int64_t default_value);

    /**
     * @brief Gets the value as a 32-bit signed integer.
     * @param h The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The 32-bit integer value.
     * @note This is a convenience function. Be aware of potential data truncation.
     */
    int32_t (*Json_GetInt32)(const SPF_JsonValue_Handle* h, int32_t default_value);

    /**
     * @brief Gets the value as a 64-bit unsigned integer.
     * @param h The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The unsigned integer value.
     */
    uint64_t (*Json_GetUint)(const SPF_JsonValue_Handle* h, uint64_t default_value);

    /**
     * @brief Gets the value as a double-precision float.
     * @param h The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The double value.
     */
    double (*Json_GetFloat)(const SPF_JsonValue_Handle* h, double default_value);

    /**
     * @brief Gets the value as a string.
     * @param h The handle to the JSON value.
     * @param[out] out_buffer A buffer to store the string.
     * @param buffer_size The size of the output buffer in bytes.
     * @return The number of characters written (excluding null terminator). If the
     *         return value is >= `buffer_size`, the output was truncated. Returns 0
     *         if the handle is invalid or not a string.
     */
    int (*Json_GetString)(const SPF_JsonValue_Handle* h, char* out_buffer, int buffer_size);

    /**
     * @brief Checks if the JSON object represented by the handle has a specific member.
     * @param h A handle to a JSON value. Must be of type `SPF_JSON_TYPE_OBJECT`.
     * @param memberName The name of the member to check for.
     * @return `true` if the handle is a valid object and contains the member, `false` otherwise.
     */
    bool (*Json_HasMember)(const SPF_JsonValue_Handle* h, const char* memberName);

    /**
     * @brief Retrieves a handle to a member of a JSON object.
     * @param h A handle to a JSON value. Must be of type `SPF_JSON_TYPE_OBJECT`.
     * @param memberName The name of the member to retrieve.
     * @return A new handle to the member's value if found, otherwise `NULL`.
     *         The lifetime of this handle is managed by the framework.
     */
    SPF_JsonValue_Handle* (*Json_GetMember)(const SPF_JsonValue_Handle* h, const char* memberName);

    /**
     * @brief Gets the number of elements in a JSON array.
     * @param h A handle to a JSON value. Must be of type `SPF_JSON_TYPE_ARRAY`.
     * @return The number of elements in the array, or 0 if the handle is not a valid array.
     */
    int (*Json_GetArraySize)(const SPF_JsonValue_Handle* h);

    /**
     * @brief Retrieves a handle to an element at a specific index in a JSON array.
     * @param h A handle to a JSON value. Must be of type `SPF_JSON_TYPE_ARRAY`.
     * @param index The zero-based index of the element to retrieve.
     * @return A new handle to the array element's value if the index is valid, otherwise `NULL`.
     *         The lifetime of this handle is managed by the framework.
     */
    SPF_JsonValue_Handle* (*Json_GetArrayItem)(const SPF_JsonValue_Handle* h, int index);

} SPF_JsonReader_API;

#ifdef __cplusplus
}
#endif
