/**
 * @file ExampleEnvironmentAPI.cpp
 * @brief Implementation of the SPF Environment API example.
 *
 * @details HANDLE MODEL: Env_* getters take SPF_Environment_Handle* (from GetHandle/GetHandleForLoadAPI)
 * plus a caller buffer. Always size the buffer — getters never return owning std::string.
 *
 * Lifecycle: create in OnLoad (early path resolution needed by other modules, e.g. Sound/JSON
 * data dirs), destroy in OnUnload to avoid dangling handle use after framework teardown.
 */
#include "ExampleEnvironmentAPI.hpp"

#include "SPF/SPF_API/SPF_Icons.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

namespace ExamplePlugin {

void EnvironmentAPI_OnLoad() {
  if (!g_ctx.loadAPI || !g_ctx.loadAPI->environment || g_ctx.environmentHandle) {
    return;  // Already created or API missing — idempotent init.
  }

  g_ctx.environmentAPI = g_ctx.loadAPI->environment;
  // Env_GetContext(PLUGIN_NAME): plugin-scoped environment context, valid from OnLoad.
  g_ctx.environmentHandle = g_ctx.environmentAPI->Env_GetContext(PLUGIN_NAME);
}

void EnvironmentAPI_OnUnload() {
  // Null after destroy — any accidental late call must no-op, not use freed memory.
  g_ctx.environmentHandle = nullptr;
}

void RenderEnvironmentTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.environmentAPI || !g_ctx.environmentHandle) {
    ui->UI_Text("Environment API is not available.");
    return;
  }

  auto env = g_ctx.environmentAPI;
  auto h = g_ctx.environmentHandle;
  char buffer[512];

  ui->UI_TextWrapped("This tab demonstrates the Environment API, which provides details about the game, framework, and system.");
  ui->UI_Separator();

  // --- Section 1: Game & Profile ---
  if (ui->UI_TreeNode(ICON_FA_TRUCK " Game & Profile")) {
    env->Env_GetGameName(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Game Name", buffer);

    env->Env_GetGameVersion(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Game Version", buffer);

    env->Env_GetActiveProfileName(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Active Profile", buffer);

    ui->UI_TreePop();
  }

  // --- Section 2: Filesystem Paths ---
  // These are resolved by the framework — do not hand-roll game-dir concatenation.
  if (ui->UI_TreeNode(ICON_FA_FOLDER_OPEN " Resolved Paths")) {
    env->Env_GetSCSUserDir(h, buffer, sizeof(buffer));
    ui->UI_LabelText("User Dir (/home)", buffer);

    env->Env_GetCurrentProfilePath(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Profile Path", buffer);

    env->Env_GetSCSMusicDir(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Music Dir", buffer);

    ui->UI_TreePop();
  }

  // --- Section 3: Runtime Status ---
  if (ui->UI_TreeNode(ICON_FA_CHART_LINE " Runtime Status")) {
    ui->UI_LabelText("VR Active", env->Env_IsVRActive(h) ? "Yes" : "No");
    ui->UI_LabelText("Tobii DLL", env->Env_IsTobiiDllLoaded(h) ? "Loaded" : "Not Loaded");
    ui->UI_LabelText("Steam Overlay", env->Env_IsSteamOverlayDllLoaded(h) ? "Loaded" : "Not Loaded");

    env->Env_GetMultiplayerStatus(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Multiplayer", buffer);

    env->Env_GetRendererName(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Renderer", buffer);

    ui->UI_TreePop();
  }

  // --- Section 4: System Info ---
  if (ui->UI_TreeNode(ICON_FA_GEAR " System Info")) {
    env->Env_GetOSName(h, buffer, sizeof(buffer));
    ui->UI_LabelText("OS Version", buffer);

    env->Env_GetSystemLocale(h, buffer, sizeof(buffer));
    ui->UI_LabelText("Locale", buffer);

    ui->UI_TreePop();
  }
}

}  // namespace ExamplePlugin
