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
void MyPlugin_OnActivated() {
    if (s_coreAPI && s_coreAPI->telemetry) {
        s_telemetryHandle = s_coreAPI->telemetry->GetContext("MyPlugin");
    }
}
```

### 1. Polling (Using `Get...()` Functions)

**Method:** Directly call `Get...()` functions (e.g., `GetTruckData`, `GetJobConstants`) to retrieve a snapshot of the current telemetry data.

**Use Cases:**
*   **Infrequent Data Retrieval:** When you only need to check a value occasionally, for example, when a user opens a UI window or presses a specific key.
*   **UI Display (Low Frequency):** For displaying data that doesn't need to be updated every single frame, such as a summary panel that refreshes every few seconds.
*   **One-off Checks:** To get the initial state of a telemetry value.

**Advantages:** Simple to implement, easy to understand.
**Disadvantages:** Can be inefficient if used for high-frequency updates, as the plugin actively requests data even if it hasn't changed.

### 2. Event-Driven (Using `RegisterFor...()` Functions)

**Method:** Subscribe to specific telemetry events. The framework will call your registered callback function *only when the relevant data changes*. This method uses a RAII (Resource Acquisition Is Initialization) approach for managing subscriptions.

**Use Cases:**
*   **High-Frequency Data Monitoring:** Ideal for data that changes every frame or very frequently (e.g., speed, RPM, controls).
*   **Performance-Critical Logic:** When your plugin needs to react immediately and efficiently to changes without constantly polling.
*   **Reduced CPU Usage:** Your plugin code is only executed when necessary.

**Advantages:** Highly efficient, reactive, and automatically manages the subscription lifecycle.
**Disadvantages:** Requires defining callback functions and handling an event-driven flow.

### Workflow for Event-Driven Subscriptions:

1.  **Define Callback:** Create a C-style callback function matching the signature for the event you need (e.g., `SPF_Telemetry_TruckData_Callback`).
2.  **Register Callback:** Call the corresponding `RegisterFor...()` function (e.g., `RegisterForTruckData`), passing your context handle, the callback function, and any user data. This function will return a `SPF_Telemetry_Callback_Handle*`.
3.  **Automatic Lifetime Management:** The returned `SPF_Telemetry_Callback_Handle*` represents the subscription. You are **no longer required to manually unregister it**. The framework automatically manages the lifetime of this subscription. When your plugin's main `SPF_Telemetry_Handle` (obtained via `GetContext`) is destroyed during plugin shutdown, all associated callback subscriptions are automatically and safely unregistered.



## Function Reference

The API consists of a series of getter functions that populate C structs (defined in `SPF_TelemetryData.h`) with telemetry data.

| Function | Populates Struct | Description |
|---|---|---|
| `GetGameState` | `SPF_GameState*` | General game version and state info. |
| `GetTimestamps` | `SPF_Timestamps*` | Simulation and render timestamps. |
| `GetCommonData` | `SPF_CommonData*` | Common data like game time and rest stops. |
| `GetTruckConstants`| `SPF_TruckConstants*`| Static configuration of the player's truck. |
| `GetTruckData` | `SPF_TruckData*` | Live, dynamic data for the player's truck. |
| `GetTrailers` | `SPF_Trailer[]` | Data for all attached trailers. |
| `GetJobConstants` | `SPF_JobConstants*`| Static information about the current job. |
| `GetJobData` | `SPF_JobData*` | Dynamic data about the current job. |
| `GetNavigationData`| `SPF_NavigationData*`| Data from the in-game GPS. |
| `GetControls` | `SPF_Controls*` | Player control input data. |
| `GetSpecialEvents`| `SPF_SpecialEvents*` | Flags for one-time gameplay events. |
| `GetGameplayEvents`| `SPF_GameplayEvents*`| Detailed data for the most recent event. |
| `GetGearboxConstants`|`SPF_GearboxConstants*`| H-shifter layout information. |

## Event-Driven Registration Reference

This section lists the functions used to subscribe to telemetry data updates. These functions follow a RAII pattern, returning a handle that automatically manages the subscription's lifetime.

| Function | Callback Signature | Description |
|---|---|---|
| `RegisterForGameState` | `SPF_Telemetry_GameState_Callback` | Registers for general game state changes. |
| `RegisterForTimestamps` | `SPF_Telemetry_Timestamps_Callback` | Registers for game time and timestamp updates. |
| `RegisterForCommonData` | `SPF_Telemetry_CommonData_Callback` | Registers for common, frequently-updated data. |
| `RegisterForTruckConstants`| `SPF_Telemetry_TruckConstants_Callback`| Registers for static truck configuration changes. |
| `RegisterForTruckData` | `SPF_Telemetry_TruckData_Callback` | Registers for live, dynamic truck data updates. |
| `RegisterForTrailerConstants`| `SPF_Telemetry_TrailerConstants_Callback`| Registers for static trailer configuration changes. |
| `RegisterForTrailers` | `SPF_Telemetry_Trailers_Callback` | Registers for live data updates for active trailers. The callback receives a filtered list. |
| `RegisterForJobConstants` | `SPF_Telemetry_JobConstants_Callback`| Registers for static job information changes. |
| `RegisterForJobData` | `SPF_Telemetry_JobData_Callback` | Registers for dynamic job data updates. |
| `RegisterForNavigationData`| `SPF_Telemetry_NavigationData_Callback`| Registers for in-game GPS data updates. |
| `RegisterForControls` | `SPF_Telemetry_Controls_Callback` | Registers for player control input updates. |
| `RegisterForSpecialEvents`| `SPF_Telemetry_SpecialEvents_Callback`| Registers for one-time gameplay event flags. |
| `RegisterForGameplayEvents`| `SPF_Telemetry_GameplayEvents_Callback`| Registers for detailed data for the most recent event. |
| `RegisterForGearboxConstants`|`SPF_Telemetry_GearboxConstants_Callback`| Registers for H-shifter layout information changes. |

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
    s_coreAPI->telemetry->GetTruckData(s_telemetryHandle, &truck_data);

    // Convert speed from m/s to kph for display
    float current_speed_kph = truck_data.speed * 3.6f;
    
    // Format and log the speed (using other APIs)
    char buffer[128];
    s_coreAPI->formatting->Format(buffer, sizeof(buffer), "Current speed: %.2f kph", current_speed_kph);
    
    // Use throttled logging to avoid spamming the log file every frame
    s_coreAPI->logger->LogThrottled(
        s_coreAPI->logger->GetLogger("MyPlugin"),
        SPF_LOG_INFO, 
        "myplugin.speedlog", // A unique key for this log message
        1000, // Log at most once every 1000ms (1 second)
        buffer
    );
}
```
