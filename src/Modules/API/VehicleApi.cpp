#include "SPF/Modules/API/VehicleApi.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/SPF_API/SPF_Vehicle_API.h"
#include "SPF/Utils/Windows.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Modules::API {
using namespace SPF::Data::GameData;

void VehicleApi::FillVehicleApi(SPF_Vehicle_API* vehicle_api) {
  if (!vehicle_api) return;
  vehicle_api->Veh_IsReady = &T_Vehicle_IsReady;
  vehicle_api->Veh_GetPlayerVehicle = &T_Vehicle_GetPlayerVehicle;
  vehicle_api->Veh_GetVehicleById = &T_Vehicle_GetVehicleById;
  vehicle_api->Veh_GetCount = &T_Vehicle_GetCount;
  vehicle_api->Veh_GetAllHandles = &T_Vehicle_GetAllHandles;

  vehicle_api->Veh_GetTrafficManagerPtr = &T_Vehicle_GetTrafficManagerPtr;
  vehicle_api->Veh_GetLocalPlayerControllerPtr = &T_Vehicle_GetLocalPlayerControllerPtr;

  vehicle_api->Veh_AreAllOffsetsFound = &T_Vehicle_AreAllOffsetsFound;
  vehicle_api->Veh_IsFinderReady = &T_Vehicle_IsFinderReady;
  vehicle_api->Veh_RefreshOffsets = &T_Vehicle_RefreshOffsets;

  vehicle_api->Veh_GetId = &T_Vehicle_GetId;
  vehicle_api->Veh_GetRawAddress = &T_Vehicle_GetRawAddress;
  vehicle_api->Veh_GetPatience = &T_Vehicle_GetPatience;
  vehicle_api->Veh_GetSafety = &T_Vehicle_GetSafety;
  vehicle_api->Veh_GetTargetSpeed = &T_Vehicle_GetTargetSpeed;
  vehicle_api->Veh_GetSpeedLimit = &T_Vehicle_GetSpeedLimit;
  vehicle_api->Veh_GetLaneSpeedInput = &T_Vehicle_GetLaneSpeedInput;
  vehicle_api->Veh_GetCurrentSpeed = &T_Vehicle_GetCurrentSpeed;
  vehicle_api->Veh_GetAcceleration = &T_Vehicle_GetAcceleration;
}

bool VehicleApi::T_Vehicle_IsReady() { return GameObjectVehicleService::GetInstance().AreAllFindersReady(); }

SPF_VehicleHandle VehicleApi::T_Vehicle_GetPlayerVehicle() { return reinterpret_cast<SPF_VehicleHandle>(GameObjectVehicleService::GetInstance().GetPlayerVehiclePtr()); }

SPF_VehicleHandle VehicleApi::T_Vehicle_GetVehicleById(int32_t id) {
  auto vehicles = GameObjectVehicleService::GetInstance().GetAllVehiclesFullInfo();
  for (const auto& v : vehicles) {
    if (v.id == id) return reinterpret_cast<SPF_VehicleHandle>(v.pointer);
  }
  return nullptr;
}

uint32_t VehicleApi::T_Vehicle_GetCount() { return static_cast<uint32_t>(GameObjectVehicleService::GetInstance().GetAllVehiclesFullInfo().size()); }

uint32_t VehicleApi::T_Vehicle_GetAllHandles(SPF_VehicleHandle* out_handles, uint32_t max_count) {
  if (!out_handles || max_count == 0) return 0;
  auto vehicles = GameObjectVehicleService::GetInstance().GetAllVehiclesFullInfo();
  uint32_t count = 0;
  for (const auto& v : vehicles) {
    if (count >= max_count) break;
    out_handles[count++] = reinterpret_cast<SPF_VehicleHandle>(v.pointer);
  }
  return count;
}

uintptr_t VehicleApi::T_Vehicle_GetTrafficManagerPtr() { return GameObjectVehicleService::GetInstance().GetTrafficManagerAddr(); }

uintptr_t VehicleApi::T_Vehicle_GetLocalPlayerControllerPtr() { return GameObjectVehicleService::GetInstance().GetLocalPlayerControllerAddr(); }

bool VehicleApi::T_Vehicle_AreAllOffsetsFound() { return GameObjectVehicleService::GetInstance().AreAllFindersReady(); }

bool VehicleApi::T_Vehicle_IsFinderReady(const char* finderName) { return GameObjectVehicleService::GetInstance().IsFinderReady(finderName); }

bool VehicleApi::T_Vehicle_RefreshOffsets() { return GameObjectVehicleService::GetInstance().TryFindAllOffsets(); }

int32_t VehicleApi::T_Vehicle_GetId(SPF_VehicleHandle h) {
  if (!h) return -1;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetVehicleIdOffset();
  if (IsBadReadPtr((void*)addr, sizeof(int32_t))) return -1;
  return *reinterpret_cast<int32_t*>(addr);
}

uintptr_t VehicleApi::T_Vehicle_GetRawAddress(SPF_VehicleHandle h) { return reinterpret_cast<uintptr_t>(h); }

float VehicleApi::T_Vehicle_GetPatience(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetPatienceOffset();
  if (IsBadReadPtr((void*)addr, sizeof(float))) return 0.0f;
  return *reinterpret_cast<float*>(addr);
}

float VehicleApi::T_Vehicle_GetSafety(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetSafetyOffset();
  if (IsBadReadPtr((void*)addr, sizeof(float))) return 0.0f;
  return *reinterpret_cast<float*>(addr);
}

float VehicleApi::T_Vehicle_GetTargetSpeed(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetTargetSpeedOffset();
  if (IsBadReadPtr((void*)addr, sizeof(float))) return 0.0f;
  return *reinterpret_cast<float*>(addr);
}

float VehicleApi::T_Vehicle_GetSpeedLimit(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetSpeedLimitOffset();
  if (IsBadReadPtr((void*)addr, sizeof(float))) return 0.0f;
  return *reinterpret_cast<float*>(addr);
}

float VehicleApi::T_Vehicle_GetLaneSpeedInput(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t addr = reinterpret_cast<uintptr_t>(h) + svc.GetLaneSpeedInputOffset();
  if (IsBadReadPtr((void*)addr, sizeof(float))) return 0.0f;
  return *reinterpret_cast<float*>(addr);
}

float VehicleApi::T_Vehicle_GetCurrentSpeed(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t actor = reinterpret_cast<uintptr_t>(h);

  // The sub-object (physics component) is inlined at the offset.
  uintptr_t subObj = actor + svc.GetVehicleSubObjectOffset();
  if (IsBadReadPtr((void*)subObj, sizeof(uintptr_t))) return 0.0f;

  // Safe reading of VTable (vptr is at the start of the sub-object)
  uintptr_t vtable = *reinterpret_cast<uintptr_t*>(subObj);
  if (IsBadReadPtr((void*)vtable, sizeof(uintptr_t))) return 0.0f;

  uintptr_t fnAddr = vtable + svc.GetVtableGetCurrentSpeedOffset();
  if (IsBadReadPtr((void*)fnAddr, sizeof(uintptr_t))) return 0.0f;

  using GetSpeedFn = float (*)(uintptr_t);
  auto fn = reinterpret_cast<GetSpeedFn>(*reinterpret_cast<uintptr_t*>(fnAddr));
  return fn ? fn(subObj) : 0.0f;
}

float VehicleApi::T_Vehicle_GetAcceleration(SPF_VehicleHandle h) {
  if (!h) return 0.0f;
  auto& svc = GameObjectVehicleService::GetInstance();
  uintptr_t actor = reinterpret_cast<uintptr_t>(h);

  // The sub-object (physics component) is inlined at the offset.
  uintptr_t subObj = actor + svc.GetVehicleSubObjectOffset();
  if (IsBadReadPtr((void*)subObj, sizeof(uintptr_t))) return 0.0f;

  // Safe reading of VTable (vptr is at the start of the sub-object)
  uintptr_t vtable = *reinterpret_cast<uintptr_t*>(subObj);
  if (IsBadReadPtr((void*)vtable, sizeof(uintptr_t))) return 0.0f;

  uintptr_t fnAddr = vtable + svc.GetVtableGetAccelerationOffset();
  if (IsBadReadPtr((void*)fnAddr, sizeof(uintptr_t))) return 0.0f;

  using GetAccelFn = float (*)(uintptr_t);
  auto fn = reinterpret_cast<GetAccelFn>(*reinterpret_cast<uintptr_t*>(fnAddr));
  return fn ? fn(subObj) : 0.0f;
}

}  // namespace Modules::API
SPF_NS_END
