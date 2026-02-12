# Plugin Manifest API

The Manifest is the most critical part of an SPF plugin. It serves as a "contract" between your plugin and the framework, declaring its identity, features, default settings, and requirements *before* the plugin is fully loaded.

In the current architecture, the manifest is defined using a **Builder API**. Instead of filling a static memory structure, the plugin invokes a series of functions provided by the framework to describe its parameters. This ensures absolute ABI stability and allows for easy integration with various programming languages.

## The Entry Point: `SPF_GetManifestAPI`

Every SPF plugin **must** export a C-compatible function named `SPF_GetManifestAPI`. The framework looks for this function by name when it first attempts to load your DLL.

Your implementation must assign your manifest construction function (e.g., `BuildManifest`) to the provided API structure.

```c
#include "SPF/SPF_API/SPF_Manifest_API.h"

// Prototype of your manifest construction function
void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api);

extern "C" {
    SPF_PLUGIN_EXPORT bool SPF_GetManifestAPI(SPF_Manifest_API* out_api) {
        if (out_api) {
            out_api->BuildManifest = MyPlugin_BuildManifest;
            return true;
        }
        return false;
    }
}
```

## The `BuildManifest` Function

This is where you define your plugin's properties. The framework invokes this function, passing two arguments:
1.  **`h` (Handle)**: An opaque pointer to the manifest object in the framework's memory.
2.  **`api`**: A table of function pointers used to populate the data.

```c
void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // api->... calls go here
}
```

---


## 1. Plugin Identity (`Info_...`)

These functions define how your plugin is identified and displayed in the UI.

*   `Info_SetName(h, "Name")`: Unique programmatic ID. No spaces. Used for folder names and logs.
*   `Info_SetVersion(h, "1.0.0")`: Plugin version string.
*   `Info_SetMinFrameworkVersion(h, "1.0.6")`: Minimum SPF version required to load this plugin.
*   `Info_SetAuthor(h, "Author Name")`: Your name or organization.
*   `Info_SetDescriptionLiteral(h, "Text")`: A plain-text description of the plugin.
*   **Social Links**: Dedicated functions are available for Discord, GitHub, Youtube, Steam, Patreon, SCS Forum, and Website URLs.


## 2. Configuration Policy (`Policy_...`)

Defines rules for interacting with the framework's configuration engine.

*   `Policy_SetAllowUserConfig(h, true)`: If enabled, the framework manages a `settings.json` file for the plugin and enables the Settings UI.
*   `Policy_AddConfigurableSystem(h, "system")`: Adds a specific tab to the plugin's settings menu.
    *   **Valid Values**: `"settings"` (Custom variables), `"logging"`, `"localization"`, `"ui"`.
*   `Policy_AddRequiredHook(h, "HookName")`: Declares a mandatory dependency on a hook. The framework will force-enable this hook.


## 3. Custom Settings Defaults (`Settings_SetJson`)

This function defines the structure and default values for your plugin-specific variables.

```cpp
const char* defaults = R"json({
    "enable_feature_x": true,
    "power_level": 50,
    "ui_colors": { "main": [1.0, 0.0, 0.0] }
})";
api->Settings_SetJson(h, defaults);
```
*Note: This data is placed strictly within the `"settings": { ... }` section of your configuration file.*


## 4. System Defaults (`Defaults_...`)

*   **Logging**: `Defaults_SetLogging(h, level, fileSink)`
    *   `level`: `"trace"`, `"debug"`, `"info"`, `"warn"`, `"error"`, `"critical"`.
    *   `fileSink`: `true` to enable a dedicated log file for the plugin.
*   **Localization**: `Defaults_SetLocalization(h, "en")`
    *   Sets the default initial language.
*   **Keybinds**: `Defaults_AddKeybind(h, group, action, type, key, consume)`
    *   **Universal Signature**: Since version 1.0.7, this function only takes the identity of the bind. All processing parameters are set to sensible framework defaults and can be further tuned by the user in the UI.
    *   `type`: The hardware device type.
        *   Digital: `"keyboard"`, `"gamepad"`, `"mouse"`.
        *   Analog (Optimized for continuous movement): `"gamepad_axis"`, `"mouse_axis"`, `"joystick_axis"`.
    *   `key`: The specific button or axis name (e.g., `"v"`, `"LEFT_STICK_X"`, `"LEFT_TRIGGER_AXIS"`).
    *   `consume`: Input consumption policy (`"always"`, `"never"`, `"on_ui_focus"`, `"manual"`).
    
    **Default Axis Parameters (Internal):**
    When an action is bound to an axis type, the framework automatically applies the following defaults:
    *   `mode`: `"analog"` (provides smooth float value).
    *   `deadzone`: `0.0` (raw input passed initially).
    *   `saturation`: `1.0` (max value at full physical deflection).
    *   `sensitivity`: `1.0` (linear scale).
    *   `curve`: `"linear"`.
    *   `side`: `"both"` (full axis range [-1, 1]).
    *   `invert`: `false`.
    *   `range_min/max`: `-1.0` and `1.0` (standard for sticks).
    
    **Default Button Parameters (Internal):**
    For digital buttons, the following defaults are used:
    *   `press_type`: `"short"`.
    *   `behavior`: `"toggle"`.
    *   `press_threshold_ms`: `500`.

*   **Windows**: `Defaults_AddWindow(h, name, visible, interactive, x, y, w, h, collapsed, autoScroll)`
    *   Defines the initial state of your plugin's ImGui windows.


## 5. Metadata and UI Rendering (`Meta_...`)

Metadata provides human-readable labels and instructs the framework on which UI widgets to use.

### `Meta_AddCustomSetting`
Links a JSON key from your defaults to a specific UI widget.

*   `keyPath`: Path to the variable (e.g., `"ui_colors.main"`).
*   `title`: Label displayed in the UI (can be a localization key).
*   `desc`: Tooltip description.
*   `widgetType`: `"slider"`, `"drag"`, `"combo"`, `"radio"`, `"color3"`, `"multiline"`, `"input_with_hint"`.
*   `widgetParamsJson`: Widget-specific configuration in JSON format.

**Widget Parameter Examples:**
*   **Slider**: `"{ \"min\": 0, \"max\": 100, \"format\": \"%d %%\" }"`
*   **Combo Box**: `"{ \"options\": [ { \"value\": \"A\", \"labelKey\": \"Option A\" } ] }"`
*   **Multiline**: `"{ \"height_in_lines\": 5 }"`


## Complete Implementation Example

```cpp
#include "SPF/SPF_API/SPF_Manifest_API.h"

void MyPlugin_BuildManifest(SPF_Manifest_Builder_Handle* h, const SPF_Manifest_Builder_API* api) {
    // 1. Identity
    api->Info_SetName(h, "SimplePlugin");
    api->Info_SetVersion(h, "1.0.0");
    api->Info_SetAuthor(h, "Developer");

    // 2. Policy
    api->Policy_SetAllowUserConfig(h, true);
    api->Policy_AddConfigurableSystem(h, "settings");
    api->Policy_AddConfigurableSystem(h, "keybinds");

    // 3. Custom Data
    api->Settings_SetJson(h, "{ \"volume\": 0.5 }");

    // 4. UI Rendering
    api->Meta_AddCustomSetting(h, "volume", "Volume", "Plugin audio level", 
                               "slider", "{ \"min\": 0.0, \"max\": 1.0, \"format\": \"%.2f\" }", false);

    // 5. Default Keybind
    api->Defaults_AddKeybind(h, "Main", "Open", "keyboard", "KEY_F10", "always");
    api->Meta_AddKeybind(h, "Main", "Open", "Open Menu", "Press F10 to enter settings.");
}
```