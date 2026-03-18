/**                                                                                               
 * @file GameWorldService.cpp                                                                          
 * @brief Implementation of the GameWorldService for managing and manipulating the game environment.
 *
 * @details This service acts as the central logic unit for interacting with the 
 *          game's visual world state (e.g., time of day, skybox, lighting). 
 *          It utilizes the GameWorldDataFinder to dynamically resolve game 
 *          memory addresses and offsets, ensuring stability across game updates.
 */ 

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData {

GameWorldService::GameWorldService() = default;

GameWorldService& GameWorldService::GetInstance() {
  static GameWorldService instance;
  return instance;
}

void GameWorldService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
  logger->Info("Attempting to initialize GameWorldService...");

  RegisterFinders();

  m_isInitialized = false;     // Initially not ready, will be set by TryFindAllOffsets
  logger->Info("GameWorldService initialization finished. Waiting for critical offsets.");
}

void GameWorldService::Shutdown() {
  if (m_isInitialized) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
    logger->Info("GameWorldService has been shut down.");
    m_isInitialized = false;

    // Clear all data members to their initial state
    m_environmentBasePtr = 0;
    m_envObjectOffset = 0;
    m_timeOffset = 0;
    m_updateFnAddr = 0;
  }
}

void GameWorldService::RegisterFinders() {
  m_dataFinders.push_back(std::make_unique<Finders::WorldDataFinder>());
}

bool GameWorldService::TryFindAllOffsets() {
  if (m_isInitialized) return true;  // Already found everything

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");
  logger->Info("Attempting to find all necessary game data offsets for GameWorldService.");

  bool all_critical_found_this_pass = true;

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady())  // Only try to find if not already ready
    {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("-> Finder '{}' succeeded.", finder->GetName());
      } else {
        logger->Warn("-> Finder '{}' failed. Will retry.", finder->GetName());
        // For now, WorldDataFinder is our only and critical finder
        if (strcmp(finder->GetName(), "WorldDataFinder") == 0) {
          all_critical_found_this_pass = false;
        }
      }
    }
  }

  if (all_critical_found_this_pass && AreAllFindersReady()) {
    m_isInitialized = true;
    logger->Info("GameWorldService: Successfully found all world data offsets.");
    return true;
  }

  return m_isInitialized;
}

bool GameWorldService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool GameWorldService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      return false;
    }
  }
  return true;
}

uint32_t GameWorldService::GetPreviewTime() {
  if (!m_isInitialized) return 0;

  // This retrieves the visual environment time (skybox/lighting state).
  // Note: This value is distinct from the actual game simulation clock.
  uintptr_t baseObjAddr = *(uintptr_t*)m_environmentBasePtr;
  if (!baseObjAddr) return 0;

  uintptr_t envObject = *(uintptr_t*)(baseObjAddr + m_envObjectOffset);
  if (!envObject) return 0;

  return *(uint32_t*)(envObject + m_timeOffset);
}

void GameWorldService::SetPreviewTime(uint32_t totalMinutes) {
  if (!m_isInitialized) return;

  // Ensure minutes are within the valid day range (0 - 1439)
  uint32_t normalizedMinutes = totalMinutes % 1440;

  uintptr_t baseObjAddr = *(uintptr_t*)m_environmentBasePtr;
  if (!baseObjAddr) return;

  uintptr_t envObject = *(uintptr_t*)(baseObjAddr + m_envObjectOffset);
  if (!envObject) return;

  // Update the visual time minutes (used for skybox and shadow calculations).
  // In the game engine, if the simulation is unpaused, this value will be
  // overwritten by the real game time logic on the next frame.
  *(uint32_t*)(envObject + m_timeOffset) = normalizedMinutes;
  
  // Reset the secondary state field (UpdateEnvironmentState uses this to detect changes)
  *(uint32_t*)(envObject + m_timeOffset + 4) = 0;

  // Trigger the Environment Update function (RCX = envObject)
  // This function recalculates sun position, fog, and light scattering based on the time set above.
  typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
  UpdateEnv_t UpdateEnv = (UpdateEnv_t)m_updateFnAddr;

  if (UpdateEnv) {
    UpdateEnv(envObject);
  }
}

} // namespace Data::GameData
SPF_NS_END
