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

} SPF_Environment_API;

#ifdef __cplusplus
}
#endif
