#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "SPF_TelemetryData.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare the handle type to make it an opaque pointer for the C API
typedef struct SPF_Telemetry_Handle SPF_Telemetry_Handle;

/**
 * @struct SPF_Telemetry_API
 * @brief API for accessing telemetry data from the game.
 *
 * @details This API provides a set of functions to retrieve various pieces of
 *          telemetry data. The data is organized into several structures,
 *          separating static 'constant' data from dynamic, frequently-updated data.
 *
 * @section Workflow
 * 1.  **Get Context**: In your `OnLoad` function, call `GetContext()` to get a
 *     handle for your plugin. This handle is required for all other calls.
 * 2.  **Call Getters**: Use the function pointers in this struct to retrieve the
 *     desired data. For example, to get the truck's current speed, you would
 *     first get the `SPF_TruckData` struct and then access the `speed` member.
 *
 * @section Data Types
 * - **Constants**: Data that describes the configuration of the truck, trailer,
 *   or job (e.g., brand name, fuel capacity, gear ratios). This data only
 *   changes when the configuration changes (e.g., buying a new truck).
 * - **Data**: Dynamic data that changes frequently (e.g., speed, RPM, wheel
 *   rotation, world position). This is the data you would typically poll in
 *   your `OnUpdate` function or a separate thread.
 *
 * @section Example
 * @code{.cpp}
 * const SPF_Telemetry_API* telemetry_api = ...; // Obtained from the framework
 * SPF_Telemetry_Handle* tel_handle = telemetry_api->GetContext("MyPlugin");
 *
 * void OnUpdate() {
 *     SPF_TruckData truck_data;
 *     telemetry_api->GetTruckData(tel_handle, &truck_data);
 *
 *     float current_speed_kph = truck_data.speed * 3.6f;
 *     Log("Current speed: %.2f kph", current_speed_kph);
 * }
 * @endcode
 */
typedef struct SPF_Telemetry_API {
    /**
     * @brief Gets a telemetry context handle for the plugin.
     * @param pluginName The name of the plugin requesting the context.
     * @return A handle to the telemetry context, required for all other calls.
     */
    SPF_Telemetry_Handle* (*GetContext)(const char* pluginName);

    /**
     * @brief Retrieves general game state information.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GameState` struct to be filled.
     */
    void (*GetGameState)(SPF_Telemetry_Handle* handle, SPF_GameState* out_data);

    /**
     * @brief Retrieves various game timestamps.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_Timestamps` struct to be filled.
     */
    void (*GetTimestamps)(SPF_Telemetry_Handle* handle, SPF_Timestamps* out_data);

    /**
     * @brief Retrieves common, frequently updated data.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_CommonData` struct to be filled.
     */
    void (*GetCommonData)(SPF_Telemetry_Handle* handle, SPF_CommonData* out_data);

    /**
     * @brief Retrieves static configuration data for the player's truck.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_TruckConstants` struct to be filled.
     */
    void (*GetTruckConstants)(SPF_Telemetry_Handle* handle, SPF_TruckConstants* out_data);

    /**
     * @brief Retrieves dynamic, live data for the player's truck.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_TruckData` struct to be filled.
     */
    void (*GetTruckData)(SPF_Telemetry_Handle* handle, SPF_TruckData* out_data);

    /**
     * @brief Retrieves data for all attached trailers.
     * @param handle The telemetry context handle.
     * @param[out] out_trailers A pointer to an array of `SPF_Trailer` structs.
     * @param[in,out] in_out_count As input, the capacity of the `out_trailers` array.
     *                             As output, the actual number of trailers written to the array.
     */
    void (*GetTrailers)(SPF_Telemetry_Handle* handle, SPF_Trailer* out_trailers, uint32_t* in_out_count);

    /**
     * @brief Retrieves static information about the current job.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_JobConstants` struct to be filled.
     */
    void (*GetJobConstants)(SPF_Telemetry_Handle* handle, SPF_JobConstants* out_data);

    /**
     * @brief Retrieves dynamic data about the current job.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_JobData` struct to be filled.
     */
    void (*GetJobData)(SPF_Telemetry_Handle* handle, SPF_JobData* out_data);

    /**
     * @brief Retrieves data from the in-game GPS/navigation system.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_NavigationData` struct to be filled.
     */
    void (*GetNavigationData)(SPF_Telemetry_Handle* handle, SPF_NavigationData* out_data);

    /**
     * @brief Retrieves player control input data.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_Controls` struct to be filled.
     */
    void (*GetControls)(SPF_Telemetry_Handle* handle, SPF_Controls* out_data);

    /**
     * @brief Retrieves flags for special one-time gameplay events.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_SpecialEvents` struct to be filled.
     */
    void (*GetSpecialEvents)(SPF_Telemetry_Handle* handle, SPF_SpecialEvents* out_data);

    /**
     * @brief Retrieves detailed data for the most recent gameplay event.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GameplayEvents` struct to be filled.
     */
    void (*GetGameplayEvents)(SPF_Telemetry_Handle* handle, SPF_GameplayEvents* out_data);

    /**
     * @brief Retrieves constants related to the H-shifter gearbox layout.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GearboxConstants` struct to be filled.
     */
    void (*GetGearboxConstants)(SPF_Telemetry_Handle* handle, SPF_GearboxConstants* out_data);

    /**
     * @brief Gets the ID of the last gameplay event that occurred.
     * @param handle The telemetry context handle.
     * @param[out] out_buffer A character buffer to receive the event ID string.
     * @param buffer_size The size of the output buffer.
     * @return The number of characters written to the buffer.
     */
    int (*GetLastGameplayEventId)(SPF_Telemetry_Handle* handle, char* out_buffer, int buffer_size);

} SPF_Telemetry_API;

#ifdef __cplusplus
}
#endif
