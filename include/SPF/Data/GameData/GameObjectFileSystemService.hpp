#pragma once

#include "SPF/Namespace.hpp"
#include <cstdint>
#include <vector>
#include <memory>

SPF_NS_BEGIN
namespace Data::GameData {

class IFileSystemDataFinder; // Forward declaration

/**
 * @class GameObjectFileSystemService
 * @brief A singleton service that provides memory offsets and pointers for the game's internal file system (UFS).
 */
class GameObjectFileSystemService {
public:
    static GameObjectFileSystemService& GetInstance();

    GameObjectFileSystemService(const GameObjectFileSystemService&) = delete;
    void operator=(const GameObjectFileSystemService&) = delete;

    void Initialize();
    bool TryFindAllOffsets();
    void Reset();
    bool AreAllFindersReady() const;
    bool IsFinderReady(const char* finderName) const;

    // --- Found Addresses & Offsets (Getters) ---
    uintptr_t GetHomePathPtrAddr() const { return m_homePathPtrAddr; }
    uintptr_t GetDevicesArrayAddr() const { return m_devicesArrayAddr; }
    uintptr_t GetManagersCountAddr() const { return m_managersCountAddr; }
    uint32_t GetMountListHeadOffset() const { return m_mountListHeadOffset; }
    uint32_t GetPhysicalDevicePathOffset() const { return m_physDevicePathOffset; }
    uint32_t GetNodeDeviceOffset() const { return m_nodeDeviceOffset; }
    uint32_t GetNodeVPathOffset() const { return m_nodeVPathOffset; }
    uint32_t GetStringBufferOffset() const { return m_stringBufferOffset; }

    // Redirection to centralized SessionService for core offsets
    uintptr_t GetGamePtrAddr() const;
    uint32_t GetProfileHandleOffset() const;
    intptr_t GetGamePtrAdjustment() const { return 0; } // Adjustment is 0 in v1.59.2+

    // --- Found Addresses & Offsets (Setters for finders) ---
    void SetHomePathPtrAddr(uintptr_t addr) { m_homePathPtrAddr = addr; }
    void SetDevicesArrayAddr(uintptr_t addr) { m_devicesArrayAddr = addr; }
    void SetManagersCountAddr(uintptr_t addr) { m_managersCountAddr = addr; }
    void SetMountListHeadOffset(uint32_t offset) { m_mountListHeadOffset = offset; }
    void SetPhysicalDevicePathOffset(uint32_t offset) { m_physDevicePathOffset = offset; }
    void SetNodeDeviceOffset(uint32_t offset) { m_nodeDeviceOffset = offset; }
    void SetNodeVPathOffset(uint32_t offset) { m_nodeVPathOffset = offset; }
    void SetStringBufferOffset(uint32_t offset) { m_stringBufferOffset = offset; }

private:
    GameObjectFileSystemService() = default;
    ~GameObjectFileSystemService() = default;

    void RegisterFinders();

    bool m_isInitialized = false;
    uintptr_t m_homePathPtrAddr = 0;
    uintptr_t m_devicesArrayAddr = 0;
    uintptr_t m_managersCountAddr = 0;
    uint32_t m_mountListHeadOffset = 0;
    uint32_t m_physDevicePathOffset = 0;
    uint32_t m_nodeDeviceOffset = 0;
    uint32_t m_nodeVPathOffset = 0;
    uint32_t m_stringBufferOffset = 0;

    std::vector<std::unique_ptr<IFileSystemDataFinder>> m_dataFinders;
};

} // namespace Data::GameData
SPF_NS_END
