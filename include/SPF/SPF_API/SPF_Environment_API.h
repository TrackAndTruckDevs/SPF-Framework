/**
 * @file SPF_Environment_API.h
 * @brief API for retrieving information about the game, framework, and system environment.
 *
 * @details This API provides plugins with comprehensive data about the current execution
 *          context. It covers framework metadata, game identification, filesystem paths
 *          (including UFS resolved paths), and runtime status (VR, Multiplayer, etc.).
 *
 * All information exposed in the framework's "Environment Information" UI window is
 * accessible through this API.
 *
 * ================================================================================================
 * KEY CONCEPTS
 * ================================================================================================
 *
 * 1. **Context-Based**: Most calls require an 'SPF_Environment_Handle'. Get it
 *    once during 'OnLoad' or 'OnActivated' using 'Env_GetContext()'.
 *
 * 2. **ABI Stability**: This API uses a function table. New functions will be added to the
 *    end of the structure, ensuring that older plugins remain compatible without recompilation.
 *
 * 3. **String Handling**: Functions returning strings use the buffer/size pattern.
 *    - You provide a pointer to a char array and its size.
 *    - The function returns the actual length of the string (excluding null terminator).
 *    - If the return value >= buffer_size, the string was truncated.
 *
 * 4. **Path Normalization**: All paths returned by this API use the platform's preferred
 *    separators ('\' on Windows).
 *
 * ================================================================================================
 * USAGE EXAMPLE (C++)
 * ================================================================================================
 * @code
 * void MyPlugin_OnActivated(const SPF_Core_API* api) {
 *     SPF_Environment_Handle* h = api->environment->Env_GetContext("MyPlugin");
 *
 *     // 1. Get the profile name
 *     char profileName[64];
 *     api->environment->Env_GetActiveProfileName(h, profileName, sizeof(profileName));
 *
 *     // 2. Check if the game is in Convoy mode
 *     char mpStatus[32];
 *     api->environment->Env_GetMultiplayerStatus(h, mpStatus, sizeof(mpStatus));
 *     if (strcmp(mpStatus, "Convoy") == 0) {
 *         // ... logic for multiplayer ...
 *     }
 *
 *     // 3. Get the physical path to the mods folder
 *     char modsPath[MAX_PATH];
 *     api->environment->Env_GetSCSModsDir(h, modsPath, sizeof(modsPath));
 * }
 * @endcode
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to the plugin's environment context.
 * @details The framework manages the memory for this handle. Do not attempt to free it.
 */
typedef struct SPF_Environment_Handle SPF_Environment_Handle;

/**
 * @brief Describes one entry from the game's VFS mount tables (Section 8).
 */
typedef struct SPF_VfsMountInfo {
  char vpath[256];         /**< Virtual path (e.g. "/spf/MyPlugin", "/home"). */
  char physical_path[512]; /**< Physical disk path backing the mount. */
  int pool_index;          /**< 0 = core, 1 = user, 2 = mod, 3 = scs, 4 = root. */
  int order;               /**< Mount order inside the pool (higher = resolved earlier). */
} SPF_VfsMountInfo;

/**
 * @struct SPF_Environment_API
 * @brief Table of function pointers to access environment data.
 */
typedef struct SPF_Environment_API {
  /**
   * @brief Gets an environment context handle for the plugin.
   * @param pluginName The name of the plugin requesting the context.
   * @return A handle to the environment context, or NULL on error.
   */
  SPF_Environment_Handle* (*Env_GetContext)(const char* pluginName);

  // =============================================================================================
  // Section 1: Framework Information
  // =============================================================================================

  /**
   * @brief Gets the current version of the SPF Framework.
   * @param h The context handle obtained from Env_GetContext.
   * @param out_buffer Buffer to receive the version string.
   * @param buffer_size Size of the output buffer.
   * @return The actual length of the version string (e.g. "1.1.0-beta").
   */
  int (*Env_GetFrameworkVersion)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the build type of the framework.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the string.
   * @return Length of string. Returns "Stable" or "Beta".
   */
  int (*Env_GetFrameworkBuildType)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the compilation configuration.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the string.
   * @return Length of string. Returns "Release" or "Debug".
   */
  int (*Env_GetFrameworkConfiguration)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the full physical path to the spf-framework.dll file.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the absolute path.
   * @return Length of the path string.
   */
  int (*Env_GetFrameworkLoaderPath)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // =============================================================================================
  // Section 2: Game Information
  // =============================================================================================

  /**
   * @brief Gets the full name of the game.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the name.
   * @return Length of string. Example: "American Truck Simulator".
   */
  int (*Env_GetGameName)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the internal game code.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the code.
   * @return Length of string. Returns "ats" or "eut2".
   */
  int (*Env_GetGameCode)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the full game version string.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the version.
   * @return Length of string. Example: "1.50.1.2s".
   */
  int (*Env_GetGameVersion)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the Steam Application ID for the current game.
   * @param h The context handle.
   * @return 270880 for ATS, 227300 for ETS2, or 0 if not a Steam version.
   */
  uint32_t (*Env_GetGameSteamAppId)(SPF_Environment_Handle* h);

  /**
   * @brief Checks if the game is a Steam version.
   * @param h The context handle.
   * @return true if steam_api64.dll is loaded in the process.
   */
  bool (*Env_IsSteamVersion)(SPF_Environment_Handle* h);

  /**
   * @brief Gets the full path to the game's executable file.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetGameExePath)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the path to the game's root data folder (where .scs files are located).
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetGameRootPath)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the raw command line string used to launch the game.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the command line.
   * @return Length of string.
   */
  int (*Env_GetGameCommandLine)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // =============================================================================================
  // Section 3: Filesystem Paths (UFS Resolved)
  // =============================================================================================

  /**
   * @brief Gets the framework's base directory (spfAssets).
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of string.
   */
  int (*Env_GetFrameworkBasePath)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the game's user directory in "Documents".
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetSCSUserDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the mods directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetSCSModsDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the current active profile folder.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return String length, or 0 if no profile is active.
   */
  int (*Env_GetCurrentProfilePath)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the music directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetSCSMusicDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the screenshots directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path.
   * @return Length of the path string.
   */
  int (*Env_GetSCSScreenshotsDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // =============================================================================================
  // Section 4: System Information
  // =============================================================================================

  /**
   * @brief Gets the OS version name and build number.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the string.
   * @return Length of string. Example: "Windows 11 (Build 22631)".
   */
  int (*Env_GetOSName)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the system locale code.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the code.
   * @return Length of string. Example: "en-US", "uk-UA".
   */
  int (*Env_GetSystemLocale)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // =============================================================================================
  // Section 5: Runtime Status & Environment
  // =============================================================================================

  /**
   * @brief Gets the human-readable display name of the active profile.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the name.
   * @return Length of string. Example: "JohnDoe".
   */
  int (*Env_GetActiveProfileName)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Checks if the game is running in VR mode.
   * @param h The context handle.
   * @return true if -oculus or -openvr is in the command line or openvr_api.dll is loaded.
   */
  bool (*Env_IsVRActive)(SPF_Environment_Handle* h);

  /**
   * @brief Checks if the Tobii Eye Tracker integration DLL is loaded.
   * @param h The context handle.
   * @return true if tobii_gameintegration_x64.dll is present in memory.
   */
  bool (*Env_IsTobiiDllLoaded)(SPF_Environment_Handle* h);

  /**
   * @brief Gets the active graphics renderer name.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the name.
   * @return Length of string. Returns "DirectX 11", "DirectX 12", or "OpenGL".
   */
  int (*Env_GetRendererName)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the current multiplayer status.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the status string.
   * @return Length of string. Returns "None", "Convoy", or "TruckersMP".
   */
  int (*Env_GetMultiplayerStatus)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Checks if the Steam Overlay renderer DLL is loaded.
   * @param h The context handle.
   * @return true if GameOverlayRenderer64.dll is present.
   */
  bool (*Env_IsSteamOverlayDllLoaded)(SPF_Environment_Handle* h);

  // =============================================================================================
  // Section 6: Plugin Sandboxing (Helper Paths)
  // =============================================================================================

  /**
   * @brief Gets the root physical path of the calling plugin.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path (e.g., "spfPlugins/MyPlugin/").
   * @return Length of the path string.
   */
  int (*Env_GetPluginDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the plugin's configuration directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path (e.g., "spfPlugins/MyPlugin/config/").
   * @return Length of the path string.
   */
  int (*Env_GetPluginConfigDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the plugin's localization directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path (e.g., "spfPlugins/MyPlugin/localization/").
   * @return Length of the path string.
   */
  int (*Env_GetPluginLocalizationDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the plugin's logs directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path (e.g., "spfPlugins/MyPlugin/logs/").
   * @return Length of the path string.
   */
  int (*Env_GetPluginLogsDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Gets the physical path to the plugin's data directory.
   * @param h The context handle.
   * @param out_buffer Buffer to receive the path (e.g., "spfPlugins/MyPlugin/data/").
   * @return Length of the path string.
   */
  int (*Env_GetPluginDataDir)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  /**
   * @brief Helper to create a directory or a tree of directories.
   * @param h The context handle.
   * @param path The full physical path to create.
   * @return true if the directory was created or already exists.
   */
  bool (*Env_CreatePath)(SPF_Environment_Handle* h, const char* path);

  /**
   * @brief Gets the type of the active profile (e.g., "Steam Cloud", "Local", "Demo").
   * @param h The context handle.
   * @param out_buffer Buffer to receive the type string.
   * @param buffer_size Size of the output buffer.
   * @return Length of the type string.
   */
  int (*Env_GetActiveProfileType)(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // =============================================================================================
  // Section 7: Plugin VFS Mounting
  // =============================================================================================

  /**
   * @brief Mounts a physical directory into the game VFS under "/spf/<PluginName>".
   * @details The game resolves files under the virtual path to the plugin's physical
   *          directory. The mount is idempotent: calling again with the same directory
   *          returns the same virtual path. All mounts are automatically removed when
   *          the plugin is disabled or unloaded, even if the plugin forgets to unmount.
   * @param h The context handle obtained from Env_GetContext.
   * @param physical_path Physical directory to mount (e.g. from Env_GetPluginDir).
   * @param pool_index VFS pool: -1 = default (user), 0 = core, 1 = user, 2 = mod, 3 = scs.
   *          Pool precedence is fixed (highest first): core > user > mod > scs; the
   *          first pool holding the file wins. Mount recency does not determine the
   *          winner: a newer mount does not override an older one — even if a mod
   *          folder is mounted later, base content registered earlier may still
   *          shadow it.
   * @param order Mount order inside the pool: higher value = resolved earlier.
   *          Only matters when two mounts in the same pool overlap in virtual path
   *          prefixes (e.g. "/spf/A" vs "/spf/A/sub"); unrelated prefixes never
   *          compete. The game itself uses 100000 (base), 159-190 (DLC), 1001+
   *          (mods), 650 (home), 0 (steam). Pick a value below the game content of
   *          the target pool (e.g. 650 for user/mod pools) if the plugin content
   *          must lose to it. Any int is accepted; no game-side limits.
   * @param out_vpath Buffer to receive the virtual path (e.g. "/spf/MyPlugin").
   * @param buffer_size Size of the output buffer.
   * @return true if the directory is mounted (or already was).
   * @note Must be called from the game thread (OnActivated / OnGameWorldReady).
   */
  bool (*Env_VfsMount)(SPF_Environment_Handle* h, const char* physical_path, int pool_index, int order, char* out_vpath, int buffer_size);

  /**
   * @brief Unmounts all VFS mounts created by this plugin.
   * @param h The context handle obtained from Env_GetContext.
   * @return true.
   */
  bool (*Env_VfsUnmount)(SPF_Environment_Handle* h);

  // =============================================================================================
  // Section 8: VFS Mount Enumeration
  // =============================================================================================

  /**
   * @brief Gets the total number of mounts across all VFS pools.
   * @param h The context handle obtained from Env_GetContext.
   * @return Total mount count in the game's live tables (includes game and mod
   *         mounts, not only this plugin's), or 0 when the finder is not ready.
   * @note Call from the game thread. The count comes from a snapshot cache that
   *       is invalidated only when the game's mount tables change.
   */
  int (*Env_VfsGetMountCount)(SPF_Environment_Handle* h);

  /**
   * @brief Reads one mount entry by flat index (pools in index order).
   * @details On success fills all SPF_VfsMountInfo fields from the game's live
   *          mount tables: vpath (virtual path), physical_path (disk path),
   *          pool_index and order. Entries cover every mount in the game,
   *          including game and mod mounts, not only this plugin's.
   * @param h The context handle obtained from Env_GetContext.
   * @param index Zero-based index in [0, Env_VfsGetMountCount).
   * @param out_info Struct receiving the mount entry. Only valid when this
   *        function returns true; on false its contents are untouched.
   * @return true if the index was valid and out_info was filled.
   * @note Strings may be empty when the backing game memory could not be read;
   *       pool_index and order are always set on success. Call from the game
   *       thread. Iterate: count = Env_VfsGetMountCount(h); for (i = 0; i <
   *       count; ++i) Env_VfsGetMountAt(h, i, &info).
   */
  bool (*Env_VfsGetMountAt)(SPF_Environment_Handle* h, int index, SPF_VfsMountInfo* out_info);

} SPF_Environment_API;

#ifdef __cplusplus
}
#endif
