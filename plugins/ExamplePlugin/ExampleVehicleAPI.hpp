/**
 * @file ExampleVehicleAPI.hpp
 * @brief Complete example of the SPF Vehicle API — enumerate traffic, inspect one vehicle.
 *
 * @details Vehicle API returns opaque SPF_VehicleHandle values for AI/player traffic.
 * Handles can disappear when vehicles despawn — re-validate against the live list each frame.
 *
 * DEVELOPER NOTE: For "show info about nearby cars" or "select a traffic car", start here.
 */
#pragma once

#include "SPF/SPF_API/SPF_UI_API.h"

namespace ExamplePlugin {

/**
 * @brief Renders the Traffic Inspector tab: vehicle combo + selected-vehicle telemetry.
 * @details Requires g_ctx.vehicleAPI (cached in OnActivated).
 */
void RenderVehicleTab(SPF_UI_API* ui, void* user_data);

}  // namespace ExamplePlugin
