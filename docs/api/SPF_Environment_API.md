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

---
**`int Env_GetActiveProfileType(SPF_Environment_Handle* h, char* out_buffer, int buffer_size)`**
Gets the type of the currently active profile.
*   **Returns:** Actual string length. Returns one of: `"Steam Cloud"`, `"Local"`, `"Preview"`, `"Academy"`, `"Demo"`.

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

<br>

### Section 7: Plugin VFS Mounting

These functions mount a physical directory from the plugin into the game's virtual file system (UFS), so the game itself can read files from the plugin's folder. The game receives the returned virtual path (e.g. `/spf/MyPlugin`) and resolves it like any other in-game path.

**Pool resolution.** Paths resolve across overlay pools in a fixed order (highest precedence first): `core` → `user` → `mod` → `scs`. The first pool that contains the file wins.

| `pool_index` | Pool | Precedence | Contents |
|---|---|---|---|
| `-1` (default) | user | 2 | home directory, music, screenshots, temp |
| `0` | core | 3 | effects, locale, core.scs |
| `1` | user | 2 | same pool as the default |
| `2` | mod | 1 | workshop / local mods |
| `3` | scs | 0 | base.scs, def.scs, all dlc_* |

**Mount recency does not win.** A newer mount does not override an older one: even if a mod folder is mounted after the base content, the base mount may still shadow it. Do not rely on "mounted later = wins" — pick the correct pool and keep the virtual path distinct (`/spf/<PluginName>`).

All mounts are removed automatically when the plugin is disabled or unloaded, even if the plugin forgets to unmount.

---
**`bool Env_VfsMount(SPF_Environment_Handle* h, const char* physical_path, int pool_index, int order, char* out_vpath, int buffer_size)`**
Mounts a physical directory into the game VFS under `/spf/<PluginName>`. The mount is idempotent: calling again with the same directory returns the same virtual path.
*   **Parameters:**
    *   `physical_path`: Physical directory to mount (e.g. from `Env_GetPluginDir`).
    *   `pool_index`: Target pool — see the table above; `-1` selects the default pool (`user`).
    *   `order`: Mount order inside the pool — **higher value = resolved earlier**. Only matters when two mounts in the same pool overlap in virtual path prefixes (e.g. `/spf/A` vs `/spf/A/sub`); unrelated prefixes never compete. The game uses `100000` (base), `159-190` (DLC), `1001+` (mods), `650` (home), `0` (steam). Pick a value below the game content of the target pool (e.g. `650` for user/mod pools) if the plugin content must lose to it. Any `int` is accepted; no game-side limits.
    *   `out_vpath` / `buffer_size`: Buffer receiving the virtual path (e.g. `"/spf/MyPlugin"`).
*   **Returns:** `true` if the directory is mounted (or already was).
*   **Note:** Must be called from the game thread (`OnActivated` / `OnGameWorldReady`).

---
**`bool Env_VfsUnmount(SPF_Environment_Handle* h)`**
Unmounts all VFS mounts created by this plugin.
*   **Returns:** `true`.
*   **Note:** The framework also unmounts everything automatically on plugin disable/unload.

---

### Section 8: VFS Mount Enumeration

These functions read the game's live UFS mount tables — all pools, game and mod mounts included, not only this plugin's. Use them to verify that a mount registered correctly or to inspect the game's VFS layout.

Reads are live and unguarded by locks: call from the game thread (`OnActivated` / `OnGameWorldReady`). Entries are snapshot values; a mount unmounted after enumeration may still appear in an already-copied struct.

**`SPF_VfsMountInfo` fields:**
*   `vpath[256]` — virtual path (e.g. `"/spf/MyPlugin"`, `"/home"`).
*   `physical_path[512]` — physical disk path backing the mount.
*   `pool_index` — `0` = core, `1` = user, `2` = mod, `3` = scs, `4` = root.
*   `order` — mount order inside the pool (higher = resolved earlier in that pool).

---
**`int Env_VfsGetMountCount(SPF_Environment_Handle* h)`**
Returns the total number of mounts across all VFS pools.
*   **Returns:** mount count, or `0` when the finder is not ready.

---
**`bool Env_VfsGetMountAt(SPF_Environment_Handle* h, int index, SPF_VfsMountInfo* out_info)`**
Reads the mount at a flat zero-based index (pools in index order) and fills **all four** `SPF_VfsMountInfo` fields: `vpath`, `physical_path`, `pool_index`, `order` (field descriptions above).
*   **Parameters:**
    *   `index`: `0 .. Env_VfsGetMountCount()-1`.
    *   `out_info`: struct receiving the entry — only valid when the function returns `true`; on `false` its contents are untouched.
*   **Returns:** `true` if the index was valid and `out_info` was filled.
*   **What to expect:**
    *   Entries cover **every** mount in the game (game, DLC, mods and your plugin's) — filter by `pool_index` or by comparing `vpath` with your own if you only care about a subset.
    *   `pool_index` and `order` are always set on success; `vpath` / `physical_path` may be empty strings if the backing game memory could not be read.
    *   Reads come from a snapshot cache invalidated only when the game's mount tables change, so a tight enumeration loop does not touch game memory per entry.
*   **Example:**
    ```c
    int n = env->Env_VfsGetMountCount(h);
    for (int i = 0; i < n; ++i) {
      SPF_VfsMountInfo m;
      if (!env->Env_VfsGetMountAt(h, i, &m)) break;
      // m.vpath, m.physical_path, m.pool_index, m.order
    }
    ```
