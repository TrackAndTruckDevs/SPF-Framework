/**
 * @file ExampleEnvironmentAPI.hpp
 * @brief Complete example of the SPF Environment API — game/profile/OS paths and status.
 *
 * @details Environment answers "where am I running?" — game name/version, active profile,
 * resolved directories (user home, music), VR/overlay presence, renderer, locale.
 *
 * DEVELOPER NOTE: Needed early if you write files (use Env_GetPluginDataDir for the
 * plugin's private data folder). The handle is created once in EnvironmentAPI_OnLoad.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Creates g_ctx.environmentHandle from loadAPI. Called from OnLoad (before OnActivated).
 * @details Needs g_ctx.loadAPI set first. Falls back if already set — safe to call twice.
 */
void EnvironmentAPI_OnLoad();

/**
 * @brief Destroys the environment handle on unload so no late getter can touch freed state.
 */
void EnvironmentAPI_OnUnload();

/**
 * @brief Renders the Environment tab: game/profile info, resolved paths, runtime, system.
 * @details Requires environmentAPI + environmentHandle.
 */
void RenderEnvironmentTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
