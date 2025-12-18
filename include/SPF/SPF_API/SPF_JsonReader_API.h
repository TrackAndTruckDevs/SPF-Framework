/**
 * @file SPF_JsonReader_API.h
 * @brief Defines a stable C-API for reading data from a JSON object handle.
 *
 * @details This API is provided by the framework to plugins in specific callbacks
 *          (like `OnSettingChanged`) where the plugin needs to inspect a JSON
 *          value without the risks of ABI incompatibility. It provides a safe,
 *          read-only view of a JSON object managed by the core framework.
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
 *          or know about its internal data structures (like `nlohmann::json`).
 *
 * @section Workflow
 * 1. Receive this API as an argument in a callback (e.g., `OnSettingChanged`).
 * 2. When in a callback, check the accompanying `keyPath` parameter to identify the setting.
 *    Note that this key path is the full path, including the system name (e.g., "settings.some_number").
 * 3. Use `GetType()` to determine the data type of the provided `SPF_JsonValue_Handle`.
 * 4. Call the appropriate getter function (`GetBool`, `GetInt`, etc.) based on the type.
 */
typedef struct SPF_JsonReader_API {
    /**
     * @brief Gets the data type of the JSON value.
     * @param handle The handle to the JSON value.
     * @return The SPF_JsonType enum value, allowing you to know which
     *         getter function to call.
     */
    SPF_JsonType (*GetType)(const SPF_JsonValue_Handle* handle);

    /**
     * @brief Gets the value as a boolean.
     * @param handle The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not boolean.
     * @return The boolean value.
     */
    bool (*GetBool)(const SPF_JsonValue_Handle* handle, bool default_value);

    /**
     * @brief Gets the value as a 64-bit signed integer.
     * @param handle The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The integer value.
     */
    int64_t (*GetInt)(const SPF_JsonValue_Handle* handle, int64_t default_value);

    /**
     * @brief Gets the value as a 32-bit signed integer.
     * @param handle The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The 32-bit integer value.
     * @note This is a convenience function. Be aware of potential data truncation.
     */
    int32_t (*GetInt32)(const SPF_JsonValue_Handle* handle, int32_t default_value);

    /**
     * @brief Gets the value as a 64-bit unsigned integer.
     * @param handle The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The unsigned integer value.
     */
    uint64_t (*GetUint)(const SPF_JsonValue_Handle* handle, uint64_t default_value);

    /**
     * @brief Gets the value as a double-precision float.
     * @param handle The handle to the JSON value.
     * @param default_value The value to return if the handle is invalid or the
     *                      type is not a compatible number.
     * @return The double value.
     */
    double (*GetFloat)(const SPF_JsonValue_Handle* handle, double default_value);

    /**
     * @brief Gets the value as a string.
     * @param handle The handle to the JSON value.
     * @param[out] out_buffer A buffer to store the string.
     * @param buffer_size The size of the output buffer.
     * @return The number of characters written (excluding null terminator). If the
     *         return value is >= `buffer_size`, the output was truncated. Returns 0
     *         if the handle is invalid or not a string.
     */
    int (*GetString)(const SPF_JsonValue_Handle* handle, char* out_buffer, int buffer_size);

    /**
     * @brief Checks if the JSON object represented by the handle has a specific member.
     * @param handle A handle to a JSON value. Must be of type `SPF_JSON_TYPE_OBJECT`.
     * @param memberName The name of the member to check for.
     * @return `true` if the handle is a valid object and contains the member, `false` otherwise.
     */
    bool (*HasMember)(const SPF_JsonValue_Handle* handle, const char* memberName);

    /**
     * @brief Retrieves a handle to a member of a JSON object.
     * @param handle A handle to a JSON value. Must be of type `SPF_JSON_TYPE_OBJECT`.
     * @param memberName The name of the member to retrieve.
     * @return A new handle to the member's value if found, otherwise `NULL`.
     *         The lifetime of this handle is managed by the framework.
     */
    SPF_JsonValue_Handle* (*GetMember)(const SPF_JsonValue_Handle* handle, const char* memberName);

    /**
     * @brief Gets the number of elements in a JSON array.
     * @param handle A handle to a JSON value. Must be of type `SPF_JSON_TYPE_ARRAY`.
     * @return The number of elements in the array, or 0 if the handle is not a valid array.
     */
    int (*GetArraySize)(const SPF_JsonValue_Handle* handle);

    /**
     * @brief Retrieves a handle to an element at a specific index in a JSON array.
     * @param handle A handle to a JSON value. Must be of type `SPF_JSON_TYPE_ARRAY`.
     * @param index The zero-based index of the element to retrieve.
     * @return A new handle to the array element's value if the index is valid, otherwise `NULL`.
     *         The lifetime of this handle is managed by the framework.
     */
    SPF_JsonValue_Handle* (*GetArrayItem)(const SPF_JsonValue_Handle* handle, int index);

} SPF_JsonReader_API;

#ifdef __cplusplus
}
#endif
