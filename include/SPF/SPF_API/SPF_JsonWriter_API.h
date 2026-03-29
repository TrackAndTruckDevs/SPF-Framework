/**                                                                                               
* @file SPF_JsonWriter_API.h                                                                          
* @brief API for creating and modifying JSON data using opaque handles.
*                                                                                                 
* @details This API provides a way for plugins to build complex JSON structures in memory
*          without needing to link against external JSON libraries. All operations are performed
*          through opaque handles, ensuring ABI stability.
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // Create a JSON object: { "name": "Gemini", "version": 2, "features": ["AI", "Speed"] }
* SPF_JsonValue_Handle* root = writer->Json_CreateObject();
* writer->Json_SetString(root, "name", "Gemini");
* writer->Json_SetInt(root, "version", 2);
* 
* SPF_JsonValue_Handle* features = writer->Json_CreateArray();
* writer->Json_ArrayAppendString(features, "AI");
* writer->Json_ArrayAppendString(features, "Speed");
* 
* writer->Json_SetNode(root, "features", features);
* 
* // Save it using IO_API...
* io->Json_SaveToFile(root, "plugins/MyPlugin/data/info.json", true);
* 
* // Don't forget to destroy the root handle when finished
* writer->Json_DestroyHandle(root);
* @endcode                                                                                        
*/ 

#pragma once

#include "SPF_JsonReader_API.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_JsonWriter_API
 * @brief A set of C-style functions for building and modifying JSON structures.
 */
typedef struct SPF_JsonWriter_API {
    /**
     * @brief Creates an empty JSON object {}.
     * @return A new handle to a JSON object. The plugin is responsible for calling
     *         Json_DestroyHandle if the handle is not passed to the framework for storage.
     */
    SPF_JsonValue_Handle* (*Json_CreateObject)();

    /**
     * @brief Creates an empty JSON array [].
     * @return A new handle to a JSON array.
     */
    SPF_JsonValue_Handle* (*Json_CreateArray)();

    /**
     * @brief Explicitly destroys a JSON handle created by the plugin and frees its memory.
     * @param h The handle to destroy.
     */
    void (*Json_DestroyHandle)(SPF_JsonValue_Handle* h);

    // --- Object Modification (Key-Value) ---

    /**
     * @brief Sets an integer value for a specific key in a JSON object.
     * @param h Handle to a JSON object.
     * @param key The key name.
     * @param value The integer value.
     */
    void (*Json_SetInt)(SPF_JsonValue_Handle* h, const char* key, int64_t value);

    /**
     * @brief Sets a floating-point value for a specific key in a JSON object.
     */
    void (*Json_SetDouble)(SPF_JsonValue_Handle* h, const char* key, double value);

    /**
     * @brief Sets a boolean value for a specific key in a JSON object.
     */
    void (*Json_SetBool)(SPF_JsonValue_Handle* h, const char* key, bool value);

    /**
     * @brief Sets a string value for a specific key in a JSON object.
     */
    void (*Json_SetString)(SPF_JsonValue_Handle* h, const char* key, const char* value);
    
    /**
     * @brief Attaches an existing JSON node (object or array) to another object.
     * @param h Handle to the parent JSON object.
     * @param key The key name for the node.
     * @param nodeHandle Handle to the node to attach.
     */
    void (*Json_SetNode)(SPF_JsonValue_Handle* h, const char* key, SPF_JsonValue_Handle* nodeHandle);

    // --- Array Modification ---

    /**
     * @brief Appends an integer value to the end of a JSON array.
     * @param h Handle to a JSON array.
     * @param value The value to append.
     */
    void (*Json_ArrayAppendInt)(SPF_JsonValue_Handle* h, int64_t value);

    /**
     * @brief Appends a floating-point value to the end of a JSON array.
     */
    void (*Json_ArrayAppendDouble)(SPF_JsonValue_Handle* h, double value);

    /**
     * @brief Appends a boolean value to the end of a JSON array.
     */
    void (*Json_ArrayAppendBool)(SPF_JsonValue_Handle* h, bool value);

    /**
     * @brief Appends a string value to the end of a JSON array.
     */
    void (*Json_ArrayAppendString)(SPF_JsonValue_Handle* h, const char* value);

    /**
     * @brief Appends an existing JSON node (object or array) to the end of an array.
     * @param h Handle to the parent JSON array.
     * @param nodeHandle Handle to the node to append.
     */
    void (*Json_ArrayAppendNode)(SPF_JsonValue_Handle* h, SPF_JsonValue_Handle* nodeHandle);

    // --- Utility Operations ---

    /**
     * @brief Removes a member from a JSON object by its key name.
     */
    void (*Json_RemoveMember)(SPF_JsonValue_Handle* h, const char* key);

    /**
     * @brief Removes an item from a JSON array by its index.
     */
    void (*Json_RemoveArrayItem)(SPF_JsonValue_Handle* h, int index);

    /**
     * @brief Clears all elements/members from a JSON object or array.
     */
    void (*Json_Clear)(SPF_JsonValue_Handle* h);

} SPF_JsonWriter_API;

#ifdef __cplusplus
}
#endif
