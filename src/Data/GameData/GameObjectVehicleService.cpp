#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Data/GameData/IObjectDataFinder.hpp"
#include "SPF/Data/GameData/Finders/ObjectVehicleManagerFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
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

std::vector<GameObjectVehicleService::VehicleFullInfo> GameObjectVehicleService::GetAllVehiclesFullInfo() const {
    std::vector<VehicleFullInfo> vehicleInfo;

    if (!m_isInitialized) {
        return vehicleInfo; // Return empty if service is not ready
    }


    // 1. Get vehicle count
    uint32_t vehicleCount = *reinterpret_cast<uint32_t*>(m_pTrafficManagerAddr + m_vehicleCountOffset);
    if (vehicleCount == 0 || vehicleCount > 500) { // Sanity check
        return vehicleInfo;
    }

    // 2. Get pVehicleArrayData
    uintptr_t pVehicleArrayData = *reinterpret_cast<uintptr_t*>(m_pTrafficManagerAddr + m_pArrayObjectOffset);
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

        // --- Read properties (Final Corrected Logic from User's Analysis) ---
        info.acceleration = 0.0f;
        info.current_speed = 0.0f;

        if (pVehicleObject) {
            // As per user's detailed memory layout:
            // 1. Get the address of the component's v-table, which is the value at pVehicleObject + 16.
            uintptr_t component_vtable_addr = *(uintptr_t*)(pVehicleObject + 16);

            if (component_vtable_addr) {
                uintptr_t* vtable = (uintptr_t*)component_vtable_addr;
                using GetFloatFn = float (*)(void*);

                // The 'this' pointer for these calls is unconventional, as per `LEA RCX,[RSI + 0x10]`.
                // It's the address of the v-table member itself within the main object.
                void* this_for_call = (void*)(pVehicleObject + 16);

                // 2. The second entry in this v-table (offset +8) is GetCurrentSpeed.
                uintptr_t pfnGetCurrentSpeedAddr = vtable[1];
                if (pfnGetCurrentSpeedAddr) {
                    GetFloatFn GetCurrentSpeed = (GetFloatFn)pfnGetCurrentSpeedAddr;
                    info.current_speed = GetCurrentSpeed(this_for_call); 
                }

                // 3. The third entry in this v-table (offset +16) is GetAcceleration.
                uintptr_t pfnGetAccelerationAddr = vtable[2];
                if (pfnGetAccelerationAddr) {
                    GetFloatFn GetAcceleration = (GetFloatFn)pfnGetAccelerationAddr;
                    info.acceleration = GetAcceleration(this_for_call); 
                }
            }
        }
        
        vehicleInfo.push_back(info);
    }

    return vehicleInfo;
}

} // namespace Data::GameData
SPF_NS_END