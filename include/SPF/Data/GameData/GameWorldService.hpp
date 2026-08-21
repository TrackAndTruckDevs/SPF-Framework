#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/IGameWorldDataFinder.hpp"
#include "SPF/Data/GameData/IWorldScopedService.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {

/**
 * @class GameWorldService
 * @brief A service that provides memory offsets and pointers for core game world data (time, engine state).
 */
class GameWorldService : public IWorldScopedService {
 public:
  static GameWorldService& GetInstance();

  GameWorldService(const GameWorldService&) = delete;
  void operator=(const GameWorldService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady();
  bool IsFinderReady(const char* name) const;
  bool AreAllFindersReady() const;
  bool TryFindAllOffsets();

  // --- IWorldScopedService ---
  const char* GetName() const override { return "GameWorldService"; }
  void ResetForWorldReload() override { Shutdown(); }
  bool TryFinalizeWorldInit() override { return TryFindAllOffsets(); }

  // --- Public Getters ---
  intptr_t GetTimeOffset() const { return m_timeOffset; }
  intptr_t GetSimulationTimeOffset() const { return m_simulationTimeOffset; }
  intptr_t GetSubMinuteSecondsOffset() const { return m_subMinuteSecondsOffset; }
  intptr_t GetRealPlayTimeOffset() const { return m_realPlayTimeOffset; }
  intptr_t GetRealPlaySecondsOffset() const { return m_realPlaySecondsOffset; }
  intptr_t GetMapScaleOffset() const { return m_mapScaleOffset; }
  intptr_t GetGlobalWarpOffset() const { return m_globalWarpOffset; }
  intptr_t GetPauseStatusOffset() const { return m_pauseStatusOffset; }
  intptr_t GetGlobalHaltOffset() const { return m_globalHaltOffset; }
  intptr_t GetSimulationHaltOffset() const { return m_simulationHaltOffset; }
  intptr_t GetTrafficHaltOffset() const { return m_trafficHaltOffset; }
  intptr_t GetRealDeltaTimeOffset() const { return m_realDeltaTimeOffset; }
  intptr_t GetSkyboxAutoUpdateOffset() const { return m_skyboxAutoUpdateOffset; }
  uintptr_t GetUpdateFnAddr() const { return m_updateFnAddr; }

  // --- City_data offset getters (resolved by CityDataFinder via reflection) ---
  intptr_t GetCityNameOffset() const { return m_cityNameOffset; }
  intptr_t GetCityNameLocalizedOffset() const { return m_cityNameLocalizedOffset; }
  intptr_t GetShortCityNameOffset() const { return m_shortCityNameOffset; }
  intptr_t GetShortCityNameLocalizedOffset() const { return m_shortCityNameLocalizedOffset; }
  intptr_t GetCityGroupOffset() const { return m_cityGroupOffset; }
  intptr_t GetCityPinScaleFactorOffset() const { return m_cityPinScaleFactorOffset; }
  intptr_t GetMapXOffsetsOffset() const { return m_mapXOffsetsOffset; }
  intptr_t GetMapYOffsetsOffset() const { return m_mapYOffsetsOffset; }
  intptr_t GetPriceCoefOffset() const { return m_priceCoefOffset; }
  intptr_t GetCountryOffset() const { return m_countryOffset; }
  intptr_t GetPopulationOffset() const { return m_populationOffset; }
  intptr_t GetKeyCityOffset() const { return m_keyCityOffset; }
  intptr_t GetTimeZoneOffset() const { return m_timeZoneOffset; }
  intptr_t GetKdopArrayOffset() const { return m_kdopArrayOffset; }
  intptr_t GetKdopCountOffset() const { return m_kdopCountOffset; }
  intptr_t GetCityItemTypeOffset() const { return m_cityItemTypeOffset; }
  uint8_t GetCityItemType() const { return m_cityItemType; }
  intptr_t GetCityRecordOffset() const { return m_cityRecordOffset; }
  intptr_t GetCityFlagsOffset() const { return m_cityFlagsOffset; }
  intptr_t GetCityUidOffset() const { return m_cityUidOffset; }
  intptr_t GetCityScaleOffset() const { return m_cityScaleOffset; }
  intptr_t GetCityRadiusOffset() const { return m_cityRadiusOffset; }
  intptr_t GetCityVtablePointCountSlot() const { return m_cityVtablePointCountSlot; }
  intptr_t GetCityVtableGetPointSlot() const { return m_cityVtableGetPointSlot; }
  float GetCityPointScale() const { return m_cityPointScale; }
  intptr_t GetCityStringBufOffset() const { return m_cityStringBufOffset; }

  // --- Public Setters (for finders) ---
  void SetTimeOffset(intptr_t val) { m_timeOffset = val; }
  void SetSimulationTimeOffset(intptr_t val) { m_simulationTimeOffset = val; }
  void SetSubMinuteSecondsOffset(intptr_t val) { m_subMinuteSecondsOffset = val; }
  void SetMapScaleOffset(intptr_t val) { m_mapScaleOffset = val; }
  void SetRealPlayTimeOffset(intptr_t val) { m_realPlayTimeOffset = val; }
  void SetRealPlaySecondsOffset(intptr_t val) { m_realPlaySecondsOffset = val; }
  void SetGlobalWarpOffset(intptr_t val) { m_globalWarpOffset = val; }
  void SetPauseStatusOffset(intptr_t val) { m_pauseStatusOffset = val; }
  void SetRealDeltaTimeOffset(intptr_t val) { m_realDeltaTimeOffset = val; }
  void SetSkyboxAutoUpdateOffset(intptr_t val) { m_skyboxAutoUpdateOffset = val; }
  void SetUpdateFnAddr(uintptr_t val) { m_updateFnAddr = val; }
  void SetGlobalHaltOffset(intptr_t val) { m_globalHaltOffset = val; }
  void SetSimulationHaltOffset(intptr_t val) { m_simulationHaltOffset = val; }
  void SetTrafficHaltOffset(intptr_t val) { m_trafficHaltOffset = val; }

  // --- City_data offset setters (used by CityDataFinder) ---
  void SetCityNameOffset(intptr_t val) { m_cityNameOffset = val; }
  void SetCityNameLocalizedOffset(intptr_t val) { m_cityNameLocalizedOffset = val; }
  void SetShortCityNameOffset(intptr_t val) { m_shortCityNameOffset = val; }
  void SetShortCityNameLocalizedOffset(intptr_t val) { m_shortCityNameLocalizedOffset = val; }
  void SetCityGroupOffset(intptr_t val) { m_cityGroupOffset = val; }
  void SetCityPinScaleFactorOffset(intptr_t val) { m_cityPinScaleFactorOffset = val; }
  void SetMapXOffsetsOffset(intptr_t val) { m_mapXOffsetsOffset = val; }
  void SetMapYOffsetsOffset(intptr_t val) { m_mapYOffsetsOffset = val; }
  void SetPriceCoefOffset(intptr_t val) { m_priceCoefOffset = val; }
  void SetCountryOffset(intptr_t val) { m_countryOffset = val; }
  void SetPopulationOffset(intptr_t val) { m_populationOffset = val; }
  void SetKeyCityOffset(intptr_t val) { m_keyCityOffset = val; }
  void SetTimeZoneOffset(intptr_t val) { m_timeZoneOffset = val; }
  void SetKdopArrayOffset(intptr_t val) { m_kdopArrayOffset = val; }
  void SetKdopCountOffset(intptr_t val) { m_kdopCountOffset = val; }
  void SetCityItemTypeOffset(intptr_t val) { m_cityItemTypeOffset = val; }
  void SetCityItemType(uint8_t val) { m_cityItemType = val; }
  void SetCityRecordOffset(intptr_t val) { m_cityRecordOffset = val; }
  void SetCityFlagsOffset(intptr_t val) { m_cityFlagsOffset = val; }
  void SetCityUidOffset(intptr_t val) { m_cityUidOffset = val; }
  void SetCityScaleOffset(intptr_t val) { m_cityScaleOffset = val; }
  void SetCityRadiusOffset(intptr_t val) { m_cityRadiusOffset = val; }
  void SetCityVtablePointCountSlot(intptr_t val) { m_cityVtablePointCountSlot = val; }
  void SetCityVtableGetPointSlot(intptr_t val) { m_cityVtableGetPointSlot = val; }
  void SetCityPointScale(float val) { m_cityPointScale = val; }
  void SetCityStringBufOffset(intptr_t val) { m_cityStringBufOffset = val; }

  // --- World Manipulation Methods ---
  uint32_t GetPreviewTime();
  void SetPreviewTime(uint32_t totalMinutes);

  uint32_t GetSimulationTime();
  void SetSimulationTime(uint32_t totalMinutes);

  void SetSkyboxAutoUpdate(bool enabled);

  // --- City Data Methods ---
  uint32_t GetCityCount();
  int GetCityName(uint32_t index, char* outBuffer, int bufferSize);
  uint32_t GetCityUid(uint32_t index);

  /**
   * @brief Resolves the world position of a city by its uid (city_data +0x0C).
   * @details Cities are read from the GameplayManager's kdop array. The point
   *          data comes from the kdop item's GetPoint slot (+0x58), which stores
   *          fixed-point (1/256) ints in X, Y, Z order.
   * @param uid The city uid.
   * @param outX Output X coordinate (game world units).
   * @param outY Output Y coordinate (elevation).
   * @param outZ Output Z coordinate (game world units).
   * @return True if the city was found and coordinates were written.
   */
  bool GetCityPosition(uint32_t uid, float* outX, float* outY, float* outZ);

  /** @brief Sets the world position of a city by uid (fixed-point 1/256 write). */
  bool SetCityPosition(uint32_t uid, float x, float y, float z);

  /** @brief Number of geometry points for a city (vtable slot +0x68). */
  uint32_t GetCityPointCount(uint32_t index);
  /** @brief Resolves the i-th geometry point of a city (vtable slot +0x70). */
  bool GetCityPoint(uint32_t index, uint32_t pointIndex, float* outX, float* outY, float* outZ);
  /** @brief kdop item +0x50 float (bounding scale/radius factor). */
  float GetCityItemScale(uint32_t index);
  /** @brief kdop item +0x54 float (bounding scale/radius factor). */
  float GetCityItemRadius(uint32_t index);
  /** @brief Sets kdop item +0x50 float. */
  bool SetCityItemScale(uint32_t index, float val);
  /** @brief Sets kdop item +0x54 float. */
  bool SetCityItemRadius(uint32_t index, float val);

  // --- city_data string fields (get-only) ---
  int GetCityNameLocalized(uint32_t index, char* outBuffer, int bufferSize);
  int GetCityShortName(uint32_t index, char* outBuffer, int bufferSize);
  int GetCityShortNameLocalized(uint32_t index, char* outBuffer, int bufferSize);

  // --- city_data numeric fields (get + set) ---
  uint32_t GetCityGroup(uint32_t index);
  float GetCityPinScaleFactor(uint32_t index);
  /** @brief Reads the per-zoom map X offsets array (up to maxCount elements). */
  bool GetCityMapXOffsets(uint32_t index, float* out, size_t maxCount);
  /** @brief Reads the per-zoom map Y offsets array (up to maxCount elements). */
  bool GetCityMapYOffsets(uint32_t index, float* out, size_t maxCount);
  float GetCityPriceCoef(uint32_t index);
  uint32_t GetCityCountry(uint32_t index);
  uint32_t GetCityPopulation(uint32_t index);
  bool GetCityKeyCity(uint32_t index);
  uint32_t GetCityTimeZone(uint32_t index);

  bool SetCityGroup(uint32_t index, uint32_t val);
  bool SetCityPinScaleFactor(uint32_t index, float val);
  /** @brief Writes the per-zoom map X offsets array (count elements). */
  bool SetCityMapXOffsets(uint32_t index, const float* values, size_t count);
  /** @brief Writes the per-zoom map Y offsets array (count elements). */
  bool SetCityMapYOffsets(uint32_t index, const float* values, size_t count);
  bool SetCityPriceCoef(uint32_t index, float val);
  bool SetCityCountry(uint32_t index, uint32_t val);
  bool SetCityPopulation(uint32_t index, uint32_t val);
  bool SetCityKeyCity(uint32_t index, bool val);
  bool SetCityTimeZone(uint32_t index, uint32_t val);

  // --- Core/Engine Methods ---
  uint32_t GetRealPlayTime();
  float GetMapScale();
  float GetGlobalWarp();
  void SetGlobalWarp(float warp);
  bool IsGamePaused();
  void SetGamePaused(bool paused);
  void SetEngineHalt(bool halted);
  double GetRealDeltaTime();

  // --- Time Calculation Helpers ---
  uint32_t GetGameDay();    // Total game days
  uint32_t GetDayOfWeek();  // 0 = Monday, 6 = Sunday
  uint32_t GetGameWeek();   // Current game week index

 private:
  GameWorldService();
  ~GameWorldService() = default;

  void RegisterFinders();

  /**
   * @brief Resolves the GameplayManager instance pointer via ManagerCoreService.
   * @return The GameplayManager object pointer (0 if not resolved yet).
   */
  uintptr_t ResolveEnvironmentBase() const;

  /**
   * @brief Scans the GameplayManager's kdop array for city items.
   * @details Rebuilds the internal city cache (name + position) whenever the
   *          underlying array data pointer or element count changes.
   * @return True if the cache is non-empty after the refresh.
   */
  bool RefreshCityCache();

  /** @brief A single cached city entry resolved from the game memory. */
  struct CityEntry {
    std::string name;      ///< City display name.
    uint32_t uid = 0;      ///< City uid (city_data +0x0C).
    uintptr_t item = 0;    ///< kdop item pointer (for vtable/scale/radius access).
    uintptr_t record = 0;  ///< city_data record pointer (for attribute fields).
    float x = 0.0f;        ///< World X coordinate.
    float y = 0.0f;        ///< World Y coordinate (elevation).
    float z = 0.0f;        ///< World Z coordinate.
  };

  // --- Runtime State ---
  bool m_isInitialized = false;
  bool m_pluginHalted = false;
  std::vector<std::unique_ptr<IGameWorldDataFinder>> m_dataFinders;

  std::vector<CityEntry> m_cityCache;
  uintptr_t m_cityCacheDataPtr = 0;
  uintptr_t m_cityCacheCount = 0;

  // --- World Data Offsets and Pointers ---
  intptr_t m_timeOffset = 0;

  intptr_t m_simulationTimeOffset = 0;
  intptr_t m_subMinuteSecondsOffset = 0;

  intptr_t m_realPlayTimeOffset = 0;
  intptr_t m_realPlaySecondsOffset = 0;

  intptr_t m_mapScaleOffset = 0;

  intptr_t m_globalWarpOffset = 0;
  intptr_t m_pauseStatusOffset = 0;

  intptr_t m_globalHaltOffset = 0;
  intptr_t m_simulationHaltOffset = 0;
  intptr_t m_trafficHaltOffset = 0;

  intptr_t m_realDeltaTimeOffset = 0;

  intptr_t m_skyboxAutoUpdateOffset = 0;
  uintptr_t m_updateFnAddr = 0;

  // --- City_data attribute offsets (resolved by CityDataFinder via reflection) ---
  intptr_t m_cityNameOffset = 0;
  intptr_t m_cityNameLocalizedOffset = 0;
  intptr_t m_shortCityNameOffset = 0;
  intptr_t m_shortCityNameLocalizedOffset = 0;
  intptr_t m_cityGroupOffset = 0;
  intptr_t m_cityPinScaleFactorOffset = 0;
  intptr_t m_mapXOffsetsOffset = 0;
  intptr_t m_mapYOffsetsOffset = 0;
  intptr_t m_priceCoefOffset = 0;
  intptr_t m_countryOffset = 0;
  intptr_t m_populationOffset = 0;
  intptr_t m_keyCityOffset = 0;
  intptr_t m_timeZoneOffset = 0;
  intptr_t m_kdopArrayOffset = 0;
  intptr_t m_kdopCountOffset = 0;
  intptr_t m_cityItemTypeOffset = 0;
  uint8_t m_cityItemType = 0;
  intptr_t m_cityRecordOffset = 0;
  intptr_t m_cityFlagsOffset = 0;
  intptr_t m_cityUidOffset = 0;
  intptr_t m_cityScaleOffset = 0;
  intptr_t m_cityRadiusOffset = 0;
  intptr_t m_cityVtablePointCountSlot = 0;
  intptr_t m_cityVtableGetPointSlot = 0;
  float m_cityPointScale = 0.0f;
  intptr_t m_cityStringBufOffset = 0;
};

}  // namespace Data::GameData
SPF_NS_END
