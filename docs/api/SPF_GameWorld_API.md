# SPF Game World API

The SPF Game World API provides an interface for inspecting and interacting with the core engine simulation clock, world environment time (skybox/lighting), simulation pause/halt status, and execution speed (warp).

## Getting the API

Request the Game World API from the framework during your plugin's initialization.

**Example: C**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_GameWorld_API.h"

// Global pointer to the Game World API
SPF_GameWorld_API* s_gameWorldAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_gameWorldAPI = (SPF_GameWorld_API*)params->GetAPI(SPF_API_GAMEWORLD); // Or retrieve via core_api->gameworld inside OnActivated
}
```

**Example: OnActivated callback (Recommended)**
```cpp
SPF_GameWorld_API* s_gameWorldAPI = NULL;

void OnActivated(const SPF_Core_API* core_api) {
    s_gameWorldAPI = core_api->gameworld;
}
```

## Key Concepts

1. **Dual Clock Systems**: The game maintains two separate clocks:
   - **Simulation Time**: The logical game simulation clock (used for routes, deadlines, job delivery schedules).
   - **Preview/Skybox Time**: The visual time of day (skybox texture transitions, lighting calculations). It can be frozen/auto-updated.
2. **Engine Halting**: The engine update loop can be hard-halted, freezing physics, traffic updates, and telemetry accumulation safely.

## Usage Example

```c
// 1. Check if the service is ready
if (s_gameWorldAPI->GW_IsReady()) {
    // 2. Read simulation and visual states
    uint32_t simMinutes = s_gameWorldAPI->GW_GetSimulationTime();
    uint32_t previewMinutes = s_gameWorldAPI->GW_GetPreviewTime();
    
    // Convert to days/hours/minutes
    uint32_t simDays = simMinutes / 1440;
    uint32_t simHours = (simMinutes % 1440) / 60;
    uint32_t simMins = simMinutes % 60;
    
    printf("Sim Clock: Day %u, %02u:%02u\n", simDays, simHours, simMins);
    
    // 3. Control simulation speed
    float warp = s_gameWorldAPI->GW_GetGlobalWarp();
    if (warp > 1.0f) {
        // Slow down warp speed
        s_gameWorldAPI->GW_SetGlobalWarp(1.0f);
    }
}
```

## Function Reference

### Management & Discovery

---
**`bool GW_IsReady()`**
Checks if the Game World Service is fully initialized.

---
**`bool GW_IsFinderReady(const char* finderName)`**
Checks if a specific finder (e.g., `"WorldDataFinder"`) is ready.

---
**`bool GW_AreAllOffsetsFound()`**
Checks if all required memory offsets were successfully found.

---
**`bool GW_RefreshOffsets()`**
Forces the framework to re-scan game memory for offsets.

### Skybox & Visual Clock

| Function | Return Type | Description |
|---|---|---|
| **`GW_GetPreviewTime()`** | `uint32_t` | Returns the visual environment time (in total minutes). |
| **`GW_SetPreviewTime(totalMinutes)`** | `void` | Sets the visual environment time (used for skybox/shadows). |
| **`GW_SetSkyboxAutoUpdate(enabled)`** | `void` | Disables or enables auto-updating of the visual skybox clock from the simulation clock. |

### Simulation Clock & Game Time

| Function | Return Type | Description |
|---|---|---|
| **`GW_GetSimulationTime()`** | `uint32_t` | Returns the logical game simulation clock (in total minutes). |
| **`GW_SetSimulationTime(totalMinutes)`** | `void` | Overwrites the game simulation clock. |
| **`GW_GetRealPlayTime()`** | `uint32_t` | Returns the player's total real play time (in minutes). |
| **`GW_GetGameDay()`** | `uint32_t` | Returns the total game days elapsed. |
| **`GW_GetDayOfWeek()`** | `uint32_t` | Returns the current day index of the week (0 = Monday, 6 = Sunday). |
| **`GW_GetGameWeek()`** | `uint32_t` | Returns the current game week index. |

### Simulation Control & Warp

| Function | Return Type | Description |
|---|---|---|
| **`GW_GetMapScale()`** | `float` | Returns the current map scale factor (e.g., 19.0 on highways, 3.0 in cities). |
| **`GW_GetGlobalWarp()`** | `float` | Returns the global warp factor (clock speed modifier). |
| **`GW_SetGlobalWarp(warp)`** | `void` | Overwrites the global warp factor (e.g., `2.0` for double speed, `0.5` for half). |
| **`GW_IsGamePaused()`** | `bool` | Returns true if the game simulation is currently paused or halted. |
| **`GW_SetGamePaused(paused)`** | `void` | Sets the simulation paused/unpaused state. |
| **`GW_SetEngineHalt(halted)`** | `void` | Hard halts all engine systems (physics, traffic, logic updates). |
| **`GW_GetRealDeltaTime()`** | `double` | Returns the real frame delta time (in seconds). |

### City Data & Geometry

| Function | Return Type | Description |
|---|---|---|
| **`GW_GetCityCount()`** | `uint32_t` | Gets the total number of cities in the current game world (or `0` if not ready). |
| **`GW_GetCityName(index, out_buffer, buffer_size)`** | `int` | Copies the raw name of a city into the provided buffer. Returns full length or `-1`. |
| **`GW_GetCityPosition(uid, out_x, out_y, out_z)`** | `bool` | Resolves the averaged world position coordinates of a city by its UID. |
| **`GW_GetCityUid(index)`** | `uint32_t` | Gets the UID of a city by its cache index (returns `0` if invalid). |
| **`GW_SetCityPosition(uid, x, y, z)`** | `bool` | Sets the world position of a city by its UID (fixed-point 1/256 write). |
| **`GW_GetCityPointCount(index)`** | `uint32_t` | Gets the number of geometry points for a city. |
| **`GW_GetCityPoint(index, point_index, out_x, out_y, out_z)`** | `bool` | Resolves the i-th geometry point of a city. |
| **`GW_GetCityItemScale(index)`** | `float` | Gets the kdop item scale factor (bounding scale factor). |
| **`GW_GetCityItemRadius(index)`** | `float` | Gets the kdop item bounding radius. |
| **`GW_SetCityItemScale(index, val)`** | `bool` | Sets the kdop item bounding scale factor. |
| **`GW_SetCityItemRadius(index, val)`** | `bool` | Sets the kdop item bounding radius. |
| **`GW_GetCityNameLocalized(index, out_buffer, buffer_size)`** | `int` | Copies the localized display name of a city into the provided buffer. |
| **`GW_GetCityShortName(index, out_buffer, buffer_size)`** | `int` | Copies the short name of a city into the provided buffer. |
| **`GW_GetCityShortNameLocalized(index, out_buffer, buffer_size)`** | `int` | Copies the localized short name of a city into the provided buffer. |
| **`GW_GetCityGroup(index)`** | `uint32_t` | Gets the city group ID. |
| **`GW_SetCityGroup(index, val)`** | `bool` | Sets the city group ID. |
| **`GW_GetCityPinScaleFactor(index)`** | `float` | Gets the city pin scale factor. |
| **`GW_SetCityPinScaleFactor(index, val)`** | `bool` | Sets the city pin scale factor (first element of array). |
| **`GW_GetCityMapXOffsets(index, out, max_count)`** | `bool` | Reads the per-zoom map X offsets array of a city. |
| **`GW_GetCityMapYOffsets(index, out, max_count)`** | `bool` | Reads the per-zoom map Y offsets array of a city. |
| **`GW_SetCityMapXOffsets(index, values, count)`** | `bool` | Writes the per-zoom map X offsets array of a city. |
| **`GW_SetCityMapYOffsets(index, values, count)`** | `bool` | Writes the per-zoom map Y offsets array of a city. |
| **`GW_GetCityPriceCoef(index)`** | `float` | Gets the city price coefficient. |
| **`GW_SetCityPriceCoef(index, val)`** | `bool` | Sets the city price coefficient. |
| **`GW_GetCityCountry(index)`** | `uint32_t` | Gets the country ID reference of a city. |
| **`GW_SetCityCountry(index, val)`** | `bool` | Sets the country ID reference of a city. |
| **`GW_GetCityPopulation(index)`** | `uint32_t` | Gets the population of a city. |
| **`GW_SetCityPopulation(index, val)`** | `bool` | Sets the population of a city. |
| **`GW_GetCityKeyCity(index)`** | `bool` | Checks if a city is marked as a key city. |
| **`GW_SetCityKeyCity(index, val)`** | `bool` | Sets the key city flag of a city. |
| **`GW_GetCityTimeZone(index)`** | `uint32_t` | Gets the time zone ID of a city. |
| **`GW_SetCityTimeZone(index, val)`** | `bool` | Sets the time zone ID of a city. |
