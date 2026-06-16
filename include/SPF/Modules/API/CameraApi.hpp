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
  static void T_Camera_SetBehindLiveState(float pitch, float yaw, float zoom);
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

  // --- New Debug Camera Control Trampolines ---
  static void T_Camera_SetDebugPosLock(bool locked);
  static bool T_Camera_GetDebugPosLock(bool* out_locked);
  static void T_Camera_SetDebugRotLock(bool locked);
  static bool T_Camera_GetDebugRotLock(bool* out_locked);
  static void T_Camera_SetDebugOrbitMode(bool enabled);
  static bool T_Camera_GetDebugOrbitMode(bool* out_enabled);
  static void T_Camera_SetDebugOrbitSpeed(float speed);
  static bool T_Camera_GetDebugOrbitSpeed(float* out_speed);
  static void* T_Camera_GetDebugSelectedObject();
  static void T_Camera_SetDebugSelectedObject(void* ptr);
  static void* T_Camera_GetDebugHoveredObject();

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

  // --- Framework & Service Status Trampolines ---
  static bool T_Camera_IsServiceReady();
  static bool T_Camera_AreAllOffsetsFound();
  static bool T_Camera_IsFinderReady(const char* finderName);
  static bool T_Camera_RefreshOffsets();

  // --- Viewport & Projection Trampolines ---
  static bool T_Camera_GetViewport(float* x1, float* x2, float* y1, float* y2);
  static uintptr_t T_Camera_GetCameraParamsObjectPtr();

  // --- Animation Preparation Trampolines ---
  static bool T_Camera_Anim_Prepare();

  // --- Object Targeting & Inspection Trampolines ---
  static uintptr_t T_Camera_GetDebugObjectAddress(void* ptr);

  // --- New Interior Advanced Settings Trampolines ---
  static bool T_Camera_GetInteriorOutside(bool* out_val);
  static void T_Camera_SetInteriorOutside(bool val);
  static bool T_Camera_GetInteriorNearPlane(float* out_val);
  static void T_Camera_SetInteriorNearPlane(float val);
  static bool T_Camera_GetInteriorFarPlane(float* out_val);
  static void T_Camera_SetInteriorFarPlane(float val);
  static bool T_Camera_GetInteriorMouseSensitivity(float* out_val);
  static void T_Camera_SetInteriorMouseSensitivity(float val);
  static bool T_Camera_GetInteriorShakeAnimStep(float* out_val);
  static void T_Camera_SetInteriorShakeAnimStep(float val);
  static bool T_Camera_GetInteriorShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetInteriorShakeAnimScaleMin(float val);
  static bool T_Camera_GetInteriorShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetInteriorShakeAnimScaleMax(float val);
  static bool T_Camera_GetInteriorHandShakeLimit(float* out_val);
  static void T_Camera_SetInteriorHandShakeLimit(float val);
  static bool T_Camera_GetInteriorHandShakeSpeed(float* out_val);
  static void T_Camera_SetInteriorHandShakeSpeed(float val);
  static bool T_Camera_GetInteriorZoomFovFactor(float* out_val);
  static void T_Camera_SetInteriorZoomFovFactor(float val);
  static bool T_Camera_GetInteriorZoomSpeed(float* out_val);
  static void T_Camera_SetInteriorZoomSpeed(float val);

  // --- Azimuth Overrides Trampolines ---
  static size_t T_Camera_GetInteriorAzimuthOverridesCount();
  static void* T_Camera_GetInteriorAzimuthOverrideAddress(size_t index);
  static bool T_Camera_GetInteriorAzimuthOverrideOutside(size_t index, bool* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideOutside(size_t index, bool val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartAzimuth(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideStartAzimuth(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideEndAzimuth(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideEndAzimuth(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartUpLimit(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideStartUpLimit(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideEndUpLimit(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideEndUpLimit(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartDownLimit(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideStartDownLimit(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideEndDownLimit(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideEndDownLimit(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartUpDownDefault(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideStartUpDownDefault(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideEndUpDownDefault(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideEndUpDownDefault(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartLeftRightDefault(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideStartLeftRightDefault(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideEndLeftRightDefault(size_t index, float* out_val);
  static void T_Camera_SetInteriorAzimuthOverrideEndLeftRightDefault(size_t index, float val);
  static bool T_Camera_GetInteriorAzimuthOverrideStartHeadOffset(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetInteriorAzimuthOverrideStartHeadOffset(size_t index, float x, float y, float z);
  static bool T_Camera_GetInteriorAzimuthOverrideEndHeadOffset(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetInteriorAzimuthOverrideEndHeadOffset(size_t index, float x, float y, float z);

  // --- Shake Animation Trampolines ---
  static size_t T_Camera_GetInteriorShakeAnimCount();
  static bool T_Camera_GetInteriorShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetInteriorShakeAnim(size_t index, float x, float y, float z);

  // --- New Behind Advanced Settings Trampolines ---
  static bool T_Camera_GetBehindValidation(bool* out_val);
  static void T_Camera_SetBehindValidation(bool val);
  static bool T_Camera_GetBehindValidationSettings(float* out_radius, float* out_speed_pos, float* out_speed_neg);
  static void T_Camera_SetBehindValidationSettings(float radius, float speed_pos, float speed_neg);
  static bool T_Camera_GetBehindSpeedFovChangeFactor(float* out_val);
  static void T_Camera_SetBehindSpeedFovChangeFactor(float val);
  static bool T_Camera_GetBehindShakeAnimStep(float* out_val);
  static void T_Camera_SetBehindShakeAnimStep(float val);
  static bool T_Camera_GetBehindShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetBehindShakeAnimScaleMin(float val);
  static bool T_Camera_GetBehindShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetBehindShakeAnimScaleMax(float val);
  static size_t T_Camera_GetBehindShakeAnimCount();
  static bool T_Camera_GetBehindShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetBehindShakeAnim(size_t index, float x, float y, float z);

  // --- New Top Advanced Settings Trampolines ---
  static bool T_Camera_GetTopOffsetsZ(float* forward, float* backward);
  static void T_Camera_SetTopOffsetsZ(float forward, float backward);
  static bool T_Camera_GetTopAdaptiveSettings(float* out_factor, bool* out_use_adaptive);
  static void T_Camera_SetTopAdaptiveSettings(float factor, bool use_adaptive);
  static bool T_Camera_GetTopPlaneSettings(float* out_near, float* out_far);
  static void T_Camera_SetTopPlaneSettings(float near_p, float far_p);
  static bool T_Camera_GetTopValidation(bool* out_val);
  static void T_Camera_SetTopValidation(bool val);
  static bool T_Camera_GetTopValidationSettings(float* out_speed_pos, float* out_speed_neg);
  static void T_Camera_SetTopValidationSettings(float speed_pos, float speed_neg);
  static bool T_Camera_GetTopShakeAnimStep(float* out_val);
  static void T_Camera_SetTopShakeAnimStep(float val);
  static bool T_Camera_GetTopShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetTopShakeAnimScaleMin(float val);
  static bool T_Camera_GetTopShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetTopShakeAnimScaleMax(float val);
  static size_t T_Camera_GetTopShakeAnimCount();
  static bool T_Camera_GetTopShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetTopShakeAnim(size_t index, float x, float y, float z);

  // --- New Cabin Advanced Settings Trampolines ---
  static bool T_Camera_GetCabinShakeAnimStep(float* out_val);
  static void T_Camera_SetCabinShakeAnimStep(float val);
  static bool T_Camera_GetCabinShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetCabinShakeAnimScaleMin(float val);
  static bool T_Camera_GetCabinShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetCabinShakeAnimScaleMax(float val);
  static size_t T_Camera_GetCabinShakeAnimCount();
  static bool T_Camera_GetCabinShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetCabinShakeAnim(size_t index, float x, float y, float z);

  // --- New Window Advanced Settings Trampolines ---
  static bool T_Camera_GetWindowRelativeHeadtrackingAzimuth(bool* out_val);
  static void T_Camera_SetWindowRelativeHeadtrackingAzimuth(bool val);
  static bool T_Camera_GetWindowAutoCenterMoveDirection(int32_t* out_val);
  static void T_Camera_SetWindowAutoCenterMoveDirection(int32_t val);
  static bool T_Camera_GetWindowShakeAnimStep(float* out_val);
  static void T_Camera_SetWindowShakeAnimStep(float val);
  static bool T_Camera_GetWindowShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetWindowShakeAnimScaleMin(float val);
  static bool T_Camera_GetWindowShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetWindowShakeAnimScaleMax(float val);
  static size_t T_Camera_GetWindowShakeAnimCount();
  static bool T_Camera_GetWindowShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetWindowShakeAnim(size_t index, float x, float y, float z);

  // --- New Bumper Advanced Settings Trampolines ---
  static bool T_Camera_GetBumperShakeAnimStep(float* out_val);
  static void T_Camera_SetBumperShakeAnimStep(float val);
  static bool T_Camera_GetBumperShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetBumperShakeAnimScaleMin(float val);
  static bool T_Camera_GetBumperShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetBumperShakeAnimScaleMax(float val);
  static size_t T_Camera_GetBumperShakeAnimCount();
  static bool T_Camera_GetBumperShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetBumperShakeAnim(size_t index, float x, float y, float z);

  // --- New Wheel Advanced Settings Trampolines ---
  static bool T_Camera_GetWheelShakeAnimStep(float* out_val);
  static void T_Camera_SetWheelShakeAnimStep(float val);
  static bool T_Camera_GetWheelShakeAnimScaleMin(float* out_val);
  static void T_Camera_SetWheelShakeAnimScaleMin(float val);
  static bool T_Camera_GetWheelShakeAnimScaleMax(float* out_val);
  static void T_Camera_SetWheelShakeAnimScaleMax(float val);
  static size_t T_Camera_GetWheelShakeAnimCount();
  static bool T_Camera_GetWheelShakeAnim(size_t index, float* out_x, float* out_y, float* out_z);
  static void T_Camera_SetWheelShakeAnim(size_t index, float x, float y, float z);
  };
  }  // namespace Modules::API
  SPF_NS_END