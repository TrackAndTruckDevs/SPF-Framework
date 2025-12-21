#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "SPF_TelemetryData.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward-declare the handle type to make it an opaque pointer for the C API
typedef struct SPF_Telemetry_Handle SPF_Telemetry_Handle;
typedef struct SPF_Telemetry_Callback_Handle SPF_Telemetry_Callback_Handle;

// =================================================================================================
// Callback Definitions
// =================================================================================================
// These function pointer types define the signatures for callbacks that plugins can register
// to receive telemetry data updates in an event-driven manner.

/**
 * @brief Callback for when game state data (e.g., pause status, game version) is updated.
 * @param data A pointer to the structure containing the latest game state information.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_GameState_Callback)(const SPF_GameState* data, void* user_data);

/**
 * @brief Callback for when game time and timestamp data is updated.
 * @param data A pointer to the structure containing the latest timestamp information.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_Timestamps_Callback)(const SPF_Timestamps* data, void* user_data);

/**
 * @brief Callback for when common, frequently-updated data (e.g., in-game time, rest stop info) is updated.
 * @param data A pointer to the structure containing the latest common telemetry data.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_CommonData_Callback)(const SPF_CommonData* data, void* user_data);

/**
 * @brief Callback for when the truck's static configuration changes (e.g., buying a new truck, upgrading parts).
 * @param data A pointer to the structure containing the new configuration constants for the truck.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_TruckConstants_Callback)(const SPF_TruckConstants* data, void* user_data);

/**
 * @brief Callback for when the truck's dynamic data is updated (fired each frame).
 * @param data A pointer to the structure containing live truck data (speed, RPM, etc.).
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_TruckData_Callback)(const SPF_TruckData* data, void* user_data);

/**
 * @brief Callback for when a trailer's static configuration changes.
 * @param data A pointer to the structure containing the new configuration constants for a trailer.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_TrailerConstants_Callback)(const SPF_TrailerConstants* data, void* user_data);

/**
 * @brief Callback for when the list of attached trailers is updated (fired each frame).
 * @param trailers A pointer to an array of `SPF_Trailer` structures.
 * @param count The number of trailers in the array.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_Trailers_Callback)(const SPF_Trailer* trailers, uint32_t count, void* user_data);

/**
 * @brief Callback for when the current job's static information changes (e.g., starting a new job).
 * @param data A pointer to the structure containing the new configuration constants for the job.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_JobConstants_Callback)(const SPF_JobConstants* data, void* user_data);

/**
 * @brief Callback for when the current job's dynamic data is updated (fired each frame).
 * @param data A pointer to the structure containing live job data (e.g., cargo damage).
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_JobData_Callback)(const SPF_JobData* data, void* user_data);

/**
 * @brief Callback for when navigation and route advisor data is updated (fired each frame).
 * @param data A pointer to the structure containing live navigation data (e.g., distance to destination).
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_NavigationData_Callback)(const SPF_NavigationData* data, void* user_data);

/**
 * @brief Callback for when player control inputs are updated (fired each frame).
 * @param data A pointer to the structure containing player input data (steering, throttle, etc.).
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_Controls_Callback)(const SPF_Controls* data, void* user_data);

/**
 * @brief Callback for when special, single-frame events occur (e.g., job finished, tollgate used).
 *        Note: These are boolean flags that are true for only one frame. For more detailed event
 *        data, use `SPF_Telemetry_GameplayEvents_Callback`.
 * @param data A pointer to the structure containing the special event flags.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_SpecialEvents_Callback)(const SPF_SpecialEvents* data, void* user_data);

/**
 * @brief Callback for when a gameplay event occurs (e.g., a fine, job delivery).
 * @param event_id A UTF-8 encoded, null-terminated string identifying the type of event. To identify
 *                 the event, perform a string comparison against the constants defined in the SCS SDK
 *                 headers (e.g., `SCS_TELEMETRY_GAMEPLAY_EVENT_player_fined`, which has the value "player.fined").
 * @param data A pointer to the gameplay event data structure. You MUST check `event_id` first
 *             to know which field of this struct is valid (e.g., if `event_id` is "player.fined",
 *             you should access the `player_fined` member).
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_GameplayEvents_Callback)(const char* event_id, const SPF_GameplayEvents* data, void* user_data);

/**
 * @brief Callback for when the H-shifter gearbox configuration changes.
 * @param data A pointer to the structure containing the new H-shifter layout constants.
 * @param user_data The custom pointer you provided when registering the callback.
 */
typedef void (*SPF_Telemetry_GearboxConstants_Callback)(const SPF_GearboxConstants* data, void* user_data);


/**
 * @struct SPF_Telemetry_API
 * @brief API for accessing telemetry data from the game.
 *
 * @details
 * This API provides two main ways to access telemetry data:
 * 1.  **Polling (Manual Getters)**: Using the `Get...()` functions (e.g., `GetTruckData`) to request a
 *     "snapshot" of the current data at any time. This is simple and useful for one-off data retrieval.
 * 2.  **Event-Driven (Callback Subscription)**: Using the `RegisterFor...()` functions (e.g., `RegisterForTruckData`).
 *     This is the **strongly recommended method** for handling data that changes frequently, as it is far
 *     more efficient.
 *
 * @section Workflow (Event-Driven)
 * The event-driven system now uses a RAII (Resource Acquisition Is Initialization) approach for safe and
 * automatic management of callback subscriptions.
 *
 * 1.  **Get Context Handle:** In your plugin's activation phase, get your plugin's main telemetry context
 *     handle via `GetContext("MyPluginName")`. This handle will manage all subsequent subscriptions.
 *
 * 2.  **Define Callback:** Create a callback function with the correct signature for the event you need
 *     (e.g., `SPF_Telemetry_TruckData_Callback`).
 *
 * 3.  **Register Callback:** Call the corresponding `RegisterFor...()` function, passing your context handle,
 *     the callback function, and any user data. The function will return a `SPF_Telemetry_Callback_Handle*`.
 *
 * 4.  **Automatic Cleanup:** The returned callback handle represents the subscription. You can store this
 *     handle if you need to reference it, but you are **no longer required to manually unregister it**.
 *     The lifetime of the subscription is automatically tied to the main context handle obtained in Step 1.
 *     When the framework destroys your plugin's context handle during shutdown, all its associated
 *     callback subscriptions are automatically and safely unregistered.
 *
 *
 * @section Example: Reacting to Truck Data Updates
 * @code{.c}
 * // Global context for the plugin
 * MyPluginContext g_my_context;
 *
 * // 1. Define the callback function.
 * void OnTruckData(const SPF_TruckData* data, void* user_data) {
 *     // user_data points to our global context
 *     MyPluginContext* ctx = (MyPluginContext*)user_data;
 *     // Cache the new data
 *     ctx->last_truck_data = *data;
 *     printf("Current speed: %f m/s\n", data->speed);
 * }
 *
 * // In your plugin's activation function (e.g., OnActivated):
 * void ActivatePlugin(const SPF_Core_API* core_api) {
 *     g_my_context.telemetry_api = core_api->telemetry;
 *
 *     // 2. Get the main context handle
 *     g_my_context.telemetry_handle = g_my_context.telemetry_api->GetContext("MyPlugin");
 *
 *     // 3. Register the callback. The returned handle's lifetime is managed automatically.
 *     g_my_context.truck_data_callback_handle = g_my_context.telemetry_api->RegisterForTruckData(
 *         g_my_context.telemetry_handle,
 *         OnTruckData,
 *         &g_my_context
 *     );
 *
 *     // No manual unregister call is needed in OnUnload!
 * }
 * @endcode
 */
typedef struct SPF_Telemetry_API {



    
    /**
     * @brief Gets a telemetry context handle for the plugin. This handle is required for all other calls.
     * @param pluginName The name of the plugin requesting the context. Must not be NULL.
     * @return An opaque handle to the telemetry context, or NULL if an error occurs.
     */
    SPF_Telemetry_Handle* (*GetContext)(const char* pluginName);

    /**
     * @brief Registers a callback for game state changes.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when game state data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForGameState)(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameState_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for timestamp updates.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when timestamp data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForTimestamps)(SPF_Telemetry_Handle* handle, SPF_Telemetry_Timestamps_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for common data updates (e.g., game time).
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when common data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForCommonData)(SPF_Telemetry_Handle* handle, SPF_Telemetry_CommonData_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for when the truck's configuration (e.g., brand, engine) changes.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when truck constant data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForTruckConstants)(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckConstants_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for when a trailer's configuration changes.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when trailer constant data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForTrailerConstants)(SPF_Telemetry_Handle* handle, SPF_Telemetry_TrailerConstants_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for live truck data updates (e.g., speed, RPM), fired every frame.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when truck data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForTruckData)(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckData_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for live trailer data updates, fired every frame.
     * @details The callback receives a pointer to an array of `SPF_Trailer` structs and a count.
     *          This list is automatically filtered by the framework to only include active trailers.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when trailer data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForTrailers)(SPF_Telemetry_Handle* handle, SPF_Telemetry_Trailers_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for when the current job's configuration changes.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when job constant data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForJobConstants)(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobConstants_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for live job data updates (e.g., cargo damage).
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when job data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForJobData)(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobData_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for live navigation data updates.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when navigation data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForNavigationData)(SPF_Telemetry_Handle* handle, SPF_Telemetry_NavigationData_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for live player control input updates.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when control data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForControls)(SPF_Telemetry_Handle* handle, SPF_Telemetry_Controls_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for one-frame special events (e.g., job delivery).
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when a special event occurs.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForSpecialEvents)(SPF_Telemetry_Handle* handle, SPF_Telemetry_SpecialEvents_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for detailed gameplay events (e.g., fines, deliveries).
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when a gameplay event occurs.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForGameplayEvents)(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameplayEvents_Callback callback, void* user_data);

    /**
     * @brief Registers a callback for when the H-shifter gearbox configuration changes.
     * @param handle The telemetry context handle for your plugin.
     * @param callback The function to be called when gearbox constant data is updated.
     * @param user_data Optional user-defined data to be passed to the callback.
     * @return An opaque handle that represents the subscription. Its lifetime is managed automatically
     *         by the parent `SPF_Telemetry_Handle`.
     */
    SPF_Telemetry_Callback_Handle* (*RegisterForGearboxConstants)(SPF_Telemetry_Handle* handle, SPF_Telemetry_GearboxConstants_Callback callback, void* user_data);

    /**
     * @brief Retrieves a snapshot of the current game state (version, pause status, etc.).
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GameState` struct to be filled with data.
     */
    void (*GetGameState)(SPF_Telemetry_Handle* handle, SPF_GameState* out_data);

    /**
     * @brief Retrieves a snapshot of the current game timestamps.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_Timestamps` struct to be filled with data.
     */
    void (*GetTimestamps)(SPF_Telemetry_Handle* handle, SPF_Timestamps* out_data);

    /**
     * @brief Retrieves a snapshot of common, frequently updated data (game time, rest stops).
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_CommonData` struct to be filled with data.
     */
    void (*GetCommonData)(SPF_Telemetry_Handle* handle, SPF_CommonData* out_data);

    /**
     * @brief Retrieves the static configuration data for the player's current truck (e.g., brand, fuel capacity).
     * This data only changes when the truck configuration changes.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_TruckConstants` struct to be filled with data.
     */
    void (*GetTruckConstants)(SPF_Telemetry_Handle* handle, SPF_TruckConstants* out_data);

    /**
     * @brief Retrieves a snapshot of the dynamic, live data for the player's truck (e.g., speed, RPM).
     * For continuous monitoring, it is highly recommended to use `RegisterForTruckData` instead.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_TruckData` struct to be filled with data.
     */
    void (*GetTruckData)(SPF_Telemetry_Handle* handle, SPF_TruckData* out_data);

    /**
     * @brief Retrieves a snapshot of data for all active trailers.
     * @details This function automatically filters the raw trailer data from the game to return only
     *          active trailers (i.e., those that are connected or have a valid ID).
     * @param handle The telemetry context handle.
     * @param[out] out_trailers A pointer to an array of `SPF_Trailer` structs. The caller is responsible for allocating enough space.
     * @param[in,out] in_out_count As input, this must point to a `uint32_t` holding the capacity of the `out_trailers` array.
     *                             As output, the value will be updated with the actual number of active trailers written to the array.
     */
    void (*GetTrailers)(SPF_Telemetry_Handle* handle, SPF_Trailer* out_trailers, uint32_t* in_out_count);

    /**
     * @brief Retrieves static information about the current job.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_JobConstants` struct to be filled with data.
     */
    void (*GetJobConstants)(SPF_Telemetry_Handle* handle, SPF_JobConstants* out_data);

    /**
     * @brief Retrieves a snapshot of dynamic data about the current job.
     * For continuous monitoring, it is highly recommended to use `RegisterForJobData` instead.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_JobData` struct to be filled with data.
     */
    void (*GetJobData)(SPF_Telemetry_Handle* handle, SPF_JobData* out_data);

    /**
     * @brief Retrieves a snapshot of data from the in-game GPS and route advisor.
     * For continuous monitoring, it is highly recommended to use `RegisterForNavigationData` instead.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_NavigationData` struct to be filled with data.
     */
    void (*GetNavigationData)(SPF_Telemetry_Handle* handle, SPF_NavigationData* out_data);

    /**
     * @brief Retrieves a snapshot of player control inputs.
     * For continuous monitoring, it is highly recommended to use `RegisterForControls` instead.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_Controls` struct to be filled with data.
     */
    void (*GetControls)(SPF_Telemetry_Handle* handle, SPF_Controls* out_data);

    /**
     * @brief Retrieves a snapshot of flags for special one-time gameplay events.
     * Note: This provides simple boolean flags. For detailed data about the event (e.g., fine amount),
     * you must use `RegisterForGameplayEvents`.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_SpecialEvents` struct to be filled with data.
     */
    void (*GetSpecialEvents)(SPF_Telemetry_Handle* handle, SPF_SpecialEvents* out_data);

    /**
     * @brief Retrieves detailed data for the most recent gameplay event.
     * This function only returns the data for the single last event. To reliably process all events,
     * you must subscribe using `RegisterForGameplayEvents`.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GameplayEvents` struct to be filled with data.
     */
    void (*GetGameplayEvents)(SPF_Telemetry_Handle* handle, SPF_GameplayEvents* out_data);

    /**
     * @brief Retrieves constants related to the H-shifter gearbox layout.
     * @param handle The telemetry context handle.
     * @param[out] out_data Pointer to an `SPF_GearboxConstants` struct to be filled with data.
     */
    void (*GetGearboxConstants)(SPF_Telemetry_Handle* handle, SPF_GearboxConstants* out_data);

    /**
     * @brief Gets the ID string of the last gameplay event that occurred (e.g., "player.fined").
     * This is useful for checking the last event without getting the full data payload.
     * @param handle The telemetry context handle.
     * @param[out] out_buffer A character buffer to receive the event ID string.
     * @param buffer_size The size of the output buffer.
     * @return The number of characters written to the buffer, or the required buffer size if the provided buffer is too small.
     */
    int (*GetLastGameplayEventId)(SPF_Telemetry_Handle* handle, char* out_buffer, int buffer_size);

} SPF_Telemetry_API;

#ifdef __cplusplus
}
#endif
