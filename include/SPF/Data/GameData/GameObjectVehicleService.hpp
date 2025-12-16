#pragma once

#include "SPF/Namespace.hpp"
#include <cstdint>
#include <vector>
#include <memory>

SPF_NS_BEGIN
namespace Data::GameData {

class IObjectDataFinder; // Forward declaration

class GameObjectVehicleService {
public:
    struct VehicleFullInfo {
        int32_t id;
        uintptr_t pointer;
        float patience;
        float safety;
        float target_speed;
        float speed_limit;
        float lane_speed_input;
        float acceleration;
        float current_speed;
    };

    static GameObjectVehicleService& GetInstance();

    GameObjectVehicleService(const GameObjectVehicleService&) = delete;
    void operator=(const GameObjectVehicleService&) = delete;

    void Initialize();
    bool TryFindAllOffsets();
    void Reset();
    bool AreAllFindersReady() const;

    // --- Public Getters ---
    uintptr_t GetTrafficManagerAddr() const { return m_pTrafficManagerAddr; }
    uintptr_t GetPArrayObjectOffset() const { return m_pArrayObjectOffset; }
    uintptr_t GetVehicleCountOffset() const { return m_vehicleCountOffset; }
    uintptr_t GetSpawnedVehicleStructSize() const { return m_spawnedVehicleStructSize; }
    uintptr_t GetVehicleIdOffset() const { return m_vehicleIdOffset; }
    uintptr_t GetPatienceOffset() const { return m_patienceOffset; }
    uintptr_t GetSafetyOffset() const { return m_safetyOffset; }
    uintptr_t GetTargetSpeedOffset() const { return m_targetSpeedOffset; }
    uintptr_t GetSpeedLimitOffset() const { return m_speedLimitOffset; }
    uintptr_t GetLaneSpeedInputOffset() const { return m_laneSpeedInputOffset; }
    std::vector<VehicleFullInfo> GetAllVehiclesFullInfo() const;

    // --- Public Setters (for use by finder implementations) ---
    void SetTrafficManagerAddr(uintptr_t ptr) {
        m_pTrafficManagerAddr = ptr;
    }

    void SetPArrayObjectOffset(uintptr_t offset) {
        m_pArrayObjectOffset = offset;
    }

    void SetVehicleCountOffset(uintptr_t offset) {
        m_vehicleCountOffset = offset;
    }

    void SetSpawnedVehicleStructSize(uintptr_t size) {
        m_spawnedVehicleStructSize = size;
    }

    void SetVehicleIdOffset(uintptr_t offset) {
        m_vehicleIdOffset = offset;
    }

    void SetPatienceOffset(uintptr_t offset) {
        m_patienceOffset = offset;
    }

    void SetSafetyOffset(uintptr_t offset) {
        m_safetyOffset = offset;
    }

    void SetTargetSpeedOffset(uintptr_t offset) {
        m_targetSpeedOffset = offset;
    }

    void SetSpeedLimitOffset(uintptr_t offset) {
        m_speedLimitOffset = offset;
    }

    void SetLaneSpeedInputOffset(uintptr_t offset) {
        m_laneSpeedInputOffset = offset;
    }

private:
    GameObjectVehicleService() = default;
    ~GameObjectVehicleService() = default;

    void RegisterFinders();

    bool m_isInitialized = false;
    uintptr_t m_pTrafficManagerAddr = 0;
    uintptr_t m_pArrayObjectOffset = 0;
    uintptr_t m_vehicleCountOffset = 0;
    uintptr_t m_spawnedVehicleStructSize = 0;
    uintptr_t m_vehicleIdOffset = 0;
    uintptr_t m_patienceOffset = 0;
    uintptr_t m_safetyOffset = 0;
    uintptr_t m_targetSpeedOffset = 0;
    uintptr_t m_speedLimitOffset = 0;
    uintptr_t m_laneSpeedInputOffset = 0;
    std::vector<std::unique_ptr<IObjectDataFinder>> m_dataFinders;
};

} // namespace Data::GameData
SPF_NS_END
