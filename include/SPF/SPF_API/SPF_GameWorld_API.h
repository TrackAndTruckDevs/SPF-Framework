/**
 * @file SPF_GameWorld_API.h
 * @brief API for inspecting and interacting with the core game world state (time, clock, engine pause/halt, warp).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

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
} SPF_GameWorld_API;

#ifdef __cplusplus
}
#endif
