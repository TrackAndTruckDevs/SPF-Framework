/**
 * @file ExampleGameWorldAPI.cpp
 * @brief Implementation of the SPF GameWorld API example.
 *
 * @details TWO CLOCKS:
 *   - Simulation time: game calendar/clock (days, weeks, jobs) — GW_Get/SetSimulationTime.
 *   - Preview/skybox time: what the lighting engine shows — GW_Get/SetPreviewTime.
 *   They can diverge when SkyboxAutoUpdate is off (manual preview control).
 *
 * WHY lock before editing preview: if auto-update is on, the engine overwrites
 * SetPreviewTime every frame and the slider appears to "snap back".
 */
#include "ExampleGameWorldAPI.hpp"

#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>

namespace ExamplePlugin {

void RenderGameWorldTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.gameworldAPI || !g_ctx.coreAPI) {
    ui->UI_Text("Game World API is not available.");
    return;
  }

  auto gw = g_ctx.gameworldAPI;
  auto format = g_ctx.coreAPI->formatting;

  if (!gw->GW_IsReady()) {
    ui->UI_Text("Game World Service is not ready.");
    return;
  }

  ui->UI_Text("This tab displays and controls game world state, clocks, and simulation speed.");
  ui->UI_Separator();

  char buffer[256];

  // --- Section 1: Time Clocks ---
  ui->UI_Text("Simulation Clock:");

  uint32_t simTotalMinutes = gw->GW_GetSimulationTime();
  uint32_t simDays = gw->GW_GetGameDay();
  uint32_t simWeek = gw->GW_GetGameWeek();
  uint32_t dayOfWeek = gw->GW_GetDayOfWeek();

  const char* daysOfWeek[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
  const char* dowStr = (dayOfWeek < 7) ? daysOfWeek[dayOfWeek] : "Unknown";

  char sTimeBuffer[16];
  format->Fmt_Format(sTimeBuffer, sizeof(sTimeBuffer), "%02d:%02d", (simTotalMinutes % 1440) / 60, simTotalMinutes % 60);

  format->Fmt_Format(buffer, sizeof(buffer), "  Time: %s | Day: %d (%s) | Week: %d", sTimeBuffer, simDays + 1, dowStr, simWeek + 1);
  ui->UI_Text(buffer);

  // Simulation slider operates on minutes-within-day; dayStart anchors the absolute minute.
  int sliderSimMins = (int)(simTotalMinutes % 1440);
  if (ui->UI_SliderInt("Set Simulation Time", &sliderSimMins, 0, 1439, "%d min", SPF_SLIDER_FLAG_NONE)) {
    uint32_t dayStart = (uint32_t)(simDays * 1440);
    gw->GW_SetSimulationTime(dayStart + (uint32_t)sliderSimMins);
  }

  // Day-shift buttons: ±1440 minutes = ±1 day. Clamp-ish via underflow guard on -1 Day.
  if (ui->UI_Button("-1 Day", 0, 0)) {
    if (simTotalMinutes >= 1440) gw->GW_SetSimulationTime(simTotalMinutes - 1440);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("+1 Day", 0, 0)) {
    gw->GW_SetSimulationTime(simTotalMinutes + 1440);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Reset Day Time", 0, 0)) {
    gw->GW_SetSimulationTime((uint32_t)(simDays * 1440));
  }

  ui->UI_Separator();

  // --- Visual Skybox/Lighting Preview Clock ---
  ui->UI_Text("Visual Skybox/Lighting Preview Clock:");

  // skyboxLock UI-state mirrors "disable auto-update". Framework default is AutoUpdate=true
  // (Lock=false). static keeps the checkbox stable across frames without g_ctx.
  static bool skyboxLock = false;

  if (ui->UI_Checkbox("Lock Visual Time (Disable Skybox Auto Update)", &skyboxLock)) {
    gw->GW_SetSkyboxAutoUpdate(!skyboxLock);
  }

  int currentVisualMins = (int)gw->GW_GetPreviewTime();
  int sliderVisualMins = currentVisualMins % 1440;

  char vTimeBuffer[16];
  format->Fmt_Format(vTimeBuffer, sizeof(vTimeBuffer), "%02d:%02d", sliderVisualMins / 60, sliderVisualMins % 60);

  format->Fmt_Format(buffer, sizeof(buffer), "  Preview Time: %s", vTimeBuffer);
  ui->UI_Text(buffer);

  // Disable slider while auto-update would overwrite manual edits.
  if (!skyboxLock) {
    ui->UI_BeginDisabled(true);
  }

  if (ui->UI_SliderInt("Set Visual Time Slider", &sliderVisualMins, 0, 1439, "%d min", SPF_SLIDER_FLAG_NONE)) {
    uint32_t currentDayStart = (currentVisualMins / 1440) * 1440;
    gw->GW_SetPreviewTime(currentDayStart + (uint32_t)sliderVisualMins);
  }

  if (!skyboxLock) {
    ui->UI_EndDisabled();
    ui->UI_Text("Hint: Lock Visual Time to manually adjust the skybox slider.");
  }

  ui->UI_Separator();

  // --- Section 2: Engine Telemetry & Pause ---
  ui->UI_Text("Engine & Simulation Control:");

  float mapScale = gw->GW_GetMapScale();
  format->Fmt_Format(buffer, sizeof(buffer), "  Map Scale: %.1f (1:%.1f)", mapScale, mapScale);
  ui->UI_Text(buffer);

  double deltaTime = gw->GW_GetRealDeltaTime();
  format->Fmt_Format(buffer, sizeof(buffer), "  Real Frame Delta Time: %.6f s", deltaTime);
  ui->UI_Text(buffer);

  uint32_t realPlayTime = gw->GW_GetRealPlayTime();
  format->Fmt_Format(buffer, sizeof(buffer), "  Total Real Play Time: %u min", realPlayTime);
  ui->UI_Text(buffer);

  float globalWarp = gw->GW_GetGlobalWarp();
  format->Fmt_Format(buffer, sizeof(buffer), "  Global Time Warp: %.2f", globalWarp);
  ui->UI_Text(buffer);

  static float targetWarp = 1.0f;
  if (ui->UI_SliderFloat("Time Warp", &targetWarp, 0.0f, 10.0f, "%.2f", SPF_SLIDER_FLAG_NONE)) {
    gw->GW_SetGlobalWarp(targetWarp);
  }

  ui->UI_Separator();

  bool isPaused = gw->GW_IsGamePaused();
  format->Fmt_Format(buffer, sizeof(buffer), "  Simulation Paused: %s", isPaused ? "Yes" : "No");
  ui->UI_Text(buffer);

  if (ui->UI_Button(isPaused ? "Resume Simulation" : "Pause Simulation", 0, 0)) {
    gw->GW_SetGamePaused(!isPaused);
  }

  ui->UI_SameLine(0, 5);

  // Engine halt is a hard cut — independent of the softer "pause".
  static bool engineHalt = false;
  if (ui->UI_Checkbox("Hard Engine Halt", &engineHalt)) {
    gw->GW_SetEngineHalt(engineHalt);
  }
}

}  // namespace ExamplePlugin
