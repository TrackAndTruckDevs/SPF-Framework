#include "SPF/Modules/API/GameWorldApi.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include <Windows.h>

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

}  // namespace Modules::API
SPF_NS_END
