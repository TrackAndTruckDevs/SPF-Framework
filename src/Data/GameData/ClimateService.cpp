/**                                                                                               
 * @file ClimateService.cpp                                                                          
 * @brief Implementation of the ClimateService for managing weather and environment.
 */ 

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <algorithm>
#include <string>

SPF_NS_BEGIN
namespace Data::GameData {

static std::string DecodeSCSToken(uint64_t token) {
    if (token == 0) return "";
    static const char* alphabet = "0123456789abcdefghijklmnopqrstuvwxyz_";
    std::string res = "";
    uint64_t temp = token & 0x7FFFFFFFFFFFFFFF;
    while (temp > 0) {
        uint64_t idx = temp % 38;
        if (idx == 0) break; 
        res += alphabet[idx - 1];
        temp /= 38;
    }
    return res;
}

static std::string ResolveSCSUnitName(uint32_t unitId) {
    if (unitId == 0) return "none";

    static uintptr_t tableAddr = 0;
    if (tableAddr == 0) {
        uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
        tableAddr = base + 0x2DD59D0; 
    }

    uint32_t poolIdx = (unitId >> 20) & 0x7FF;
    uint32_t itemIdx = (unitId & 0xFFFF);

    uintptr_t* table = (uintptr_t*)tableAddr;
    if (IsBadReadPtr(&table[poolIdx], sizeof(uintptr_t))) return "err_tbl";
    
    uintptr_t poolPtr = table[poolIdx];
    if (!poolPtr || IsBadReadPtr((void*)poolPtr, 0x70)) return "err_pool";

    // itemSize is at +0x68 (13th qword)
    uint64_t itemSize64 = *(uint64_t*)(poolPtr + 0x68); 
    uint32_t itemSize = (uint32_t)(itemSize64 & 0xFF);
    if (itemSize == 0 || itemSize > 16) return "err_size"; 
    
    uintptr_t itemsBase = *(uintptr_t*)(poolPtr + 0x00);
    if (!itemsBase) return "err_base";
    
    size_t offset = (size_t)itemIdx * itemSize * 8;
    if (IsBadReadPtr((void*)(itemsBase + offset), itemSize * 8)) return "err_item";

    uint64_t* tokenPtr = (uint64_t*)(itemsBase + offset);
    std::string fullName = "";
    for (uint32_t i = 0; i < itemSize; ++i) {
        std::string part = DecodeSCSToken(tokenPtr[i]);
        if (part.empty()) break;
        if (!fullName.empty()) fullName += ".";
        fullName += part;
    }

    return fullName.empty() ? "unknown" : fullName;
}

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

intptr_t ClimateService::GetVerOffset(intptr_t offset159, intptr_t offset160) const {
    return IsVersion1_60() ? offset160 : offset159;
}

ClimateService::ClimateService() = default;

ClimateService& ClimateService::GetInstance() {
  static ClimateService instance;
  return instance;
}

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
    m_rainIntensityOffset = 0;
    m_roadWetnessOffset = 0;
    m_fogColorOffset = 0;
    m_fogDensityOffset = 0;
    m_lightningEnabledOffset = 0;
    m_lightningIntensityOffset = 0;
    m_temperatureOffset = 0;
    m_weatherTransStartTimeOffset = 0;
    m_weatherTransDurationOffset = 0;
    m_weatherBlendingFactorOffset = 0;
    m_skyboxAutoUpdateOffset = 0;
  }
}

void ClimateService::RegisterFinders() {
  m_dataFinders.push_back(std::make_unique<Finders::ClimateDataFinder>());
}

bool ClimateService::IsReady() {
  return m_isInitialized && AreAllFindersReady();
}

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

// --- Weather & Environment methods ---

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
            *(int32_t*)(env + m_weatherTransitionOffset) = 0; // State: Stable (0)
        }
        if (m_weatherBlendingFactorOffset) {
            *(float*)(env + m_weatherBlendingFactorOffset) = (mode == 0) ? 0.0f : 1.0f;
        }
    } else {
        // Interpolated transition: set target only and start blending
        *(int32_t*)(env + m_weatherTargetOffset) = mode;
        if (m_weatherTransitionOffset) {
            *(int32_t*)(env + m_weatherTransitionOffset) = 1; // State: Interpolating (1)
            
            // Logic from Ghidra FUN_140468ed0 (140468fd1 - 140469019)
            if (m_weatherTransStartTimeOffset) {
                uint32_t totalMinutes = *(uint32_t*)(env + m_timeOffset); // 0x3E58
                float secondsFrac = *(float*)(env + m_timeOffset + 4); // 0x3E5C
                
                // minutes % 1440 (0x5a0)
                uint32_t minutesInDay = totalMinutes % 1440;
                uint32_t startTimeMs = (uint32_t)(secondsFrac * 60000.0f) + (minutesInDay * 60000);
                *(uint32_t*)(env + m_weatherTransStartTimeOffset) = startTimeMs;
                }
                }
                }

    // Trigger environment update to apply weather changes immediately
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
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

    return *(float*)(env + GetVerOffset(0x3ff0, 0x4010)); // ATS Hardcoded
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

    // 0xc0 = Nice weather container, 0x100 = Bad weather container
    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10); // Usually 24 (hourly)

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
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

float ClimateService::GetRoadWetness() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x3ffc, 0x401C)); // ATS Mixed value
}

void ClimateService::SetRoadWetness(float wetness) {
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

    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    if (!profiles_ptr || profileCount == 0) return;

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // Rain Max Wetness Array Pointer is at profile + 0x488 (1.59) / 0x4A8 (1.60)
        uintptr_t wetArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x488, 0x4A8));
        uint64_t wetArrayCount = *(uint64_t*)(profile + GetVerOffset(0x490, 0x4B0));

        if (wetArrayPtr && (uint64_t)variationIdx < wetArrayCount) {
            *(float*)(wetArrayPtr + (variationIdx * 4)) = wetness;
        }
    }

    *(float*)(env + GetVerOffset(0x3ffc, 0x401C)) = wetness; // Force mixed value

    // Trigger update
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

float ClimateService::GetFogDensity() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x3fe8, 0x4008)); // ATS Mixed value
}

void ClimateService::SetFogDensity(float density) {
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

    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    if (!profiles_ptr || profileCount == 0) return;

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // Fog Density Array Pointer is at profile + 0x2C8 (1.59) / 0x2E8 (1.60)
        uintptr_t fogArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x2C8, 0x2E8));
        uint64_t fogArrayCount = *(uint64_t*)(profile + GetVerOffset(0x2D0, 0x2F0));

        if (fogArrayPtr && (uint64_t)variationIdx < fogArrayCount) {
            *(float*)(fogArrayPtr + (variationIdx * 4)) = density;
        }
    }

    *(float*)(env + GetVerOffset(0x3fe8, 0x4008)) = density; // Force mixed value

    // Trigger update
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

float ClimateService::GetFogOffset() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x3fe4, 0x4004)); // ATS Mixed value
}

void ClimateService::SetFogOffset(float offset) {
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

    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    if (!profiles_ptr || profileCount == 0) return;

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // Fog Offset Array Pointer is at profile + 0x2A8 (1.59) / 0x2C8 (1.60)
        uintptr_t offsetArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x2A8, 0x2C8));
        uint64_t offsetArrayCount = *(uint64_t*)(profile + GetVerOffset(0x2B0, 0x2D0));

        if (offsetArrayPtr && (uint64_t)variationIdx < offsetArrayCount) {
            *(float*)(offsetArrayPtr + (variationIdx * 4)) = offset;
        }
    }

    *(float*)(env + GetVerOffset(0x3fe4, 0x4004)) = offset; // Force mixed value

    // Trigger update
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

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

bool ClimateService::IsLightningEnabled() {
    if (!m_isInitialized) return false;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return false;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return false;

    return *(bool*)(env + GetVerOffset(0x3ef1, 0x3f11)); 
}

void ClimateService::SetLightningEnabled(bool enabled) {
    if (!m_isInitialized) return;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return;

    *(bool*)(env + GetVerOffset(0x3ef1, 0x3f11)) = enabled;
}

float ClimateService::GetLightningIntensity() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x3ff4, 0x4014)); // ATS Hardcoded
}

void ClimateService::SetLightningIntensity(float intensity) {
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

    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    if (!profiles_ptr || profileCount == 0) return;

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // Lightning Intensity Array Pointer is at profile + 0x468 (1.59) / 0x488 (1.60)
        uintptr_t lightArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x468, 0x488));
        uint64_t lightArrayCount = *(uint64_t*)(profile + GetVerOffset(0x470, 0x490));

        if (lightArrayPtr && (uint64_t)variationIdx < lightArrayCount) {
            *(float*)(lightArrayPtr + (variationIdx * 4)) = intensity;
        }
    }

    *(float*)(env + GetVerOffset(0x3ff4, 0x4014)) = intensity;

    // Trigger update
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

float ClimateService::GetSnowIntensity() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x4000, 0x4020)); // ATS Mixed value
}

void ClimateService::SetSnowIntensity(float intensity) {
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

    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    if (!profiles_ptr || profileCount == 0) return;

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // Snow Intensity Array Pointer is at profile + 0x4C8 (1.59) / 0x4E8 (1.60)
        uintptr_t snowArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x4C8, 0x4E8));
        uint64_t snowArrayCount = *(uint64_t*)(profile + GetVerOffset(0x4D0, 0x4F0));

        if (snowArrayPtr && (uint64_t)variationIdx < snowArrayCount) {
            *(float*)(snowArrayPtr + (variationIdx * 4)) = intensity;
        }
    }

    *(float*)(env + GetVerOffset(0x4000, 0x4020)) = intensity; // Force mixed value (0x3f30 + 0xD0)

    // Trigger update
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

float ClimateService::GetSunAppearance(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x188, 0x1A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x190, 0x1B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetSunAppearance(int profileSlot, uint32_t variationIdx, float val) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x188, 0x1A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x190, 0x1B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = val;
}

float ClimateService::GetDashboardTemperature() {
    if (!m_isInitialized) return 0.0f;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0.0f;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env) return 0.0f;

    return *(float*)(env + GetVerOffset(0x28c, 0x2a4)); // Fixed offset from FUN_14045e370
}

void ClimateService::GetSnowflakeSize(float& minSize, float& maxSize) {
    minSize = 0.0f; maxSize = 0.0f;
    if (!m_isInitialized) return;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return;

    minSize = *(float*)(env + GetVerOffset(0x4004, 0x4024)); // 0x3f30 + 0xD4
    maxSize = *(float*)(env + GetVerOffset(0x4008, 0x4028)); // 0x3f30 + 0xD8
}

void ClimateService::SetSnowflakeSize(float minSize, float maxSize) {
    if (!m_isInitialized) return;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env || !m_climatePtrOffset) return;

    uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
    if (!climate) return;

    int32_t mode = GetWeatherMode();
    uint32_t variationIdx = GetSkyboxIndex(mode);
    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // snowflake_size_range is a vec2 array at profile + 0x4E8 (1.59) / 0x508 (1.60)
        uintptr_t sizeArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x4E8, 0x508));
        uint64_t sizeArrayCount = *(uint64_t*)(profile + GetVerOffset(0x4F0, 0x510));
        if (sizeArrayPtr && (uint64_t)variationIdx < sizeArrayCount) {
            *(float*)(sizeArrayPtr + (variationIdx * 8)) = minSize;
            *(float*)(sizeArrayPtr + (variationIdx * 8) + 4) = maxSize;
        }
    }

    *(float*)(env + GetVerOffset(0x4004, 0x4024)) = minSize;
    *(float*)(env + GetVerOffset(0x4008, 0x4028)) = maxSize;
}

void ClimateService::GetSnowChaos(float& rate, float& weight) {
    rate = 0.0f; weight = 0.0f;
    if (!m_isInitialized) return;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return;

    rate = *(float*)(env + GetVerOffset(0x400c, 0x402C));   // 0x3f30 + 0xDC (Chaos Rate)
    weight = *(float*)(env + GetVerOffset(0x4010, 0x4030)); // 0x3f30 + 0xE0 (Chaos Weight)
}

void ClimateService::SetSnowChaos(float rate, float weight) {
    if (!m_isInitialized) return;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env || !m_climatePtrOffset) return;

    uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
    if (!climate) return;

    int32_t mode = GetWeatherMode();
    uint32_t variationIdx = GetSkyboxIndex(mode);
    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t profiles_ptr = *(uintptr_t*)(container + 0x08);
    uint64_t profileCount = *(uint64_t*)(container + 0x10);

    for (uint64_t i = 0; i < profileCount; ++i) {
        uintptr_t profile = *(uintptr_t*)(profiles_ptr + (i * 8));
        if (!profile) continue;

        // snow_chaos_rate at 0x508, snow_chaos_weight at 0x528 (1.59)
        uintptr_t rateArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x508, 0x528));
        uintptr_t weightArrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x528, 0x548));
        
        if (rateArrayPtr) *(float*)(rateArrayPtr + (variationIdx * 4)) = rate;
        if (weightArrayPtr) *(float*)(weightArrayPtr + (variationIdx * 4)) = weight;
    }

    *(float*)(env + GetVerOffset(0x400c, 0x402C)) = rate;
    *(float*)(env + GetVerOffset(0x4010, 0x4030)) = weight;
}

float ClimateService::GetCloudSpeed(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x3A8, 0x3C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x3B0, 0x3D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetCloudSpeed(int profileSlot, uint32_t variationIdx, float speed) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x3A8, 0x3C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x3B0, 0x3D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = speed;
}

float ClimateService::GetColorSaturation(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x608, 0x628));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x610, 0x630));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 1.0f;
}

void ClimateService::SetColorSaturation(int profileSlot, uint32_t variationIdx, float saturation) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x608, 0x628));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x610, 0x630));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = saturation;
    }
}

float ClimateService::GetSunGlowSize(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x648, 0x668));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x650, 0x670));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetSunGlowSize(int profileSlot, uint32_t variationIdx, float size) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x648, 0x668));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x650, 0x670));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = size;
}

float ClimateService::GetSunShadowStrength(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x1C8, 0x1E8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x1D0, 0x1F0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetSunShadowStrength(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x1C8, 0x1E8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x1D0, 0x1F0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetStability(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x848, 0x868));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x850, 0x870));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetStability(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x848, 0x868));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x850, 0x870));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

uint64_t ClimateService::GetSkyboxCount(int32_t weatherMode) {
    if (!m_isInitialized) return 0;
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return 0;
    basePtr += m_environmentAdjustment;

    uintptr_t env = *(uintptr_t*)(basePtr + m_envObjectOffset);
    if (!env || !m_climatePtrOffset) return 0;

    uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
    if (!climate) return 0;

    // Nice/Bad weather containers: 1.59 (0xc0/0x100), 1.60 (0xd0/0x110)
    uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
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

    uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t indexArray = *(uintptr_t*)(container + 0x28);
    uint64_t slotCount = *(uint64_t*)(container + 0x30); 

    if (!indexArray || (uint64_t)slot >= slotCount) return 0;

    return (uint32_t)*(uint64_t*)(indexArray + (slot * 8));
}

std::string ClimateService::GetCurrentClimateName() {
    if (!m_isInitialized) return "unknown";
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return "unknown";
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return "unknown";

    uintptr_t climate = *(uintptr_t*)(env + m_climatePtrOffset);
    if (!climate) return "unknown";

    // Climate name UnitID is at +0x0c
    uint32_t unitId = *(uint32_t*)(climate + 0x0c);
    return ResolveSCSUnitName(unitId);
}

std::vector<ClimateService::ClimateInfo> ClimateService::GetAvailableClimates() {
    std::vector<ClimateInfo> climates;
    if (!m_isInitialized) return climates;

    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return climates;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return climates;

    // Based on FUN_14046e020: 1.59 (+0x130/0x138), 1.60 (+0x150/0x158)
    uintptr_t arrayPtr = *(uintptr_t*)(env + GetVerOffset(0x130, 0x150));
    uint64_t count = *(uint64_t*)(env + GetVerOffset(0x138, 0x158));

    uintptr_t unitTableAddr = (uintptr_t)GetModuleHandleA(NULL) + 0x2DD59D0;

    if (arrayPtr && count > 0 && count < 512) {
        uintptr_t* entries = (uintptr_t*)arrayPtr;
        for (uint64_t i = 0; i < count; ++i) {
            uintptr_t climateObj = entries[i];
            if (climateObj) {
                uint32_t unitId = *(uint32_t*)(climateObj + 0x0c);

                uint32_t poolIdx = (unitId >> 20) & 0x7FF;
                uint32_t itemIdx = (unitId & 0xFFFF);
                uintptr_t poolPtr = ((uintptr_t*)unitTableAddr)[poolIdx];

                if (poolPtr) {
                    uint8_t itemSize = *(uint8_t*)(poolPtr + 0x68);
                    uintptr_t itemsBase = *(uintptr_t*)(poolPtr + 0x00);
                    uint64_t* tokenPtr = (uint64_t*)(itemsBase + (itemIdx * itemSize) * 8);

                    uint64_t shortToken = tokenPtr[itemSize - 1];
                    std::string name = ResolveSCSUnitName(unitId);

                    if (!name.empty() && name.find("err_") == std::string::npos) {
                        climates.push_back({name, shortToken});
                    }
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
    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    
    uint32_t idx = GetActiveProfileIndex(profileSlot);
    uintptr_t profilesArray = *(uintptr_t*)(container + 0x08);
    uint64_t count = *(uint64_t*)(container + 0x10);

    if (profilesArray && idx < count) {
        uintptr_t profile = *(uintptr_t*)(profilesArray + (idx * 8));
        if (profile) {
            uint32_t unitId = *(uint32_t*)(profile + 0x0c);
            return ResolveSCSUnitName(unitId);
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

void ClimateService::LogSunProfileData(int profileSlot) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;

    int32_t mode = GetWeatherMode();
    std::string profileName = GetActiveProfileName(profileSlot);
    uint32_t varIdx = GetSkyboxIndex(mode);

    logger->Info("--- SUN PROFILE DISCOVERY DUMP ---");
    logger->Info("Profile: {} | Mode: {} | VarIdx: {}", profileName, mode, varIdx);
    logger->Info("----------------------------------");

    // Scan every 8 bytes to not miss any pointers
    for (uint32_t offset = 0x10; offset <= 0x900; offset += 8) {
        uintptr_t arrayPtr = *(uintptr_t*)(profile + offset);
        uint64_t count = *(uint64_t*)(profile + offset + 8);

        // Standard SCS array check
        if (arrayPtr > 0x10000 && count > 0 && count < 100) {
            float* data = (float*)arrayPtr;
            uint32_t safeIdx = (varIdx < count) ? varIdx : 0;
            
            float v1 = data[safeIdx * 3 + 0];
            float v2 = data[safeIdx * 3 + 1];
            float v3 = data[safeIdx * 3 + 2];
            float f1 = ((float*)arrayPtr)[safeIdx];

            // Add semantic labels for verified offsets (offsets are for 1.59, we check both)
            const char* label = "";
            if (offset == GetVerOffset(0x48, 0x68)) label = "[AMBIENT]";
            else if (offset == GetVerOffset(0x68, 0x88)) label = "[DIFFUSE]";
            else if (offset == GetVerOffset(0x88, 0xA8)) label = "[SPECULAR]";
            else if (offset == GetVerOffset(0xE8, 0x108)) label = "[SKY_COLOR]";
            else if (offset == GetVerOffset(0x108, 0x128)) label = "[SKY_BOTTOM]";
            else if (offset == GetVerOffset(0x168, 0x188)) label = "[SUN_COLOR]";
            else if (offset == GetVerOffset(0x188, 0x1A8)) label = "[SUN_OPACITY]";
            else if (offset == GetVerOffset(0x1A8, 0x1C8)) label = "[SUN_HALO]";
            else if (offset == GetVerOffset(0x248, 0x268)) label = "[FOG_COLOR]";
            else if (offset == GetVerOffset(0x2E8, 0x308)) label = "[TEMPERATURE]";
            else if (offset == GetVerOffset(0x448, 0x468)) label = "[RAIN_INT]";
            else if (offset == GetVerOffset(0x608, 0x628)) label = "[SATURATION]";

            logger->Info("Offset 0x{:03X}: {} [V3: {:.3f}, {:.3f}, {:.3f}] [F1: {:.3f}] (Count: {})", 
                         offset, label, v1, v2, v3, f1, (uint32_t)count);
            
            // Arrays are 0x20 in size, skip to next potential array
            // offset += 0x18; // Optional, but 8-byte scan is safer
        }
    }
    logger->Info("--- END OF DISCOVERY DUMP ---");
}

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
    uintptr_t container = climate + (mode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    
    uint32_t idx = GetActiveProfileIndex(profileSlot);
    uintptr_t profilesArray = *(uintptr_t*)(container + 0x08);
    uint64_t count = *(uint64_t*)(container + 0x10);

    if (profilesArray && (uint64_t)idx < count) {
        return *(uintptr_t*)(profilesArray + (idx * 8));
    }
    return 0;
}

// --- Profile Parameter Methods (Internal helpers for semantic API) ---

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
            typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
            ((UpdateEnv_t)updateFn)(env);
        }
    }
}

Utils::Vector3 ClimateService::GetAmbientColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x48, 0x68), variationIdx);
}

void ClimateService::SetAmbientColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x48, 0x68), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSunColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x168, 0x188), variationIdx);
}

void ClimateService::SetSunColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x168, 0x188), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSunDiffuseColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x68, 0x88), variationIdx);
}

void ClimateService::SetSunDiffuseColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x68, 0x88), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSunSpecularColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x88, 0xA8), variationIdx);
}

void ClimateService::SetSunSpecularColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x88, 0xA8), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSkyColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0xE8, 0x108), variationIdx);
}

void ClimateService::SetSkyColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0xE8, 0x108), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSkyBottomColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x108, 0x128), variationIdx);
}

void ClimateService::SetSkyBottomColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x108, 0x128), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSunHaloColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x1A8, 0x1C8), variationIdx);
}

void ClimateService::SetSunHaloColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x1A8, 0x1C8), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetFogColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x248, 0x268), variationIdx);
}

void ClimateService::SetFogColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x248, 0x268), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetColorBalance(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x5E8, 0x608), variationIdx);
}

void ClimateService::SetColorBalance(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x5E8, 0x608), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetMoonColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x1E8, 0x208), variationIdx);
}

void ClimateService::SetMoonColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x1E8, 0x208), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetMoonHaloColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x208, 0x228), variationIdx);
}

void ClimateService::SetMoonHaloColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x208, 0x228), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetFogColor2(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x268, 0x288), variationIdx);
}

void ClimateService::SetFogColor2(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x268, 0x288), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetStarmapColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x128, 0x148), variationIdx);
}

void ClimateService::SetStarmapColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x128, 0x148), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetStarsColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x148, 0x168), variationIdx);
}

void ClimateService::SetStarsColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x148, 0x168), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetSunshaftColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x628, 0x648), variationIdx);
}

void ClimateService::SetSunshaftColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x628, 0x648), variationIdx, color, env, m_updateFnAddr);
}

Utils::Vector3 ClimateService::GetLowIntensityColor(int profileSlot, uint32_t variationIdx) {
    return GetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x6A8, 0x6C8), variationIdx);
}

void ClimateService::SetLowIntensityColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color) {
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    uintptr_t env = basePtr ? *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset) : 0;
    SetProfileVec3Internal(GetActiveProfilePtr(profileSlot), GetVerOffset(0x6A8, 0x6C8), variationIdx, color, env, m_updateFnAddr);
}

float ClimateService::GetEnvIntensity(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0xA8, 0xC8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0xB0, 0xD0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetEnvIntensity(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0xA8, 0xC8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0xB0, 0xD0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetEnvStaticMod(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0xC8, 0xE8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0xD0, 0xF0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetEnvStaticMod(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0xC8, 0xE8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0xD0, 0xF0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetMoonHaloScale(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x228, 0x248));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x230, 0x250));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetMoonHaloScale(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x228, 0x248));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x230, 0x250));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetFogVGradient(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x288, 0x2A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x290, 0x2B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetFogVGradient(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x288, 0x2A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x290, 0x2B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetSunOpacity(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x188, 0x1A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x190, 0x1B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 1.0f;
}

void ClimateService::SetSunOpacity(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x188, 0x1A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x190, 0x1B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = value;
    }
}

float ClimateService::GetContrast(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x788, 0x7A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x790, 0x7B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 1.0f;
}

void ClimateService::SetContrast(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x788, 0x7A8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x790, 0x7B0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = value;
    }
}

float ClimateService::GetBloomIntensity(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x808, 0x828));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x810, 0x830));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 1.0f;
}

void ClimateService::SetBloomIntensity(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x808, 0x828));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x810, 0x830));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = value;
    }
}

float ClimateService::GetBloomThreshold(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.5f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7C8, 0x7E8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7D0, 0x7F0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 0.5f;
}

void ClimateService::SetBloomThreshold(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7C8, 0x7E8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7D0, 0x7F0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = value;
    }
}

float ClimateService::GetBloomLimit(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7E8, 0x808));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7F0, 0x810));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        return *(float*)(arrayPtr + variationIdx * 4);
    }
    return 1.0f;
}

void ClimateService::SetBloomLimit(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7E8, 0x808));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7F0, 0x810));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) {
        *(float*)(arrayPtr + variationIdx * 4) = value;
    }
}

void ClimateService::GetExposureLimits(int profileSlot, uint32_t variationIdx, float& minScale, float& maxScale) {
    minScale = 0.5f; maxScale = 2.0f;
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    
    uintptr_t minArray = *(uintptr_t*)(profile + GetVerOffset(0x6C8, 0x6E8));
    uintptr_t maxArray = *(uintptr_t*)(profile + GetVerOffset(0x6E8, 0x708));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x6D0, 0x6F0));

    if (minArray > 0x1000 && (uint64_t)variationIdx < count) minScale = *(float*)(minArray + variationIdx * 4);
    if (maxArray > 0x1000 && (uint64_t)variationIdx < count) maxScale = *(float*)(maxArray + variationIdx * 4);
}

void ClimateService::SetExposureLimits(int profileSlot, uint32_t variationIdx, float minScale, float maxScale) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;

    uintptr_t minArray = *(uintptr_t*)(profile + GetVerOffset(0x6C8, 0x6E8));
    uintptr_t maxArray = *(uintptr_t*)(profile + GetVerOffset(0x6E8, 0x708));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x6D0, 0x6F0));

    if (minArray > 0x1000 && (uint64_t)variationIdx < count) *(float*)(minArray + variationIdx * 4) = minScale;
    if (maxArray > 0x1000 && (uint64_t)variationIdx < count) *(float*)(maxArray + variationIdx * 4) = maxScale;
}

float ClimateService::GetSunshaftSize(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x648, 0x668));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x650, 0x670));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetSunshaftSize(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x648, 0x668));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x650, 0x670));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetRainAdditionalAmbient(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x4A8, 0x4C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x4B0, 0x4D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetRainAdditionalAmbient(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x4A8, 0x4C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x4B0, 0x4D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetSnowAdditionalAmbient(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x508, 0x528));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x510, 0x530));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.0f;
}

void ClimateService::SetSnowAdditionalAmbient(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x508, 0x528));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x510, 0x530));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetBloomStandardDeviation(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.5f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x828, 0x848));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x830, 0x850));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.5f;
}

void ClimateService::SetBloomStandardDeviation(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x828, 0x848));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x830, 0x850));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetShoulderLength(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 0.5f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7A8, 0x7C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7B0, 0x7D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 0.5f;
}

void ClimateService::SetShoulderLength(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x7A8, 0x7C8));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x7B0, 0x7D0));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

float ClimateService::GetTargetGray(int profileSlot, uint32_t variationIdx) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return 1.0f;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x768, 0x788));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x770, 0x790));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) return *(float*)(arrayPtr + variationIdx * 4);
    return 1.0f;
}

void ClimateService::SetTargetGray(int profileSlot, uint32_t variationIdx, float value) {
    uintptr_t profile = GetActiveProfilePtr(profileSlot);
    if (!profile) return;
    uintptr_t arrayPtr = *(uintptr_t*)(profile + GetVerOffset(0x768, 0x788));
    uint64_t count = *(uint64_t*)(profile + GetVerOffset(0x770, 0x790));
    if (arrayPtr > 0x1000 && (uint64_t)variationIdx < count) *(float*)(arrayPtr + variationIdx * 4) = value;
}

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

void ClimateService::SetClimate(uint64_t token, bool instant) {
    if (!m_isInitialized || !m_setClimateFnAddr) return;

    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return;

    typedef void(__fastcall* SetClimate_t)(uintptr_t env, uint64_t* pToken, uint8_t instant);
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

    uintptr_t container = climate + (weatherMode == 0 ? GetVerOffset(0xc0, 0xd0) : GetVerOffset(0x100, 0x110));
    uintptr_t indexArray = *(uintptr_t*)(container + 0x28);
    uint64_t slotCount = *(uint64_t*)(container + 0x30);

    if (!indexArray) return;

    // Set the selected variation index for all time slots
    for (uint64_t i = 0; i < slotCount; ++i) {
        *(uint64_t*)(indexArray + (i * 8)) = (uint64_t)index;
    }

    // Trigger environment update to apply texture changes
    if (m_updateFnAddr) {
        typedef void(__fastcall* UpdateEnv_t)(uintptr_t rcx);
        ((UpdateEnv_t)m_updateFnAddr)(env);
    }
}

void ClimateService::LogEnvironmentState() {
    if (!m_isInitialized) return;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");

    int32_t mode = GetWeatherMode();
    uint32_t skyboxIdx = GetSkyboxIndex(mode);
    float rain = GetRainIntensity();
    float wetness = GetRoadWetness();
    float snow = GetSnowIntensity();
    float fog = GetFogDensity();
    float sunApp = GetSunAppearance(0, skyboxIdx);
    float lightning = GetLightningIntensity();

    float snowMin, snowMax, chaosRate, chaosWeight;
    GetSnowflakeSize(snowMin, snowMax);
    GetSnowChaos(chaosRate, chaosWeight);

    // Visibility offsets from UpdateEnvironmentValues analysis
    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    float vis1 = 0.0f, vis2 = 0.0f;
    if (basePtr) {
        uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
        if (env) {
            vis1 = *(float*)(env + GetVerOffset(0x1664, 0x1684));
            vis2 = *(float*)(env + GetVerOffset(0x1668, 0x1688));
        }
    }

    logger->Info("--- Environment State Dump ---");
    logger->Info("Weather Mode: {} ({})", mode, (mode == 0 ? "Nice" : "Bad"));
    logger->Info("Skybox Index (Slot 0): {}", skyboxIdx);
    logger->Info("Rain Intensity (0x3ff0): {:.4f}", rain);
    logger->Info("Road Wetness (0x3ffc): {:.4f}", wetness);
    logger->Info("Snow Intensity (0x4000): {:.4f}", snow);
    logger->Info("Snowflake Size (0x4004/08): {:.4f} / {:.4f}", snowMin, snowMax);
    logger->Info("Snow Chaos (0x400C/10): Rate={:.4f}, Weight={:.4f}", chaosRate, chaosWeight);
    logger->Info("Fog Density (0x3fe8): {:.4f}", fog);
    logger->Info("Visibility 1 (0x1664): {:.4f}", vis1);
    logger->Info("Visibility 2 (0x1668): {:.4f}", vis2);
    logger->Info("Sun Appearance (0x3ff8): {:.4f}", sunApp);
    logger->Info("Lightning Intensity (0x3ff4): {:.4f}", lightning);
    logger->Info("------------------------------");

    DumpEnvironmentMemory();
}

void ClimateService::DumpEnvironmentMemory() {
    if (!m_isInitialized) return;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateService");

    uintptr_t basePtr = *(uintptr_t*)m_environmentBasePtr;
    if (!basePtr) return;
    uintptr_t env = *(uintptr_t*)(basePtr + m_environmentAdjustment + m_envObjectOffset);
    if (!env) return;

    logger->Info("--- RAW ENVIRONMENT DUMP (Floats from 0x3f30) ---");
    
    // Dump 128 floats (512 bytes) starting from 0x3f30 / 0x3f50
    uintptr_t startAddr = env + GetVerOffset(0x3f30, 0x3f50);
    for (int i = 0; i < 128; ++i) {
        float val = *(float*)(startAddr + (i * 4));
        // Only log if it's a "reasonable" float or we want full dump
        // Let's log everything to be sure
        logger->Info("[+0x{:03X}] 0x{:04X}: {:.6f}", (i * 4), (uint32_t)(0x3f30 + (i * 4)), val);
    }
    logger->Info("--- END OF RAW DUMP ---");
}

}  // namespace Data::GameData
SPF_NS_END
