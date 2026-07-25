#include "SPF/Data/GameData/ClimateService.hpp"

#include "SPF/Namespace.hpp"
#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"
#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/EnvironmentManager.hpp"
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
// Version Utilities
// ============================================================================

bool ClimateService::IsVersion1_60() const {
  if (m_versionCache == -1) {
    std::string ver = System::EnvironmentManager::GetInstance().GetGameInfo().version;
    if (ver.size() >= 4) {
      if (ver.substr(0, 4) >= "1.60") {
        m_versionCache = 1;
      } else {
        m_versionCache = 0;
      }
    } else {
      m_versionCache = 0;
    }
  }
  return m_versionCache == 1;
}

intptr_t GameData::ClimateService::GetVerOffset(intptr_t offset159, intptr_t offset160) const { return IsVersion1_60() ? offset160 : offset159; }

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
    m_weatherTargetOffset = 0;
    m_weatherTransitionOffset = 0;
    m_climatePtrOffset = 0;
    m_climateUnitIdOffset = 0;
    m_climateArrayOffset = 0;
    m_climateCountOffset = 0;
    m_weatherTransStartTimeOffset = 0;
    m_weatherBlendingFactorOffset = 0;
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

  uintptr_t arrayPtr = *(uintptr_t*)(env + GetVerOffset(m_climateArrayOffset, m_climateArrayOffset));
  uint64_t count = *(uint64_t*)(env + GetVerOffset(m_climateCountOffset, m_climateCountOffset));

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

std::string ClimateService::GetActiveProfileName(int profileSlot) {
  if (!m_isInitialized) return "unknown";
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return "unknown";
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return "unknown";

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return "unknown";

  int32_t mode = *(int32_t*)(env + m_weatherModeOffset);
  uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50

  uint32_t idx = GetActiveProfileIndex(profileSlot);
  uintptr_t profilesArray = *(uintptr_t*)(container + 0x08);
  uint64_t count = *(uint64_t*)(container + 0x10);

  if (profilesArray && idx < count) {
    uintptr_t profile = *(uintptr_t*)(profilesArray + (idx * 8));
    if (profile) {
      uint32_t unitId = *(uint32_t*)(profile + 0x0c);
      return Hooks::GameTools::ScsNameResolver::GetInstance().ResolveUnitName(unitId);
    }
  }
  return "none";
}

uint32_t ClimateService::GetActiveProfileIndex(int profileSlot) {
  if (!m_isInitialized) return 0xFFFFFFFF;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0xFFFFFFFF;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0xFFFFFFFF;

  return *(uint32_t*)(env + (profileSlot == 0 ? GetVerOffset(0x4540, 0x4560) : GetVerOffset(0x4544, 0x4564)));
}

// ============================================================================
// Climate API — Control
// ============================================================================

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

void ClimateService::SetSkyboxIndex(int32_t weatherMode, uint32_t index) {
  if (!m_isInitialized) return;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_climatePtrOffset) return;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return;

  uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50
  uintptr_t indexArray = *(uintptr_t*)(container + 0x28);
  uint64_t slotCount = *(uint64_t*)(container + 0x30);

  if (!indexArray) return;

  // Set the selected variation index for all time slots
  for (uint64_t i = 0; i < slotCount; ++i) {
    *(uint64_t*)(indexArray + (i * 8)) = (uint64_t)index;
  }

  // Trigger environment update to apply texture changes
  if (m_updateFnAddr) {
    typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
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

void ClimateService::SetWeatherMode(int32_t mode, bool instant) {
  if (!m_isInitialized) return;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_weatherTargetOffset) return;

  if (instant) {
    // Forced transition: set both current and target
    *(int32_t*)(env + m_weatherModeOffset) = mode;
    *(int32_t*)(env + m_weatherTargetOffset) = mode;
    if (m_weatherTransitionOffset) {
      *(int32_t*)(env + m_weatherTransitionOffset) = 0;  // State: Stable (0)
    }
    if (m_weatherBlendingFactorOffset) {
      *(float*)(env + m_weatherBlendingFactorOffset) = (mode == 0) ? 0.0f : 1.0f;
    }
  } else {
    // Interpolated transition: set target only and start blending
    *(int32_t*)(env + m_weatherTargetOffset) = mode;
    if (m_weatherTransitionOffset) {
      *(int32_t*)(env + m_weatherTransitionOffset) = 1;  // State: Interpolating (1)

      // Logic from Ghidra FUN_140468ed0 (140468fd1 - 140469019)
      if (m_weatherTransStartTimeOffset) {
        uint32_t totalMinutes = *(uint32_t*)(env + m_timeOffset);  // 0x3E58
        float secondsFrac = *(float*)(env + m_timeOffset + 4);     // 0x3E5C

        // minutes % 1440 (0x5a0)
        uint32_t minutesInDay = totalMinutes % 1440;
        uint32_t startTimeMs = (uint32_t)(secondsFrac * 60000.0f) + (minutesInDay * 60000);
        *(uint32_t*)(env + m_weatherTransStartTimeOffset) = startTimeMs;
      }
    }
  }

  // Trigger environment update to apply weather changes immediately
  if (m_updateFnAddr) {
    typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
}

float ClimateService::GetRainIntensity() {
  if (!m_isInitialized) return 0.0f;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0.0f;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env) return 0.0f;

  return *(float*)(env + GetVerOffset(0x3ff0, 0x4010));  // ATS Hardcoded
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
  uint32_t variationIdx = GetSkyboxIndex(mode);

  // 0xc0 = Nice weather container, 0x100 = Bad weather container (1.60 -> 0x120)
  uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50
  uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
  uint64_t profileCount = *(uint64_t*)(container + 0x10);  // Usually 24 (hourly)

  if (!profiles_ptr || profileCount == 0) return;

  // Iterate through all hourly sun profiles
  for (uint64_t i = 0; i < profileCount; ++i) {
    uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
    if (!profile) continue;

    // Rain Intensity Array Pointer is at profile + 0x448 (1.59) / 0x468 (1.60) Rain Intensity Array Count is at profile + 0x450
    uintptr_t rainArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x448, 0x468));
    uint64_t rainArrayCount = *(uint64_t*)(profile + GetVerOffset(0x450, 0x470));

    if (rainArrayPtr && (uint64_t)variationIdx < rainArrayCount) {
      // Write our value directly into the profile source
      *(float*)(rainArrayPtr + (variationIdx * 4)) = intensity;
    }
  }

  // Also write to the immediate runtime value to see instant feedback in the GET slider
  *(float*)(env + GetVerOffset(0x3ff0, 0x4010)) = intensity;

  // Trigger update (same as moving skybox index)
  if (m_updateFnAddr) {
    typedef void(__fastcall * UpdateEnv_t)(uintptr_t rcx);
    ((UpdateEnv_t)m_updateFnAddr)(env);
  }
}

// ============================================================================
// Skybox Queries
// ============================================================================

uint64_t ClimateService::GetSkyboxCount(int32_t weatherMode) {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_climatePtrOffset) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return 0;

  // Nice/Bad weather containers: 1.59 (0xc0/0x100), 1.60 (0xd0/0x120)
  uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50
  uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
  if (!profiles_ptr) return 0;

  uintptr_t first_profile = *(uintptr_t*)profiles_ptr;
  if (!first_profile) return 0;

  // Variation count is at SunProfile + 0x310 (1.59) / 0x330 (1.60)
  return *(uint64_t*)(first_profile + GetVerOffset(0x310, 0x330));
}

uint32_t ClimateService::GetSkyboxIndex(int32_t weatherMode, uint32_t slot) {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  basePtr += m_environmentAdjustment;

  uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
  if (!env || !m_climatePtrOffset) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return 0;

  uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50
  uintptr_t indexArray = *(uintptr_t*)(container + 0x28);
  uint64_t slotCount = *(uint64_t*)(container + 0x30);

  if (!indexArray || (uint64_t)slot >= slotCount) return 0;

  return (uint32_t)*(uint64_t*)(indexArray + (slot * 8));
}

// ============================================================================
// Profile-Based Scalar Parameters
// ============================================================================

float ClimateService::GetTemperature(int profileSlot, uint32_t variationIdx) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return 0.0f;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x2E8, 0x308));
  uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x2F0, 0x310));
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
  return 0.0f;
}

void ClimateService::SetTemperature(int profileSlot, uint32_t variationIdx, float val) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x2E8, 0x308));
  uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x2F0, 0x310));
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = val;
}

// ============================================================================
// Internal Helpers
// ============================================================================

void ClimateService::EnsureInitialKick() {
  if (!m_isInitialized) return;

  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return;

  // Perform the "kick" by re-setting the current skybox index to all slots.
  int32_t mode = GetWeatherMode();
  uint32_t currentIdx = GetSkyboxIndex(mode, 0);
  SetSkyboxIndex(mode, currentIdx);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
  logger->Info("ClimateService: Automatic environment kick performed for mode {}.", mode);
}

uintptr_t ClimateService::GetActiveProfilePtr(int profileSlot) {
  if (!m_isInitialized) return 0;
  uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
  if (!basePtr) return 0;
  uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
  if (!env) return 0;

  uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
  if (!climate) return 0;

  int32_t mode = *(int32_t*)(env + m_weatherModeOffset);
  uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x120));  // Hydra: FUN_1404d5f50 @ 0x1404d5f50

  uint32_t idx = GetActiveProfileIndex(profileSlot);
  uintptr_t profilesArray = *(uintptr_t*)(container + 0x08);
  uint64_t count = *(uint64_t*)(container + 0x10);

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
  uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x868, 0x888));
  uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x870, 0x890));
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
  return 1.0f;
}

void ClimateService::SetWeight(int profileSlot, uint32_t variationIdx, float value) {
  uintptr_t profile = GetActiveProfilePtr(profileSlot);
  if (!profile) return;
  uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x868, 0x888));
  uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x870, 0x890));
  if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

}  // namespace Data::GameData
SPF_NS_END
