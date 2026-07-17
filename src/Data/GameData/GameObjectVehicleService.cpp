#include "SPF/Data/GameData/GameObjectVehicleService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/ObjectVehicleManagerFinder.hpp"
#include "SPF/Data/GameData/IObjectDataFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>
#include <memory>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

GameObjectVehicleService& GameObjectVehicleService::GetInstance() {
  static GameObjectVehicleService instance;
  return instance;
}

void GameObjectVehicleService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectVehicleService");
  logger->Info("Initializing Game Object Vehicle Service...");

  RegisterFinders();
  logger->Info("Registered {0} data finders.", m_dataFinders.size());

  m_isInitialized = false;
  logger->Info("Game Object Vehicle Service initialization finished. Waiting for critical offsets.");
}

void GameObjectVehicleService::RegisterFinders() {
  m_dataFinders.push_back(std::make_unique<Finders::ObjectManagerFinder>());
  // In the future, other finders can be added here.
}

bool GameObjectVehicleService::TryFindAllOffsets() {
  if (m_isInitialized) {
    return true;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectVehicleService");

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
    logger->Info("All game object data finders are now ready. Service is fully initialized.");
    return true;
  }

  return m_isInitialized;
}

bool GameObjectVehicleService::AreAllFindersReady() const {
  if (m_dataFinders.empty()) {
    return false;
  }

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      return false;
    }
  }
  return true;
}

bool GameObjectVehicleService::IsFinderReady(const char* finderName) const {
  if (!finderName) return false;
  for (const auto& finder : m_dataFinders) {
    if (finder->GetName() == finderName) {
      return finder->IsReady();
    }
  }
  return false;
}

uintptr_t GameObjectVehicleService::GetLocalPlayerControllerAddr() const {
  uintptr_t trafficManager = GetTrafficManagerAddr();
  if (trafficManager == 0 || m_localPlayerControllerOffset == 0) {
    return 0;
  }
  return *reinterpret_cast<uintptr_t*>(trafficManager + m_localPlayerControllerOffset);
}

std::vector<GameObjectVehicleService::VehicleFullInfo> GameObjectVehicleService::GetAllVehiclesFullInfo() const {
  std::vector<VehicleFullInfo> vehicleInfo;

  if (!m_isInitialized) {
    return vehicleInfo;  // Return empty if service is not ready
  }

  // 1. Get vehicle count
  uintptr_t trafficManager = GetTrafficManagerAddr();
  uint32_t vehicleCount = *reinterpret_cast<uint32_t*>(trafficManager + m_vehicleCountOffset);
  if (vehicleCount == 0 || vehicleCount > 500) {  // Sanity check
    return vehicleInfo;
  }

  // 2. Get pVehicleArrayData
  uintptr_t pVehicleArrayData = *reinterpret_cast<uintptr_t*>(trafficManager + m_pArrayObjectOffset);
  if (!pVehicleArrayData) {
    return vehicleInfo;
  }

  vehicleInfo.reserve(vehicleCount);

  // 3. Loop through the array once
  for (uint32_t i = 0; i < vehicleCount; ++i) {
    uintptr_t pSpawnedVehicleStruct = pVehicleArrayData + (i * m_spawnedVehicleStructSize);
    uintptr_t pVehicleObject = *reinterpret_cast<uintptr_t*>(pSpawnedVehicleStruct);

    if (!pVehicleObject) {
      continue;
    }

    VehicleFullInfo info = {};
    info.pointer = pVehicleObject;

    // Read properties using the dynamic offsets found by the finder
    info.id = *reinterpret_cast<int32_t*>(pVehicleObject + m_vehicleIdOffset);
    info.patience = *reinterpret_cast<float*>(pVehicleObject + m_patienceOffset);
    info.safety = *reinterpret_cast<float*>(pVehicleObject + m_safetyOffset);
    info.target_speed = *reinterpret_cast<float*>(pVehicleObject + m_targetSpeedOffset);
    info.speed_limit = *reinterpret_cast<float*>(pVehicleObject + m_speedLimitOffset);
    info.lane_speed_input = *reinterpret_cast<float*>(pVehicleObject + m_laneSpeedInputOffset);

    info.acceleration = 0.0f;
    info.current_speed = 0.0f;

    // Physical properties like speed and acceleration are managed by a specific sub-component within the vehicle actor.
    if (pVehicleObject && m_vehicleSubObjectOffset) {
      uintptr_t vtable_addr = *reinterpret_cast<uintptr_t*>(pVehicleObject + m_vehicleSubObjectOffset);
      if (vtable_addr) {
        uintptr_t* vtable = reinterpret_cast<uintptr_t*>(vtable_addr);
        using GetFloatFn = float (*)(void*);

        // The virtual methods expect the address of the sub-component itself as the 'this' pointer (RCX).
        void* this_ptr = reinterpret_cast<void*>(pVehicleObject + m_vehicleSubObjectOffset);

        if (m_vtableGetCurrentSpeedOffset) {
          auto pfn = reinterpret_cast<GetFloatFn>(vtable[m_vtableGetCurrentSpeedOffset / 8]);
          if (pfn) info.current_speed = pfn(this_ptr);
        }

        if (m_vtableGetAccelerationOffset) {
          auto pfn = reinterpret_cast<GetFloatFn>(vtable[m_vtableGetAccelerationOffset / 8]);
          if (pfn) info.acceleration = pfn(this_ptr);
        }
      }
    }

    vehicleInfo.push_back(info);
  }

  return vehicleInfo;
}

uintptr_t GameObjectVehicleService::GetPlayerVehiclePtr() const {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectVehicleService");

  uintptr_t trafficManager = GetTrafficManagerAddr();
  if (trafficManager == 0 || m_localPlayerControllerOffset == 0 || m_playerVehicleInControllerOffset == 0) {
    return 0;
  }

  // 1. Get the local player controller pointer
  uintptr_t pControllerAddr = trafficManager + m_localPlayerControllerOffset;
  uintptr_t pController = *reinterpret_cast<uintptr_t*>(pControllerAddr);
  if (!pController) {
    logger->Warn("GetPlayerVehiclePtr: Player Controller is NULL at 0x{:X}", pControllerAddr);
    return 0;
  }

  // 2. Get the player's vehicle (Actor) pointer from the controller
  uintptr_t pVehicleAddr = pController + m_playerVehicleInControllerOffset;
  uintptr_t pVehicle = *reinterpret_cast<uintptr_t*>(pVehicleAddr);

  static uintptr_t lastLoggedVehicle = 0;
  if (pVehicle != lastLoggedVehicle) {
    if (pVehicle) {
      logger->Debug("GetPlayerVehiclePtr: Resolved player truck at 0x{:X}", pVehicle);
    } else {
      logger->Warn("GetPlayerVehiclePtr: Player truck became NULL");
    }
    lastLoggedVehicle = pVehicle;
  }

  return pVehicle;
}

}  // namespace Data::GameData
SPF_NS_END