/**
 * @file ExampleSoundAPI.cpp
 * @brief Implementation of the SPF Sound API example (bank + horn→bell replacement).
 *
 * @details REPLACEMENT FLOW (exact port of monolithic OnUpdate logic):
 *   Checkbox ON  → SND_LoadBankFile (plugin .bank under Env_GetPluginDataDir)
 *   OnUpdate     → resolve bellEventIndex via bank GUID (lazy), on resolve create
 *                  instance + cache horn indices (path prefix event:/horn/),
 *                  per-frame read live 'play' param → SetEventVolume(0) on game
 *                  horn, edge-trigger bell Start/Stop
 *   Checkbox OFF → stop bell only + clear hornEventCount in OnUpdate; UI
 *                  uncheck does full teardown (stop, release, unload)
 *
 * WHY poll in OnUpdate: no push "horn started" callback — polling is the
 * supported pattern. Detection uses 'play' param, NOT live instance count.
 */
#include "ExampleSoundAPI.hpp"

#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/SPF_API/SPF_Sound_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>
#include <cstring>

namespace ExamplePlugin {

void SoundAPI_OnUpdate() {
  // PERFORMANCE: entire block only runs while checkbox is on AND sound system ready.
  // When disabled, zero sound API calls per frame (except the cheap disable-cleanup).
  if (!g_ctx.replaceHornEnabled || !g_ctx.soundAPI || !g_ctx.coreAPI || !g_ctx.soundAPI->SND_IsReady()) {
    // --- Cleanup when horn replacement is disabled ---
    // Stop the bell and clear cached horn event indices so they're re-scanned
    // if re-enabled. Bank + instance stay (re-enable is cheap).
    if (!g_ctx.replaceHornEnabled && g_ctx.bellReplacementActive && g_ctx.bellInstance) {
      if (g_ctx.soundAPI) {
        g_ctx.soundAPI->SND_StopEvent(g_ctx.bellInstance, true);
      }
      g_ctx.bellReplacementActive = false;
      g_ctx.hornEventCount = 0;
    }
    return;
  }
  auto snd = g_ctx.soundAPI;

  // --- Step 1: Lazy-resolve the bell event index ---
  // After SND_LoadBankFile the event is NOT immediately in the global cache —
  // FMOD needs at least one System::Update() tick. Retry each frame until GUID lookup works.
  // Resolution: GetBankEventCount → GetBankEventGuid(bank, 0) → FindEventIndexByGuid.
  if (g_ctx.bellEventIndex < 0 && g_ctx.bellBank) {
    int bankEventCount = snd->SND_GetBankEventCount(g_ctx.bellBank);
    if (bankEventCount > 0) {
      uint8_t eventGuid[16];
      if (snd->SND_GetBankEventGuid(g_ctx.bellBank, 0, eventGuid)) {
        g_ctx.bellEventIndex = snd->SND_FindEventIndexByGuid(eventGuid);
      }
    }

    if (g_ctx.bellEventIndex >= 0) {
      // Reusable instance from plugin bank; release with SND_ReleaseEvent on shutdown.
      g_ctx.bellInstance = snd->SND_CreateEventInstance(g_ctx.bellEventIndex);

      // Cache ALL game horn events (path prefix event:/horn/) — each truck brand
      // has its own horn variant; monitor all of them.
      int eventCount = snd->SND_GetEventCount();
      g_ctx.hornEventCount = 0;
      for (int i = 0; i < eventCount && g_ctx.hornEventCount < 32; i++) {
        char path[256];
        snd->SND_GetEventPath(i, path, sizeof(path));
        if (strncmp(path, "event:/horn/", 12) == 0) {
          g_ctx.hornEventIndices[g_ctx.hornEventCount++] = i;
        }
      }

      char logBuf[128];
      g_ctx.coreAPI->formatting->Fmt_Format(logBuf, sizeof(logBuf),
        "Bell event resolved, horn events cached: %d", g_ctx.hornEventCount);
      g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, logBuf);
    }
  }

  // Safety: recreate instance if lost (e.g. after world reload).
  if (!g_ctx.bellInstance && g_ctx.bellEventIndex >= 0) {
    g_ctx.bellInstance = snd->SND_CreateEventInstance(g_ctx.bellEventIndex);
  }

  // --- Step 2: Per-frame horn detection via 'play' parameter ---
  // HOW THE GAME HORN WORKS:
  //   - Game creates a live EventInstance on button press, keeps it while held.
  //   - Instance has local parameter "play" (0.0=released, 1.0=pressed).
  // WHY NOT live instance count / playback state alone:
  //   - Live count may include idle pre-allocated instances.
  //   - Playback state doesn't distinguish pressed from idle; default params on
  //     idle instances cause false positives (play=1.0 on STOPPED instance).
  // FILTERING: only read parameter when state == PLAYING (0).
  // ACTION: play > 0 → SetEventVolume(0) on the GAME instance. Never StopEvent
  //   on game-owned instances (undefined behavior).
  bool anyHornActive = false;
  for (int h = 0; h < g_ctx.hornEventCount; h++) {
    int idx = g_ctx.hornEventIndices[h];
    int liveCount = snd->SND_GetEventLiveInstanceCount(idx);
    for (int j = 0; j < liveCount; j++) {
      void* inst = snd->SND_GetEventLiveInstance(idx, j);
      if (!inst) continue;
      int state = snd->SND_GetEventPlaybackState(inst);
      if (state != 0) continue;  // 0 = FMOD_STUDIO_PLAYBACK_PLAYING
      float playValue = 0.0f;
      if (snd->SND_GetEventParameter(inst, "play", &playValue) && playValue > 0.0f) {
        anyHornActive = true;
        snd->SND_SetEventVolume(inst, 0.0f);  // Silence the game horn
      }
    }
  }

  // --- Step 3: Bell state machine ---
  // IDLE→PLAYING: anyHornActive true (button pressed); PLAYING→IDLE: released.
  // Bell is LOOPED — Start plays until Stop. bellReplacementActive prevents
  // calling StartEvent every frame (would restart from the beginning).
  if (anyHornActive && !g_ctx.bellReplacementActive && g_ctx.bellInstance) {
    snd->SND_StartEvent(g_ctx.bellInstance);
    g_ctx.bellReplacementActive = true;
  } else if (!anyHornActive && g_ctx.bellReplacementActive && g_ctx.bellInstance) {
    snd->SND_StopEvent(g_ctx.bellInstance, true);  // true = allow fadeout
    g_ctx.bellReplacementActive = false;
  }
}

void SoundAPI_Shutdown() {
  if (!g_ctx.soundAPI) {
    g_ctx.replaceHornEnabled = false;
    g_ctx.bellEventIndex = -1;
    g_ctx.bellInstance = nullptr;
    g_ctx.bellBank = nullptr;
    g_ctx.hornEventCount = 0;
    g_ctx.bellTestPlaying = false;
    g_ctx.bellReplacementActive = false;
    return;
  }

  auto snd = g_ctx.soundAPI;
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
  g_ctx.hornEventCount = 0;
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

  // Show bus info
  if (ui->UI_TreeNode("Bus List")) {
    for (int i = 0; i < busCount && i < 100; i++) {
      char busPath[256];
      snd->SND_GetBusPath(i, busPath, sizeof(busPath));
      float vol = snd->SND_GetBusVolume(i);
      bool muted = snd->SND_GetBusMute(i);
      g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "[%d] %s (vol: %.2f%s)", i, busPath, vol, muted ? ", MUTED" : "");
      ui->UI_Text(buffer);
    }
    ui->UI_TreePop();
  }

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
          g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Horn replacement enabled, bank loaded. Bell event will resolve on next tick.");
        } else {
          g_ctx.replaceHornEnabled = false;
          g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Failed to load bicycle_bell.bank.");
        }
      }
    }
  } else {
    ui->UI_TextColored(0.4f, 1.0f, 0.4f, 1.0f, "Horn replacement ACTIVE");
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Bell event: %d | Horn events cached: %d", g_ctx.bellEventIndex, g_ctx.hornEventCount);
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
        g_ctx.hornEventCount = 0;
        g_ctx.bellTestPlaying = false;
        g_ctx.bellReplacementActive = false;
        g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Horn replacement disabled.");
      }
    }
  }

  ui->UI_Separator();

  // --- Event Browser ---
  if (ui->UI_TreeNode("Event Browser")) {
    int maxShow = eventCount < 200 ? eventCount : 200;
    for (int i = 0; i < maxShow; i++) {
      char path[256];
      snd->SND_GetEventPath(i, path, sizeof(path));
      int liveCount = snd->SND_GetEventLiveInstanceCount(i);
      if (liveCount > 0) {
        g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "[%d] %s (live: %d)", i, path, liveCount);
        ui->UI_TextColored(1.0f, 0.8f, 0.2f, 1.0f, buffer);
      }
    }
    ui->UI_TreePop();
  }
}

}  // namespace ExamplePlugin
