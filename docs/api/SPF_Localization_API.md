# SPF Localization API

The SPF Localization API provides a complete system for adding multilingual support to your plugin's UI and messages. It works by loading strings from simple JSON files based on the user's selected language.

## Core Concepts

### File Structure
The framework automatically discovers translation files placed in a `localization` sub-folder within your plugin's main directory. The files should be named with the language code (e.g., "en", "uk", "de").

**Example Structure:**
```
MyPlugin/
├── MyPlugin.dll
└───localization/
    ├── en.json
    └── uk.json
```

### JSON File Format
Your translation files should be simple key-value JSON objects. You can nest objects, and access them using dot notation in your code.

**Example `en.json`:**
```json
{
  "my_window": {
    "title": "My Awesome Plugin Window",
    "buttons": {
      "ok": "OK",
      "cancel": "Cancel"
    }
  },
  "language": {
    "en": "English",
    "uk": "Ukrainian"
  }
}
```


## Getting the API Context

To use the localization API, you first need to get your plugin's unique handle.

```cpp
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Localization_API.h"

const SPF_Localization_API* s_locAPI = NULL;
SPF_Localization_Handle* s_myPluginLoc = NULL;

void MyPlugin_OnActivated(const SPF_Core_API* core_api) {
    s_locAPI = core_api->localization;
    
    if (s_locAPI) {
        s_myPluginLoc = s_locAPI->Loc_GetContext("MyPlugin");
    }
}
```

## Function Reference

---
**`SPF_Localization_Handle* Loc_GetContext(const char* pluginName)`**
Gets a localization context handle for your plugin.
*   **pluginName:** Your plugin's name, which must match the manifest.
*   **Returns:** A handle to the localization context, or `NULL`.

---
**`int Loc_GetString(SPF_Localization_Handle* h, const char* key, char* out_buffer, int buffer_size)`**
Retrieves a translated string from the currently loaded language file.
*   **h:** The context handle obtained from `Loc_GetContext`.
*   **key:** The key for the string (e.g., "my_window.title").
*   **out_buffer:** Buffer to receive the string.
*   **buffer_size:** Size of the output buffer.
*   **Returns:** Number of characters written (excluding null terminator).

---
**`bool Loc_SetLanguage(SPF_Localization_Handle* h, const char* langCode)`**
Changes the active language at runtime. This will load the corresponding file (e.g., `de.json` for `"de"`). The system will fall back to the default language for any keys not found in the new file.

---
**`const char** Loc_GetAvailableLanguages(SPF_Localization_Handle* h, int* count)`**
Gets a list of all language codes discovered in your plugin's `localization/` directory.
*   **count:** A pointer to an `int` that will be filled with the number of languages found.
*   **Returns:** A `const char**` array of language codes. This memory is managed by the framework and should not be modified or freed.

---
**`const char* Loc_GetFrameworkLanguage()`**
Gets the language code currently used by the framework interface (e.g., "en", "uk"). Plugins can use this to automatically synchronize their language with the framework's settings.

---
**`bool Loc_HasLanguage(SPF_Localization_Handle* h, const char* langCode)`**
Checks if a specific translation file (e.g., `uk.json`) exists for this plugin. This is useful for smart synchronization, allowing a plugin to skip switching if it doesn't support the framework's current language.

## Complete Example

This example shows how to get a translated window title and how to create a language selector.

```c
#include <stdio.h> // For snprintf

// Assumes s_locAPI and s_myPluginLoc are initialized.
// Assumes 'ui' is a pointer to the SPF_UI_API.

void RenderMyWindow(const SPF_UI_API* ui) {
    char window_title[128];
    s_locAPI->Loc_GetString(s_myPluginLoc, "my_window.title", window_title, sizeof(window_title));
    
    ui->Begin(window_title, NULL, 0);
    // ... window content ...
    ui->End();
}

void RenderSettings(const SPF_UI_API* ui) {
    int lang_count = 0;
    const char** lang_codes = s_locAPI->Loc_GetAvailableLanguages(s_myPluginLoc, &lang_count);

    // In a real UI, you would use a combo box. Here we just show buttons.
    for (int i = 0; i < lang_count; i++) {
        char lang_display_name[64];
        char lang_key[32];
        
        // Construct the special key "language.[lang_code]" to get the translated language name
        snprintf(lang_key, sizeof(lang_key), "language.%s", lang_codes[i]);
        
        // Get the display name for the language (e.g., "English" for "en")
        s_locAPI->Loc_GetString(s_myPluginLoc, lang_key, lang_display_name, sizeof(lang_display_name));
        
        if (ui->Button(lang_display_name, 0, 0)) {
            // Set the new language when the button is clicked
            s_locAPI->Loc_SetLanguage(s_myPluginLoc, lang_codes[i]);
        }
    }
}
```
