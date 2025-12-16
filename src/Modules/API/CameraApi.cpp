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

  camera_api->SwitchTo = &CameraApi::T_Camera_SwitchTo;
  camera_api->GetCameraObject = &CameraApi::T_Camera_GetCameraObject;
  camera_api->GetCurrentCamera = &CameraApi::T_Camera_GetCurrentCamera;
  camera_api->ResetToDefaults = &CameraApi::T_Camera_ResetToDefaults;

  // Interior
  camera_api->GetInteriorSeatPos = &T_Camera_GetInteriorSeatPos;
  camera_api->SetInteriorSeatPos = &T_Camera_SetInteriorSeatPos;
  camera_api->GetInteriorHeadRot = &T_Camera_GetInteriorHeadRot;
  camera_api->SetInteriorHeadRot = &T_Camera_SetInteriorHeadRot;
  camera_api->GetInteriorFov = &T_Camera_GetInteriorFov;
  camera_api->GetInteriorFinalFov = &T_Camera_GetInteriorFinalFov;
  camera_api->SetInteriorFov = &T_Camera_SetInteriorFov;
  camera_api->GetInteriorRotationLimits = &T_Camera_GetInteriorRotationLimits;
  camera_api->SetInteriorRotationLimits = &T_Camera_SetInteriorRotationLimits;
  camera_api->GetInteriorRotationDefaults = &T_Camera_GetInteriorRotationDefaults;
  camera_api->SetInteriorRotationDefaults = &T_Camera_SetInteriorRotationDefaults;

  // Behind
  camera_api->GetBehindLiveState = &T_Camera_GetBehindLiveState;
  camera_api->GetBehindDistanceSettings = &T_Camera_GetBehindDistanceSettings;
  camera_api->SetBehindDistanceSettings = &T_Camera_SetBehindDistanceSettings;
  camera_api->GetBehindElevationSettings = &T_Camera_GetBehindElevationSettings;
  camera_api->SetBehindElevationSettings = &T_Camera_SetBehindElevationSettings;
  camera_api->GetBehindPivot = &T_Camera_GetBehindPivot;
  camera_api->SetBehindPivot = &T_Camera_SetBehindPivot;
  camera_api->GetBehindDynamicOffset = &T_Camera_GetBehindDynamicOffset;
  camera_api->SetBehindDynamicOffset = &T_Camera_SetBehindDynamicOffset;
  camera_api->GetBehindFov = &T_Camera_GetBehindFov;
  camera_api->GetBehindFinalFov = &T_Camera_GetBehindFinalFov;
  camera_api->SetBehindFov = &T_Camera_SetBehindFov;

  // Top
  camera_api->GetTopHeight = &T_Camera_GetTopHeight;
  camera_api->GetTopSpeed = &T_Camera_GetTopSpeed;
  camera_api->GetTopOffsets = &T_Camera_GetTopOffsets;
  camera_api->SetTopHeight = &T_Camera_SetTopHeight;
  camera_api->SetTopSpeed = &T_Camera_SetTopSpeed;
  camera_api->SetTopOffsets = &T_Camera_SetTopOffsets;
  camera_api->GetTopFov = &T_Camera_GetTopFov;
  camera_api->GetTopFinalFov = &T_Camera_GetTopFinalFov;
  camera_api->SetTopFov = &T_Camera_SetTopFov;

  // Window
  camera_api->GetWindowHeadOffset = &T_Camera_GetWindowHeadOffset;
  camera_api->GetWindowLiveRotation = &T_Camera_GetWindowLiveRotation;
  camera_api->GetWindowRotationLimits = &T_Camera_GetWindowRotationLimits;
  camera_api->GetWindowRotationDefaults = &T_Camera_GetWindowRotationDefaults;
  camera_api->SetWindowHeadOffset = &T_Camera_SetWindowHeadOffset;
  camera_api->SetWindowLiveRotation = &T_Camera_SetWindowLiveRotation;
  camera_api->SetWindowRotationLimits = &T_Camera_SetWindowRotationLimits;
  camera_api->SetWindowRotationDefaults = &T_Camera_SetWindowRotationDefaults;
  camera_api->GetWindowFov = &T_Camera_GetWindowFov;
  camera_api->GetWindowFinalFov = &T_Camera_GetWindowFinalFov;
  camera_api->SetWindowFov = &T_Camera_SetWindowFov;

  // Bumper
  camera_api->GetBumperOffset = &T_Camera_GetBumperOffset;
  camera_api->SetBumperOffset = &T_Camera_SetBumperOffset;
  camera_api->GetBumperFov = &T_Camera_GetBumperFov;
  camera_api->GetBumperFinalFov = &T_Camera_GetBumperFinalFov;
  camera_api->SetBumperFov = &T_Camera_SetBumperFov;

  // Wheel
  camera_api->GetWheelOffset = &T_Camera_GetWheelOffset;
  camera_api->SetWheelOffset = &T_Camera_SetWheelOffset;
  camera_api->GetWheelFov = &T_Camera_GetWheelFov;
  camera_api->GetWheelFinalFov = &T_Camera_GetWheelFinalFov;
  camera_api->SetWheelFov = &T_Camera_SetWheelFov;

  // Cabin
  camera_api->GetCabinFov = &T_Camera_GetCabinFov;
  camera_api->GetCabinFinalFov = &T_Camera_GetCabinFinalFov;
  camera_api->SetCabinFov = &T_Camera_SetCabinFov;

  // TV
  camera_api->GetTVMaxDistance = &T_Camera_GetTVMaxDistance;
  camera_api->GetTVPrefabUplift = &T_Camera_GetTVPrefabUplift;
  camera_api->GetTVRoadUplift = &T_Camera_GetTVRoadUplift;
  camera_api->SetTVMaxDistance = &T_Camera_SetTVMaxDistance;
  camera_api->SetTVPrefabUplift = &T_Camera_SetTVPrefabUplift;
  camera_api->SetTVRoadUplift = &T_Camera_SetTVRoadUplift;
  camera_api->GetTVFov = &T_Camera_GetTVFov;
  camera_api->GetTVFinalFov = &T_Camera_GetTVFinalFov;
  camera_api->SetTVFov = &T_Camera_SetTVFov;

  // World Coords
  camera_api->GetCameraWorldCoordinates = &T_Camera_GetWorldCoordinates;

  // Free Cam
  camera_api->GetFreePosition = &T_Camera_GetFreePosition;
  camera_api->SetFreePosition = &T_Camera_SetFreePosition;
  camera_api->GetFreeQuaternion = &T_Camera_GetFreeQuaternion;
  camera_api->GetFreeOrientation = &T_Camera_GetFreeOrientation;
  camera_api->SetFreeOrientation = &T_Camera_SetFreeOrientation;
  camera_api->GetFreeFov = &T_Camera_GetFreeFov;
  camera_api->GetFreeFinalFov = &T_Camera_GetFreeFinalFov;
  camera_api->SetFreeFov = &T_Camera_SetFreeFov;
  camera_api->GetFreeSpeed = &T_Camera_GetFreeSpeed;
  camera_api->SetFreeSpeed = &T_Camera_SetFreeSpeed;

  // Debug
  camera_api->EnableDebugCamera = &T_Camera_EnableDebugCamera;
  camera_api->GetDebugCameraEnabled = &T_Camera_GetDebugCameraEnabled;
  camera_api->SetDebugCameraMode = &T_Camera_SetDebugCameraMode;
  camera_api->GetDebugCameraMode = &T_Camera_GetDebugCameraMode;

  // Debug HUD & UI
  camera_api->SetDebugHudVisible = &T_Camera_SetDebugHudVisible;
  camera_api->GetDebugHudVisible = &T_Camera_GetDebugHudVisible;
  camera_api->SetDebugHudPosition = &T_Camera_SetDebugHudPosition;
  camera_api->GetDebugHudPosition = &T_Camera_GetDebugHudPosition;
  camera_api->SetDebugGameUiVisible = &T_Camera_SetDebugGameUiVisible;
  camera_api->GetDebugGameUiVisible = &T_Camera_GetDebugGameUiVisible;

  // Debug Camera State Management
  camera_api->GetStateCount = &T_Camera_GetStateCount;
  camera_api->GetCurrentStateIndex = &T_Camera_GetCurrentStateIndex;
  camera_api->GetState = &T_Camera_GetState;
  camera_api->ApplyState = &T_Camera_ApplyState;
  camera_api->CycleState = &T_Camera_CycleState;
  camera_api->SaveCurrentState = &T_Camera_SaveCurrentState;
  camera_api->ReloadStatesFromFile = &T_Camera_ReloadStatesFromFile;

  // In-Memory State Management
  camera_api->ClearAllStatesInMemory = &T_Camera_ClearAllStatesInMemory;
  camera_api->AddStateInMemory = &T_Camera_AddStateInMemory;
  camera_api->EditStateInMemory = &T_Camera_EditStateInMemory;
  camera_api->DeleteStateInMemory = &T_Camera_DeleteStateInMemory;

  // Animation Control
  camera_api->Anim_Play = &T_Anim_Play;
  camera_api->Anim_Pause = &T_Anim_Pause;
  camera_api->Anim_Stop = &T_Anim_Stop;
  camera_api->Anim_GoToFrame = &T_Anim_GoToFrame;
  camera_api->Anim_ScrubTo = &T_Anim_ScrubTo;
  camera_api->Anim_SetReverse = &T_Anim_SetReverse;
  camera_api->Anim_GetPlaybackState = &T_Anim_GetPlaybackState;
  camera_api->Anim_GetCurrentFrame = &T_Anim_GetCurrentFrame;
  camera_api->Anim_GetCurrentFrameProgress = &T_Anim_GetCurrentFrameProgress;
  camera_api->Anim_IsReversed = &T_Anim_IsReversed;
}

}  // namespace Modules::API
SPF_NS_END