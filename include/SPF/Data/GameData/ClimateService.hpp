#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include "SPF/Utils/Vec2.hpp"
#include "SPF/Utils/Vec3.hpp"

#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

// Forward declarations
class IClimateDataFinder;

struct ProfileRef {
  uint64_t index;
  bool isBad;  // false = nice, true = bad
};

class ClimateService : public IWorldScopedService {
 public:
  static ClimateService& GetInstance();

  ClimateService(const ClimateService&) = delete;
  void operator=(const ClimateService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady();
  bool IsFinderReady(const char* name) const;
  bool AreAllFindersReady() const;
  bool TryFindAllOffsets();

  // --- IWorldScopedService ---
  const char* GetName() const override { return "ClimateService"; }
  void ResetForWorldReload() override { Shutdown(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }

  // --- Public Getters for Environment ---
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- Public Setters (for finders) ---
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }

  // --- Bad Weather (for finders) ---
  void SetBadWeatherFactorPtr(uintptr_t val) { m_badWeatherFactorPtr = val; }
  void SetRemainingBadWeatherOffset(intptr_t val) { m_remainingBadWeatherOffset = val; }

  // --- Weather and Environment Setters (for finders) ---
  void SetWeatherModeOffset(intptr_t val) { m_weatherModeOffset = val; }
  void SetNextWeatherModeOffset(intptr_t val) { m_nextWeatherModeOffset = val; }
  void SetClimatePtrOffset(intptr_t val) { m_climatePtrOffset = val; }
  void SetClimateUnitIdOffset(intptr_t val) { m_climateUnitIdOffset = val; }
  void SetClimateArrayOffset(intptr_t val) { m_climateArrayOffset = val; }
  void SetClimateCountOffset(intptr_t val) { m_climateCountOffset = val; }
  void SetSetWeatherModeFnAddr(uintptr_t addr) { m_setWeatherModeFnAddr = addr; }
  void SetSetClimateFnAddr(uintptr_t addr) { m_setClimateFnAddr = addr; }
  void SetActiveProfileIndexOffset(intptr_t val) { m_activeProfileIndexOffset = val; }
  void SetNextProfileIndexOffset(intptr_t val) { m_nextProfileIndexOffset = val; }

  void SetContainerNiceOffset(intptr_t val) { m_containerNiceOffset = val; }
  void SetContainerBadOffset(intptr_t val) { m_containerBadOffset = val; }
  void SetProfilesArrayOffset(intptr_t val) { m_profilesArrayOffset = val; }
  void SetContainerCountOffset(intptr_t val) { m_containerCountOffset = val; }
  void SetSunAngleOffset(intptr_t val) { m_sunAngleOffset = val; }
  void SetWeatherBlendProgressFnAddr(uintptr_t addr) { m_weatherBlendFnAddr = addr; }
  void SetTransitionDurationAddr(uintptr_t addr) { m_transitionDurationAddr = addr; }

  // --- Core Getters (for finders) ---
  uintptr_t GetBadWeatherFactorPtr() const { return m_badWeatherFactorPtr; }
  intptr_t GetRemainingBadWeatherOffset() const { return m_remainingBadWeatherOffset; }
  intptr_t GetWeatherModeOffset() const { return m_weatherModeOffset; }
  intptr_t GetNextWeatherModeOffset() const { return m_nextWeatherModeOffset; }
  intptr_t GetClimatePtrOffset() const { return m_climatePtrOffset; }
  intptr_t GetClimateUnitIdOffset() const { return m_climateUnitIdOffset; }
  intptr_t GetClimateArrayOffset() const { return m_climateArrayOffset; }
  intptr_t GetClimateCountOffset() const { return m_climateCountOffset; }
  uintptr_t GetSetWeatherModeFnAddr() const { return m_setWeatherModeFnAddr; }
  uintptr_t GetSetClimateFnAddr() const { return m_setClimateFnAddr; }
  intptr_t GetActiveProfileIndexOffset() const { return m_activeProfileIndexOffset; }
  intptr_t GetNextProfileIndexOffset() const { return m_nextProfileIndexOffset; }
  intptr_t GetContainerNiceOffset() const { return m_containerNiceOffset; }
  intptr_t GetContainerBadOffset() const { return m_containerBadOffset; }
  intptr_t GetProfilesArrayOffset() const { return m_profilesArrayOffset; }
  intptr_t GetContainerCountOffset() const { return m_containerCountOffset; }
  intptr_t GetSunAngleOffset() const { return m_sunAngleOffset; }
  uintptr_t GetWeatherBlendProgressFnAddr() const { return m_weatherBlendFnAddr; }
  uintptr_t GetTransitionDurationAddr() const { return m_transitionDurationAddr; }

  // --- Sun Profile Reflection Attribute Setters (for finders) ---
  void SetLowElevationOffset(intptr_t val) { m_lowElevationOffset = val; }
  intptr_t GetLowElevationOffset() const { return m_lowElevationOffset; }
  void SetHighElevationOffset(intptr_t val) { m_highElevationOffset = val; }
  intptr_t GetHighElevationOffset() const { return m_highElevationOffset; }
  void SetSunDirectionOffset(intptr_t val) { m_sunDirectionOffset = val; }
  intptr_t GetSunDirectionOffset() const { return m_sunDirectionOffset; }

  void SetTemperatureOffset(intptr_t val) { m_temperatureOffset = val; }
  intptr_t GetTemperatureOffset() const { return m_temperatureOffset; }
  void SetSkyboxTextureOffset(intptr_t val) { m_skyboxTextureOffset = val; }
  intptr_t GetSkyboxTextureOffset() const { return m_skyboxTextureOffset; }
  void SetSkycloudMaskTextureOffset(intptr_t val) { m_skycloudMaskTextureOffset = val; }
  intptr_t GetSkycloudMaskTextureOffset() const { return m_skycloudMaskTextureOffset; }
  void SetLightningMaskOffset(intptr_t val) { m_lightningMaskOffset = val; }
  intptr_t GetLightningMaskOffset() const { return m_lightningMaskOffset; }
  void SetStarsTextureOffset(intptr_t val) { m_starsTextureOffset = val; }
  intptr_t GetStarsTextureOffset() const { return m_starsTextureOffset; }
  void SetMirrorSkyTextureOffset(intptr_t val) { m_mirrorSkyTextureOffset = val; }
  intptr_t GetMirrorSkyTextureOffset() const { return m_mirrorSkyTextureOffset; }
  void SetAmbientOffset(intptr_t val) { m_ambientOffset = val; }
  intptr_t GetAmbientOffset() const { return m_ambientOffset; }
  void SetDiffuseOffset(intptr_t val) { m_diffuseOffset = val; }
  intptr_t GetDiffuseOffset() const { return m_diffuseOffset; }
  void SetSpecularOffset(intptr_t val) { m_specularOffset = val; }
  intptr_t GetSpecularOffset() const { return m_specularOffset; }
  void SetEnvOffset(intptr_t val) { m_envOffset = val; }
  intptr_t GetEnvOffset() const { return m_envOffset; }
  void SetEnvStaticModOffset(intptr_t val) { m_envStaticModOffset = val; }
  intptr_t GetEnvStaticModOffset() const { return m_envStaticModOffset; }
  void SetSkyColorOffset(intptr_t val) { m_skyColorOffset = val; }
  intptr_t GetSkyColorOffset() const { return m_skyColorOffset; }
  void SetSkyBottomColorOffset(intptr_t val) { m_skyBottomColorOffset = val; }
  intptr_t GetSkyBottomColorOffset() const { return m_skyBottomColorOffset; }
  void SetStarmapColorOffset(intptr_t val) { m_starmapColorOffset = val; }
  intptr_t GetStarmapColorOffset() const { return m_starmapColorOffset; }
  void SetStarsColorOffset(intptr_t val) { m_starsColorOffset = val; }
  intptr_t GetStarsColorOffset() const { return m_starsColorOffset; }
  void SetSunColorOffset(intptr_t val) { m_sunColorOffset = val; }
  intptr_t GetSunColorOffset() const { return m_sunColorOffset; }
  void SetSunOpacityOffset(intptr_t val) { m_sunOpacityOffset = val; }
  intptr_t GetSunOpacityOffset() const { return m_sunOpacityOffset; }
  void SetSunHaloColorOffset(intptr_t val) { m_sunHaloColorOffset = val; }
  intptr_t GetSunHaloColorOffset() const { return m_sunHaloColorOffset; }
  void SetSunShadowStrengthOffset(intptr_t val) { m_sunShadowStrengthOffset = val; }
  intptr_t GetSunShadowStrengthOffset() const { return m_sunShadowStrengthOffset; }
  void SetMoonColorOffset(intptr_t val) { m_moonColorOffset = val; }
  intptr_t GetMoonColorOffset() const { return m_moonColorOffset; }
  void SetMoonHaloColorOffset(intptr_t val) { m_moonHaloColorOffset = val; }
  intptr_t GetMoonHaloColorOffset() const { return m_moonHaloColorOffset; }
  void SetMoonHaloScaleOffset(intptr_t val) { m_moonHaloScaleOffset = val; }
  intptr_t GetMoonHaloScaleOffset() const { return m_moonHaloScaleOffset; }
  void SetFogColorOffset(intptr_t val) { m_fogColorOffset = val; }
  intptr_t GetFogColorOffset() const { return m_fogColorOffset; }
  void SetFogColor2Offset(intptr_t val) { m_fogColor2Offset = val; }
  intptr_t GetFogColor2Offset() const { return m_fogColor2Offset; }
  void SetFogVgradientOffset(intptr_t val) { m_fogVgradientOffset = val; }
  intptr_t GetFogVgradientOffset() const { return m_fogVgradientOffset; }
  void SetFogOffsetOffset(intptr_t val) { m_fogOffsetOffset = val; }
  intptr_t GetFogOffsetOffset() const { return m_fogOffsetOffset; }
  void SetFogDensityOffset(intptr_t val) { m_fogDensityOffset = val; }
  intptr_t GetFogDensityOffset() const { return m_fogDensityOffset; }
  void SetSpeedCoefOffset(intptr_t val) { m_speedCoefOffset = val; }
  intptr_t GetSpeedCoefOffset() const { return m_speedCoefOffset; }
  void SetCloudShadowWeightOffset(intptr_t val) { m_cloudShadowWeightOffset = val; }
  intptr_t GetCloudShadowWeightOffset() const { return m_cloudShadowWeightOffset; }
  void SetCloudShadowTextureOffset(intptr_t val) { m_cloudShadowTextureOffset = val; }
  intptr_t GetCloudShadowTextureOffset() const { return m_cloudShadowTextureOffset; }
  void SetCloudShadowAreaSizeOffset(intptr_t val) { m_cloudShadowAreaSizeOffset = val; }
  intptr_t GetCloudShadowAreaSizeOffset() const { return m_cloudShadowAreaSizeOffset; }
  void SetCloudShadowSpeedOffset(intptr_t val) { m_cloudShadowSpeedOffset = val; }
  intptr_t GetCloudShadowSpeedOffset() const { return m_cloudShadowSpeedOffset; }
  void SetRainIntensityOffset(intptr_t val) { m_rainIntensityOffset = val; }
  intptr_t GetRainIntensityOffset() const { return m_rainIntensityOffset; }
  void SetLightningIntensityOffset(intptr_t val) { m_lightningIntensityOffset = val; }
  intptr_t GetLightningIntensityOffset() const { return m_lightningIntensityOffset; }
  void SetRainMaxWetnessOffset(intptr_t val) { m_rainMaxWetnessOffset = val; }
  intptr_t GetRainMaxWetnessOffset() const { return m_rainMaxWetnessOffset; }
  void SetRainAdditionalAmbientOffset(intptr_t val) { m_rainAdditionalAmbientOffset = val; }
  intptr_t GetRainAdditionalAmbientOffset() const { return m_rainAdditionalAmbientOffset; }
  void SetSnowIntensityOffset(intptr_t val) { m_snowIntensityOffset = val; }
  intptr_t GetSnowIntensityOffset() const { return m_snowIntensityOffset; }
  void SetSnowFlakeSizeRangeOffset(intptr_t val) { m_snowFlakeSizeRangeOffset = val; }
  intptr_t GetSnowFlakeSizeRangeOffset() const { return m_snowFlakeSizeRangeOffset; }
  void SetSnowChaosRateOffset(intptr_t val) { m_snowChaosRateOffset = val; }
  intptr_t GetSnowChaosRateOffset() const { return m_snowChaosRateOffset; }
  void SetSnowChaosWeightOffset(intptr_t val) { m_snowChaosWeightOffset = val; }
  intptr_t GetSnowChaosWeightOffset() const { return m_snowChaosWeightOffset; }
  void SetSnowAdditionalAmbientOffset(intptr_t val) { m_snowAdditionalAmbientOffset = val; }
  intptr_t GetSnowAdditionalAmbientOffset() const { return m_snowAdditionalAmbientOffset; }
  void SetWindTypeOffset(intptr_t val) { m_windTypeOffset = val; }
  intptr_t GetWindTypeOffset() const { return m_windTypeOffset; }
  void SetDofStartOffset(intptr_t val) { m_dofStartOffset = val; }
  intptr_t GetDofStartOffset() const { return m_dofStartOffset; }
  void SetDofTransitionOffset(intptr_t val) { m_dofTransitionOffset = val; }
  intptr_t GetDofTransitionOffset() const { return m_dofTransitionOffset; }
  void SetDofFilterSizeOffset(intptr_t val) { m_dofFilterSizeOffset = val; }
  intptr_t GetDofFilterSizeOffset() const { return m_dofFilterSizeOffset; }
  void SetColorBalanceOffset(intptr_t val) { m_colorBalanceOffset = val; }
  intptr_t GetColorBalanceOffset() const { return m_colorBalanceOffset; }
  void SetColorSaturationOffset(intptr_t val) { m_colorSaturationOffset = val; }
  intptr_t GetColorSaturationOffset() const { return m_colorSaturationOffset; }
  void SetSunshaftColorOffset(intptr_t val) { m_sunshaftColorOffset = val; }
  intptr_t GetSunshaftColorOffset() const { return m_sunshaftColorOffset; }
  void SetSunshaftSizeOffset(intptr_t val) { m_sunshaftSizeOffset = val; }
  intptr_t GetSunshaftSizeOffset() const { return m_sunshaftSizeOffset; }
  void SetLowIntensityMinimumOffset(intptr_t val) { m_lowIntensityMinimumOffset = val; }
  intptr_t GetLowIntensityMinimumOffset() const { return m_lowIntensityMinimumOffset; }
  void SetLowIntensityMaximumOffset(intptr_t val) { m_lowIntensityMaximumOffset = val; }
  intptr_t GetLowIntensityMaximumOffset() const { return m_lowIntensityMaximumOffset; }
  void SetLowIntensityColorOffset(intptr_t val) { m_lowIntensityColorOffset = val; }
  intptr_t GetLowIntensityColorOffset() const { return m_lowIntensityColorOffset; }
  void SetMinScaleOffset(intptr_t val) { m_minScaleOffset = val; }
  intptr_t GetMinScaleOffset() const { return m_minScaleOffset; }
  void SetMaxScaleOffset(intptr_t val) { m_maxScaleOffset = val; }
  intptr_t GetMaxScaleOffset() const { return m_maxScaleOffset; }
  void SetScaleOverrideOffset(intptr_t val) { m_scaleOverrideOffset = val; }
  intptr_t GetScaleOverrideOffset() const { return m_scaleOverrideOffset; }
  void SetDarkAdaptationSpeedOffset(intptr_t val) { m_darkAdaptationSpeedOffset = val; }
  intptr_t GetDarkAdaptationSpeedOffset() const { return m_darkAdaptationSpeedOffset; }
  void SetBrightAdaptationSpeedOffset(intptr_t val) { m_brightAdaptationSpeedOffset = val; }
  intptr_t GetBrightAdaptationSpeedOffset() const { return m_brightAdaptationSpeedOffset; }
  void SetTargetGrayOffset(intptr_t val) { m_targetGrayOffset = val; }
  intptr_t GetTargetGrayOffset() const { return m_targetGrayOffset; }
  void SetContrastOffset(intptr_t val) { m_contrastOffset = val; }
  intptr_t GetContrastOffset() const { return m_contrastOffset; }
  void SetShoulderLengthOffset(intptr_t val) { m_shoulderLengthOffset = val; }
  intptr_t GetShoulderLengthOffset() const { return m_shoulderLengthOffset; }
  void SetBloomThresholdOffset(intptr_t val) { m_bloomThresholdOffset = val; }
  intptr_t GetBloomThresholdOffset() const { return m_bloomThresholdOffset; }
  void SetBloomLimitOffset(intptr_t val) { m_bloomLimitOffset = val; }
  intptr_t GetBloomLimitOffset() const { return m_bloomLimitOffset; }
  void SetBloomIntensityOffset(intptr_t val) { m_bloomIntensityOffset = val; }
  intptr_t GetBloomIntensityOffset() const { return m_bloomIntensityOffset; }
  void SetBloomStandardDeviationOffset(intptr_t val) { m_bloomStandardDeviationOffset = val; }
  intptr_t GetBloomStandardDeviationOffset() const { return m_bloomStandardDeviationOffset; }
  void SetStabilityOffset(intptr_t val) { m_stabilityOffset = val; }
  intptr_t GetStabilityOffset() const { return m_stabilityOffset; }
  void SetWeightOffset(intptr_t val) { m_weightOffset = val; }
  intptr_t GetWeightOffset() const { return m_weightOffset; }

  // --- Env Profile Reflection Attribute Setters (for finders) ---
  void SetLampsOnElevationOffset(intptr_t val) { m_lampsOnElevationOffset = val; }
  intptr_t GetLampsOnElevationOffset() const { return m_lampsOnElevationOffset; }
  void SetDayInYearOffset(intptr_t val) { m_dayInYearOffset = val; }
  intptr_t GetDayInYearOffset() const { return m_dayInYearOffset; }
  void SetSummerTimeOffset(intptr_t val) { m_summerTimeOffset = val; }
  intptr_t GetSummerTimeOffset() const { return m_summerTimeOffset; }
  void SetThunderstormProbabilityOffset(intptr_t val) { m_thunderstormProbabilityOffset = val; }
  intptr_t GetThunderstormProbabilityOffset() const { return m_thunderstormProbabilityOffset; }
  void SetEnvProfilePtrOffset(intptr_t val) { m_envProfilePtrOffset = val; }
  intptr_t GetEnvProfilePtrOffset() const { return m_envProfilePtrOffset; }

  // --- Bad Weather Factor & Timer ---
  float GetBadWeatherFactor();
  void SetBadWeatherFactor(float val);
  uint32_t GetBadWeatherMode();
  float GetRemainingBadWeatherTime();

  // --- Weather & Environment methods ---
  int32_t GetWeatherMode();
  int32_t GetNextWeatherMode();
  void SetNextWeatherMode(int32_t mode);
  void SetWeatherMode(int32_t mode, bool instant = true);

  // ─── ProfileRef Helpers ──────────────────────────────────
  ProfileRef ActiveProfile();
  ProfileRef NextProfile();
  static ProfileRef Profile(uint64_t index, bool isBad);

  // ─── Sun Profile Attribute Macros (ProfileRef only — no overloads) ─────
#define SPF_FLOAT_VAR_ATTR(api, member) \
  uint64_t Get##api##Count(ProfileRef prof) { return GetProfileCount(m_##member##Offset, prof); } \
  float Get##api(ProfileRef prof) { return GetProfileFloat(m_##member##Offset, prof); } \
  void Set##api(ProfileRef prof, float val) { SetProfileFloat(m_##member##Offset, prof, val); } \
  float Get##api##ByIndex(ProfileRef prof, uint64_t varIdx) { return GetProfileFloatByIndex(m_##member##Offset, prof, varIdx); } \
  void Set##api##ByIndex(ProfileRef prof, uint64_t varIdx, float val) { SetProfileFloatByIndex(m_##member##Offset, prof, varIdx, val); } \
  float GetBlended##api() { return GetBlendedFloat(m_##member##Offset); } \
  void SetBlended##api(float val, float minVal, float maxVal) { SetBlendedFloat(m_##member##Offset, val, minVal, maxVal); }

#define SPF_VEC3_ATTR(api, member) \
  uint64_t Get##api##Count(ProfileRef prof) { return GetProfileCount(m_##member##Offset, prof); } \
  Utils::Vector3 Get##api(ProfileRef prof) { return GetProfileVec3(m_##member##Offset, prof); } \
  void Set##api(ProfileRef prof, const Utils::Vector3& val) { SetProfileVec3(m_##member##Offset, prof, val); } \
  Utils::Vector3 Get##api##ByIndex(ProfileRef prof, uint64_t varIdx) { return GetProfileVec3ByIndex(m_##member##Offset, prof, varIdx); } \
  void Set##api##ByIndex(ProfileRef prof, uint64_t varIdx, const Utils::Vector3& val) { SetProfileVec3ByIndex(m_##member##Offset, prof, varIdx, val); } \
  Utils::Vector3 GetBlended##api() { return GetBlendedVec3(m_##member##Offset); } \
  void SetBlended##api(const Utils::Vector3& val, float maxComponent) { SetBlendedVec3(m_##member##Offset, val, maxComponent); }

#define SPF_INT_VAR_ATTR(api, member) \
  uint64_t Get##api##Count(ProfileRef prof) { return GetProfileCount(m_##member##Offset, prof); } \
  int32_t Get##api(ProfileRef prof) { return GetProfileInt(m_##member##Offset, prof); } \
  void Set##api(ProfileRef prof, int32_t val) { SetProfileInt(m_##member##Offset, prof, val); } \
  int32_t Get##api##ByIndex(ProfileRef prof, uint64_t varIdx) { return GetProfileIntByIndex(m_##member##Offset, prof, varIdx); } \
  void Set##api##ByIndex(ProfileRef prof, uint64_t varIdx, int32_t val) { SetProfileIntByIndex(m_##member##Offset, prof, varIdx, val); }

#define SPF_TEXTURE_ATTR(api, member) \
  uint64_t Get##api##Count(ProfileRef prof) { return GetProfileCount(m_##member##Offset, prof); } \
  std::string Get##api(ProfileRef prof) { return GetProfileTexture(m_##member##Offset, prof); } \
  void Set##api(ProfileRef prof, const std::string& val) { SetProfileTexture(m_##member##Offset, prof, val); } \
  std::string Get##api##ByIndex(ProfileRef prof, uint64_t varIdx) { return GetProfileTextureByIndex(m_##member##Offset, prof, varIdx); } \
  void Set##api##ByIndex(ProfileRef prof, uint64_t varIdx, const std::string& val) { SetProfileTextureByIndex(m_##member##Offset, prof, varIdx, val); }

#define SPF_VEC2_ATTR(api, member) \
  uint64_t Get##api##Count(ProfileRef prof) { return GetProfileCount(m_##member##Offset, prof); } \
  Utils::Vec2f Get##api(ProfileRef prof) { return GetProfileVec2(m_##member##Offset, prof); } \
  void Set##api(ProfileRef prof, const Utils::Vec2f& val) { SetProfileVec2(m_##member##Offset, prof, val); } \
  Utils::Vec2f Get##api##ByIndex(ProfileRef prof, uint64_t varIdx) { return GetProfileVec2ByIndex(m_##member##Offset, prof, varIdx); } \
  void Set##api##ByIndex(ProfileRef prof, uint64_t varIdx, const Utils::Vec2f& val) { SetProfileVec2ByIndex(m_##member##Offset, prof, varIdx, val); } \
  Utils::Vec2f GetBlended##api() { return GetBlendedVec2(m_##member##Offset); } \
  void SetBlended##api(const Utils::Vec2f& val, float maxComponent) { SetBlendedVec2(m_##member##Offset, val, maxComponent); }

  // --- Scalar (raw radians in memory, API returns degrees; sun_direction is int32_t -1/0/1) ---
  float GetLowElevation(ProfileRef prof);
  void SetLowElevation(ProfileRef prof, float deg);
  float GetHighElevation(ProfileRef prof);
  void SetHighElevation(ProfileRef prof, float deg);
  int32_t GetSunDirection(ProfileRef prof);
  void SetSunDirection(ProfileRef prof, int32_t val);

  // --- Float with variations ---
  SPF_FLOAT_VAR_ATTR(Temperature, temperature)
  SPF_FLOAT_VAR_ATTR(SunOpacity, sunOpacity)
  SPF_FLOAT_VAR_ATTR(SunShadowStrength, sunShadowStrength)
  SPF_FLOAT_VAR_ATTR(MoonHaloScale, moonHaloScale)
  SPF_FLOAT_VAR_ATTR(FogVgradient, fogVgradient)
  SPF_FLOAT_VAR_ATTR(FogOffset, fogOffset)
  SPF_FLOAT_VAR_ATTR(FogDensity, fogDensity)
  SPF_FLOAT_VAR_ATTR(SpeedCoef, speedCoef)
  SPF_FLOAT_VAR_ATTR(CloudShadowWeight, cloudShadowWeight)
  SPF_FLOAT_VAR_ATTR(RainIntensity, rainIntensity)
  SPF_FLOAT_VAR_ATTR(LightningIntensity, lightningIntensity)
  SPF_FLOAT_VAR_ATTR(RainMaxWetness, rainMaxWetness)
  SPF_FLOAT_VAR_ATTR(RainAdditionalAmbient, rainAdditionalAmbient)
  SPF_FLOAT_VAR_ATTR(SnowIntensity, snowIntensity)
  SPF_FLOAT_VAR_ATTR(SnowChaosRate, snowChaosRate)
  SPF_FLOAT_VAR_ATTR(SnowChaosWeight, snowChaosWeight)
  SPF_FLOAT_VAR_ATTR(SnowAdditionalAmbient, snowAdditionalAmbient)
  SPF_FLOAT_VAR_ATTR(DofStart, dofStart)
  SPF_FLOAT_VAR_ATTR(DofTransition, dofTransition)
  SPF_FLOAT_VAR_ATTR(DofFilterSize, dofFilterSize)
  SPF_FLOAT_VAR_ATTR(ColorBalance, colorBalance)
  SPF_FLOAT_VAR_ATTR(ColorSaturation, colorSaturation)
  SPF_FLOAT_VAR_ATTR(SunshaftSize, sunshaftSize)
  SPF_FLOAT_VAR_ATTR(LowIntensityMinimum, lowIntensityMinimum)
  SPF_FLOAT_VAR_ATTR(LowIntensityMaximum, lowIntensityMaximum)
  SPF_FLOAT_VAR_ATTR(MinScale, minScale)
  SPF_FLOAT_VAR_ATTR(MaxScale, maxScale)
  SPF_FLOAT_VAR_ATTR(ScaleOverride, scaleOverride)
  SPF_FLOAT_VAR_ATTR(DarkAdaptationSpeed, darkAdaptationSpeed)
  SPF_FLOAT_VAR_ATTR(BrightAdaptationSpeed, brightAdaptationSpeed)
  SPF_FLOAT_VAR_ATTR(TargetGray, targetGray)
  SPF_FLOAT_VAR_ATTR(Contrast, contrast)
  SPF_FLOAT_VAR_ATTR(ShoulderLength, shoulderLength)
  SPF_FLOAT_VAR_ATTR(BloomThreshold, bloomThreshold)
  SPF_FLOAT_VAR_ATTR(BloomLimit, bloomLimit)
  SPF_FLOAT_VAR_ATTR(BloomIntensity, bloomIntensity)
  SPF_FLOAT_VAR_ATTR(BloomStandardDeviation, bloomStandardDeviation)
  SPF_FLOAT_VAR_ATTR(Stability, stability)
  SPF_FLOAT_VAR_ATTR(MirrorSkyTexture, mirrorSkyTexture)
  SPF_FLOAT_VAR_ATTR(Env, env)
  SPF_FLOAT_VAR_ATTR(EnvStaticMod, envStaticMod)

  // --- Int ---
  SPF_INT_VAR_ATTR(Weight, weight)
  SPF_INT_VAR_ATTR(WindType, windType)

   // --- Vec2 ---
  SPF_VEC2_ATTR(CloudShadowAreaSize, cloudShadowAreaSize)
  SPF_VEC2_ATTR(CloudShadowSpeed, cloudShadowSpeed)
  SPF_VEC2_ATTR(SnowFlakeSizeRange, snowFlakeSizeRange)

  // --- Vec3 ---
  SPF_VEC3_ATTR(Ambient, ambient)
  SPF_VEC3_ATTR(Diffuse, diffuse)
  SPF_VEC3_ATTR(Specular, specular)
  SPF_VEC3_ATTR(SkyColor, skyColor)
  SPF_VEC3_ATTR(SkyBottomColor, skyBottomColor)
  SPF_VEC3_ATTR(StarmapColor, starmapColor)
  SPF_VEC3_ATTR(StarsColor, starsColor)
  SPF_VEC3_ATTR(SunColor, sunColor)
  SPF_VEC3_ATTR(SunHaloColor, sunHaloColor)
  SPF_VEC3_ATTR(MoonColor, moonColor)
  SPF_VEC3_ATTR(MoonHaloColor, moonHaloColor)
  SPF_VEC3_ATTR(FogColor, fogColor)
  SPF_VEC3_ATTR(FogColor2, fogColor2)
  SPF_VEC3_ATTR(SunshaftColor, sunshaftColor)
  SPF_VEC3_ATTR(LowIntensityColor, lowIntensityColor)

  // --- Texture ---
  SPF_TEXTURE_ATTR(SkyboxTexture, skyboxTexture)
  SPF_TEXTURE_ATTR(SkycloudMaskTexture, skycloudMaskTexture)
  SPF_TEXTURE_ATTR(LightningMask, lightningMask)
  SPF_TEXTURE_ATTR(StarsTexture, starsTexture)
  SPF_TEXTURE_ATTR(CloudShadowTexture, cloudShadowTexture)

  uint64_t GetActiveVariationIndex();
  void SetActiveVariationIndex(uint64_t varIdx);
  uint64_t GetNextVariationIndex();
  void SetNextVariationIndex(uint64_t varIdx);

  struct ClimateInfo {
    std::string name;
    uint64_t token;
  };

  std::string GetCurrentClimateName();
  std::vector<ClimateInfo> GetAvailableClimates();
  void SetClimate(uint64_t token, bool instant = true);

  // --- Sun Profile API ---
  int32_t GetActiveSunProfileIndex();
  int32_t GetNextSunProfileIndex();
  int32_t GetSunProfileCount(bool isBad);
  std::string GetSunProfileName(int32_t index, bool isBad);
  float GetSunProfileElevation(int32_t index);
  float GetTransitionProgress();

  float GetSunAngle();
  float GetWeatherBlendProgress();
  void SetTransitionDuration(int32_t minutes);

  void DumpVec3ToLog(intptr_t offset, const char* name);

  // --- Env Profile API (env-level, no ProfileRef needed) ---
  float GetLampsOnElevation();
  void SetLampsOnElevation(float val);
  float GetDayInYear();
  void SetDayInYear(float val);
  float GetSummerTime();
  void SetSummerTime(float val);
  float GetThunderstormProbability();
  void SetThunderstormProbability(float val);

 private:
  ClimateService();
  ~ClimateService() = default;

  void RegisterFinders();
  uintptr_t GetClimateContainer(bool isBad);
  uintptr_t GetCurrentClimateContainer();
  void EnsureInitialKick();

  /**
   * @brief Resolves the GameplayManager instance pointer via ManagerCoreService.
   * @return The GameplayManager object pointer (0 if not resolved yet).
   */
  uintptr_t ResolveEnvironmentBase() const;

  uintptr_t GetProfilePtr(ProfileRef prof);

  // --- Profile Data Helpers (always need ProfileRef) ---
  uintptr_t GetEnvObject();
  uintptr_t GetEnvProfileData();
  void UpdateEnvironment(uintptr_t env);
  uint64_t GetProfileCount(intptr_t offset, ProfileRef prof);
  float* GetProfileArray(intptr_t offset, ProfileRef prof);
  float GetProfileScalar(intptr_t offset, ProfileRef prof);
  void SetProfileScalar(intptr_t offset, ProfileRef prof, float val);
  float GetProfileFloat(intptr_t offset, ProfileRef prof);
  float GetProfileFloatByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx);
  void SetProfileFloat(intptr_t offset, ProfileRef prof, float val);
  void SetProfileFloatByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, float val);
  int32_t GetProfileInt(intptr_t offset, ProfileRef prof);
  int32_t GetProfileIntByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx);
  void SetProfileInt(intptr_t offset, ProfileRef prof, int32_t val);
  void SetProfileIntByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, int32_t val);
  Utils::Vector3 GetProfileVec3(intptr_t offset, ProfileRef prof);
  Utils::Vector3 GetProfileVec3ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx);
  void SetProfileVec3(intptr_t offset, ProfileRef prof, const Utils::Vector3& val);
  void SetProfileVec3ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const Utils::Vector3& val);
  Utils::Vec2f GetProfileVec2(intptr_t offset, ProfileRef prof);
  Utils::Vec2f GetProfileVec2ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx);
  void SetProfileVec2(intptr_t offset, ProfileRef prof, const Utils::Vec2f& val);
  void SetProfileVec2ByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const Utils::Vec2f& val);
  std::string GetProfileTexture(intptr_t offset, ProfileRef prof);
  std::string GetProfileTextureByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx);
  void SetProfileTexture(intptr_t offset, ProfileRef prof, const std::string& val);
  void SetProfileTextureByIndex(intptr_t offset, ProfileRef prof, uint64_t varIdx, const std::string& val);

  // --- Blended Profile Data Helpers ---
  float GetBlendedFloat(intptr_t offset);
  void SetBlendedFloat(intptr_t offset, float blendedVal, float minVal, float maxVal);
  Utils::Vector3 GetBlendedVec3(intptr_t offset);
  void SetBlendedVec3(intptr_t offset, const Utils::Vector3& blendedVal, float maxComponent);
  Utils::Vec2f GetBlendedVec2(intptr_t offset);
  void SetBlendedVec2(intptr_t offset, const Utils::Vec2f& blendedVal, float maxComponent);

  // --- Runtime State ---
  bool m_isInitialized = false;
  std::vector<std::unique_ptr<IClimateDataFinder>> m_dataFinders;

  // --- Environment Data Offsets and Pointers ---
  uintptr_t m_updateFnAddr = 0;

  // --- Weather and Environment Data ---
  intptr_t m_weatherModeOffset = 0;
  intptr_t m_nextWeatherModeOffset = 0;
  intptr_t m_climatePtrOffset = 0;
  intptr_t m_climateUnitIdOffset = 0;
  intptr_t m_climateArrayOffset = 0;
  intptr_t m_climateCountOffset = 0;
  uintptr_t m_setWeatherModeFnAddr = 0;
  uintptr_t m_setClimateFnAddr = 0;
  intptr_t m_activeProfileIndexOffset = 0;
  intptr_t m_nextProfileIndexOffset = 0;

  intptr_t m_containerNiceOffset = 0;
  intptr_t m_containerBadOffset = 0;
  intptr_t m_profilesArrayOffset = 0x08;
  intptr_t m_containerCountOffset = 0x10;
  intptr_t m_sunAngleOffset = 0;
  uintptr_t m_weatherBlendFnAddr = 0;
  uintptr_t m_transitionDurationAddr = 0;

  // --- Bad Weather Data ---
  uintptr_t m_badWeatherFactorPtr = 0;
  intptr_t m_remainingBadWeatherOffset = 0;

  // --- Sun Profile Reflection Attribute Offsets ---
  intptr_t m_lowElevationOffset = 0;
  intptr_t m_highElevationOffset = 0;
  intptr_t m_sunDirectionOffset = 0;
  intptr_t m_temperatureOffset = 0;
  intptr_t m_skyboxTextureOffset = 0;
  intptr_t m_skycloudMaskTextureOffset = 0;
  intptr_t m_lightningMaskOffset = 0;
  intptr_t m_starsTextureOffset = 0;
  intptr_t m_mirrorSkyTextureOffset = 0;
  intptr_t m_ambientOffset = 0;
  intptr_t m_diffuseOffset = 0;
  intptr_t m_specularOffset = 0;
  intptr_t m_envOffset = 0;
  intptr_t m_envStaticModOffset = 0;
  intptr_t m_skyColorOffset = 0;
  intptr_t m_skyBottomColorOffset = 0;
  intptr_t m_starmapColorOffset = 0;
  intptr_t m_starsColorOffset = 0;
  intptr_t m_sunColorOffset = 0;
  intptr_t m_sunOpacityOffset = 0;
  intptr_t m_sunHaloColorOffset = 0;
  intptr_t m_sunShadowStrengthOffset = 0;
  intptr_t m_moonColorOffset = 0;
  intptr_t m_moonHaloColorOffset = 0;
  intptr_t m_moonHaloScaleOffset = 0;
  intptr_t m_fogColorOffset = 0;
  intptr_t m_fogColor2Offset = 0;
  intptr_t m_fogVgradientOffset = 0;
  intptr_t m_fogOffsetOffset = 0;
  intptr_t m_fogDensityOffset = 0;
  intptr_t m_speedCoefOffset = 0;
  intptr_t m_cloudShadowWeightOffset = 0;
  intptr_t m_cloudShadowTextureOffset = 0;
  intptr_t m_cloudShadowAreaSizeOffset = 0;
  intptr_t m_cloudShadowSpeedOffset = 0;
  intptr_t m_rainIntensityOffset = 0;
  intptr_t m_lightningIntensityOffset = 0;
  intptr_t m_rainMaxWetnessOffset = 0;
  intptr_t m_rainAdditionalAmbientOffset = 0;
  intptr_t m_snowIntensityOffset = 0;
  intptr_t m_snowFlakeSizeRangeOffset = 0;
  intptr_t m_snowChaosRateOffset = 0;
  intptr_t m_snowChaosWeightOffset = 0;
  intptr_t m_snowAdditionalAmbientOffset = 0;
  intptr_t m_windTypeOffset = 0;
  intptr_t m_dofStartOffset = 0;
  intptr_t m_dofTransitionOffset = 0;
  intptr_t m_dofFilterSizeOffset = 0;
  intptr_t m_colorBalanceOffset = 0;
  intptr_t m_colorSaturationOffset = 0;
  intptr_t m_sunshaftColorOffset = 0;
  intptr_t m_sunshaftSizeOffset = 0;
  intptr_t m_lowIntensityMinimumOffset = 0;
  intptr_t m_lowIntensityMaximumOffset = 0;
  intptr_t m_lowIntensityColorOffset = 0;
  intptr_t m_minScaleOffset = 0;
  intptr_t m_maxScaleOffset = 0;
  intptr_t m_scaleOverrideOffset = 0;
  intptr_t m_darkAdaptationSpeedOffset = 0;
  intptr_t m_brightAdaptationSpeedOffset = 0;
  intptr_t m_targetGrayOffset = 0;
  intptr_t m_contrastOffset = 0;
  intptr_t m_shoulderLengthOffset = 0;
  intptr_t m_bloomThresholdOffset = 0;
  intptr_t m_bloomLimitOffset = 0;
  intptr_t m_bloomIntensityOffset = 0;
  intptr_t m_bloomStandardDeviationOffset = 0;
  intptr_t m_stabilityOffset = 0;
  intptr_t m_weightOffset = 0;

  // --- Env Profile Data ---
  intptr_t m_envProfilePtrOffset = 0;  // offset in env object to env_profile data pointer

  // --- Env Profile Reflection Attribute Offsets ---
  intptr_t m_lampsOnElevationOffset = 0;
  intptr_t m_dayInYearOffset = 0;
  intptr_t m_summerTimeOffset = 0;
  intptr_t m_thunderstormProbabilityOffset = 0;
};

}  // namespace Data::GameData
SPF_NS_END
