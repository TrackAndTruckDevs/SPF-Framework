/**
 * @file ExampleTelemetryAPI.hpp
 * @brief Complete example of the SPF Telemetry API — subscriptions, callbacks, Events view.
 *
 * @details Telemetry is push-based: you create a parent handle, subscribe to named streams
 * (game state, truck, job, navigation…), and receive data on the game thread each frame.
 * This module owns the Telemetry tab AND the Events tab (which displays the callback cache).
 *
 * DEVELOPER NOTE: For "subscribe to speed/RPM" or "read job data", see TelemetryAPI_OnActivated
 * and the callback pattern below. Data lands in g_ctx.eventDataCache — UI only reads the cache.
 */
#pragma once

#include "SPF/SPF_API/SPF_TelemetryData.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include <cstdint>

namespace ExamplePlugin {

// --- Lifecycle -------------------------------------------------------------

/**
 * @brief Creates the telemetry parent handle and registers all stream callbacks.
 * Called from OnActivated. Order: parent handle first, then RegisterFor* — children
 * auto-destroy when the parent is destroyed (no per-callback Unregister required).
 */
void TelemetryAPI_OnActivated();

/**
 * @brief Nulls the parent handle after framework teardown (and any per-frame work stops).
 * Called from OnUnload.
 */
void TelemetryAPI_OnUnload();

/**
 * @brief Optional per-frame housekeeping (e.g. throttled verbose dump of cached events).
 * Called from OnUpdate — keep it cheap; heavy work belongs in callbacks.
 */
void TelemetryAPI_OnUpdate();

// --- Event callbacks (registered in TelemetryAPI_OnActivated) ---------------
// Each has the signature the framework documents for its stream:
//   void(const T*, void* user_data)   or   void(const T*, uint32_t count, void* user_data)
// user_data is unused here — all state lives in the global g_ctx (simplest for examples).

void OnGameStateUpdate(const SPF_GameState* data, void* user_data);
void OnTimestampsUpdate(const SPF_Timestamps* data, void* user_data);
void OnCommonDataUpdate(const SPF_CommonData* data, void* user_data);
void OnTruckConstantsUpdate(const SPF_TruckConstants* data, void* user_data);
void OnTrailerConstantsUpdate(const SPF_TrailerConstants* data, void* user_data);
void OnTruckDataUpdate(const SPF_TruckData* data, void* user_data);
void OnTrailersUpdate(const SPF_Trailer* data, uint32_t count, void* user_data);
void OnJobConstantsUpdate(const SPF_JobConstants* data, void* user_data);
void OnJobDataUpdate(const SPF_JobData* data, void* user_data);
void OnNavigationDataUpdate(const SPF_NavigationData* data, void* user_data);
void OnControlsUpdate(const SPF_Controls* data, void* user_data);
void OnSpecialEventsUpdate(const SPF_SpecialEvents* data, void* user_data);
void OnGameplayEvent(const char* event_id, const SPF_GameplayEvents* data, void* user_data);
void OnGearboxConstantsUpdate(const SPF_GearboxConstants* data, void* user_data);

// --- Tabs ------------------------------------------------------------------

/**
 * @brief Renders the Telemetry tab: live snapshots of truck/job/navigation from the cache.
 * @details Demonstrates *polling* telemetry via Telemetry_Get* as a complement to callbacks.
 */
void RenderTelemetryTab(SPF_UI_API* ui, void* user_data);

/**
 * @brief Renders the Events tab: last gameplay event id + selected cache structures.
 * @details Shows data that only arrives through callbacks (no poll equivalent).
 */
void RenderEventsTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
