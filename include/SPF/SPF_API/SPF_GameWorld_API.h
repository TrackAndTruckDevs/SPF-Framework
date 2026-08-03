/**
 * @file SPF_GameWorld_API.h
 * @brief API for inspecting and interacting with the core game world state (time, clock, engine pause/halt, warp).
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// --- Function Typedefs ---

/**
 * @brief Checks if the Game World Service is fully initialized and offsets are found.
 * @return True if the service is ready for use.
 */
typedef bool (*SPF_GW_IsReady_t)();

/**
 * @brief Checks if a specific game world data finder is ready.
 * @param finderName The name of the finder to check.
 * @return True if the specific offsets are found and valid.
 */
typedef bool (*SPF_GW_IsFinderReady_t)(const char* finderName);

/**
 * @brief Checks if all required memory patterns for the game world system were successfully found.
 * @return True if all dynamic patterns were successfully resolved.
 */
typedef bool (*SPF_GW_AreAllOffsetsFound_t)();

/**
 * @brief Forces the framework to re-scan game memory for all game world-related offsets.
 * @return True if all offsets were successfully found after the refresh operation.
 */
typedef bool (*SPF_GW_RefreshOffsets_t)();

/**
 * @brief Gets the current skybox/lighting preview time.
 * @return The visual environment time in total minutes.
 */
typedef uint32_t (*SPF_GW_GetPreviewTime_t)();

/**
 * @brief Sets the skybox/lighting preview time.
 * @param totalMinutes The visual environment time in total minutes.
 */
typedef void (*SPF_GW_SetPreviewTime_t)(uint32_t totalMinutes);

/**
 * @brief Gets the actual game simulation clock time.
 * @return The simulation time in total minutes.
 */
typedef uint32_t (*SPF_GW_GetSimulationTime_t)();

/**
 * @brief Sets the actual game simulation clock time.
 * @param totalMinutes The simulation time in total minutes.
 */
typedef void (*SPF_GW_SetSimulationTime_t)(uint32_t totalMinutes);

/**
 * @brief Enables or disables auto-update of the skybox time based on simulation.
 * @param enabled True to enable auto-update, false to freeze.
 */
typedef void (*SPF_GW_SetSkyboxAutoUpdate_t)(bool enabled);

/**
 * @brief Gets the real play time.
 * @return Real play time in minutes.
 */
typedef uint32_t (*SPF_GW_GetRealPlayTime_t)();

/**
 * @brief Gets the current map scale factor.
 * @return The map scale.
 */
typedef float (*SPF_GW_GetMapScale_t)();

/**
 * @brief Gets the global time warp speed factor.
 * @return The global warp factor (1.0 is normal speed).
 */
typedef float (*SPF_GW_GetGlobalWarp_t)();

/**
 * @brief Sets the global time warp speed factor.
 * @param warp The global warp factor.
 */
typedef void (*SPF_GW_SetGlobalWarp_t)(float warp);

/**
 * @brief Checks if the game simulation is currently paused or halted.
 * @return True if the engine is halted or simulation is paused.
 */
typedef bool (*SPF_GW_IsGamePaused_t)();

/**
 * @brief Pauses or unpauses the game simulation.
 * @param paused True to pause the simulation, false to resume.
 */
typedef void (*SPF_GW_SetGamePaused_t)(bool paused);

/**
 * @brief Forces a hard halt on the engine updates (simulation, traffic, etc.).
 * @param halted True to halt the engine, false to resume.
 */
typedef void (*SPF_GW_SetEngineHalt_t)(bool halted);

/**
 * @brief Gets the real delta time between frames.
 * @return Delta time in seconds.
 */
typedef double (*SPF_GW_GetRealDeltaTime_t)();

/**
 * @brief Gets the total game days elapsed.
 * @return Total days.
 */
typedef uint32_t (*SPF_GW_GetGameDay_t)();

/**
 * @brief Gets the day of the week (0 = Monday, 6 = Sunday).
 * @return Day of week index.
 */
typedef uint32_t (*SPF_GW_GetDayOfWeek_t)();

/**
 * @brief Gets the current game week index.
 * @return Week index.
 */
typedef uint32_t (*SPF_GW_GetGameWeek_t)();

/**
 * @brief Gets the total number of cities in the current game world.
 * @return The city count, or 0 if the world data is not ready.
 */
typedef uint32_t (*SPF_GW_GetCityCount_t)();

/**
 * @brief Copies the name of a city into the provided buffer.
 *
 * @details The name is the raw game city name (e.g. "Yuma"). If the name is longer
 *          than the buffer, it is truncated but the full length is still returned.
 *
 * @param index Zero-based city index.
 * @param out_buffer Buffer to receive the name.
 * @param buffer_size Size of the output buffer.
 * @return The full name length excluding the null terminator, or -1 if the index is invalid.
 */
typedef int (*SPF_GW_GetCityName_t)(uint32_t index, char* out_buffer, int buffer_size);

/**
 * @brief Gets the uid of a city by its cache index.
 * @param index Zero-based city index.
 * @return The city uid, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityUid_t)(uint32_t index);

/**
 * @brief Resolves the world position of a city by its uid.
 *
 * @details Coordinates are averaged over every geometry point of the city's
 *          kdop item (the same way con_cmd_goto resolves the city center).
 *
 * @param uid The city uid.
 * @param out_x Output X coordinate (game world units).
 * @param out_y Output Y coordinate (elevation).
 * @param out_z Output Z coordinate (game world units).
 * @return True if the city was found and coordinates were written.
 */
typedef bool (*SPF_GW_GetCityPosition_t)(uint32_t uid, float* out_x, float* out_y, float* out_z);

/**
 * @brief Sets the world position of a city by its uid (fixed-point 1/256 write).
 * @param uid The city uid.
 * @param x New X coordinate (game world units).
 * @param y New Y coordinate (elevation).
 * @param z New Z coordinate (game world units).
 * @return True if the city was found and the position was written.
 */
typedef bool (*SPF_GW_SetCityPosition_t)(uint32_t uid, float x, float y, float z);

/**
 * @brief Gets the number of geometry points for a city (vtable slot +0x68).
 * @param index Zero-based city index.
 * @return The point count, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityPointCount_t)(uint32_t index);

/**
 * @brief Resolves the i-th geometry point of a city (vtable slot +0x70).
 * @param index Zero-based city index.
 * @param point_index Zero-based geometry point index.
 * @param out_x Output X coordinate (game world units).
 * @param out_y Output Y coordinate (elevation).
 * @param out_z Output Z coordinate (game world units).
 * @return True if the point was resolved.
 */
typedef bool (*SPF_GW_GetCityPoint_t)(uint32_t index, uint32_t point_index, float* out_x, float* out_y, float* out_z);

/**
 * @brief Gets the kdop item +0x50 float (bounding scale/radius factor).
 * @param index Zero-based city index.
 * @return The item scale, or 0 if the index is invalid.
 */
typedef float (*SPF_GW_GetCityItemScale_t)(uint32_t index);

/**
 * @brief Gets the kdop item +0x54 float (bounding scale/radius factor).
 * @param index Zero-based city index.
 * @return The item radius, or 0 if the index is invalid.
 */
typedef float (*SPF_GW_GetCityItemRadius_t)(uint32_t index);

/**
 * @brief Sets the kdop item +0x50 float.
 * @param index Zero-based city index.
 * @param val The new item scale.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityItemScale_t)(uint32_t index, float val);

/**
 * @brief Sets the kdop item +0x54 float.
 * @param index Zero-based city index.
 * @param val The new item radius.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityItemRadius_t)(uint32_t index, float val);

/**
 * @brief Copies the localized display name of a city into the provided buffer.
 * @param index Zero-based city index.
 * @param out_buffer Buffer to receive the localized name.
 * @param buffer_size Size of the output buffer.
 * @return The full name length excluding the null terminator, or -1 on failure.
 */
typedef int (*SPF_GW_GetCityNameLocalized_t)(uint32_t index, char* out_buffer, int buffer_size);

/**
 * @brief Copies the short name of a city into the provided buffer.
 * @param index Zero-based city index.
 * @param out_buffer Buffer to receive the short name.
 * @param buffer_size Size of the output buffer.
 * @return The full name length excluding the null terminator, or -1 on failure.
 */
typedef int (*SPF_GW_GetCityShortName_t)(uint32_t index, char* out_buffer, int buffer_size);

/**
 * @brief Copies the localized short name of a city into the provided buffer.
 * @param index Zero-based city index.
 * @param out_buffer Buffer to receive the localized short name.
 * @param buffer_size Size of the output buffer.
 * @return The full name length excluding the null terminator, or -1 on failure.
 */
typedef int (*SPF_GW_GetCityShortNameLocalized_t)(uint32_t index, char* out_buffer, int buffer_size);

/**
 * @brief Gets the city group id (city_data +0x20).
 * @param index Zero-based city index.
 * @return The city group id, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityGroup_t)(uint32_t index);

/**
 * @brief Sets the city group id (city_data +0x20).
 * @param index Zero-based city index.
 * @param val The new city group id.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityGroup_t)(uint32_t index, uint32_t val);

/**
 * @brief Gets the city pin scale factor (city_data array_t<float>).
 * @param index Zero-based city index.
 * @return The pin scale factor, or 0 if the index is invalid.
 */
typedef float (*SPF_GW_GetCityPinScaleFactor_t)(uint32_t index);

/**
 * @brief Sets the city pin scale factor (city_data array_t<float> first element).
 * @param index Zero-based city index.
 * @param val The new pin scale factor.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityPinScaleFactor_t)(uint32_t index, float val);

/**
 * @brief Reads the per-zoom map X offsets array of a city.
 * @param index Zero-based city index.
 * @param out Buffer receiving up to max_count elements.
 * @param max_count Maximum number of elements to write into out.
 * @return True if at least one element was read.
 */
typedef bool (*SPF_GW_GetCityMapXOffsets_t)(uint32_t index, float* out, size_t max_count);

/**
 * @brief Reads the per-zoom map Y offsets array of a city.
 * @param index Zero-based city index.
 * @param out Buffer receiving up to max_count elements.
 * @param max_count Maximum number of elements to write into out.
 * @return True if at least one element was read.
 */
typedef bool (*SPF_GW_GetCityMapYOffsets_t)(uint32_t index, float* out, size_t max_count);

/**
 * @brief Writes the per-zoom map X offsets array of a city.
 * @param index Zero-based city index.
 * @param values Buffer of values to write.
 * @param count Number of elements to write.
 * @return True if the values were written.
 */
typedef bool (*SPF_GW_SetCityMapXOffsets_t)(uint32_t index, const float* values, size_t count);

/**
 * @brief Writes the per-zoom map Y offsets array of a city.
 * @param index Zero-based city index.
 * @param values Buffer of values to write.
 * @param count Number of elements to write.
 * @return True if the values were written.
 */
typedef bool (*SPF_GW_SetCityMapYOffsets_t)(uint32_t index, const float* values, size_t count);

/**
 * @brief Gets the city price coefficient (city_data float).
 * @param index Zero-based city index.
 * @return The price coefficient, or 0 if the index is invalid.
 */
typedef float (*SPF_GW_GetCityPriceCoef_t)(uint32_t index);

/**
 * @brief Sets the city price coefficient (city_data float).
 * @param index Zero-based city index.
 * @param val The new price coefficient.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityPriceCoef_t)(uint32_t index, float val);

/**
 * @brief Gets the country id of a city (city_data country_data reference).
 * @param index Zero-based city index.
 * @return The country id, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityCountry_t)(uint32_t index);

/**
 * @brief Sets the country id of a city.
 * @param index Zero-based city index.
 * @param val The new country id.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityCountry_t)(uint32_t index, uint32_t val);

/**
 * @brief Gets the population of a city.
 * @param index Zero-based city index.
 * @return The population, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityPopulation_t)(uint32_t index);

/**
 * @brief Sets the population of a city.
 * @param index Zero-based city index.
 * @param val The new population.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityPopulation_t)(uint32_t index, uint32_t val);

/**
 * @brief Checks if a city is a key city.
 * @param index Zero-based city index.
 * @return True if the city is a key city.
 */
typedef bool (*SPF_GW_GetCityKeyCity_t)(uint32_t index);

/**
 * @brief Sets the key city flag of a city.
 * @param index Zero-based city index.
 * @param val True to mark the city as a key city.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityKeyCity_t)(uint32_t index, bool val);

/**
 * @brief Gets the time zone id of a city.
 * @param index Zero-based city index.
 * @return The time zone id, or 0 if the index is invalid.
 */
typedef uint32_t (*SPF_GW_GetCityTimeZone_t)(uint32_t index);

/**
 * @brief Sets the time zone id of a city.
 * @param index Zero-based city index.
 * @param val The new time zone id.
 * @return True if the value was written.
 */
typedef bool (*SPF_GW_SetCityTimeZone_t)(uint32_t index, uint32_t val);

/**
 * @struct SPF_GameWorld_API
 * @brief API for interacting with the core game world state and time.
 */
typedef struct SPF_GameWorld_API {
  SPF_GW_IsReady_t GW_IsReady;
  SPF_GW_IsFinderReady_t GW_IsFinderReady;
  SPF_GW_AreAllOffsetsFound_t GW_AreAllOffsetsFound;
  SPF_GW_RefreshOffsets_t GW_RefreshOffsets;

  SPF_GW_GetPreviewTime_t GW_GetPreviewTime;
  SPF_GW_SetPreviewTime_t GW_SetPreviewTime;
  SPF_GW_GetSimulationTime_t GW_GetSimulationTime;
  SPF_GW_SetSimulationTime_t GW_SetSimulationTime;
  SPF_GW_SetSkyboxAutoUpdate_t GW_SetSkyboxAutoUpdate;

  SPF_GW_GetRealPlayTime_t GW_GetRealPlayTime;
  SPF_GW_GetMapScale_t GW_GetMapScale;
  SPF_GW_GetGlobalWarp_t GW_GetGlobalWarp;
  SPF_GW_SetGlobalWarp_t GW_SetGlobalWarp;
  SPF_GW_IsGamePaused_t GW_IsGamePaused;
  SPF_GW_SetGamePaused_t GW_SetGamePaused;
  SPF_GW_SetEngineHalt_t GW_SetEngineHalt;
  SPF_GW_GetRealDeltaTime_t GW_GetRealDeltaTime;

  SPF_GW_GetGameDay_t GW_GetGameDay;
  SPF_GW_GetDayOfWeek_t GW_GetDayOfWeek;
  SPF_GW_GetGameWeek_t GW_GetGameWeek;

  SPF_GW_GetCityCount_t GW_GetCityCount;
  SPF_GW_GetCityName_t GW_GetCityName;
  SPF_GW_GetCityPosition_t GW_GetCityPosition;

  SPF_GW_GetCityUid_t GW_GetCityUid;
  SPF_GW_SetCityPosition_t GW_SetCityPosition;
  SPF_GW_GetCityPointCount_t GW_GetCityPointCount;
  SPF_GW_GetCityPoint_t GW_GetCityPoint;
  SPF_GW_GetCityItemScale_t GW_GetCityItemScale;
  SPF_GW_GetCityItemRadius_t GW_GetCityItemRadius;
  SPF_GW_SetCityItemScale_t GW_SetCityItemScale;
  SPF_GW_SetCityItemRadius_t GW_SetCityItemRadius;
  SPF_GW_GetCityNameLocalized_t GW_GetCityNameLocalized;
  SPF_GW_GetCityShortName_t GW_GetCityShortName;
  SPF_GW_GetCityShortNameLocalized_t GW_GetCityShortNameLocalized;
  SPF_GW_GetCityGroup_t GW_GetCityGroup;
  SPF_GW_SetCityGroup_t GW_SetCityGroup;
  SPF_GW_GetCityPinScaleFactor_t GW_GetCityPinScaleFactor;
  SPF_GW_SetCityPinScaleFactor_t GW_SetCityPinScaleFactor;
  SPF_GW_GetCityMapXOffsets_t GW_GetCityMapXOffsets;
  SPF_GW_GetCityMapYOffsets_t GW_GetCityMapYOffsets;
  SPF_GW_SetCityMapXOffsets_t GW_SetCityMapXOffsets;
  SPF_GW_SetCityMapYOffsets_t GW_SetCityMapYOffsets;
  SPF_GW_GetCityPriceCoef_t GW_GetCityPriceCoef;
  SPF_GW_SetCityPriceCoef_t GW_SetCityPriceCoef;
  SPF_GW_GetCityCountry_t GW_GetCityCountry;
  SPF_GW_SetCityCountry_t GW_SetCityCountry;
  SPF_GW_GetCityPopulation_t GW_GetCityPopulation;
  SPF_GW_SetCityPopulation_t GW_SetCityPopulation;
  SPF_GW_GetCityKeyCity_t GW_GetCityKeyCity;
  SPF_GW_SetCityKeyCity_t GW_SetCityKeyCity;
  SPF_GW_GetCityTimeZone_t GW_GetCityTimeZone;
  SPF_GW_SetCityTimeZone_t GW_SetCityTimeZone;
} SPF_GameWorld_API;

#ifdef __cplusplus
}
#endif
