#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare the handle type to make it an opaque pointer for the C API
typedef struct SPF_Config_Handle SPF_Config_Handle;
typedef struct SPF_JsonValue_Handle SPF_JsonValue_Handle;

/**
 * @struct SPF_Config_API
 * @brief API for interacting with plugin-specific configuration files.
 *
 * @details This API provides a simple key-value store for plugins to save and
 *          retrieve settings. The configuration is stored as a JSON file in the
 *          plugin's dedicated configuration directory (e.g., `plugins/MyPlugin/config/settings.json`).
 *          The API handles the reading and writing of the file, and provides
 *          type-safe functions for getting and setting values.
 *
 * @section Programmatic vs. UI-Based Configuration
 * Most settings defined in a plugin's manifest can be automatically managed and
 * edited by the user through the framework's main settings UI. To enable this,
 * you only need to set `configPolicy.allowUserConfig = true;` and list the
 * relevant systems (e.g., "settings", "logging") in the `userConfigurableSystems` array
 * in your manifest. 
 * 
 * This `SPF_Config_API` should be used for cases where you need to programmatically
 * get or set configuration values from your C++ code, for example, in response to
 * in-game events, for settings that are not exposed in the UI, or for complex logic.
 *
 * @section Workflow
 * 1.  **Declare in Manifest**: In your plugin's manifest (`GetManifestData`),
 *     set `configPolicy.allowUserConfig = true;`. You can also provide a
 *     default JSON structure for your settings via `settingsJson`.
 * 2.  **Get Context**: In your `OnLoad` function, call `GetContext()` with your
 *     plugin's name to get a configuration handle. This handle is your
 *     gateway to all other config functions.
 * 3.  **Get Values**: Use the getter functions (`GetInt`, `GetString`, etc.) to
 *     read values from the configuration. It's good practice to provide a
 *     default value in case the key doesn't exist in the file.
 * 4.  **Set Values**: Use the setter functions (`SetInt`, `SetString`, etc.) to
 *     change values in memory. The changes will be automatically saved to the
 *     `config.json` file when the game shuts down or when the user clicks
 *     "Save" in the framework's settings UI.
 * 5.  **React to Changes**: (Optional) Implement the `OnSettingChanged` callback
 *     in your plugin's exports. The framework will call this function whenever
 *     a setting is changed, allowing you to react in real-time.
 */
typedef struct SPF_Config_API {
    /**
     * @brief Gets a configuration context handle for the plugin.
     *
     * @details This handle is the primary identifier for all subsequent config
     *          calls. The framework uses it to associate the calls with the correct
     *          `config.json` file for your plugin. The framework manages the
     *          memory of this handle; do not free it.
     *
     * @param pluginName The name of the plugin, which MUST match the `name`
     *                   field in your plugin's manifest.
     * @return A handle to the configuration context, or `NULL` if the plugin
     *         could not be found or is not configured for user settings.
     */
    SPF_Config_Handle* (*GetContext)(const char* pluginName);

    // --- Value Getters ---

    /**
     * @brief Retrieves a string value from the configuration.
     *
     * @param handle The context handle obtained from `GetContext`.
     * @param key The key for the value. This must be a dot-separated path starting
     *            with the configuration system name (e.g., "settings", "localization").
     *            Examples: "settings.some_number", "localization.language".
     * @param defaultValue A default value to return if the key is not found.
     * @param[out] out_buffer A pointer to a character buffer to receive the string.
     * @param buffer_size The size of the output buffer.
     * @return The number of characters written to the buffer (excluding null
     *         terminator). If the return value is >= `buffer_size`, the output
     *         was truncated. Returns the length of `defaultValue` if the key
     *         was not found.
     */
    int (*GetString)(SPF_Config_Handle* handle, const char* key, const char* defaultValue, char* out_buffer, int buffer_size);

    /**
     * @brief Retrieves an integer value from the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param defaultValue A default value to return if the key is not found.
     * @return The integer value from the config, or `defaultValue` if not found.
     */
    int64_t (*GetInt)(SPF_Config_Handle* handle, const char* key, int64_t defaultValue);

    /**
     * @brief Retrieves a 32-bit integer value from the configuration.
     * @param handle The context handle.
     * @param key The key for the value (e.g., "settings.my_count").
     * @param defaultValue A default value to return if the key is not found.
     * @return The 32-bit integer value from the config, or `defaultValue` if not found.
     * @note This is a convenience function for interoperability with APIs that use `int` (like ImGui).
     *       Be aware of potential data truncation if the stored value exceeds the 32-bit integer range.
     */
    int32_t (*GetInt32)(SPF_Config_Handle* handle, const char* key, int32_t defaultValue);

    /**
     * @brief Retrieves a floating-point value from the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param defaultValue A default value to return if the key is not found.
     * @return The double value from the config, or `defaultValue` if not found.
     */
    double (*GetFloat)(SPF_Config_Handle* handle, const char* key, double defaultValue);

    /**
     * @brief Retrieves a boolean value from the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param defaultValue A default value to return if the key is not found.
     * @return The boolean value from the config, or `defaultValue` if not found.
     */
    bool (*GetBool)(SPF_Config_Handle* handle, const char* key, bool defaultValue);

    /**
     * @brief (Advanced) Retrieves a handle to a raw JSON value for a given key.
     *
     * @details This function is intended for advanced use cases where a configuration
     *          value is not a simple type (like an int or bool), but a complex JSON
     *          object or array. The standard GetInt/GetString functions cannot retrieve
     *          such structures. This function provides a way to get a "handle" to the
     *          raw JSON value.
     *
     * @section Intended Workflow
     * This function is designed to be used with the `SPF_JsonReader_API`. The typical
     * workflow is:
     * 1. Retrieve this handle for a complex setting key (e.g., "settings.my_object").
     * 2. Obtain a pointer to the `SPF_JsonReader_API`.
     * 3. Use the functions in `SPF_JsonReader_API` (like `GetType`, `GetString`, etc.)
     *    to parse the data within the handle.
     *
     * @param handle The configuration context handle obtained from `GetContext`.
     * @param key The dot-separated key for the value (e.g., "settings.my_complex_object").
     * @return An opaque handle to the JSON value, or `NULL` if the key is not found.
     *         The lifetime of this handle is managed by the framework; do not free it.
     */
    SPF_JsonValue_Handle* (*GetJsonValueHandle)(SPF_Config_Handle* handle, const char* key);

    // --- Value Setters ---

    /**
     * @brief Sets a string value in the configuration.
     * @details The change is stored in memory and will be persisted to the file
     *          on shutdown or when settings are saved in the UI.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param value The string value to set.
     */
    void (*SetString)(SPF_Config_Handle* handle, const char* key, const char* value);

    /**
     * @brief Sets an integer value in the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param value The integer value to set.
     */
    void (*SetInt)(SPF_Config_Handle* handle, const char* key, int64_t value);

    /**
     * @brief Sets a 32-bit integer value in the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param value The 32-bit integer value to set.
     */
    void (*SetInt32)(SPF_Config_Handle* handle, const char* key, int32_t value);

    /**
     * @brief Sets a floating-point value in the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param value The double value to set.
     */
    void (*SetFloat)(SPF_Config_Handle* handle, const char* key, double value);

    /**
     * @brief Sets a boolean value in the configuration.
     * @param handle The context handle.
     * @param key The key for the value.
     * @param value The boolean value to set.
     */
    void (*SetBool)(SPF_Config_Handle* handle, const char* key, bool value);

} SPF_Config_API;

#ifdef __cplusplus
}
#endif
