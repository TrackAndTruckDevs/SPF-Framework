#pragma once

#include "SPF/Hooks/IHook.hpp"
#include "SPF/Data/GameData/ICameraDataFinder.hpp"
#include "SPF/Namespace.hpp"
#include <string>
#include <cstdint>
#include <vector>
#include <memory>

SPF_NS_BEGIN
namespace Data::GameData {
/**
 * @class GameDataCameraService
 * @brief A service that provides memory offsets and pointers for the game's camera system.
 *
 * This class acts as a central repository for camera-related memory data. It no longer
 * contains the logic for finding this data itself. Instead, it manages a collection
 * of ICameraDataFinder objects, each responsible for finding a specific subset of data.
 */
class GameDataCameraService {
 public:
  static GameDataCameraService& GetInstance();

  GameDataCameraService(const GameDataCameraService&) = delete;
  void operator=(const GameDataCameraService&) = delete;

  void Initialize();
  void Shutdown();
  bool IsReady() const { return m_isInitialized; }
  bool IsFinderReady(const char* name) const;
  bool AreAllFindersReady() const;
  bool TryFindAllOffsets();

  void UpdateFinders();

  // --- Public Getters ---
  intptr_t GetCameraFovOffset() const { return m_camera_fov_offset; }
  intptr_t GetNearPlaneOffset() const { return m_near_plane_offset; }
  intptr_t GetFarPlaneOffset() const { return m_far_plane_offset; }
  intptr_t GetMouseSensitivityOffset() const { return m_mouse_sensitivity_offset; }
  intptr_t GetShakeAnimStepOffset() const { return m_shake_anim_step_offset; }
  intptr_t GetShakeAnimScaleMinOffset() const { return m_shake_anim_scale_min_offset; }
  intptr_t GetShakeAnimScaleMaxOffset() const { return m_shake_anim_scale_max_offset; }
  intptr_t GetShakeAnimOffset() const { return m_shake_anim_offset; }
  intptr_t GetHandShakeLimitOffset() const { return m_hand_shake_limit_offset; }
  intptr_t GetHandShakeSpeedOffset() const { return m_hand_shake_speed_offset; }
  uintptr_t GetStandardManagerPtrAddr() const { return m_pStandardManagerPtrAddr; }

  /**
   * @brief Returns the actual, adjusted pointer to the StandardManager object.
   * This handles game versions where the manager object's base address is offset from the global pointer.
   */
  uintptr_t GetStandardManager() const {
    if (m_pStandardManagerPtrAddr == 0) return 0;
    uintptr_t rawPtr = *reinterpret_cast<uintptr_t*>(m_pStandardManagerPtrAddr);
    if (rawPtr == 0) return 0;
    return rawPtr + m_standardManagerAdjustment;
  }

  intptr_t GetActiveCameraIdOffset() const { return m_activeCameraIdOffset; }
  uintptr_t* GetFreecamGlobalObjectPtr() const { return m_pFreecamGlobalObjectPtr; }

  /**
   * @brief Returns the actual, adjusted pointer to the Freecam Global object.
   * Similar to GetStandardManager, handles v1.59+ pointer adjustments.
   */
  uintptr_t GetFreecamGlobalObject() const {
    if (m_pFreecamGlobalObjectPtr == nullptr) return 0;
    uintptr_t rawPtr = *m_pFreecamGlobalObjectPtr;
    if (rawPtr == 0) return 0;
    return rawPtr + m_freecamGlobalObjectAdjustment;
  }

  uintptr_t GetFreecamContextOffset() const { return m_freecamContextOffset; }
  intptr_t GetInteriorSeatXOffset() const { return m_interior_seat_x_offset; }
  intptr_t GetInteriorSeatYOffset() const { return m_interior_seat_y_offset; }
  intptr_t GetInteriorSeatZOffset() const { return m_interior_seat_z_offset; }
  intptr_t GetInteriorYawOffset() const { return m_interior_yaw_offset; }
  intptr_t GetInteriorPitchOffset() const { return m_interior_pitch_offset; }
  intptr_t GetInteriorLimitLeftOffset() const { return m_interior_limit_left_offset; }
  intptr_t GetInteriorLimitRightOffset() const { return m_interior_limit_right_offset; }
  intptr_t GetInteriorLimitUpOffset() const { return m_interior_limit_up_offset; }
  intptr_t GetInteriorLimitDownOffset() const { return m_interior_limit_down_offset; }
  intptr_t GetInteriorOutsideOffset() const { return m_interior_outside_offset; }
  intptr_t GetFovBaseOffset() const { return m_fov_base_offset; }
  intptr_t GetFovHorizFinalOffset() const { return m_fov_horiz_final_offset; }
  intptr_t GetFovVertFinalOffset() const { return m_fov_vert_final_offset; }
  intptr_t GetInteriorMouseLRDefaultOffset() const { return m_interior_mouse_lr_default; }
  intptr_t GetInteriorMouseUDDefaultOffset() const { return m_interior_mouse_ud_default; }
  intptr_t GetInteriorAzimuthOverridesOffset() const { return m_interior_azimuth_overrides_offset; }
  intptr_t GetZoomFovFactorOffset() const { return m_zoom_fov_factor_offset; }
  intptr_t GetZoomSpeedOffset() const { return m_zoom_speed_offset; }
  intptr_t GetAzimuthRangeOutsideOffset() const { return m_azimuth_range_outside_offset; }
  intptr_t GetAzimuthRangeStartAzimuthOffset() const { return m_azimuth_range_start_azimuth_offset; }
  intptr_t GetAzimuthRangeEndAzimuthOffset() const { return m_azimuth_range_end_azimuth_offset; }
  intptr_t GetAzimuthRangeStartUpLimitOffset() const { return m_azimuth_range_start_up_limit_offset; }
  intptr_t GetAzimuthRangeEndUpLimitOffset() const { return m_azimuth_range_end_up_limit_offset; }
  intptr_t GetAzimuthRangeStartDownLimitOffset() const { return m_azimuth_range_start_down_limit_offset; }
  intptr_t GetAzimuthRangeEndDownLimitOffset() const { return m_azimuth_range_end_down_limit_offset; }
  intptr_t GetAzimuthRangeStartUpDownDefaultOffset() const { return m_azimuth_range_start_up_down_default_offset; }
  intptr_t GetAzimuthRangeEndUpDownDefaultOffset() const { return m_azimuth_range_end_up_down_default_offset; }
  intptr_t GetAzimuthRangeStartLeftRightDefaultOffset() const { return m_azimuth_range_start_left_right_default_offset; }
  intptr_t GetAzimuthRangeEndLeftRightDefaultOffset() const { return m_azimuth_range_end_left_right_default_offset; }
  intptr_t GetAzimuthRangeStartHeadOffsetOffset() const { return m_azimuth_range_start_head_offset_offset; }
  intptr_t GetAzimuthRangeEndHeadOffsetOffset() const { return m_azimuth_range_end_head_offset_offset; }
  uintptr_t GetCameraParamsObjectPtr() const { return m_pCameraParamsObject; }
  intptr_t GetViewportX1Offset() const { return m_viewport_x1_offset; }
  intptr_t GetViewportX2Offset() const { return m_viewport_x2_offset; }
  intptr_t GetViewportY1Offset() const { return m_viewport_y1_offset; }
  intptr_t GetViewportY2Offset() const { return m_viewport_y2_offset; }

  // --- Behind Camera Getters ---
  intptr_t GetBehindLivePitchOffset() const { return m_behind_live_pitch_offset; }
  intptr_t GetBehindLiveYawOffset() const { return m_behind_live_yaw_offset; }
  intptr_t GetBehindLiveZoomOffset() const { return m_behind_live_zoom_offset; }
  intptr_t GetBehindDistanceMinOffset() const { return m_behind_distance_min_offset; }
  intptr_t GetBehindDistanceMaxOffset() const { return m_behind_distance_max_offset; }
  intptr_t GetBehindDistanceTrailerMaxOffset() const { return m_behind_distance_trailer_max_offset; }
  intptr_t GetBehindDistanceDefaultOffset() const { return m_behind_distance_default_offset; }
  intptr_t GetBehindDistanceTrailerDefaultOffset() const { return m_behind_distance_trailer_default_offset; }
  intptr_t GetBehindDistanceChangeSpeedOffset() const { return m_behind_distance_change_speed_offset; }
  intptr_t GetBehindDistanceLazinessSpeedOffset() const { return m_behind_distance_laziness_speed_offset; }
  intptr_t GetBehindAzimuthLazinessSpeedOffset() const { return m_behind_azimuth_laziness_speed_offset; }
  intptr_t GetBehindElevationMinOffset() const { return m_behind_elevation_min_offset; }
  intptr_t GetBehindElevationMaxOffset() const { return m_behind_elevation_max_offset; }
  intptr_t GetBehindElevationDefaultOffset() const { return m_behind_elevation_default_offset; }
  intptr_t GetBehindElevationTrailerDefaultOffset() const { return m_behind_elevation_trailer_default_offset; }
  intptr_t GetBehindHeightLimitOffset() const { return m_behind_height_limit_offset; }
  intptr_t GetBehindPivotXOffset() const { return m_behind_pivot_x_offset; }
  intptr_t GetBehindPivotYOffset() const { return m_behind_pivot_y_offset; }
  intptr_t GetBehindPivotZOffset() const { return m_behind_pivot_z_offset; }
  intptr_t GetBehindDynamicOffsetMaxOffset() const { return m_behind_dynamic_offset_max_offset; }
  intptr_t GetBehindDynamicOffsetSpeedMinOffset() const { return m_behind_dynamic_offset_speed_min_offset; }
  intptr_t GetBehindDynamicOffsetSpeedMaxOffset() const { return m_behind_dynamic_offset_speed_max_offset; }
  intptr_t GetBehindDynamicOffsetLazinessSpeedOffset() const { return m_behind_dynamic_offset_laziness_speed_offset; }

  // --- Behind Camera Validation Getters ---
  intptr_t GetBehindValidationOffset() const { return m_behind_validation_offset; }
  intptr_t GetBehindValidationSpeedPositiveOffset() const { return m_behind_validation_speed_positive_offset; }
  intptr_t GetBehindValidationSpeedNegativeOffset() const { return m_behind_validation_speed_negative_offset; }
  intptr_t GetBehindValidationRadiusOffset() const { return m_behind_validation_radius_offset; }
  intptr_t GetBehindSpeedFovChangeFactorOffset() const { return m_behind_speed_fov_change_factor_offset; }

  // --- Top Camera Getters ---
  intptr_t GetTopMinHeightOffset() const { return m_top_min_height_offset; }
  intptr_t GetTopMaxHeightOffset() const { return m_top_max_height_offset; }
  intptr_t GetTopSpeedOffset() const { return m_top_speed_offset; }
  intptr_t GetTopXOffsetForwardOffset() const { return m_top_x_offset_forward_offset; }
  intptr_t GetTopXOffsetBackwardOffset() const { return m_top_x_offset_backward_offset; }

  // --- Window Camera Getters ---
  intptr_t GetWindowHeadOffsetXOffset() const { return m_window_head_offset_x; }
  intptr_t GetWindowHeadOffsetYOffset() const { return m_window_head_offset_y; }
  intptr_t GetWindowHeadOffsetZOffset() const { return m_window_head_offset_z; }
  intptr_t GetWindowLiveYawOffset() const { return m_window_live_yaw; }
  intptr_t GetWindowLivePitchOffset() const { return m_window_live_pitch; }
  intptr_t GetWindowMouseLeftLimitOffset() const { return m_window_mouse_left_limit; }
  intptr_t GetWindowMouseRightLimitOffset() const { return m_window_mouse_right_limit; }
  intptr_t GetWindowMouseLRDefaultOffset() const { return m_window_mouse_lr_default; }
  intptr_t GetWindowMouseUpLimitOffset() const { return m_window_mouse_up_limit; }
  intptr_t GetWindowMouseDownLimitOffset() const { return m_window_mouse_down_limit; }
  intptr_t GetWindowMouseUDDefaultOffset() const { return m_window_mouse_ud_default; }

  // --- Bumper Camera Getters ---
  intptr_t GetBumperOffsetXOffset() const { return m_bumper_offset_x; }
  intptr_t GetBumperOffsetYOffset() const { return m_bumper_offset_y; }
  intptr_t GetBumperOffsetZOffset() const { return m_bumper_offset_z; }

  // --- Wheel Camera Getters ---
  intptr_t GetWheelOffsetXOffset() const { return m_wheel_offset_x; }
  intptr_t GetWheelOffsetYOffset() const { return m_wheel_offset_y; }
  intptr_t GetWheelOffsetZOffset() const { return m_wheel_offset_z; }

  // --- TV Camera Getters ---
  intptr_t GetTVMaxDistanceOffset() const { return m_tv_max_distance; }
  intptr_t GetTVPrefabUpliftXOffset() const { return m_tv_prefab_uplift_x; }
  intptr_t GetTVPrefabUpliftYOffset() const { return m_tv_prefab_uplift_y; }
  intptr_t GetTVPrefabUpliftZOffset() const { return m_tv_prefab_uplift_z; }
  intptr_t GetTVRoadUpliftXOffset() const { return m_tv_road_uplift_x; }
  intptr_t GetTVRoadUpliftYOffset() const { return m_tv_road_uplift_y; }
  intptr_t GetTVRoadUpliftZOffset() const { return m_tv_road_uplift_z; }

  // --- Free Camera Getters ---
  intptr_t GetFreecamPosXOffset() const { return m_freecam_pos_x_offset; }
  intptr_t GetFreecamPosYOffset() const { return m_freecam_pos_y_offset; }
  intptr_t GetFreecamPosZOffset() const { return m_freecam_pos_z_offset; }
  intptr_t GetFreecamQuatXOffset() const { return m_freecam_quat_x_offset; }
  intptr_t GetFreecamQuatYOffset() const { return m_freecam_quat_y_offset; }
  intptr_t GetFreecamQuatZOffset() const { return m_freecam_quat_z_offset; }
  intptr_t GetFreecamQuatWOffset() const { return m_freecam_quat_w_offset; }
  intptr_t GetFreecamMysteryFloatOffset() const { return m_freecam_internal_value_offset; }
  intptr_t GetFreecamMouseXOffset() const { return m_freecam_mouse_x_offset; }
  intptr_t GetFreecamMouseYOffset() const { return m_freecam_mouse_y_offset; }
  intptr_t GetFreecamRollOffset() const { return m_freecam_roll_offset; }
  float* GetFreeCamSpeedPtr() const { return m_pFreeCamSpeed; }
  uintptr_t* GetCameraWorldCoordinatesPtr() const { return m_pCameraWorldCoordinatesPtr; }

  // --- Debug Camera Getters ---
  uintptr_t GetDebugCameraContextPtr() const { return m_pDebugCameraContext; }
  void* GetDebugCameraModeFunc() const { return m_pfnSetDebugCameraMode; }
  void* GetSetSelectedActorFunc() const { return m_pfnSetSelectedActor; }
  void* GetSetPositionLockFunc() const { return m_pfnSetPositionLock; }
  void* GetSetRotationLockFunc() const { return m_pfnSetRotationLock; }
  void* GetSetOrbitModeFunc() const { return m_pfnSetOrbitMode; }
  uintptr_t GetCacheableCvarObjectPtr() const { return m_pCacheableCvarObject; }
  void SetCacheableCvarObjectPtr(uintptr_t ptr) { m_pCacheableCvarObject = ptr; }

  intptr_t GetCvarValueOffset() const { return m_cvarValueOffset; }
  void SetCvarValueOffset(intptr_t offset) { m_cvarValueOffset = offset; }

  intptr_t GetDebugCameraModeOffset() const { return m_debugCameraModeOffset; }
  intptr_t GetDebugPosLockOffset() const { return m_debugPosLockOffset; }
  intptr_t GetDebugRotLockOffset() const { return m_debugRotLockOffset; }
  intptr_t GetDebugOrbitOffset() const { return m_debugOrbitOffset; }
  intptr_t GetDebugSelectedObjectPtrOffset() const { return m_debugSelectedObjectPtrOffset; }
  intptr_t GetDebugOrbitSpeedOffset() const { return m_debugOrbitSpeedOffset; }
  intptr_t GetDebugHoveredObjectPtrOffset() const { return m_debugHoveredObjectPtrOffset; }

  // --- Debug Camera HUD Getters ---
  void* GetSetHudVisibilityFunc() const { return m_pfnSetHudVisibility; }
  void* GetSetDebugHudPositionFunc() const { return m_pfnSetDebugHudPosition; }
  intptr_t GetHudVisibleOffset() const { return m_hudVisibleOffset; }
  intptr_t GetHudPositionOffset() const { return m_hudPositionOffset; }
  intptr_t GetGameUiVisibleOffset() const { return m_gameUiVisibleOffset; }

  // --- Debug Camera State Getters ---
  void* GetAddCameraStateFunc() const { return m_pfnAddCameraState; }
  intptr_t GetStateContextOffset() const { return m_stateContextOffset; }
  intptr_t GetStateManagerOffset() const { return m_stateManagerOffset; }
  void* GetCycleSavedStateFunc() const { return m_pfnCycleSavedState; }
  void* GetApplyStateFunc() const { return m_pfnApplyState; }
  void* GetLoadStatesFromFileFunc() const { return m_pfnLoadStatesFromFile; }
  void* GetOpenFileForCameraStateFunc() const { return m_pfnOpenFileForCameraState; }
  void* GetFormatAndWriteCameraStateFunc() const { return m_pfnFormatAndWriteCameraState; }
  intptr_t GetStateArrayOffset() const { return m_stateArrayOffset; }
  intptr_t GetStateCountOffset() const { return m_stateCountOffset; }
  intptr_t GetStateCurrentIndexOffset() const { return m_stateCurrentIndexOffset; }

  // --- Debug Camera Animation Getters ---
  void* GetUpdateAnimatedFlightFunc() const { return m_pfnUpdateAnimatedFlight; }
  intptr_t GetAnimationTimerOffset() const { return m_animationTimerOffset; }

  // --- Public Setters (for use by ICameraDataFinder implementations)
  void SetCameraFovOffset(intptr_t val) { m_camera_fov_offset = val; }
  void SetNearPlaneOffset(intptr_t val) { m_near_plane_offset = val; }
  void SetFarPlaneOffset(intptr_t val) { m_far_plane_offset = val; }
  void SetMouseSensitivityOffset(intptr_t val) { m_mouse_sensitivity_offset = val; }
  void SetShakeAnimStepOffset(intptr_t val) { m_shake_anim_step_offset = val; }
  void SetShakeAnimScaleMinOffset(intptr_t val) { m_shake_anim_scale_min_offset = val; }
  void SetShakeAnimScaleMaxOffset(intptr_t val) { m_shake_anim_scale_max_offset = val; }
  void SetShakeAnimOffset(intptr_t val) { m_shake_anim_offset = val; }
  void SetHandShakeLimitOffset(intptr_t val) { m_hand_shake_limit_offset = val; }
  void SetHandShakeSpeedOffset(intptr_t val) { m_hand_shake_speed_offset = val; }
  void SetStandardManagerPtrAddr(uintptr_t val) { m_pStandardManagerPtrAddr = val; }
  void SetStandardManagerAdjustment(intptr_t val) { m_standardManagerAdjustment = val; }
  void SetCoreOffsetsFound(bool val) { m_coreOffsetsFound = val; }
  void SetActiveCameraIdOffset(intptr_t val) { m_activeCameraIdOffset = val; }
  void SetFreecamGlobalObjectPtr(uintptr_t* val) { m_pFreecamGlobalObjectPtr = val; }
  void SetFreecamGlobalObjectAdjustment(intptr_t val) { m_freecamGlobalObjectAdjustment = val; }
  void SetFreecamContextOffset(uintptr_t val) { m_freecamContextOffset = val; }
  void SetInteriorSeatXOffset(intptr_t val) { m_interior_seat_x_offset = val; }
  void SetInteriorSeatYOffset(intptr_t val) { m_interior_seat_y_offset = val; }
  void SetInteriorSeatZOffset(intptr_t val) { m_interior_seat_z_offset = val; }
  void SetInteriorYawOffset(intptr_t val) { m_interior_yaw_offset = val; }
  void SetInteriorPitchOffset(intptr_t val) { m_interior_pitch_offset = val; }
  void SetInteriorLimitLeftOffset(intptr_t val) { m_interior_limit_left_offset = val; }
  void SetInteriorLimitRightOffset(intptr_t val) { m_interior_limit_right_offset = val; }
  void SetInteriorLimitUpOffset(intptr_t val) { m_interior_limit_up_offset = val; }
  void SetInteriorLimitDownOffset(intptr_t val) { m_interior_limit_down_offset = val; }
  void SetInteriorOutsideOffset(intptr_t val) { m_interior_outside_offset = val; }
  // --- Shared FOV Setters (for FovDataFinder) ---
  void SetFovBaseOffset(intptr_t val) { m_fov_base_offset = val; }  // Shared
  void SetFovHorizFinalOffset(intptr_t val) { m_fov_horiz_final_offset = val; }  // Shared
  void SetFovVertFinalOffset(intptr_t val) { m_fov_vert_final_offset = val; }  // Shared

  void SetInteriorMouseLRDefaultOffset(intptr_t val) { m_interior_mouse_lr_default = val; }
  void SetInteriorMouseUDDefaultOffset(intptr_t val) { m_interior_mouse_ud_default = val; }
  void SetInteriorAzimuthOverridesOffset(intptr_t val) { m_interior_azimuth_overrides_offset = val; }
  void SetZoomFovFactorOffset(intptr_t val) { m_zoom_fov_factor_offset = val; }
  void SetZoomSpeedOffset(intptr_t val) { m_zoom_speed_offset = val; }
  void SetAzimuthRangeOutsideOffset(intptr_t val) { m_azimuth_range_outside_offset = val; }
  void SetAzimuthRangeStartAzimuthOffset(intptr_t val) { m_azimuth_range_start_azimuth_offset = val; }
  void SetAzimuthRangeEndAzimuthOffset(intptr_t val) { m_azimuth_range_end_azimuth_offset = val; }
  void SetAzimuthRangeStartUpLimitOffset(intptr_t val) { m_azimuth_range_start_up_limit_offset = val; }
  void SetAzimuthRangeEndUpLimitOffset(intptr_t val) { m_azimuth_range_end_up_limit_offset = val; }
  void SetAzimuthRangeStartDownLimitOffset(intptr_t val) { m_azimuth_range_start_down_limit_offset = val; }
  void SetAzimuthRangeEndDownLimitOffset(intptr_t val) { m_azimuth_range_end_down_limit_offset = val; }
  void SetAzimuthRangeStartUpDownDefaultOffset(intptr_t val) { m_azimuth_range_start_up_down_default_offset = val; }
  void SetAzimuthRangeEndUpDownDefaultOffset(intptr_t val) { m_azimuth_range_end_up_down_default_offset = val; }
  void SetAzimuthRangeStartLeftRightDefaultOffset(intptr_t val) { m_azimuth_range_start_left_right_default_offset = val; }
  void SetAzimuthRangeEndLeftRightDefaultOffset(intptr_t val) { m_azimuth_range_end_left_right_default_offset = val; }
  void SetAzimuthRangeStartHeadOffsetOffset(intptr_t val) { m_azimuth_range_start_head_offset_offset = val; }
  void SetAzimuthRangeEndHeadOffsetOffset(intptr_t val) { m_azimuth_range_end_head_offset_offset = val; }
  void SetCameraParamsObjectPtr(uintptr_t val) { m_pCameraParamsObject = val; }
  void SetViewportX1Offset(intptr_t val) { m_viewport_x1_offset = val; }
  void SetViewportX2Offset(intptr_t val) { m_viewport_x2_offset = val; }
  void SetViewportY1Offset(intptr_t val) { m_viewport_y1_offset = val; }
  void SetViewportY2Offset(intptr_t val) { m_viewport_y2_offset = val; }

  // --- Behind Camera Setters ---
  void SetBehindLivePitchOffset(intptr_t val) { m_behind_live_pitch_offset = val; }
  void SetBehindLiveYawOffset(intptr_t val) { m_behind_live_yaw_offset = val; }
  void SetBehindLiveZoomOffset(intptr_t val) { m_behind_live_zoom_offset = val; }
  void SetBehindDistanceMinOffset(intptr_t val) { m_behind_distance_min_offset = val; }
  void SetBehindDistanceMaxOffset(intptr_t val) { m_behind_distance_max_offset = val; }
  void SetBehindDistanceTrailerMaxOffset(intptr_t val) { m_behind_distance_trailer_max_offset = val; }
  void SetBehindDistanceDefaultOffset(intptr_t val) { m_behind_distance_default_offset = val; }
  void SetBehindDistanceTrailerDefaultOffset(intptr_t val) { m_behind_distance_trailer_default_offset = val; }
  void SetBehindDistanceChangeSpeedOffset(intptr_t val) { m_behind_distance_change_speed_offset = val; }
  void SetBehindDistanceLazinessSpeedOffset(intptr_t val) { m_behind_distance_laziness_speed_offset = val; }
  void SetBehindAzimuthLazinessSpeedOffset(intptr_t val) { m_behind_azimuth_laziness_speed_offset = val; }
  void SetBehindElevationMinOffset(intptr_t val) { m_behind_elevation_min_offset = val; }
  void SetBehindElevationMaxOffset(intptr_t val) { m_behind_elevation_max_offset = val; }
  void SetBehindElevationDefaultOffset(intptr_t val) { m_behind_elevation_default_offset = val; }
  void SetBehindElevationTrailerDefaultOffset(intptr_t val) { m_behind_elevation_trailer_default_offset = val; }
  void SetBehindHeightLimitOffset(intptr_t val) { m_behind_height_limit_offset = val; }
  void SetBehindPivotXOffset(intptr_t val) { m_behind_pivot_x_offset = val; }
  void SetBehindPivotYOffset(intptr_t val) { m_behind_pivot_y_offset = val; }
  void SetBehindPivotZOffset(intptr_t val) { m_behind_pivot_z_offset = val; }
  void SetBehindDynamicOffsetMaxOffset(intptr_t val) { m_behind_dynamic_offset_max_offset = val; }
  void SetBehindDynamicOffsetSpeedMinOffset(intptr_t val) { m_behind_dynamic_offset_speed_min_offset = val; }
  void SetBehindDynamicOffsetSpeedMaxOffset(intptr_t val) { m_behind_dynamic_offset_speed_max_offset = val; }
  void SetBehindDynamicOffsetLazinessSpeedOffset(intptr_t val) { m_behind_dynamic_offset_laziness_speed_offset = val; }

  // --- Behind Camera Validation Setters ---
  void SetBehindValidationOffset(intptr_t val) { m_behind_validation_offset = val; }
  void SetBehindValidationSpeedPositiveOffset(intptr_t val) { m_behind_validation_speed_positive_offset = val; }
  void SetBehindValidationSpeedNegativeOffset(intptr_t val) { m_behind_validation_speed_negative_offset = val; }
  void SetBehindValidationRadiusOffset(intptr_t val) { m_behind_validation_radius_offset = val; }
  void SetBehindSpeedFovChangeFactorOffset(intptr_t val) { m_behind_speed_fov_change_factor_offset = val; }

  // --- Top Camera Setters ---
  void SetTopMinHeightOffset(intptr_t val) { m_top_min_height_offset = val; }
  void SetTopMaxHeightOffset(intptr_t val) { m_top_max_height_offset = val; }
  void SetTopSpeedOffset(intptr_t val) { m_top_speed_offset = val; }
  void SetTopXOffsetForwardOffset(intptr_t val) { m_top_x_offset_forward_offset = val; }
  void SetTopXOffsetBackwardOffset(intptr_t val) { m_top_x_offset_backward_offset = val; }

  // --- Window Camera Setters ---
  void SetWindowHeadOffsetXOffset(intptr_t val) { m_window_head_offset_x = val; }
  void SetWindowHeadOffsetYOffset(intptr_t val) { m_window_head_offset_y = val; }
  void SetWindowHeadOffsetZOffset(intptr_t val) { m_window_head_offset_z = val; }
  void SetWindowLiveYawOffset(intptr_t val) { m_window_live_yaw = val; }
  void SetWindowLivePitchOffset(intptr_t val) { m_window_live_pitch = val; }
  void SetWindowMouseLeftLimitOffset(intptr_t val) { m_window_mouse_left_limit = val; }
  void SetWindowMouseRightLimitOffset(intptr_t val) { m_window_mouse_right_limit = val; }
  void SetWindowMouseLRDefaultOffset(intptr_t val) { m_window_mouse_lr_default = val; }
  void SetWindowMouseUpLimitOffset(intptr_t val) { m_window_mouse_up_limit = val; }
  void SetWindowMouseDownLimitOffset(intptr_t val) { m_window_mouse_down_limit = val; }
  void SetWindowMouseUDDefaultOffset(intptr_t val) { m_window_mouse_ud_default = val; }

  // --- Bumper Camera Setters ---
  void SetBumperOffsetXOffset(intptr_t val) { m_bumper_offset_x = val; }
  void SetBumperOffsetYOffset(intptr_t val) { m_bumper_offset_y = val; }
  void SetBumperOffsetZOffset(intptr_t val) { m_bumper_offset_z = val; }

  // --- Wheel Camera Setters ---
  void SetWheelOffsetXOffset(intptr_t val) { m_wheel_offset_x = val; }
  void SetWheelOffsetYOffset(intptr_t val) { m_wheel_offset_y = val; }
  void SetWheelOffsetZOffset(intptr_t val) { m_wheel_offset_z = val; }

  // --- TV Camera Setters ---
  void SetTVMaxDistanceOffset(intptr_t val) { m_tv_max_distance = val; }
  void SetTVPrefabUpliftXOffset(intptr_t val) { m_tv_prefab_uplift_x = val; }
  void SetTVPrefabUpliftYOffset(intptr_t val) { m_tv_prefab_uplift_y = val; }
  void SetTVPrefabUpliftZOffset(intptr_t val) { m_tv_prefab_uplift_z = val; }
  void SetTVRoadUpliftXOffset(intptr_t val) { m_tv_road_uplift_x = val; }
  void SetTVRoadUpliftYOffset(intptr_t val) { m_tv_road_uplift_y = val; }
  void SetTVRoadUpliftZOffset(intptr_t val) { m_tv_road_uplift_z = val; }

  // --- Free Camera Setters ---
  void SetFreecamPosXOffset(intptr_t val) { m_freecam_pos_x_offset = val; }
  void SetFreecamPosYOffset(intptr_t val) { m_freecam_pos_y_offset = val; }
  void SetFreecamPosZOffset(intptr_t val) { m_freecam_pos_z_offset = val; }
  void SetFreecamQuatXOffset(intptr_t val) { m_freecam_quat_x_offset = val; }
  void SetFreecamQuatYOffset(intptr_t val) { m_freecam_quat_y_offset = val; }
  void SetFreecamQuatZOffset(intptr_t val) { m_freecam_quat_z_offset = val; }
  void SetFreecamQuatWOffset(intptr_t val) { m_freecam_quat_w_offset = val; }
  void SetFreecamMysteryFloatOffset(intptr_t val) { m_freecam_internal_value_offset = val; }
  void SetFreecamMouseXOffset(intptr_t val) { m_freecam_mouse_x_offset = val; }
  void SetFreecamMouseYOffset(intptr_t val) { m_freecam_mouse_y_offset = val; }
  void SetFreecamRollOffset(intptr_t val) { m_freecam_roll_offset = val; }
  void SetFreeCamSpeedPtr(float* val) { m_pFreeCamSpeed = val; }
  void SetCameraWorldCoordinatesPtr(uintptr_t* val) { m_pCameraWorldCoordinatesPtr = val; }

  // --- Debug Camera Setters ---
  void SetDebugCameraContextPtr(uintptr_t val) { m_pDebugCameraContext = val; }
  void SetDebugCameraModeFunc(void* val) { m_pfnSetDebugCameraMode = val; }
  void SetSetSelectedActorFunc(void* val) { m_pfnSetSelectedActor = val; }
  void SetSetPositionLockFunc(void* val) { m_pfnSetPositionLock = val; }
  void SetSetRotationLockFunc(void* val) { m_pfnSetRotationLock = val; }
  void SetSetOrbitModeFunc(void* val) { m_pfnSetOrbitMode = val; }

  void SetDebugCameraModeOffset(intptr_t val) { m_debugCameraModeOffset = val; }
  void SetDebugPosLockOffset(intptr_t val) { m_debugPosLockOffset = val; }
  void SetDebugRotLockOffset(intptr_t val) { m_debugRotLockOffset = val; }
  void SetDebugOrbitOffset(intptr_t val) { m_debugOrbitOffset = val; }
  void SetDebugSelectedObjectPtrOffset(intptr_t val) { m_debugSelectedObjectPtrOffset = val; }
  void SetDebugOrbitSpeedOffset(intptr_t val) { m_debugOrbitSpeedOffset = val; }
  void SetDebugHoveredObjectPtrOffset(intptr_t val) { m_debugHoveredObjectPtrOffset = val; }

  // --- Debug Camera HUD Setters ---
  void SetSetHudVisibilityFunc(void* val) { m_pfnSetHudVisibility = val; }
  void SetSetDebugHudPositionFunc(void* val) { m_pfnSetDebugHudPosition = val; }
  void SetHudVisibleOffset(intptr_t val) { m_hudVisibleOffset = val; }
  void SetHudPositionOffset(intptr_t val) { m_hudPositionOffset = val; }
  void SetGameUiVisibleOffset(intptr_t val) { m_gameUiVisibleOffset = val; }

  // --- Debug Camera State Setters ---
  void SetAddCameraStateFunc(void* val) { m_pfnAddCameraState = val; }
  void SetStateContextOffset(intptr_t val) { m_stateContextOffset = val; }
  void SetStateManagerOffset(intptr_t val) { m_stateManagerOffset = val; }
  void SetCycleSavedStateFunc(void* val) { m_pfnCycleSavedState = val; }
  void SetApplyStateFunc(void* val) { m_pfnApplyState = val; }
  void SetLoadStatesFromFileFunc(void* val) { m_pfnLoadStatesFromFile = val; }
  void SetOpenFileForCameraStateFunc(void* val) { m_pfnOpenFileForCameraState = val; }
  void SetFormatAndWriteCameraStateFunc(void* val) { m_pfnFormatAndWriteCameraState = val; }
  void SetStateArrayOffset(intptr_t val) { m_stateArrayOffset = val; }
  void SetStateCountOffset(intptr_t val) { m_stateCountOffset = val; }
  void SetStateCurrentIndexOffset(intptr_t val) { m_stateCurrentIndexOffset = val; }

  // --- Debug Camera Animation Data ---
  void* m_pfnUpdateAnimatedFlight = nullptr;
  intptr_t m_animationTimerOffset = 0;

  void SetUpdateAnimatedFlightFunc(void* val) { m_pfnUpdateAnimatedFlight = val; }
  void SetAnimationTimerOffset(intptr_t val) { m_animationTimerOffset = val; }

 private:
  GameDataCameraService();
  ~GameDataCameraService() = default;

  void RegisterFinders();

  // --- Runtime State ---
  bool m_isInitialized = false;
  bool m_coreOffsetsFound = false;
  std::vector<std::unique_ptr<ICameraDataFinder>> m_dataFinders;

  // --- Core Camera Data ---
  uintptr_t m_pStandardManagerPtrAddr = 0;
  intptr_t m_standardManagerAdjustment = 0; // v1.59: stores the adjustment (e.g. -0x10)
  intptr_t m_activeCameraIdOffset = 0;
  uintptr_t* m_pFreecamGlobalObjectPtr = nullptr;
  intptr_t m_freecamGlobalObjectAdjustment = 0;
  uintptr_t m_freecamContextOffset = 0;

  // --- Interior Camera Offsets ---
  intptr_t m_camera_fov_offset = 0;
  intptr_t m_near_plane_offset = 0;
  intptr_t m_far_plane_offset = 0;
  intptr_t m_mouse_sensitivity_offset = 0;
  intptr_t m_shake_anim_step_offset = 0;
  intptr_t m_shake_anim_scale_min_offset = 0;
  intptr_t m_shake_anim_scale_max_offset = 0;
  intptr_t m_shake_anim_offset = 0;
  intptr_t m_hand_shake_limit_offset = 0;
  intptr_t m_hand_shake_speed_offset = 0;
  intptr_t m_interior_seat_x_offset = 0;
  intptr_t m_interior_seat_y_offset = 0;
  intptr_t m_interior_seat_z_offset = 0;
  intptr_t m_interior_yaw_offset = 0;
  intptr_t m_interior_pitch_offset = 0;
  intptr_t m_interior_limit_left_offset = 0;
  intptr_t m_interior_limit_right_offset = 0;
  intptr_t m_interior_limit_up_offset = 0;
  intptr_t m_interior_limit_down_offset = 0;
  intptr_t m_interior_outside_offset = 0;
  intptr_t m_fov_base_offset = 0;
  intptr_t m_fov_horiz_final_offset = 0;
  intptr_t m_fov_vert_final_offset = 0;
  intptr_t m_interior_mouse_lr_default = 0;
  intptr_t m_interior_mouse_ud_default = 0;
  intptr_t m_interior_azimuth_overrides_offset = 0;
  intptr_t m_azimuth_range_outside_offset = 0;
  intptr_t m_azimuth_range_start_azimuth_offset = 0;
  intptr_t m_azimuth_range_end_azimuth_offset = 0;
  intptr_t m_azimuth_range_start_up_limit_offset = 0;
  intptr_t m_azimuth_range_end_up_limit_offset = 0;
  intptr_t m_azimuth_range_start_down_limit_offset = 0;
  intptr_t m_azimuth_range_end_down_limit_offset = 0;
  intptr_t m_azimuth_range_start_up_down_default_offset = 0;
  intptr_t m_azimuth_range_end_up_down_default_offset = 0;
  intptr_t m_azimuth_range_start_left_right_default_offset = 0;
  intptr_t m_azimuth_range_end_left_right_default_offset = 0;
  intptr_t m_azimuth_range_start_head_offset_offset = 0;
  intptr_t m_azimuth_range_end_head_offset_offset = 0;
  intptr_t m_zoom_fov_factor_offset = 0;
  intptr_t m_zoom_speed_offset = 0;

  // --- Free Camera Offsets ---
  intptr_t m_freecam_pos_x_offset = 0;
  intptr_t m_freecam_pos_y_offset = 0;
  intptr_t m_freecam_pos_z_offset = 0;
  intptr_t m_freecam_quat_x_offset = 0;
  intptr_t m_freecam_quat_y_offset = 0;
  intptr_t m_freecam_quat_z_offset = 0;
  intptr_t m_freecam_quat_w_offset = 0;
  intptr_t m_freecam_internal_value_offset = 0;
  intptr_t m_freecam_mouse_x_offset = 0;
  intptr_t m_freecam_mouse_y_offset = 0;
  intptr_t m_freecam_roll_offset = 0;
  float* m_pFreeCamSpeed = nullptr;

  // --- World Coordinates Data ---
  uintptr_t* m_pCameraWorldCoordinatesPtr = nullptr;

  // --- Viewport / Projection Data ---
  uintptr_t m_pCameraParamsObject = 0;
  intptr_t m_viewport_x1_offset = 0;
  intptr_t m_viewport_x2_offset = 0;
  intptr_t m_viewport_y1_offset = 0;
  intptr_t m_viewport_y2_offset = 0;

  // --- Behind Camera Offsets ---
  intptr_t m_behind_live_pitch_offset = 0;
  intptr_t m_behind_live_yaw_offset = 0;
  intptr_t m_behind_live_zoom_offset = 0;
  intptr_t m_behind_distance_min_offset = 0;
  intptr_t m_behind_distance_max_offset = 0;
  intptr_t m_behind_distance_trailer_max_offset = 0;
  intptr_t m_behind_distance_default_offset = 0;
  intptr_t m_behind_distance_trailer_default_offset = 0;
  intptr_t m_behind_distance_change_speed_offset = 0;
  intptr_t m_behind_distance_laziness_speed_offset = 0;
  intptr_t m_behind_azimuth_laziness_speed_offset = 0;
  intptr_t m_behind_elevation_min_offset = 0;
  intptr_t m_behind_elevation_max_offset = 0;
  intptr_t m_behind_elevation_default_offset = 0;
  intptr_t m_behind_elevation_trailer_default_offset = 0;
  intptr_t m_behind_height_limit_offset = 0;
  intptr_t m_behind_pivot_x_offset = 0;
  intptr_t m_behind_pivot_y_offset = 0;
  intptr_t m_behind_pivot_z_offset = 0;
  intptr_t m_behind_dynamic_offset_max_offset = 0;
  intptr_t m_behind_dynamic_offset_speed_min_offset = 0;
  intptr_t m_behind_dynamic_offset_speed_max_offset = 0;
  intptr_t m_behind_dynamic_offset_laziness_speed_offset = 0;
  intptr_t m_behind_validation_offset = 0;
  intptr_t m_behind_validation_speed_positive_offset = 0;
  intptr_t m_behind_validation_speed_negative_offset = 0;
  intptr_t m_behind_validation_radius_offset = 0;
  intptr_t m_behind_speed_fov_change_factor_offset = 0;

  // --- Top Camera Offsets ---
  intptr_t m_top_min_height_offset = 0;
  intptr_t m_top_max_height_offset = 0;
  intptr_t m_top_speed_offset = 0;
  intptr_t m_top_x_offset_forward_offset = 0;
  intptr_t m_top_x_offset_backward_offset = 0;

  // --- Window Camera Offsets ---
  intptr_t m_window_head_offset_x = 0;
  intptr_t m_window_head_offset_y = 0;
  intptr_t m_window_head_offset_z = 0;
  intptr_t m_window_live_yaw = 0;
  intptr_t m_window_live_pitch = 0;
  intptr_t m_window_mouse_left_limit = 0;
  intptr_t m_window_mouse_right_limit = 0;
  intptr_t m_window_mouse_lr_default = 0;
  intptr_t m_window_mouse_up_limit = 0;
  intptr_t m_window_mouse_down_limit = 0;
  intptr_t m_window_mouse_ud_default = 0;

  // --- Bumper Camera Offsets ---
  intptr_t m_bumper_offset_x = 0;
  intptr_t m_bumper_offset_y = 0;
  intptr_t m_bumper_offset_z = 0;

  // --- Wheel Camera Offsets ---
  intptr_t m_wheel_offset_x = 0;
  intptr_t m_wheel_offset_y = 0;
  intptr_t m_wheel_offset_z = 0;

  // --- TV Camera Offsets ---
  intptr_t m_tv_max_distance = 0;
  intptr_t m_tv_prefab_uplift_x = 0;
  intptr_t m_tv_prefab_uplift_y = 0;
  intptr_t m_tv_prefab_uplift_z = 0;
  intptr_t m_tv_road_uplift_x = 0;
  intptr_t m_tv_road_uplift_y = 0;
  intptr_t m_tv_road_uplift_z = 0;

  // --- Debug Camera Data ---
  uintptr_t m_pDebugCameraContext = 0;
  void* m_pfnSetDebugCameraMode = nullptr;
  void* m_pfnSetSelectedActor = nullptr;
  void* m_pfnSetPositionLock = nullptr;
  void* m_pfnSetRotationLock = nullptr;
  void* m_pfnSetOrbitMode = nullptr;
  uintptr_t m_pCacheableCvarObject = 0;
  intptr_t m_cvarValueOffset = 0;
  intptr_t m_debugCameraModeOffset = 0;
  intptr_t m_debugPosLockOffset = 0;
  intptr_t m_debugRotLockOffset = 0;
  intptr_t m_debugOrbitOffset = 0;
  intptr_t m_debugSelectedObjectPtrOffset = 0;
  intptr_t m_debugOrbitSpeedOffset = 0;
  intptr_t m_debugHoveredObjectPtrOffset = 0;

  // --- Debug Camera HUD Data ---
  void* m_pfnSetHudVisibility = nullptr;
  void* m_pfnSetDebugHudPosition = nullptr;
  intptr_t m_hudVisibleOffset = 0;
  intptr_t m_hudPositionOffset = 0;
  intptr_t m_gameUiVisibleOffset = 0;

  // --- Debug Camera State Data ---
  void* m_pfnAddCameraState = nullptr;
  intptr_t m_stateContextOffset = 0;
  intptr_t m_stateManagerOffset = 0;
  void* m_pfnCycleSavedState = nullptr;
  void* m_pfnApplyState = nullptr;
  void* m_pfnLoadStatesFromFile = nullptr;
  void* m_pfnOpenFileForCameraState = nullptr;
  void* m_pfnFormatAndWriteCameraState = nullptr;
  intptr_t m_stateArrayOffset = 0;
  intptr_t m_stateCountOffset = 0;
  intptr_t m_stateCurrentIndexOffset = 0;
};

}  // namespace Data::GameData
SPF_NS_END
