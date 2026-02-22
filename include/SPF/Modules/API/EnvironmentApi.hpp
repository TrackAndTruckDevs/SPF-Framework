#pragma once

#include "SPF/SPF_API/SPF_Environment_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {

class EnvironmentApi {
 public:
  static void FillEnvironmentApi(SPF_Environment_API* api);

 private:
  static SPF_Environment_Handle* Env_GetContext(const char* pluginName);

  // Section 1: Framework
  static int Env_GetFrameworkVersion(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetFrameworkBuildType(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetFrameworkConfiguration(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetFrameworkLoaderPath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // Section 2: Game
  static int Env_GetGameName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetGameCode(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetGameVersion(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static uint32_t Env_GetGameSteamAppId(SPF_Environment_Handle* h);
  static bool Env_IsSteamVersion(SPF_Environment_Handle* h);
  static int Env_GetGameExePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetGameRootPath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetGameCommandLine(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // Section 3: Paths
  static int Env_GetFrameworkBasePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetSCSUserDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetSCSModsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetCurrentProfilePath(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetSCSMusicDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetSCSScreenshotsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // Section 4: System
  static int Env_GetOSName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetSystemLocale(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);

  // Section 5: Status
  static int Env_GetActiveProfileName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static bool Env_IsVRActive(SPF_Environment_Handle* h);
  static bool Env_IsTobiiDllLoaded(SPF_Environment_Handle* h);
  static int Env_GetRendererName(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetMultiplayerStatus(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static bool Env_IsSteamOverlayDllLoaded(SPF_Environment_Handle* h);

  // Section 6: Sandboxing
  static int Env_GetPluginDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetPluginConfigDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetPluginLocalizationDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetPluginLogsDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static int Env_GetPluginDataDir(SPF_Environment_Handle* h, char* out_buffer, int buffer_size);
  static bool Env_CreatePath(SPF_Environment_Handle* h, const char* path);
};

}  // namespace Modules::API
SPF_NS_END
