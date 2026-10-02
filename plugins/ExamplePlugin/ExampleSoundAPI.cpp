/**
 * @file ExampleSoundAPI.cpp
 * @brief Implementation of the SPF Sound API example (bank + horn→bell replacement).
 *
 * @details REPLACEMENT FLOW (interception-based, zero first-sample leak):
 *   Checkbox ON  → SND_LoadBankFile (plugin .bank under Env_GetPluginDataDir),
 *                  synchronous bell-event resolve + SND_CreateEventInstance,
 *                  then SND_SuppressEventPlayback("event:/horn/") — the
 *                  EventInstance::start detour blocks every game horn start
 *                  before FMOD plays a single sample. Every failure rolls the
 *                  checkbox back with an error log.
 *   OnUpdate     → press edge = suppressed-start counter changed → start bell;
 *                  release = suppressed instance gone or its 'play' param
 *                  drops to 0. No frame fallbacks, no lazy retries.
 *   Checkbox OFF → full teardown (unsuppress, stop, release, unload) in the
 *                  checkbox handler itself.
 *
 * WHY the counter: the game re-issues start while the key is held — each call
 * is intercepted and counted, so a counter change = "held this frame", from
 * the first sample, with the game horn never reaching the speakers.
 *
 * SOUNDREF REBINDING EXAMPLE (game layer):
 *   Click   ON → SND_RegisterSoundRefOverride (pure map; applies on next soundref activation)
 *   Click   OFF → SND_UnregisterSoundRefOverride(click soundref) restores original
 *   Music   ON → Env_VfsMount + SND_SoundRef_Replace (automatic: register + source
 *                 rewrite + stop -> activate -> resume rebind; bank loads via game VFS)
 *   Music   OFF → SND_UnregisterSoundRefOverride(music soundref) — same rebind back
 *   UI shows the live state of that soundref path (source / active / override)
 *   via SND_FindSoundRefIndex + SND_GetSoundRefSource + SND_IsSoundRefActive.
 */
#include "ExampleSoundAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/SPF_API/SPF_Sound_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>
#include <cstring>

namespace ExamplePlugin {

namespace {

// The game binds its menu/UI click through this .soundref path. The game
// triggers this binding itself on every click — we do not play anything.
const char kUiClickSoundref[] = "/sound/ui/ui_click.soundref";
// Replacement source: the "error" event inside the game's own UI bank.
// Pair verified in-game: click soundref → /sound/ui/ui.bank#error.
const char kUiClickErrorSource[] = "/sound/ui/ui.bank#error";

// Suppression prefix: one rule covers every truck's horn variant.
const char kHornEventPrefix[] = "event:/horn/";

// The game's menu music binding (path from game binary rdata string scan).
const char kMusicMainMenuSoundref[] = "/sound/music/music_main_menu.soundref";
// Replacement source: plugin data dir is mounted at /spf/ExamplePlugin
// (Env_VfsMount), so the game resolves the bank through that VFS path.
const char kMusicMainMenuSource[] = "/spf/ExamplePlugin/disc1.bank#music/main_menu";

}  // namespace

void SoundAPI_OnUpdate() {
  // PERFORMANCE: entire block only runs while checkbox is on AND sound system ready.
  if (!g_ctx.replaceHornEnabled || !g_ctx.soundAPI || !g_ctx.coreAPI || !g_ctx.soundAPI->SND_IsReady()) {
    return;
  }
  auto snd = g_ctx.soundAPI;

  // --- Step 1: Press/release detection via the suppression counter ---
  // While the horn key is held the game keeps (re)issuing EventInstance::start
  // on event:/horn/* — every call is blocked by the framework detour and
  // counted. A counter change = "button held this frame", from the first
  // sample; the game horn never reaches the speakers.
  const unsigned long long count = snd->SND_GetSuppressedStartCount(kHornEventPrefix);
  const bool pressed = count != g_ctx.hornActivityCount;
  if (pressed) {
    g_ctx.hornActivityCount = count;
  }

  // Release: the suppressed game instance still carries the live "play"
  // parameter (setParameter passes through) — play dropping to 0 means the
  // key is up. The instance being gone (released by the game) is equally a
  // release: there is nothing left to track.
  bool released = false;
  if (g_ctx.bellReplacementActive) {
    void* sup = snd->SND_GetLastSuppressedInstance(kHornEventPrefix);
    if (!sup) {
      released = true;
    } else {
      float playValue = 1.0f;
      if (snd->SND_GetEventParameter(sup, "play", &playValue)) {
        released = (playValue <= 0.0f);
      } else {
        released = true;
        static bool s_loggedParamFail = false;
        if (!s_loggedParamFail) {
          s_loggedParamFail = true;
          g_ctx.coreAPI->logger->Log(
            g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN,
            "Horn: 'play' parameter unreadable on suppressed instance — treating as released.");
        }
      }
    }
  }

  // --- Step 2: Bell state machine ---
  // IDLE→PLAYING: press edge; PLAYING→IDLE: release. bellReplacementActive
  // prevents calling StartEvent every frame (would restart from the beginning).
  if (pressed && !g_ctx.bellReplacementActive) {
    if (g_ctx.bellInstance) {
      snd->SND_StartEvent(g_ctx.bellInstance);
      g_ctx.bellReplacementActive = true;
    } else {
      g_ctx.coreAPI->logger->Log(
        g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN,
        "Horn press detected but bell instance is null — check enable-time logs.");
    }
  } else if (released && g_ctx.bellReplacementActive && g_ctx.bellInstance) {
    snd->SND_StopEvent(g_ctx.bellInstance, true);  // true = allow fadeout
    g_ctx.bellReplacementActive = false;
  }
}

void SoundAPI_Shutdown() {
  if (!g_ctx.soundAPI) {
    g_ctx.replaceHornEnabled = false;
    g_ctx.soundRefClickEnabled = false;
    g_ctx.musicReplaceEnabled = false;
    g_ctx.musicBank = nullptr;
    g_ctx.bellEventIndex = -1;
    g_ctx.bellInstance = nullptr;
    g_ctx.bellBank = nullptr;
    g_ctx.hornActivityCount = 0;
    g_ctx.bellTestPlaying = false;
    g_ctx.bellReplacementActive = false;
    return;
  }

  auto snd = g_ctx.soundAPI;

  // --- SoundRef override cleanup: restore the original click sound ---
  // Overrides are world-scoped; unregister so the game keeps its binding
  // after this plugin's teardown.
  if (g_ctx.soundRefClickEnabled) {
    snd->SND_UnregisterSoundRefOverride(kUiClickSoundref);
    g_ctx.soundRefClickEnabled = false;
  }

  // --- Music replacement cleanup: restore original menu music binding ---
  if (g_ctx.musicReplaceEnabled) {
    snd->SND_UnregisterSoundRefOverride(kMusicMainMenuSoundref);
    g_ctx.musicReplaceEnabled = false;
  }
  if (g_ctx.musicBank) {
    snd->SND_UnloadBank(g_ctx.musicBank);
    g_ctx.musicBank = nullptr;
  }

  // --- Suppression cleanup: let the game horn play again ---
  if (g_ctx.replaceHornEnabled) {
    snd->SND_UnsuppressEventPlayback(kHornEventPrefix);
  }

  if (g_ctx.bellInstance) {
    snd->SND_StopEvent(g_ctx.bellInstance, true);
    snd->SND_ReleaseEvent(g_ctx.bellInstance);
    g_ctx.bellInstance = nullptr;
  }
  if (g_ctx.bellBank) {
    snd->SND_UnloadBank(g_ctx.bellBank);
    g_ctx.bellBank = nullptr;
  }
  g_ctx.bellEventIndex = -1;
  g_ctx.hornActivityCount = 0;
  g_ctx.bellTestPlaying = false;
  g_ctx.bellReplacementActive = false;
  g_ctx.replaceHornEnabled = false;
}

void RenderSoundTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.soundAPI) {
    ui->UI_Text("Sound API is not available.");
    return;
  }

  auto snd = g_ctx.soundAPI;

  if (!snd->SND_IsReady()) {
    ui->UI_Text("Sound system is not ready. Load into the game world first.");
    return;
  }

  char buffer[256];

  // --- Sound System Info ---
  ui->UI_Text("Sound System Status");
  ui->UI_Separator();

  int eventCount = snd->SND_GetEventCount();
  int busCount = snd->SND_GetBusCount();
  int bankCount = snd->SND_GetBankCount();
  g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Events: %d | Buses: %d | Banks: %d", eventCount, busCount, bankCount);
  ui->UI_Text(buffer);

  ui->UI_Separator();

  // --- Horn Replacement UI ---
  // When the user checks "Replace horn", the bank is loaded here (single entry point).
  // The actual horn detection and bell playback runs in SoundAPI_OnUpdate() every frame.
  ui->UI_Text("Horn Replacement");
  ui->UI_Separator();

  if (!g_ctx.replaceHornEnabled) {
    if (ui->UI_Checkbox("Replace horn with bicycle bell", &g_ctx.replaceHornEnabled)) {
      if (g_ctx.replaceHornEnabled) {
        // Load the bank
        char dataDir[512];
        g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataDir, sizeof(dataDir));
        char bankPath[1024];
        g_ctx.coreAPI->formatting->Fmt_Format(bankPath, sizeof(bankPath), "%s\\bicycle_bell.bank", dataDir);

        g_ctx.bellBank = snd->SND_LoadBankFile(bankPath, nullptr);
        if (g_ctx.bellBank) {
          g_ctx.bellEventIndex = -1;
          g_ctx.bellInstance = nullptr;

          // LoadBankFile flushed System::Update and cleared the event cache,
          // so the GUID lookup is valid immediately — no lazy retries.
          int bankEventCount = snd->SND_GetBankEventCount(g_ctx.bellBank);
          uint8_t eventGuid[16] = {};
          if (bankEventCount > 0 && snd->SND_GetBankEventGuid(g_ctx.bellBank, 0, eventGuid)) {
            g_ctx.bellEventIndex = snd->SND_FindEventIndexByGuid(eventGuid);
          }
          if (g_ctx.bellEventIndex < 0) {
            char logBuf[192];
            g_ctx.coreAPI->formatting->Fmt_Format(logBuf, sizeof(logBuf),
              "Horn replacement: bell event not resolvable after load (bankEventCount=%d) — rolling back.", bankEventCount);
            g_ctx.coreAPI->logger->Log(
              g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, logBuf);
            snd->SND_UnloadBank(g_ctx.bellBank);
            g_ctx.bellBank = nullptr;
            g_ctx.replaceHornEnabled = false;
            return;
          }

          g_ctx.bellInstance = snd->SND_CreateEventInstance(g_ctx.bellEventIndex);
          if (!g_ctx.bellInstance) {
            char logBuf[192];
            g_ctx.coreAPI->formatting->Fmt_Format(logBuf, sizeof(logBuf),
              "Horn replacement: SND_CreateEventInstance failed (event index %d) — rolling back.", g_ctx.bellEventIndex);
            g_ctx.coreAPI->logger->Log(
              g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, logBuf);
            snd->SND_UnloadBank(g_ctx.bellBank);
            g_ctx.bellBank = nullptr;
            g_ctx.bellEventIndex = -1;
            g_ctx.replaceHornEnabled = false;
            return;
          }

          // Block the game horn at the FMOD start boundary (first-sample
          // silence) and reset the press-edge baseline.
          if (!snd->SND_SuppressEventPlayback(kHornEventPrefix)) {
            g_ctx.coreAPI->logger->Log(
              g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN,
              "Horn replacement: suppression not armed (EventInstance::start hook unavailable) — rolling back.");
            snd->SND_ReleaseEvent(g_ctx.bellInstance);
            g_ctx.bellInstance = nullptr;
            snd->SND_UnloadBank(g_ctx.bellBank);
            g_ctx.bellBank = nullptr;
            g_ctx.bellEventIndex = -1;
            g_ctx.replaceHornEnabled = false;
            return;
          }
          g_ctx.hornActivityCount = snd->SND_GetSuppressedStartCount(kHornEventPrefix);
          g_ctx.bellReplacementActive = false;
          {
            char bellPath[256] = {};
            snd->SND_GetEventPath(g_ctx.bellEventIndex, bellPath, sizeof(bellPath));
            char logBuf[320];
            g_ctx.coreAPI->formatting->Fmt_Format(logBuf, sizeof(logBuf),
              "Horn replacement enabled: bank loaded, event %d ('%s'), instance created, suppression armed.",
              g_ctx.bellEventIndex, bellPath[0] ? bellPath : "<unresolved>");
            g_ctx.coreAPI->logger->Log(
              g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, logBuf);
          }
        } else {
          g_ctx.replaceHornEnabled = false;
          g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Failed to load bicycle_bell.bank.");
        }
      }
    }
  } else {
    ui->UI_TextColored(0.4f, 1.0f, 0.4f, 1.0f, "Horn replacement ACTIVE");
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Bell event: %d | Suppressed horn starts: %llu",
      g_ctx.bellEventIndex, (unsigned long long)g_ctx.hornActivityCount);
    ui->UI_Text(buffer);

    // Manual test playback — independent of horn detection (bellTestPlaying guard).
    if (g_ctx.bellTestPlaying) {
      if (ui->UI_Button("Stop Bell", 0, 0)) {
        if (g_ctx.bellInstance) {
          snd->SND_StopEvent(g_ctx.bellInstance, true);
        }
        g_ctx.bellTestPlaying = false;
      }
    } else {
      if (ui->UI_Button("Test Bell", 0, 0)) {
        if (g_ctx.bellInstance) {
          snd->SND_StartEvent(g_ctx.bellInstance);
          g_ctx.bellTestPlaying = true;
        }
      }
    }

    if (ui->UI_Checkbox("Replace horn with bicycle bell", &g_ctx.replaceHornEnabled)) {
      if (!g_ctx.replaceHornEnabled) {
        // Cleanup — identical to monolithic original (full feature teardown):
        snd->SND_UnsuppressEventPlayback(kHornEventPrefix);
        if (g_ctx.bellInstance) {
          snd->SND_StopEvent(g_ctx.bellInstance, true);
          snd->SND_ReleaseEvent(g_ctx.bellInstance);
          g_ctx.bellInstance = nullptr;
        }
        if (g_ctx.bellBank) {
          snd->SND_UnloadBank(g_ctx.bellBank);
          g_ctx.bellBank = nullptr;
        }
        g_ctx.bellEventIndex = -1;
        g_ctx.hornActivityCount = 0;
        g_ctx.bellTestPlaying = false;
        g_ctx.bellReplacementActive = false;
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Horn replacement disabled.");
      }
    }
  }

  ui->UI_Separator();

  // --- SoundRef Rebinding (game layer) ---
  // Unlike the horn example above (FMOD layer: we create and play our own
  // instance), this example rebinds a GAME-OWNED sound. The game keeps
  // triggering its UI click itself — we only change which FMOD event that
  // binding resolves to. No instance is created or played by the plugin.
  ui->UI_Text("SoundRef Rebinding");
  ui->UI_Separator();

  if (ui->UI_Checkbox("Replace UI click with error sound", &g_ctx.soundRefClickEnabled)) {
    if (g_ctx.soundRefClickEnabled) {
      // Rebind: game's click soundref -> error event in the game's own UI bank.
      //
      // This section is the MANUAL example - every step is spelled out. The
      // Menu Music section at the bottom is the convenience example: its
      // SND_SoundRef_Replace performs this same sequence in one call (but
      // queues it for the game thread instead of acting on the caller).
      //
      // Step 1: register the override in the framework's map (pure: it only
      //         stores the mapping; live events are untouched for now).
      bool registered = snd->SND_RegisterSoundRefOverride(kUiClickSoundref, kUiClickErrorSource);
      if (!registered) {
        g_ctx.soundRefClickEnabled = false;
        g_ctx.coreAPI->logger->Log(
          g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN,
          "Failed to register SoundRef override (path not found in catalog).");
      } else {
        // Step 2: find the LIVE game event behind this soundref path.
        void* clickEvent = snd->SND_FindGameEventByPath(kUiClickSoundref);
        if (!clickEvent) {
          // Event not created yet - the map alone is enough: the framework's
          // SoundRef LoadConfig hook applies the override automatically the
          // next time the game activates this soundref.
          g_ctx.coreAPI->logger->Log(
            g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
            "SoundRef override registered; event not live yet - takes effect on next activation by the game.");
        } else {
          // Step 3: remember playback state (the click is a one-shot, idle
          //         between clicks; loops/menus may be PLAYING right now).
          uint32_t wasState = snd->SND_GetGameEventPlaybackState(clickEvent);
          // Step 4: re-create the instance: stop old -> release -> activate.
          //         Activate re-runs SoundRef_LoadConfig, whose hook records
          //         the original source (for OFF) and applies our override,
          //         so the fresh instance is built from the error event.
          bool activated = snd->SND_GameEvent_Activate(clickEvent);
          // Step 5: Activate never starts playback - resume if it was playing.
          if (activated && wasState == SPF_SOUND_EVENT_STATE_PLAYING) {
            snd->SND_GameEvent_Start(clickEvent);
          }
          g_ctx.coreAPI->logger->Log(
            g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
            "SoundRef override applied manually: click -> error. Open the game menu and you will hear the error sound instead of the click.");
        }
      }
    } else {
      // Unregister is the reverse chain in one call: it restores the recorded
      // original source on live events and queues the same rebind
      // (stop -> activate -> resume) on the game thread - the original click
      // sound comes back.
      if (snd->SND_UnregisterSoundRefOverride(kUiClickSoundref)) {
        g_ctx.coreAPI->logger->Log(
          g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
          "SoundRef override removed, original click sound restored.");
      }
    }
  }

  // --- Live state of THIS soundref path, read back from the game ---
  // SND_FindSoundRefIndex / SND_GetSoundRefSource / SND_IsSoundRefActive /
  // SND_GetSoundRefOverride read the framework's snapshot cache (O(1)); the
  // snapshot is rebuilt only on actual changes (game LoadConfig rebind or
  // register/unregister) — safe to call every frame.
  // SND_FindSoundRefIndex     → catalog index of the .soundref path
  // SND_GetSoundRefSource     → current live bank#event binding (shows the
  //                              override while registered)
  // SND_IsSoundRefActive      → whether a live sound_event exists right now
  // SND_GetSoundRefOverride   → override WE registered (empty = none)
  char srSource[256] = {};
  char srOverride[256] = {};
  int srIdx = snd->SND_FindSoundRefIndex(kUiClickSoundref);
  bool srFound = srIdx >= 0;
  bool srActive = false;
  if (srFound) {
    snd->SND_GetSoundRefSource(srIdx, srSource, sizeof(srSource));
    srActive = snd->SND_IsSoundRefActive(kUiClickSoundref);
    snd->SND_GetSoundRefOverride(kUiClickSoundref, srOverride, sizeof(srOverride));
  }

  if (srFound) {
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "SoundRef: %s", kUiClickSoundref);
    ui->UI_Text(buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Live source: %s%s",
      srSource[0] ? srSource : "(no live event)", srActive ? "" : " (inactive)");
    ui->UI_Text(buffer);

    if (srOverride[0]) {
      g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Override: %s", srOverride);
      ui->UI_TextColored(1.0f, 0.8f, 0.2f, 1.0f, buffer);
    } else {
      ui->UI_TextColored(0.6f, 0.6f, 0.6f, 1.0f, "Override: none");
    }
  } else {
    ui->UI_TextColored(1.0f, 0.4f, 0.4f, 1.0f, "SoundRef path not found in catalog.");
  }

  ui->UI_Separator();

  // --- Menu music replacement (automatic: VFS mount + SND_SoundRef_Replace) ---
  // Unlike the click example above (rebinding within an already-loaded game
  // bank), this one points the game at a bank it does not know. ONE automatic
  // call does everything: SND_SoundRef_Replace registers the override, rewrites
  // the live sound_event source and queues a game-thread rebind: stop old
  // instance -> activate new source (the GAME loads our bank from VFS during
  // activate) -> resume if was playing.
  ui->UI_Text("Menu Music Replacement");
  ui->UI_Separator();

  if (ui->UI_Checkbox("Replace menu music with plugin bank", &g_ctx.musicReplaceEnabled)) {
    if (g_ctx.musicReplaceEnabled) {
      // 1. Mount the plugin data dir — game sees /spf/ExamplePlugin/disc1.bank.
      //    Env_VfsMount is idempotent (same dir → same vpath, no duplicate mount).
      char dataDir[512];
      g_ctx.environmentAPI->Env_GetPluginDataDir(g_ctx.environmentHandle, dataDir, sizeof(dataDir));
      char vpath[256];
      bool mounted = g_ctx.environmentAPI->Env_VfsMount(
        g_ctx.environmentHandle, dataDir, -1, 5000, vpath, sizeof(vpath));

      // 2. Automatic full-cycle replace: register + source rewrite +
      //    stop -> activate -> resume rebind (bank loads via game VFS itself).
      if (mounted &&
          snd->SND_SoundRef_Replace(kMusicMainMenuSoundref, kMusicMainMenuSource)) {
        g_ctx.coreAPI->logger->Log(
          g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
          "Menu music replaced: soundref -> /spf/ExamplePlugin/disc1.bank#music/main_menu.");
      } else {
        // Roll back partial state so the checkbox reflects reality.
        snd->SND_UnregisterSoundRefOverride(kMusicMainMenuSoundref);
        g_ctx.musicReplaceEnabled = false;
        g_ctx.coreAPI->logger->Log(
          g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN,
          "Menu music replacement failed (mount/soundref). Check log.");
      }
    } else {
      // Restore the original binding: source rewrite back + same rebind —
      // the original game music resumes automatically.
      snd->SND_UnregisterSoundRefOverride(kMusicMainMenuSoundref);
      g_ctx.coreAPI->logger->Log(
        g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO,
        "Menu music replacement disabled, original sound restored.");
    }
  }

  // --- Volume + mute for the menu-music bus (bus:/game/ui_music) ---
  int musicBusIndex = -1;
  {
    int busCount = snd->SND_GetBusCount();
    for (int i = 0; i < busCount; ++i) {
      char busPath[256] = {};
      snd->SND_GetBusPath(i, busPath, sizeof(busPath));
      if (std::strcmp(busPath, "bus:/game/ui_music") == 0) {
        musicBusIndex = i;
        break;
      }
    }
  }

  if (musicBusIndex >= 0) {
    g_ctx.musicVolume = snd->SND_GetBusVolume(musicBusIndex);
    g_ctx.musicMuted = snd->SND_GetBusMute(musicBusIndex);
    if (!g_ctx.musicOrigCaptured) {
      g_ctx.musicOrigVolume = g_ctx.musicVolume;
      g_ctx.musicOrigMuted = g_ctx.musicMuted;
      g_ctx.musicOrigCaptured = true;
    }
    if (ui->UI_SliderFloat("Menu Music Volume", &g_ctx.musicVolume, 0.0f, 1.0f, "%.2f", SPF_SLIDER_FLAG_NONE)) {
      snd->SND_SetBusVolume(musicBusIndex, g_ctx.musicVolume);
    }
    if (ui->UI_Checkbox("Mute Menu Music Bus", &g_ctx.musicMuted)) {
      snd->SND_SetBusMute(musicBusIndex, g_ctx.musicMuted);
    }
    if (ui->UI_Button("Reset Music Bus", 0, 0)) {
      g_ctx.musicVolume = g_ctx.musicOrigVolume;
      g_ctx.musicMuted = g_ctx.musicOrigMuted;
      snd->SND_SetBusVolume(musicBusIndex, g_ctx.musicOrigVolume);
      snd->SND_SetBusMute(musicBusIndex, g_ctx.musicOrigMuted);
    }
  } else {
    ui->UI_TextColored(1.0f, 0.4f, 0.4f, 1.0f, "bus:/game/ui_music not found");
  }

  // --- Live state of the menu music soundref, same readback as click ---
  char mSource[256] = {};
  char mOverride[256] = {};
  int mIdx = snd->SND_FindSoundRefIndex(kMusicMainMenuSoundref);
  bool mFound = mIdx >= 0;
  bool mActive = false;
  if (mFound) {
    snd->SND_GetSoundRefSource(mIdx, mSource, sizeof(mSource));
    mActive = snd->SND_IsSoundRefActive(kMusicMainMenuSoundref);
    snd->SND_GetSoundRefOverride(kMusicMainMenuSoundref, mOverride, sizeof(mOverride));
  }

  if (mFound) {
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "SoundRef: %s", kMusicMainMenuSoundref);
    ui->UI_Text(buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Live source: %s%s",
      mSource[0] ? mSource : "(no live event)", mActive ? "" : " (inactive)");
    ui->UI_Text(buffer);

    if (mOverride[0]) {
      g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Override: %s", mOverride);
      ui->UI_TextColored(1.0f, 0.8f, 0.2f, 1.0f, buffer);
    } else {
      ui->UI_TextColored(0.6f, 0.6f, 0.6f, 1.0f, "Override: none");
    }
  } else {
    ui->UI_TextColored(1.0f, 0.4f, 0.4f, 1.0f, "SoundRef path not found in catalog.");
  }

  ui->UI_Separator();

}

}  // namespace ExamplePlugin
