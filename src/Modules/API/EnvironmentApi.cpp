#include "SPF/Modules/API/EnvironmentApi.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include <cstring>
#include <string>
#include <map>
#include <Windows.h>

/**
 * @brief Actual definition of the opaque environment handle.
 * Stores the plugin name to correctly resolve sandbox paths.
 * Defined outside the namespace to match the C API declaration.
 */
struct SPF_Environment_Handle {
    std::string pluginName;
};

SPF_NS_BEGIN
namespace Modules::API {

/**
 * @brief Helper function to safely copy a string into a C-style buffer.
 * Returns the actual length of the string.
 */
static int SafeCopyString(const std::string& source, char* out_buffer, int buffer_size) {
    if (!out_buffer || buffer_size <= 0) return static_cast<int>(source.length());
    
    size_t copyLen = (source.length() < static_cast<size_t>(buffer_size)) ? source.length() : static_cast<size_t>(buffer_size - 1);
    std::memcpy(out_buffer, source.c_str(), copyLen);
    out_buffer[copyLen] = '\0';
    
    return static_cast<int>(source.length());
}

SPF_Environment_Handle* EnvironmentApi::Env_GetContext(const char* pluginName) {
    if (!pluginName) return nullptr;
    
    static std::map<std::string, SPF_Environment_Handle*> s_handles;
    std::string name(pluginName);
    
    if (s_handles.find(name) == s_handles.end()) {
        s_handles[name] = new SPF_Environment_Handle{name};
    }
    
    return s_handles[name];
}

// Section 1: Framework
int EnvironmentApi::Env_GetFrameworkVersion(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetFrameworkInfo().version, out, size);
}

int EnvironmentApi::Env_GetFrameworkBuildType(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetFrameworkInfo().buildType, out, size);
}

int EnvironmentApi::Env_GetFrameworkConfiguration(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetFrameworkInfo().configuration, out, size);
}

int EnvironmentApi::Env_GetFrameworkLoaderPath(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetFrameworkInfo().loaderPath.string(), out, size);
}

// Section 2: Game
int EnvironmentApi::Env_GetGameName(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().name, out, size);
}

int EnvironmentApi::Env_GetGameCode(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().code, out, size);
}

int EnvironmentApi::Env_GetGameVersion(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().version, out, size);
}

uint32_t EnvironmentApi::Env_GetGameSteamAppId(SPF_Environment_Handle* h) {
    return System::EnvironmentManager::GetInstance().GetGameInfo().steamAppId;
}

bool EnvironmentApi::Env_IsSteamVersion(SPF_Environment_Handle* h) {
    return System::EnvironmentManager::GetInstance().GetGameInfo().isSteamVersion;
}

int EnvironmentApi::Env_GetGameExePath(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().exePath.string(), out, size);
}

int EnvironmentApi::Env_GetGameRootPath(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().rootPath.string(), out, size);
}

int EnvironmentApi::Env_GetGameCommandLine(SPF_Environment_Handle* h, char* out, int size) {
    std::wstring wcmd = System::EnvironmentManager::GetInstance().GetGameInfo().commandLine;
    std::string cmd;
    if (!wcmd.empty()) {
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wcmd[0], (int)wcmd.size(), NULL, 0, NULL, NULL);
        cmd.resize(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, &wcmd[0], (int)wcmd.size(), &cmd[0], size_needed, NULL, NULL);
    }
    return SafeCopyString(cmd, out, size);
}

// Section 3: Paths
int EnvironmentApi::Env_GetFrameworkBasePath(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::PathManager::GetBasePath().string(), out, size);
}

int EnvironmentApi::Env_GetSCSUserDir(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::PathManager::GetSCSUserDir().string(), out, size);
}

int EnvironmentApi::Env_GetSCSModsDir(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::PathManager::GetSCSModsDir().string(), out, size);
}

int EnvironmentApi::Env_GetCurrentProfilePath(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::PathManager::GetCurrentProfilePath().string(), out, size);
}

int EnvironmentApi::Env_GetSCSMusicDir(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().musicPath.string(), out, size);
}

int EnvironmentApi::Env_GetSCSScreenshotsDir(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetGameInfo().screenshotPath.string(), out, size);
}

// Section 4: System
int EnvironmentApi::Env_GetOSName(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetSystemInfo().osName, out, size);
}

int EnvironmentApi::Env_GetSystemLocale(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetSystemInfo().locale, out, size);
}

// Section 5: Status
int EnvironmentApi::Env_GetActiveProfileName(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetStatus().profileName, out, size);
}

bool EnvironmentApi::Env_IsVRActive(SPF_Environment_Handle* h) {
    return System::EnvironmentManager::GetInstance().GetStatus().isVR;
}

bool EnvironmentApi::Env_IsTobiiDllLoaded(SPF_Environment_Handle* h) {
    return System::EnvironmentManager::GetInstance().GetStatus().isTobiiActive;
}

int EnvironmentApi::Env_GetRendererName(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetStatus().renderer, out, size);
}

int EnvironmentApi::Env_GetMultiplayerStatus(SPF_Environment_Handle* h, char* out, int size) {
    return SafeCopyString(System::EnvironmentManager::GetInstance().GetStatus().multiplayer, out, size);
}

bool EnvironmentApi::Env_IsSteamOverlayDllLoaded(SPF_Environment_Handle* h) {
    return System::EnvironmentManager::GetInstance().GetStatus().isSteamOverlayActive;
}

// Section 6: Sandboxing
int EnvironmentApi::Env_GetPluginDir(SPF_Environment_Handle* h, char* out, int size) {
    if (!h) return 0;
    return SafeCopyString(System::PathManager::GetPluginDir(h->pluginName).string(), out, size);
}

int EnvironmentApi::Env_GetPluginConfigDir(SPF_Environment_Handle* h, char* out, int size) {
    if (!h) return 0;
    return SafeCopyString(System::PathManager::GetPluginConfigDir(h->pluginName).string(), out, size);
}

int EnvironmentApi::Env_GetPluginLocalizationDir(SPF_Environment_Handle* h, char* out, int size) {
    if (!h) return 0;
    return SafeCopyString(System::PathManager::GetPluginLocalizationDir(h->pluginName).string(), out, size);
}

int EnvironmentApi::Env_GetPluginLogsDir(SPF_Environment_Handle* h, char* out, int size) {
    if (!h) return 0;
    return SafeCopyString(System::PathManager::GetPluginLogsDir(h->pluginName).string(), out, size);
}

int EnvironmentApi::Env_GetPluginDataDir(SPF_Environment_Handle* h, char* out, int size) {
    if (!h) return 0;
    return SafeCopyString(System::PathManager::GetPluginDataDir(h->pluginName).string(), out, size);
}

bool EnvironmentApi::Env_CreatePath(SPF_Environment_Handle* h, const char* path) {
    if (!path) return false;
    try {
        std::filesystem::path p(path);
        if (std::filesystem::exists(p)) return true;
        return std::filesystem::create_directories(p);
    } catch (...) {
        return false;
    }
}

void EnvironmentApi::FillEnvironmentApi(SPF_Environment_API* api) {
    if (!api) return;

    api->Env_GetContext = &EnvironmentApi::Env_GetContext;

    api->Env_GetFrameworkVersion = &EnvironmentApi::Env_GetFrameworkVersion;
    api->Env_GetFrameworkBuildType = &EnvironmentApi::Env_GetFrameworkBuildType;
    api->Env_GetFrameworkConfiguration = &EnvironmentApi::Env_GetFrameworkConfiguration;
    api->Env_GetFrameworkLoaderPath = &EnvironmentApi::Env_GetFrameworkLoaderPath;

    api->Env_GetGameName = &EnvironmentApi::Env_GetGameName;
    api->Env_GetGameCode = &EnvironmentApi::Env_GetGameCode;
    api->Env_GetGameVersion = &EnvironmentApi::Env_GetGameVersion;
    api->Env_GetGameSteamAppId = &EnvironmentApi::Env_GetGameSteamAppId;
    api->Env_IsSteamVersion = &EnvironmentApi::Env_IsSteamVersion;
    api->Env_GetGameExePath = &EnvironmentApi::Env_GetGameExePath;
    api->Env_GetGameRootPath = &EnvironmentApi::Env_GetGameRootPath;
    api->Env_GetGameCommandLine = &EnvironmentApi::Env_GetGameCommandLine;

    api->Env_GetFrameworkBasePath = &EnvironmentApi::Env_GetFrameworkBasePath;
    api->Env_GetSCSUserDir = &EnvironmentApi::Env_GetSCSUserDir;
    api->Env_GetSCSModsDir = &EnvironmentApi::Env_GetSCSModsDir;
    api->Env_GetCurrentProfilePath = &EnvironmentApi::Env_GetCurrentProfilePath;
    api->Env_GetSCSMusicDir = &EnvironmentApi::Env_GetSCSMusicDir;
    api->Env_GetSCSScreenshotsDir = &EnvironmentApi::Env_GetSCSScreenshotsDir;

    api->Env_GetOSName = &EnvironmentApi::Env_GetOSName;
    api->Env_GetSystemLocale = &EnvironmentApi::Env_GetSystemLocale;

    api->Env_GetActiveProfileName = &EnvironmentApi::Env_GetActiveProfileName;
    api->Env_IsVRActive = &EnvironmentApi::Env_IsVRActive;
    api->Env_IsTobiiDllLoaded = &EnvironmentApi::Env_IsTobiiDllLoaded;
    api->Env_GetRendererName = &EnvironmentApi::Env_GetRendererName;
    api->Env_GetMultiplayerStatus = &EnvironmentApi::Env_GetMultiplayerStatus;
    api->Env_IsSteamOverlayDllLoaded = &EnvironmentApi::Env_IsSteamOverlayDllLoaded;

    api->Env_GetPluginDir = &EnvironmentApi::Env_GetPluginDir;
    api->Env_GetPluginConfigDir = &EnvironmentApi::Env_GetPluginConfigDir;
    api->Env_GetPluginLocalizationDir = &EnvironmentApi::Env_GetPluginLocalizationDir;
    api->Env_GetPluginLogsDir = &EnvironmentApi::Env_GetPluginLogsDir;
    api->Env_GetPluginDataDir = &EnvironmentApi::Env_GetPluginDataDir;
    api->Env_CreatePath = &EnvironmentApi::Env_CreatePath;
}

} // namespace Modules::API
SPF_NS_END
