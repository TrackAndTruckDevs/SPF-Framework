#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"

#include <cstdint>
#include <memory>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

/**
 * @class GameWorldService
 * @brief A service that provides memory offsets and pointers for core game world data (time, engine state).
 */
class GameWorldService {
 public:
  static GameWorldService& GetInstance();

  GameWorldService(const GameWorldService&) = delete;
  void operator=(const GameWorldService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady();
  bool IsFinderReady(const char* name) const;
  bool AreAllFindersReady() const;
  bool TryFindAllOffsets();

  // --- Public Getters ---
  intptr_t GetTimeOffset() const { return m_timeOffset; }
  intptr_t GetSimulationTimeOffset() const { return m_simulationTimeOffset; }
  intptr_t GetSubMinuteSecondsOffset() const { return m_subMinuteSecondsOffset; }
  intptr_t GetRealPlayTimeOffset() const { return m_realPlayTimeOffset; }
  intptr_t GetRealPlaySecondsOffset() const { return m_realPlaySecondsOffset; }
  intptr_t GetMapScaleOffset() const { return m_mapScaleOffset; }
  intptr_t GetGlobalWarpOffset() const { return m_globalWarpOffset; }
  intptr_t GetPauseStatusOffset() const { return m_pauseStatusOffset; }
  intptr_t GetGlobalHaltOffset() const { return m_globalHaltOffset; }
  intptr_t GetSimulationHaltOffset() const { return m_simulationHaltOffset; }
  intptr_t GetTrafficHaltOffset() const { return m_trafficHaltOffset; }
  intptr_t GetRealDeltaTimeOffset() const { return m_realDeltaTimeOffset; }
  intptr_t GetSkyboxAutoUpdateOffset() const { return m_skyboxAutoUpdateOffset; }
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- Public Setters (for finders) ---
  void SetTimeOffset(intptr_t val) { m_timeOffset = val; }
  void SetSimulationTimeOffset(intptr_t val) { m_simulationTimeOffset = val; }
  void SetSubMinuteSecondsOffset(intptr_t val) { m_subMinuteSecondsOffset = val; }
  void SetMapScaleOffset(intptr_t val) { m_mapScaleOffset = val; }
  void SetRealPlayTimeOffset(intptr_t val) { m_realPlayTimeOffset = val; }
  void SetRealPlaySecondsOffset(intptr_t val) { m_realPlaySecondsOffset = val; }
  void SetGlobalWarpOffset(intptr_t val) { m_globalWarpOffset = val; }
  void SetPauseStatusOffset(intptr_t val) { m_pauseStatusOffset = val; }
  void SetRealDeltaTimeOffset(intptr_t val) { m_realDeltaTimeOffset = val; }
  void SetSkyboxAutoUpdateOffset(intptr_t val) { m_skyboxAutoUpdateOffset = val; }
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }
  void SetGlobalHaltOffset(intptr_t val) { m_globalHaltOffset = val; }
  void SetSimulationHaltOffset(intptr_t val) { m_simulationHaltOffset = val; }
  void SetTrafficHaltOffset(intptr_t val) { m_trafficHaltOffset = val; }

  // --- World Manipulation Methods ---
  uint32_t GetPreviewTime();
  void SetPreviewTime(uint32_t totalMinutes);

  uint32_t GetSimulationTime();
  void SetSimulationTime(uint32_t totalMinutes);

  void SetSkyboxAutoUpdate(bool enabled);

  // --- Core/Engine Methods ---
  uint32_t GetRealPlayTime();
  float GetMapScale();
  float GetGlobalWarp();
  void SetGlobalWarp(float warp);
  bool IsGamePaused();
  void SetGamePaused(bool paused);
  void SetEngineHalt(bool halted);
  double GetRealDeltaTime();

  // --- Time Calculation Helpers ---
  uint32_t GetGameDay();    // Total game days
  uint32_t GetDayOfWeek();  // 0 = Monday, 6 = Sunday
  uint32_t GetGameWeek();   // Current game week index

 private:
  GameWorldService();
  ~GameWorldService() = default;

  void RegisterFinders();

  /**
   * @brief Resolves the GameplayManager instance pointer via ManagerCoreService.
   * @return The GameplayManager object pointer (0 if not resolved yet).
   */
  uintptr_t ResolveEnvironmentBase() const;

  // --- Runtime State ---
  bool m_isInitialized = false;
  bool m_pluginHalted = false;
  std::vector<std::unique_ptr<IGameWorldDataFinder>> m_dataFinders;

  // --- World Data Offsets and Pointers ---
  intptr_t m_timeOffset = 0;

  intptr_t m_simulationTimeOffset = 0;
  intptr_t m_subMinuteSecondsOffset = 0;

  intptr_t m_realPlayTimeOffset = 0;
  intptr_t m_realPlaySecondsOffset = 0;

  intptr_t m_mapScaleOffset = 0;

  intptr_t m_globalWarpOffset = 0;
  intptr_t m_pauseStatusOffset = 0;

  intptr_t m_globalHaltOffset = 0;
  intptr_t m_simulationHaltOffset = 0;
  intptr_t m_trafficHaltOffset = 0;

  intptr_t m_realDeltaTimeOffset = 0;

  intptr_t m_skyboxAutoUpdateOffset = 0;
  uintptr_t m_updateFnAddr = 0;
};

}  // namespace Data::GameData
SPF_NS_END
