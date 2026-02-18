/**                                                                                               
* @file SPF_Vehicle_API.h                                                                         
* @brief API for inspecting and interacting with vehicles in the game world.
*                                                                                                 
* @details This API provides an interface to the game's traffic system and individual vehicle 
* actors. It uses an opaque handle system (SPF_VehicleHandle) to represent vehicles, 
* ensuring maximum stability and performance.
*                                                                                                 
* ================================================================================================
* KEY CONCEPTS                                                                                    
* ================================================================================================
*                                                                                                 
* 1. **Handle-Based**: Most functions require a vehicle handle (`SPF_VehicleHandle`). Get a 
*    handle for the player's truck using `Veh_GetPlayerVehicle()` or scan the world using 
*    `Veh_GetAllHandles()`.
*                                                                                                 
* 2. **Opaque Objects**: The handles point to internal game objects. Do not attempt to 
*    dereference them directly. Always use the provided getter functions.
*                                                                                                 
* 3. **Validation**: Before using a handle obtained in a previous frame, it is recommended 
*    to verify if the vehicle still exists (e.g., by checking if its ID is still valid).
*                                                                                                 
* ================================================================================================
* USAGE EXAMPLE (C++)                                                                             
* ================================================================================================
* @code                                                                                           
* // Get the player's current vehicle handle
* SPF_VehicleHandle h = api->Vehicle->Veh_GetPlayerVehicle();
*
* if (h) {
*     // Retrieve properties using the handle 'h'
*     float speed = api->Vehicle->Veh_GetCurrentSpeed(h);
*     int32_t id = api->Vehicle->Veh_GetId(h);
*     
*     printf("Player Truck [ID: %d] Speed: %.2f m/s\n", id, speed);
* }
* @endcode                                                                                        
*/ 

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =================================================================================================
// Types                                                                                            
// =================================================================================================

/**                                                                                                 
 * @brief Opaque handle to a vehicle object (Actor) in the game memory.
 */ 
typedef void* SPF_VehicleHandle;

// --- Function Typedefs ---

/**
 * @brief Checks if the Vehicle Service is fully initialized and offsets are found.
 * @return True if the service is ready for use.
 */
typedef bool (*SPF_Veh_IsReady_t)();

/**
 * @brief Gets the handle for the vehicle currently controlled by the player.
 * @return A valid handle, or NULL if the player is not in a vehicle.
 */
typedef SPF_VehicleHandle (*SPF_Veh_GetPlayerVehicle_t)();

/**
 * @brief Finds a vehicle handle by its unique traffic ID.
 * @param id The traffic identifier assigned by the game.
 * @return A handle to the vehicle, or NULL if not found.
 */
typedef SPF_VehicleHandle (*SPF_Veh_GetVehicleById_t)(int32_t id);

/**
 * @brief Gets the total number of active vehicles (AI and player) in the world.
 * @return The count of active vehicles.
 */
typedef uint32_t (*SPF_Veh_GetCount_t)();

/**
 * @brief Fills a provided array with handles for all active vehicles.
 * @param[out] out_handles Pointer to an array to receive the handles.
 * @param max_count The maximum number of handles the array can store.
 * @return The number of handles actually written to the array.
 */
typedef uint32_t (*SPF_Veh_GetAllHandles_t)(SPF_VehicleHandle* out_handles, uint32_t max_count);

/**
 * @brief Gets the raw memory address of the global Traffic Manager object.
 * @return The absolute memory address (uintptr_t).
 */
typedef uintptr_t (*SPF_Veh_GetTrafficManagerPtr_t)();

/**
 * @brief Gets the raw memory address of the Local Player Controller.
 * This object resides within the Traffic Manager and controls the player's truck.
 * @return The absolute memory address (uintptr_t).
 */
typedef uintptr_t (*SPF_Veh_GetLocalPlayerControllerPtr_t)();

// --- Framework & Service Status ---

/** 
 * @brief Checks if all memory offsets for the vehicle system were successfully found. 
 * @return True if all dynamic patterns were successfully resolved.
 */
typedef bool (*SPF_Veh_AreAllOffsetsFound_t)();

/** 
 * @brief Checks if a specific vehicle data finder is ready. 
 * @param finderName The name of the finder to check.
 * @return True if the specific offsets are found and valid.
 */
typedef bool (*SPF_Veh_IsFinderReady_t)(const char* finderName);

/** 
 * @brief Forces the framework to re-scan game memory for all vehicle-related offsets. 
 * @return True if all offsets were successfully found after the refresh operation.
 */
typedef bool (*SPF_Veh_RefreshOffsets_t)();

// --- Vehicle Property Accessors ---

/**
 * @brief Retrieves the unique traffic identifier for a vehicle.
 * @param h The vehicle handle.
 * @return The traffic ID, or -1 for the player's truck.
 */
typedef int32_t (*SPF_Veh_GetId_t)(SPF_VehicleHandle h);

/**
 * @brief Gets the absolute memory address of the vehicle object.
 * @param h The vehicle handle.
 * @return The raw uintptr_t memory address.
 */
typedef uintptr_t (*SPF_Veh_GetRawAddress_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the AI driver's patience level.
 * @param h The vehicle handle.
 * @return The patience value (typically 0.0 to 1.0).
 */
typedef float (*SPF_Veh_GetPatience_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the AI driver's safety margin factor.
 * @param h The vehicle handle.
 * @return The safety factor.
 */
typedef float (*SPF_Veh_GetSafety_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the speed the vehicle is currently attempting to reach.
 * @param h The vehicle handle.
 * @return The target speed in meters per second (m/s).
 */
typedef float (*SPF_Veh_GetTargetSpeed_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the current legal or logic-imposed speed limit for the vehicle.
 * @param h The vehicle handle.
 * @return The speed limit in meters per second (m/s).
 */
typedef float (*SPF_Veh_GetSpeedLimit_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the internal lane speed input value.
 * @param h The vehicle handle.
 * @return The lane speed input value.
 */
typedef float (*SPF_Veh_GetLaneSpeedInput_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the current real-world speed of the vehicle.
 * @param h The vehicle handle.
 * @return The speed in meters per second (m/s).
 */
typedef float (*SPF_Veh_GetCurrentSpeed_t)(SPF_VehicleHandle h);

/**
 * @brief Retrieves the current instantaneous acceleration of the vehicle.
 * @param h The vehicle handle.
 * @return The acceleration in meters per second squared (m/s^2).
 */
typedef float (*SPF_Veh_GetAcceleration_t)(SPF_VehicleHandle h);


/**
 * @struct SPF_Vehicle_API
 * @brief API for interacting with the game's traffic and vehicle system.
 */
typedef struct SPF_Vehicle_API {
    SPF_Veh_IsReady_t Veh_IsReady;
    SPF_Veh_GetPlayerVehicle_t Veh_GetPlayerVehicle;
    SPF_Veh_GetVehicleById_t Veh_GetVehicleById;
    SPF_Veh_GetCount_t Veh_GetCount;
    SPF_Veh_GetAllHandles_t Veh_GetAllHandles;
    
    SPF_Veh_GetTrafficManagerPtr_t Veh_GetTrafficManagerPtr;
    SPF_Veh_GetLocalPlayerControllerPtr_t Veh_GetLocalPlayerControllerPtr;

    SPF_Veh_AreAllOffsetsFound_t Veh_AreAllOffsetsFound;
    SPF_Veh_IsFinderReady_t Veh_IsFinderReady;
    SPF_Veh_RefreshOffsets_t Veh_RefreshOffsets;

    SPF_Veh_GetId_t Veh_GetId;
    SPF_Veh_GetRawAddress_t Veh_GetRawAddress;
    SPF_Veh_GetPatience_t Veh_GetPatience;
    SPF_Veh_GetSafety_t Veh_GetSafety;
    SPF_Veh_GetTargetSpeed_t Veh_GetTargetSpeed;
    SPF_Veh_GetSpeedLimit_t Veh_GetSpeedLimit;
    SPF_Veh_GetLaneSpeedInput_t Veh_GetLaneSpeedInput;
    SPF_Veh_GetCurrentSpeed_t Veh_GetCurrentSpeed;
    SPF_Veh_GetAcceleration_t Veh_GetAcceleration;
} SPF_Vehicle_API;

#ifdef __cplusplus
}
#endif
