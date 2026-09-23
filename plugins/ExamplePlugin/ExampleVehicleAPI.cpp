/**
 * @file ExampleVehicleAPI.cpp
 * @brief Implementation of the SPF Vehicle API example.
 *
 * @details HANDLE LIFETIME: SPF_VehicleHandle is only valid while that vehicle exists.
 * Pattern used here (every frame):
 *   1. Veh_GetAllHandles → refresh the list
 *   2. if selected handle not in list → clear selection
 *   3. only then read Veh_Get* for the selection
 *
 * FORGETTING step 2 is the classic use-after-despawn bug (stale handle → bad data).
 */
#include "ExampleVehicleAPI.hpp"

#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/SPF_API/SPF_Vehicle_API.h"

#include "ExamplePlugin.hpp"

#include <cstdint>

namespace ExamplePlugin {

void RenderVehicleTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.vehicleAPI) {
    ui->UI_Text("Vehicle API is not available.");
    return;
  }

  // 1. Refresh handle list every frame — traffic comes and goes.
  uint32_t count = g_ctx.vehicleAPI->Veh_GetCount();

  // Over-allocate so GetAllHandles never truncates on a sudden spawn burst.
  if (g_ctx.vehicleHandles.size() < count + 10) {
    g_ctx.vehicleHandles.resize(count + 50);
  }

  uint32_t actualCount = g_ctx.vehicleAPI->Veh_GetAllHandles(g_ctx.vehicleHandles.data(), (uint32_t)g_ctx.vehicleHandles.size());

  // 2. Build combo preview; drop selection if the vehicle despawned.
  char previewText[64] = "Select Vehicle...";
  if (g_ctx.selectedVehicle) {
    bool stillExists = false;
    for (uint32_t i = 0; i < actualCount; ++i) {
      if (g_ctx.vehicleHandles[i] == g_ctx.selectedVehicle) {
        stillExists = true;
        break;
      }
    }

    if (stillExists) {
      int32_t id = g_ctx.vehicleAPI->Veh_GetId(g_ctx.selectedVehicle);
      g_ctx.coreAPI->formatting->Fmt_Format(previewText, sizeof(previewText), "Vehicle ID: %d", id);
    } else {
      g_ctx.selectedVehicle = nullptr;  // Despawned — do not keep reading its handle.
      g_ctx.coreAPI->formatting->Fmt_Format(previewText, sizeof(previewText), "Select Vehicle...");
    }
  }

  // 3. Combo of live vehicles; onSelect only stores the handle (no heavy work in the list loop).
  if (ui->UI_BeginCombo("Target Vehicle", previewText, SPF_COMBO_FLAG_NONE)) {
    for (uint32_t i = 0; i < actualCount; ++i) {
      SPF_VehicleHandle h = g_ctx.vehicleHandles[i];
      int32_t id = g_ctx.vehicleAPI->Veh_GetId(h);

      char itemLabel[64];
      g_ctx.coreAPI->formatting->Fmt_Format(itemLabel, sizeof(itemLabel), "Vehicle #%d", id);

      bool isSelected = (g_ctx.selectedVehicle == h);
      if (ui->UI_Selectable(itemLabel, isSelected, SPF_SELECTABLE_FLAG_NONE, 0.0f, 0.0f)) {
        g_ctx.selectedVehicle = h;
      }
    }
    ui->UI_EndCombo();
  }

  ui->UI_Separator();

  // 4. Display for selection — pure getters; safe because step 2 validated the handle.
  if (g_ctx.selectedVehicle) {
    SPF_VehicleHandle h = g_ctx.selectedVehicle;

    float speed = g_ctx.vehicleAPI->Veh_GetCurrentSpeed(h);
    float accel = g_ctx.vehicleAPI->Veh_GetAcceleration(h);
    float targetSpeed = g_ctx.vehicleAPI->Veh_GetTargetSpeed(h);
    float speedLimit = g_ctx.vehicleAPI->Veh_GetSpeedLimit(h);
    float patience = g_ctx.vehicleAPI->Veh_GetPatience(h);
    float safety = g_ctx.vehicleAPI->Veh_GetSafety(h);

    char buffer[64];

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s (%.0f km/h)", speed, speed * 3.6f);
    ui->UI_LabelText("Speed", buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s^2", accel);
    ui->UI_LabelText("Acceleration", buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s", targetSpeed);
    ui->UI_LabelText("Target Speed", buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f m/s", speedLimit);
    ui->UI_LabelText("Speed Limit", buffer);

    ui->UI_Separator();

    // AI behaviour scores exposed by the traffic system.
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f", patience);
    ui->UI_LabelText("AI Patience", buffer);

    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "%.2f", safety);
    ui->UI_LabelText("AI Safety", buffer);

    // Raw game address — for advanced debugging / Cheat Engine cross-check.
    uintptr_t addr = g_ctx.vehicleAPI->Veh_GetRawAddress(h);
    g_ctx.coreAPI->formatting->Fmt_Format(buffer, sizeof(buffer), "0x%llX", (unsigned long long)addr);
    ui->UI_LabelText("Address", buffer);
  } else {
    ui->UI_Text("Please select a vehicle to inspect.");
    ui->UI_Text("Note: Use the 'Traffic' debug camera mode to see vehicle IDs.");
  }
}

}  // namespace ExamplePlugin
