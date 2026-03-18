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
  void SetEnvObjectOffset(intptr_t val) { m_envObjectOffset = val; }
  void SetTimeOffset(intptr_t val) { m_timeOffset = val; }
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }

  // --- World Manipulation Methods ---
  uint32_t GetPreviewTime();
  void SetPreviewTime(uint32_t totalMinutes);

 private:
  GameWorldService();
  ~GameWorldService() = default;

  void RegisterFinders();

  // --- Runtime State ---
  bool m_isInitialized = false;
  std::vector<std::unique_ptr<IGameWorldDataFinder>> m_dataFinders;

  // --- World Data ---
  uintptr_t m_environmentBasePtr = 0;
  intptr_t m_envObjectOffset = 0;
  intptr_t m_timeOffset = 0;
  uintptr_t m_updateFnAddr = 0;
};

}  // namespace Data::GameData
SPF_NS_END
