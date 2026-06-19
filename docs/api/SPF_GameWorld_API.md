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
