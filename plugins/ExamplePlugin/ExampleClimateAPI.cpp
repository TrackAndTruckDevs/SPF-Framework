/**
 * @file ExampleClimateAPI.cpp
 * @brief Implementation of the SPF Climate API example.
 *
 * @details READ vs WRITE:
 *   GetBlended* — passive snapshots of simulation state (safe every frame).
 *   CL_SetWeatherMode — active control; second arg `true` forces the change.
 *
 * AUTO-TOGGLE: counts OnUpdate ticks instead of wall-clock so behavior matches the
 * frame rate the plugin actually runs at (simple, good enough for a demo).
 */
#include "ExampleClimateAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>

namespace ExamplePlugin {

void ClimateAPI_OnUpdate() {
  // Early-out: no API, not ready (pre-world), or feature off → zero cost this frame.
  if (!g_ctx.climateAPI || !g_ctx.climateAPI->CL_IsReady() || !g_ctx.weatherAutoToggle) {
    return;
  }

  // ~2 seconds at 60 FPS. static survives across frames without being in g_ctx.
  static int toggleCounter = 0;
  toggleCounter++;
  if (toggleCounter >= 120) {
    toggleCounter = 0;
    // Flip nice(0) ↔ bad(1). `true` = apply immediately rather than waiting for natural transition.
    int32_t currentMode = g_ctx.climateAPI->CL_GetWeatherMode();
    g_ctx.climateAPI->CL_SetWeatherMode(currentMode == 0 ? 1 : 0, true);
  }
}

void RenderClimateTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.climateAPI) {
    ui->UI_Text("Climate API is not available.");
    return;
  }
  if (!g_ctx.climateAPI->CL_IsReady()) {
    ui->UI_Text("Climate Service is not ready yet.");
    return;
  }

  auto climate = g_ctx.climateAPI;
  auto format = g_ctx.coreAPI->formatting;

  ui->UI_TextWrapped("This tab displays live values from the Climate API and lets you toggle weather automatically.");
  ui->UI_Separator();

  char buffer[256];

  // --- Climate Name ---
  {
    char name[64];
    if (climate->CL_GetCurrentClimateName(name, sizeof(name)) > 0) {
      format->Fmt_Format(buffer, sizeof(buffer), "Current Climate: %s", name);
      ui->UI_Text(buffer);
    }
  }

  // --- Weather Mode (current + scheduled next) ---
  {
    int32_t mode = climate->CL_GetWeatherMode();
    int32_t nextMode = climate->CL_GetNextWeatherMode();
    const char* modeStr = (mode == 0) ? "Nice" : "Bad";
    const char* nextStr = (nextMode == 0) ? "Nice" : "Bad";
    format->Fmt_Format(buffer, sizeof(buffer), "Weather Mode: %s (next: %s)", modeStr, nextStr);
    ui->UI_Text(buffer);
  }

  // --- Bad-weather intensity / remaining time ---
  {
    float factor = climate->CL_GetBadWeatherFactor();
    format->Fmt_Format(buffer, sizeof(buffer), "Bad Weather Factor: %.3f", factor);
    ui->UI_Text(buffer);

    uint32_t badMode = climate->CL_GetBadWeatherMode();
    format->Fmt_Format(buffer, sizeof(buffer), "Bad Weather Mode: %s", badMode ? "Active" : "Inactive");
    ui->UI_Text(buffer);

    float remaining = climate->CL_GetRemainingBadWeatherTime();
    format->Fmt_Format(buffer, sizeof(buffer), "Remaining Bad Weather Time: %.1f sec", remaining);
    ui->UI_Text(buffer);
  }

  // --- Blended (interpolated) visual values ---
  ui->UI_Separator();
  ui->UI_Text("Blended (interpolated) values:");

  {
    float temp = climate->GetBlendedTemperature();
    format->Fmt_Format(buffer, sizeof(buffer), "Temperature: %.2f", temp);
    ui->UI_Text(buffer);

    float rain = climate->GetBlendedRainIntensity();
    format->Fmt_Format(buffer, sizeof(buffer), "Rain Intensity: %.3f", rain);
    ui->UI_Text(buffer);

    float fog = climate->GetBlendedFogDensity();
    format->Fmt_Format(buffer, sizeof(buffer), "Fog Density: %.3f", fog);
    ui->UI_Text(buffer);

    float snow = climate->GetBlendedSnowIntensity();
    format->Fmt_Format(buffer, sizeof(buffer), "Snow Intensity: %.3f", snow);
    ui->UI_Text(buffer);
  }

  // --- Sun angle & transition progress ---
  ui->UI_Separator();
  ui->UI_Text("Sun & Transition:");

  {
    float sunAngle = climate->CL_GetSunAngle();
    format->Fmt_Format(buffer, sizeof(buffer), "Sun Angle: %.2f rad", sunAngle);
    ui->UI_Text(buffer);

    float transition = climate->CL_GetTransitionProgress();
    format->Fmt_Format(buffer, sizeof(buffer), "Sun Profile Transition: %.3f", transition);
    ui->UI_Text(buffer);

    float weatherBlend = climate->CL_GetWeatherBlendProgress();
    format->Fmt_Format(buffer, sizeof(buffer), "Weather Blend: %.3f", weatherBlend);
    ui->UI_Text(buffer);
  }

  // --- Auto-toggle flag consumed by ClimateAPI_OnUpdate ---
  ui->UI_Separator();
  if (ui->UI_Checkbox("Auto-toggle weather (nice/bad)", &g_ctx.weatherAutoToggle)) {
    if (g_ctx.weatherAutoToggle) {
      g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Climate auto-toggle enabled.");
    } else {
      g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Climate auto-toggle disabled.");
    }
  }
  if (g_ctx.weatherAutoToggle) {
    ui->UI_TextColored(0.4f, 1.0f, 0.4f, 1.0f, "Weather is now flipping every ~2 seconds.");
  }
}

}  // namespace ExamplePlugin
