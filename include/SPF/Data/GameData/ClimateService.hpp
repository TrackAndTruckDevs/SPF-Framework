#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Utils/Vec3.hpp"
#include <cstdint>
#include <vector>
#include <memory>
#include <string>

SPF_NS_BEGIN
namespace Data::GameData {

// Forward declarations
class IClimateDataFinder;

/**
 * @class ClimateService
 * @brief A service that provides memory offsets and methods for climate and weather data.
 */
class ClimateService {
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

  // --- Public Getters for Environment ---
  uintptr_t GetEnvironmentBasePtr() const { return m_environmentBasePtr; }
  intptr_t GetEnvObjectOffset() const { return m_envObjectOffset; }
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- Public Setters (for finders) ---
  void SetEnvironmentBasePtr(uintptr_t val) { m_environmentBasePtr = val; }
  void SetEnvObjectOffset(intptr_t val) { m_envObjectOffset = val; }
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }
  void SetEnvironmentAdjustment(intptr_t val) { m_environmentAdjustment = val; }

  // --- Weather and Environment Setters (for finders) ---
  void SetWeatherModeOffset(intptr_t val) { m_weatherModeOffset = val; }
  void SetWeatherTargetOffset(intptr_t val) { m_weatherTargetOffset = val; }
  void SetWeatherTransitionOffset(intptr_t val) { m_weatherTransitionOffset = val; }
  void SetClimatePtrOffset(intptr_t val) { m_climatePtrOffset = val; }
  void SetRainIntensityOffset(intptr_t val) { m_rainIntensityOffset = val; }
  void SetRoadWetnessOffset(intptr_t val) { m_roadWetnessOffset = val; }
  void SetFogColorOffset(intptr_t val) { m_fogColorOffset = val; }
  void SetFogDensityOffset(intptr_t val) { m_fogDensityOffset = val; }
  void SetLightningEnabledOffset(intptr_t val) { m_lightningEnabledOffset = val; }
  void SetLightningIntensityOffset(intptr_t val) { m_lightningIntensityOffset = val; }
  void SetTemperatureOffset(intptr_t val) { m_temperatureOffset = val; }
  void SetWeatherTransStartTimeOffset(intptr_t val) { m_weatherTransStartTimeOffset = val; }
  void SetWeatherTransDurationOffset(intptr_t val) { m_weatherTransDurationOffset = val; }
  void SetWeatherBlendingFactorOffset(intptr_t val) { m_weatherBlendingFactorOffset = val; }
  void SetSkyboxAutoUpdateOffset(intptr_t val) { m_skyboxAutoUpdateOffset = val; }
  void SetActiveProfileIndexAOffset(intptr_t val) { m_activeProfileIndexAOffset = val; }
  void SetActiveProfileIndexBOffset(intptr_t val) { m_activeProfileIndexBOffset = val; }
  void SetNiceProfilesArrayOffset(intptr_t val) { m_niceProfilesArrayOffset = val; }
  void SetBadProfilesArrayOffset(intptr_t val) { m_badProfilesArrayOffset = val; }
  void SetProfileNameTokenOffset(intptr_t val) { m_profileNameTokenOffset = val; }
  void SetSetClimateFnAddr(uintptr_t addr) { m_setClimateFnAddr = addr; }
  void SetTimeOffset(intptr_t val) { m_timeOffset = val; }

  // --- Weather & Environment methods ---
  int32_t GetWeatherMode();
  void SetWeatherMode(int32_t mode, bool instant = true);

  float GetRainIntensity();
  void SetRainIntensity(float intensity);

  float GetRoadWetness();
  void SetRoadWetness(float wetness);

  float GetFogDensity();
  void SetFogDensity(float density);

  float GetFogOffset();
  void SetFogOffset(float offset);

  bool IsLightningEnabled();
  void SetLightningEnabled(bool enabled);

  float GetLightningIntensity();
  void SetLightningIntensity(float intensity);

  float GetSnowIntensity();
  void SetSnowIntensity(float intensity);

  float GetSunAppearance(int profileSlot, uint32_t variationIdx);
  void SetSunAppearance(int profileSlot, uint32_t variationIdx, float val);

  float GetTemperature(int profileSlot, uint32_t variationIdx);
  void SetTemperature(int profileSlot, uint32_t variationIdx, float val);

  float GetDashboardTemperature();

  void GetSnowflakeSize(float& minSize, float& maxSize);
  void SetSnowflakeSize(float minSize, float maxSize);

  void GetSnowChaos(float& rate, float& weight);
  void SetSnowChaos(float rate, float weight);

  float GetCloudSpeed(int profileSlot, uint32_t variationIdx);
  void SetCloudSpeed(int profileSlot, uint32_t variationIdx, float speed);

  float GetColorSaturation(int profileSlot, uint32_t variationIdx);
  void SetColorSaturation(int profileSlot, uint32_t variationIdx, float saturation);

  float GetSunGlowSize(int profileSlot, uint32_t variationIdx);
  void SetSunGlowSize(int profileSlot, uint32_t variationIdx, float size);

  uint64_t GetSkyboxCount(int32_t weatherMode);
  void SetSkyboxIndex(int32_t weatherMode, uint32_t index);
  uint32_t GetSkyboxIndex(int32_t weatherMode, uint32_t slot = 0);

  struct ClimateInfo {
      std::string name;
      uint64_t token;
  };

  std::string GetCurrentClimateName();
  std::vector<ClimateInfo> GetAvailableClimates();
  void SetClimate(uint64_t token, bool instant = true);
  std::string GetActiveProfileName(int profileSlot = 0);
  uint32_t GetActiveProfileIndex(int profileSlot = 0);

  void LogSunProfileData(int profileSlot);

  // --- Profile Parameter Methods (Semantic API) ---
  Utils::Vector3 GetAmbientColor(int profileSlot, uint32_t variationIdx);
  void SetAmbientColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSunColor(int profileSlot, uint32_t variationIdx);
  void SetSunColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSunDiffuseColor(int profileSlot, uint32_t variationIdx);
  void SetSunDiffuseColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSunSpecularColor(int profileSlot, uint32_t variationIdx);
  void SetSunSpecularColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSkyColor(int profileSlot, uint32_t variationIdx);
  void SetSkyColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSkyBottomColor(int profileSlot, uint32_t variationIdx);
  void SetSkyBottomColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetStarmapColor(int profileSlot, uint32_t variationIdx);
  void SetStarmapColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetStarsColor(int profileSlot, uint32_t variationIdx);
  void SetStarsColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSunHaloColor(int profileSlot, uint32_t variationIdx);
  void SetSunHaloColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetMoonColor(int profileSlot, uint32_t variationIdx);
  void SetMoonColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetMoonHaloColor(int profileSlot, uint32_t variationIdx);
  void SetMoonHaloColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetFogColor(int profileSlot, uint32_t variationIdx);
  void SetFogColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetFogColor2(int profileSlot, uint32_t variationIdx);
  void SetFogColor2(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetColorBalance(int profileSlot, uint32_t variationIdx);
  void SetColorBalance(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetSunshaftColor(int profileSlot, uint32_t variationIdx);
  void SetSunshaftColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  Utils::Vector3 GetLowIntensityColor(int profileSlot, uint32_t variationIdx);
  void SetLowIntensityColor(int profileSlot, uint32_t variationIdx, const Utils::Vector3& color);

  float GetEnvIntensity(int profileSlot, uint32_t variationIdx);
  void SetEnvIntensity(int profileSlot, uint32_t variationIdx, float value);

  float GetEnvStaticMod(int profileSlot, uint32_t variationIdx);
  void SetEnvStaticMod(int profileSlot, uint32_t variationIdx, float value);

  float GetSunOpacity(int profileSlot, uint32_t variationIdx);
  void SetSunOpacity(int profileSlot, uint32_t variationIdx, float value);

  float GetSunShadowStrength(int profileSlot, uint32_t variationIdx);
  void SetSunShadowStrength(int profileSlot, uint32_t variationIdx, float value);

  float GetMoonHaloScale(int profileSlot, uint32_t variationIdx);
  void SetMoonHaloScale(int profileSlot, uint32_t variationIdx, float value);

  float GetFogVGradient(int profileSlot, uint32_t variationIdx);
  void SetFogVGradient(int profileSlot, uint32_t variationIdx, float value);

  float GetCloudShadowWeight(int profileSlot, uint32_t variationIdx);
  void SetCloudShadowWeight(int profileSlot, uint32_t variationIdx, float value);

  float GetRainAdditionalAmbient(int profileSlot, uint32_t variationIdx);
  void SetRainAdditionalAmbient(int profileSlot, uint32_t variationIdx, float value);

  float GetSnowAdditionalAmbient(int profileSlot, uint32_t variationIdx);
  void SetSnowAdditionalAmbient(int profileSlot, uint32_t variationIdx, float value);

  float GetSunshaftSize(int profileSlot, uint32_t variationIdx);
  void SetSunshaftSize(int profileSlot, uint32_t variationIdx, float value);

  void GetDOFParams(int profileSlot, uint32_t variationIdx, float& start, float& transition, float& filterSize);
  void SetDOFParams(int profileSlot, uint32_t variationIdx, float start, float transition, float filterSize);

  void GetLowIntensityParams(int profileSlot, uint32_t variationIdx, float& min, float& max);
  void SetLowIntensityParams(int profileSlot, uint32_t variationIdx, float min, float max);

  float GetTargetGray(int profileSlot, uint32_t variationIdx);
  void SetTargetGray(int profileSlot, uint32_t variationIdx, float value);

  float GetContrast(int profileSlot, uint32_t variationIdx);
  void SetContrast(int profileSlot, uint32_t variationIdx, float value);

  float GetShoulderLength(int profileSlot, uint32_t variationIdx);
  void SetShoulderLength(int profileSlot, uint32_t variationIdx, float value);

  float GetBloomIntensity(int profileSlot, uint32_t variationIdx);
  void SetBloomIntensity(int profileSlot, uint32_t variationIdx, float value);

  float GetBloomThreshold(int profileSlot, uint32_t variationIdx);
  void SetBloomThreshold(int profileSlot, uint32_t variationIdx, float value);

  float GetBloomLimit(int profileSlot, uint32_t variationIdx);
  void SetBloomLimit(int profileSlot, uint32_t variationIdx, float value);

  float GetBloomStandardDeviation(int profileSlot, uint32_t variationIdx);
  void SetBloomStandardDeviation(int profileSlot, uint32_t variationIdx, float value);

  float GetStability(int profileSlot, uint32_t variationIdx);
  void SetStability(int profileSlot, uint32_t variationIdx, float value);

  float GetWeight(int profileSlot, uint32_t variationIdx);
  void SetWeight(int profileSlot, uint32_t variationIdx, float value);

  void GetExposureLimits(int profileSlot, uint32_t variationIdx, float& minScale, float& maxScale);
  void SetExposureLimits(int profileSlot, uint32_t variationIdx, float minScale, float maxScale);

//   void SetSkyboxAutoUpdate(bool enabled);

  /**
   * @brief Logs the current environment state (rain, fog, etc.) to the logger.
   */
  void LogEnvironmentState();

  /**
   * @brief Dumps a raw block of environment memory for offset verification.
   */
  void DumpEnvironmentMemory();

  /**
   * @brief Returns true if the current game version is 1.60 or newer.
   */
  bool IsVersion1_60() const;

 private:
  ClimateService();
  ~ClimateService() = default;

  void RegisterFinders();
  uintptr_t GetActiveProfilePtr(int profileSlot);
  void EnsureInitialKick();

  /**
   * @brief Helper to get version-aware hardcoded offsets.
   */
  intptr_t GetVerOffset(intptr_t offset159, intptr_t offset160) const;

  // --- Runtime State ---
  bool m_isInitialized = false;
  mutable int8_t m_versionCache = -1; // -1: Unknown, 0: 1.59, 1: 1.60+
  int32_t m_lastKickedMode = -1; // -1 = No kick yet, 0 = Nice kicked, 1 = Bad kicked
  std::vector<std::unique_ptr<IClimateDataFinder>> m_dataFinders;

  // --- Environment Data Offsets and Pointers ---
  uintptr_t m_environmentBasePtr = 0;    // Pointer to MainEngineObject
  intptr_t m_environmentAdjustment = 0; // Dynamic adjustment
  intptr_t m_envObjectOffset = 0;        // Offset to Environment object
  uintptr_t m_updateFnAddr = 0;          // Address of the UpdateEnvironmentState function

  // --- Weather and Environment Data ---
  intptr_t m_weatherModeOffset = 0;      // 0x3e50
  intptr_t m_weatherTargetOffset = 0;    // 0x3e54
  intptr_t m_weatherTransitionOffset = 0;// 0x4554
  intptr_t m_climatePtrOffset = 0;       // 0x2a98
  intptr_t m_rainIntensityOffset = 0;    // 0x3f14
  intptr_t m_roadWetnessOffset = 0;      // 0x3f18
  intptr_t m_fogColorOffset = 0;         // 0x3f30
  intptr_t m_fogDensityOffset = 0;       // 0x3f3c
  intptr_t m_lightningEnabledOffset = 0; // 0x3ef1
  intptr_t m_lightningIntensityOffset = 0;// 0x42cc
  intptr_t m_temperatureOffset = 0;      // 0x28c
  intptr_t m_weatherTransStartTimeOffset = 0; // 0x4558
  intptr_t m_weatherTransDurationOffset = 0;  // 0x4550
  intptr_t m_weatherBlendingFactorOffset = 0; // 0x45C8
  intptr_t m_skyboxAutoUpdateOffset = 0;      // 0x46c4
  intptr_t m_activeProfileIndexAOffset = 0;   // 0x4540
  intptr_t m_activeProfileIndexBOffset = 0;   // 0x4544
  intptr_t m_niceProfilesArrayOffset = 0;     // 0xC0
  intptr_t m_badProfilesArrayOffset = 0;      // 0x100
  intptr_t m_profileNameTokenOffset = 0;      // 0x08
  uintptr_t m_setClimateFnAddr = 0;           // Address of SetClimate function
  intptr_t m_timeOffset = 0;                  // Offset to visual minutes
};

}  // namespace Data::GameData
SPF_NS_END
