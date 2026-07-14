#include "SPF/Data/GameData/GameObjectSessionService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"
#include "SPF/Data/GameData/ISessionDataFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <memory>


SPF_NS_BEGIN
namespace Data::GameData {

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
  m_isInitialized = false;
}

bool GameObjectSessionService::AreAllFindersReady() const {
  if (m_dataFinders.empty()) return false;
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

}  // namespace Data::GameData
SPF_NS_END
