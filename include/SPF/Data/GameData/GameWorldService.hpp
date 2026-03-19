#pragma once

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"
#include "SPF/Namespace.hpp"
#include <cstdint>
#include <vector>
#include <memory>

SPF_NS_BEGIN
namespace Data::GameData {

/**
 * @class GameWorldService
 * @brief A service that provides memory offsets and pointers for game world data (time, weather, etc.).
 */
class GameWorldService {
 public:
  static GameWorldService& GetInstance();

  GameWorldService(const GameWorldService&) = delete;
  void operator=(const GameWorldService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady() const { return m_isInitialized; }
  bool IsFinderReady(const char* name) const;
  bool AreAllFindersReady() const;
  bool TryFindAllOffsets();

  // --- Public Getters ---
  uintptr_t GetEnvironmentBasePtr() const { return m_environmentBasePtr; }
  intptr_t GetEnvObjectOffset() const { return m_envObjectOffset; }
  intptr_t GetTimeOffset() const { return m_timeOffset; }
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- Public Setters (for use by IGameWorldDataFinder implementations) ---
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
  void SetFrameCounterOffset(intptr_t val) { m_frameCounterOffset = val; }
  void SetRealDeltaTimeOffset(intptr_t val) { m_realDeltaTimeOffset = val; }
  void SetSkyboxAutoUpdateOffset(intptr_t val) { m_skyboxAutoUpdateOffset = val; }
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }

  // --- World Manipulation Methods ---
  uint32_t GetPreviewTime();
  void SetPreviewTime(uint32_t totalMinutes);

  uint32_t GetSimulationTime();
  void SetSimulationTime(uint32_t totalMinutes);

  float GetTimeScale();
  void SetTimeScale(float scale);

  void SetSkyboxAutoUpdate(bool enabled);

  // --- New Core/World methods ---
  uint32_t GetRealPlayTime();
  float GetMapScale();
  float GetGlobalWarp();
  void SetGlobalWarp(float warp);
  bool IsGamePaused();
  void SetGamePaused(bool paused);
  uint32_t GetFrameCounter();
  double GetRealDeltaTime();

  // --- Time Calculation Helpers ---
  uint32_t GetGameDay();         // Total game days (since start of epoch)
  uint32_t GetDayOfWeek();       // 0 = Monday, 6 = Sunday
  uint32_t GetGameWeek();        // Current game week index

 private:
  GameWorldService();
  ~GameWorldService() = default;

  void RegisterFinders();

  // --- Runtime State ---
  bool m_isInitialized = false;
  std::vector<std::unique_ptr<IGameWorldDataFinder>> m_dataFinders;

  // --- World Data Offsets and Pointers ---
  uintptr_t m_environmentBasePtr = 0;    // Pointer to MainEngineObject (DAT_143368640)
  uintptr_t m_timeMgrPtrAddr = 0;       // Pointer to TimeManager object (DAT_142cf7668)
  
  intptr_t m_envObjectOffset = 0;        // Offset to Environment object (0x7e8) from MainEngine
  intptr_t m_timeOffset = 0;             // Offset to Visual/Preview minutes (0x3e50) from EnvironmentObject
  
  /**
   * @brief SIMULATION TIME (Game World Clock)
   * This is the time seen on the truck's dashboard. It is accelerated (Map Scale)
   * and can jump forward during activities like resting or using the ferry.
   * Unit: Total minutes passed in the game world.
   */
  intptr_t m_simulationTimeOffset = 0;   // Offset to game world minutes (0x15c) from TimeManager
  intptr_t m_subMinuteSecondsOffset = 0; // Offset to game world seconds accumulator (0x160)
  
  /**
   * @brief REAL PLAY TIME (Profile Playtime)
   * This is the actual wall-clock time the player has spent in the simulation (unpaused).
   * It has a 1:1 ratio with real-world time and does not jump during world events.
   * Unit: Total minutes spent in the current session/profile.
   */
  intptr_t m_realPlayTimeOffset = 0;     // Offset to real playtime minutes (0x1e0) from TimeManager
  intptr_t m_realPlaySecondsOffset = 0;  // Offset to real playtime seconds accumulator (0x1e4)
  
  intptr_t m_mapScaleOffset = 0;         // Offset to Map/Local Scale (0x2f3c). (e.g. 19.0 on highways, 3.0 in cities)

  /**
   * @brief CORE ENGINE PARAMETERS (from CoreApp)
   * These parameters control the global behavior of the game engine.
   */
  intptr_t m_globalWarpOffset = 0;       // Offset to global warp/speed (0x66c)
  intptr_t m_pauseStatusOffset = 0;      // Offset to global pause flag (0x859)
  intptr_t m_frameCounterOffset = 0;     // Offset to engine frame counter (0x19c)
  intptr_t m_realDeltaTimeOffset = 0;    // Offset to real-time delta in microseconds (0x8e8)

  intptr_t m_skyboxAutoUpdateOffset = 0; // Offset to auto-update flag (0x46c4) in EnvironmentObject
  uintptr_t m_updateFnAddr = 0;          // Address of the UpdateEnvironmentState function
};

}  // namespace Data::GameData
SPF_NS_END
