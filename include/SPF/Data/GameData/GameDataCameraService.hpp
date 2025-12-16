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
  uintptr_t GetStandardManagerPtrAddr() const { return m_pStandardManagerPtrAddr; }
  intptr_t GetActiveCameraIdOffset() const { return m_activeCameraIdOffset; }
  uintptr_t* GetFreecamGlobalObjectPtr() const { return m_pFreecamGlobalObjectPtr; }
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
  intptr_t GetFovBaseOffset() const { return m_fov_base_offset; }
  intptr_t GetFovHorizFinalOffset() const { return m_fov_horiz_final_offset; }
  intptr_t GetFovVertFinalOffset() const { return m_fov_vert_final_offset; }
  intptr_t GetInteriorMouseLRDefaultOffset() const { return m_interior_mouse_lr_default; }
  intptr_t GetInteriorMouseUDDefaultOffset() const { return m_interior_mouse_ud_default; }
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
  intptr_t GetFreecamMysteryFloatOffset() const { return m_freecam_mystery_float_offset; }
  intptr_t GetFreecamMouseXOffset() const { return m_freecam_mouse_x_offset; }
  intptr_t GetFreecamMouseYOffset() const { return m_freecam_mouse_y_offset; }
  intptr_t GetFreecamRollOffset() const { return m_freecam_roll_offset; }
  float* GetFreeCamSpeedPtr() const { return m_pFreeCamSpeed; }
  uintptr_t* GetCameraWorldCoordinatesPtr() const { return m_pCameraWorldCoordinatesPtr; }

  // --- Debug Camera Getters ---
  uintptr_t GetDebugCameraContextPtr() const { return m_pDebugCameraContext; }
  void* GetDebugCameraModeFunc() const { return m_pfnSetDebugCameraMode; }
  uintptr_t GetCacheableCvarObjectPtr() const { return m_pCacheableCvarObject; }
  void SetCacheableCvarObjectPtr(uintptr_t ptr) { m_pCacheableCvarObject = ptr; }

  intptr_t GetCvarValueOffset() const { return m_cvarValueOffset; }
  void SetCvarValueOffset(intptr_t offset) { m_cvarValueOffset = offset; }

  intptr_t GetDebugCameraModeOffset() const { return m_debugCameraModeOffset; }

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

  // --- Public Setters (for use by ICameraDataFinder implementations) ---
  void SetStandardManagerPtrAddr(uintptr_t val) { m_pStandardManagerPtrAddr = val; }
  void SetCoreOffsetsFound(bool val) { m_coreOffsetsFound = val; }
  void SetActiveCameraIdOffset(intptr_t val) { m_activeCameraIdOffset = val; }
  void SetFreecamGlobalObjectPtr(uintptr_t* val) { m_pFreecamGlobalObjectPtr = val; }
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
  // --- Shared FOV Setters (for FovDataFinder) ---
  void SetFovBaseOffset(intptr_t val) { m_fov_base_offset = val; }  // Shared
  void SetFovHorizFinalOffset(intptr_t val) { m_fov_horiz_final_offset = val; }  // Shared
  void SetFovVertFinalOffset(intptr_t val) { m_fov_vert_final_offset = val; }  // Shared

  void SetInteriorMouseLRDefaultOffset(intptr_t val) { m_interior_mouse_lr_default = val; }
  void SetInteriorMouseUDDefaultOffset(intptr_t val) { m_interior_mouse_ud_default = val; }
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
  void SetFreecamMysteryFloatOffset(intptr_t val) { m_freecam_mystery_float_offset = val; }
  void SetFreecamMouseXOffset(intptr_t val) { m_freecam_mouse_x_offset = val; }
  void SetFreecamMouseYOffset(intptr_t val) { m_freecam_mouse_y_offset = val; }
  void SetFreecamRollOffset(intptr_t val) { m_freecam_roll_offset = val; }
  void SetFreeCamSpeedPtr(float* val) { m_pFreeCamSpeed = val; }
  void SetCameraWorldCoordinatesPtr(uintptr_t* val) { m_pCameraWorldCoordinatesPtr = val; }

  // --- Debug Camera Setters ---
  void SetDebugCameraContextPtr(uintptr_t val) { m_pDebugCameraContext = val; }
  void SetDebugCameraModeFunc(void* val) { m_pfnSetDebugCameraMode = val; }

  void SetDebugCameraModeOffset(intptr_t val) { m_debugCameraModeOffset = val; }

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

  // --- Debug Camera Animation Setters ---
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
  intptr_t m_activeCameraIdOffset = 0;
  uintptr_t* m_pFreecamGlobalObjectPtr = nullptr;
  uintptr_t m_freecamContextOffset = 0;

  // --- Interior Camera Offsets ---
  intptr_t m_interior_seat_x_offset = 0;
  intptr_t m_interior_seat_y_offset = 0;
  intptr_t m_interior_seat_z_offset = 0;
  intptr_t m_interior_yaw_offset = 0;
  intptr_t m_interior_pitch_offset = 0;
  intptr_t m_interior_limit_left_offset = 0;
  intptr_t m_interior_limit_right_offset = 0;
  intptr_t m_interior_limit_up_offset = 0;
  intptr_t m_interior_limit_down_offset = 0;
  intptr_t m_fov_base_offset = 0;
  intptr_t m_fov_horiz_final_offset = 0;
  intptr_t m_fov_vert_final_offset = 0;
  intptr_t m_interior_mouse_lr_default = 0;
  intptr_t m_interior_mouse_ud_default = 0;

  // --- Free Camera Offsets ---
  intptr_t m_freecam_pos_x_offset = 0;
  intptr_t m_freecam_pos_y_offset = 0;
  intptr_t m_freecam_pos_z_offset = 0;
  intptr_t m_freecam_quat_x_offset = 0;
  intptr_t m_freecam_quat_y_offset = 0;
  intptr_t m_freecam_quat_z_offset = 0;
  intptr_t m_freecam_quat_w_offset = 0;
  intptr_t m_freecam_mystery_float_offset = 0;
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
  uintptr_t m_pCacheableCvarObject = 0;
  intptr_t m_cvarValueOffset = 0;
  intptr_t m_debugCameraModeOffset = 0;

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

  // --- Debug Camera Animation Data ---
  void* m_pfnUpdateAnimatedFlight = nullptr;
  intptr_t m_animationTimerOffset = 0;
};

}  // namespace Data::GameData
SPF_NS_END
