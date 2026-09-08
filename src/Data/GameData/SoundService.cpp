#include "SPF/Data/GameData/SoundService.hpp"

#include "SPF/Data/GameData/Finders/SoundDataFinder.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <synchapi.h>
#include <utility>
#include <vector>
#include <windows.h>

namespace SPF::Data::GameData {

SoundService::SoundService() { WorldServiceRegistry::Get().Register(this); }

SoundService& SoundService::GetInstance() {
  static SoundService instance;
  return instance;
}

void SoundService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");
  logger->Info("Attempting to initialize SoundService...");

  RegisterFinders();

  m_isInitialized = false;
  logger->Info("SoundService initialization finished. Waiting for critical offsets.");
}

void SoundService::Shutdown() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  m_isInitialized = false;

  m_bankListLockOffset = 0;
  m_bankListHeadOffset = 0;
  m_bankListSentinelOffset = 0;
  m_bankEventListHeadOffset = 0;
  m_bankPathStringOffset = 0;
  m_eventListTerminatorOffset = 0;
  m_eventPathOffset = 0;
  m_eventGuidOffset = 0;

  for (const auto& finder : m_dataFinders) {
    finder->Reset();
  }

  logger->Info("SoundService has been shut down.");
}

void SoundService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::SoundDataFinder>()); }

bool SoundService::TryFindAllOffsets() {
  if (m_isInitialized) return true;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  // SoundService depends on SoundManager address resolved by ManagerCoreService.
  // Do not resolve offsets until the manager is available to avoid null dereferences.
  if (!ManagerCoreService::GetInstance().IsSoundManagerReady()) {
    logger->Warn("SoundService: SoundManager not resolved yet. Waiting for ManagerCoreService.");
    return false;
  }

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("[Success] Finder '{}' completed successfully.", finder->GetName());
      } else {
        logger->Warn("[Failed] Finder '{}' could not resolve all patterns. Will retry on next tick.", finder->GetName());
        return false;
      }
    }
  }

  m_isInitialized = true;
  logger->Info("SoundService: All offsets found. Service is READY.");
  return true;
}

bool SoundService::IsReady() { return m_isInitialized; }


std::vector<SoundBankGroup> SoundService::GetSoundBankGroups() {
  std::vector<SoundBankGroup> result;
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("SoundService");

  if (!m_isInitialized) {
    logger->Warn("GetSoundBankGroups: Service not initialized.");
    return result;
  }

  uintptr_t soundSystem = ManagerCoreService::GetInstance().GetSoundManagerAddr();
  if (!soundSystem) {
    logger->Warn("GetSoundBankGroups: SoundManager address is null.");
    return result;
  }

  uintptr_t lockAddr = soundSystem + m_bankListLockOffset;
  AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  uintptr_t bankListHead = *reinterpret_cast<uintptr_t*>(soundSystem + m_bankListHeadOffset);
  uintptr_t bankSentinel = soundSystem + m_bankListSentinelOffset;

  uintptr_t bankNode = bankListHead;

  while (bankNode != bankSentinel) {
    const char* bankPath = *reinterpret_cast<const char**>(bankNode + m_bankPathStringOffset);
    SoundBankGroup group;
    group.bankPath = bankPath ? bankPath : "";

    uintptr_t eventNode = *reinterpret_cast<uintptr_t*>(bankNode + m_bankEventListHeadOffset);
    uintptr_t eventSentinel = bankNode + m_bankEventListHeadOffset + m_eventListTerminatorOffset;

    while (eventNode != eventSentinel) {
      const char* eventPath = *reinterpret_cast<const char**>(eventNode + m_eventPathOffset);

      SoundEvent ev;
      ev.bankPath = group.bankPath;
      ev.eventPath = eventPath ? eventPath : "";
      memcpy(ev.guid, reinterpret_cast<void*>(eventNode + m_eventGuidOffset), 16);

      group.events.push_back(std::move(ev));

      eventNode = *reinterpret_cast<uintptr_t*>(eventNode);
    }

    result.push_back(std::move(group));
    bankNode = *reinterpret_cast<uintptr_t*>(bankNode);
  }

  ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(lockAddr));

  int totalEvents = 0;
  for (const auto& g : result) totalEvents += static_cast<int>(g.events.size());
  logger->Info("GetSoundBankGroups: {} banks found, {} events total.", result.size(), totalEvents);
  return result;
}
}  // namespace SPF::Data::GameData