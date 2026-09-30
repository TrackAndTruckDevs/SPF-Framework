#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"

#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Data/GameData/IFileSystemDataFinder.hpp"
#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace SPF::Data::GameData {

GameObjectFileSystemService& GameObjectFileSystemService::GetInstance() {
  static GameObjectFileSystemService instance;
  return instance;
}

void GameObjectFileSystemService::Initialize() {
  if (m_isInitialized) return;
  RegisterFinders();
  m_isInitialized = true;
}

void GameObjectFileSystemService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::FileSystemDataFinder>()); }

bool GameObjectFileSystemService::TryFindAllOffsets() {
  bool allFound = true;
  for (auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (!finder->TryFindOffsets(*this)) {
        allFound = false;
      }
    }
  }
  return allFound;
}

uintptr_t GameObjectFileSystemService::GetGamePtrAddr() const { return GameObjectSessionService::GetInstance().GetGamePtrAddr(); }

uint32_t GameObjectFileSystemService::GetProfileHandleOffset() const { return GameObjectSessionService::GetInstance().GetProfileHandleOffset(); }

void GameObjectFileSystemService::Reset() {
  m_homePathPtrAddr = 0;
  m_devicesArrayAddr = 0;
  m_managersCountAddr = 0;
  m_mountListHeadOffset = 0;
  m_physDevicePathOffset = 0;
  m_nodeDeviceOffset = 0;
  m_nodeVPathOffset = 0;
  m_stringBufferOffset = 0;
  m_ufsMountDeviceAddr = 0;
  m_ufsUnmountDeviceAddr = 0;
  m_poolArrayAddr = 0;
  m_poolCountAddr = 0;
  // m_vfsMounts intentionally kept: live UFS mounts must survive Reset.

  for (auto& finder : m_dataFinders) {
    // In the current architecture, finders handle their own state
  }
}

bool GameObjectFileSystemService::AreAllFindersReady() const {
  if (m_dataFinders.empty()) return false;
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

bool GameObjectFileSystemService::IsFinderReady(const char* finderName) const {
  for (const auto& finder : m_dataFinders) {
    if (std::string(finder->GetName()) == finderName) {
      return finder->IsReady();
    }
  }
  return false;
}

// --- VFS Mount API ---

namespace {

// UFS_MountDevice mountParams: 40 bytes, layout from ufs_mount_home_dir /
// ufs_setup_screenshots_dir call sites (type-1 directory device).
struct UfsMountParams {
  int32_t type;          // 1 = standard directory device
  int32_t pool;          // pool index (0=core, 1=user, 2=mod, 3=scs)
  const char* physical;  // physical disk path
  const char* vpath;     // virtual path in VFS
  int32_t field24;       // must be 0
  int32_t order;         // mount order (650 = music, 1000 = temp)
  int32_t field32;       // must be 0
  int32_t field36;       // must be 0
};
// Game ABI: UFS_MountDevice reads the params block as int* up to index 9 (+0x24),
// so the struct end is +0x28 = 40 bytes. Every game call site (ufs_mount_home_dir,
// setup_ufs music/pictures) builds an identical 0x28-byte stack block:
// {type, pool, physical*, vpath*, field24, order, field32, field36}.
static_assert(sizeof(UfsMountParams) == 40, "UFS mountParams must be 40 bytes");

using UfsMountDeviceFn = uintptr_t (*)(UfsMountParams*);
using UfsUnmountDeviceFn = void (*)(uintptr_t pool, uintptr_t device);

// Default pool for plugin mounts (API pool_index -1). Index 1 = "user" per the game's
// pool-name table used by UFS_RegisterMount (table[pool_index+0x70] when logging
// "[fs] device %s mounted to %s pool."): [0]="core" [1]="user" [2]="mod" [3]="scs".
// All game user-content mounts (home, music, pictures) pass pool = 1 as well.
constexpr int kVfsDefaultPool = 1;  // "user"

// UFS mount node layout written by UFS_RegisterMount (node alloc via pool+0x78
// vtable, inserted at list head pool+0x88):
//   +0x00 next      — `*node = old_first` (doubly-linked list, next field);
//                     (finder: m_nodeNextOffset);
//   +0x08 prev      — written alongside; walker follows +0x00 only.
//   +0x10 device    — `node[2] = device` (finder: m_nodeDeviceOffset);
//   +0x18 vpath     — prism_string via `node[3]` (finder: m_nodeVPathOffset);
//   +0x38 order     — `*(uint32*)(node + 7) = param_4` = the order argument of
//                     UFS_RegisterMount (from MountParams.order);
//                     (finder: m_nodeOrderOffset);
//   +0x3C/+0x3D     — flag bytes param_5/param_6.

// Safety cap for ReadGameCString: any legitimate game path is far below 1 KiB;
// no NUL within the cap means a garbage pointer → refuse to read.
constexpr size_t kVfsCStringCap = 1024;

// Hard caps: the game has 5 pools and tens of mounts. A corrupted chain must
// not spin through heap garbage for millions of hops.
constexpr size_t kVfsMaxPools = 8;
constexpr int kVfsMaxNodes = 4096;

// Page-granular bounded read of a game-owned C-string: refuses to return data
// when no NUL terminator is found within the cap (garbage guard).
std::string ReadGameCString(const char* src) {
  if (!src) return {};
  const uintptr_t addr = reinterpret_cast<uintptr_t>(src);
  if (!Utils::PatternFinder::IsValidAddress(addr)) return {};
  for (size_t i = 0; i < kVfsCStringCap; ++i) {
    const uintptr_t cur = addr + i;
    if ((cur & 0xFFF) == 0 && !Utils::PatternFinder::IsValidAddress(cur)) return {};
    if (src[i] == '\0') return std::string(src, i);
  }
  return {};
}

}  // namespace

bool GameObjectFileSystemService::MountVfsFolder(const std::string& pluginName, const char* physicalPath, int poolIndex, int order, std::string& outVfsPath) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectFileSystemService");

  if (!physicalPath || !physicalPath[0]) {
    if (logger) logger->Error("VFS mount: empty physical path for plugin '{}'", pluginName);
    return false;
  }

  const std::string vpath = "/spf/" + pluginName;
  std::lock_guard<std::mutex> lock(m_vfsMountsMutex);

  // Idempotent: same plugin + same directory -> same virtual path.
  for (const auto& rec : m_vfsMounts) {
    if (rec.pluginName == pluginName && rec.vpath == vpath) {
      if (rec.physicalPath == physicalPath) {
        outVfsPath = vpath;
        if (logger) logger->Info("VFS mount: '{}' already mounted for plugin '{}'", vpath, pluginName);
        return true;
      }
      if (logger) logger->Error("VFS mount: '{}' already mounted from '{}' for plugin '{}', cannot remount", vpath, rec.physicalPath, pluginName);
      return false;
    }
  }

  if (!m_ufsMountDeviceAddr || !m_ufsUnmountDeviceAddr || !m_poolArrayAddr || !m_poolCountAddr) {
    if (logger) logger->Error("VFS mount: finder addresses not ready for plugin '{}'", pluginName);
    return false;
  }

  const int pool = (poolIndex < 0) ? kVfsDefaultPool : poolIndex;
  if (pool > 3) {
    if (logger) logger->Error("VFS mount: invalid pool index {} for plugin '{}'", pool, pluginName);
    return false;
  }
  const size_t poolCount = *reinterpret_cast<size_t*>(m_poolCountAddr);
  if (static_cast<size_t>(pool) >= poolCount) {
    if (logger) logger->Error("VFS mount: pool index {} out of range ({}) for plugin '{}'", pool, poolCount, pluginName);
    return false;
  }

  UfsMountParams params{};
  params.type = 1;
  params.pool = pool;
  params.physical = physicalPath;
  params.vpath = vpath.c_str();
  params.field24 = 0;
  params.order = order;
  params.field32 = 0;
  params.field36 = 0;

  auto mountFn = reinterpret_cast<UfsMountDeviceFn>(m_ufsMountDeviceAddr);
  uintptr_t device = mountFn(&params);
  if (!device) {
    if (logger) logger->Error("VFS mount: UFS_MountDevice failed for '{}' (plugin '{}')", vpath, pluginName);
    return false;
  }

  // UFS_MountDevice returns the caller's device reference; the pool list holds a raw
  // non-owning pointer (UFS_RegisterMount does not addref), so it must be released
  // after UFS_UnmountDevice unlinks the pool node.
  m_vfsMounts.push_back({pluginName, vpath, physicalPath, pool, device});
  if (logger) logger->Info("VFS mount: '{}' -> '{}' (pool={}) for plugin '{}'", vpath, physicalPath, pool, pluginName);
  outVfsPath = vpath;
  return true;
}

void GameObjectFileSystemService::UnmountVfsFolders(const std::string& pluginName) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameObjectFileSystemService");
  std::lock_guard<std::mutex> lock(m_vfsMountsMutex);

  auto it = m_vfsMounts.begin();
  while (it != m_vfsMounts.end()) {
    if (it->pluginName != pluginName) {
      ++it;
      continue;
    }

    if (m_ufsUnmountDeviceAddr && m_poolArrayAddr && m_poolCountAddr) {
      const size_t poolCount = *reinterpret_cast<size_t*>(m_poolCountAddr);
      uintptr_t poolBase = *reinterpret_cast<uintptr_t*>(m_poolArrayAddr);
      if (poolBase && static_cast<size_t>(it->poolIndex) < poolCount) {
        uintptr_t pool = *reinterpret_cast<uintptr_t*>(poolBase + static_cast<uintptr_t>(it->poolIndex) * sizeof(uintptr_t));
        auto unmountFn = reinterpret_cast<UfsUnmountDeviceFn>(m_ufsUnmountDeviceAddr);
        unmountFn(pool, it->device);

        // Drop the caller reference — same (**vtable)(device, 1) pattern the game
        // uses in its UFS_MountDevice failure paths.
        uintptr_t vtable = *reinterpret_cast<uintptr_t*>(it->device);
        if (vtable) {
          auto releaseFn = reinterpret_cast<void (*)(uintptr_t, int)>(*reinterpret_cast<uintptr_t*>(vtable));
          releaseFn(it->device, 1);
        }
        if (logger) logger->Info("VFS unmount: '{}' (pool={}) for plugin '{}'", it->vpath, it->poolIndex, pluginName);
      } else if (logger) {
        logger->Error("VFS unmount: invalid pool {} for '{}', mount leaked", it->poolIndex, it->vpath);
      }
    } else if (logger) {
      logger->Error("VFS unmount: finder addresses not ready, mount '{}' leaked", it->vpath);
    }

    it = m_vfsMounts.erase(it);
  }
}

int GameObjectFileSystemService::WalkVfsMounts(const std::function<bool(int, uintptr_t)>& fn) const {
  if (!m_poolArrayAddr || !m_poolCountAddr || !m_mountListHeadOffset || !m_nodeDeviceOffset || !m_nodeVPathOffset || !m_physDevicePathOffset) return 0;
  if (!Utils::PatternFinder::IsValidAddress(m_poolCountAddr) || !Utils::PatternFinder::IsValidAddress(m_poolArrayAddr)) return 0;

  const size_t poolCount = *reinterpret_cast<size_t*>(m_poolCountAddr);
  const uintptr_t poolArray = *reinterpret_cast<uintptr_t*>(m_poolArrayAddr);
  if (!poolArray || poolCount == 0 || !Utils::PatternFinder::IsValidAddress(poolArray)) return 0;
  const size_t poolLimit = poolCount < kVfsMaxPools ? poolCount : kVfsMaxPools;

  int visited = 0;
  for (size_t p = 0; p < poolLimit; ++p) {
    const uintptr_t slot = poolArray + p * sizeof(uintptr_t);
    if (!Utils::PatternFinder::IsValidAddress(slot)) break;
    const uintptr_t pool = *reinterpret_cast<uintptr_t*>(slot);
    if (!pool || !Utils::PatternFinder::IsValidAddress(pool)) continue;

    // The list-head field at pool+headOffset doubles as the sentinel node
    // address: its value is the first node, and an empty list points at itself.
    const uintptr_t headField = pool + m_mountListHeadOffset;
    if (!Utils::PatternFinder::IsValidAddress(headField)) continue;
    uintptr_t node = *reinterpret_cast<uintptr_t*>(headField);
    while (node && node != headField && Utils::PatternFinder::IsValidAddress(node)) {
      ++visited;
      if (visited > kVfsMaxNodes) return visited;
      if (!fn(static_cast<int>(p), node)) return visited;
      node = *reinterpret_cast<uintptr_t*>(node + m_nodeNextOffset);
    }
  }
  return visited;
}

GameObjectFileSystemService::VfsMountInfo GameObjectFileSystemService::MakeVfsMountInfo(int poolIndex, uintptr_t node) const {
  VfsMountInfo info;
  info.poolIndex = poolIndex;

  if (Utils::PatternFinder::IsValidAddress(node + m_nodeOrderOffset)) {
    info.order = *reinterpret_cast<int*>(node + m_nodeOrderOffset);
  }

  const uintptr_t vpathAddr = node + m_nodeVPathOffset;
  if (Utils::PatternFinder::IsValidAddress(vpathAddr)) {
    info.vpath = Hooks::GameTools::PrismStringResolver::GetInstance().ReadString(reinterpret_cast<const void*>(vpathAddr));
  }

  if (Utils::PatternFinder::IsValidAddress(node + m_nodeDeviceOffset)) {
    const uintptr_t device = *reinterpret_cast<uintptr_t*>(node + m_nodeDeviceOffset);
    if (device && Utils::PatternFinder::IsValidAddress(device + m_physDevicePathOffset)) {
      info.physicalPath = ReadGameCString(*reinterpret_cast<const char**>(device + m_physDevicePathOffset));
    }
  }
  return info;
}

int GameObjectFileSystemService::GetVfsMountCount() const { return static_cast<int>(GetVfsMountsSnapshot().size()); }

bool GameObjectFileSystemService::GetVfsMountAt(int index, VfsMountInfo& out) const {
  const auto& snapshot = GetVfsMountsSnapshot();
  if (index < 0 || index >= static_cast<int>(snapshot.size())) return false;
  out = snapshot[index];
  return true;
}

uint64_t GameObjectFileSystemService::ComputeMountFingerprint() const {
  if (!m_poolArrayAddr || !m_poolCountAddr || !Utils::PatternFinder::IsValidAddress(m_poolCountAddr)) return 0;
  const size_t poolCount = *reinterpret_cast<size_t*>(m_poolCountAddr);
  const uintptr_t poolArray = *reinterpret_cast<uintptr_t*>(m_poolArrayAddr);
  if (!poolArray || !Utils::PatternFinder::IsValidAddress(poolArray)) return 0;

  uint64_t fp = static_cast<uint64_t>(poolCount) * 0x9E3779B97F4A7C15ull;
  const size_t poolLimit = poolCount < kVfsMaxPools ? poolCount : kVfsMaxPools;
  for (size_t p = 0; p < poolLimit; ++p) {
    const uintptr_t slot = poolArray + p * sizeof(uintptr_t);
    if (!Utils::PatternFinder::IsValidAddress(slot)) break;
    const uintptr_t pool = *reinterpret_cast<uintptr_t*>(slot);
    if (!pool || !Utils::PatternFinder::IsValidAddress(pool)) continue;
    uint64_t h = static_cast<uint64_t>(pool);
    if (Utils::PatternFinder::IsValidAddress(pool + m_poolCountOffset)) {
      h = h * 31 + *reinterpret_cast<uint32_t*>(pool + m_poolCountOffset);
    }
    if (Utils::PatternFinder::IsValidAddress(pool + m_mountListHeadOffset)) {
      h = h * 31 + static_cast<uint64_t>(*reinterpret_cast<uintptr_t*>(pool + m_mountListHeadOffset));
    }
    fp = fp * 1000003ull + h;
  }
  return fp;
}

const std::vector<GameObjectFileSystemService::VfsMountInfo>& GameObjectFileSystemService::GetVfsMountsSnapshot() const {
  const uint64_t fp = ComputeMountFingerprint();
  if (m_vfsSnapshotValid && fp == m_vfsSnapshotFingerprint) return m_vfsSnapshot;

  m_vfsSnapshot.clear();
  WalkVfsMounts([this](int poolIndex, uintptr_t node) {
    m_vfsSnapshot.push_back(MakeVfsMountInfo(poolIndex, node));
    return true;
  });
  m_vfsSnapshotFingerprint = fp;
  m_vfsSnapshotValid = true;
  return m_vfsSnapshot;
}

}  // namespace SPF::Data::GameData
