#pragma once

#include "SPF/Namespace.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

// Forward declarations
class IClimateDataFinder;

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
  void SetClimateUnitIdOffset(intptr_t val) { m_climateUnitIdOffset = val; }
  void SetClimateArrayOffset(intptr_t val) { m_climateArrayOffset = val; }
  void SetClimateCountOffset(intptr_t val) { m_climateCountOffset = val; }
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
  void SetSetClimateFnAddr(uintptr_t addr) { m_setClimateFnAddr = addr; }
  void SetTimeOffset(intptr_t val) { m_timeOffset = val; }

  // --- Weather & Environment methods ---
  int32_t GetWeatherMode();
  void SetWeatherMode(int32_t mode, bool instant = true);

  float GetRainIntensity();
  void SetRainIntensity(float intensity);

  float GetTemperature(int profileSlot, uint32_t variationIdx);
  void SetTemperature(int profileSlot, uint32_t variationIdx, float val);

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

  float GetWeight(int profileSlot, uint32_t variationIdx);
  void SetWeight(int profileSlot, uint32_t variationIdx, float value);

  bool IsVersion1_60() const;

 private:
  ClimateService();
  ~ClimateService() = default;

  void RegisterFinders();
  uintptr_t GetActiveProfilePtr(int profileSlot);
  void EnsureInitialKick();

  intptr_t GetVerOffset(intptr_t offset159, intptr_t offset160) const;

  // --- Runtime State ---
  bool m_isInitialized = false;
  mutable int8_t m_versionCache = -1;
  std::vector<std::unique_ptr<IClimateDataFinder>> m_dataFinders;

  // --- Environment Data Offsets and Pointers ---
  uintptr_t m_environmentBasePtr = 0;
  intptr_t m_environmentAdjustment = 0;
  intptr_t m_envObjectOffset = 0;
  uintptr_t m_updateFnAddr = 0;

  // --- Weather and Environment Data ---
  intptr_t m_weatherModeOffset = 0;
  intptr_t m_weatherTargetOffset = 0;
  intptr_t m_weatherTransitionOffset = 0;
  intptr_t m_climatePtrOffset = 0;
  intptr_t m_climateUnitIdOffset = 0;
  intptr_t m_climateArrayOffset = 0;
  intptr_t m_climateCountOffset = 0;
  intptr_t m_rainIntensityOffset = 0;
  intptr_t m_roadWetnessOffset = 0;
  intptr_t m_fogColorOffset = 0;
  intptr_t m_fogDensityOffset = 0;
  intptr_t m_lightningEnabledOffset = 0;
  intptr_t m_lightningIntensityOffset = 0;
  intptr_t m_temperatureOffset = 0;
  intptr_t m_weatherTransStartTimeOffset = 0;
  intptr_t m_weatherTransDurationOffset = 0;
  intptr_t m_weatherBlendingFactorOffset = 0;
  intptr_t m_skyboxAutoUpdateOffset = 0;
  uintptr_t m_setClimateFnAddr = 0;
  intptr_t m_timeOffset = 0;
};

}  // namespace Data::GameData
SPF_NS_END
