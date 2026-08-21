#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IManagerDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <cstdint>
#include <memory>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

/**
 * @class ManagerCoreService
 * @brief A singleton service that owns and runs core manager finders (GameplayManager, ...).
 */
class ManagerCoreService : public IWorldScopedService {
 public:
  static ManagerCoreService& GetInstance();

  ManagerCoreService(const ManagerCoreService&) = delete;
  void operator=(const ManagerCoreService&) = delete;

  void Initialize();
  void Reset();
  bool TryFindAllOffsets();
  bool AreAllFindersReady() const;
  bool IsFinderReady(const char* finderName) const;

  // --- IWorldScopedService ---
  const char* GetName() const override { return "ManagerCoreService"; }
  void ResetForWorldReload() override { Reset(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }
  bool IsGameplayManagerReady() const { return m_isInitialized && m_gameplayManagerAddr != 0; }
  bool IsCameraManagerReady() const { return m_isInitialized && m_cameraManagerAddr != 0; }
  bool IsEnvObjectOffsetReady() const { return m_isInitialized && m_envObjectOffset != 0; }
  bool IsTimeManagerReady() const { return m_isInitialized && m_timeMgrPtrAddr != 0; }
  bool IsReady() const { return IsGameplayManagerReady() && IsCameraManagerReady() && IsEnvObjectOffsetReady() && IsTimeManagerReady(); }

  // --- Public Getters ---
  uintptr_t GetGameplayManagerAddr() const { return m_gameplayManagerAddr; }
  uintptr_t GetCameraManagerAddr() const { return m_cameraManagerAddr; }
  intptr_t GetEnvObjectOffset() const { return m_envObjectOffset; }
  uintptr_t GetTimeMgrPtrAddr() const { return m_timeMgrPtrAddr; }

  // --- Public Setters (for use by finder implementations) ---
  void SetGameplayManagerAddr(uintptr_t addr) { m_gameplayManagerAddr = addr; }
  void SetCameraManagerAddr(uintptr_t addr) { m_cameraManagerAddr = addr; }
  void SetEnvObjectOffset(intptr_t offset) { m_envObjectOffset = offset; }
  void SetTimeMgrPtrAddr(uintptr_t addr) { m_timeMgrPtrAddr = addr; }

 private:
  ManagerCoreService();
  ~ManagerCoreService() = default;

  void RegisterFinders();

  bool m_isInitialized = false;
  uintptr_t m_gameplayManagerAddr = 0;
  uintptr_t m_cameraManagerAddr = 0;
  intptr_t m_envObjectOffset = 0;
  uintptr_t m_timeMgrPtrAddr = 0;
  std::vector<std::unique_ptr<IManagerDataFinder>> m_dataFinders;
};

}  // namespace Data::GameData
SPF_NS_END
