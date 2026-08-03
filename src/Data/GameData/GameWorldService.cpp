#include "SPF/Data/GameData/GameWorldService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/SEHGuard.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>

SPF_NS_BEGIN
namespace Data::GameData {

namespace {

// /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14041c3a0[14041c3a0]) con_cmd_goto ---/
// The city list is the full kdop array hosted on the GameplayManager object.
//   gm   = *(GameplayManager slot);              (ManagerCoreService -> base+0x3554CA0)
//   data = *(gm + 0x18);                          -> array_t<owner_ptr_t<kdop_item_t>> data
//   count = *(gm + 0x20);                         -> element count
//   item = *(data + i*8);                         -> kdop_item_t
//   item_type  = *(byte*)(item + 0x0A) == 0x0C    -> city kdop item
//   item_flags = *(byte*)(item + 0x34)            -> bit0 clear = city candidate
//   record     = *(qword*)(item + 0x48)           -> city record (non-null, even)
//   name       = prism::string at (record + 0x10) -> buffer pointer stored at (record + 0x18)
//   point      = GetPoint(item, 0) via vtable slot 0x70 -> int32[3] {X, Y, Z} in 1/256

typedef uint32_t(__fastcall* KdopPointCountFn_t)(uintptr_t item);
typedef uintptr_t(__fastcall* KdopGetPointFn_t)(uintptr_t item, uint32_t index);

/**
 * @brief Copies a prism::string field out of a city_data record.
 * @details The attribute offset points at the prism::string object; its buffer
 *          pointer lives at object + 0x08.
 */
int CopyRecordString(uintptr_t record, intptr_t attrOffset, intptr_t bufOffset, char* outBuffer, int bufferSize) {
  if (!outBuffer || bufferSize <= 0) return -1;
  outBuffer[0] = '\0';
  if (!record || attrOffset == 0 || bufOffset == 0) return -1;

  uintptr_t strObj = record + attrOffset;
  if (!Utils::PatternFinder::IsValidAddress(strObj) || !Utils::PatternFinder::IsValidAddress(strObj + bufOffset)) return -1;

  uintptr_t bufPtr = *(uintptr_t*)(strObj + bufOffset);
  if (!Utils::PatternFinder::IsValidAddress(bufPtr)) return -1;

  int len = 0;
  while (len < bufferSize - 1) {
    uintptr_t chAddr = bufPtr + len;
    if (!Utils::PatternFinder::IsValidAddress(chAddr)) break;
    char c = *(char*)chAddr;
    if (c == '\0') break;
    outBuffer[len++] = c;
  }
  outBuffer[len] = '\0';
  return len;
}

uint32_t ReadRecordU32(uintptr_t record, intptr_t attrOffset, uint32_t fallback = 0) {
  if (!record || attrOffset == 0) return fallback;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return fallback;
  return *(uint32_t*)(record + attrOffset);
}

bool WriteRecordU32(uintptr_t record, intptr_t attrOffset, uint32_t val) {
  if (!record || attrOffset == 0) return false;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return false;
  *(uint32_t*)(record + attrOffset) = val;
  return true;
}

float ReadRecordFloat(uintptr_t record, intptr_t attrOffset, float fallback = 0.0f) {
  if (!record || attrOffset == 0) return fallback;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return fallback;
  return *(float*)(record + attrOffset);
}

bool ReadRecordBoolByte(uintptr_t record, intptr_t attrOffset) {
  if (!record || attrOffset == 0) return false;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return false;
  return *(uint8_t*)(record + attrOffset) != 0;
}

bool WriteRecordBoolByte(uintptr_t record, intptr_t attrOffset, bool val) {
  if (!record || attrOffset == 0) return false;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return false;
  *(uint8_t*)(record + attrOffset) = val ? 1 : 0;
  return true;
}

bool WriteRecordFloat(uintptr_t record, intptr_t attrOffset, float val) {
  if (!record || attrOffset == 0) return false;
  if (!Utils::PatternFinder::IsValidAddress(record + attrOffset)) return false;
  *(float*)(record + attrOffset) = val;
  return true;
}

// Layout of an embedded array_t<float> object: { vptr@+0, data@+8, count@+0x10, cap@+0x18 }.
constexpr intptr_t kArrayDataOffset = 0x08;    // array_t<T> -> element buffer
constexpr intptr_t kArrayCountOffset = 0x10;   // array_t<T> -> element count

/** Number of per-zoom map offset entries. */
constexpr size_t kMapOffsetsCount = 8;

/**
 * @brief Reads the first element of an embedded array_t<float> field.
 * @return value[0], or 0.0f when the array is empty/unavailable.
 */
float ReadRecordFloatArrayFirst(uintptr_t record, intptr_t attrOffset) {
  if (!record || attrOffset == 0) return 0.0f;
  uintptr_t arrObj = record + attrOffset;
  if (!Utils::PatternFinder::IsValidAddress(arrObj + kArrayCountOffset)) return 0.0f;
  uintptr_t count = *(uintptr_t*)(arrObj + kArrayCountOffset);
  if (count == 0) return 0.0f;
  uintptr_t dataPtr = *(uintptr_t*)(arrObj + kArrayDataOffset);
  if (!Utils::PatternFinder::IsValidAddress(dataPtr)) return 0.0f;
  return *(float*)dataPtr;
}

/**
 * @brief Reads the elements of an embedded array_t<float> field.
 * @param out Buffer receiving up to maxCount elements.
 * @return The element count actually available (clamped to maxCount), or 0.
 */
size_t ReadRecordFloatArray(uintptr_t record, intptr_t attrOffset, float* out, size_t maxCount) {
  if (!record || attrOffset == 0 || !out || maxCount == 0) return 0;
  uintptr_t arrObj = record + attrOffset;
  if (!Utils::PatternFinder::IsValidAddress(arrObj + kArrayCountOffset)) return 0;
  uintptr_t count = *(uintptr_t*)(arrObj + kArrayCountOffset);
  if (count == 0) return 0;
  uintptr_t dataPtr = *(uintptr_t*)(arrObj + kArrayDataOffset);
  if (!Utils::PatternFinder::IsValidAddress(dataPtr)) return 0;
  size_t n = (size_t)count;
  if (n > maxCount) n = maxCount;
  for (size_t i = 0; i < n; ++i) out[i] = ((float*)dataPtr)[i];
  return n;
}

/**
 * @brief Writes the first element of an embedded array_t<float> field.
 * @return True when the array has at least one element and the write succeeded.
 */
bool WriteRecordFloatArrayFirst(uintptr_t record, intptr_t attrOffset, float val) {
  if (!record || attrOffset == 0) return false;
  uintptr_t arrObj = record + attrOffset;
  if (!Utils::PatternFinder::IsValidAddress(arrObj + kArrayCountOffset)) return false;
  uintptr_t count = *(uintptr_t*)(arrObj + kArrayCountOffset);
  if (count == 0) return false;
  uintptr_t dataPtr = *(uintptr_t*)(arrObj + kArrayDataOffset);
  if (!Utils::PatternFinder::IsValidAddress(dataPtr)) return false;
  *(float*)dataPtr = val;
  return true;
}

/**
 * @brief Writes the elements of an embedded array_t<float> field.
 * @return True when the array holds at least count elements and the write succeeded.
 */
bool WriteRecordFloatArray(uintptr_t record, intptr_t attrOffset, const float* values, size_t count) {
  if (!record || attrOffset == 0 || !values || count == 0) return false;
  uintptr_t arrObj = record + attrOffset;
  if (!Utils::PatternFinder::IsValidAddress(arrObj + kArrayCountOffset)) return false;
  uintptr_t cap = *(uintptr_t*)(arrObj + kArrayCountOffset);
  if (cap < count) return false;
  uintptr_t dataPtr = *(uintptr_t*)(arrObj + kArrayDataOffset);
  if (!Utils::PatternFinder::IsValidAddress(dataPtr)) return false;
  for (size_t i = 0; i < count; ++i) ((float*)dataPtr)[i] = values[i];
  return true;
}

}  // namespace

GameWorldService::GameWorldService() = default;

GameWorldService& GameWorldService::GetInstance() {
  static GameWorldService instance;
  return instance;
}

void GameWorldService::Initialize() {
  RegisterFinders();

  m_isInitialized = false;
}

void GameWorldService::Shutdown() {
  if (m_isInitialized) {
    m_isInitialized = false;

    m_timeOffset = 0;
    m_simulationTimeOffset = 0;
    m_subMinuteSecondsOffset = 0;
    m_realPlayTimeOffset = 0;
    m_realPlaySecondsOffset = 0;
    m_mapScaleOffset = 0;
    m_globalWarpOffset = 0;
    m_pauseStatusOffset = 0;
    m_realDeltaTimeOffset = 0;
    m_updateFnAddr = 0;
    m_globalHaltOffset = 0;
    m_simulationHaltOffset = 0;
    m_trafficHaltOffset = 0;

    m_cityCache.clear();
    m_cityCacheDataPtr = 0;
    m_cityCacheCount = 0;
  }
}

void GameWorldService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::WorldDataFinder>()); }

uintptr_t GameWorldService::ResolveEnvironmentBase() const {
  uintptr_t slot = ManagerCoreService::GetInstance().GetGameplayManagerAddr();
  if (!Utils::PatternFinder::IsValidAddress(slot)) return 0;
  return *(uintptr_t*)slot;
}

bool GameWorldService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  // GameWorldService depends on GameplayManager and its Environment Object offset
  // (both resolved by ManagerCoreService). Do not resolve offsets until the
  // manager is available to avoid null dereferences.
  if (!ManagerCoreService::GetInstance().IsGameplayManagerReady() || !ManagerCoreService::GetInstance().IsEnvObjectOffsetReady()) {
    return false;
  }

  bool all_critical_found_this_pass = true;

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (!finder->TryFindOffsets(*this)) {
        if (strcmp(finder->GetName(), "WorldDataFinder") == 0) {
          all_critical_found_this_pass = false;
        }
      }
    }
  }

  if (all_critical_found_this_pass && AreAllFindersReady()) {
    m_isInitialized = true;
    return true;
  }

  return m_isInitialized;
}

bool GameWorldService::IsReady() {
  return m_isInitialized && AreAllFindersReady() && ManagerCoreService::GetInstance().IsGameplayManagerReady() &&
         ManagerCoreService::GetInstance().IsEnvObjectOffsetReady();
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
    if (!finder->IsReady()) return false;
  }
  return true;
}

// --- World Manipulation Methods ---

uint32_t GameWorldService::GetPreviewTime() {
  if (!m_isInitialized) return 0;

  // This retrieves the visual environment time (skybox/lighting state).
  // Note: This value is distinct from the actual game simulation clock.
  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return 0;

  uintptr_t envObject = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());

  if (!envObject) return 0;

  return *(uint32_t*)(envObject + m_timeOffset);
}

void GameWorldService::SetPreviewTime(uint32_t totalMinutes) {
  if (!m_isInitialized) return;

  uint32_t normalizedMinutes = totalMinutes % (1440 * 7);  // Use full week cycle for visual consistency

  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return;

  uintptr_t envObject = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!envObject) return;

  // Update the visual time minutes (used for skybox and shadow calculations).
  // In the game engine, if the simulation is unpaused, this value will be
  // overwritten by the real game time logic on the next frame unless
  // auto-update is disabled at 0x46c4.
  *(uint32_t*)(envObject + m_timeOffset) = normalizedMinutes;
  *(float*)(envObject + m_timeOffset + 4) = 0.0f;  // Visual seconds

  typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
  UpdateEnv_t UpdateEnv = (UpdateEnv_t)m_updateFnAddr;

  if (UpdateEnv) {
    UpdateEnv(envObject);
  }
}

uint32_t GameWorldService::GetSimulationTime() {
  if (!m_isInitialized) return 0;

  uintptr_t timeMgrPtrAddr = ManagerCoreService::GetInstance().GetTimeMgrPtrAddr();
  if (!timeMgrPtrAddr) return 0;

  uintptr_t timeMgr = *(uintptr_t*)timeMgrPtrAddr;
  if (!timeMgr) return 0;

  return *(uint32_t*)(timeMgr + m_simulationTimeOffset);
}

void GameWorldService::SetSimulationTime(uint32_t totalMinutes) {
  if (!m_isInitialized) return;

  uintptr_t timeMgrPtrAddr = ManagerCoreService::GetInstance().GetTimeMgrPtrAddr();
  if (!timeMgrPtrAddr) return;

  uintptr_t timeMgr = *(uintptr_t*)timeMgrPtrAddr;
  if (!timeMgr) return;

  *(uint32_t*)(timeMgr + m_simulationTimeOffset) = totalMinutes;
  *(float*)(timeMgr + m_subMinuteSecondsOffset) = 0.0f;
}

uint32_t GameWorldService::GetRealPlayTime() {
  if (!m_isInitialized) return 0;

  uintptr_t timeMgrPtrAddr = ManagerCoreService::GetInstance().GetTimeMgrPtrAddr();
  if (!timeMgrPtrAddr) return 0;

  uintptr_t timeMgr = *(uintptr_t*)timeMgrPtrAddr;
  if (!timeMgr) return 0;

  // In version 1.60+, Real Play Time is part of an array_t<uint32_t>.
  // We detect this by the offset value (e.g., 0x1B98 vs 0x1C8).
  if (m_realPlayTimeOffset > 0x1000) {
    // Read the data pointer from the array_t structure (at offset +0x08).
    // As per Ghidra 1.60 analysis, the structure is accessed via an array helper.
    uintptr_t arrayDataPtr = *(uintptr_t*)(timeMgr + m_realPlayTimeOffset + 0x08);
    if (!arrayDataPtr) return 0;

    // Read the first element (minutes) which corresponds to the local player.
    return *(uint32_t*)arrayDataPtr;
  }

  // Version 1.59 and older: direct uint32_t access.
  return *(uint32_t*)(timeMgr + m_realPlayTimeOffset);
}

float GameWorldService::GetMapScale() {
  if (!m_isInitialized) return 1.0f;

  uintptr_t envBaseObj = ResolveEnvironmentBase();
  if (!envBaseObj) return 1.0f;

  return *(float*)(envBaseObj + m_mapScaleOffset);
}

uint32_t GameWorldService::GetGameDay() { return GetSimulationTime() / 1440; }

uint32_t GameWorldService::GetDayOfWeek() { return GetGameDay() % 7; }

uint32_t GameWorldService::GetGameWeek() { return GetGameDay() / 7; }

float GameWorldService::GetGlobalWarp() {
  if (!m_isInitialized || m_globalWarpOffset == 0) return 1.0f;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return 1.0f;

  return *(float*)(coreApp + m_globalWarpOffset);
}

void GameWorldService::SetGlobalWarp(float warp) {
  if (!m_isInitialized || m_globalWarpOffset == 0) return;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return;

  *(float*)(coreApp + m_globalWarpOffset) = warp;
}

bool GameWorldService::IsGamePaused() {
  if (!m_isInitialized || m_globalHaltOffset == 0) return false;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return false;

  // If any halt counter is > 0, the game is technically paused/halted
  return *(int32_t*)(coreApp + m_globalHaltOffset) > 0 || *(int32_t*)(coreApp + m_simulationHaltOffset) > 0;
}

void GameWorldService::SetGamePaused(bool paused) { SetEngineHalt(paused); }

void GameWorldService::SetEngineHalt(bool halted) {
  if (!m_isInitialized || m_globalHaltOffset == 0 || m_simulationHaltOffset == 0) return;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return;

  *(int32_t*)(coreApp + m_globalHaltOffset) = halted ? 1 : 0;
  *(int32_t*)(coreApp + m_simulationHaltOffset) = halted ? 1 : 0;
  *(int32_t*)(coreApp + m_trafficHaltOffset) = halted ? 1 : 0;

  m_pluginHalted = halted;
}

double GameWorldService::GetRealDeltaTime() {
  if (!m_isInitialized || m_realDeltaTimeOffset == 0) return 0.0;

  uintptr_t coreApp = GameDataCameraService::GetInstance().GetCameraParamsObjectPtr();
  if (!coreApp) return 0.0;

  // Values is stored in microseconds, convert to seconds
  uint64_t microSecs = *(uint64_t*)(coreApp + m_realDeltaTimeOffset);
  return (double)microSecs * 1e-06;
}

void GameWorldService::SetSkyboxAutoUpdate(bool enabled) {
  if (!m_isInitialized) return;

  uintptr_t basePtr = ResolveEnvironmentBase();
  if (!basePtr) return;

  uintptr_t envObject = *(uintptr_t*)(basePtr + ManagerCoreService::GetInstance().GetEnvObjectOffset());
  if (!envObject) return;

  *(int32_t*)(envObject + m_skyboxAutoUpdateOffset) = enabled ? 0 : 1;
}

// --- City Data Methods ---

bool GameWorldService::RefreshCityCache() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldService");

  uintptr_t gameplayManager = ResolveEnvironmentBase();
  if (!gameplayManager) {
    return false;
  }

  uintptr_t dataPtr = *(uintptr_t*)(gameplayManager + m_kdopArrayOffset);
  uintptr_t count = *(uintptr_t*)(gameplayManager + m_kdopCountOffset);

  // Fast path: same underlying array, cache is still valid.
  if (dataPtr == m_cityCacheDataPtr && count == m_cityCacheCount && !m_cityCache.empty()) {
    return true;
  }

  m_cityCache.clear();
  m_cityCacheDataPtr = dataPtr;
  m_cityCacheCount = count;

  if (count == 0) {
    return false;
  }

  // The kdop slot array is one contiguous block, so validate the block once
  // instead of calling VirtualQuery per slot (that was the ~10s bottleneck).
  uintptr_t lastSlot = dataPtr + (count - 1) * sizeof(uintptr_t);
  if (!Utils::PatternFinder::IsValidAddress(dataPtr) || !Utils::PatternFinder::IsValidAddress(lastSlot)) {
    return false;
  }

  uint32_t citiesFound = 0;
  for (uintptr_t i = 0; i < count; ++i) {
    uintptr_t item = *(uintptr_t*)(dataPtr + i * sizeof(uintptr_t));
    // Trust item pointers the way con_cmd_goto does (dereferences them
    // unconditionally); cheap sanity filter: non-null, above null pages,
    // below the kernel range.
    if (item == 0 || item < 0x10000 || (item >> 56) != 0) continue;

    // Type check: city kdop items carry the resolved city item type.
    uint8_t itemType = *(uint8_t*)(item + m_cityItemTypeOffset);
    if (itemType != m_cityItemType) continue;

    // Flag check: bit0 clear marks a real city (same filter as con_cmd_goto).
    uint8_t itemFlags = *(uint8_t*)(item + m_cityFlagsOffset);
    if ((itemFlags & 1) != 0) continue;

    // Record: must be non-null and even (owner_ptr tag).
    uintptr_t record = *(uintptr_t*)(item + m_cityRecordOffset);
    if (record == 0 || (record & 1) != 0) continue;
    if (!Utils::PatternFinder::IsValidAddress(record)) continue;

    // Name: prism::string object at (record + city_name offset); buffer pointer
    // stored at (object + 0x08).
    uintptr_t nameObj = record + m_cityNameOffset;
    if (!Utils::PatternFinder::IsValidAddress(nameObj)) continue;

    uintptr_t nameBuf = *(uintptr_t*)(nameObj + GetCityStringBufOffset());
    if (!Utils::PatternFinder::IsValidAddress(nameBuf)) continue;

    char nameBuffer[128];
    size_t nameLen = 0;
    while (nameLen < sizeof(nameBuffer) - 1) {
      uintptr_t chAddr = nameBuf + nameLen;
      if (!Utils::PatternFinder::IsValidAddress(chAddr)) break;
      char c = *(char*)chAddr;
      if (c == '\0') break;
      nameBuffer[nameLen++] = c;
    }
    nameBuffer[nameLen] = '\0';
    if (nameLen == 0) continue;

    // Coordinates: GetPoint(item, 0) via vtable slot 0x70 -> int32[3] {X, Y, Z} in 1/256.
    CityEntry entry;
    entry.name = nameBuffer;
    entry.item = item;
    entry.record = record;
    entry.uid = ReadRecordU32(record, m_cityUidOffset);

    int32_t rawX = 0, rawY = 0, rawZ = 0;
    bool pointOk = Utils::InvokeSafe([&]() {
      uintptr_t vtable = *(uintptr_t*)item;
      if (!vtable) return;
      KdopGetPointFn_t pfnGetPoint = (KdopGetPointFn_t)(*(uintptr_t*)(vtable + GetCityVtableGetPointSlot()));
      if (!pfnGetPoint) return;
      uintptr_t pointPtr = pfnGetPoint(item, 0);
      if (!pointPtr) return;
      rawX = *(int32_t*)(pointPtr + 0);
      rawY = *(int32_t*)(pointPtr + 4);
      rawZ = *(int32_t*)(pointPtr + 8);
    });
    if (pointOk) {
      entry.x = (float)(rawX * GetCityPointScale());
      entry.y = (float)(rawY * GetCityPointScale());
      entry.z = (float)(rawZ * GetCityPointScale());
    }

    m_cityCache.push_back(std::move(entry));
    ++citiesFound;
  }

  logger->Debug("RefreshCityCache: scanned {} items, resolved {} cities.", count, citiesFound);
  return !m_cityCache.empty();
}

uint32_t GameWorldService::GetCityCount() {
  if (!RefreshCityCache()) return 0;
  return (uint32_t)m_cityCache.size();
}

int GameWorldService::GetCityName(uint32_t index, char* outBuffer, int bufferSize) {
  if (!outBuffer || bufferSize <= 0) return -1;
  outBuffer[0] = '\0';

  if (!RefreshCityCache()) return -1;
  if ((uintptr_t)index >= m_cityCache.size()) return -1;

  const std::string& name = m_cityCache[index].name;
  int fullLength = (int)name.size();

  int copyLength = fullLength;
  if (copyLength > bufferSize - 1) copyLength = bufferSize - 1;
  memcpy(outBuffer, name.c_str(), (size_t)copyLength);
  outBuffer[copyLength] = '\0';

  return fullLength;
}

uint32_t GameWorldService::GetCityUid(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;
  return m_cityCache[index].uid;
}

bool GameWorldService::GetCityPosition(uint32_t uid, float* outX, float* outY, float* outZ) {
  if (!outX || !outY || !outZ) return false;
  if (!RefreshCityCache()) return false;

  for (const auto& city : m_cityCache) {
    if (city.uid != uid) continue;

    // Average every geometry point of the kdop item (polygon outline), the
    // same way con_cmd_goto resolves the city center via the vtable.
    uintptr_t item = city.item;
    if (!item) return false;

    double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
    uint32_t pointCount = 0;
    bool resolved = false;

    bool ok = Utils::InvokeSafe([&]() {
      uintptr_t vtable = *(uintptr_t*)item;
      if (!vtable) return;

      KdopPointCountFn_t pfnCount = (KdopPointCountFn_t)(*(uintptr_t*)(vtable + GetCityVtablePointCountSlot()));
      KdopGetPointFn_t pfnGetPoint = (KdopGetPointFn_t)(*(uintptr_t*)(vtable + GetCityVtableGetPointSlot()));
      if (!pfnCount || !pfnGetPoint) return;

      uint32_t count = pfnCount(item);
      if (count == 0) return;

      for (uint32_t i = 0; i < count; ++i) {
        uintptr_t pointPtr = pfnGetPoint(item, i);
        if (!pointPtr) continue;
        sumX += *(int32_t*)(pointPtr + 0) * GetCityPointScale();
        sumY += *(int32_t*)(pointPtr + 4) * GetCityPointScale();
        sumZ += *(int32_t*)(pointPtr + 8) * GetCityPointScale();
        ++pointCount;
      }
      resolved = pointCount > 0;
    });

    if (!ok || !resolved) return false;

    *outX = (float)(sumX / (double)pointCount);
    *outY = (float)(sumY / (double)pointCount);
    *outZ = (float)(sumZ / (double)pointCount);
    return true;
  }

  return false;
}

bool GameWorldService::SetCityPosition(uint32_t uid, float x, float y, float z) {
  if (!RefreshCityCache()) return false;

  for (const auto& city : m_cityCache) {
    if (city.uid != uid) continue;
    uintptr_t item = city.item;
    if (!item) return false;

    int32_t rawX = (int32_t)(x / GetCityPointScale());
    int32_t rawY = (int32_t)(y / GetCityPointScale());
    int32_t rawZ = (int32_t)(z / GetCityPointScale());

    bool ok = Utils::InvokeSafe([&]() {
      uintptr_t vtable = *(uintptr_t*)item;
      if (!vtable) return;
      KdopGetPointFn_t pfnGetPoint = (KdopGetPointFn_t)(*(uintptr_t*)(vtable + GetCityVtableGetPointSlot()));
      if (!pfnGetPoint) return;
      uintptr_t pointPtr = pfnGetPoint(item, 0);
      if (!pointPtr) return;
      *(int32_t*)(pointPtr + 0) = rawX;
      *(int32_t*)(pointPtr + 4) = rawY;
      *(int32_t*)(pointPtr + 8) = rawZ;
    });

    return ok;
  }

  return false;
}

uint32_t GameWorldService::GetCityPointCount(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;

  uintptr_t item = m_cityCache[index].item;
  if (!item) return 0;

  uint32_t result = 0;
  bool ok = Utils::InvokeSafe([&]() {
    uintptr_t vtable = *(uintptr_t*)item;
    if (!vtable) return;
    KdopPointCountFn_t pfnCount = (KdopPointCountFn_t)(*(uintptr_t*)(vtable + GetCityVtablePointCountSlot()));
    if (pfnCount) result = pfnCount(item);
  });

  return ok ? result : 0;
}

bool GameWorldService::GetCityPoint(uint32_t index, uint32_t pointIndex, float* outX, float* outY, float* outZ) {
  if (!outX || !outY || !outZ) return false;
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;

  uintptr_t item = m_cityCache[index].item;
  if (!item) return false;

  int32_t rawX = 0, rawY = 0, rawZ = 0;
  bool gotPoint = false;
  bool ok = Utils::InvokeSafe([&]() {
    uintptr_t vtable = *(uintptr_t*)item;
    if (!vtable) return;
    KdopGetPointFn_t pfnGetPoint = (KdopGetPointFn_t)(*(uintptr_t*)(vtable + GetCityVtableGetPointSlot()));
    if (!pfnGetPoint) return;
    uintptr_t pointPtr = pfnGetPoint(item, pointIndex);
    if (!pointPtr) return;
    rawX = *(int32_t*)(pointPtr + 0);
    rawY = *(int32_t*)(pointPtr + 4);
    rawZ = *(int32_t*)(pointPtr + 8);
    gotPoint = true;
  });

  if (!ok || !gotPoint) return false;

  *outX = (float)(rawX * GetCityPointScale());
  *outY = (float)(rawY * GetCityPointScale());
  *outZ = (float)(rawZ * GetCityPointScale());
  return true;
}

float GameWorldService::GetCityItemScale(uint32_t index) {
  if (!RefreshCityCache()) return 0.0f;
  if ((uintptr_t)index >= m_cityCache.size()) return 0.0f;
  uintptr_t item = m_cityCache[index].item;
  if (!item) return 0.0f;
  if (!Utils::PatternFinder::IsValidAddress(item + m_cityScaleOffset)) return 0.0f;
  return *(float*)(item + m_cityScaleOffset);
}

float GameWorldService::GetCityItemRadius(uint32_t index) {
  if (!RefreshCityCache()) return 0.0f;
  if ((uintptr_t)index >= m_cityCache.size()) return 0.0f;
  uintptr_t item = m_cityCache[index].item;
  if (!item) return 0.0f;
  if (!Utils::PatternFinder::IsValidAddress(item + m_cityRadiusOffset)) return 0.0f;
  return *(float*)(item + m_cityRadiusOffset);
}

bool GameWorldService::SetCityItemScale(uint32_t index, float val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  uintptr_t item = m_cityCache[index].item;
  if (!item) return false;
  if (!Utils::PatternFinder::IsValidAddress(item + m_cityScaleOffset)) return false;
  *(float*)(item + m_cityScaleOffset) = val;
  return true;
}

bool GameWorldService::SetCityItemRadius(uint32_t index, float val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  uintptr_t item = m_cityCache[index].item;
  if (!item) return false;
  if (!Utils::PatternFinder::IsValidAddress(item + m_cityRadiusOffset)) return false;
  *(float*)(item + m_cityRadiusOffset) = val;
  return true;
}

int GameWorldService::GetCityNameLocalized(uint32_t index, char* outBuffer, int bufferSize) {
  if (!RefreshCityCache()) return -1;
  if ((uintptr_t)index >= m_cityCache.size()) return -1;
  return CopyRecordString(m_cityCache[index].record, m_cityNameLocalizedOffset, GetCityStringBufOffset(), outBuffer, bufferSize);
}

int GameWorldService::GetCityShortName(uint32_t index, char* outBuffer, int bufferSize) {
  if (!RefreshCityCache()) return -1;
  if ((uintptr_t)index >= m_cityCache.size()) return -1;
  return CopyRecordString(m_cityCache[index].record, m_shortCityNameOffset, GetCityStringBufOffset(), outBuffer, bufferSize);
}

int GameWorldService::GetCityShortNameLocalized(uint32_t index, char* outBuffer, int bufferSize) {
  if (!RefreshCityCache()) return -1;
  if ((uintptr_t)index >= m_cityCache.size()) return -1;
  return CopyRecordString(m_cityCache[index].record, m_shortCityNameLocalizedOffset, GetCityStringBufOffset(), outBuffer, bufferSize);
}

uint32_t GameWorldService::GetCityGroup(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;
  return ReadRecordU32(m_cityCache[index].record, m_cityGroupOffset);
}

float GameWorldService::GetCityPinScaleFactor(uint32_t index) {
  if (!RefreshCityCache()) return 0.0f;
  if ((uintptr_t)index >= m_cityCache.size()) return 0.0f;
  return ReadRecordFloatArrayFirst(m_cityCache[index].record, m_cityPinScaleFactorOffset);
}

bool GameWorldService::GetCityMapXOffsets(uint32_t index, float* out, size_t maxCount) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return ReadRecordFloatArray(m_cityCache[index].record, m_mapXOffsetsOffset, out, maxCount) > 0;
}

bool GameWorldService::GetCityMapYOffsets(uint32_t index, float* out, size_t maxCount) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return ReadRecordFloatArray(m_cityCache[index].record, m_mapYOffsetsOffset, out, maxCount) > 0;
}

float GameWorldService::GetCityPriceCoef(uint32_t index) {
  if (!RefreshCityCache()) return 0.0f;
  if ((uintptr_t)index >= m_cityCache.size()) return 0.0f;
  return ReadRecordFloat(m_cityCache[index].record, m_priceCoefOffset);
}

uint32_t GameWorldService::GetCityCountry(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;
  return ReadRecordU32(m_cityCache[index].record, m_countryOffset);
}

uint32_t GameWorldService::GetCityPopulation(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;
  return ReadRecordU32(m_cityCache[index].record, m_populationOffset);
}

bool GameWorldService::GetCityKeyCity(uint32_t index) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return ReadRecordBoolByte(m_cityCache[index].record, m_keyCityOffset);
}

uint32_t GameWorldService::GetCityTimeZone(uint32_t index) {
  if (!RefreshCityCache()) return 0;
  if ((uintptr_t)index >= m_cityCache.size()) return 0;
  return ReadRecordU32(m_cityCache[index].record, m_timeZoneOffset);
}

bool GameWorldService::SetCityGroup(uint32_t index, uint32_t val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordU32(m_cityCache[index].record, m_cityGroupOffset, val);
}

bool GameWorldService::SetCityPinScaleFactor(uint32_t index, float val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordFloatArrayFirst(m_cityCache[index].record, m_cityPinScaleFactorOffset, val);
}

bool GameWorldService::SetCityMapXOffsets(uint32_t index, const float* values, size_t count) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordFloatArray(m_cityCache[index].record, m_mapXOffsetsOffset, values, count);
}

bool GameWorldService::SetCityMapYOffsets(uint32_t index, const float* values, size_t count) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordFloatArray(m_cityCache[index].record, m_mapYOffsetsOffset, values, count);
}

bool GameWorldService::SetCityPriceCoef(uint32_t index, float val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordFloat(m_cityCache[index].record, m_priceCoefOffset, val);
}

bool GameWorldService::SetCityCountry(uint32_t index, uint32_t val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordU32(m_cityCache[index].record, m_countryOffset, val);
}

bool GameWorldService::SetCityPopulation(uint32_t index, uint32_t val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordU32(m_cityCache[index].record, m_populationOffset, val);
}

bool GameWorldService::SetCityKeyCity(uint32_t index, bool val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordBoolByte(m_cityCache[index].record, m_keyCityOffset, val);
}

bool GameWorldService::SetCityTimeZone(uint32_t index, uint32_t val) {
  if (!RefreshCityCache()) return false;
  if ((uintptr_t)index >= m_cityCache.size()) return false;
  return WriteRecordU32(m_cityCache[index].record, m_timeZoneOffset, val);
}

}  // namespace Data::GameData
SPF_NS_END
