# SPF Vehicle API

The SPF Vehicle API provides an interface for interacting with the game's traffic system and individual vehicle objects. It uses an opaque handle system (`SPF_VehicleHandle`) to ensure maximum stability across game updates.

## Getting the API

Request the Vehicle API from the framework during your plugin's initialization.

**Example: C**
```c
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_Vehicle_API.h"

// Global pointer to the Vehicle API
SPF_Vehicle_API* s_vehicleAPI = NULL;

SPF_PLUGIN_ENTRY void MyPlugin_Init(const SPF_Plugin_Init_Params* params) {
    s_vehicleAPI = (SPF_Vehicle_API*)params->GetAPI(SPF_API_VEHICLE);
}
```

## Key Concepts

1.  **Handle-Based**: Most functions require an `SPF_VehicleHandle`. Think of it as a "key" to a specific vehicle.
2.  **Opaque Objects**: Handles point to internal game memory. Never attempt to read from them directly; always use the provided `Veh_Get...` functions.
3.  **Typedefs**: All API functions are defined via typedefs for consistency (e.g., `SPF_Veh_GetCurrentSpeed_t`).

## Usage Example

```c
// 1. Get the player's truck
SPF_VehicleHandle hPlayer = s_vehicleAPI->Veh_GetPlayerVehicle();

if (hPlayer) {
    // 2. Read properties
    float speed = s_vehicleAPI->Veh_GetCurrentSpeed(hPlayer);
    float patience = s_vehicleAPI->Veh_GetPatience(hPlayer);
    
    printf("Truck Speed: %.2f m/s, Driver Patience: %.2f\n", speed, patience);
}

// 3. Scan all traffic
SPF_VehicleHandle handles[100];
uint32_t count = s_vehicleAPI->Veh_GetAllHandles(handles, 100);

for (uint32_t i = 0; i < count; i++) {
    int32_t id = s_vehicleAPI->Veh_GetId(handles[i]);
    // Do something with each vehicle...
}
```

## Function Reference

### Management & Discovery

---
**`bool Veh_IsReady()`**
Checks if the Vehicle Service is initialized.

---
**`SPF_VehicleHandle Veh_GetPlayerVehicle()`**
Returns a handle to the vehicle currently controlled by the player. Returns `NULL` if not in a vehicle.

---
**`SPF_VehicleHandle Veh_GetVehicleById(int32_t id)`**
Finds a vehicle in the traffic system by its unique ID. Returns `NULL` if no vehicle with that ID exists.

---
**`uint32_t Veh_GetCount()`**
Returns the total number of vehicles currently tracked by the game.

---
**`uint32_t Veh_GetAllHandles(SPF_VehicleHandle* out_handles, uint32_t max_count)`**
Fills an array with handles for all active vehicles. Returns the actual number of handles written.

---
**`uintptr_t Veh_GetTrafficManagerPtr()`**
Returns the raw memory address of the global Traffic Manager object.

---
**`uintptr_t Veh_GetLocalPlayerControllerPtr()`**
Returns the raw memory address of the Local Player Controller.

### Framework & Service Status

---
**`bool Veh_AreAllOffsetsFound()`**
Checks if all required memory patterns for the vehicle system were successfully found.

---
**`bool Veh_IsFinderReady(const char* finderName)`**
Checks if a specific vehicle data finder (e.g., "ObjectManager") is ready.

---
**`bool Veh_RefreshOffsets()`**
Forces a re-scan of game memory for all vehicle-related offsets.

### Vehicle Properties

These functions retrieve real-time data for a given vehicle handle `h`.

| Function | Return Type | Description |
|---|---|---|
| **`Veh_GetId(h)`** | `int32_t` | The unique traffic ID (-1 for player). |
| **`Veh_GetRawAddress(h)`** | `uintptr_t` | The absolute memory address of the Actor. |
| **`Veh_GetPatience(h)`** | `float` | AI driver's patience level (0.0 to 1.0). |
| **`Veh_GetSafety(h)`** | `float` | AI driver's safety margin factor. |
| **`Veh_GetTargetSpeed(h)`** | `float` | Speed the vehicle is trying to reach (m/s). |
| **`Veh_GetSpeedLimit(h)`** | `float` | Current legal speed limit for this vehicle (m/s). |
| **`Veh_GetLaneSpeedInput(h)`** | `float` | Internal lane speed calculation parameter. |
| **`Veh_GetCurrentSpeed(h)`** | `float` | Current actual speed of the vehicle (m/s). |
| **`Veh_GetAcceleration(h)`** | `float` | Current instantaneous acceleration (m/s²). |
