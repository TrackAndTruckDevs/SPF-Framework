#include "SPF/Data/GameData/GameObjectSessionService.hpp"

#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"
#include "SPF/Data/GameData/ISessionDataFinder.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

namespace SPF::Data::GameData {

GameObjectSessionService& GameObjectSessionService::GetInstance() {
  static GameObjectSessionService instance;
  return instance;
}

void GameObjectSessionService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectSessionService");
  logger->Info("Initializing Session Service...");
  RegisterFinders();
}

void GameObjectSessionService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::SessionDataFinder>()); }

bool GameObjectSessionService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectSessionService");
  for (auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("-> Finder '{}' succeeded.", finder->GetName());
      }
    }
  }

  if (AreAllFindersReady()) {
    m_isInitialized = true;
    logger->Info("Session Service initialized and ready.");
    return true;
  }
  return false;
}

void GameObjectSessionService::Reset() {
  m_sessionMgrPtrAddr = 0;
  m_gamePtrAddr = 0;
  m_profileHandleOffset = 0;
  m_convoyStatusOffset = 0;
  m_profileDisplayNameOffset = 0;
  m_profileTypeOffset = 0;
  m_gameLanguagePtrAddr = 0;
  m_setLanguageFuncAddr = 0;
  m_languageListFuncAddr = 0;
  m_isInitialized = false;

  for (auto& finder : m_dataFinders) {
    finder->Reset();
  }
}

std::string GameObjectSessionService::GetGameLanguage() const {
  if (!m_gameLanguagePtrAddr) return {};

  // m_gameLanguagePtrAddr is PTR_PTR_142e068d0 — a string_dyn_t global:
  // vtable@0, buffer@8. The language chars live at buffer (e.g. "ru_ru").
  if (IsBadReadPtr(reinterpret_cast<void*>(m_gameLanguagePtrAddr), 16)) return {};
  uintptr_t langStringPtr = *reinterpret_cast<uintptr_t*>(m_gameLanguagePtrAddr + 8);
  if (!langStringPtr || IsBadReadPtr(reinterpret_cast<void*>(langStringPtr), 1)) return {};

  return reinterpret_cast<const char*>(langStringPtr);
}

std::vector<std::string> GameObjectSessionService::GetLanguageList() const {
  std::vector<std::string> result;
  if (!m_languageListFuncAddr) return result;

  using ListFunc = void* (*)();
  const void* list = reinterpret_cast<ListFunc>(m_languageListFuncAddr)();
  if (!list || IsBadReadPtr(list, 24)) return result;

  // array_t<string_dyn_t>: data@+8, count@+16, stride 0x20,
  // buffer@element+8 (Ghidra SetApplicationLanguage iteration).
  const char* data = *reinterpret_cast<const char* const*>(reinterpret_cast<const char*>(list) + 8);
  const size_t count = *reinterpret_cast<const size_t*>(reinterpret_cast<const char*>(list) + 16);
  if (!data || count == 0 || count > 64) return result;
  if (IsBadReadPtr(data, count * 0x20)) return result;

  auto& resolver = Hooks::GameTools::PrismStringResolver::GetInstance();
  for (size_t i = 0; i < count; ++i) {
    std::string lang = resolver.ReadString(data + i * 0x20);
    if (!lang.empty()) result.push_back(std::move(lang));
  }
  return result;
}

std::string GameObjectSessionService::SetGameLanguage(const std::string& lang) {
  if (!m_setLanguageFuncAddr || lang.empty()) return {};

  // input: prism::string { const char* buffer } — callee reads [0] only
  const char* langBuffer = lang.c_str();
  struct PrismStringView {
    const char* buffer;
  } in{langBuffer};

  // game setter: SetApplicationLanguage + CVar_SetValue(cache) in one call,
  // output string destroyed by the callee.
  using SetFunc = void (*)(void*);
  reinterpret_cast<SetFunc>(m_setLanguageFuncAddr)(&in);

  return GetGameLanguage();
}

bool GameObjectSessionService::AreAllFindersReady() const {
  if (m_dataFinders.empty()) return false;
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

}  // namespace SPF::Data::GameData
