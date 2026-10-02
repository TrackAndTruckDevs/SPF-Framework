/**
 * @file ExampleSoundAPI.hpp
 * @brief Complete example of the SPF Sound API — FMOD banks/events/buses + horn→bell replacement.
 *
 * @details Sound API wraps FMOD Studio: load banks, browse events/buses, start/stop instances,
 * and (here) replace the game horn with a plugin-bank bicycle bell by blocking the game's
 * EventInstance::start calls at the FMOD level (SND_SuppressEventPlayback) and edge-triggering
 * the bell from the suppression counter — see SoundAPI_OnUpdate for the exact detection loop.
 *
 * DEVELOPER NOTE: For "play my sound from a plugin bank", follow the checkbox-on
 * sequence in RenderSoundTab: load bank → resolve event index (bank GUID →
 * FindEventIndexByGuid) → create instance — all synchronous, failures roll the
 * checkbox back with an error log. Start instances from UI or Update.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Per-frame horn detection / bell playback while g_ctx.replaceHornEnabled.
 * Called from OnUpdate. Press edge via suppressed-start counter (SND_SuppressEventPlayback
 * blocks the game horn before its first sample), release via the suppressed instance
 * 'play' param (or the instance being gone) → edge-triggers bell Start/Stop.
 * Cheap when the feature is off; no lazy resolution or frame-count fallbacks.
 */
void SoundAPI_OnUpdate();

/**
 * @brief Full cleanup: stop/release bell instance, unload bank, reset indices.
 * Called from OnUnload (and after unchecking the replace checkbox in the tab).
 */
void SoundAPI_Shutdown();

/**
 * @brief Renders the Sound tab: system stats, bus list, horn replacement UI, event browser.
 * @details Bank load → event resolve → instance create → SND_SuppressEventPlayback happen
 * here on checkbox-on (single entry point, each failure rolls back with a log); detection
 * + bell state machine run in SoundAPI_OnUpdate.
 */
void RenderSoundTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
