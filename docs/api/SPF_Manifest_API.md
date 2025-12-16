# The Plugin Manifest

The manifest is the most important part of an SPF plugin. It is a "contract" that your plugin makes with the framework, declaring its identity, features, default settings, and requirements *before* it is fully loaded.

The entire manifest is defined by filling out a single C-style struct, `SPF_ManifestData_C`, within a function that your plugin exports. The framework calls this function at startup to learn about your plugin.

## The Entry Point: `SPF_GetManifestAPI`

Every SPF plugin **must** export a C-function named `SPF_GetManifestAPI`. The framework looks for this function by name when it first loads your DLL.

Your implementation of this function should be simple: you just need to assign your manifest-generating function to the provided API struct.

```c
#include "SPF/SPF_API/SPF_Manifest_API.h"

// Forward declaration of our function that will fill the manifest
void MyPlugin_GetManifestData(SPF_ManifestData_C& out_manifest);

// The main exported function must be wrapped in extern "C"
extern "C" {
    // Use your plugin's export macro, e.g., __declspec(dllexport)
    SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
        if (out_api) {
            out_api->GetManifestData = MyPlugin_GetManifestData;
            return true;
        }
        return false;
    }
}
```

## The `GetManifestData` Function

This is where you will define everything about your plugin. The framework will call this function, passing it a reference to a `SPF_ManifestData_C` struct that you must fill out.

```c
void MyPlugin_GetManifestData(SPF_ManifestData_C& out_manifest) {
    // ... all your definitions go here ...
}
```

### Main Data Blocks

The `SPF_ManifestData_C` struct is composed of several smaller structs.

---
**`info` (`SPF_InfoData_C`)**
Contains essential metadata about your plugin.

*   `name`: A unique programmatic name for your plugin (e.g., `"MyPlugin"`).
*   `version`: The plugin's version string (e.g., `"1.2.0"`).
*   `author`: Your name or your organization's name.
*   `descriptionKey` / `descriptionLiteral`: A description for the UI.

---
**`configPolicy` (`SPF_ConfigPolicyData_C`)**
Defines the plugin's requirements and configuration rules.

*   `allowUserConfig`: Set to `true` to allow users to have their own `settings.json`.
*   `userConfigurableSystems`: An array of strings listing which framework systems (e.g., `"logging"`, `"ui"`) the user can configure for your plugin.
*   `requiredHooks`: A critical array where you list hooks that are essential for your plugin to function (e.g., `"GameConsole"`). The framework will ensure these are always enabled.

---
**`settingsJson` (`const char*`)**
A JSON string literal containing default values for your plugin's custom settings. See the `SPF_Config_API` documentation for more on how this is used.

---
**`keybinds` (`SPF_KeybindsData_C`)**
Defines all the "actions" your plugin provides and their default key combinations. Each action needs a `groupName`, `actionName`, and one or more default key definitions.

---
**`ui` (`SPF_UIData_C`)**
Defines the default state for any UI windows your plugin creates, including their name, default visibility, position, and size.

### The Metadata System

For every data object you define (a keybind, a UI window, a custom setting), you can also provide **metadata**. Metadata consists of a user-friendly title and a detailed description, which the framework uses to build the Settings UI automatically.

You provide metadata by filling out one of the `...Metadata` arrays in the manifest (e.g., `customSettingsMetadata`, `keybindsMetadata`).

Each metadata entry has a `titleKey` and `descriptionKey`. These fields can be used in two ways:

*   **With Localization (Recommended):** Provide a key that matches an entry in your localization files (e.g., `"my_setting.title"`). The framework will look up the translation.
*   **As Literal Text:** Provide the text directly (e.g., `"My Awesome Setting"`). If the framework can't find a translation key with that name, it will fall back to using the string as literal text.

Providing metadata is optional, but **highly recommended** for a good user experience.

## Complete Example

Here is a simplified example of a `GetManifestData` implementation.

```c
#include "SPF/SPF_API/SPF_Manifest_API.h"
#include <string.h> // For strncpy_s or a safe equivalent

void MyPlugin_GetManifestData(SPF_ManifestData_C& out_manifest) {
    // 1. Info
    strncpy_s(out_manifest.info.name, "MyPlugin", sizeof(out_manifest.info.name));
    strncpy_s(out_manifest.info.version, "1.0.0", sizeof(out_manifest.info.version));
    strncpy_s(out_manifest.info.author, "My Name", sizeof(out_manifest.info.author));
    strncpy_s(out_manifest.info.descriptionKey, "myplugin.description", sizeof(out_manifest.info.descriptionKey));

    // 2. Policy
    out_manifest.configPolicy.allowUserConfig = true;
    out_manifest.configPolicy.requiredHooksCount = 1;
    strncpy_s(out_manifest.configPolicy.requiredHooks[0], "GameConsole", sizeof(out_manifest.configPolicy.requiredHooks[0]));

    // 3. Keybind Action
    out_manifest.keybinds.actionCount = 1;
    auto& keybindAction = out_manifest.keybinds.actions[0];
    strncpy_s(keybindAction.groupName, "MyPlugin.UI", sizeof(keybindAction.groupName));
    strncpy_s(keybindAction.actionName, "toggle_window", sizeof(keybindAction.actionName));
    keybindAction.definitionCount = 1;
    strncpy_s(keybindAction.definitions[0].key, "DIK_F6", sizeof(keybindAction.definitions[0].key));
    // It's good practice to fill out all fields, even if with default values
    strncpy_s(keybindAction.definitions[0].type, "keyboard", sizeof(keybindAction.definitions[0].type));
    strncpy_s(keybindAction.definitions[0].pressType, "short", sizeof(keybindAction.definitions[0].pressType));
    strncpy_s(keybindAction.definitions[0].consume, "on_ui_focus", sizeof(keybindAction.definitions[0].consume));
    strncpy_s(keybindAction.definitions[0].behavior, "toggle", sizeof(keybindAction.definitions[0].behavior));

    // 4. Metadata for the Keybind
    out_manifest.keybindsMetadataCount = 1;
    auto& keybindMeta = out_manifest.keybindsMetadata[0];
    strncpy_s(keybindMeta.groupName, "MyPlugin.UI", sizeof(keybindMeta.groupName));
    strncpy_s(keybindMeta.actionName, "toggle_window", sizeof(keybindMeta.actionName));
    strncpy_s(keybindMeta.titleKey, "keybind.toggle_window.title", sizeof(keybindMeta.titleKey));
    strncpy_s(keybindMeta.descriptionKey, "keybind.toggle_window.desc", sizeof(keybindMeta.descriptionKey));
}
```
