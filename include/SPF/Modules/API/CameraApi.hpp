#pragma once

#include "SPF/SPF_API/SPF_Camera_API.h"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Modules::API {
class CameraApi {
 public:
  static void FillCameraAPI(SPF_Camera_API* camera_api);

 private:
  // --- Core Camera Trampolines ---
  static void T_Camera_SwitchTo(SPF_CameraType cameraType);
  static void* T_Camera_GetCameraObject(void* manager, int index);
  static bool T_Camera_GetCurrentCamera(SPF_CameraType* out_cameraType);
  static void T_Camera_ResetToDefaults(SPF_CameraType cameraType);

  // --- Interior Camera Trampolines ---
  static bool T_Camera_GetInteriorSeatPos(float* x, float* y, float* z);
  static void T_Camera_SetInteriorSeatPos(float x, float y, float z);
  static bool T_Camera_GetInteriorHeadRot(float* yaw, float* pitch);
  static void T_Camera_SetInteriorHeadRot(float yaw, float pitch);
  static bool T_Camera_GetInteriorFov(float* fov);
  static bool T_Camera_GetInteriorFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetInteriorFov(float fov);
  static bool T_Camera_GetInteriorRotationLimits(float* left, float* right, float* up, float* down);
  static void T_Camera_SetInteriorRotationLimits(float left, float right, float up, float down);
  static bool T_Camera_GetInteriorRotationDefaults(float* lr, float* ud);
  static void T_Camera_SetInteriorRotationDefaults(float lr, float ud);

  // --- Behind Camera Trampolines ---
  static bool T_Camera_GetBehindLiveState(float* pitch, float* yaw, float* zoom);
  static bool T_Camera_GetBehindDistanceSettings(float* min, float* max, float* trailer_max_offset, float* def, float* trailer_def, float* change_speed, float* laziness);
  static void T_Camera_SetBehindDistanceSettings(float min, float max, float trailer_max_offset, float def, float trailer_def, float change_speed, float laziness);
  static bool T_Camera_GetBehindElevationSettings(float* azimuth_laziness, float* min, float* max, float* def, float* trailer_def, float* height_limit);
  static void T_Camera_SetBehindElevationSettings(float azimuth_laziness, float min, float max, float def, float trailer_def, float height_limit);
  static bool T_Camera_GetBehindPivot(float* x, float* y, float* z);
  static void T_Camera_SetBehindPivot(float x, float y, float z);
  static bool T_Camera_GetBehindDynamicOffset(float* max, float* speed_min, float* speed_max, float* laziness);
  static void T_Camera_SetBehindDynamicOffset(float max, float speed_min, float speed_max, float laziness);
  static bool T_Camera_GetBehindFov(float* fov);
  static bool T_Camera_GetBehindFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetBehindFov(float fov);

  // --- Top Camera Trampolines ---
  static bool T_Camera_GetTopHeight(float* min_height, float* max_height);
  static bool T_Camera_GetTopSpeed(float* speed);
  static bool T_Camera_GetTopOffsets(float* forward, float* backward);
  static void T_Camera_SetTopHeight(float min_height, float max_height);
  static void T_Camera_SetTopSpeed(float speed);
  static void T_Camera_SetTopOffsets(float forward, float backward);
  static bool T_Camera_GetTopFov(float* fov);
  static bool T_Camera_GetTopFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetTopFov(float fov);

  // --- Window Camera Trampolines ---
  static bool T_Camera_GetWindowHeadOffset(float* x, float* y, float* z);
  static bool T_Camera_GetWindowLiveRotation(float* yaw, float* pitch);
  static bool T_Camera_GetWindowRotationLimits(float* left, float* right, float* up, float* down);
  static bool T_Camera_GetWindowRotationDefaults(float* lr, float* ud);
  static void T_Camera_SetWindowHeadOffset(float x, float y, float z);
  static void T_Camera_SetWindowLiveRotation(float yaw, float pitch);
  static void T_Camera_SetWindowRotationLimits(float left, float right, float up, float down);
  static void T_Camera_SetWindowRotationDefaults(float lr, float ud);
  static bool T_Camera_GetWindowFov(float* fov);
  static bool T_Camera_GetWindowFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetWindowFov(float fov);

  // --- Bumper Camera Trampolines ---
  static bool T_Camera_GetBumperOffset(float* offset_x, float* offset_y, float* offset_z);
  static void T_Camera_SetBumperOffset(float offset_x, float offset_y, float offset_z);
  static bool T_Camera_GetBumperFov(float* fov);
  static bool T_Camera_GetBumperFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetBumperFov(float fov);

  // --- Wheel Camera Trampolines ---
  static bool T_Camera_GetWheelOffset(float* offset_x, float* offset_y, float* offset_z);
  static void T_Camera_SetWheelOffset(float offset_x, float offset_y, float offset_z);
  static bool T_Camera_GetWheelFov(float* fov);
  static bool T_Camera_GetWheelFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetWheelFov(float fov);

  // --- Cabin Camera Trampolines ---
  static bool T_Camera_GetCabinFov(float* fov);
  static bool T_Camera_GetCabinFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetCabinFov(float fov);

  // --- TV Camera Trampolines ---
  static bool T_Camera_GetTVMaxDistance(float* max_distance);
  static bool T_Camera_GetTVPrefabUplift(float* x, float* y, float* z);
  static bool T_Camera_GetTVRoadUplift(float* x, float* y, float* z);
  static void T_Camera_SetTVMaxDistance(float max_distance);
  static void T_Camera_SetTVPrefabUplift(float x, float y, float z);
  static void T_Camera_SetTVRoadUplift(float x, float y, float z);
  static bool T_Camera_GetTVFov(float* fov);
  static bool T_Camera_GetTVFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetTVFov(float fov);

  // --- World Coordinates Trampolines ---
  static bool T_Camera_GetWorldCoordinates(float* x, float* y, float* z);

  // --- Free Camera Trampolines ---
  static bool T_Camera_GetFreePosition(float* x, float* y, float* z);
  static bool T_Camera_GetFreeQuaternion(float* x, float* y, float* z, float* w);
  static void T_Camera_SetFreePosition(float x, float y, float z);
  static bool T_Camera_GetFreeOrientation(float* mouse_x, float* mouse_y, float* roll);
  static void T_Camera_SetFreeOrientation(float mouse_x, float mouse_y, float roll);
  static bool T_Camera_GetFreeFov(float* fov);
  static bool T_Camera_GetFreeFinalFov(float* out_horiz, float* out_vert);
  static void T_Camera_SetFreeFov(float fov);
  static bool T_Camera_GetFreeSpeed(float* speed);
  static void T_Camera_SetFreeSpeed(float speed);

  // --- Debug Camera Trampolines ---
  static void T_Camera_EnableDebugCamera(bool enable);
  static bool T_Camera_GetDebugCameraEnabled(bool* out_isEnabled);
  static void T_Camera_SetDebugCameraMode(SPF_DebugCameraMode mode);
  static bool T_Camera_GetDebugCameraMode(SPF_DebugCameraMode* out_mode);

  // Debug Camera HUD & UI Trampolines
  static void T_Camera_SetDebugHudVisible(bool visible);
  static bool T_Camera_GetDebugHudVisible(bool* out_isVisible);
  static void T_Camera_SetDebugHudPosition(SPF_DebugHudPosition position);
  static bool T_Camera_GetDebugHudPosition(SPF_DebugHudPosition* out_position);
  static void T_Camera_SetDebugGameUiVisible(bool visible);
  static bool T_Camera_GetDebugGameUiVisible(bool* out_isVisible);

  // --- Debug Camera State Trampolines ---
  static int T_Camera_GetStateCount();
  static int T_Camera_GetCurrentStateIndex();
  static bool T_Camera_GetState(int index, SPF_CameraState_t* out_state);
  static void T_Camera_ApplyState(int index);
  static void T_Camera_CycleState(int direction);
  static void T_Camera_SaveCurrentState();
  static void T_Camera_ReloadStatesFromFile();

  // --- In-Memory State Trampolines ---
  static void T_Camera_ClearAllStatesInMemory();
  static void T_Camera_AddStateInMemory(const SPF_CameraState_t* state);
  static bool T_Camera_EditStateInMemory(int index, const SPF_CameraState_t* newState);
  static void T_Camera_DeleteStateInMemory(int index);

  // --- Animation Control Trampolines ---
  static void T_Anim_Play(int startIndex);
  static void T_Anim_Pause();
  static void T_Anim_Stop();
  static void T_Anim_GoToFrame(int frameIndex);
  static void T_Anim_ScrubTo(float position);
  static void T_Anim_SetReverse(bool isReversed);
  static SPF_AnimPlaybackState T_Anim_GetPlaybackState();
  static int T_Anim_GetCurrentFrame();
  static float T_Anim_GetCurrentFrameProgress();
  static bool T_Anim_IsReversed();
};
}  // namespace Modules::API
SPF_NS_END