/**
 * @file ExampleTelemetryAPI.cpp
 * @brief Implementation of the SPF Telemetry API example (subscriptions + both data tabs).
 *
 * @details ARCHITECTURE:
 *   TelemetryAPI_OnActivated  → Tel_GetContext + N × Tel_RegisterFor*
 *   callbacks (game thread)   → memcpy into g_ctx.eventDataCache
 *   RenderTelemetry/Events    → read cache only (never call Register from render)
 *   TelemetryAPI_OnLoad path  → parent handle destroy implies child cleanup
 *
 * WHY a parent handle: one lifetime for all subscriptions. Destroying the parent on
 * unload guarantees no dangling callback into unloaded DLL code.
 *
 * CALLBACK CONTRACT: run fast — these fire on the game/simulation thread. Do not call
 * back into heavy framework APIs that may not be re-entrant; cache and return.
 */
#include "ExampleTelemetryAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_TelemetryData.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>
#include <cstdio>

namespace ExamplePlugin {

// =====================================================================================
// Lifecycle
// =====================================================================================

void TelemetryAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->telemetry) {
    return;
  }
  auto tel = g_ctx.coreAPI->telemetry;
  auto logger = g_ctx.coreAPI->logger;
  auto fmt = g_ctx.coreAPI->formatting;

  // Parent handle — must exist before any Tel_RegisterFor*.
  g_ctx.telemetryHandle = tel->Tel_GetContext(PLUGIN_NAME);
  if (!g_ctx.telemetryHandle) {
    if (logger) logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_ERROR, "Tel_GetContext failed.");
    return;
  }

  char log_buffer[256];

  // Each Tel_RegisterFor* stores the returned callback handle on g_ctx for explicit lifetime
  // documentation. The framework still tears them down with the parent — we do not Unregister
  // individually on unload (see TelemetryAPI_OnUnload).
  // user_data = &g_ctx: callbacks cast it back to PluginContext* to fill eventDataCache.
  g_ctx.gameStateCallback = tel->Tel_RegisterForGameState(g_ctx.telemetryHandle, OnGameStateUpdate, &g_ctx);
  g_ctx.timestampsCallback = tel->Tel_RegisterForTimestamps(g_ctx.telemetryHandle, OnTimestampsUpdate, &g_ctx);
  g_ctx.commonDataCallback = tel->Tel_RegisterForCommonData(g_ctx.telemetryHandle, OnCommonDataUpdate, &g_ctx);
  g_ctx.truckConstantsCallback = tel->Tel_RegisterForTruckConstants(g_ctx.telemetryHandle, OnTruckConstantsUpdate, &g_ctx);
  g_ctx.trailerConstantsCallback = tel->Tel_RegisterForTrailerConstants(g_ctx.telemetryHandle, OnTrailerConstantsUpdate, &g_ctx);
  g_ctx.truckDataCallback = tel->Tel_RegisterForTruckData(g_ctx.telemetryHandle, OnTruckDataUpdate, &g_ctx);
  g_ctx.trailersCallback = tel->Tel_RegisterForTrailers(g_ctx.telemetryHandle, OnTrailersUpdate, &g_ctx);
  g_ctx.jobConstantsCallback = tel->Tel_RegisterForJobConstants(g_ctx.telemetryHandle, OnJobConstantsUpdate, &g_ctx);
  g_ctx.jobDataCallback = tel->Tel_RegisterForJobData(g_ctx.telemetryHandle, OnJobDataUpdate, &g_ctx);
  g_ctx.navigationDataCallback = tel->Tel_RegisterForNavigationData(g_ctx.telemetryHandle, OnNavigationDataUpdate, &g_ctx);
  g_ctx.controlsCallback = tel->Tel_RegisterForControls(g_ctx.telemetryHandle, OnControlsUpdate, &g_ctx);
  g_ctx.specialEventsCallback = tel->Tel_RegisterForSpecialEvents(g_ctx.telemetryHandle, OnSpecialEventsUpdate, &g_ctx);
  g_ctx.gameplayEventsCallback = tel->Tel_RegisterForGameplayEvents(g_ctx.telemetryHandle, OnGameplayEvent, &g_ctx);
  g_ctx.gearboxConstantsCallback = tel->Tel_RegisterForGearboxConstants(g_ctx.telemetryHandle, OnGearboxConstantsUpdate, &g_ctx);

  if (logger && fmt) {
    fmt->Fmt_Format(log_buffer, sizeof(log_buffer), "Registered telemetry callbacks (parent=%p).", (void*)g_ctx.telemetryHandle);
    logger->Log(logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
  }
}

void TelemetryAPI_OnUnload() {
  // Framework destroys parent → all RegisterFor* children. We only drop our references
  // so no future frame accidentally treats them as live.
  g_ctx.telemetryHandle = nullptr;
  g_ctx.gameStateCallback = nullptr;
  g_ctx.timestampsCallback = nullptr;
  g_ctx.commonDataCallback = nullptr;
  g_ctx.truckConstantsCallback = nullptr;
  g_ctx.trailerConstantsCallback = nullptr;
  g_ctx.truckDataCallback = nullptr;
  g_ctx.trailersCallback = nullptr;
  g_ctx.jobConstantsCallback = nullptr;
  g_ctx.jobDataCallback = nullptr;
  g_ctx.navigationDataCallback = nullptr;
  g_ctx.controlsCallback = nullptr;
  g_ctx.specialEventsCallback = nullptr;
  g_ctx.gameplayEventsCallback = nullptr;
  g_ctx.gearboxConstantsCallback = nullptr;
}

void TelemetryAPI_OnUpdate() {
  // Example of a deferred diagnostic: the LogThrottled call is intentionally commented
  // (as in the original demo) — enable it when debugging event flow. Keep OnUpdate cheap.
  //
  // if (g_ctx.coreAPI && g_ctx.coreAPI->logger && g_ctx.coreAPI->logger->LogThrottled) {
  //   const char* msg = "Telemetry cache is receiving updates (throttled log demo).";
  //   g_ctx.coreAPI->logger->LogThrottled(1000, msg);
  // }
  (void)0;
}

// =====================================================================================
// Callbacks — write into g_ctx.eventDataCache only (fast, no UI, no allocate/free)
// =====================================================================================

void OnGameStateUpdate(const SPF_GameState* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.gameState = *data;
}

void OnTimestampsUpdate(const SPF_Timestamps* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.timestamps = *data;
}

void OnCommonDataUpdate(const SPF_CommonData* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.commonData = *data;
}

void OnTruckConstantsUpdate(const SPF_TruckConstants* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.truckConstants = *data;
}

void OnTrailerConstantsUpdate(const SPF_TrailerConstants* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.trailerConstants = *data;
}

void OnTruckDataUpdate(const SPF_TruckData* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.truckData = *data;
}

void OnTrailersUpdate(const SPF_Trailer* data, uint32_t count, void* user_data) {
  (void)user_data;
  // Array stream: replace the whole vector atomically from the game thread.
  g_ctx.eventDataCache.trailers.clear();
  if (data && count > 0) {
    g_ctx.eventDataCache.trailers.assign(data, data + count);
  }
}

void OnJobConstantsUpdate(const SPF_JobConstants* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.jobConstants = *data;
}

void OnJobDataUpdate(const SPF_JobData* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.jobData = *data;
}

void OnNavigationDataUpdate(const SPF_NavigationData* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.navigationData = *data;
}

void OnControlsUpdate(const SPF_Controls* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.controls = *data;
}

void OnSpecialEventsUpdate(const SPF_SpecialEvents* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.specialEvents = *data;
}

void OnGameplayEvent(const char* event_id, const SPF_GameplayEvents* data, void* user_data) {
  (void)user_data;
  // Named gameplay event: stash id for UI + merge payload into the last-known events struct.
  if (event_id) {
    snprintf(g_ctx.eventDataCache.lastGameplayEventId, sizeof(g_ctx.eventDataCache.lastGameplayEventId), "%s", event_id);
  }
  if (data) g_ctx.eventDataCache.gameplayEvents = *data;
}

void OnGearboxConstantsUpdate(const SPF_GearboxConstants* data, void* user_data) {
  (void)user_data;
  if (data) g_ctx.eventDataCache.gearboxConstants = *data;
}

// =====================================================================================
// Telemetry tab — poll path (Tel_Get*) as a teaching contrast to callbacks above
// =====================================================================================

void RenderTelemetryTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  // --- Telemetry Polling vs. Event-Driven ---
  // This tab demonstrates direct polling of telemetry data using Tel_Get...() functions.
  // For high-frequency updates prefer the event-driven callbacks registered in
  // TelemetryAPI_OnActivated (they fill g_ctx.eventDataCache). Use Tel_Get* for
  // infrequent snapshots or specific UI displays.
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->telemetry || !g_ctx.coreAPI->formatting || !ui) {
    ui->UI_Text("Telemetry API is not available.");
    return;
  }
  auto tel = g_ctx.coreAPI->telemetry;
  auto fmt = g_ctx.coreAPI->formatting;
  char buffer[256];

  ui->UI_Text("This tab displays live data from the Telemetry API.");
  ui->UI_Separator();

  auto telemetry = tel->Tel_GetContext(PLUGIN_NAME);

  // --- Truck data (poll) ---
  SPF_TruckData truck_data = {};
  tel->Tel_GetTruckData(telemetry, &truck_data, sizeof(SPF_TruckData));
  fmt->Fmt_Format(buffer, sizeof(buffer), "Speed: %.0f kph", truck_data.speed * 3.6f);
      ui->UI_Text(buffer);
  fmt->Fmt_Format(buffer, sizeof(buffer), "Engine RPM: %.0f", truck_data.engine_rpm);
      ui->UI_Text(buffer);
  fmt->Fmt_Format(buffer, sizeof(buffer), "Gear: %d", truck_data.displayed_gear);
  ui->UI_Text(buffer);
  ui->UI_Separator();

  // --- Job data (poll) ---
  SPF_JobConstants job_constants = {};
  tel->Tel_GetJobConstants(telemetry, &job_constants, sizeof(SPF_JobConstants));
  SPF_JobData job_data = {};
  tel->Tel_GetJobData(telemetry, &job_data, sizeof(SPF_JobData));
  if (job_data.on_job) {
    ui->UI_Text("Currently on a job!");
    fmt->Fmt_Format(buffer, sizeof(buffer), "Cargo: %s", job_constants.cargo_name);
      ui->UI_Text(buffer);
    fmt->Fmt_Format(buffer, sizeof(buffer), "Destination: %s, %s", job_constants.destination_company, job_constants.destination_city);
      ui->UI_Text(buffer);
    fmt->Fmt_Format(buffer, sizeof(buffer), "Cargo Damage: %.1f%%", job_data.cargo_damage * 100.0f);
      ui->UI_Text(buffer);
    } else {
    ui->UI_Text("Not currently on a job.");
  }
}

// =====================================================================================
// Events tab — displays callback-only data from g_ctx.eventDataCache
// =====================================================================================

void RenderEventsTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.coreAPI || !g_ctx.coreAPI->formatting || !ui) {
    ui->UI_Text("Formatting API is not available.");
    return;
  }
  auto fmt = g_ctx.coreAPI->formatting;
  auto& cache = g_ctx.eventDataCache;
  char buffer[512];

  ui->UI_Text("This tab displays the last data received from event callbacks.");
  ui->UI_Separator();

  fmt->Fmt_Format(buffer, sizeof(buffer), "Last Gameplay Event: %s", cache.lastGameplayEventId);
  ui->UI_Text(buffer);
  ui->UI_Separator();

  ui->UI_Text("Game State:");
  fmt->Fmt_Format(buffer, sizeof(buffer), "  Paused: %s", cache.gameState.paused ? "Yes" : "No");
    ui->UI_Text(buffer);
  ui->UI_Separator();

  ui->UI_Text("Truck Data:");
  fmt->Fmt_Format(buffer, sizeof(buffer), "  Speed: %.0f kph", cache.truckData.speed * 3.6f);
    ui->UI_Text(buffer);
  fmt->Fmt_Format(buffer, sizeof(buffer), "  Engine RPM: %.0f", cache.truckData.engine_rpm);
    ui->UI_Text(buffer);
  ui->UI_Separator();

  ui->UI_Text("Trailer Info:");
  fmt->Fmt_Format(buffer, sizeof(buffer), "  Attached Trailers: %zu", cache.trailers.size());
  ui->UI_Text(buffer);
  if (!cache.trailers.empty()) {
    fmt->Fmt_Format(buffer, sizeof(buffer), "  Trailer 1 Brand: %s", cache.trailers[0].constants.brand);
    ui->UI_Text(buffer);
    ui->UI_TreePop();
  }
}

}  // namespace ExamplePlugin
