#include "SPF/Data/GameData/ManagerCoreService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/ManagerCoreDataFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <memory>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

ManagerCoreService& ManagerCoreService::GetInstance() {
  static ManagerCoreService instance;
  return instance;
}

void ManagerCoreService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ManagerCoreService");
  logger->Info("Initializing Manager Core Service...");

  RegisterFinders();
  logger->Info("Registered {0} data finders.", m_dataFinders.size());

  m_isInitialized = false;
  logger->Info("Manager Core Service initialization finished. Waiting for critical offsets.");
}

void ManagerCoreService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::ManagerCoreDataFinder>()); }

bool ManagerCoreService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ManagerCoreService");

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("-> Finder '{0}' succeeded.", finder->GetName());
      } else {
        logger->Warn("-> Finder '{0}' failed. Will retry.", finder->GetName());
      }
    }
  }

  if (AreAllFindersReady()) {
    m_isInitialized = true;
    logger->Info("All manager data finders are now ready. Service is fully initialized.");
    return true;
  }

  return m_isInitialized;
}

bool ManagerCoreService::AreAllFindersReady() const {
  if (m_dataFinders.empty()) return false;

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

bool ManagerCoreService::IsFinderReady(const char* finderName) const {
  if (!finderName) return false;
  for (const auto& finder : m_dataFinders) {
    if (finder->GetName() == finderName) {
      return finder->IsReady();
    }
  }
  return false;
}

void ManagerCoreService::Reset() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ManagerCoreService");
  logger->Info("Manager Core Service has been reset.");

  m_isInitialized = false;
  m_gameplayManagerAddr = 0;
  m_cameraManagerAddr = 0;
  m_envObjectOffset = 0;
}

}  // namespace Data::GameData
SPF_NS_END
