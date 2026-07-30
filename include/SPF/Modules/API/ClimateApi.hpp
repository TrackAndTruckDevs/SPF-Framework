#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_Climate_API.h"

#include <cstdint>

#ifdef SPF_CL_DECL_FLOAT
#undef SPF_CL_DECL_FLOAT
#endif
#define SPF_CL_DECL_FLOAT(name)                                                                      \
  static uint64_t T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile);                         \
  static float T_Climate_Get##name(SPF_Climate_ProfileRef profile);                                   \
  static void T_Climate_Set##name(SPF_Climate_ProfileRef profile, float value);                        \
  static float T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx);             \
  static void T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, float value); \
  static float T_Climate_GetBlended##name();                                                           \
  static void T_Climate_SetBlended##name(float val, float min, float max);

#ifdef SPF_CL_DECL_INT32
#undef SPF_CL_DECL_INT32
#endif
#define SPF_CL_DECL_INT32(name)                                                                      \
  static uint64_t T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile);                         \
  static int32_t T_Climate_Get##name(SPF_Climate_ProfileRef profile);                                 \
  static void T_Climate_Set##name(SPF_Climate_ProfileRef profile, int32_t value);                      \
  static int32_t T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx);           \
  static void T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, int32_t value);

#ifdef SPF_CL_DECL_VECTOR3
#undef SPF_CL_DECL_VECTOR3
#endif
#define SPF_CL_DECL_VECTOR3(name)                                                                    \
  static uint64_t T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile);                         \
  static void T_Climate_Get##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3* out);           \
  static void T_Climate_Set##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector3 v);              \
  static void T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector3* out); \
  static void T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector3 v); \
  static void T_Climate_GetBlended##name(SPF_Climate_Vector3* out);                                    \
  static void T_Climate_SetBlended##name(SPF_Climate_Vector3 v, float maxComp);

#ifdef SPF_CL_DECL_VECTOR2
#undef SPF_CL_DECL_VECTOR2
#endif
#define SPF_CL_DECL_VECTOR2(name)                                                                    \
  static uint64_t T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile);                         \
  static void T_Climate_Get##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2* out);           \
  static void T_Climate_Set##name(SPF_Climate_ProfileRef profile, SPF_Climate_Vector2 v);              \
  static void T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector2* out); \
  static void T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, SPF_Climate_Vector2 v); \
  static void T_Climate_GetBlended##name(SPF_Climate_Vector2* out);                                    \
  static void T_Climate_SetBlended##name(SPF_Climate_Vector2 v, float maxComp);

#ifdef SPF_CL_DECL_TEXTURE
#undef SPF_CL_DECL_TEXTURE
#endif
#define SPF_CL_DECL_TEXTURE(name)                                                                    \
  static uint64_t T_Climate_Get##name##Count(SPF_Climate_ProfileRef profile);                         \
  static int T_Climate_Get##name(SPF_Climate_ProfileRef profile, char* buf, int sz);                   \
  static void T_Climate_Set##name(SPF_Climate_ProfileRef profile, const char* val);                    \
  static int T_Climate_Get##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, char* buf, int sz); \
  static void T_Climate_Set##name##ByIndex(SPF_Climate_ProfileRef profile, uint64_t idx, const char* val);

SPF_NS_BEGIN
namespace Modules::API {

/**
 * @brief Bridges the C climate API to the internal ClimateService singleton.
 *
 * @details All methods are static and act as thin wrappers that delegate to
 *          ClimateService::GetInstance(). The FillClimateApi() method populates
 *          an SPF_Climate_API struct with the correct function pointers.
 */
class ClimateApi {
 public:
  static void FillClimateApi(SPF_Climate_API* climate_api);

 private:
  // --- Section 1: Lifecycle ---
  static bool T_Climate_IsReady();
  static bool T_Climate_IsFinderReady(const char* finderName);
  static bool T_Climate_AreAllOffsetsFound();
  static bool T_Climate_RefreshOffsets();

  // --- Section 2: Climate Selection ---
  static int T_Climate_GetCurrentClimateName(char* outBuffer, int bufferSize);
  static int T_Climate_GetAvailableClimateCount();
  static bool T_Climate_GetAvailableClimateByIndex(int index, char* outNameBuffer, int nameBufferSize, uint64_t* outToken);
  static void T_Climate_SetClimate(uint64_t climateToken, bool instant);

  // --- Section 3: Sun Profile ---
  static int32_t T_Climate_GetActiveSunProfileIndex();
  static int32_t T_Climate_GetNextSunProfileIndex();
  static int32_t T_Climate_GetSunProfileCount(bool isBad);
  static int T_Climate_GetSunProfileName(int32_t index, bool isBad, char* outBuffer, int bufferSize);
  static float T_Climate_GetSunProfileElevation(int32_t index);
  static float T_Climate_GetTransitionProgress();
  static float T_Climate_GetSunAngle();
  static float T_Climate_GetWeatherBlendProgress();
  static void T_Climate_SetTransitionDuration(int32_t transitionDurationMinutes);

  // --- Section 4: Weather Mode ---
  static int32_t T_Climate_GetWeatherMode();
  static int32_t T_Climate_GetNextWeatherMode();
  static void T_Climate_SetWeatherMode(int32_t mode, bool instant);

  // --- Section 5: Bad Weather ---
  static float T_Climate_GetBadWeatherFactor();
  static void T_Climate_SetBadWeatherFactor(float factor);
  static uint32_t T_Climate_GetBadWeatherMode();
  static float T_Climate_GetRemainingBadWeatherTime();

  // --- Section 6: Env Profile ---
  static float T_Climate_GetLampsOnElevation();
  static void T_Climate_SetLampsOnElevation(float elevationDegrees);
  static float T_Climate_GetDayInYear();
  static void T_Climate_SetDayInYear(float dayValue);
  static float T_Climate_GetSummerTime();
  static void T_Climate_SetSummerTime(float offsetHours);
  static float T_Climate_GetThunderstormProbability();
  static void T_Climate_SetThunderstormProbability(float probability);

  // --- Section 7: Profile Helpers, Elevation & Direction ---
  static SPF_Climate_ProfileRef T_Climate_ActiveProfile();
  static SPF_Climate_ProfileRef T_Climate_NextProfile();
  static float T_Climate_GetLowElevation(SPF_Climate_ProfileRef profile);
  static void T_Climate_SetLowElevation(SPF_Climate_ProfileRef profile, float elevationDegrees);
  static float T_Climate_GetHighElevation(SPF_Climate_ProfileRef profile);
  static void T_Climate_SetHighElevation(SPF_Climate_ProfileRef profile, float elevationDegrees);
  static int32_t T_Climate_GetSunDirection(SPF_Climate_ProfileRef profile);
  static void T_Climate_SetSunDirection(SPF_Climate_ProfileRef profile, int32_t direction);

  // --- Section 8: Variation Index ---
  static uint64_t T_Climate_GetActiveVariationIndex();
  static void T_Climate_SetActiveVariationIndex(uint64_t variationIndex);
  static uint64_t T_Climate_GetNextVariationIndex();
  static void T_Climate_SetNextVariationIndex(uint64_t variationIndex);

  // ---------------------------------------------------------------------------
  // Sections 9-13: Variant Attributes (declared via macros for conciseness)
  // ---------------------------------------------------------------------------

  // --- Section 9: Float variants (41) ---
  SPF_CL_DECL_FLOAT(Temperature)
  SPF_CL_DECL_FLOAT(SunOpacity)
  SPF_CL_DECL_FLOAT(SunShadowStrength)
  SPF_CL_DECL_FLOAT(MoonHaloScale)
  SPF_CL_DECL_FLOAT(FogVgradient)
  SPF_CL_DECL_FLOAT(FogOffset)
  SPF_CL_DECL_FLOAT(FogDensity)
  SPF_CL_DECL_FLOAT(SpeedCoef)
  SPF_CL_DECL_FLOAT(CloudShadowWeight)
  SPF_CL_DECL_FLOAT(RainIntensity)
  SPF_CL_DECL_FLOAT(LightningIntensity)
  SPF_CL_DECL_FLOAT(RainMaxWetness)
  SPF_CL_DECL_FLOAT(RainAdditionalAmbient)
  SPF_CL_DECL_FLOAT(SnowIntensity)
  SPF_CL_DECL_FLOAT(SnowChaosRate)
  SPF_CL_DECL_FLOAT(SnowChaosWeight)
  SPF_CL_DECL_FLOAT(SnowAdditionalAmbient)
  SPF_CL_DECL_FLOAT(DofStart)
  SPF_CL_DECL_FLOAT(DofTransition)
  SPF_CL_DECL_FLOAT(DofFilterSize)
  SPF_CL_DECL_FLOAT(ColorBalance)
  SPF_CL_DECL_FLOAT(ColorSaturation)
  SPF_CL_DECL_FLOAT(SunshaftSize)
  SPF_CL_DECL_FLOAT(LowIntensityMinimum)
  SPF_CL_DECL_FLOAT(LowIntensityMaximum)
  SPF_CL_DECL_FLOAT(DarkAdaptationSpeed)
  SPF_CL_DECL_FLOAT(BrightAdaptationSpeed)
  SPF_CL_DECL_FLOAT(TargetGray)
  SPF_CL_DECL_FLOAT(MinScale)
  SPF_CL_DECL_FLOAT(MaxScale)
  SPF_CL_DECL_FLOAT(ScaleOverride)
  SPF_CL_DECL_FLOAT(Contrast)
  SPF_CL_DECL_FLOAT(ShoulderLength)
  SPF_CL_DECL_FLOAT(BloomThreshold)
  SPF_CL_DECL_FLOAT(BloomLimit)
  SPF_CL_DECL_FLOAT(BloomIntensity)
  SPF_CL_DECL_FLOAT(BloomStandardDeviation)
  SPF_CL_DECL_FLOAT(Stability)
  SPF_CL_DECL_FLOAT(MirrorSkyTexture)
  SPF_CL_DECL_FLOAT(Env)
  SPF_CL_DECL_FLOAT(EnvStaticMod)

  // --- Section 10: Int32 variants (2) ---
  SPF_CL_DECL_INT32(Weight)
  SPF_CL_DECL_INT32(WindType)

  // --- Section 11: Vector3 variants (15) ---
  SPF_CL_DECL_VECTOR3(Ambient)
  SPF_CL_DECL_VECTOR3(Diffuse)
  SPF_CL_DECL_VECTOR3(Specular)
  SPF_CL_DECL_VECTOR3(SkyColor)
  SPF_CL_DECL_VECTOR3(SkyBottomColor)
  SPF_CL_DECL_VECTOR3(StarmapColor)
  SPF_CL_DECL_VECTOR3(StarsColor)
  SPF_CL_DECL_VECTOR3(SunColor)
  SPF_CL_DECL_VECTOR3(SunHaloColor)
  SPF_CL_DECL_VECTOR3(MoonColor)
  SPF_CL_DECL_VECTOR3(MoonHaloColor)
  SPF_CL_DECL_VECTOR3(FogColor)
  SPF_CL_DECL_VECTOR3(FogColor2)
  SPF_CL_DECL_VECTOR3(SunshaftColor)
  SPF_CL_DECL_VECTOR3(LowIntensityColor)

  // --- Section 12: Vector2 variants (3) ---
  SPF_CL_DECL_VECTOR2(CloudShadowAreaSize)
  SPF_CL_DECL_VECTOR2(CloudShadowSpeed)
  SPF_CL_DECL_VECTOR2(SnowFlakeSizeRange)

  // --- Section 13: Texture variants (5) ---
  SPF_CL_DECL_TEXTURE(SkyboxTexture)
  SPF_CL_DECL_TEXTURE(SkycloudMaskTexture)
  SPF_CL_DECL_TEXTURE(LightningMask)
  SPF_CL_DECL_TEXTURE(StarsTexture)
  SPF_CL_DECL_TEXTURE(CloudShadowTexture)
};

}  // namespace Modules::API
SPF_NS_END
