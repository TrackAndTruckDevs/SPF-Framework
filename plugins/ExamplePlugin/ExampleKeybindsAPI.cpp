/**
 * @file ExampleKeybindsAPI.cpp
 * @brief Implementation of the SPF KeyBinds API example.
 *
 * @details MENTAL MODEL:
 *   Action id  = stable string code and bind table agree on ("MainWindow.toggle").
 *   Title/Desc = player-facing strings for the settings screen (dynamic metadata only;
 *                values are localization keys when using RegisterActionMetadata).
 *   Callback   = what to do when the player presses the bound input.
 *
 * If the player never binds the action, the callback never fires — design features to be
 * fully usable with zero binds (UI buttons) and treat binds as a shortcut layer.
 */
#include "ExampleKeybindsAPI.hpp"

#include "SPF/SPF_API/SPF_Icons.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

#include <cstring>  // strstr — skip known static actions when restoring dynamic ones

namespace ExamplePlugin {

// ------------------------------------------------------------------------------------------------
// Lifecycle
// ------------------------------------------------------------------------------------------------

void KeybindsAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds) {
    return;
  }

  // Context handle groups *this plugin's* actions. One context for the plugin lifetime.
  g_ctx.keybindsHandle = g_ctx.coreAPI->keybinds->Kbind_GetContext(PLUGIN_NAME);
  if (!g_ctx.keybindsHandle) {
    return;
  }

  // --- Static actions: always present; player may or may not bind them ---
  // Callback must be void(void) for Kbind_Register (or use Kbind_Register_Ex for user_data).
  g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "MainWindow.toggle", OnToggleMainWindow);

  // Camera module owns the *behavior* (OnCameraKeybind lives with ExampleCameraAPI) but
  // registration often happens next to other built-ins so the action list stays readable.
  // See also: ExampleCameraAPI for the callback implementation itself.
  // (Declared there; registered here only if we include that TU's symbol — the original
  //  example registers both from this module's activation path via OnActivated.)

  // Demo honk: capture-less lambda converts to a plain function pointer for Kbind_Register.
  // Keep the body tiny — real play sounds belong in ExampleSoundAPI behind a named helper.
  g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "Demo.honk", []() {
    auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);
    g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, "BEEP! (Honk action triggered in plugin)");
  });

  // --- Restore dynamic actions the player already bound ---
  // WHY explicit restore? Dynamic metadata is added by *code* each session; the framework
  // remembers key→action_id mappings, but without re-registering the action id the mapping
  // has no target. Same id ⇒ same player bind reconnects.
  // Skip the static ids above — re-registering them as Ex would replace the dedicated callbacks.
  int actionCount = g_ctx.coreAPI->keybinds->Kbind_GetActionCount(g_ctx.keybindsHandle);
  for (int i = 0; i < actionCount; i++) {
    char actionName[128];
    g_ctx.coreAPI->keybinds->Kbind_GetActionNameByIndex(g_ctx.keybindsHandle, i, actionName, sizeof(actionName));

    if (strstr(actionName, "MainWindow.toggle") == nullptr && strstr(actionName, "Camera.cycle") == nullptr && strstr(actionName, "Demo.honk") == nullptr && strstr(actionName, "Test.Axis") == nullptr) {
      // user_data tag lets OnDynamicActionTriggered distinguish "restored on load"
      // from "user just clicked Register" in the same callback.
      g_ctx.coreAPI->keybinds->Kbind_Register_Ex(g_ctx.keybindsHandle, actionName, OnDynamicActionTriggered, (void*)"RestoredDynamic");
    }
  }

  g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Registered keybinds.");
}

// ------------------------------------------------------------------------------------------------
// Callbacks
// ------------------------------------------------------------------------------------------------

void OnToggleMainWindow() {
  if (!g_ctx.uiAPI || !g_ctx.mainWindowHandle) {
    return;
  }
  // Read current visibility from the framework, then apply the inverse — keybinds never
  // touch tab contents, only the window shell created in OnRegisterUI.
  const bool isCurrentlyVisible = g_ctx.uiAPI->UI_IsVisible(g_ctx.mainWindowHandle);
  g_ctx.uiAPI->UI_SetVisibility(g_ctx.mainWindowHandle, !isCurrentlyVisible);

  char log_buffer[256];
  g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Main window visibility toggled to: %s", !isCurrentlyVisible ? "visible" : "hidden");
  g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
}

void OnDynamicActionTriggered(const char* action_id, void* user_data) {
  if (!g_ctx.coreAPI) {
    return;
  }
  auto logger = g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME);

  char log_buf[256];
  g_ctx.coreAPI->formatting->Fmt_Format(log_buf, sizeof(log_buf), ">>> DYNAMIC ACTION TRIGGERED: %s (Context: %s) <<<", action_id ? action_id : "NULL", user_data ? (const char*)user_data : "NULL");
  g_ctx.coreAPI->logger->Log(logger, SPF_LOG_INFO, log_buf);

  if (g_ctx.uiAPI) {
    char msg_buf[256];
    g_ctx.coreAPI->formatting->Fmt_Format(msg_buf, sizeof(msg_buf), "Action '%s' Triggered!", action_id ? action_id : "Unknown");
    SPF_Notification_Params p = {SPF_NOTIFICATION_SUCCESS, msg_buf, SPF_NOTIF_MODE_TOP, 2.0f};
    g_ctx.uiAPI->UI_ShowNotification(&p);
  }
}

// ------------------------------------------------------------------------------------------------
// UI: Dynamic Keybinds tab
// ------------------------------------------------------------------------------------------------

void RenderDynamicKeybindsTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;  // Passed by framework/parent; state lives on g_ctx / statics for this demo.

  if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds || !g_ctx.keybindsHandle || !ui) {
    ui->UI_Text("Keybinds API not available.");
    return;
  }

  ui->UI_Text("This tab tests dynamic keybind registration at runtime.");
  ui->UI_Separator();

  // Collect metadata first, then register in one call — splitting metadata and callback
  // into two steps would leave half-registered actions if the second call failed.
  static char inputActionID[64] = "MyCustomAction";
  static char inputTitle[64] = "My Custom Action";
  static char inputDesc[128] = "This action was added manually.";

  ui->UI_Text("Action ID (internal):");
  ui->UI_InputText("##ActionID", inputActionID, sizeof(inputActionID), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_Text("Title (Display Name):");
  ui->UI_InputText("##ActionTitle", inputTitle, sizeof(inputTitle), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_Text("Description (Tooltip):");
  ui->UI_InputText("##ActionDesc", inputDesc, sizeof(inputDesc), SPF_INPUT_TEXT_FLAG_NONE);
  ui->UI_Spacing();

  if (ui->UI_Button(ICON_FA_PLUS " Register New Dynamic Action", 0, 0)) {
    if (inputActionID[0] == '\0') {
      SPF_Notification_Params p = {SPF_NOTIFICATION_ERROR, "Action ID cannot be empty!", SPF_NOTIF_MODE_TOP, 3.0f};
      ui->UI_ShowNotification(&p);
    } else {
      // One call: metadata + callback + context pointer. Same callback for every dynamic
      // action — distinguish behavior by action_id inside OnDynamicActionTriggered.
      g_ctx.coreAPI->keybinds->Kbind_RegisterActionMetadata(g_ctx.keybindsHandle, inputActionID, inputTitle, inputDesc, OnDynamicActionTriggered, (void*)"DynamicContext");

      char successMsg[256];
      g_ctx.coreAPI->formatting->Fmt_Format(successMsg, sizeof(successMsg), "Action '%s' registered!", inputActionID);
      SPF_Notification_Params p = {SPF_NOTIFICATION_HINT, successMsg, SPF_NOTIF_MODE_STACK, 3.0f};
      ui->UI_ShowNotification(&p);
    }
  }

  ui->UI_Separator();
  ui->UI_Text("Currently Managed Actions:");
  int count = g_ctx.coreAPI->keybinds->Kbind_GetActionCount(g_ctx.keybindsHandle);
  if (count == 0) {
    ui->UI_Text("No actions found.");
  } else {
    for (int i = 0; i < count; i++) {
      char name[128];
      g_ctx.coreAPI->keybinds->Kbind_GetActionNameByIndex(g_ctx.keybindsHandle, i, name, sizeof(name));
      ui->UI_PushID_Int(i);
      if (ui->UI_Button(ICON_FA_TRASH_CAN, 0, 0)) {
        g_ctx.coreAPI->keybinds->Kbind_UnregisterActionMetadata(g_ctx.keybindsHandle, name);
        char delMsg[256];
        g_ctx.coreAPI->formatting->Fmt_Format(delMsg, sizeof(delMsg), "Removed action: %s", name);
        SPF_Notification_Params p = {SPF_NOTIFICATION_WARNING, delMsg, SPF_NOTIF_MODE_STACK, 3.0f};
        ui->UI_ShowNotification(&p);
      }
      ui->UI_SameLine(0, 5);
      ui->UI_Text(name);
      ui->UI_PopID();
    }
  }
}

// ------------------------------------------------------------------------------------------------
// UI: input blocking (General tab)
// ------------------------------------------------------------------------------------------------

void RenderKeybindBlockingSection(SPF_UI_API* ui) {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds) {
    ui->UI_Text("Keybinds API is not available.");
    return;
  }

  ui->UI_Text("Dynamic Input Blocking (requires 'Plugin Managed' in settings):");

  // Level 1: suppress one bound action by id while the checkbox is on.
  // WHY on change only? The framework applies/removes game-side suppression when the
  // flag *changes*; your job is to report the desired mode, not re-apply every frame.
  if (ui->UI_Checkbox("Block Game Horn (H key)", &g_ctx.isHonkIntercepted)) {
    auto h = g_ctx.coreAPI->keybinds->Kbind_GetContext(PLUGIN_NAME);
    g_ctx.coreAPI->keybinds->Kbind_SetBlockState(h, "Demo.honk", g_ctx.isHonkIntercepted);
  }

  // Level 2: suppress game mouse-look so dragging the plugin window doesn't turn the camera.
  // Third/fourth args are additional blocking dimensions (keyboard/gamepad) — false here
  // because this demo only blocks look.
  static bool blockMouse = false;
  if (ui->UI_Checkbox("Block Game Mouse Look", &blockMouse)) {
    ui->UI_SetMouseBlockState(blockMouse, false, false);
  }

  ui->UI_Separator();
}

}  // namespace ExamplePlugin
