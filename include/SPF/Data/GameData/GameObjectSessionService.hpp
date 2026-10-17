#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace SPF::Data::GameData {

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



  /** @brief Address of the game's current language pointer global (PTR_PTR_142e068d0). */
  uintptr_t GetGameLanguagePtrAddr() const { return m_gameLanguagePtrAddr; }

  /** @brief FUN_14041c6c0(lang) — sets application language AND writes CVar cache in one call. */
  uintptr_t GetSetLanguageFuncAddr() const { return m_setLanguageFuncAddr; }

  /** @brief Returns prism_string array of available languages (locale/ subdirs). */
  uintptr_t GetLanguageListFuncAddr() const { return m_languageListFuncAddr; }

  /** @brief Current game language string (e.g. "ru_ru"), or empty. */
  std::string GetGameLanguage() const;

  /** @brief Languages installed under /locale/ (array_t<string_dyn_t>, stride 0x20). */
  std::vector<std::string> GetLanguageList() const;

  /** @brief Calls game SetLanguage (app language + CVar cache); returns the effective language. */
  std::string SetGameLanguage(const std::string& lang);

  // --- Setters for finders ---
  void SetSessionMgrPtrAddr(uintptr_t addr) { m_sessionMgrPtrAddr = addr; }
  void SetGamePtrAddr(uintptr_t addr) { m_gamePtrAddr = addr; }
  void SetProfileHandleOffset(uint32_t offset) { m_profileHandleOffset = offset; }
  void SetConvoyStatusOffset(uint32_t offset) { m_convoyStatusOffset = offset; }
  void SetProfileDisplayNameOffset(uint32_t offset) { m_profileDisplayNameOffset = offset; }
  void SetProfileTypeOffset(uint32_t offset) { m_profileTypeOffset = offset; }
  void SetGameLanguagePtrAddr(uintptr_t addr) { m_gameLanguagePtrAddr = addr; }
  void SetSetLanguageFuncAddr(uintptr_t addr) { m_setLanguageFuncAddr = addr; }
  void SetLanguageListFuncAddr(uintptr_t addr) { m_languageListFuncAddr = addr; }

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
  uintptr_t m_gameLanguagePtrAddr = 0;
  uintptr_t m_setLanguageFuncAddr = 0;
  uintptr_t m_languageListFuncAddr = 0;

  std::vector<std::unique_ptr<ISessionDataFinder>> m_dataFinders;
};

}  // namespace SPF::Data::GameData
