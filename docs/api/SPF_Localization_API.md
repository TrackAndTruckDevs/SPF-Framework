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

To use the API, you must first get your plugin's `SPF_Localization_Handle`.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Localization_API.h"

SPF_Localization_API* s_locAPI = NULL;
SPF_Localization_Handle* s_myPluginLoc = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_locAPI = (SPF_Localization_API*)params->GetAPI(SPF_API_LOCALIZATION);
    
    if (s_locAPI) {
        s_myPluginLoc = s_locAPI->GetContext("MyPlugin");
    }
}
```

## Function Reference

---
**`SPF_Localization_Handle* GetContext(const char* pluginName)`**
Gets a localization context handle for your plugin.

---
**`int GetString(SPF_Localization_Handle* handle, const char* key, char* out_buffer, int buffer_size)`**
Retrieves a translated string from the currently loaded language file.
*   **key:** The key for the string. For nested objects, use dot notation (e.g., `"my_window.buttons.ok"`).
*   **out_buffer:** A buffer to store the resulting string.
*   **Returns:** The number of characters written. A return value `>= buffer_size` indicates truncation.

---
**`bool SetLanguage(SPF_Localization_Handle* handle, const char* langCode)`**
Changes the active language at runtime. This will load the corresponding file (e.g., `de.json` for `"de"`). The system will fall back to the default language for any keys not found in the new file.

---
**`const char** GetAvailableLanguages(SPF_Localization_Handle* handle, int* count)`**
Gets a list of all language codes discovered in your plugin's `localization/` directory.
*   **count:** A pointer to an `int` that will be filled with the number of languages found.
*   **Returns:** A `const char**` array of language codes. This memory is managed by the framework and should not be modified or freed.

## Complete Example

This example shows how to get a translated window title and how to create a language selector.

```c
#include <stdio.h> // For snprintf

// Assumes s_locAPI and s_myPluginLoc are initialized.
// Assumes 'ui' is a pointer to the SPF_UI_API.

void RenderMyWindow(const SPF_UI_API* ui) {
    char window_title[128];
    s_locAPI->GetString(s_myPluginLoc, "my_window.title", window_title, sizeof(window_title));
    
    ui->Begin(window_title, NULL, 0);
    // ... window content ...
    ui->End();
}

void RenderSettings(const SPF_UI_API* ui) {
    int lang_count = 0;
    const char** lang_codes = s_locAPI->GetAvailableLanguages(s_myPluginLoc, &lang_count);

    // In a real UI, you would use a combo box. Here we just show buttons.
    for (int i = 0; i < lang_count; i++) {
        char lang_display_name[64];
        char lang_key[32];
        
        // Construct the special key "language.[lang_code]" to get the translated language name
        snprintf(lang_key, sizeof(lang_key), "language.%s", lang_codes[i]);
        
        // Get the display name for the language (e.g., "English" for "en")
        s_locAPI->GetString(s_myPluginLoc, lang_key, lang_display_name, sizeof(lang_display_name));
        
        if (ui->Button(lang_display_name, 0, 0)) {
            // Set the new language when the button is clicked
            s_locAPI->SetLanguage(s_myPluginLoc, lang_codes[i]);
        }
    }
}
```
