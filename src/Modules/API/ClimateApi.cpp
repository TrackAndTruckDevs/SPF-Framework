#include "SPF/Modules/API/ClimateApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/SPF_API/SPF_Climate_API.h"

#include <cstdint>
#include <cstring>

SPF_NS_BEGIN
namespace Modules::API {

using namespace SPF::Data::GameData;

// ================================================================================================
// Implementation Macros — generate ClimateApi:: scoped methods for variant attributes
// ================================================================================================

#define IMPL_CL_FLOAT(name)                                                                                                 \
  uint64_t ClimateApi::T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile) {                                              \
    return ClimateService::GetInstance().Get##name##Count({profile.index, profile.isBad});                                   \
  }                                                                                                                         \
  float ClimateApi::T_Climate_Get##name(SPF_Climate_ProfileRef profile) {                                                        \
    return ClimateService::GetInstance().Get##name({profile.index, profile.isBad});                                          \
  }                                                                                                                         \
  void ClimateApi::T_Climate_Set##name(SPF_Climate_ProfileRef profile, float value) {                                            \
    ClimateService::GetInstance().Set##name({profile.index, profile.isBad}, value);                                          \
  }                                                                                                                         \
  float ClimateApi::T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx) {                                 \
    return ClimateService::GetInstance().Get##name##ByIndex({profile.index, profile.isBad}, idx);                            \
  }                                                                                                                         \
  void ClimateApi::T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, float value) {                     \
    ClimateService::GetInstance().Set##name##ByIndex({profile.index, profile.isBad}, idx, value);                            \
  }                                                                                                                         \
  float ClimateApi::T_Climate_GetBlended##name() {                                                                               \
    return ClimateService::GetInstance().GetBlended##name();                                                                 \
  }                                                                                                                         \
  void ClimateApi::T_Climate_SetBlended##name(float val, float min, float max) {                                                 \
    ClimateService::GetInstance().SetBlended##name(val, min, max);                                                           \
  }

#define ASSIGN_CL_FLOAT(api, name) \
  (api)->Get##name##Count = &ClimateApi::T_Climate_Get##name##Count; \
  (api)->Get##name = &ClimateApi::T_Climate_Get##name; \
  (api)->Set##name = &ClimateApi::T_Climate_Set##name; \
  (api)->Get##name##ByIndex = &ClimateApi::T_Climate_Get##name##ByIndex; \
  (api)->Set##name##ByIndex = &ClimateApi::T_Climate_Set##name##ByIndex; \
  (api)->GetBlended##name = &ClimateApi::T_Climate_GetBlended##name; \
  (api)->SetBlended##name = &ClimateApi::T_Climate_SetBlended##name

#define IMPL_CL_INT32(name)                                                                                                \
  uint64_t ClimateApi::T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile) {                                             \
    return ClimateService::GetInstance().Get##name##Count({profile.index, profile.isBad});                                  \
  }                                                                                                                        \
  int32_t ClimateApi::T_Climate_Get##name(SPF_Climate_ProfileRef profile) {                                                     \
    return ClimateService::GetInstance().Get##name({profile.index, profile.isBad});                                         \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name(SPF_Climate_ProfileRef profile, int32_t value) {                                         \
    ClimateService::GetInstance().Set##name({profile.index, profile.isBad}, value);                                         \
  }                                                                                                                        \
  int32_t ClimateApi::T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx) {                              \
    return ClimateService::GetInstance().Get##name##ByIndex({profile.index, profile.isBad}, idx);                            \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, int32_t value) {                  \
    ClimateService::GetInstance().Set##name##ByIndex({profile.index, profile.isBad}, idx, value);                            \
  }

#define ASSIGN_CL_INT32(api, name) \
  (api)->Get##name##Count = &ClimateApi::T_Climate_Get##name##Count; \
  (api)->Get##name = &ClimateApi::T_Climate_Get##name; \
  (api)->Set##name = &ClimateApi::T_Climate_Set##name; \
  (api)->Get##name##ByIndex = &ClimateApi::T_Climate_Get##name##ByIndex; \
  (api)->Set##name##ByIndex = &ClimateApi::T_Climate_Set##name##ByIndex

#define IMPL_CL_VECTOR3(name)                                                                                              \
  uint64_t ClimateApi::T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile) {                                             \
    return ClimateService::GetInstance().Get##name##Count({profile.index, profile.isBad});                                  \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Get##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3* out) {                              \
    auto v = ClimateService::GetInstance().Get##name({profile.index, profile.isBad});                                       \
    out->x = v.x; out->y = v.y; out->z = v.z;                                                                              \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3 v) {                                 \
    ClimateService::GetInstance().Set##name({profile.index, profile.isBad}, {v.x, v.y, v.z});                               \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector3* out) {       \
    auto v = ClimateService::GetInstance().Get##name##ByIndex({profile.index, profile.isBad}, idx);                         \
    out->x = v.x; out->y = v.y; out->z = v.z;                                                                              \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector3 v) {          \
    ClimateService::GetInstance().Set##name##ByIndex({profile.index, profile.isBad}, idx, {v.x, v.y, v.z});                 \
  }                                                                                                                        \
  void ClimateApi::T_Climate_GetBlended##name(SPF_Climate_Vector3* out) {                                                       \
    auto v = ClimateService::GetInstance().GetBlended##name();                                                               \
    out->x = v.x; out->y = v.y; out->z = v.z;                                                                              \
  }                                                                                                                        \
  void ClimateApi::T_Climate_SetBlended##name(SPF_Climate_Vector3 v, float maxComp) {                                           \
    ClimateService::GetInstance().SetBlended##name({v.x, v.y, v.z}, maxComp);                                               \
  }

#define ASSIGN_CL_VECTOR3(api, name) \
  (api)->Get##name##Count = &ClimateApi::T_Climate_Get##name##Count; \
  (api)->Get##name = &ClimateApi::T_Climate_Get##name; \
  (api)->Set##name = &ClimateApi::T_Climate_Set##name; \
  (api)->Get##name##ByIndex = &ClimateApi::T_Climate_Get##name##ByIndex; \
  (api)->Set##name##ByIndex = &ClimateApi::T_Climate_Set##name##ByIndex; \
  (api)->GetBlended##name = &ClimateApi::T_Climate_GetBlended##name; \
  (api)->SetBlended##name = &ClimateApi::T_Climate_SetBlended##name

#define IMPL_CL_VECTOR2(name)                                                                                              \
  uint64_t ClimateApi::T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile) {                                             \
    return ClimateService::GetInstance().Get##name##Count({profile.index, profile.isBad});                                  \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Get##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2* out) {                              \
    auto v = ClimateService::GetInstance().Get##name({profile.index, profile.isBad});                                       \
    out->x = v.x; out->y = v.y;                                                                                             \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2 v) {                                 \
    ClimateService::GetInstance().Set##name({profile.index, profile.isBad}, {v.x, v.y});                                    \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector2* out) {       \
    auto v = ClimateService::GetInstance().Get##name##ByIndex({profile.index, profile.isBad}, idx);                         \
    out->x = v.x; out->y = v.y;                                                                                             \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector2 v) {          \
    ClimateService::GetInstance().Set##name##ByIndex({profile.index, profile.isBad}, idx, {v.x, v.y});                      \
  }                                                                                                                        \
  void ClimateApi::T_Climate_GetBlended##name(SPF_Climate_Vector2* out) {                                                       \
    auto v = ClimateService::GetInstance().GetBlended##name();                                                               \
    out->x = v.x; out->y = v.y;                                                                                             \
  }                                                                                                                        \
  void ClimateApi::T_Climate_SetBlended##name(SPF_Climate_Vector2 v, float maxComp) {                                           \
    ClimateService::GetInstance().SetBlended##name({v.x, v.y}, maxComp);                                                     \
  }

#define ASSIGN_CL_VECTOR2(api, name) \
  (api)->Get##name##Count = &ClimateApi::T_Climate_Get##name##Count; \
  (api)->Get##name = &ClimateApi::T_Climate_Get##name; \
  (api)->Set##name = &ClimateApi::T_Climate_Set##name; \
  (api)->Get##name##ByIndex = &ClimateApi::T_Climate_Get##name##ByIndex; \
  (api)->Set##name##ByIndex = &ClimateApi::T_Climate_Set##name##ByIndex; \
  (api)->GetBlended##name = &ClimateApi::T_Climate_GetBlended##name; \
  (api)->SetBlended##name = &ClimateApi::T_Climate_SetBlended##name

#define IMPL_CL_TEXTURE(name)                                                                                              \
  uint64_t ClimateApi::T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile) {                                             \
    return ClimateService::GetInstance().Get##name##Count({profile.index, profile.isBad});                                  \
  }                                                                                                                        \
  int ClimateApi::T_Climate_Get##name(SPF_Climate_ProfileRef profile, char* buf, int sz) {                                      \
    auto s = ClimateService::GetInstance().Get##name({profile.index, profile.isBad});                                       \
    int len = static_cast<int>(s.length());                                                                                 \
    if (buf && sz > 0) { std::strncpy(buf, s.c_str(), static_cast<size_t>(sz - 1)); buf[sz - 1] = '\0'; }                  \
    return len;                                                                                                             \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name(SPF_Climate_ProfileRef profile, const char* val) {                                       \
    ClimateService::GetInstance().Set##name({profile.index, profile.isBad}, val ? std::string(val) : "");                    \
  }                                                                                                                        \
  int ClimateApi::T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, char* buf, int sz) {               \
    auto s = ClimateService::GetInstance().Get##name##ByIndex({profile.index, profile.isBad}, idx);                         \
    int len = static_cast<int>(s.length());                                                                                 \
    if (buf && sz > 0) { std::strncpy(buf, s.c_str(), static_cast<size_t>(sz - 1)); buf[sz - 1] = '\0'; }                  \
    return len;                                                                                                             \
  }                                                                                                                        \
  void ClimateApi::T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, const char* val) {                \
    ClimateService::GetInstance().Set##name##ByIndex({profile.index, profile.isBad}, idx, val ? std::string(val) : "");     \
  }

#define ASSIGN_CL_TEXTURE(api, name) \
  (api)->Get##name##Count = &ClimateApi::T_Climate_Get##name##Count; \
  (api)->Get##name = &ClimateApi::T_Climate_Get##name; \
  (api)->Set##name = &ClimateApi::T_Climate_Set##name; \
  (api)->Get##name##ByIndex = &ClimateApi::T_Climate_Get##name##ByIndex; \
  (api)->Set##name##ByIndex = &ClimateApi::T_Climate_Set##name##ByIndex

// ================================================================================================
// Definitions for variant attributes (41 float, 2 int32, 15 vec3, 3 vec2, 5 texture)
// ================================================================================================

IMPL_CL_FLOAT(Temperature)
IMPL_CL_FLOAT(SunOpacity)
IMPL_CL_FLOAT(SunShadowStrength)
IMPL_CL_FLOAT(MoonHaloScale)
IMPL_CL_FLOAT(FogVgradient)
IMPL_CL_FLOAT(FogOffset)
IMPL_CL_FLOAT(FogDensity)
IMPL_CL_FLOAT(SpeedCoef)
IMPL_CL_FLOAT(CloudShadowWeight)
IMPL_CL_FLOAT(RainIntensity)
IMPL_CL_FLOAT(LightningIntensity)
IMPL_CL_FLOAT(RainMaxWetness)
IMPL_CL_FLOAT(RainAdditionalAmbient)
IMPL_CL_FLOAT(SnowIntensity)
IMPL_CL_FLOAT(SnowChaosRate)
IMPL_CL_FLOAT(SnowChaosWeight)
IMPL_CL_FLOAT(SnowAdditionalAmbient)
IMPL_CL_FLOAT(DofStart)
IMPL_CL_FLOAT(DofTransition)
IMPL_CL_FLOAT(DofFilterSize)
IMPL_CL_FLOAT(ColorBalance)
IMPL_CL_FLOAT(ColorSaturation)
IMPL_CL_FLOAT(SunshaftSize)
IMPL_CL_FLOAT(LowIntensityMinimum)
IMPL_CL_FLOAT(LowIntensityMaximum)
IMPL_CL_FLOAT(DarkAdaptationSpeed)
IMPL_CL_FLOAT(BrightAdaptationSpeed)
IMPL_CL_FLOAT(TargetGray)
IMPL_CL_FLOAT(MinScale)
IMPL_CL_FLOAT(MaxScale)
IMPL_CL_FLOAT(ScaleOverride)
IMPL_CL_FLOAT(Contrast)
IMPL_CL_FLOAT(ShoulderLength)
IMPL_CL_FLOAT(BloomThreshold)
IMPL_CL_FLOAT(BloomLimit)
IMPL_CL_FLOAT(BloomIntensity)
IMPL_CL_FLOAT(BloomStandardDeviation)
IMPL_CL_FLOAT(Stability)
IMPL_CL_FLOAT(MirrorSkyTexture)
IMPL_CL_FLOAT(Env)
IMPL_CL_FLOAT(EnvStaticMod)

IMPL_CL_INT32(Weight)
IMPL_CL_INT32(WindType)

IMPL_CL_VECTOR3(Ambient)
IMPL_CL_VECTOR3(Diffuse)
IMPL_CL_VECTOR3(Specular)
IMPL_CL_VECTOR3(SkyColor)
IMPL_CL_VECTOR3(SkyBottomColor)
IMPL_CL_VECTOR3(StarmapColor)
IMPL_CL_VECTOR3(StarsColor)
IMPL_CL_VECTOR3(SunColor)
IMPL_CL_VECTOR3(SunHaloColor)
IMPL_CL_VECTOR3(MoonColor)
IMPL_CL_VECTOR3(MoonHaloColor)
IMPL_CL_VECTOR3(FogColor)
IMPL_CL_VECTOR3(FogColor2)
IMPL_CL_VECTOR3(SunshaftColor)
IMPL_CL_VECTOR3(LowIntensityColor)

IMPL_CL_VECTOR2(CloudShadowAreaSize)
IMPL_CL_VECTOR2(CloudShadowSpeed)
IMPL_CL_VECTOR2(SnowFlakeSizeRange)

IMPL_CL_TEXTURE(SkyboxTexture)
IMPL_CL_TEXTURE(SkycloudMaskTexture)
IMPL_CL_TEXTURE(LightningMask)
IMPL_CL_TEXTURE(StarsTexture)
IMPL_CL_TEXTURE(CloudShadowTexture)

// ================================================================================================
// FillClimateApi
// ================================================================================================

void ClimateApi::FillClimateApi(SPF_Climate_API* climate_api) {
  if (!climate_api) return;

  // --- Section 1: Lifecycle ---
  climate_api->CL_IsReady = &T_Climate_IsReady;
  climate_api->CL_IsFinderReady = &T_Climate_IsFinderReady;
  climate_api->CL_AreAllOffsetsFound = &T_Climate_AreAllOffsetsFound;
  climate_api->CL_RefreshOffsets = &T_Climate_RefreshOffsets;

  // --- Section 2: Climate Selection ---
  climate_api->CL_GetCurrentClimateName = &T_Climate_GetCurrentClimateName;
  climate_api->CL_GetAvailableClimateCount = &T_Climate_GetAvailableClimateCount;
  climate_api->CL_GetAvailableClimateByIndex = &T_Climate_GetAvailableClimateByIndex;
  climate_api->CL_SetClimate = &T_Climate_SetClimate;

  // --- Section 3: Sun Profile ---
  climate_api->CL_GetActiveSunProfileIndex = &T_Climate_GetActiveSunProfileIndex;
  climate_api->CL_GetNextSunProfileIndex = &T_Climate_GetNextSunProfileIndex;
  climate_api->CL_GetSunProfileCount = &T_Climate_GetSunProfileCount;
  climate_api->CL_GetSunProfileName = &T_Climate_GetSunProfileName;
  climate_api->CL_GetSunProfileElevation = &T_Climate_GetSunProfileElevation;
  climate_api->CL_GetTransitionProgress = &T_Climate_GetTransitionProgress;
  climate_api->CL_GetSunAngle = &T_Climate_GetSunAngle;
  climate_api->CL_GetWeatherBlendProgress = &T_Climate_GetWeatherBlendProgress;
  climate_api->CL_SetTransitionDuration = &T_Climate_SetTransitionDuration;

  // --- Section 4: Weather Mode ---
  climate_api->CL_GetWeatherMode = &T_Climate_GetWeatherMode;
  climate_api->CL_GetNextWeatherMode = &T_Climate_GetNextWeatherMode;
  climate_api->CL_SetWeatherMode = &T_Climate_SetWeatherMode;

  // --- Section 5: Bad Weather ---
  climate_api->CL_GetBadWeatherFactor = &T_Climate_GetBadWeatherFactor;
  climate_api->CL_SetBadWeatherFactor = &T_Climate_SetBadWeatherFactor;
  climate_api->CL_GetBadWeatherMode = &T_Climate_GetBadWeatherMode;
  climate_api->CL_GetRemainingBadWeatherTime = &T_Climate_GetRemainingBadWeatherTime;

  // --- Section 6: Env Profile ---
  climate_api->CL_GetLampsOnElevation = &T_Climate_GetLampsOnElevation;
  climate_api->CL_SetLampsOnElevation = &T_Climate_SetLampsOnElevation;
  climate_api->CL_GetDayInYear = &T_Climate_GetDayInYear;
  climate_api->CL_SetDayInYear = &T_Climate_SetDayInYear;
  climate_api->CL_GetSummerTime = &T_Climate_GetSummerTime;
  climate_api->CL_SetSummerTime = &T_Climate_SetSummerTime;
  climate_api->CL_GetThunderstormProbability = &T_Climate_GetThunderstormProbability;
  climate_api->CL_SetThunderstormProbability = &T_Climate_SetThunderstormProbability;

  // --- Section 7: Profile Helpers, Elevation & Direction ---
  climate_api->CL_ActiveProfile = &T_Climate_ActiveProfile;
  climate_api->CL_NextProfile = &T_Climate_NextProfile;
  climate_api->CL_GetLowElevation = &T_Climate_GetLowElevation;
  climate_api->CL_SetLowElevation = &T_Climate_SetLowElevation;
  climate_api->CL_GetHighElevation = &T_Climate_GetHighElevation;
  climate_api->CL_SetHighElevation = &T_Climate_SetHighElevation;
  climate_api->CL_GetSunDirection = &T_Climate_GetSunDirection;
  climate_api->CL_SetSunDirection = &T_Climate_SetSunDirection;

  // --- Section 8: Variation Index ---
  climate_api->CL_GetActiveVariationIndex = &T_Climate_GetActiveVariationIndex;
  climate_api->CL_SetActiveVariationIndex = &T_Climate_SetActiveVariationIndex;
  climate_api->CL_GetNextVariationIndex = &T_Climate_GetNextVariationIndex;
  climate_api->CL_SetNextVariationIndex = &T_Climate_SetNextVariationIndex;

  // --- Section 9: Float Variants ---
  ASSIGN_CL_FLOAT(climate_api, Temperature);
  ASSIGN_CL_FLOAT(climate_api, SunOpacity);
  ASSIGN_CL_FLOAT(climate_api, SunShadowStrength);
  ASSIGN_CL_FLOAT(climate_api, MoonHaloScale);
  ASSIGN_CL_FLOAT(climate_api, FogVgradient);
  ASSIGN_CL_FLOAT(climate_api, FogOffset);
  ASSIGN_CL_FLOAT(climate_api, FogDensity);
  ASSIGN_CL_FLOAT(climate_api, SpeedCoef);
  ASSIGN_CL_FLOAT(climate_api, CloudShadowWeight);
  ASSIGN_CL_FLOAT(climate_api, RainIntensity);
  ASSIGN_CL_FLOAT(climate_api, LightningIntensity);
  ASSIGN_CL_FLOAT(climate_api, RainMaxWetness);
  ASSIGN_CL_FLOAT(climate_api, RainAdditionalAmbient);
  ASSIGN_CL_FLOAT(climate_api, SnowIntensity);
  ASSIGN_CL_FLOAT(climate_api, SnowChaosRate);
  ASSIGN_CL_FLOAT(climate_api, SnowChaosWeight);
  ASSIGN_CL_FLOAT(climate_api, SnowAdditionalAmbient);
  ASSIGN_CL_FLOAT(climate_api, DofStart);
  ASSIGN_CL_FLOAT(climate_api, DofTransition);
  ASSIGN_CL_FLOAT(climate_api, DofFilterSize);
  ASSIGN_CL_FLOAT(climate_api, ColorBalance);
  ASSIGN_CL_FLOAT(climate_api, ColorSaturation);
  ASSIGN_CL_FLOAT(climate_api, SunshaftSize);
  ASSIGN_CL_FLOAT(climate_api, LowIntensityMinimum);
  ASSIGN_CL_FLOAT(climate_api, LowIntensityMaximum);
  ASSIGN_CL_FLOAT(climate_api, DarkAdaptationSpeed);
  ASSIGN_CL_FLOAT(climate_api, BrightAdaptationSpeed);
  ASSIGN_CL_FLOAT(climate_api, TargetGray);
  ASSIGN_CL_FLOAT(climate_api, MinScale);
  ASSIGN_CL_FLOAT(climate_api, MaxScale);
  ASSIGN_CL_FLOAT(climate_api, ScaleOverride);
  ASSIGN_CL_FLOAT(climate_api, Contrast);
  ASSIGN_CL_FLOAT(climate_api, ShoulderLength);
  ASSIGN_CL_FLOAT(climate_api, BloomThreshold);
  ASSIGN_CL_FLOAT(climate_api, BloomLimit);
  ASSIGN_CL_FLOAT(climate_api, BloomIntensity);
  ASSIGN_CL_FLOAT(climate_api, BloomStandardDeviation);
  ASSIGN_CL_FLOAT(climate_api, Stability);
  ASSIGN_CL_FLOAT(climate_api, MirrorSkyTexture);
  ASSIGN_CL_FLOAT(climate_api, Env);
  ASSIGN_CL_FLOAT(climate_api, EnvStaticMod);

  // --- Section 10: Int32 Variants ---
  ASSIGN_CL_INT32(climate_api, Weight);
  ASSIGN_CL_INT32(climate_api, WindType);

  // --- Section 11: Vector3 Variants ---
  ASSIGN_CL_VECTOR3(climate_api, Ambient);
  ASSIGN_CL_VECTOR3(climate_api, Diffuse);
  ASSIGN_CL_VECTOR3(climate_api, Specular);
  ASSIGN_CL_VECTOR3(climate_api, SkyColor);
  ASSIGN_CL_VECTOR3(climate_api, SkyBottomColor);
  ASSIGN_CL_VECTOR3(climate_api, StarmapColor);
  ASSIGN_CL_VECTOR3(climate_api, StarsColor);
  ASSIGN_CL_VECTOR3(climate_api, SunColor);
  ASSIGN_CL_VECTOR3(climate_api, SunHaloColor);
  ASSIGN_CL_VECTOR3(climate_api, MoonColor);
  ASSIGN_CL_VECTOR3(climate_api, MoonHaloColor);
  ASSIGN_CL_VECTOR3(climate_api, FogColor);
  ASSIGN_CL_VECTOR3(climate_api, FogColor2);
  ASSIGN_CL_VECTOR3(climate_api, SunshaftColor);
  ASSIGN_CL_VECTOR3(climate_api, LowIntensityColor);

  // --- Section 12: Vector2 Variants ---
  ASSIGN_CL_VECTOR2(climate_api, CloudShadowAreaSize);
  ASSIGN_CL_VECTOR2(climate_api, CloudShadowSpeed);
  ASSIGN_CL_VECTOR2(climate_api, SnowFlakeSizeRange);

  // --- Section 13: Texture Variants ---
  ASSIGN_CL_TEXTURE(climate_api, SkyboxTexture);
  ASSIGN_CL_TEXTURE(climate_api, SkycloudMaskTexture);
  ASSIGN_CL_TEXTURE(climate_api, LightningMask);
  ASSIGN_CL_TEXTURE(climate_api, StarsTexture);
  ASSIGN_CL_TEXTURE(climate_api, CloudShadowTexture);
}

// ================================================================================================
// Section 1: Lifecycle
// ================================================================================================

bool ClimateApi::T_Climate_IsReady() { return ClimateService::GetInstance().IsReady(); }
bool ClimateApi::T_Climate_IsFinderReady(const char* finderName) { return ClimateService::GetInstance().IsFinderReady(finderName); }
bool ClimateApi::T_Climate_AreAllOffsetsFound() { return ClimateService::GetInstance().AreAllFindersReady(); }
bool ClimateApi::T_Climate_RefreshOffsets() { return ClimateService::GetInstance().TryFindAllOffsets(); }

// ================================================================================================
// Section 2: Climate Selection
// ================================================================================================

int ClimateApi::T_Climate_GetCurrentClimateName(char* outBuffer, int bufferSize) {
  auto name = ClimateService::GetInstance().GetCurrentClimateName();
  int len = static_cast<int>(name.length());
  if (outBuffer && bufferSize > 0) {
    std::strncpy(outBuffer, name.c_str(), static_cast<size_t>(bufferSize - 1));
    outBuffer[bufferSize - 1] = '\0';
  }
  return len;
}

int ClimateApi::T_Climate_GetAvailableClimateCount() {
  return static_cast<int>(ClimateService::GetInstance().GetAvailableClimates().size());
}

bool ClimateApi::T_Climate_GetAvailableClimateByIndex(int index, char* outNameBuffer, int nameBufferSize, uint64_t* outToken) {
  auto climates = ClimateService::GetInstance().GetAvailableClimates();
  if (index < 0 || static_cast<size_t>(index) >= climates.size()) return false;
  const auto& info = climates[static_cast<size_t>(index)];
  if (outNameBuffer && nameBufferSize > 0) {
    std::strncpy(outNameBuffer, info.name.c_str(), static_cast<size_t>(nameBufferSize - 1));
    outNameBuffer[nameBufferSize - 1] = '\0';
  }
  if (outToken) *outToken = info.token;
  return true;
}

void ClimateApi::T_Climate_SetClimate(uint64_t climateToken, bool instant) {
  ClimateService::GetInstance().SetClimate(climateToken, instant);
}

// ================================================================================================
// Section 3: Sun Profile
// ================================================================================================

int32_t ClimateApi::T_Climate_GetActiveSunProfileIndex() { return ClimateService::GetInstance().GetActiveSunProfileIndex(); }
int32_t ClimateApi::T_Climate_GetNextSunProfileIndex() { return ClimateService::GetInstance().GetNextSunProfileIndex(); }
int32_t ClimateApi::T_Climate_GetSunProfileCount(bool isBad) { return ClimateService::GetInstance().GetSunProfileCount(isBad); }

int ClimateApi::T_Climate_GetSunProfileName(int32_t index, bool isBad, char* outBuffer, int bufferSize) {
  auto name = ClimateService::GetInstance().GetSunProfileName(index, isBad);
  int len = static_cast<int>(name.length());
  if (outBuffer && bufferSize > 0) {
    std::strncpy(outBuffer, name.c_str(), static_cast<size_t>(bufferSize - 1));
    outBuffer[bufferSize - 1] = '\0';
  }
  return len;
}

float ClimateApi::T_Climate_GetSunProfileElevation(int32_t index) { return ClimateService::GetInstance().GetSunProfileElevation(index); }
float ClimateApi::T_Climate_GetTransitionProgress() { return ClimateService::GetInstance().GetTransitionProgress(); }
float ClimateApi::T_Climate_GetSunAngle() { return ClimateService::GetInstance().GetSunAngle(); }
float ClimateApi::T_Climate_GetWeatherBlendProgress() { return ClimateService::GetInstance().GetWeatherBlendProgress(); }
void ClimateApi::T_Climate_SetTransitionDuration(int32_t transitionDurationMinutes) { ClimateService::GetInstance().SetTransitionDuration(transitionDurationMinutes); }

// ================================================================================================
// Section 4: Weather Mode
// ================================================================================================

int32_t ClimateApi::T_Climate_GetWeatherMode() { return ClimateService::GetInstance().GetWeatherMode(); }
int32_t ClimateApi::T_Climate_GetNextWeatherMode() { return ClimateService::GetInstance().GetNextWeatherMode(); }
void ClimateApi::T_Climate_SetWeatherMode(int32_t mode, bool instant) { ClimateService::GetInstance().SetWeatherMode(mode, instant); }

// ================================================================================================
// Section 5: Bad Weather
// ================================================================================================

float ClimateApi::T_Climate_GetBadWeatherFactor() { return ClimateService::GetInstance().GetBadWeatherFactor(); }
void ClimateApi::T_Climate_SetBadWeatherFactor(float factor) { ClimateService::GetInstance().SetBadWeatherFactor(factor); }
uint32_t ClimateApi::T_Climate_GetBadWeatherMode() { return ClimateService::GetInstance().GetBadWeatherMode(); }
float ClimateApi::T_Climate_GetRemainingBadWeatherTime() { return ClimateService::GetInstance().GetRemainingBadWeatherTime(); }

// ================================================================================================
// Section 6: Env Profile
// ================================================================================================

float ClimateApi::T_Climate_GetLampsOnElevation() { return ClimateService::GetInstance().GetLampsOnElevation(); }
void ClimateApi::T_Climate_SetLampsOnElevation(float elevationDegrees) { ClimateService::GetInstance().SetLampsOnElevation(elevationDegrees); }
float ClimateApi::T_Climate_GetDayInYear() { return ClimateService::GetInstance().GetDayInYear(); }
void ClimateApi::T_Climate_SetDayInYear(float dayValue) { ClimateService::GetInstance().SetDayInYear(dayValue); }
float ClimateApi::T_Climate_GetSummerTime() { return ClimateService::GetInstance().GetSummerTime(); }
void ClimateApi::T_Climate_SetSummerTime(float offsetHours) { ClimateService::GetInstance().SetSummerTime(offsetHours); }
float ClimateApi::T_Climate_GetThunderstormProbability() { return ClimateService::GetInstance().GetThunderstormProbability(); }
void ClimateApi::T_Climate_SetThunderstormProbability(float probability) { ClimateService::GetInstance().SetThunderstormProbability(probability); }

// ================================================================================================
// Section 7: Profile Helpers, Elevation & Direction
// ================================================================================================

SPF_Climate_ProfileRef ClimateApi::T_Climate_ActiveProfile() {
  auto p = ClimateService::GetInstance().ActiveProfile();
  return {p.index, p.isBad};
}

SPF_Climate_ProfileRef ClimateApi::T_Climate_NextProfile() {
  auto p = ClimateService::GetInstance().NextProfile();
  return {p.index, p.isBad};
}

float ClimateApi::T_Climate_GetLowElevation(SPF_Climate_ProfileRef profile) {
  return ClimateService::GetInstance().GetLowElevation({profile.index, profile.isBad});
}

void ClimateApi::T_Climate_SetLowElevation(SPF_Climate_ProfileRef profile, float elevationDegrees) {
  ClimateService::GetInstance().SetLowElevation({profile.index, profile.isBad}, elevationDegrees);
}

float ClimateApi::T_Climate_GetHighElevation(SPF_Climate_ProfileRef profile) {
  return ClimateService::GetInstance().GetHighElevation({profile.index, profile.isBad});
}

void ClimateApi::T_Climate_SetHighElevation(SPF_Climate_ProfileRef profile, float elevationDegrees) {
  ClimateService::GetInstance().SetHighElevation({profile.index, profile.isBad}, elevationDegrees);
}

int32_t ClimateApi::T_Climate_GetSunDirection(SPF_Climate_ProfileRef profile) {
  return ClimateService::GetInstance().GetSunDirection({profile.index, profile.isBad});
}

void ClimateApi::T_Climate_SetSunDirection(SPF_Climate_ProfileRef profile, int32_t direction) {
  ClimateService::GetInstance().SetSunDirection({profile.index, profile.isBad}, direction);
}

// ================================================================================================
// Section 8: Variation Index
// ================================================================================================

uint64_t ClimateApi::T_Climate_GetActiveVariationIndex() { return ClimateService::GetInstance().GetActiveVariationIndex(); }
void ClimateApi::T_Climate_SetActiveVariationIndex(uint64_t variationIndex) { ClimateService::GetInstance().SetActiveVariationIndex(variationIndex); }
uint64_t ClimateApi::T_Climate_GetNextVariationIndex() { return ClimateService::GetInstance().GetNextVariationIndex(); }
void ClimateApi::T_Climate_SetNextVariationIndex(uint64_t variationIndex) { ClimateService::GetInstance().SetNextVariationIndex(variationIndex); }

}  // namespace Modules::API
SPF_NS_END
