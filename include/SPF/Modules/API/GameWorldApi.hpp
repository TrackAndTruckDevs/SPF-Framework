#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_GameWorld_API.h"

#include <cstdint>

SPF_NS_BEGIN
namespace Modules::API {
class GameWorldApi {
 public:
  static void FillGameWorldApi(SPF_GameWorld_API* gameworld_api);

 private:
  static bool T_GW_IsReady();
  static bool T_GW_IsFinderReady(const char* finderName);
  static bool T_GW_AreAllOffsetsFound();
  static bool T_GW_RefreshOffsets();

  static uint32_t T_GW_GetPreviewTime();
  static void T_GW_SetPreviewTime(uint32_t totalMinutes);
  static uint32_t T_GW_GetSimulationTime();
  static void T_GW_SetSimulationTime(uint32_t totalMinutes);
  static void T_GW_SetSkyboxAutoUpdate(bool enabled);

  static uint32_t T_GW_GetRealPlayTime();
  static float T_GW_GetMapScale();
  static float T_GW_GetGlobalWarp();
  static void T_GW_SetGlobalWarp(float warp);
  static bool T_GW_IsGamePaused();
  static void T_GW_SetGamePaused(bool paused);
  static void T_GW_SetEngineHalt(bool halted);
  static double T_GW_GetRealDeltaTime();

  static uint32_t T_GW_GetGameDay();
  static uint32_t T_GW_GetDayOfWeek();
  static uint32_t T_GW_GetGameWeek();
};
}  // namespace Modules::API
SPF_NS_END
