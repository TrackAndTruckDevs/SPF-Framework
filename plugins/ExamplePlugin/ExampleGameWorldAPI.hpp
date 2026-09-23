/**
 * @file ExampleGameWorldAPI.hpp
 * @brief Complete example of the SPF GameWorld API — sim time, skybox, warp, pause.
 *
 * @details GameWorld controls the *simulation* clock and engine-level state
 * (pause, time warp, skybox auto-update) as opposed to Telemetry's read-only stream.
 *
 * DEVELOPER NOTE: For "read/set in-game time" or "pause the game", start here.
 * GW_IsReady() is false until the world is loaded — gate all GW_* calls on it.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Renders the Game World tab: simulation clock, visual preview clock, warp/pause.
 * @details Requires g_ctx.gameworldAPI (cached in OnActivated) and GW_IsReady().
 */
void RenderGameWorldTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
