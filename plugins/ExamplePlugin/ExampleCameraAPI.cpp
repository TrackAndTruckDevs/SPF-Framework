/**
 * @file ExampleCameraAPI.cpp
 * @brief Implementation of the SPF Camera API example.
 *
 * @details TWO SURFACES:
 *   - Direct control: Cam_GetCurrentCamera / Cam_SwitchTo from UI buttons.
 *   - Player shortcut: OnCameraKeybind registered as keybind "Camera.cycle".
 *
 * READY CHECK: Cam_* may fail before the world is loaded — always branch on the
 * boolean return rather than assuming a valid camera type.
 */
#include "ExampleCameraAPI.hpp"

#include "SPF/SPF_API/SPF_Camera_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include "ExamplePlugin.hpp"

namespace ExamplePlugin {

void CameraAPI_OnActivated() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds || !g_ctx.keybindsHandle) {
    return;
  }

  // Static action: fixed id from the manifest, dedicated callback.
  // Must run after KeybindsAPI_OnActivated (handle) and before dynamic restore
  // so the restore loop can skip "Camera.cycle" by name (see ExampleKeybindsAPI).
  g_ctx.coreAPI->keybinds->Kbind_Register(g_ctx.keybindsHandle, "Camera.cycle", OnCameraKeybind);

  g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, "Registered Camera.cycle keybind.");
}

void OnCameraKeybind() {
  if (!g_ctx.coreAPI || !g_ctx.coreAPI->camera) {
    return;
  }

  SPF_CameraType current_camera_type;
  if (g_ctx.coreAPI->camera->Cam_GetCurrentCamera(&current_camera_type)) {
    // Cycle order: Interior → Behind → Developer Free → Interior.
    SPF_CameraType next_camera_type = (current_camera_type == SPF_CAMERA_INTERIOR) ? SPF_CAMERA_BEHIND : (current_camera_type == SPF_CAMERA_BEHIND) ? SPF_CAMERA_DEVELOPER_FREE : SPF_CAMERA_INTERIOR;
    g_ctx.coreAPI->camera->Cam_SwitchTo(next_camera_type);

    char log_buffer[256];
    g_ctx.coreAPI->formatting->Fmt_Format(log_buffer, sizeof(log_buffer), "Switched camera from %d to %d via keybind.", current_camera_type, next_camera_type);
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_INFO, log_buffer);
  } else {
    g_ctx.coreAPI->logger->Log(g_ctx.coreAPI->logger->Log_GetContext(PLUGIN_NAME), SPF_LOG_WARN, "Could not get current camera type to cycle.");
  }
}

void RenderCameraTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.coreAPI || !g_ctx.coreAPI->camera || !ui) {
    ui->UI_Text("Camera API is not available.");
    return;
  }
  ui->UI_Text("Use this tab to interact with the game's camera system.");
  ui->UI_Text("You can also press F6 to cycle through the cameras.");
  ui->UI_Separator();

  // Current mode as an integer — map to names yourself if the enum has no string helper.
  SPF_CameraType current_camera;
  if (g_ctx.coreAPI->camera->Cam_GetCurrentCamera(&current_camera)) {
    char buffer[256];
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "Current Camera Type: %d", current_camera);
    ui->UI_Text(buffer);
  } else {
    ui->UI_Text("Could not retrieve current camera type.");
  }
  ui->UI_Separator();

  // Direct switch buttons — each Cam_SwitchTo is a one-shot request, no need to poll.
  ui->UI_Text("Switch to a specific camera:");
  if (ui->UI_Button("Interior", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_INTERIOR);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Behind", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_BEHIND);
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Developer Free", 0, 0)) g_ctx.coreAPI->camera->Cam_SwitchTo(SPF_CAMERA_DEVELOPER_FREE);
  ui->UI_Separator();

  // World-space position of the active camera (useful for developer tools / markers).
  float x, y, z;
  if (g_ctx.coreAPI->camera->Cam_GetCameraWorldCoordinates(&x, &y, &z)) {
    char buffer[256];
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "X: %.2f, Y: %.2f, Z: %.2f", x, y, z);
    ui->UI_Text("Current Camera Position:");
    ui->UI_Text(buffer);
  } else {
    ui->UI_Text("Could not get camera world coordinates.");
  }
  ui->UI_Separator();

  // --- Issue #12 full test ---
  ui->UI_Text("Issue #12 test");

  // 1) Finder readiness — both name variants.
  bool all_found = g_ctx.coreAPI->camera->Cam_AreAllOffsetsFound();
  bool ready_short = g_ctx.coreAPI->camera->Cam_IsFinderReady("InteriorCamera");
  bool ready_full = g_ctx.coreAPI->camera->Cam_IsFinderReady("InteriorCameraDataFinder");
  char ready_buffer[256];
  g_ctx.coreAPI->formatting->Fmt_Format(ready_buffer, sizeof(ready_buffer),
                                        "AllOffsetsFound=%d IsFinderReady(InteriorCamera)=%d IsFinderReady(InteriorCameraDataFinder)=%d",
                                        all_found ? 1 : 0, ready_short ? 1 : 0, ready_full ? 1 : 0);
  ui->UI_Text(ready_buffer);
  ui->UI_Separator();

  // 2) Roll verification.
  static float s_rollTest = 0.0f;
  ui->UI_Text("Roll test:");
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("+10 deg", 0, 0)) {
    s_rollTest = 10.0f;
    g_ctx.coreAPI->camera->Cam_SetInteriorRoll(s_rollTest);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("-10 deg", 0, 0)) {
    s_rollTest = -10.0f;
    g_ctx.coreAPI->camera->Cam_SetInteriorRoll(s_rollTest);
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Reset", 0, 0)) {
    s_rollTest = 0.0f;
    g_ctx.coreAPI->camera->Cam_SetInteriorRoll(0.0f);
  }
  float roll_readback = 0.0f;
  bool roll_ok = g_ctx.coreAPI->camera->Cam_GetInteriorRoll(&roll_readback);
  char roll_buffer[256];
  g_ctx.coreAPI->formatting->Fmt_Format(roll_buffer, sizeof(roll_buffer), "Requested: %.1f deg | GetInteriorRoll: %s (%.2f deg)",
                                        s_rollTest, roll_ok ? "OK" : "FAILED", roll_readback);
  ui->UI_Text(roll_buffer);
  ui->UI_Separator();

  // 3) Head rotation — absolute overwrite.
  ui->UI_Text("HeadRot absolute (radians):");
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Pitch -0.5", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw, -0.5f);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Pitch +0.5", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw, 0.5f);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Pitch 0", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw, 0.0f);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Yaw +0.5", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw + 0.5f, pitch);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Yaw 0", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(0.0f, pitch);
    }
  }
  ui->UI_Separator();

  // 4) Additive offset — the exact usage pattern from the issue:
  //    basePitch + newPitchOffset, re-read every press.
  ui->UI_Text("HeadRot additive:");
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Pitch +0.3", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw, pitch + 0.3f);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Pitch -0.3", 0, 0)) {
    float yaw, pitch;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw, &pitch)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorHeadRot(yaw, pitch - 0.3f);
    }
  }
  ui->UI_SameLine(0, 5);
  if (ui->UI_Button("Roll +5 (additive)", 0, 0)) {
    float roll = 0.0f;
    if (g_ctx.coreAPI->camera->Cam_GetInteriorRoll(&roll)) {
      g_ctx.coreAPI->camera->Cam_SetInteriorRoll(roll + 5.0f);
      s_rollTest = roll + 5.0f;
    }
  }
  ui->UI_Separator();

  // 5) Readbacks — verify writes actually landed (or got overwritten by the game).
  float yaw_now = 0.0f, pitch_now = 0.0f;
  bool head_ok = g_ctx.coreAPI->camera->Cam_GetInteriorHeadRot(&yaw_now, &pitch_now);
  char head_buffer[256];
  g_ctx.coreAPI->formatting->Fmt_Format(head_buffer, sizeof(head_buffer),
                                        "GetInteriorHeadRot: %s | yaw=%.3f pitch=%.3f rad",
                                        head_ok ? "OK" : "FAILED", yaw_now, pitch_now);
  ui->UI_Text(head_buffer);

  float lim_l, lim_r, lim_u, lim_d;
  bool lim_ok = g_ctx.coreAPI->camera->Cam_GetInteriorRotationLimits(&lim_l, &lim_r, &lim_u, &lim_d);
  char lim_buffer[256];
  g_ctx.coreAPI->formatting->Fmt_Format(lim_buffer, sizeof(lim_buffer),
                                        "GetInteriorRotationLimits: %s | L=%.3f R=%.3f U=%.3f D=%.3f",
                                        lim_ok ? "OK" : "FAILED", lim_l, lim_r, lim_u, lim_d);
  ui->UI_Text(lim_buffer);
}

}  // namespace ExamplePlugin
