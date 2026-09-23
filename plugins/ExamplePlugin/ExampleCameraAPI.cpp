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
}

}  // namespace ExamplePlugin
