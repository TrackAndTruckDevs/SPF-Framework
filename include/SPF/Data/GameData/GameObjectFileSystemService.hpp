#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SPF::Data::GameData {

class IFileSystemDataFinder;  // Forward declaration

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
  uint32_t GetNodeNextOffset() const { return m_nodeNextOffset; }
  uint32_t GetNodeOrderOffset() const { return m_nodeOrderOffset; }
  uint32_t GetPoolCountOffset() const { return m_poolCountOffset; }

  // --- VFS Mount API (Found Addresses) ---
  uintptr_t GetUfsMountDeviceAddr() const { return m_ufsMountDeviceAddr; }
  uintptr_t GetUfsUnmountDeviceAddr() const { return m_ufsUnmountDeviceAddr; }
  uintptr_t GetPoolArrayAddr() const { return m_poolArrayAddr; }
  uintptr_t GetPoolCountAddr() const { return m_poolCountAddr; }

  // Redirection to centralized SessionService for core offsets
  uintptr_t GetGamePtrAddr() const;
  uint32_t GetProfileHandleOffset() const;
  intptr_t GetGamePtrAdjustment() const { return 0; }  // Adjustment is 0 in v1.59.2+

  // --- Found Addresses & Offsets (Setters for finders) ---
  void SetHomePathPtrAddr(uintptr_t addr) { m_homePathPtrAddr = addr; }
  void SetDevicesArrayAddr(uintptr_t addr) { m_devicesArrayAddr = addr; }
  void SetManagersCountAddr(uintptr_t addr) { m_managersCountAddr = addr; }
  void SetMountListHeadOffset(uint32_t offset) { m_mountListHeadOffset = offset; }
  void SetPhysicalDevicePathOffset(uint32_t offset) { m_physDevicePathOffset = offset; }
  void SetNodeDeviceOffset(uint32_t offset) { m_nodeDeviceOffset = offset; }
  void SetNodeVPathOffset(uint32_t offset) { m_nodeVPathOffset = offset; }
  void SetStringBufferOffset(uint32_t offset) { m_stringBufferOffset = offset; }
  void SetNodeNextOffset(uint32_t offset) { m_nodeNextOffset = offset; }
  void SetNodeOrderOffset(uint32_t offset) { m_nodeOrderOffset = offset; }
  void SetPoolCountOffset(uint32_t offset) { m_poolCountOffset = offset; }

  void SetUfsMountDeviceAddr(uintptr_t addr) { m_ufsMountDeviceAddr = addr; }
  void SetUfsUnmountDeviceAddr(uintptr_t addr) { m_ufsUnmountDeviceAddr = addr; }
  void SetPoolArrayAddr(uintptr_t addr) { m_poolArrayAddr = addr; }
  void SetPoolCountAddr(uintptr_t addr) { m_poolCountAddr = addr; }

  // --- VFS Mount API ---
  bool MountVfsFolder(const std::string& pluginName, const char* physicalPath, int poolIndex, int order, std::string& outVfsPath);
  void UnmountVfsFolders(const std::string& pluginName);

  // --- VFS Mount Enumeration (live game mount tables) ---
  struct VfsMountInfo {
    std::string vpath;
    std::string physicalPath;
    int poolIndex = -1;
    int order = 0;
  };

  int GetVfsMountCount() const;
  bool GetVfsMountAt(int index, VfsMountInfo& out) const;

  // Cached snapshot of the live mount tables; rebuilt only when the cheap
  // fingerprint (pool count + per-pool mount count/head) changes.
  const std::vector<VfsMountInfo>& GetVfsMountsSnapshot() const;

 private:
  GameObjectFileSystemService() = default;
  ~GameObjectFileSystemService() = default;

  void RegisterFinders();

  // Visits every live UFS mount node across all pools; fn returns false to stop
  // early. Returns the number of nodes visited.
  int WalkVfsMounts(const std::function<bool(int, uintptr_t)>& fn) const;
  VfsMountInfo MakeVfsMountInfo(int poolIndex, uintptr_t node) const;
  uint64_t ComputeMountFingerprint() const;

  mutable std::vector<VfsMountInfo> m_vfsSnapshot;
  mutable uint64_t m_vfsSnapshotFingerprint = 0;
  mutable bool m_vfsSnapshotValid = false;

  bool m_isInitialized = false;
  uintptr_t m_homePathPtrAddr = 0;
  uintptr_t m_devicesArrayAddr = 0;
  uintptr_t m_managersCountAddr = 0;
  uint32_t m_mountListHeadOffset = 0;
  uint32_t m_physDevicePathOffset = 0;
  uint32_t m_nodeDeviceOffset = 0;
  uint32_t m_nodeVPathOffset = 0;
  uint32_t m_stringBufferOffset = 0;
  uint32_t m_nodeNextOffset = 0;
  uint32_t m_nodeOrderOffset = 0;
  // Per-pool mount count field (UFS_RegisterMount increments it, UFS_UnmountDevice decrements).
  uint32_t m_poolCountOffset = 0;

  // VFS mount API addresses (resolved by FileSystemDataFinder Phase 3)
  uintptr_t m_ufsMountDeviceAddr = 0;
  uintptr_t m_ufsUnmountDeviceAddr = 0;
  uintptr_t m_poolArrayAddr = 0;
  uintptr_t m_poolCountAddr = 0;

  struct VfsMountRecord {
    std::string pluginName;
    std::string vpath;
    std::string physicalPath;
    int poolIndex = 0;
    uintptr_t device = 0;
  };

  std::vector<VfsMountRecord> m_vfsMounts;
  std::mutex m_vfsMountsMutex;

  std::vector<std::unique_ptr<IFileSystemDataFinder>> m_dataFinders;
};

}  // namespace SPF::Data::GameData
