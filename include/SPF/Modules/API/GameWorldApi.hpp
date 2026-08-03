#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_GameWorld_API.h"

#include <cstddef>
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

  static uint32_t T_GW_GetCityCount();
  static int T_GW_GetCityName(uint32_t index, char* outBuffer, int bufferSize);
  static bool T_GW_GetCityPosition(uint32_t uid, float* outX, float* outY, float* outZ);
  static uint32_t T_GW_GetCityUid(uint32_t index);
  static bool T_GW_SetCityPosition(uint32_t uid, float x, float y, float z);
  static uint32_t T_GW_GetCityPointCount(uint32_t index);
  static bool T_GW_GetCityPoint(uint32_t index, uint32_t pointIndex, float* outX, float* outY, float* outZ);
  static float T_GW_GetCityItemScale(uint32_t index);
  static float T_GW_GetCityItemRadius(uint32_t index);
  static bool T_GW_SetCityItemScale(uint32_t index, float val);
  static bool T_GW_SetCityItemRadius(uint32_t index, float val);
  static int T_GW_GetCityNameLocalized(uint32_t index, char* outBuffer, int bufferSize);
  static int T_GW_GetCityShortName(uint32_t index, char* outBuffer, int bufferSize);
  static int T_GW_GetCityShortNameLocalized(uint32_t index, char* outBuffer, int bufferSize);
  static uint32_t T_GW_GetCityGroup(uint32_t index);
  static bool T_GW_SetCityGroup(uint32_t index, uint32_t val);
  static float T_GW_GetCityPinScaleFactor(uint32_t index);
  static bool T_GW_SetCityPinScaleFactor(uint32_t index, float val);
  static bool T_GW_GetCityMapXOffsets(uint32_t index, float* out, size_t maxCount);
  static bool T_GW_GetCityMapYOffsets(uint32_t index, float* out, size_t maxCount);
  static bool T_GW_SetCityMapXOffsets(uint32_t index, const float* values, size_t count);
  static bool T_GW_SetCityMapYOffsets(uint32_t index, const float* values, size_t count);
  static float T_GW_GetCityPriceCoef(uint32_t index);
  static bool T_GW_SetCityPriceCoef(uint32_t index, float val);
  static uint32_t T_GW_GetCityCountry(uint32_t index);
  static bool T_GW_SetCityCountry(uint32_t index, uint32_t val);
  static uint32_t T_GW_GetCityPopulation(uint32_t index);
  static bool T_GW_SetCityPopulation(uint32_t index, uint32_t val);
  static bool T_GW_GetCityKeyCity(uint32_t index);
  static bool T_GW_SetCityKeyCity(uint32_t index, bool val);
  static uint32_t T_GW_GetCityTimeZone(uint32_t index);
  static bool T_GW_SetCityTimeZone(uint32_t index, uint32_t val);
};
}  // namespace Modules::API
SPF_NS_END
