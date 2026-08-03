#include "SPF/Modules/API/GameWorldApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/SPF_API/SPF_GameWorld_API.h"

#include <cstdint>

SPF_NS_BEGIN
namespace Modules::API {
using namespace SPF::Data::GameData;

void GameWorldApi::FillGameWorldApi(SPF_GameWorld_API* gameworld_api) {
  if (!gameworld_api) return;
  gameworld_api->GW_IsReady = &T_GW_IsReady;
  gameworld_api->GW_IsFinderReady = &T_GW_IsFinderReady;
  gameworld_api->GW_AreAllOffsetsFound = &T_GW_AreAllOffsetsFound;
  gameworld_api->GW_RefreshOffsets = &T_GW_RefreshOffsets;

  gameworld_api->GW_GetPreviewTime = &T_GW_GetPreviewTime;
  gameworld_api->GW_SetPreviewTime = &T_GW_SetPreviewTime;
  gameworld_api->GW_GetSimulationTime = &T_GW_GetSimulationTime;
  gameworld_api->GW_SetSimulationTime = &T_GW_SetSimulationTime;
  gameworld_api->GW_SetSkyboxAutoUpdate = &T_GW_SetSkyboxAutoUpdate;

  gameworld_api->GW_GetRealPlayTime = &T_GW_GetRealPlayTime;
  gameworld_api->GW_GetMapScale = &T_GW_GetMapScale;
  gameworld_api->GW_GetGlobalWarp = &T_GW_GetGlobalWarp;
  gameworld_api->GW_SetGlobalWarp = &T_GW_SetGlobalWarp;
  gameworld_api->GW_IsGamePaused = &T_GW_IsGamePaused;
  gameworld_api->GW_SetGamePaused = &T_GW_SetGamePaused;
  gameworld_api->GW_SetEngineHalt = &T_GW_SetEngineHalt;
  gameworld_api->GW_GetRealDeltaTime = &T_GW_GetRealDeltaTime;

  gameworld_api->GW_GetGameDay = &T_GW_GetGameDay;
  gameworld_api->GW_GetDayOfWeek = &T_GW_GetDayOfWeek;
  gameworld_api->GW_GetGameWeek = &T_GW_GetGameWeek;

  gameworld_api->GW_GetCityCount = &T_GW_GetCityCount;
  gameworld_api->GW_GetCityName = &T_GW_GetCityName;
  gameworld_api->GW_GetCityPosition = &T_GW_GetCityPosition;
  gameworld_api->GW_GetCityUid = &T_GW_GetCityUid;
  gameworld_api->GW_SetCityPosition = &T_GW_SetCityPosition;
  gameworld_api->GW_GetCityPointCount = &T_GW_GetCityPointCount;
  gameworld_api->GW_GetCityPoint = &T_GW_GetCityPoint;
  gameworld_api->GW_GetCityItemScale = &T_GW_GetCityItemScale;
  gameworld_api->GW_GetCityItemRadius = &T_GW_GetCityItemRadius;
  gameworld_api->GW_SetCityItemScale = &T_GW_SetCityItemScale;
  gameworld_api->GW_SetCityItemRadius = &T_GW_SetCityItemRadius;
  gameworld_api->GW_GetCityNameLocalized = &T_GW_GetCityNameLocalized;
  gameworld_api->GW_GetCityShortName = &T_GW_GetCityShortName;
  gameworld_api->GW_GetCityShortNameLocalized = &T_GW_GetCityShortNameLocalized;
  gameworld_api->GW_GetCityGroup = &T_GW_GetCityGroup;
  gameworld_api->GW_SetCityGroup = &T_GW_SetCityGroup;
  gameworld_api->GW_GetCityPinScaleFactor = &T_GW_GetCityPinScaleFactor;
  gameworld_api->GW_SetCityPinScaleFactor = &T_GW_SetCityPinScaleFactor;
  gameworld_api->GW_GetCityMapXOffsets = &T_GW_GetCityMapXOffsets;
  gameworld_api->GW_GetCityMapYOffsets = &T_GW_GetCityMapYOffsets;
  gameworld_api->GW_SetCityMapXOffsets = &T_GW_SetCityMapXOffsets;
  gameworld_api->GW_SetCityMapYOffsets = &T_GW_SetCityMapYOffsets;
  gameworld_api->GW_GetCityPriceCoef = &T_GW_GetCityPriceCoef;
  gameworld_api->GW_SetCityPriceCoef = &T_GW_SetCityPriceCoef;
  gameworld_api->GW_GetCityCountry = &T_GW_GetCityCountry;
  gameworld_api->GW_SetCityCountry = &T_GW_SetCityCountry;
  gameworld_api->GW_GetCityPopulation = &T_GW_GetCityPopulation;
  gameworld_api->GW_SetCityPopulation = &T_GW_SetCityPopulation;
  gameworld_api->GW_GetCityKeyCity = &T_GW_GetCityKeyCity;
  gameworld_api->GW_SetCityKeyCity = &T_GW_SetCityKeyCity;
  gameworld_api->GW_GetCityTimeZone = &T_GW_GetCityTimeZone;
  gameworld_api->GW_SetCityTimeZone = &T_GW_SetCityTimeZone;
}

bool GameWorldApi::T_GW_IsReady() { return GameWorldService::GetInstance().IsReady(); }
bool GameWorldApi::T_GW_IsFinderReady(const char* finderName) { return GameWorldService::GetInstance().IsFinderReady(finderName); }
bool GameWorldApi::T_GW_AreAllOffsetsFound() { return GameWorldService::GetInstance().AreAllFindersReady(); }
bool GameWorldApi::T_GW_RefreshOffsets() { return GameWorldService::GetInstance().TryFindAllOffsets(); }

uint32_t GameWorldApi::T_GW_GetPreviewTime() { return GameWorldService::GetInstance().GetPreviewTime(); }
void GameWorldApi::T_GW_SetPreviewTime(uint32_t totalMinutes) { GameWorldService::GetInstance().SetPreviewTime(totalMinutes); }
uint32_t GameWorldApi::T_GW_GetSimulationTime() { return GameWorldService::GetInstance().GetSimulationTime(); }
void GameWorldApi::T_GW_SetSimulationTime(uint32_t totalMinutes) { GameWorldService::GetInstance().SetSimulationTime(totalMinutes); }
void GameWorldApi::T_GW_SetSkyboxAutoUpdate(bool enabled) { GameWorldService::GetInstance().SetSkyboxAutoUpdate(enabled); }

uint32_t GameWorldApi::T_GW_GetRealPlayTime() { return GameWorldService::GetInstance().GetRealPlayTime(); }
float GameWorldApi::T_GW_GetMapScale() { return GameWorldService::GetInstance().GetMapScale(); }
float GameWorldApi::T_GW_GetGlobalWarp() { return GameWorldService::GetInstance().GetGlobalWarp(); }
void GameWorldApi::T_GW_SetGlobalWarp(float warp) { GameWorldService::GetInstance().SetGlobalWarp(warp); }
bool GameWorldApi::T_GW_IsGamePaused() { return GameWorldService::GetInstance().IsGamePaused(); }
void GameWorldApi::T_GW_SetGamePaused(bool paused) { GameWorldService::GetInstance().SetGamePaused(paused); }
void GameWorldApi::T_GW_SetEngineHalt(bool halted) { GameWorldService::GetInstance().SetEngineHalt(halted); }
double GameWorldApi::T_GW_GetRealDeltaTime() { return GameWorldService::GetInstance().GetRealDeltaTime(); }

uint32_t GameWorldApi::T_GW_GetGameDay() { return GameWorldService::GetInstance().GetGameDay(); }
uint32_t GameWorldApi::T_GW_GetDayOfWeek() { return GameWorldService::GetInstance().GetDayOfWeek(); }
uint32_t GameWorldApi::T_GW_GetGameWeek() { return GameWorldService::GetInstance().GetGameWeek(); }

uint32_t GameWorldApi::T_GW_GetCityCount() { return GameWorldService::GetInstance().GetCityCount(); }
int GameWorldApi::T_GW_GetCityName(uint32_t index, char* outBuffer, int bufferSize) { return GameWorldService::GetInstance().GetCityName(index, outBuffer, bufferSize); }
bool GameWorldApi::T_GW_GetCityPosition(uint32_t uid, float* outX, float* outY, float* outZ) { return GameWorldService::GetInstance().GetCityPosition(uid, outX, outY, outZ); }
uint32_t GameWorldApi::T_GW_GetCityUid(uint32_t index) { return GameWorldService::GetInstance().GetCityUid(index); }
bool GameWorldApi::T_GW_SetCityPosition(uint32_t uid, float x, float y, float z) { return GameWorldService::GetInstance().SetCityPosition(uid, x, y, z); }
uint32_t GameWorldApi::T_GW_GetCityPointCount(uint32_t index) { return GameWorldService::GetInstance().GetCityPointCount(index); }
bool GameWorldApi::T_GW_GetCityPoint(uint32_t index, uint32_t pointIndex, float* outX, float* outY, float* outZ) { return GameWorldService::GetInstance().GetCityPoint(index, pointIndex, outX, outY, outZ); }
float GameWorldApi::T_GW_GetCityItemScale(uint32_t index) { return GameWorldService::GetInstance().GetCityItemScale(index); }
float GameWorldApi::T_GW_GetCityItemRadius(uint32_t index) { return GameWorldService::GetInstance().GetCityItemRadius(index); }
bool GameWorldApi::T_GW_SetCityItemScale(uint32_t index, float val) { return GameWorldService::GetInstance().SetCityItemScale(index, val); }
bool GameWorldApi::T_GW_SetCityItemRadius(uint32_t index, float val) { return GameWorldService::GetInstance().SetCityItemRadius(index, val); }
int GameWorldApi::T_GW_GetCityNameLocalized(uint32_t index, char* outBuffer, int bufferSize) { return GameWorldService::GetInstance().GetCityNameLocalized(index, outBuffer, bufferSize); }
int GameWorldApi::T_GW_GetCityShortName(uint32_t index, char* outBuffer, int bufferSize) { return GameWorldService::GetInstance().GetCityShortName(index, outBuffer, bufferSize); }
int GameWorldApi::T_GW_GetCityShortNameLocalized(uint32_t index, char* outBuffer, int bufferSize) { return GameWorldService::GetInstance().GetCityShortNameLocalized(index, outBuffer, bufferSize); }
uint32_t GameWorldApi::T_GW_GetCityGroup(uint32_t index) { return GameWorldService::GetInstance().GetCityGroup(index); }
bool GameWorldApi::T_GW_SetCityGroup(uint32_t index, uint32_t val) { return GameWorldService::GetInstance().SetCityGroup(index, val); }
float GameWorldApi::T_GW_GetCityPinScaleFactor(uint32_t index) { return GameWorldService::GetInstance().GetCityPinScaleFactor(index); }
bool GameWorldApi::T_GW_SetCityPinScaleFactor(uint32_t index, float val) { return GameWorldService::GetInstance().SetCityPinScaleFactor(index, val); }
bool GameWorldApi::T_GW_GetCityMapXOffsets(uint32_t index, float* out, size_t maxCount) { return GameWorldService::GetInstance().GetCityMapXOffsets(index, out, maxCount); }
bool GameWorldApi::T_GW_GetCityMapYOffsets(uint32_t index, float* out, size_t maxCount) { return GameWorldService::GetInstance().GetCityMapYOffsets(index, out, maxCount); }
bool GameWorldApi::T_GW_SetCityMapXOffsets(uint32_t index, const float* values, size_t count) { return GameWorldService::GetInstance().SetCityMapXOffsets(index, values, count); }
bool GameWorldApi::T_GW_SetCityMapYOffsets(uint32_t index, const float* values, size_t count) { return GameWorldService::GetInstance().SetCityMapYOffsets(index, values, count); }
float GameWorldApi::T_GW_GetCityPriceCoef(uint32_t index) { return GameWorldService::GetInstance().GetCityPriceCoef(index); }
bool GameWorldApi::T_GW_SetCityPriceCoef(uint32_t index, float val) { return GameWorldService::GetInstance().SetCityPriceCoef(index, val); }
uint32_t GameWorldApi::T_GW_GetCityCountry(uint32_t index) { return GameWorldService::GetInstance().GetCityCountry(index); }
bool GameWorldApi::T_GW_SetCityCountry(uint32_t index, uint32_t val) { return GameWorldService::GetInstance().SetCityCountry(index, val); }
uint32_t GameWorldApi::T_GW_GetCityPopulation(uint32_t index) { return GameWorldService::GetInstance().GetCityPopulation(index); }
bool GameWorldApi::T_GW_SetCityPopulation(uint32_t index, uint32_t val) { return GameWorldService::GetInstance().SetCityPopulation(index, val); }
bool GameWorldApi::T_GW_GetCityKeyCity(uint32_t index) { return GameWorldService::GetInstance().GetCityKeyCity(index); }
bool GameWorldApi::T_GW_SetCityKeyCity(uint32_t index, bool val) { return GameWorldService::GetInstance().SetCityKeyCity(index, val); }
uint32_t GameWorldApi::T_GW_GetCityTimeZone(uint32_t index) { return GameWorldService::GetInstance().GetCityTimeZone(index); }
bool GameWorldApi::T_GW_SetCityTimeZone(uint32_t index, uint32_t val) { return GameWorldService::GetInstance().SetCityTimeZone(index, val); }

}  // namespace Modules::API
SPF_NS_END
