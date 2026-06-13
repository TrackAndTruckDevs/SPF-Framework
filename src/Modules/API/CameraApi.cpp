#include "SPF/Modules/API/CameraApi.hpp"
#include "SPF/SPF_API/SPF_Camera_API.h"

// Includes from PluginManager.cpp
#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/GameCamera/GameCameraDebug.hpp"
#include "SPF/GameCamera/GameCameraInterior.hpp"
#include "SPF/GameCamera/GameCameraBehind.hpp"
#include "SPF/GameCamera/GameCameraTop.hpp"
#include "SPF/GameCamera/GameCameraCabin.hpp"
#include "SPF/GameCamera/GameCameraWindow.hpp"
#include "SPF/GameCamera/GameCameraBumper.hpp"
#include "SPF/GameCamera/GameCameraWheel.hpp"
#include "SPF/GameCamera/GameCameraTV.hpp"
#include "SPF/GameCamera/GameCameraFree.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/Vec3.hpp"

SPF_NS_BEGIN
namespace Modules::API {
using namespace SPF::GameCamera;
using namespace SPF::Data::GameData;
using namespace SPF::Utils;

// --- Core Camera Trampolines ---
void CameraApi::T_Camera_SwitchTo(SPF_CameraType cameraType) { GameCameraManager::GetInstance().SwitchTo(static_cast<GameCamera::GameCameraType>(cameraType)); }

void* CameraApi::T_Camera_GetCameraObject(void* manager, int index) {
  auto func = SPF::Hooks::CameraHooks::GetInstance().GetGetCameraObjectFunc();
  if (func) {
    return func(manager, index);
  }
  return nullptr;
}

bool CameraApi::T_Camera_GetCurrentCamera(SPF_CameraType* out_cameraType) {
  if (!out_cameraType) return false;
  auto type = GameCameraManager::GetInstance().GetCurrentCameraType();
  if (static_cast<int>(type) == -1) {
    return false;
  }
  *out_cameraType = static_cast<SPF_CameraType>(type);
  return true;
}

void CameraApi::T_Camera_ResetToDefaults(SPF_CameraType cameraType) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(static_cast<GameCamera::GameCameraType>(cameraType));
  if (pCamera) {
    pCamera->ResetToDefaults();
  }
}

// --- Interior Camera Trampolines ---
bool CameraApi::T_Camera_GetInteriorSeatPos(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interiorCam->GetSeatPosition(x, y, z);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorSeatPos(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interiorCam->SetSeatPosition(x, y, z);
  }
}

bool CameraApi::T_Camera_GetInteriorHeadRot(float* yaw, float* pitch) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interiorCam->GetHeadRotation(yaw, pitch);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorHeadRot(float yaw, float pitch) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interiorCam->SetHeadRotation(yaw, pitch);
  }
}

bool CameraApi::T_Camera_GetInteriorFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interiorCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interiorCam->SetFov(fov);
  }
}

bool CameraApi::T_Camera_GetInteriorRotationLimits(float* left, float* right, float* up, float* down) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interiorCam->GetRotationLimits(left, right, up, down);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorRotationLimits(float left, float right, float up, float down) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interiorCam->SetRotationLimits(left, right, up, down);
  }
}

bool CameraApi::T_Camera_GetInteriorRotationDefaults(float* lr, float* ud) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interiorCam->GetRotationDefaults(lr, ud);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorRotationDefaults(float lr, float ud) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interiorCam->SetRotationDefaults(lr, ud);
  }
}
// --- Behind Camera Trampolines ---
bool CameraApi::T_Camera_GetBehindLiveState(float* pitch, float* yaw, float* zoom) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetLiveState(pitch, yaw, zoom);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindLiveState(float pitch, float yaw, float zoom) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetLiveState(pitch, yaw, zoom);
  }
}

bool CameraApi::T_Camera_GetBehindDistanceSettings(float* min, float* max, float* trailer_max_offset, float* def, float* trailer_def, float* change_speed, float* laziness) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetDistanceSettings(min, max, trailer_max_offset, def, trailer_def, change_speed, laziness);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindDistanceSettings(float min, float max, float trailer_max_offset, float def, float trailer_def, float change_speed, float laziness) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetDistanceSettings(min, max, trailer_max_offset, def, trailer_def, change_speed, laziness);
  }
}

bool CameraApi::T_Camera_GetBehindElevationSettings(float* azimuth_laziness, float* min, float* max, float* def, float* trailer_def, float* height_limit) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetElevationSettings(azimuth_laziness, min, max, def, trailer_def, height_limit);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindElevationSettings(float azimuth_laziness, float min, float max, float def, float trailer_def, float height_limit) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetElevationSettings(azimuth_laziness, min, max, def, trailer_def, height_limit);
  }
}

bool CameraApi::T_Camera_GetBehindPivot(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetPivot(x, y, z);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindPivot(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetPivot(x, y, z);
  }
}

bool CameraApi::T_Camera_GetBehindDynamicOffset(float* max, float* speed_min, float* speed_max, float* laziness) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetDynamicOffset(max, speed_min, speed_max, laziness);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindDynamicOffset(float max, float speed_min, float speed_max, float laziness) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetDynamicOffset(max, speed_min, speed_max, laziness);
  }
}

bool CameraApi::T_Camera_GetBehindFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return behindCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetBehindFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    behindCam->SetFov(fov);
  }
}

// --- Top Camera Trampolines ---
bool CameraApi::T_Camera_GetTopHeight(float* min_height, float* max_height) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    return topCam->GetHeight(min_height, max_height);
  }
  return false;
}

bool CameraApi::T_Camera_GetTopSpeed(float* speed) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    return topCam->GetSpeed(speed);
  }
  return false;
}

bool CameraApi::T_Camera_GetTopOffsets(float* forward, float* backward) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    return topCam->GetOffsets(forward, backward);
  }
  return false;
}

void CameraApi::T_Camera_SetTopHeight(float min_height, float max_height) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    topCam->SetHeight(min_height, max_height);
  }
}

void CameraApi::T_Camera_SetTopSpeed(float speed) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    topCam->SetSpeed(speed);
  }
}

void CameraApi::T_Camera_SetTopOffsets(float forward, float backward) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    topCam->SetOffsets(forward, backward);
  }
}

bool CameraApi::T_Camera_GetTopFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    return topCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetTopFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
    topCam->SetFov(fov);
  }
}

// --- Window Camera Trampolines ---
bool CameraApi::T_Camera_GetWindowHeadOffset(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return windowCam->GetHeadOffset(x, y, z);
  }
  return false;
}

bool CameraApi::T_Camera_GetWindowLiveRotation(float* yaw, float* pitch) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return windowCam->GetLiveRotation(yaw, pitch);
  }
  return false;
}

bool CameraApi::T_Camera_GetWindowRotationLimits(float* left, float* right, float* up, float* down) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return windowCam->GetRotationLimits(left, right, up, down);
  }
  return false;
}

bool CameraApi::T_Camera_GetWindowRotationDefaults(float* lr, float* ud) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return windowCam->GetRotationDefaults(lr, ud);
  }
  return false;
}

void CameraApi::T_Camera_SetWindowHeadOffset(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    windowCam->SetHeadOffset(x, y, z);
  }
}

void CameraApi::T_Camera_SetWindowLiveRotation(float yaw, float pitch) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    windowCam->SetLiveRotation(yaw, pitch);
  }
}

void CameraApi::T_Camera_SetWindowRotationLimits(float left, float right, float up, float down) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    windowCam->SetRotationLimits(left, right, up, down);
  }
}

void CameraApi::T_Camera_SetWindowRotationDefaults(float lr, float ud) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    windowCam->SetRotationDefaults(lr, ud);
  }
}

bool CameraApi::T_Camera_GetWindowFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return windowCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetWindowFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    windowCam->SetFov(fov);
  }
}

// --- Bumper Camera Trampolines ---
bool CameraApi::T_Camera_GetBumperOffset(float* offset_x, float* offset_y, float* offset_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BumperCamera);
  if (auto* bumperCam = dynamic_cast<GameCameraBumper*>(pCamera)) {
    return bumperCam->GetOffset(offset_x, offset_y, offset_z);
  }
  return false;
}

void CameraApi::T_Camera_SetBumperOffset(float offset_x, float offset_y, float offset_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BumperCamera);
  if (auto* bumperCam = dynamic_cast<GameCameraBumper*>(pCamera)) {
    bumperCam->SetOffset(offset_x, offset_y, offset_z);
  }
}

bool CameraApi::T_Camera_GetBumperFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BumperCamera);
  if (auto* bumperCam = dynamic_cast<GameCameraBumper*>(pCamera)) {
    return bumperCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetBumperFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BumperCamera);
  if (auto* bumperCam = dynamic_cast<GameCameraBumper*>(pCamera)) {
    bumperCam->SetFov(fov);
  }
}

// --- Wheel Camera Trampolines ---
bool CameraApi::T_Camera_GetWheelOffset(float* offset_x, float* offset_y, float* offset_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WheelCamera);
  if (auto* wheelCam = dynamic_cast<GameCameraWheel*>(pCamera)) {
    return wheelCam->GetOffset(offset_x, offset_y, offset_z);
  }
  return false;
}

void CameraApi::T_Camera_SetWheelOffset(float offset_x, float offset_y, float offset_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WheelCamera);
  if (auto* wheelCam = dynamic_cast<GameCameraWheel*>(pCamera)) {
    wheelCam->SetOffset(offset_x, offset_y, offset_z);
  }
}

bool CameraApi::T_Camera_GetWheelFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WheelCamera);
  if (auto* wheelCam = dynamic_cast<GameCameraWheel*>(pCamera)) {
    return wheelCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetWheelFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WheelCamera);
  if (auto* wheelCam = dynamic_cast<GameCameraWheel*>(pCamera)) {
    wheelCam->SetFov(fov);
  }
}

// --- Cabin Camera Trampolines ---
bool CameraApi::T_Camera_GetCabinFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::CabinCamera);
  if (auto* cabinCam = dynamic_cast<GameCameraCabin*>(pCamera)) {
    return cabinCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetCabinFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::CabinCamera);
  if (auto* cabinCam = dynamic_cast<GameCameraCabin*>(pCamera)) {
    cabinCam->SetFov(fov);
  }
}

// --- TV Camera Trampolines ---
bool CameraApi::T_Camera_GetTVMaxDistance(float* max_distance) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    return tvCam->GetMaxDistance(max_distance);
  }
  return false;
}

bool CameraApi::T_Camera_GetTVPrefabUplift(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    return tvCam->GetPrefabUplift(x, y, z);
  }
  return false;
}

bool CameraApi::T_Camera_GetTVRoadUplift(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    return tvCam->GetRoadUplift(x, y, z);
  }
  return false;
}

void CameraApi::T_Camera_SetTVMaxDistance(float max_distance) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    tvCam->SetMaxDistance(max_distance);
  }
}

void CameraApi::T_Camera_SetTVPrefabUplift(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    tvCam->SetPrefabUplift(x, y, z);
  }
}

void CameraApi::T_Camera_SetTVRoadUplift(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    tvCam->SetRoadUplift(x, y, z);
  }
}

bool CameraApi::T_Camera_GetTVFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    return tvCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetTVFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
    tvCam->SetFov(fov);
  }
}

// --- World Coordinates Trampolines ---
bool CameraApi::T_Camera_GetWorldCoordinates(float* x, float* y, float* z) {
  if (!x || !y || !z) return false;
  auto& gameData = GameDataCameraService::GetInstance();
  Vector3* pCameraWorldCoords = reinterpret_cast<Vector3*>(gameData.GetCameraWorldCoordinatesPtr());
  if (pCameraWorldCoords) {
    *x = pCameraWorldCoords->x;
    *y = pCameraWorldCoords->y;
    *z = pCameraWorldCoords->z;
    return true;
  }
  return false;
}

// --- Free Camera Trampolines ---
bool CameraApi::T_Camera_GetFreePosition(float* x, float* y, float* z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return freeCam->GetPosition(x, y, z);
  }
  return false;
}

void CameraApi::T_Camera_SetFreePosition(float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    freeCam->SetPosition(x, y, z);
  }
}

bool CameraApi::T_Camera_GetFreeQuaternion(float* x, float* y, float* z, float* w) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return freeCam->GetQuaternion(x, y, z, w);
  }
  return false;
}

bool CameraApi::T_Camera_GetFreeOrientation(float* mouse_x, float* mouse_y, float* roll) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return freeCam->GetOrientation(mouse_x, mouse_y, roll);
  }
  return false;
}

void CameraApi::T_Camera_SetFreeOrientation(float mouse_x, float mouse_y, float roll) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    freeCam->SetOrientation(mouse_x, mouse_y, roll);
  }
}

bool CameraApi::T_Camera_GetFreeFov(float* fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return freeCam->GetFov(fov);
  }
  return false;
}

void CameraApi::T_Camera_SetFreeFov(float fov) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    freeCam->SetFov(fov);
  }
}

bool CameraApi::T_Camera_GetFreeSpeed(float* speed) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return freeCam->GetSpeed(speed);
  }
  return false;
}

void CameraApi::T_Camera_SetFreeSpeed(float speed) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
    freeCam->SetSpeed(speed);
  }
}

// --- Debug Camera Trampolines ---
void CameraApi::T_Camera_EnableDebugCamera(bool enable) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetEnabled(enable);
  }
}

bool CameraApi::T_Camera_GetDebugCameraEnabled(bool* out_isEnabled) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetEnabled(out_isEnabled);
  }
  return false;
}

void CameraApi::T_Camera_SetDebugCameraMode(SPF_DebugCameraMode mode) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetMode(static_cast<GameCamera::DebugCameraMode>(mode));
  }
}

bool CameraApi::T_Camera_GetDebugCameraMode(SPF_DebugCameraMode* out_mode) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetCurrentMode(reinterpret_cast<GameCamera::DebugCameraMode*>(out_mode));
  }
  return false;
}

// --- Debug Camera HUD & UI Trampolines ---

void CameraApi::T_Camera_SetDebugHudVisible(bool visible) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetHudVisible(visible);
  }
}

bool CameraApi::T_Camera_GetDebugHudVisible(bool* out_isVisible) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetHudVisible(out_isVisible);
  }
  return false;
}

void CameraApi::T_Camera_SetDebugHudPosition(SPF_DebugHudPosition position) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetHudPosition(static_cast<GameCamera::DebugHudPosition>(position));
  }
}

bool CameraApi::T_Camera_GetDebugHudPosition(SPF_DebugHudPosition* out_position) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetHudPosition(reinterpret_cast<GameCamera::DebugHudPosition*>(out_position));
  }
  return false;
}

void CameraApi::T_Camera_SetDebugGameUiVisible(bool visible) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetGameUiVisible(visible);
  }
}

bool CameraApi::T_Camera_GetDebugGameUiVisible(bool* out_isVisible) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetGameUiVisible(out_isVisible);
  }
  return false;
}

// --- New Debug Camera Control Trampolines ---

void CameraApi::T_Camera_SetDebugPosLock(bool locked) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetPosLock(locked);
  }
}

bool CameraApi::T_Camera_GetDebugPosLock(bool* out_locked) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetPosLock(out_locked);
  }
  return false;
}

void CameraApi::T_Camera_SetDebugRotLock(bool locked) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetRotLock(locked);
  }
}

bool CameraApi::T_Camera_GetDebugRotLock(bool* out_locked) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetRotLock(out_locked);
  }
  return false;
}

void CameraApi::T_Camera_SetDebugOrbitMode(bool enabled) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetOrbitMode(enabled);
  }
}

bool CameraApi::T_Camera_GetDebugOrbitMode(bool* out_enabled) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetOrbitMode(out_enabled);
  }
  return false;
}

void CameraApi::T_Camera_SetDebugOrbitSpeed(float speed) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetOrbitSpeed(speed);
  }
}

bool CameraApi::T_Camera_GetDebugOrbitSpeed(float* out_speed) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return debugCam->GetOrbitSpeed(out_speed);
  }
  return false;
}

void* CameraApi::T_Camera_GetDebugSelectedObject() {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return reinterpret_cast<void*>(debugCam->GetSelectedObjectPtr());
  }
  return nullptr;
}

void CameraApi::T_Camera_SetDebugSelectedObject(void* ptr) {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    debugCam->SetSelectedObjectPtr(reinterpret_cast<uintptr_t>(ptr));
  }
}

void* CameraApi::T_Camera_GetDebugHoveredObject() {
  if (auto* debugCam = GameCameraManager::GetInstance().GetDebugCamera()) {
    return reinterpret_cast<void*>(debugCam->GetHoveredObjectPtr());
  }
  return nullptr;
}

// --- Debug Camera State Trampolines ---
int CameraApi::T_Camera_GetStateCount() {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    return stateCam->GetStateCount();
  }
  return 0;
}

int CameraApi::T_Camera_GetCurrentStateIndex() {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    return stateCam->GetCurrentStateIndex();
  }
  return -1;
}

bool CameraApi::T_Camera_GetState(int index, SPF_CameraState_t* out_state) {
  if (!out_state) return false;

  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    // We need to reinterpret_cast because the C-API struct and C++ struct are defined in different files,
    // even though they are identical in layout.
    return stateCam->GetState(index, reinterpret_cast<GameCameraDebugState::CameraState&>(*out_state));
  }
  return false;
}

void CameraApi::T_Camera_ApplyState(int index) {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->ApplyState(index);
  }
}

void CameraApi::T_Camera_CycleState(int direction) {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->CycleState(direction);
  }
}

void CameraApi::T_Camera_SaveCurrentState() {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->SaveState();
  }
}

void CameraApi::T_Camera_ReloadStatesFromFile() {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->ReloadStatesFromFile();
  }
}

// --- In-Memory State Trampolines ---
void CameraApi::T_Camera_ClearAllStatesInMemory() {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->ClearAllStatesInMemory();
  }
}

void CameraApi::T_Camera_AddStateInMemory(const SPF_CameraState_t* state) {
  if (!state) return;
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->AddStateToMemory(reinterpret_cast<const GameCameraDebugState::CameraState&>(*state));
  }
}

bool CameraApi::T_Camera_EditStateInMemory(int index, const SPF_CameraState_t* newState) {
  if (!newState) return false;
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    return stateCam->EditStateInMemory(index, reinterpret_cast<const GameCameraDebugState::CameraState&>(*newState));
  }
  return false;
}

void CameraApi::T_Camera_DeleteStateInMemory(int index) {
  if (auto* stateCam = GameCameraManager::GetInstance().GetDebugStateCamera()) {
    stateCam->DeleteStateInMemory(index);
  }
}

// --- Animation Control Trampolines ---
void CameraApi::T_Anim_Play(int startIndex) {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->Play(startIndex);
  }
}

void CameraApi::T_Anim_Pause() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->Pause();
  }
}

void CameraApi::T_Anim_Stop() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->Stop();
  }
}

void CameraApi::T_Anim_GoToFrame(int frameIndex) {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->GoToFrame(frameIndex);
  }
}

void CameraApi::T_Anim_ScrubTo(float position) {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->ScrubTo(position);
  }
}

void CameraApi::T_Anim_SetReverse(bool isReversed) {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    animController->SetReverse(isReversed);
  }
}

SPF_AnimPlaybackState CameraApi::T_Anim_GetPlaybackState() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    return static_cast<SPF_AnimPlaybackState>(animController->GetPlaybackState());
  }
  return SPF_ANIM_STOPPED;
}

int CameraApi::T_Anim_GetCurrentFrame() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    return animController->GetCurrentFrame();
  }
  return 0;
}

float CameraApi::T_Anim_GetCurrentFrameProgress() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    return animController->GetCurrentFrameProgress();
  }
  return 0.0f;
}

bool CameraApi::T_Anim_IsReversed() {
  if (auto* animController = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    return animController->IsReversed();
  }
  return false;
}

// --- Framework & Service Status Trampolines ---
bool CameraApi::T_Camera_IsServiceReady() { return GameCameraManager::GetInstance().IsInstalled(); }
bool CameraApi::T_Camera_AreAllOffsetsFound() { return GameDataCameraService::GetInstance().AreAllFindersReady(); }
bool CameraApi::T_Camera_IsFinderReady(const char* finderName) { return GameDataCameraService::GetInstance().IsFinderReady(finderName); }
bool CameraApi::T_Camera_RefreshOffsets() { return GameDataCameraService::GetInstance().TryFindAllOffsets(); }

// --- Viewport & Projection Trampolines ---
bool CameraApi::T_Camera_GetViewport(float* x1, float* x2, float* y1, float* y2) {
  if (!x1 || !x2 || !y1 || !y2) return false;
  auto& data = GameDataCameraService::GetInstance();
  uintptr_t pParams = data.GetCameraParamsObjectPtr();
  if (!pParams) return false;

  *x1 = *reinterpret_cast<float*>(pParams + data.GetViewportX1Offset());
  *x2 = *reinterpret_cast<float*>(pParams + data.GetViewportX2Offset());
  *y1 = *reinterpret_cast<float*>(pParams + data.GetViewportY1Offset());
  *y2 = *reinterpret_cast<float*>(pParams + data.GetViewportY2Offset());
  return true;
}

uintptr_t CameraApi::T_Camera_GetCameraParamsObjectPtr() { return GameDataCameraService::GetInstance().GetCameraParamsObjectPtr(); }

// --- Animation Preparation Trampolines ---
bool CameraApi::T_Camera_Anim_Prepare() {
  if (auto* anim = GameCameraManager::GetInstance().GetDebugAnimationController()) {
    return anim->PrepareForPlayback();
  }
  return false;
}

// --- Object Targeting & Inspection Trampolines ---
uintptr_t CameraApi::T_Camera_GetDebugObjectAddress(void* ptr) {
  return reinterpret_cast<uintptr_t>(ptr);
}

// --- New Interior Advanced Settings Trampolines ---

bool CameraApi::T_Camera_GetInteriorOutside(bool* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetOutside(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorOutside(bool val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetOutside(val);
  }
}

bool CameraApi::T_Camera_GetInteriorNearPlane(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetNearPlane(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorNearPlane(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetNearPlane(val);
  }
}

bool CameraApi::T_Camera_GetInteriorFarPlane(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetFarPlane(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorFarPlane(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetFarPlane(val);
  }
}

bool CameraApi::T_Camera_GetInteriorMouseSensitivity(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetMouseSensitivity(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorMouseSensitivity(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetMouseSensitivity(val);
  }
}

bool CameraApi::T_Camera_GetInteriorShakeAnimStep(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetShakeAnimStep(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorShakeAnimStep(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetShakeAnimStep(val);
  }
}

bool CameraApi::T_Camera_GetInteriorShakeAnimScaleMin(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetShakeAnimScaleMin(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorShakeAnimScaleMin(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetShakeAnimScaleMin(val);
  }
}

bool CameraApi::T_Camera_GetInteriorShakeAnimScaleMax(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetShakeAnimScaleMax(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorShakeAnimScaleMax(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetShakeAnimScaleMax(val);
  }
}

bool CameraApi::T_Camera_GetInteriorHandShakeLimit(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetHandShakeLimit(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorHandShakeLimit(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetHandShakeLimit(val);
  }
}

bool CameraApi::T_Camera_GetInteriorHandShakeSpeed(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetHandShakeSpeed(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorHandShakeSpeed(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetHandShakeSpeed(val);
  }
}

bool CameraApi::T_Camera_GetInteriorZoomFovFactor(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetZoomFovFactor(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorZoomFovFactor(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetZoomFovFactor(val);
  }
}

bool CameraApi::T_Camera_GetInteriorZoomSpeed(float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetZoomSpeed(out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorZoomSpeed(float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetZoomSpeed(val);
  }
}

// --- Azimuth Overrides Trampolines ---

size_t CameraApi::T_Camera_GetInteriorAzimuthOverridesCount() {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverridesCount();
  }
  return 0;
}

void* CameraApi::T_Camera_GetInteriorAzimuthOverrideAddress(size_t index) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideAddress(index);
  }
  return nullptr;
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideOutside(size_t index, bool* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideOutside(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideOutside(size_t index, bool val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideOutside(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartAzimuth(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartAzimuth(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartAzimuth(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartAzimuth(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndAzimuth(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndAzimuth(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndAzimuth(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndAzimuth(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartUpLimit(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartUpLimit(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartUpLimit(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartUpLimit(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndUpLimit(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndUpLimit(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndUpLimit(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndUpLimit(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartDownLimit(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartDownLimit(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartDownLimit(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartDownLimit(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndDownLimit(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndDownLimit(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndDownLimit(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndDownLimit(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartUpDownDefault(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartUpDownDefault(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartUpDownDefault(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartUpDownDefault(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndUpDownDefault(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndUpDownDefault(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndUpDownDefault(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndUpDownDefault(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartLeftRightDefault(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartLeftRightDefault(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartLeftRightDefault(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartLeftRightDefault(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndLeftRightDefault(size_t index, float* out_val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndLeftRightDefault(index, out_val);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndLeftRightDefault(size_t index, float val) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndLeftRightDefault(index, val);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideStartHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideStartHeadOffset(index, out_x, out_y, out_z);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideStartHeadOffset(size_t index, float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideStartHeadOffset(index, x, y, z);
  }
}

bool CameraApi::T_Camera_GetInteriorAzimuthOverrideEndHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetAzimuthOverrideEndHeadOffset(index, out_x, out_y, out_z);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorAzimuthOverrideEndHeadOffset(size_t index, float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetAzimuthOverrideEndHeadOffset(index, x, y, z);
  }
}

// --- Shake Animation Trampolines ---

size_t CameraApi::T_Camera_GetInteriorShakeAnimCount() {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetShakeAnimCount();
  }
  return 0;
}

bool CameraApi::T_Camera_GetInteriorShakeAnim(size_t index, float* out_x, float* out_y, float* out_z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return interior->GetShakeAnim(index, out_x, out_y, out_z);
  }
  return false;
}

void CameraApi::T_Camera_SetInteriorShakeAnim(size_t index, float x, float y, float z) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* interior = dynamic_cast<GameCameraInterior*>(pCamera)) {
    interior->SetShakeAnim(index, x, y, z);
  }
}


// --- FinalFOV Trampolines ---
bool CameraApi::T_Camera_GetInteriorFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::InteriorCamera);
  if (auto* cam = dynamic_cast<GameCameraInterior*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetBehindFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BehindCamera);
  if (auto* cam = dynamic_cast<GameCameraBehind*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetTopFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TopCamera);
  if (auto* cam = dynamic_cast<GameCameraTop*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetWindowFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WindowCamera);
  if (auto* cam = dynamic_cast<GameCameraWindow*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetBumperFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::BumperCamera);
  if (auto* cam = dynamic_cast<GameCameraBumper*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetWheelFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::WheelCamera);
  if (auto* cam = dynamic_cast<GameCameraWheel*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetCabinFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::CabinCamera);
  if (auto* cam = dynamic_cast<GameCameraCabin*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetTVFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::TVCamera);
  if (auto* cam = dynamic_cast<GameCameraTV*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

bool CameraApi::T_Camera_GetFreeFinalFov(float* out_horiz, float* out_vert) {
  auto* pCamera = GameCameraManager::GetInstance().GetCamera(GameCamera::GameCameraType::DeveloperFreeCamera);
  if (auto* cam = dynamic_cast<GameCameraFree*>(pCamera)) {
    return cam->GetFinalFov(out_horiz, out_vert);
  }
  return false;
}

// --- API Filling Function ---
void CameraApi::FillCameraAPI(SPF_Camera_API* camera_api) {
  if (!camera_api) return;

  camera_api->Cam_SwitchTo = &CameraApi::T_Camera_SwitchTo;
  camera_api->Cam_GetCameraObject = &CameraApi::T_Camera_GetCameraObject;
  camera_api->Cam_GetCurrentCamera = &CameraApi::T_Camera_GetCurrentCamera;
  camera_api->Cam_ResetToDefaults = &CameraApi::T_Camera_ResetToDefaults;

  // Interior
  camera_api->Cam_GetInteriorSeatPos = &T_Camera_GetInteriorSeatPos;
  camera_api->Cam_SetInteriorSeatPos = &T_Camera_SetInteriorSeatPos;
  camera_api->Cam_GetInteriorHeadRot = &T_Camera_GetInteriorHeadRot;
  camera_api->Cam_SetInteriorHeadRot = &T_Camera_SetInteriorHeadRot;
  camera_api->Cam_GetInteriorFov = &T_Camera_GetInteriorFov;
  camera_api->Cam_GetInteriorFinalFov = &T_Camera_GetInteriorFinalFov;
  camera_api->Cam_SetInteriorFov = &T_Camera_SetInteriorFov;
  camera_api->Cam_GetInteriorRotationLimits = &T_Camera_GetInteriorRotationLimits;
  camera_api->Cam_SetInteriorRotationLimits = &T_Camera_SetInteriorRotationLimits;
  camera_api->Cam_GetInteriorRotationDefaults = &T_Camera_GetInteriorRotationDefaults;
  camera_api->Cam_SetInteriorRotationDefaults = &T_Camera_SetInteriorRotationDefaults;

  // Behind
  camera_api->Cam_GetBehindLiveState = &T_Camera_GetBehindLiveState;
  camera_api->Cam_SetBehindLiveState = &T_Camera_SetBehindLiveState;
  camera_api->Cam_GetBehindDistanceSettings = &T_Camera_GetBehindDistanceSettings;
  camera_api->Cam_SetBehindDistanceSettings = &T_Camera_SetBehindDistanceSettings;
  camera_api->Cam_GetBehindElevationSettings = &T_Camera_GetBehindElevationSettings;
  camera_api->Cam_SetBehindElevationSettings = &T_Camera_SetBehindElevationSettings;
  camera_api->Cam_GetBehindPivot = &T_Camera_GetBehindPivot;
  camera_api->Cam_SetBehindPivot = &T_Camera_SetBehindPivot;
  camera_api->Cam_GetBehindDynamicOffset = &T_Camera_GetBehindDynamicOffset;
  camera_api->Cam_SetBehindDynamicOffset = &T_Camera_SetBehindDynamicOffset;
  camera_api->Cam_GetBehindFov = &T_Camera_GetBehindFov;
  camera_api->Cam_GetBehindFinalFov = &T_Camera_GetBehindFinalFov;
  camera_api->Cam_SetBehindFov = &T_Camera_SetBehindFov;

  // Top
  camera_api->Cam_GetTopHeight = &T_Camera_GetTopHeight;
  camera_api->Cam_GetTopSpeed = &T_Camera_GetTopSpeed;
  camera_api->Cam_GetTopOffsets = &T_Camera_GetTopOffsets;
  camera_api->Cam_SetTopHeight = &T_Camera_SetTopHeight;
  camera_api->Cam_SetTopSpeed = &T_Camera_SetTopSpeed;
  camera_api->Cam_SetTopOffsets = &T_Camera_SetTopOffsets;
  camera_api->Cam_GetTopFov = &T_Camera_GetTopFov;
  camera_api->Cam_GetTopFinalFov = &T_Camera_GetTopFinalFov;
  camera_api->Cam_SetTopFov = &T_Camera_SetTopFov;

  // Window
  camera_api->Cam_GetWindowHeadOffset = &T_Camera_GetWindowHeadOffset;
  camera_api->Cam_GetWindowLiveRotation = &T_Camera_GetWindowLiveRotation;
  camera_api->Cam_GetWindowRotationLimits = &T_Camera_GetWindowRotationLimits;
  camera_api->Cam_GetWindowRotationDefaults = &T_Camera_GetWindowRotationDefaults;
  camera_api->Cam_SetWindowHeadOffset = &T_Camera_SetWindowHeadOffset;
  camera_api->Cam_SetWindowLiveRotation = &T_Camera_SetWindowLiveRotation;
  camera_api->Cam_SetWindowRotationLimits = &T_Camera_SetWindowRotationLimits;
  camera_api->Cam_SetWindowRotationDefaults = &T_Camera_SetWindowRotationDefaults;
  camera_api->Cam_GetWindowFov = &T_Camera_GetWindowFov;
  camera_api->Cam_GetWindowFinalFov = &T_Camera_GetWindowFinalFov;
  camera_api->Cam_SetWindowFov = &T_Camera_SetWindowFov;

  // Bumper
  camera_api->Cam_GetBumperOffset = &T_Camera_GetBumperOffset;
  camera_api->Cam_SetBumperOffset = &T_Camera_SetBumperOffset;
  camera_api->Cam_GetBumperFov = &T_Camera_GetBumperFov;
  camera_api->Cam_GetBumperFinalFov = &T_Camera_GetBumperFinalFov;
  camera_api->Cam_SetBumperFov = &T_Camera_SetBumperFov;

  // Wheel
  camera_api->Cam_GetWheelOffset = &T_Camera_GetWheelOffset;
  camera_api->Cam_SetWheelOffset = &T_Camera_SetWheelOffset;
  camera_api->Cam_GetWheelFov = &T_Camera_GetWheelFov;
  camera_api->Cam_GetWheelFinalFov = &T_Camera_GetWheelFinalFov;
  camera_api->Cam_SetWheelFov = &T_Camera_SetWheelFov;

  // Cabin
  camera_api->Cam_GetCabinFov = &T_Camera_GetCabinFov;
  camera_api->Cam_GetCabinFinalFov = &T_Camera_GetCabinFinalFov;
  camera_api->Cam_SetCabinFov = &T_Camera_SetCabinFov;

  // TV
  camera_api->Cam_GetTVMaxDistance = &T_Camera_GetTVMaxDistance;
  camera_api->Cam_GetTVPrefabUplift = &T_Camera_GetTVPrefabUplift;
  camera_api->Cam_GetTVRoadUplift = &T_Camera_GetTVRoadUplift;
  camera_api->Cam_SetTVMaxDistance = &T_Camera_SetTVMaxDistance;
  camera_api->Cam_SetTVPrefabUplift = &T_Camera_SetTVPrefabUplift;
  camera_api->Cam_SetTVRoadUplift = &T_Camera_SetTVRoadUplift;
  camera_api->Cam_GetTVFov = &T_Camera_GetTVFov;
  camera_api->Cam_GetTVFinalFov = &T_Camera_GetTVFinalFov;
  camera_api->Cam_SetTVFov = &T_Camera_SetTVFov;

  // World Coords
  camera_api->Cam_GetCameraWorldCoordinates = &T_Camera_GetWorldCoordinates;

  // Free Cam
  camera_api->Cam_GetFreePosition = &T_Camera_GetFreePosition;
  camera_api->Cam_SetFreePosition = &T_Camera_SetFreePosition;
  camera_api->Cam_GetFreeQuaternion = &T_Camera_GetFreeQuaternion;
  camera_api->Cam_GetFreeOrientation = &T_Camera_GetFreeOrientation;
  camera_api->Cam_SetFreeOrientation = &T_Camera_SetFreeOrientation;
  camera_api->Cam_GetFreeFov = &T_Camera_GetFreeFov;
  camera_api->Cam_GetFreeFinalFov = &T_Camera_GetFreeFinalFov;
  camera_api->Cam_SetFreeFov = &T_Camera_SetFreeFov;
  camera_api->Cam_GetFreeSpeed = &T_Camera_GetFreeSpeed;
  camera_api->Cam_SetFreeSpeed = &T_Camera_SetFreeSpeed;

  // Debug
  camera_api->Cam_EnableDebugCamera = &T_Camera_EnableDebugCamera;
  camera_api->Cam_GetDebugCameraEnabled = &T_Camera_GetDebugCameraEnabled;
  camera_api->Cam_SetDebugCameraMode = &T_Camera_SetDebugCameraMode;
  camera_api->Cam_GetDebugCameraMode = &T_Camera_GetDebugCameraMode;

  // Debug HUD & UI
  camera_api->Cam_SetDebugHudVisible = &T_Camera_SetDebugHudVisible;
  camera_api->Cam_GetDebugHudVisible = &T_Camera_GetDebugHudVisible;
  camera_api->Cam_SetDebugHudPosition = &T_Camera_SetDebugHudPosition;
  camera_api->Cam_GetDebugHudPosition = &T_Camera_GetDebugHudPosition;
  camera_api->Cam_SetDebugGameUiVisible = &T_Camera_SetDebugGameUiVisible;
  camera_api->Cam_GetDebugGameUiVisible = &T_Camera_GetDebugGameUiVisible;

  // New Debug Controls
  camera_api->Cam_SetDebugPosLock = &T_Camera_SetDebugPosLock;
  camera_api->Cam_GetDebugPosLock = &T_Camera_GetDebugPosLock;
  camera_api->Cam_SetDebugRotLock = &T_Camera_SetDebugRotLock;
  camera_api->Cam_GetDebugRotLock = &T_Camera_GetDebugRotLock;
  camera_api->Cam_SetDebugOrbitMode = &T_Camera_SetDebugOrbitMode;
  camera_api->Cam_GetDebugOrbitMode = &T_Camera_GetDebugOrbitMode;
  camera_api->Cam_SetDebugOrbitSpeed = &T_Camera_SetDebugOrbitSpeed;
  camera_api->Cam_GetDebugOrbitSpeed = &T_Camera_GetDebugOrbitSpeed;
  camera_api->Cam_GetDebugSelectedObject = &T_Camera_GetDebugSelectedObject;
  camera_api->Cam_SetDebugSelectedObject = &T_Camera_SetDebugSelectedObject;
  camera_api->Cam_GetDebugHoveredObject = &T_Camera_GetDebugHoveredObject;

  // Debug Camera State Management
  camera_api->Cam_GetStateCount = &T_Camera_GetStateCount;
  camera_api->Cam_GetCurrentStateIndex = &T_Camera_GetCurrentStateIndex;
  camera_api->Cam_GetState = &T_Camera_GetState;
  camera_api->Cam_ApplyState = &T_Camera_ApplyState;
  camera_api->Cam_CycleState = &T_Camera_CycleState;
  camera_api->Cam_SaveCurrentState = &T_Camera_SaveCurrentState;
  camera_api->Cam_ReloadStatesFromFile = &T_Camera_ReloadStatesFromFile;

  // In-Memory State Management
  camera_api->Cam_ClearAllStatesInMemory = &T_Camera_ClearAllStatesInMemory;
  camera_api->Cam_AddStateInMemory = &T_Camera_AddStateInMemory;
  camera_api->Cam_EditStateInMemory = &T_Camera_EditStateInMemory;
  camera_api->Cam_DeleteStateInMemory = &T_Camera_DeleteStateInMemory;

  // Animation Control
  camera_api->Cam_Anim_Play = &T_Anim_Play;
  camera_api->Cam_Anim_Pause = &T_Anim_Pause;
  camera_api->Cam_Anim_Stop = &T_Anim_Stop;
  camera_api->Cam_Anim_GoToFrame = &T_Anim_GoToFrame;
  camera_api->Cam_Anim_ScrubTo = &T_Anim_ScrubTo;
  camera_api->Cam_Anim_SetReverse = &T_Anim_SetReverse;
  camera_api->Cam_Anim_GetPlaybackState = &T_Anim_GetPlaybackState;
  camera_api->Cam_Anim_GetCurrentFrame = &T_Anim_GetCurrentFrame;
  camera_api->Cam_Anim_GetCurrentFrameProgress = &T_Anim_GetCurrentFrameProgress;
  camera_api->Cam_Anim_IsReversed = &T_Anim_IsReversed;

  // Framework & Status
  camera_api->Cam_IsServiceReady = &T_Camera_IsServiceReady;
  camera_api->Cam_AreAllOffsetsFound = &T_Camera_AreAllOffsetsFound;
  camera_api->Cam_IsFinderReady = &T_Camera_IsFinderReady;
  camera_api->Cam_RefreshOffsets = &T_Camera_RefreshOffsets;

  // Viewport
  camera_api->Cam_GetViewport = &T_Camera_GetViewport;
  camera_api->Cam_GetCameraParamsObjectPtr = &T_Camera_GetCameraParamsObjectPtr;

  // Animation Prep
  camera_api->Cam_Anim_Prepare = &T_Camera_Anim_Prepare;

  // Object Targeting
  camera_api->Cam_GetDebugObjectAddress = &T_Camera_GetDebugObjectAddress;

  // New Interior Advanced Settings
  camera_api->Cam_GetInteriorOutside = &T_Camera_GetInteriorOutside;
  camera_api->Cam_SetInteriorOutside = &T_Camera_SetInteriorOutside;
  camera_api->Cam_GetInteriorNearPlane = &T_Camera_GetInteriorNearPlane;
  camera_api->Cam_SetInteriorNearPlane = &T_Camera_SetInteriorNearPlane;
  camera_api->Cam_GetInteriorFarPlane = &T_Camera_GetInteriorFarPlane;
  camera_api->Cam_SetInteriorFarPlane = &T_Camera_SetInteriorFarPlane;
  camera_api->Cam_GetInteriorMouseSensitivity = &T_Camera_GetInteriorMouseSensitivity;
  camera_api->Cam_SetInteriorMouseSensitivity = &T_Camera_SetInteriorMouseSensitivity;
  camera_api->Cam_GetInteriorShakeAnimStep = &T_Camera_GetInteriorShakeAnimStep;
  camera_api->Cam_SetInteriorShakeAnimStep = &T_Camera_SetInteriorShakeAnimStep;
  camera_api->Cam_GetInteriorShakeAnimScaleMin = &T_Camera_GetInteriorShakeAnimScaleMin;
  camera_api->Cam_SetInteriorShakeAnimScaleMin = &T_Camera_SetInteriorShakeAnimScaleMin;
  camera_api->Cam_GetInteriorShakeAnimScaleMax = &T_Camera_GetInteriorShakeAnimScaleMax;
  camera_api->Cam_SetInteriorShakeAnimScaleMax = &T_Camera_SetInteriorShakeAnimScaleMax;
  camera_api->Cam_GetInteriorHandShakeLimit = &T_Camera_GetInteriorHandShakeLimit;
  camera_api->Cam_SetInteriorHandShakeLimit = &T_Camera_SetInteriorHandShakeLimit;
  camera_api->Cam_GetInteriorHandShakeSpeed = &T_Camera_GetInteriorHandShakeSpeed;
  camera_api->Cam_SetInteriorHandShakeSpeed = &T_Camera_SetInteriorHandShakeSpeed;
  camera_api->Cam_GetInteriorZoomFovFactor = &T_Camera_GetInteriorZoomFovFactor;
  camera_api->Cam_SetInteriorZoomFovFactor = &T_Camera_SetInteriorZoomFovFactor;
  camera_api->Cam_GetInteriorZoomSpeed = &T_Camera_GetInteriorZoomSpeed;
  camera_api->Cam_SetInteriorZoomSpeed = &T_Camera_SetInteriorZoomSpeed;

  // Azimuth Overrides
  camera_api->Cam_GetInteriorAzimuthOverridesCount = &T_Camera_GetInteriorAzimuthOverridesCount;
  camera_api->Cam_GetInteriorAzimuthOverrideAddress = &T_Camera_GetInteriorAzimuthOverrideAddress;
  camera_api->Cam_GetInteriorAzimuthOverrideOutside = &T_Camera_GetInteriorAzimuthOverrideOutside;
  camera_api->Cam_SetInteriorAzimuthOverrideOutside = &T_Camera_SetInteriorAzimuthOverrideOutside;
  camera_api->Cam_GetInteriorAzimuthOverrideStartAzimuth = &T_Camera_GetInteriorAzimuthOverrideStartAzimuth;
  camera_api->Cam_SetInteriorAzimuthOverrideStartAzimuth = &T_Camera_SetInteriorAzimuthOverrideStartAzimuth;
  camera_api->Cam_GetInteriorAzimuthOverrideEndAzimuth = &T_Camera_GetInteriorAzimuthOverrideEndAzimuth;
  camera_api->Cam_SetInteriorAzimuthOverrideEndAzimuth = &T_Camera_SetInteriorAzimuthOverrideEndAzimuth;
  camera_api->Cam_GetInteriorAzimuthOverrideStartUpLimit = &T_Camera_GetInteriorAzimuthOverrideStartUpLimit;
  camera_api->Cam_SetInteriorAzimuthOverrideStartUpLimit = &T_Camera_SetInteriorAzimuthOverrideStartUpLimit;
  camera_api->Cam_GetInteriorAzimuthOverrideEndUpLimit = &T_Camera_GetInteriorAzimuthOverrideEndUpLimit;
  camera_api->Cam_SetInteriorAzimuthOverrideEndUpLimit = &T_Camera_SetInteriorAzimuthOverrideEndUpLimit;
  camera_api->Cam_GetInteriorAzimuthOverrideStartDownLimit = &T_Camera_GetInteriorAzimuthOverrideStartDownLimit;
  camera_api->Cam_SetInteriorAzimuthOverrideStartDownLimit = &T_Camera_SetInteriorAzimuthOverrideStartDownLimit;
  camera_api->Cam_GetInteriorAzimuthOverrideEndDownLimit = &T_Camera_GetInteriorAzimuthOverrideEndDownLimit;
  camera_api->Cam_SetInteriorAzimuthOverrideEndDownLimit = &T_Camera_SetInteriorAzimuthOverrideEndDownLimit;
  camera_api->Cam_GetInteriorAzimuthOverrideStartUpDownDefault = &T_Camera_GetInteriorAzimuthOverrideStartUpDownDefault;
  camera_api->Cam_SetInteriorAzimuthOverrideStartUpDownDefault = &T_Camera_SetInteriorAzimuthOverrideStartUpDownDefault;
  camera_api->Cam_GetInteriorAzimuthOverrideEndUpDownDefault = &T_Camera_GetInteriorAzimuthOverrideEndUpDownDefault;
  camera_api->Cam_SetInteriorAzimuthOverrideEndUpDownDefault = &T_Camera_SetInteriorAzimuthOverrideEndUpDownDefault;
  camera_api->Cam_GetInteriorAzimuthOverrideStartLeftRightDefault = &T_Camera_GetInteriorAzimuthOverrideStartLeftRightDefault;
  camera_api->Cam_SetInteriorAzimuthOverrideStartLeftRightDefault = &T_Camera_SetInteriorAzimuthOverrideStartLeftRightDefault;
  camera_api->Cam_GetInteriorAzimuthOverrideEndLeftRightDefault = &T_Camera_GetInteriorAzimuthOverrideEndLeftRightDefault;
  camera_api->Cam_SetInteriorAzimuthOverrideEndLeftRightDefault = &T_Camera_SetInteriorAzimuthOverrideEndLeftRightDefault;
  camera_api->Cam_GetInteriorAzimuthOverrideStartHeadOffset = &T_Camera_GetInteriorAzimuthOverrideStartHeadOffset;
  camera_api->Cam_SetInteriorAzimuthOverrideStartHeadOffset = &T_Camera_SetInteriorAzimuthOverrideStartHeadOffset;
  camera_api->Cam_GetInteriorAzimuthOverrideEndHeadOffset = &T_Camera_GetInteriorAzimuthOverrideEndHeadOffset;
  camera_api->Cam_SetInteriorAzimuthOverrideEndHeadOffset = &T_Camera_SetInteriorAzimuthOverrideEndHeadOffset;

  // Shake Animation
  camera_api->Cam_GetInteriorShakeAnimCount = &T_Camera_GetInteriorShakeAnimCount;
  camera_api->Cam_GetInteriorShakeAnim = &T_Camera_GetInteriorShakeAnim;
  camera_api->Cam_SetInteriorShakeAnim = &T_Camera_SetInteriorShakeAnim;
}

}  // namespace Modules::API
SPF_NS_END