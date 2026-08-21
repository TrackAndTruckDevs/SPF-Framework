# SPF Telemetry API

The SPF Telemetry API provides plugins with read-only access to a rich set of data from the game, based on the official SCS Telemetry SDK. This includes everything from the truck's speed and RPM to detailed job information and gameplay events.

## Data Philosophy: Constants vs. Data

The API separates data into two main categories to optimize performance:

*   **Constants**: Static data that describes the configuration of the truck, trailer, or job. This data rarely changes during gameplay (e.g., truck brand, fuel capacity, gear ratios). You typically only need to fetch this data once, for example, in your `OnActivated` function.
*   **Data**: Dynamic data that changes frequently, often every frame (e.g., speed, RPM, wheel rotation, world position). This is the data you would typically poll for in your `OnUpdate` function or a separate, high-frequency thread.

## API Usage Workflow: Polling vs. Event-Driven

The Telemetry API offers two distinct methods for accessing data, each suited for different use cases. Both approaches require obtaining a context handle once.

### Getting the Context Handle

First, retrieve your plugin's telemetry context handle. This handle is crucial as it manages your subscriptions and data requests.

```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Telemetry_API.h"

// Assume s_coreAPI is already assigned in your plugin's OnLoad function
const SPF_Core_API* s_coreAPI = NULL; 
SPF_Telemetry_Handle* s_telemetryHandle = NULL;

// In your OnActivated function (or similar plugin lifecycle entry point):
void MyPlugin_OnActivated(const SPF_Core_API* api) {
    if (api && api->telemetry) {
        s_telemetryHandle = api->telemetry->Tel_GetContext("MyPlugin");
    }
}
```

### 1. Polling (Using `Tel_Get...()` Functions)

**Method:** Directly call `Tel_Get...()` functions (e.g., `Tel_GetTruckData`, `Tel_GetJobConstants`) to retrieve a snapshot of the current telemetry data.

> [!IMPORTANT]
> **ABI Stability**: When calling polling functions, you MUST provide the size of your local structure (e.g., `sizeof(SPF_TruckData)`). This allows the framework to safely populate your data even if new fields are added to the API in the future.

**Use Cases:**
*   **Infrequent Data Retrieval:** When you only need to check a value occasionally, for example, when a user opens a UI window or presses a specific key.
*   **UI Display (Low Frequency):** For displaying data that doesn't need to be updated every single frame, such as a summary panel that refreshes every few seconds.
*   **One-off Checks:** To get the initial state of a telemetry value.

**Advantages:** Simple to implement, easy to understand.
**Disadvantages:** Can be inefficient if used for high-frequency updates, as the plugin actively requests data even if it hasn't changed.

### 2. Event-Driven (Using `Tel_RegisterFor...()` Functions)

**Method:** Subscribe to specific telemetry events. The framework will call your registered callback function *only when the relevant data changes*. This method uses a RAII (Resource Acquisition Is Initialization) approach for managing subscriptions.

**Use Cases:**
*   **High-Frequency Data Monitoring:** Ideal for data that changes every frame or very frequently (e.g., speed, RPM, controls).
*   **Performance-Critical Logic:** When your plugin needs to react immediately and efficiently to changes without constantly polling.
*   **Reduced CPU Usage:** Your plugin code is only executed when necessary.

**Advantages:** Highly efficient, reactive, and automatically manages the subscription lifecycle.
**Disadvantages:** Requires defining callback functions and handling an event-driven flow.

### Workflow for Event-Driven Subscriptions:

1.  **Define Callback:** Create a C-style callback function matching the signature for the event you need (e.g., `SPF_Telemetry_TruckData_Callback`).
2.  **Register Callback:** Call the corresponding `Tel_RegisterFor...()` function (e.g., `Tel_RegisterForTruckData`), passing your context handle, the callback function, and any user data. This function will return a `SPF_Telemetry_Callback_Handle*`.
3.  **Automatic Lifetime Management:** The returned `SPF_Telemetry_Callback_Handle*` represents the subscription. You are **no longer required to manually unregister it**. The framework automatically manages the lifetime of this subscription. When your plugin's main `SPF_Telemetry_Handle` (obtained via `Tel_GetContext`) is destroyed during plugin shutdown, all associated callback subscriptions are automatically and safely unregistered.



## Function Reference

The API consists of a series of getter functions that populate C structs (defined in `SPF_TelemetryData.h`) with telemetry data.

| Function | Populates Struct | Description |
|---|---|---|
| `Tel_GetGameState` | `SPF_GameState*` | General game version and state info. Requires `struct_size`. |
| `Tel_GetTimestamps` | `SPF_Timestamps*` | Simulation and render timestamps. Requires `struct_size`. |
| `Tel_GetCommonData` | `SPF_CommonData*` | Common data like game time and rest stops. Requires `struct_size`. |
| `Tel_GetTruckConstants`| `SPF_TruckConstants*`| Static configuration of the player's truck. Requires `struct_size`. |
| `Tel_GetTruckData` | `SPF_TruckData*` | Live, dynamic data for the player's truck. Requires `struct_size`. |
| `Tel_GetTrailers` | `SPF_Trailer[]` | Data for all attached trailers. Requires `struct_size` of a single element. |
| `Tel_GetJobConstants` | `SPF_JobConstants*`| Static information about the current job. Requires `struct_size`. |
| `Tel_GetJobData` | `SPF_JobData*` | Dynamic data about the current job. Requires `struct_size`. |
| `Tel_GetNavigationData`| `SPF_NavigationData*`| Data from the in-game GPS. Requires `struct_size`. |
| `Tel_GetControls` | `SPF_Controls*` | Player control input data. Requires `struct_size`. |
| `Tel_GetSpecialEvents`| `SPF_SpecialEvents*` | Flags for one-time gameplay events. Requires `struct_size`. |
| `Tel_GetGameplayEvents`| `SPF_GameplayEvents*`| Detailed data for the most recent event. Requires `struct_size`. |
| `Tel_GetGearboxConstants`|`SPF_GearboxConstants*`| H-shifter layout information. Requires `struct_size`. |

## Event-Driven Registration Reference

This section lists the functions used to subscribe to telemetry data updates.

| Function | Callback Signature | Description |
|---|---|---|
| `Tel_RegisterForGameState` | `SPF_Telemetry_GameState_Callback` | Registers for general game state changes. |
| `Tel_RegisterForTimestamps` | `SPF_Telemetry_Timestamps_Callback` | Registers for game time and timestamp updates. |
| `Tel_RegisterForCommonData` | `SPF_Telemetry_CommonData_Callback` | Registers for common, frequently-updated data. |
| `Tel_RegisterForTruckConstants`| `SPF_Telemetry_TruckConstants_Callback`| Registers for static truck configuration changes. |
| `Tel_RegisterForTruckData` | `SPF_Telemetry_TruckData_Callback` | Registers for live, dynamic truck data updates. |
| `Tel_RegisterForTrailerConstants`| `SPF_Telemetry_TrailerConstants_Callback`| Registers for static trailer configuration changes. |
| `Tel_RegisterForTrailers` | `SPF_Telemetry_Trailers_Callback` | Registers for live data updates for active trailers. |
| `Tel_RegisterForJobConstants` | `SPF_Telemetry_JobConstants_Callback`| Registers for static job information changes. |
| `Tel_RegisterForJobData` | `SPF_Telemetry_JobData_Callback` | Registers for dynamic job data updates. |
| `Tel_RegisterForNavigationData`| `SPF_Telemetry_NavigationData_Callback`| Registers for in-game GPS data updates. |
| `Tel_RegisterForControls` | `SPF_Telemetry_Controls_Callback` | Registers for player control input updates. |
| `Tel_RegisterForSpecialEvents`| `SPF_Telemetry_SpecialEvents_Callback`| Registers for one-time gameplay event flags. |
| `Tel_RegisterForGameplayEvents`| `SPF_Telemetry_GameplayEvents_Callback`| Registers for detailed data for the most recent event. |
| `Tel_RegisterForGearboxConstants`|`SPF_Telemetry_GearboxConstants_Callback`| Registers for H-shifter layout information changes. |
| `Tel_RegisterForWorldReload` | `SPF_Telemetry_WorldReload_Callback` | Registers for world (re)load detection (raw SCS `timer_restart` signal). |

### World Reload Detection

`Tel_RegisterForWorldReload` exposes the raw SCS SDK signal (`SCS_TELEMETRY_FRAME_START_FLAG_timer_restart`): the game's frame timers were reset and started counting from zero again, which happens every time a new world starts loading — including the very first load.

> [!NOTE]
> This is the **raw** SDK signal. For framework-ordered lifecycle events use the plugin exports instead: `OnWorldUnloaded` fires before the framework resets its world-scoped services, and `OnGameWorldReady` fires when the new world is fully loaded. The telemetry callback may fire earlier in the same frame than both of them.

```c
void OnWorldReload(void* user_data) {
    // A new world started loading (raw SCS timer_restart signal).
}

// In OnActivated:
s_coreAPI->telemetry->Tel_RegisterForWorldReload(s_telemetryHandle, OnWorldReload, NULL);
```

## Data Structure Reference

This section details the most commonly used data structures. For a complete list of all fields, please refer to `SPF_TelemetryData.h`.

---
### `SPF_TruckData`
Contains dynamic, frequently changing data about the player's truck.

**Key Fields:**
*   `float speed`: Speed of the truck in meters/second.
*   `float engine_rpm`: Current engine RPM.
*   `int32_t gear`: Currently selected gear (0=N, >0=F, <0=R).
*   `float fuel_amount`: Current amount of fuel in liters.
*   `SPF_DPlacement world_placement`: High-precision world position and orientation.
*   `float wear_engine`, `wear_transmission`, etc.: Wear levels from 0.0 (no wear) to 1.0 (max wear).
*   `bool lblinker`, `rblinker`: State of the blinkers.
*   `SPF_WheelData wheels[...]`: An array containing live data for each wheel (suspension, on_ground status, etc.).

---
### `SPF_TruckConstants`
Contains static, unchanging properties of the player's truck.

**Key Fields:**
*   `char brand[256]`: Display name of the truck's brand (e.g., "Scania").
*   `char name[256]`: Display name of the truck model (e.g., "S 730").
*   `float fuel_capacity`: Maximum fuel tank capacity in liters.
*   `uint32_t forward_gear_count`: Number of forward gears.
*   `float gear_ratios_forward[...]`: Array of forward gear ratios.
*   `uint32_t wheel_count`: Number of wheels on the truck.
*   `SPF_WheelConstants wheels[...]`: An array of static data for each wheel (radius, position, etc.).

---
### `SPF_JobData` & `SPF_JobConstants`
These structs describe the current job. `SPF_JobConstants` contains information that doesn't change during a job, while `SPF_JobData` contains live information.

**Key Constant Fields (`SPF_JobConstants`):**
*   `uint64_t income`: The total income for completing the job.
*   `char cargo_name[256]`: Display name of the cargo.
*   `float cargo_mass`: Total mass of the cargo in kilograms.
*   `char destination_city[256]`: Display name of the destination city.
*   `char destination_company[256]`: Display name of the destination company.

**Key Data Fields (`SPF_JobData`):**
*   `bool on_job`: Is the player currently on a job?
*   `float cargo_damage`: Current damage to the cargo (0.0 to 1.0).
*   `uint32_t remaining_delivery_minutes`: Remaining time for the delivery in in-game minutes.

---
### `SPF_SpecialEvents` & `SPF_GameplayEvents`
`SPF_SpecialEvents` contains boolean flags that become `true` for a single frame when a specific event occurs. When a flag is true, you can then query the `SPF_GameplayEvents` struct to get detailed information about that event.

**Example Event Flags (`SPF_SpecialEvents`):**
*   `bool job_delivered`: True for one frame when a job is delivered.
*   `bool fined`: True for one frame when the player is fined.

When `fined` is true, the `player_fined` member of the `SPF_GameplayEvents` struct will be populated with details like the `fine_amount` and `fine_offence`.

## Complete Example

This example shows how to get the current speed in `OnUpdate` and log it.

```c
// Assumes s_coreAPI and s_telemetryHandle are initialized.

void MyPlugin_OnUpdate() {
    if (!s_coreAPI || !s_telemetryHandle) return;

    SPF_TruckData truck_data;
    // Pass sizeof(truck_data) for ABI stability
    s_coreAPI->telemetry->Tel_GetTruckData(s_telemetryHandle, &truck_data, sizeof(SPF_TruckData));

    // Convert speed from m/s to kph for display
    float current_speed_kph = truck_data.speed * 3.6f;
    
    // Format and log the speed (using Formatting API)
    char buffer[128];
    s_coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Current speed: %.2f kph", current_speed_kph);
    
    // Use throttled logging to avoid spamming
    s_coreAPI->logger->Log(s_telemetryHandle, SPF_LOG_INFO, buffer); 
    // Note: Use a throttled wrapper if logging inside OnUpdate!
}
```
