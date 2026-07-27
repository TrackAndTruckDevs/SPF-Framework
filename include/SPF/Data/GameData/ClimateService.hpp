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
  void SetNextWeatherModeOffset(intptr_t val) { m_nextWeatherModeOffset = val; }
  void SetClimatePtrOffset(intptr_t val) { m_climatePtrOffset = val; }
  void SetClimateUnitIdOffset(intptr_t val) { m_climateUnitIdOffset = val; }
  void SetClimateArrayOffset(intptr_t val) { m_climateArrayOffset = val; }
  void SetClimateCountOffset(intptr_t val) { m_climateCountOffset = val; }
  void SetSetWeatherModeFnAddr(uintptr_t addr) { m_setWeatherModeFnAddr = addr; }
  void SetSetClimateFnAddr(uintptr_t addr) { m_setClimateFnAddr = addr; }
  void SetActiveProfileIndexOffset(intptr_t val) { m_activeProfileIndexOffset = val; }
  void SetNextProfileIndexOffset(intptr_t val) { m_nextProfileIndexOffset = val; }
  void SetContainerSelectorOffset(intptr_t val) { m_containerSelectorOffset = val; }
  void SetContainerNiceOffset(intptr_t val) { m_containerNiceOffset = val; }
  void SetContainerBadOffset(intptr_t val) { m_containerBadOffset = val; }
  void SetProfilesArrayOffset(intptr_t val) { m_profilesArrayOffset = val; }
  void SetContainerCountOffset(intptr_t val) { m_containerCountOffset = val; }
  void SetSunAngleOffset(intptr_t val) { m_sunAngleOffset = val; }
  void SetWeatherBlendProgressFnAddr(uintptr_t addr) { m_weatherBlendFnAddr = addr; }
  void SetTransitionDurationAddr(uintptr_t addr) { m_transitionDurationAddr = addr; }

  // --- Weather & Environment methods ---
  int32_t GetWeatherMode();
  int32_t GetNextWeatherMode();
  void SetNextWeatherMode(int32_t mode);
  void SetWeatherMode(int32_t mode, bool instant = true);

  float GetRainIntensity();
  void SetRainIntensity(float intensity);

  float GetTemperature(int profileSlot, uint32_t variationIdx);
  void SetTemperature(int profileSlot, uint32_t variationIdx, float val);

  struct ClimateInfo {
    std::string name;
    uint64_t token;
  };

  std::string GetCurrentClimateName();
  std::vector<ClimateInfo> GetAvailableClimates();
  void SetClimate(uint64_t token, bool instant = true);

  float GetWeight(int profileSlot, uint32_t variationIdx);
  void SetWeight(int profileSlot, uint32_t variationIdx, float value);

  // --- Sun Profile API ---
  int32_t GetActiveSunProfileIndex();
  int32_t GetNextSunProfileIndex();
  int32_t GetSunProfileCount();
  std::string GetSunProfileName(int32_t index);
  float GetSunProfileElevation(int32_t index);
  float GetTransitionProgress();

  float GetSunAngle();
  float GetWeatherBlendProgress();
  void SetTransitionDuration(int32_t minutes);

 private:
  ClimateService();
  ~ClimateService() = default;

  void RegisterFinders();
  uintptr_t GetClimateContainer();
  uintptr_t GetActiveProfilePtr(int profileSlot);
  void EnsureInitialKick();

  // --- Runtime State ---
  bool m_isInitialized = false;
  std::vector<std::unique_ptr<IClimateDataFinder>> m_dataFinders;

  // --- Environment Data Offsets and Pointers ---
  uintptr_t m_environmentBasePtr = 0;
  intptr_t m_environmentAdjustment = 0;
  intptr_t m_envObjectOffset = 0;
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
  intptr_t m_containerSelectorOffset = 0;
  intptr_t m_containerNiceOffset = 0;
  intptr_t m_containerBadOffset = 0;
  intptr_t m_profilesArrayOffset = 0x08;
  intptr_t m_containerCountOffset = 0x10;
  intptr_t m_sunAngleOffset = 0;
  uintptr_t m_weatherBlendFnAddr = 0;
  uintptr_t m_transitionDurationAddr = 0;
};

}  // namespace Data::GameData
SPF_NS_END
