#pragma once

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"
#include "SPF/Namespace.hpp"
#include "SPF/Utils/Vec3.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

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
  uintptr_t GetEnvironmentBasePtr() const { return m_environmentBasePtr; }
  intptr_t GetEnvObjectOffset() const { return m_envObjectOffset; }
  intptr_t GetTimeOffset() const { return m_timeOffset; }
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- Public Setters (for finders) ---
  void SetEnvironmentBasePtr(uintptr_t val) { m_environmentBasePtr = val; }
  void SetTimeMgrPtrAddr(uintptr_t val) { m_timeMgrPtrAddr = val; }
  void SetEnvObjectOffset(intptr_t val) { m_envObjectOffset = val; }
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
  void SetEnvironmentAdjustment(intptr_t val) { m_environmentAdjustment = val; }
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
  uint32_t GetGameDay();         // Total game days
  uint32_t GetDayOfWeek();       // 0 = Monday, 6 = Sunday
  uint32_t GetGameWeek();        // Current game week index

 private:
  GameWorldService();
  ~GameWorldService() = default;

  void RegisterFinders();

  // --- Runtime State ---
  bool m_isInitialized = false;
  bool m_pluginHalted = false;
  std::vector<std::unique_ptr<IGameWorldDataFinder>> m_dataFinders;

  // --- World Data Offsets and Pointers ---
  uintptr_t m_environmentBasePtr = 0;    
  intptr_t m_environmentAdjustment = 0; 
  uintptr_t m_timeMgrPtrAddr = 0;       
  
  intptr_t m_envObjectOffset = 0;        
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
