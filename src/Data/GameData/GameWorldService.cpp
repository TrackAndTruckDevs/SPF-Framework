/**                                                                                               
 * @file GameWorldService.cpp                                                                          
 * @brief Implementation of the GameWorldService for managing core engine state and world clock.
 */ 

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <algorithm>

SPF_NS_BEGIN
namespace Data::GameData {

GameWorldService::GameWorldService() = default;

GameWorldService& GameWorldService::GetInstance() {
  static GameWorldService instance;
  return instance;
}

void GameWorldService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
  logger->Info("Attempting to initialize GameWorldService...");

  RegisterFinders();

  m_isInitialized = false;
  logger->Info("GameWorldService initialization finished. Waiting for critical offsets.");
}

void GameWorldService::Shutdown() {
  if (m_isInitialized) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
    logger->Info("GameWorldService has been shut down.");
    m_isInitialized = false;

    m_environmentBasePtr = 0;
    m_environmentAdjustment = 0;
    m_timeMgrPtrAddr = 0;
    m_envObjectOffset = 0;
    m_timeOffset = 0;
    m_simulationTimeOffset = 0;
    m_subMinuteSecondsOffset = 0;
    m_realPlayTimeOffset = 0;
    m_realPlaySecondsOffset = 0;
    m_mapScaleOffset = 0;
    m_globalWarpOffset = 0;
    m_pauseStatusOffset = 0;
    m_frameCounterOffset = 0;
    m_realDeltaTimeOffset = 0;
    m_updateFnAddr = 0;
    m_globalHaltOffset = 0;
    m_simulationHaltOffset = 0;
    m_trafficHaltOffset = 0;
  }
}

void GameWorldService::RegisterFinders() {
  m_dataFinders.push_back(std::make_unique<Finders::WorldDataFinder>());
}

bool GameWorldService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
  logger->Info("Attempting to find all necessary game data offsets for GameWorldService.");

  bool all_critical_found_this_pass = true;

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("-> Finder '{}' succeeded.", finder->GetName());
      } else {
        logger->Warn("-> Finder '{}' failed. Will retry.", finder->GetName());
        if (strcmp(finder->GetName(), "WorldDataFinder") == 0) {
          all_critical_found_this_pass = false;
        }
      }
    }
  }

  if (all_critical_found_this_pass && AreAllFindersReady()) {
    m_isInitialized = true;
    logger->Info("GameWorldService: Successfully found all world data offsets.");
    return true;
  }

  return m_isInitialized;
}

bool GameWorldService::IsReady() {
  return m_isInitialized && AreAllFindersReady();
}

bool GameWorldService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool GameWorldService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}



// --- World Manipulation Methods ---

uint32_t GameWorldService::GetPreviewTime() {
  if (!m_isInitialized) return 0;

  // This retrieves the visual environment time (skybox/lighting state).
  // Note: This value is distinct from the actual game simulation clock.
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  basePtr += m_environmentAdjustment;

  uintptr_t envObject = *(uintptr_t*)(basePtr + m_envObjectOffset);

  if (!envObject) return 0;

  return *(uint32_t*)(envObject + m_timeOffset);
}

void GameWorldService::SetPreviewTime(uint32_t totalMinutes) {
  if (!m_isInitialized) return;

  uint32_t normalizedMinutes = totalMinutes % (1440 * 7); // Use full week cycle for visual consistency

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;

  uintptr_t envObject = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!envObject) return;

  // Update the visual time minutes (used for skybox and shadow calculations).
  // In the game engine, if the simulation is unpaused, this value will be
  // overwritten by the real game time logic on the next frame unless 
  // auto-update is disabled at 0x46c4.
  *(uint32_t*)(envObject + m_timeOffset) = normalizedMinutes;
  *(float*)(envObject + m_timeOffset + 4) = 0.0f; // Visual seconds

  typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
  UpdateEnv_t UpdateEnv = (UpdateEnv_t)m_updateFnAddr;

  if (UpdateEnv) {
    UpdateEnv(envObject);
  }
}

uint32_t GameWorldService::GetSimulationTime() {
  if (!m_isInitialized || m_timeMgrPtrAddr == 0) return 0;

  uintptr_t timeMgr = *(uintptr_t*)m_timeMgrPtrAddr;
  if (!timeMgr) return 0;

  return *(uint32_t*)(timeMgr + m_simulationTimeOffset);
}

void GameWorldService::SetSimulationTime(uint32_t totalMinutes) {
  if (!m_isInitialized || m_timeMgrPtrAddr == 0) return;

  uintptr_t timeMgr = *(uintptr_t*)m_timeMgrPtrAddr;
  if (!timeMgr) return;

  *(uint32_t*)(timeMgr + m_simulationTimeOffset) = totalMinutes;
  *(float*)(timeMgr + m_subMinuteSecondsOffset) = 0.0f;
}

uint32_t GameWorldService::GetRealPlayTime() {
  if (!m_isInitialized || m_timeMgrPtrAddr == 0) return 0;

  uintptr_t timeMgr = *(uintptr_t*)m_timeMgrPtrAddr;
  if (!timeMgr) return 0;

  // In version 1.60+, Real Play Time is part of an array_t<uint32_t>.
  // We detect this by the offset value (e.g., 0x1B98 vs 0x1C8).
  if (m_realPlayTimeOffset > 0x1000) {
    // Read the data pointer from the array_t structure (at offset +0x08).
    // As per Ghidra 1.60 analysis, the structure is accessed via an array helper.
    uintptr_t arrayDataPtr = *(uintptr_t*)(timeMgr + m_realPlayTimeOffset + 0x08);
    if (!arrayDataPtr) return 0;

    // Read the first element (minutes) which corresponds to the local player.
    return *(uint32_t*)arrayDataPtr;
  }

  // Version 1.59 and older: direct uint32_t access.
  return *(uint32_t*)(timeMgr + m_realPlayTimeOffset);
}

float GameWorldService::GetMapScale() {
  if (!m_isInitialized || m_environmentBasePtr == 0) return 1.0f;

  uintptr_t envBaseObj = *(uintptr_t*)m_environmentBasePtr;
  if (!envBaseObj) return 1.0f;
  envBaseObj += m_environmentAdjustment;

  return *(float*)(envBaseObj + m_mapScaleOffset);
}

uint32_t GameWorldService::GetGameDay() {
  return GetSimulationTime() / 1440;
}

uint32_t GameWorldService::GetDayOfWeek() {
  return GetGameDay() % 7;
}

uint32_t GameWorldService::GetGameWeek() {
  return GetGameDay() / 7;
}

float GameWorldService::GetGlobalWarp() {
  if (!m_isInitialized || m_globalWarpOffset == 0) return 1.0f;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return 1.0f;

  return *(float*)(coreApp + m_globalWarpOffset);
}

void GameWorldService::SetGlobalWarp(float warp) {
  if (!m_isInitialized || m_globalWarpOffset == 0) return;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return;

  *(float*)(coreApp + m_globalWarpOffset) = warp;
}

bool GameWorldService::IsGamePaused() {
  if (!m_isInitialized || m_globalHaltOffset == 0) return false;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return false;

  // If any halt counter is > 0, the game is technically paused/halted
  return *(int32_t*)(coreApp + m_globalHaltOffset) > 0 || 
         *(int32_t*)(coreApp + m_simulationHaltOffset) > 0;
}

void GameWorldService::SetGamePaused(bool paused) {
    SetEngineHalt(paused);
}

void GameWorldService::SetEngineHalt(bool halted) {
  if (!m_isInitialized || m_globalHaltOffset == 0 || m_simulationHaltOffset == 0) return;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return;

  *(int32_t*)(coreApp + m_globalHaltOffset) = halted ? 1 : 0;
  *(int32_t*)(coreApp + m_simulationHaltOffset) = halted ? 1 : 0;
  *(int32_t*)(coreApp + m_trafficHaltOffset) = halted ? 1 : 0;
  
  m_pluginHalted = halted;
}

uint32_t GameWorldService::GetFrameCounter() {
  if (!m_isInitialized || m_frameCounterOffset == 0) return 0;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return 0;

  return *(uint32_t*)(coreApp + m_frameCounterOffset);
}

double GameWorldService::GetRealDeltaTime() {
  if (!m_isInitialized || m_realDeltaTimeOffset == 0) return 0.0;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return 0.0;

  // Values is stored in microseconds, convert to seconds
  uint64_t microSecs = *(uint64_t*)(coreApp + m_realDeltaTimeOffset);
  return (double)microSecs * 1e-06;
}


void GameWorldService::SetSkyboxAutoUpdate(bool enabled) {
  if (!m_isInitialized) return;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;

  uintptr_t envObject = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!envObject) return;

  *(int32_t*)(envObject + m_skyboxAutoUpdateOffset) = enabled ? 0 : 1;
}

}  // namespace Data::GameData
SPF_NS_END
