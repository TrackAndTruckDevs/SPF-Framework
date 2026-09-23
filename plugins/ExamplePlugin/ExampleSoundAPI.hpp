/**
 * @file ExampleSoundAPI.hpp
 * @brief Complete example of the SPF Sound API — FMOD banks/events/buses + horn→bell replacement.
 *
 * @details Sound API wraps FMOD Studio: load banks, browse events/buses, start/stop instances,
 * and (here) replace the game horn with a plugin-bank bicycle bell by polling the live
 * 'play' parameter each frame — see SoundAPI_OnUpdate for the exact detection/silence loop.
 *
 * DEVELOPER NOTE: For "play my sound from a plugin bank", follow bank load + event index
 * resolution (bank GUID → FindEventIndexByGuid) in RenderSoundTab / SoundAPI_OnUpdate,
 * then start instances from UI or Update.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Per-frame horn detection / bell playback while g_ctx.replaceHornEnabled.
 * Called from OnUpdate. Resolves bellEventIndex via bank GUID (lazy), caches
 * horn indices (event:/horn/ prefix), reads live 'play' param → SetEventVolume(0)
 * on game horn, edge-triggers bell Start/Stop. Cheap when the feature is off.
 */
void SoundAPI_OnUpdate();

/**
 * @brief Full cleanup: stop/release bell instance, unload bank, reset indices.
 * Called from OnUnload (and after unchecking the replace checkbox in the tab).
 */
void SoundAPI_Shutdown();

/**
 * @brief Renders the Sound tab: system stats, bus list, horn replacement UI, event browser.
 * @details Bank load happens here on checkbox-on (single entry point);
 * detection + 'play' param silencing + bell state machine run in SoundAPI_OnUpdate.
 */
void RenderSoundTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
