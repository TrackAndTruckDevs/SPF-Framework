/**
 * @file ExampleClimateAPI.hpp
 * @brief Complete example of the SPF Climate API — weather state, blend values, auto-toggle.
 *
 * @details Climate API reads live weather/climate simulation data and can force weather modes.
 * Use it for screenshots, tests, or tools that need deterministic weather.
 *
 * DEVELOPER NOTE: CL_IsReady() gates every call — climate is unavailable before the world loads.
 * Per-frame work belongs in ClimateAPI_OnUpdate, not in the render path.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Per-frame climate logic: auto-toggles weather when g_ctx.weatherAutoToggle is set.
 * @details Called from OnUpdate. Frame-count based delay (~2s at 60 FPS) demonstrates
 * turning a UI flag into timed game-state changes without timers/threads.
 */
void ClimateAPI_OnUpdate();

/**
 * @brief Renders the Climate tab: current/next weather, blend factors, sun, auto-toggle.
 */
void RenderClimateTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
