/**                                                                                               
* @file SPF_JsonIO_API.h                                                                          
* @brief API for loading, saving, and parsing JSON data.
*                                                                                                 
* @details This API acts as the bridge between JSON structures in memory (represented by
*          SPF_JsonValue_Handle) and external sources like files or strings. 
*/ 

#pragma once

#include "SPF_JsonReader_API.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_JsonIO_API
 * @brief Functions for JSON serialization and deserialization.
 */
typedef struct SPF_JsonIO_API {
    /**
     * @brief Parses a JSON string and returns a handle to the resulting structure.
     * @param jsonString A null-terminated string containing valid JSON.
     * @return A handle to the JSON root (object or array), or NULL if parsing fails.
     */
    SPF_JsonValue_Handle* (*Json_ParseString)(const char* jsonString);

    /**
     * @brief Loads a JSON file from disk and parses it.
     * @param filePath The full path to the JSON file.
     * @return A handle to the JSON root, or NULL if the file couldn't be read or parsed.
     */
    SPF_JsonValue_Handle* (*Json_LoadFromFile)(const char* filePath);

    /**
     * @brief Serializes a JSON structure to a string.
     * @param h Handle to the JSON root.
     * @param prettyPrint If true, the output string will be formatted with indentation.
     * @param[out] out_buffer Buffer to store the resulting JSON string.
     * @param buffer_size Size of the output buffer.
     * @return Number of characters written. Truncated if >= buffer_size.
     */
    int (*Json_ToString)(const SPF_JsonValue_Handle* h, bool prettyPrint, char* out_buffer, int buffer_size);

    /**
     * @brief Saves a JSON structure to a file on disk.
     * @param h Handle to the JSON root.
     * @param filePath The full path where the file should be saved.
     * @param prettyPrint If true, the file will be formatted with indentation.
     * @return true if the file was saved successfully, false otherwise.
     */
    bool (*Json_SaveToFile)(const SPF_JsonValue_Handle* h, const char* filePath, bool prettyPrint);

    /**
     * @brief Validates if a string is a valid JSON.
     * @param jsonString The string to validate.
     * @return true if the string is valid JSON, false otherwise.
     */
    bool (*Json_IsValid)(const char* jsonString);

} SPF_JsonIO_API;

#ifdef __cplusplus
}
#endif
