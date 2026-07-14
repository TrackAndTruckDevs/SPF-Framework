#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_Vehicle_API.h"

#include <cstdint>


SPF_NS_BEGIN
namespace Modules::API {
class VehicleApi {
 public:
  static void FillVehicleApi(SPF_Vehicle_API* vehicle_api);

 private:
  static bool T_Vehicle_IsReady();
  static SPF_VehicleHandle T_Vehicle_GetPlayerVehicle();
  static SPF_VehicleHandle T_Vehicle_GetVehicleById(int32_t id);
  static uint32_t T_Vehicle_GetCount();
  static uint32_t T_Vehicle_GetAllHandles(SPF_VehicleHandle* out_handles, uint32_t max_count);

  static uintptr_t T_Vehicle_GetTrafficManagerPtr();
  static uintptr_t T_Vehicle_GetLocalPlayerControllerPtr();

  static bool T_Vehicle_AreAllOffsetsFound();
  static bool T_Vehicle_IsFinderReady(const char* finderName);
  static bool T_Vehicle_RefreshOffsets();

  static int32_t T_Vehicle_GetId(SPF_VehicleHandle h);
  static uintptr_t T_Vehicle_GetRawAddress(SPF_VehicleHandle h);
  static float T_Vehicle_GetPatience(SPF_VehicleHandle h);
  static float T_Vehicle_GetSafety(SPF_VehicleHandle h);
  static float T_Vehicle_GetTargetSpeed(SPF_VehicleHandle h);
  static float T_Vehicle_GetSpeedLimit(SPF_VehicleHandle h);
  static float T_Vehicle_GetLaneSpeedInput(SPF_VehicleHandle h);
  static float T_Vehicle_GetCurrentSpeed(SPF_VehicleHandle h);
  static float T_Vehicle_GetAcceleration(SPF_VehicleHandle h);
};
}  // namespace Modules::API
SPF_NS_END
