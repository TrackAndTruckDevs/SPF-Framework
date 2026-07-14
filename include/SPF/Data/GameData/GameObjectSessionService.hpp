#pragma once

#include "SPF/Namespace.hpp"

#include <cstdint>
#include <memory>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

class ISessionDataFinder;  // Forward declaration

/**
 * @class GameObjectSessionService
 * @brief Singleton service providing memory addresses for game sessions (Multiplayer/Convoy) and active profiles.
 */
class GameObjectSessionService {
 public:
  static GameObjectSessionService& GetInstance();

  GameObjectSessionService(const GameObjectSessionService&) = delete;
  void operator=(const GameObjectSessionService&) = delete;

  void Initialize();
  bool TryFindAllOffsets();
  void Reset();
  bool AreAllFindersReady() const;

  // --- Found Addresses ---
  uintptr_t GetSessionMgrPtrAddr() const { return m_sessionMgrPtrAddr; }
  uintptr_t GetGamePtrAddr() const { return m_gamePtrAddr; }
  uint32_t GetProfileHandleOffset() const { return m_profileHandleOffset; }
  uint32_t GetConvoyStatusOffset() const { return m_convoyStatusOffset; }
  uint32_t GetProfileDisplayNameOffset() const { return m_profileDisplayNameOffset; }
  uint32_t GetProfileTypeOffset() const { return m_profileTypeOffset; }

  // --- Setters for finders ---
  void SetSessionMgrPtrAddr(uintptr_t addr) { m_sessionMgrPtrAddr = addr; }
  void SetGamePtrAddr(uintptr_t addr) { m_gamePtrAddr = addr; }
  void SetProfileHandleOffset(uint32_t offset) { m_profileHandleOffset = offset; }
  void SetConvoyStatusOffset(uint32_t offset) { m_convoyStatusOffset = offset; }
  void SetProfileDisplayNameOffset(uint32_t offset) { m_profileDisplayNameOffset = offset; }
  void SetProfileTypeOffset(uint32_t offset) { m_profileTypeOffset = offset; }

 private:
  GameObjectSessionService() = default;
  ~GameObjectSessionService() = default;

  void RegisterFinders();

  bool m_isInitialized = false;
  uintptr_t m_sessionMgrPtrAddr = 0;
  uintptr_t m_gamePtrAddr = 0;
  uint32_t m_profileHandleOffset = 0;
  uint32_t m_convoyStatusOffset = 0;
  uint32_t m_profileDisplayNameOffset = 0;
  uint32_t m_profileTypeOffset = 0;

  std::vector<std::unique_ptr<ISessionDataFinder>> m_dataFinders;
};

}  // namespace Data::GameData
SPF_NS_END
