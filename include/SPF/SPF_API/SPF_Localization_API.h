#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare the handle type to make it an opaque pointer for the C API
typedef struct SPF_Localization_Handle SPF_Localization_Handle;

/**
 * @struct SPF_Localization_API
 * @brief API for the framework's localization system.
 *
 * @details This API allows a plugin to load and use translation files, enabling
 *          multilingual support for its UI and messages. The system works by
 *          associating a plugin with a set of translation files (e.g., JSON)
 *          and providing functions to retrieve strings based on a key.
 *
 * @section Workflow
 * 1.  **File Structure**: Place your translation files in a dedicated folder
 *     within your plugin's directory, typically `localization/`. For example:
 *     - `MyPlugin/localization/en.json`
 *     - `MyPlugin/localization/uk.json`
 * 2.  **Manifest**: Specify the default language in your plugin's manifest, e.g., `"en"`.
 * 3.  **Get Context**: In your `OnLoad` function, call `GetContext()` to get a
 *     handle for your plugin.
 * 4.  **Get Strings**: Use `GetString()` with the handle and a key to retrieve
 *     translated strings.
 * 5.  **Change Language**: (Optional) Use `SetLanguage()` to change the active
 *     language at runtime.
 *
 * @section LanguageDisplayNames Providing Language Display Names
 * When displaying a list of available languages in the UI (e.g., in the settings panel),
 * the framework automatically constructs translation keys for each language code.
 *
 * For a language with code `[lang_code]` (e.g., "en", "uk"), the system will look for
 * a key in the format `language.[lang_code]` (e.g., "language.en", "language.uk").
 *
 * To ensure your language names are properly translated in the UI, you must include
 * these keys in your component's localization files (e.g., `MyPlugin/localization/en.json`).
 *
 * @b Example:
 * In your `[component]/localization/en.json` file:
 * @code{.json}
 * {
 *   "language": {
 *     "en": "English",
 *     "uk": "Ukrainian",
 *     "fr": "French"
 *   }
 * }
 * @endcode
 *
 * The framework will then use these entries to display "English", "Ukrainian",
 * or "French" in the UI dropdowns, instead of just the raw language codes "en", "uk", "fr".
 * Each component (framework or plugin) is responsible for providing its own translations
 * for these language display names.
 */
typedef struct SPF_Localization_API {
    /**
     * @brief Gets a localization context handle for the plugin.
     *
     * @details This handle is the primary identifier for all subsequent localization
     *          calls. The framework uses it to associate the calls with the correct
     *          set of translation files that it has discovered for your plugin.
     *          The framework manages the memory of this handle; do not free it.
     *
     * @param pluginName The name of the plugin, which MUST match the `name`
     *                   field in your plugin's manifest.
     * @return A handle to the localization context, or `NULL` if the plugin
     *         could not be found.
     */
    SPF_Localization_Handle* (*GetContext)(const char* pluginName);

    /**
     * @brief Sets the active language for the component associated with the handle.
     *
     * @details This function instructs the framework to load the specified language
     *          file (e.g., `uk.json`). Subsequent calls to `GetString` will attempt
     *          to find keys in this new language file. If a key is not found in the
     *          new language, the system will fall back to the default language
     *          specified in the manifest.
     *
     * @param handle The context handle obtained from `GetContext`.
     * @param langCode The language code (e.g., "en", "uk"). This should match the
     *                 name of your translation file without the extension.
     * @return `true` on success, `false` if the language file (or its fallback)
     *         could not be loaded.
     */
    bool (*SetLanguage)(SPF_Localization_Handle* handle, const char* langCode);

    /**
     * @brief Gets a translated string from the currently loaded language file.
     *
     * @details This is the main function for retrieving localized text. It looks for
     *          the provided key in the active language file and, if not found, in the
     *          default fallback language file.
     *
     * @param handle The context handle obtained from `GetContext`.
     * @param key The key for the string (e.g., "my_window.title"). For nested JSON
     *            objects, use dot notation (e.g., "menu.main.title").
     * @param out_buffer A pointer to a character buffer to receive the string.
     * @param buffer_size The size of the output buffer.
     * @return The number of characters written to the buffer (excluding null terminator).
     *         If the return value is greater than or equal to `buffer_size`, it means
     *         the output was truncated. Returns 0 if the key was not found.
     */
    int (*GetString)(SPF_Localization_Handle* handle, const char* key, char* out_buffer, int buffer_size);

    /**
     * @brief Gets a list of available language codes discovered for this plugin.
     *
     * @details The framework automatically discovers translation files in the plugin's
     *          `localization/` directory. This function returns the list of languages
     *          it found (based on the filenames).
     *
     * @param handle The context handle.
     * @param[out] count A pointer to an integer that will be filled with the number of
     *                   languages found.
     * @return An array of C-strings with the language codes (e.g., "en", "uk").
     *         The memory for this array and its strings is managed by the framework
     *         and should not be modified or freed.
     */
    const char** (*GetAvailableLanguages)(SPF_Localization_Handle* handle, int* count);

} SPF_Localization_API;

#ifdef __cplusplus
}
#endif
