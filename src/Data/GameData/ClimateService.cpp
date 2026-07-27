#include "SPF/Data/GameData/ClimateService.hpp"

#include "SPF/Namespace.hpp"
#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"
#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/Vec3.hpp"
//#include "SPF/Utils/Windows.hpp"

#include <cstdint>
#include <cstring>
#include <libloaderapi.h>
#include <memory>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData {

// ============================================================================
// Singleton / Lifecycle
// ============================================================================

ClimateService::ClimateService() = default;

ClimateService& ClimateService::GetInstance() {
  static ClimateService instance;
  return instance;
}

// ============================================================================
// Initialization / Shutdown / Finders
// ============================================================================

void ClimateService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
  logger->Info("Attempting to initialize ClimateService...");

  RegisterFinders();

  m_isInitialized = false;
  logger->Info("ClimateService initialization finished. Waiting for critical offsets.");
}

void ClimateService::Shutdown() {
  if (m_isInitialized) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
    logger->Info("ClimateService has been shut down.");
    m_isInitialized = false;

    m_environmentBasePtr = 0;
    m_environmentAdjustment = 0;
    m_envObjectOffset = 0;
    m_updateFnAddr = 0;

    m_weatherModeOffset = 0;
    m_climatePtrOffset = 0;
    m_climateUnitIdOffset = 0;
    m_climateArrayOffset = 0;
    m_climateCountOffset = 0;
    m_weatherBlendFnAddr = 0;
    m_transitionDurationAddr = 0;
  }
}

void ClimateService::RegisterFinders() { m_dataFinders.push_back(std::make_unique<Finders::ClimateDataFinder>()); }

bool ClimateService::IsReady() { return m_isInitialized && AreAllFindersReady(); }

bool ClimateService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool ClimateService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

bool ClimateService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");

  for (auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (!finder->TryFindOffsets(*this)) {
        return false;
      }
    }
  }

  m_isInitialized = true;
  logger->Info("ClimateService: All offsets found. Service is READY.");
  return true;
}

// ============================================================================
// Climate API — Name / ID Resolution
// ============================================================================

std::string ClimateService::GetCurrentClimateName() {
  if (!m_isInitialized) return "unknown";
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return "unknown";
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return "unknown";

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return "unknown";

  // Climate name UnitID is at m_climateUnitIdOffset
  uint32_t unitId = *(uint32_t*)(climate + m_climateUnitIdOffset);
  return Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
}

std::vector<ClimateService::ClimateInfo> ClimateService::GetAvailableClimates() {
  std::vector<ClimateInfo> climates;
  if (!m_isInitialized) return climates;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return climates;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env || !m_climateArrayOffset || !m_climateCountOffset) return climates;

  uintptr_t arrayPtr = *(uintptr_t*)(env + m_climateArrayOffset);
  uint64_t count = *(uint64_t*)(env + m_climateCountOffset);

  if (arrayPtr && count > 0 && count < 512) {
    uintptr_t* entries = (uintptr_t*)arrayPtr;
    for (uint64_t i = 0; i < count; ++i) {
      uintptr_t climateObj = entries[i];
      if (climateObj) {
        uint32_t unitId = *(uint32_t*)(climateObj + m_climateUnitIdOffset);
        std::string name = Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
        uint64_t shortToken = Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitToken(unitId);

        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
        logger->Info("ClimateService: name {} , shortToken {}.", name, shortToken);

        if (!name.empty()) {
          climates.push_back({name, shortToken});
        }
      }
    }
  }
  return climates;
}

void ClimateService::SetClimate(uint64_t token, bool instant) {
  if (!m_isInitialized || !m_setClimateFnAddr) return;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return;

  typedef void(__fastcall * SetClimate_t)(uintptr_t env, uint64_t* pToken, uint8_t instant);
  SetClimate_t fn = (SetClimate_t)m_setClimateFnAddr;

  fn(env, &token, (uint8_t)instant);
}

// ============================================================================
// Sun Profile API
// ============================================================================

int32_t ClimateService::GetActiveSunProfileIndex() {
  if (!m_isInitialized) return -1;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return -1;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return -1;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  return (raw0 <= raw1) ? raw0 : raw1;
}

int32_t ClimateService::GetNextSunProfileIndex() {
  if (!m_isInitialized) return -1;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return -1;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return -1;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  return (raw0 <= raw1) ? raw1 : raw0;
}

int32_t ClimateService::GetSunProfileCount() {
  uintptr_t container = GetClimateContainer();
  if (!container) return 0;
  return (int32_t)*(uint64_t*)(container + m_containerCountOffset);
}

std::string ClimateService::GetSunProfileName(int32_t index) {
  if (index < 0) return "unknown";
  uintptr_t container = GetClimateContainer();
  if (!container) return "unknown";

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);

  if (profilesArray && (uint64_t)index < count) {
    uintptr_t profile = *(uintptr_t*)(profilesArray + (index * 8));
    if (profile) {
      uint32_t unitId = *(uint32_t*)(profile + m_climateUnitIdOffset);
      return Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
    }
  }
  return "none";
}

float ClimateService::GetTransitionProgress() {
  uintptr_t container = GetClimateContainer();
  if (!container) return 0.0f;

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);
  if (!profilesArray) return 0.0f;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0.0f;

  int32_t raw0 = (int32_t)*(uint32_t*)(env + m_activeProfileIndexOffset);
  int32_t raw1 = (int32_t)*(uint32_t*)(env + m_nextProfileIndexOffset);
  if (raw0 < 0 || raw1 < 0 || (uint64_t)raw0 >= count || (uint64_t)raw1 >= count) return 0.0f;

  uintptr_t p0 = *(uintptr_t*)(profilesArray + (raw0 * 8));
  uintptr_t p1 = *(uintptr_t*)(profilesArray + (raw1 * 8));
  if (!p0 || !p1) return 0.0f;

  float elev0 = *(float*)(p0 + 0x14); //low_elevation
  float elev1 = *(float*)(p1 + 0x18); //high_elevation
  float sunAngle = *(float*)(env + m_sunAngleOffset);

  int32_t aIdx = (raw0 <= raw1) ? raw0 : raw1;
  int32_t bIdx = (raw0 <= raw1) ? raw1 : raw0;
  float aElev = (raw0 <= raw1) ? elev0 : elev1;
  float bElev = (raw0 <= raw1) ? elev1 : elev0;

  float result;
  if (aElev <= bElev) {
    result = (sunAngle - aElev) / (bElev - aElev);
  } else {
    result = (aElev - sunAngle) / (aElev - bElev);
  }

  if (result < 0.0f) return 0.0f;
  if (result > 1.0f) return 1.0f;
  return result;
}

float ClimateService::GetSunProfileElevation(int32_t index) {
  if (index < 0) return 0.0f;
  uintptr_t container = GetClimateContainer();
  if (!container) return 0.0f;

  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);
  if (!profilesArray || (uint64_t)index >= count) return 0.0f;

  uintptr_t profile = *(uintptr_t*)(profilesArray + (index * 8));
  if (!profile) return 0.0f;

  return *(float*)(profile + 0x18);
}

float ClimateService::GetSunAngle() {
  if (!m_isInitialized) return 0.0f;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0.0f;
  return *(float*)(env + m_sunAngleOffset);
}

float ClimateService::GetWeatherBlendProgress() {
  if (!m_isInitialized || !m_weatherBlendFnAddr) return 0.0f;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0.0f;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0.0f;
  typedef float(__fastcall* BlendFn)(uintptr_t env);
  return ((BlendFn)m_weatherBlendFnAddr)(env);
}

void ClimateService::SetTransitionDuration(int32_t minutes) {
  if (!m_transitionDurationAddr) return;
  *(uint32_t*)m_transitionDurationAddr = (uint32_t)minutes * 60000 * 1000;
}

// ============================================================================
// Weather & Environment — Scalar API
// ============================================================================

int32_t ClimateService::GetWeatherMode() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_weatherModeOffset) return 0;

  return *(int32_t*)(env + m_weatherModeOffset);
}

int32_t ClimateService::GetNextWeatherMode() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  basePtr += m_environmentAdjustment;
  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_nextWeatherModeOffset) return 0;
  return *(int32_t*)(env + m_nextWeatherModeOffset);
}

void ClimateService::SetNextWeatherMode(int32_t mode) {
  if (!m_isInitialized) return;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;
  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_nextWeatherModeOffset) return;
  *(int32_t*)(env + m_nextWeatherModeOffset) = mode;
  if (m_updateFnAddr) {
    typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
}

void ClimateService::SetWeatherMode(int32_t mode, bool instant) { //del ?
  if (!m_isInitialized || !m_setWeatherModeFnAddr) return;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return;
  typedef void(__fastcall* SetWeatherModeFn)(uintptr_t env, int32_t mode, char instant);
  ((SetWeatherModeFn)m_setWeatherModeFnAddr)(env, mode, (char)instant);
}

float ClimateService::GetRainIntensity() {
  if (!m_isInitialized) return 0.0f;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0.0f;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env) return 0.0f;

  return *(float*)(env + 0x4010);
}

void ClimateService::SetRainIntensity(float intensity) {
  if (!m_isInitialized) return;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_climatePtrOffset) return;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return;

  int32_t mode = GetWeatherMode();
  uint32_t variationIdx = static_cast<uint32_t>(GetActiveSunProfileIndex());

  uintptr_t container = climate + (mode == 0 ? 0xd0 : 0x120);
  uintptr_t profiles_ptr = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t profileCount = *(uint64_t*)(container + m_containerCountOffset);

  if (!profiles_ptr || profileCount == 0) return;

  for (uint64_t i = 0; i < profileCount; ++i) {
    uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
    if (!profile) continue;

    uintptr_t rainArrayPtr = *(uintptr_t*)(profile + 0x468);
    uint64_t rainArrayCount = *(uint64_t*)(profile + 0x470);

    if (rainArrayPtr && (uint64_t)variationIdx < rainArrayCount) {
      *(float*)(rainArrayPtr + (variationIdx * 4)) = intensity;
    }
  }

  *(float*)(env + 0x4010) = intensity;

  // Trigger update (same as moving skybox index)
  if (m_updateFnAddr) {
    typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
}

// ============================================================================
// Profile-Based Scalar Parameters
// ============================================================================

float ClimateService::GetTemperature(int profileSlot, uint32_t variationIdx) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return 0.0f;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + 0x308);
  uint64_t count = *(uint64_t*)(profile + 0x310);
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
  return 0.0f;
}

void ClimateService::SetTemperature(int profileSlot, uint32_t variationIdx, float val) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + 0x308);
  uint64_t count = *(uint64_t*)(profile + 0x310);
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = val;
}

// ============================================================================
// Internal Helpers
// ============================================================================

void ClimateService::EnsureInitialKick() {
  if (!m_isInitialized || !m_updateFnAddr) return;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return;

  typedef void(__fastcall* UpdateEnv_t)(uintptr_t);
  ((UpdateEnv_t)m_updateFnAddr)(env);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
  logger->Info("ClimateService: Automatic environment kick performed.");
}

uintptr_t ClimateService::GetClimateContainer() {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate || !m_containerSelectorOffset || !m_containerNiceOffset || !m_containerBadOffset) return 0;

  int32_t sel = *(int32_t*)(env + m_containerSelectorOffset);
  return climate + (sel ? m_containerBadOffset : m_containerNiceOffset);
}

uintptr_t ClimateService::GetActiveProfilePtr(int profileSlot) {
  uintptr_t container = GetClimateContainer();
  if (!container) return 0;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0;

  uint32_t idx = *(uint32_t*)(env + (profileSlot == 0 ? m_activeProfileIndexOffset : m_nextProfileIndexOffset));
  uintptr_t profilesArray = *(uintptr_t*)(container + m_profilesArrayOffset);
  uint64_t count = *(uint64_t*)(container + m_containerCountOffset);

  if (profilesArray && (uint64_t)idx < count) {
    return *(uintptr_t*)(profilesArray + (idx * 8));
  }
  return 0;
}

// ============================================================================
// Profile Parameter Methods — Vec3 Colors (Internal Helpers)
// ============================================================================

static Utils::Vector3 GetProfileVec3Internal(uintptr_t profile, uint32_t offset, uint32_t varIdx) {
  Utils::Vector3 result = {0.0f, 0.0f, 0.0f};
  if (!profile) return result;

  uintptr_t arrayPtr = *(uintptr_t*)(profile + offset);
  uint64_t count = *(uint64_t*)(profile + offset + 8);

  if (arrayPtr > 0x1000 && (uint64_t)varIdx < count) {
    float* data = (float*)(arrayPtr + varIdx * 12);
    result.x = data[0];
    result.y = data[1];
    result.z = data[2];
  }
  return result;
}

static void SetProfileVec3Internal(uintptr_t profile, uint32_t offset, uint32_t varIdx, const Utils::Vector3& value, uintptr_t env, uintptr_t updateFn) {
  if (!profile) return;

  uintptr_t arrayPtr = *(uintptr_t*)(profile + offset);
  uint64_t count = *(uint64_t*)(profile + offset + 8);

  if (arrayPtr > 0x1000 && (uint64_t)varIdx < count) {
    float* data = (float*)(arrayPtr + varIdx * 12);
    data[0] = value.x;
    data[1] = value.y;
    data[2] = value.z;

    if (env && updateFn) {
      typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
      ((UpdateEnv_t)updateFn)(env);
    }
  }
}

// ============================================================================
// Profile Parameter Methods — Float Effects
// ============================================================================

float ClimateService::GetWeight(int profileSlot, uint32_t variationIdx) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return 1.0f;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + 0x888);
  uint64_t count = *(uint64_t*)(profile + 0x890);
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
  return 1.0f;
}

void ClimateService::SetWeight(int profileSlot, uint32_t variationIdx, float value) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + 0x888);
  uint64_t count = *(uint64_t*)(profile + 0x890);
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

}  // namespace Data::GameData
SPF_NS_END
