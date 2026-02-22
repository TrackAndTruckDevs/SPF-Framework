# SPF Environment API

The SPF Environment API provides plugins with comprehensive information about the current execution context. It covers framework metadata, game identification, filesystem paths (including UFS resolved paths), and runtime status (VR, Multiplayer, and active profile).

## Getting the API

To use the Environment API, you first need to get a pointer to the `SPF_Environment_API` struct from the framework. This is typically done during your plugin's initialization phase (`OnLoad` or `OnActivated`).

**Example: C**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Environment_API.h"

// Global pointer to the Environment API
SPF_Environment_API* s_envAPI = NULL;
SPF_Environment_Handle* s_envHandle = NULL;

void MyPlugin_OnLoad(const SPF_Load_API* api) {
    s_envAPI = api->environment;
    
    if (s_envAPI) {
        // Get a context handle for your plugin
        s_envHandle = s_envAPI->Env_GetContext("MyPlugin");
    }
}
```

## Data Types

### SPF_Environment_Handle (opaque struct)

An opaque handle that identifies your plugin's context when calling environment functions. Obtain this handle once using `Env_GetContext`.

## Function Reference

All API functions are accessed as function pointers through the `SPF_Environment_API` struct. Functions returning strings use a buffer-copy pattern and return the actual length of the string.

### Context Management

---
**`SPF_Environment_Handle* Env_GetContext(const char* pluginName)`**
Gets a unique environment context handle for the plugin.
*   **Parameters:**
    *   `pluginName`: The name of your plugin (must match the manifest).
*   **Returns:** A pointer to an opaque handle, or `NULL` on error.

<br>

### Section 1: Framework Information

These functions provide metadata about the SPF Framework itself.

---
**`int Env_GetFrameworkVersion(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the current version of the SPF Framework.
*   **Returns:** Actual string length. Example: `"1.1.0-beta"`.

---
**`int Env_GetFrameworkBuildType(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the build type (e.g., "Stable" or "Beta").

---
**`int Env_GetFrameworkConfiguration(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the compilation configuration ("Release" or "Debug").

---
**`int Env_GetFrameworkLoaderPath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the absolute physical path to the `spf-framework.dll` file.

<br>

### Section 2: Game Information

Functions to identify the running game and its environment.

---
**`int Env_GetGameName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the full name of the game (e.g., "American Truck Simulator").

---
**`int Env_GetGameCode(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the short internal game code ("ats" or "eut2").

---
**`int Env_GetGameVersion(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the full game version string (e.g., "1.50.1.2s").

---
**`uint32_t Env_GetGameSteamAppId(SPF_Environment_Handle* h)`**
Gets the Steam Application ID.
*   **Returns:** `270880` (ATS), `227300` (ETS2), or `0` if not a Steam version.

---
**`bool Env_IsSteamVersion(SPF_Environment_Handle* h)`**
Checks if the game is running as a Steam version.
*   **Returns:** `true` if `steam_api64.dll` is detected in the process.

---
**`int Env_GetGameExePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the full path to the game's executable file.

---
**`int Env_GetGameRootPath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the path to the game's root data folder (where `.scs` files are located).

---
**`int Env_GetGameCommandLine(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the raw command line string used to launch the game.

<br>

### Section 3: Filesystem Paths (UFS Resolved)

These functions return physical disk paths for virtual game directories. All paths are normalized using platform-preferred separators (`` on Windows).

---
**`int Env_GetFrameworkBasePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the framework's assets directory (`spfAssets`).

---
**`int Env_GetSCSUserDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the game's user directory in "Documents" (resolved via UFS `/home`).

---
**`int Env_GetSCSModsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the mods directory.

---
**`int Env_GetCurrentProfilePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the currently active profile folder.
*   **Returns:** Length of path, or `0` if no profile is active.

---
**`int Env_GetSCSMusicDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the music directory.

---
**`int Env_GetSCSScreenshotsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the screenshots directory.

<br>

### Section 4: System Information

---
**`int Env_GetOSName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the OS version and build number (e.g., "Windows 11 (Build 22631)").

---
**`int Env_GetSystemLocale(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the system locale code (e.g., "en-US", "uk-UA").

<br>

### Section 5: Runtime Status & Environment

---
**`int Env_GetActiveProfileName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the human-readable display name of the active profile (e.g., "JohnDoe").

---
**`bool Env_IsVRActive(SPF_Environment_Handle* h)`**
Checks if the game is running in VR mode (detects `-oculus`, `-openvr` or `openvr_api.dll`).

---
**`bool Env_IsTobiiDllLoaded(SPF_Environment_Handle* h)`**
Checks if the Tobii Eye Tracker integration DLL (`tobii_gameintegration_x64.dll`) is loaded.

---
**`int Env_GetRendererName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the active graphics renderer name ("DirectX 11", "DirectX 12", or "OpenGL").

---
**`int Env_GetMultiplayerStatus(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the current multiplayer mode ("None", "Convoy", or "TruckersMP").

---
**`bool Env_IsSteamOverlayDllLoaded(SPF_Environment_Handle* h)`**
Checks if the Steam Overlay renderer DLL (`GameOverlayRenderer64.dll`) is present in the process memory.

<br>

### Section 6: Plugin Sandboxing (Helper Paths)

These functions provide plugins with easy access to their own "sandbox" directories. These paths are relative to the plugin's own folder within the `spfPlugins` directory.

---
**`int Env_GetPluginDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the root physical path to the calling plugin's directory.
*   **Example:** `"E:\Games\ATS\bin\win_x64\spfPlugins\MyPlugin\"`.

---
**`int Env_GetPluginConfigDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the plugin's `config` folder.
*   **Example:** `"...\spfPlugins\MyPlugin\config\"`.

---
**`int Env_GetPluginLocalizationDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the plugin's `localization` folder.
*   **Example:** `"...\spfPlugins\MyPlugin\localization\"`.

---
**`int Env_GetPluginLogsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the plugin's `logs` folder.
*   **Example:** `"...\spfPlugins\MyPlugin\logs\"`.

---
**`int Env_GetPluginDataDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the physical path to the plugin's `data` folder. This is the recommended place to store databases, caches, and other persistent data.
*   **Example:** `"...\spfPlugins\MyPlugin\data\"`.

---
**`bool Env_CreatePath(SPF_Environment_Handle* h, const char* path)`**
A helper function to create a directory or a full tree of directories.
*   **Parameters:**
    *   `path`: The full physical path you want to create (usually obtained from one of the functions above).
*   **Returns:** `true` if the directory was successfully created or already exists; `false` on error.
