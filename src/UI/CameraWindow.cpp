#include "SPF/UI/CameraWindow.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include <string>
#include <vector>
#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/GameCamera/GameCameraInterior.hpp"
#include "SPF/GameCamera/GameCameraBehind.hpp"
#include "SPF/GameCamera/GameCameraTop.hpp"
#include "SPF/GameCamera/GameCameraCabin.hpp"
#include "SPF/GameCamera/GameCameraWindow.hpp"
#include "SPF/GameCamera/GameCameraBumper.hpp"
#include "SPF/GameCamera/GameCameraWheel.hpp"
#include "SPF/GameCamera/GameCameraTV.hpp"
#include "SPF/GameCamera/GameCameraFree.hpp"
#include "SPF/GameCamera/GameCameraPhoto.hpp"
#include "SPF/GameCamera/DebugCameraMode.hpp"
#include "SPF/GameCamera/DebugHudPosition.hpp"
#include "SPF/Utils/Vec3.hpp"
#include "imgui.h"

using namespace SPF::GameCamera;
using namespace SPF::Utils;
using namespace SPF::Data::GameData;
using namespace SPF::Localization;

SPF_NS_BEGIN
namespace UI {
namespace {
const char* DebugCameraModeToString(GameCamera::DebugCameraMode mode) {
  switch (mode) {
    case GameCamera::DebugCameraMode::SIMPLE:
      return "Simple";
    case GameCamera::DebugCameraMode::VIDEO:
      return "Video";
    case GameCamera::DebugCameraMode::TRAFFIC:
      return "Traffic";
    case GameCamera::DebugCameraMode::CINEMATIC:
      return "Cinematic";
    case GameCamera::DebugCameraMode::ANIMATED:
      return "Animated";
    case GameCamera::DebugCameraMode::OVERSIZE:
      return "Oversize";
    default:
      return "Unknown";
  }
}
const char* DebugHudPositionToString(GameCamera::DebugHudPosition pos) {
  switch (pos) {
    case GameCamera::DebugHudPosition::TopLeft:
      return "Top-Left";
    case GameCamera::DebugHudPosition::TopRight:
      return "Top-Right";
    case GameCamera::DebugHudPosition::BottomLeft:
      return "Bottom-Left";
    case GameCamera::DebugHudPosition::BottomRight:
      return "Bottom-Right";
    default:
      return "Unknown";
  }
}
const char* GameCameraTypeToString(GameCamera::GameCameraType type) {
  switch (type) {
    case GameCamera::GameCameraType::DeveloperFreeCamera:
      return "Developer Free";
    case GameCamera::GameCameraType::BehindCamera:
      return "Behind";
    case GameCamera::GameCameraType::InteriorCamera:
      return "Interior";
    case GameCamera::GameCameraType::BumperCamera:
      return "Bumper";
    case GameCamera::GameCameraType::WindowCamera:
      return "Window";
    case GameCamera::GameCameraType::CabinCamera:
      return "Cabin";
    case GameCamera::GameCameraType::WheelCamera:
      return "Wheel";
    case GameCamera::GameCameraType::TopCamera:
      return "Top";
    case GameCamera::GameCameraType::TVCamera:
      return "TV";
    default:
      return "Unknown";
  }
}
}  // namespace

CameraWindow::CameraWindow(const std::string& owner, const std::string& name, GameCamera::GameCameraManager& gameCameraService)
    : BaseWindow(owner, name), m_gameCameraService(gameCameraService) {
    m_titleLocalizationKey = "camera_window.title";
    m_locCurrentCamera = "camera_window.current_camera";
    m_locCameraWorldCoordinates = "camera_window.camera_world_coordinates";
    m_locCameraWorldCoordinatesNotFound = "camera_window.camera_world_coordinates_not_found";
    m_locDataNotFound = "camera_window.data_not_found";
    m_locSelectCamera = "camera_window.select_camera";
    m_locInterior = "camera_window.interior";
    m_locBehind = "camera_window.behind";
    m_locPhoto = "camera_window.photo";
    m_locTop = "camera_window.top";
    m_locCabin = "camera_window.cabin";
    m_locWindow = "camera_window.window";
    m_locBumper = "camera_window.bumper";
    m_locWheel = "camera_window.wheel";
    m_locTV = "camera_window.tv";
    m_locDeveloperFreeCamera = "camera_window.developer_free_camera";
    m_locTabInteriorCamera = "camera_window.tabs.interior_camera";
    m_locTabBehindCamera = "camera_window.tabs.behind_camera";
    m_locTabTopCamera = "camera_window.tabs.top_camera";
    m_locTabPhotoCamera = "camera_window.tabs.photo_camera";
    m_locTabCabinCamera = "camera_window.tabs.cabin_camera";

    m_locTabWindowCamera = "camera_window.tabs.window_camera";
    m_locTabBumperCamera = "camera_window.tabs.bumper_camera";
    m_locTabWheelCamera = "camera_window.tabs.wheel_camera";
    m_locTabTVCamera = "camera_window.tabs.tv_camera";
    m_locTabFreeCamera = "camera_window.tabs.free_camera";
    m_locTabDebug = "camera_window.tabs.debug";

    // Photo Camera keys
    // m_locPhotoLiveState = "camera_window.photo_camera.live_state";
    // m_locPhotoLivePitch = "camera_window.photo_camera.live_pitch";
    // m_locPhotoLiveYaw = "camera_window.photo_camera.live_yaw";
    // m_locPhotoLiveRoll = "camera_window.photo_camera.live_roll";
    // m_locPhotoLiveZoom = "camera_window.photo_camera.live_zoom";
    // m_locPhotoPosition = "camera_window.photo_camera.position";
    // m_locPhotoBaseFov = "camera_window.photo_camera.base_fov";
    // m_locPhotoNotAvailable = "camera_window.photo_camera.not_available";
    // m_locPhotoFovZoom = "camera_window.photo_camera.fov_zoom";

    m_locFovZoom = "camera_window.interior_camera.fov_zoom";
    m_locBaseFov = "camera_window.interior_camera.base_fov";
    m_locBaseFovNotFound = "camera_window.interior_camera.base_fov_not_found";
    m_locFinalHFov = "camera_window.interior_camera.final_h_fov";
    m_locFinalVFov = "camera_window.interior_camera.final_v_fov";
    m_locFinalFovNotFound = "camera_window.interior_camera.final_fov_not_found";
    m_locSeatPosition = "camera_window.interior_camera.seat_position";
    m_locSeatLr = "camera_window.interior_camera.seat_lr";
    m_locSeatUd = "camera_window.interior_camera.seat_ud";
    m_locSeatFb = "camera_window.interior_camera.seat_fb";
    m_locHeadRotation = "camera_window.interior_camera.head_rotation";
    m_locYawLr = "camera_window.interior_camera.yaw_lr";
    m_locPitchUd = "camera_window.interior_camera.pitch_ud";
    m_locMouseRotationLimits = "camera_window.interior_camera.mouse_rotation_limits";
    m_locLeftLimit = "camera_window.interior_camera.left_limit";
    m_locRightLimit = "camera_window.interior_camera.right_limit";
    m_locUpLimit = "camera_window.interior_camera.up_limit";
    m_locDownLimit = "camera_window.interior_camera.down_limit";
    m_locRotationDefaults = "camera_window.interior_camera.rotation_defaults";
    m_locDefaultLr = "camera_window.interior_camera.default_lr";
    m_locDefaultUd = "camera_window.interior_camera.default_ud";
    m_locResetToDefaults = "camera_window.interior_camera.reset_to_defaults";
    m_locInteriorCameraNotAvailable = "camera_window.interior_camera.not_available";
    m_locDefaultValuePrefix = "camera_window.default_value_prefix";

    m_locNearPlane = "camera_window.interior_camera.near_plane";
    m_locFarPlane = "camera_window.interior_camera.far_plane";
    m_locMouseSensitivity = "camera_window.interior_camera.mouse_sensitivity";
    m_locShakeAnimStep = "camera_window.interior_camera.shake_anim_step";
    m_locShakeAnimScaleMin = "camera_window.interior_camera.shake_anim_scale_min";
    m_locShakeAnimScaleMax = "camera_window.interior_camera.shake_anim_scale_max";
    m_locHandShakeLimit = "camera_window.interior_camera.hand_shake_limit";
    m_locHandShakeSpeed = "camera_window.interior_camera.hand_shake_speed";
    m_locZoomFovFactor = "camera_window.interior_camera.zoom_fov_factor";
    m_locZoomSpeedInterior = "camera_window.interior_camera.zoom_speed_interior";
    m_locAzimuthOverrides = "camera_window.interior_camera.azimuth_overrides";
    m_locRangeStartAzimuth = "camera_window.interior_camera.range_start_azimuth";
    m_locRangeEndAzimuth = "camera_window.interior_camera.range_end_azimuth";
    m_locZoneIsOutside = "camera_window.interior_camera.zone_is_outside";
    m_locStartUpLimit = "camera_window.interior_camera.start_up_limit";
    m_locEndUpLimit = "camera_window.interior_camera.end_up_limit";
    m_locStartDownLimit = "camera_window.interior_camera.start_down_limit";
    m_locEndDownLimit = "camera_window.interior_camera.end_down_limit";
    m_locStartUpDownDefault = "camera_window.interior_camera.start_up_down_default";
    m_locEndUpDownDefault = "camera_window.interior_camera.end_up_down_default";
    m_locStartLeftRightDefault = "camera_window.interior_camera.start_left_right_default";
    m_locEndLeftRightDefault = "camera_window.interior_camera.end_left_right_default";
    m_locStartHeadOffset = "camera_window.interior_camera.start_head_offset";
    m_locEndHeadOffset = "camera_window.interior_camera.end_head_offset";
    m_locShakeAnimationArray = "camera_window.interior_camera.shake_animation_array";
    m_locPointX = "camera_window.interior_camera.point_x";
    m_locPointY = "camera_window.interior_camera.point_y";
    m_locPointZ = "camera_window.interior_camera.point_z";
    m_locSelectRange = "camera_window.interior_camera.select_range";
    m_locSelectFrame = "camera_window.interior_camera.select_frame";
    m_locAdvancedCoreSettings = "camera_window.interior_camera.advanced_core_settings";
    m_locShakeSettings = "camera_window.interior_camera.shake_settings";
    m_locInteriorLogicSettings = "camera_window.interior_camera.interior_logic_settings";
    m_locAzimuthRangeDetails = "camera_window.interior_camera.azimuth_range_details";

    m_locLiveState = "camera_window.behind_camera.live_state";
    m_locLivePitch = "camera_window.behind_camera.live_pitch";
    m_locLiveYaw = "camera_window.behind_camera.live_yaw";
    m_locLiveZoom = "camera_window.behind_camera.live_zoom";
    m_locLiveStateNotFound = "camera_window.behind_camera.live_state_not_found";
    m_locDistanceZoomSettings = "camera_window.behind_camera.distance_zoom_settings";
    m_locMinDistance = "camera_window.behind_camera.min_distance";
    m_locMaxDistance = "camera_window.behind_camera.max_distance";
    m_locTrailerMaxOffset = "camera_window.behind_camera.trailer_max_offset";
    m_locDefaultDistance = "camera_window.behind_camera.default_distance";
    m_locTrailerDefaultDist = "camera_window.behind_camera.trailer_default_dist";
    m_locZoomSpeed = "camera_window.behind_camera.zoom_speed";
    m_locDistanceLaziness = "camera_window.behind_camera.distance_laziness";
    m_locDistanceZoomSettingsNotFound = "camera_window.behind_camera.distance_zoom_settings_not_found";
    m_locElevationPitchSettings = "camera_window.behind_camera.elevation_pitch_settings";
    m_locAzimuthLaziness = "camera_window.behind_camera.azimuth_laziness";
    m_locMinElevation = "camera_window.behind_camera.min_elevation";
    m_locMaxElevation = "camera_window.behind_camera.max_elevation";
    m_locDefaultElevation = "camera_window.behind_camera.default_elevation";
    m_locTrailerDefaultElev = "camera_window.behind_camera.trailer_default_elev";
    m_locHeightLimit = "camera_window.behind_camera.height_limit";
    m_locElevationPitchSettingsNotFound = "camera_window.behind_camera.elevation_pitch_settings_not_found";
    m_locPivotOffset = "camera_window.behind_camera.pivot_offset";
    m_locPivotX = "camera_window.behind_camera.pivot_x";
    m_locPivotY = "camera_window.behind_camera.pivot_y";
    m_locPivotZ = "camera_window.behind_camera.pivot_z";
    m_locPivotOffsetNotFound = "camera_window.behind_camera.pivot_offset_not_found";
    m_locDynamicOffset = "camera_window.behind_camera.dynamic_offset";
    m_locMaxDynamicOffset = "camera_window.behind_camera.max_dynamic_offset";
    m_locDynOffsetSpeedMin = "camera_window.behind_camera.dyn_offset_speed_min";
    m_locDynOffsetSpeedMax = "camera_window.behind_camera.dyn_offset_speed_max";
    m_locDynOffsetLaziness = "camera_window.behind_camera.dyn_offset_laziness";
    m_locDynamicOffsetNotFound = "camera_window.behind_camera.dynamic_offset_not_found";
    m_locBaseFovBehind = "camera_window.behind_camera.base_fov_behind";
    m_locBehindCameraNotAvailable = "camera_window.behind_camera.not_available";

    m_locBehindValidation = "camera_window.behind_camera.validation";
    m_locBehindValidationRadius = "camera_window.behind_camera.validation_radius";
    m_locBehindValidationSpeedPos = "camera_window.behind_camera.validation_speed_pos";
    m_locBehindValidationSpeedNeg = "camera_window.behind_camera.validation_speed_neg";
    m_locBehindSpeedFovFactor = "camera_window.behind_camera.speed_fov_factor";
    m_locBehindShakeSettings = "camera_window.behind_camera.shake_settings";
    m_locBehindShakeAnimStep = "camera_window.behind_camera.shake_anim_step";
    m_locBehindShakeAnimScaleMin = "camera_window.behind_camera.shake_anim_scale_min";
    m_locBehindShakeAnimScaleMax = "camera_window.behind_camera.shake_anim_scale_max";
    m_locBehindShakeAnimationArray = "camera_window.behind_camera.shake_animation_array";
    m_locBehindCollisionSettings = "camera_window.behind_camera.collision_settings";

    m_locHeightZoom = "camera_window.top_camera.height_zoom";
    m_locMinimumHeight = "camera_window.top_camera.minimum_height";
    m_locMaximumHeight = "camera_window.top_camera.maximum_height";
    m_locHeightZoomNotFound = "camera_window.top_camera.height_zoom_not_found";
    m_locMovement = "camera_window.top_camera.movement";
    m_locMovementSpeed = "camera_window.top_camera.movement_speed";
    m_locMovementNotFound = "camera_window.top_camera.movement_not_found";

    m_locDynamicOffsetTop = "camera_window.top_camera.dynamic_offset";
    m_locForwardOffsetX = "camera_window.top_camera.forward_offset_x";
    m_locBackwardOffsetX = "camera_window.top_camera.backward_offset_x";
    m_locForwardOffsetZ = "camera_window.top_camera.forward_offset_z";
    m_locBackwardOffsetZ = "camera_window.top_camera.backward_offset_z";
    m_locDynamicOffsetNotFoundTop = "camera_window.top_camera.dynamic_offset_not_found";

    m_locBaseFovTop = "camera_window.top_camera.base_fov_top";

    m_locTopCameraNotAvailable = "camera_window.top_camera.not_available";

    m_locTopNearPlane = "camera_window.top_camera.near_plane";
    m_locTopFarPlane = "camera_window.top_camera.far_plane";
    m_locTopValidation = "camera_window.top_camera.validation";
    m_locTopValidationSpeedPos = "camera_window.top_camera.validation_speed_pos";
    m_locTopValidationSpeedNeg = "camera_window.top_camera.validation_speed_neg";
    m_locTopShakeSettings = "camera_window.top_camera.shake_settings";
    m_locTopShakeAnimStep = "camera_window.top_camera.shake_anim_step";
    m_locTopShakeAnimScaleMin = "camera_window.top_camera.shake_anim_scale_min";
    m_locTopShakeAnimScaleMax = "camera_window.top_camera.shake_anim_scale_max";
    m_locTopShakeAnimationArray = "camera_window.top_camera.shake_animation_array";
    m_locTopAdaptiveSettings = "camera_window.top_camera.adaptive_settings";
    m_locTopHeightFactor = "camera_window.top_camera.height_factor";
    m_locTopUseAdaptive = "camera_window.top_camera.use_adaptive";
    m_locTopDistanceSettings = "camera_window.top_camera.distance_settings";
    m_locTopCollisionSettings = "camera_window.top_camera.collision_settings";

    m_locBaseFovCabin = "camera_window.cabin_camera.base_fov_cabin";
    m_locCabinShakeSettings = "camera_window.cabin_camera.shake_settings";
    m_locCabinShakeAnimStep = "camera_window.cabin_camera.shake_anim_step";
    m_locCabinShakeAnimScaleMin = "camera_window.cabin_camera.shake_anim_scale_min";
    m_locCabinShakeAnimScaleMax = "camera_window.cabin_camera.shake_anim_scale_max";
    m_locCabinShakeAnimationArray = "camera_window.cabin_camera.shake_animation_array";
    m_locCabinCameraNotAvailable = "camera_window.cabin_camera.not_available";
    m_locCabinCameraNotAvailable = "camera_window.cabin_camera.not_available";

    m_locHeadOffset = "camera_window.window_camera.head_offset";
    m_locHeadXWindow = "camera_window.window_camera.head_x_window";
    m_locHeadYWindow = "camera_window.window_camera.head_y_window";
    m_locHeadZWindow = "camera_window.window_camera.head_z_window";
    m_locHeadOffsetNotFound = "camera_window.window_camera.head_offset_not_found";
    m_locLiveRotation = "camera_window.window_camera.live_rotation";
    m_locLiveYawWindow = "camera_window.window_camera.live_yaw_window";
    m_locLivePitchWindow = "camera_window.window_camera.live_pitch_window";
    m_locLiveRotationNotFound = "camera_window.window_camera.live_rotation_not_found";
    m_locMouseRotationLimitsDefaults = "camera_window.window_camera.mouse_rotation_limits_defaults";
    m_locLeftLimitWindow = "camera_window.window_camera.left_limit_window";
    m_locRightLimitWindow = "camera_window.window_camera.right_limit_window";
    m_locUpLimitWindow = "camera_window.window_camera.up_limit_window";
    m_locDownLimitWindow = "camera_window.window_camera.down_limit_window";
    m_locRotationLimitsNotFoundWindow = "camera_window.window_camera.rotation_limits_not_found_window";
    m_locDefaultLrWindow = "camera_window.window_camera.default_lr_window";
    m_locDefaultUdWindow = "camera_window.window_camera.default_ud_window";
    m_locRotationDefaultsNotFoundWindow = "camera_window.window_camera.rotation_defaults_not_found_window";
    m_locBaseFovWindow = "camera_window.window_camera.base_fov_window";
    m_locWindowCameraNotAvailable = "camera_window.window_camera.not_available";

    m_locOffsetBumper = "camera_window.bumper_camera.offset";
    m_locOffsetXBumper = "camera_window.bumper_camera.offset_x_bumper";
    m_locOffsetYBumper = "camera_window.bumper_camera.offset_y_bumper";
    m_locOffsetZBumper = "camera_window.bumper_camera.offset_z_bumper";
    m_locOffsetNotFoundBumper = "camera_window.bumper_camera.offset_not_found";
    m_locBaseFovBumper = "camera_window.bumper_camera.base_fov_bumper";
    m_locBumperCameraNotAvailable = "camera_window.bumper_camera.not_available";

    m_locOffsetWheel = "camera_window.wheel_camera.offset";
    m_locOffsetXWheel = "camera_window.wheel_camera.offset_x_wheel";
    m_locOffsetYWheel = "camera_window.wheel_camera.offset_y_wheel";
    m_locOffsetZWheel = "camera_window.wheel_camera.offset_z_wheel";
    m_locOffsetNotFoundWheel = "camera_window.wheel_camera.offset_not_found";
    m_locBaseFovWheel = "camera_window.wheel_camera.base_fov_wheel";
    m_locWheelCameraNotAvailable = "camera_window.wheel_camera.not_available";

    m_locDistanceTV = "camera_window.tv_camera.distance";
    m_locMaxDistanceTV = "camera_window.tv_camera.max_distance_tv";
    m_locDistanceNotFoundTV = "camera_window.tv_camera.distance_not_found";
    m_locPrefabUpliftTV = "camera_window.tv_camera.prefab_uplift";
    m_locPrefabUpliftXTV = "camera_window.tv_camera.prefab_uplift_x_tv";
    m_locPrefabUpliftYTV = "camera_window.tv_camera.prefab_uplift_y_tv";
    m_locPrefabUpliftZTV = "camera_window.tv_camera.prefab_uplift_z_tv";
    m_locPrefabUpliftNotFoundTV = "camera_window.tv_camera.prefab_uplift_not_found";
    m_locRoadUpliftTV = "camera_window.tv_camera.road_uplift";
    m_locRoadUpliftXTV = "camera_window.tv_camera.road_uplift_x_tv";
    m_locRoadUpliftYTV = "camera_window.tv_camera.road_uplift_y_tv";
    m_locRoadUpliftZTV = "camera_window.tv_camera.road_uplift_z_tv";
    m_locRoadUpliftNotFoundTV = "camera_window.tv_camera.road_uplift_not_found";
    m_locBaseFovTV = "camera_window.tv_camera.base_fov_tv";
    m_locTVCameraNotAvailable = "camera_window.tv_camera.not_available";

    m_locPositionFreeCam = "camera_window.free_camera.position";
    m_locPositionXFreeCam = "camera_window.free_camera.position_x_freecam";
    m_locPositionYFreeCam = "camera_window.free_camera.position_y_freecam";
    m_locPositionZFreeCam = "camera_window.free_camera.position_z_freecam";
    m_locPositionNotFoundFreeCam = "camera_window.free_camera.position_not_found";
    m_locOrientationFreeCam = "camera_window.free_camera.orientation";
    m_locMouseHorizontalFreeCam = "camera_window.free_camera.mouse_horizontal_freecam";
    m_locMouseVerticalFreeCam = "camera_window.free_camera.mouse_vertical_freecam";
    m_locRollFreeCam = "camera_window.free_camera.roll_freecam";
    m_locOrientationNotFoundFreeCam = "camera_window.free_camera.orientation_not_found";
    m_locQuaternionFreeCam = "camera_window.free_camera.quaternion";
    m_locQuaternionXFreeCam = "camera_window.free_camera.quaternion_x_freecam";
    m_locQuaternionYFreeCam = "camera_window.free_camera.quaternion_y_freecam";
    m_locQuaternionZFreeCam = "camera_window.free_camera.quaternion_z_freecam";
    m_locQuaternionWFreeCam = "camera_window.free_camera.quaternion_w_freecam";
    m_locQuaternionNotFoundFreeCam = "camera_window.free_camera.quaternion_not_found";
    m_locBaseFovFreeCam = "camera_window.free_camera.base_fov_freecam";
    m_locMovementSpeedFreeCam = "camera_window.free_camera.movement_speed";
    m_locSpeedFreeCam = "camera_window.free_camera.speed_freecam";
    m_locMovementSpeedNotFoundFreeCam = "camera_window.free_camera.movement_speed_not_found";
    m_locFreeCameraNotAvailable = "camera_window.free_camera.not_available";

    m_locCurrentModeDebug = "camera_window.debug.current_mode";
    m_locCurrentModeNADebug = "camera_window.debug.current_mode_na";
    m_locEnableDebugCamera = "camera_window.debug.enable_debug_camera";
    m_locEnableDebugCameraNotFound = "camera_window.debug.enable_debug_camera_not_found";
    m_locCleanUI = "camera_window.debug.clean_ui";
    m_locCleanUINotFound = "camera_window.debug.clean_ui_not_found";
    m_locShowDebugHUD = "camera_window.debug.show_debug_hud";
    m_locShowDebugHUDNotFound = "camera_window.debug.show_debug_hud_not_found";
    m_locEnableDebugCameraToSelectMode = "camera_window.debug.enable_debug_camera_to_select_mode";
    m_locSimpleDebug = "camera_window.debug.simple_debug";
    m_locBasicDebugCameraMode = "camera_window.debug.basic_debug_camera_mode";
    m_locVideoDebug = "camera_window.debug.video_debug";
    m_locHUDPositionDebug = "camera_window.debug.hud_position_debug";
    m_locTopLeftDebug = "camera_window.debug.top_left_debug";
    m_locBottomLeftDebug = "camera_window.debug.bottom_left_debug";
    m_locTopRightDebug = "camera_window.debug.top_right_debug";
    m_locBottomRightDebug = "camera_window.debug.bottom_right_debug";
    m_locCurrentDebug = "camera_window.debug.current_debug";
    m_locCurrentNADebug = "camera_window.debug.current_na_debug";
    m_locTrafficDebug = "camera_window.debug.traffic_debug";
    m_locCameraFocusesTraffic = "camera_window.debug.camera_focuses_traffic";
    m_locCinematicDebug = "camera_window.debug.cinematic_debug";
    m_locCinematicCameraMode = "camera_window.debug.cinematic_camera_mode";
    m_locAnimatedDebug = "camera_window.debug.animated_debug";
    m_locCreatePlayAnimations = "camera_window.debug.create_play_animations";
    m_locActivateGameAnimatedMode = "camera_window.debug.activate_game_animated_mode";
    m_locCustomAnimationControls = "camera_window.debug.custom_animation_controls";
    m_locPlayingStatus = "camera_window.debug.playing_status";
    m_locPauseButton = "camera_window.debug.pause_button";
    m_locPausedStatus = "camera_window.debug.paused_status";
    m_locStoppedStatus = "camera_window.debug.stopped_status";
    m_locPlayButton = "camera_window.debug.play_button";
    m_locStopButton = "camera_window.debug.stop_button";
    m_locStatusLabel = "camera_window.debug.status_label";
    m_locReversePlayback = "camera_window.debug.reverse_playback";
    m_locTimelineLabel = "camera_window.debug.timeline_label";
    m_locStateCameraDebug = "camera_window.debug.state_camera_debug";
    m_locCreateStateCamera = "camera_window.debug.create_state_camera";
    m_locSaveKeyframe = "camera_window.debug.save_keyframe";
    m_locReloadFromFile = "camera_window.debug.reload_from_file";
    m_locClearAllMemory = "camera_window.debug.clear_all_memory";
    m_locAnimationControls = "camera_window.debug.animation_controls";
    m_locAddEditState = "camera_window.debug.add_edit_state";
    m_locPositionXYZ = "camera_window.debug.position_xyz";
    m_locInternalValue = "camera_window.debug.internal_value";
    m_locQuaternionXYZW = "camera_window.debug.quaternion_xyzw";
    m_locFOVLabel = "camera_window.debug.fov_label";
    m_locAddStateMemory = "camera_window.debug.add_state_memory";
    m_locUpdateStateMemory = "camera_window.debug.update_state_memory";
    m_locDeleteStateMemory = "camera_window.debug.delete_state_memory";
    m_locPreviousState = "camera_window.debug.previous_state";
    m_locNextState = "camera_window.debug.next_state";
    m_locActiveStateLabel = "camera_window.debug.active_state_label";
    m_locPosLabel = "camera_window.debug.pos_label";
    m_locInternalLabel = "camera_window.debug.internal_label";
    m_locQuatLabel = "camera_window.debug.quat_label";
    m_locFOVValueLabel = "camera_window.debug.fov_value_label";
    m_locActiveStateNone = "camera_window.debug.active_state_none";
    m_locSavedStatesLabel = "camera_window.debug.saved_states_label";
    m_locStatesComboLabel = "camera_window.debug.states_combo_label";
    m_locStateItemLabel = "camera_window.debug.state_item_label";
    m_locNoStatesSaved = "camera_window.debug.no_states_saved";
    m_locOversizeDebug = "camera_window.debug.oversize_debug";
    m_locCameraOversizedTrailers = "camera_window.debug.camera_oversized_trailers";
    m_locDebugCameraNotAvailable = "camera_window.debug.debug_camera_not_available";

    // Video Debug Tab
    m_locSelectionLocks = "camera_window.debug.video.selection_locks";
    m_locPosLock = "camera_window.debug.video.pos_lock";
    m_locRotLock = "camera_window.debug.video.rot_lock";
    m_locOrbitMode = "camera_window.debug.video.orbit_mode";
    m_locOrbitZoomSpeed = "camera_window.debug.video.orbit_zoom_speed";
    m_locHoveredActor = "camera_window.debug.video.hovered_actor";
    m_locSelectedActor = "camera_window.debug.video.selected_actor";
    m_locTrafficVehicles = "camera_window.debug.video.traffic_vehicles";
    m_locSelectFromList = "camera_window.debug.video.select_from_list";
    m_locCaptureHovered = "camera_window.debug.video.capture_hovered";
    m_locCaptureSelected = "camera_window.debug.video.capture_selected";
    m_locNoActorToCapture = "camera_window.debug.video.no_actor_to_capture";
    m_locCaptureMyTruck = "camera_window.debug.video.capture_my_truck";
    m_locMyTruckNotFound = "camera_window.debug.video.my_truck_not_found";

    // Traffic Debug Tab
    m_locSelectVehicle = "camera_window.debug.traffic.select_vehicle";
    m_locVehicleDetailsTraffic = "camera_window.debug.traffic.vehicle_details_traffic";
    m_locVehicleDetailsMine = "camera_window.debug.traffic.vehicle_details_mine";
    m_locPointerLabel = "camera_window.debug.traffic.pointer_label";
    m_locPatienceLabel = "camera_window.debug.traffic.patience_label";
    m_locSafetyLabel = "camera_window.debug.traffic.safety_label";
    m_locTargetSpeedLabel = "camera_window.debug.traffic.target_speed_label";
    m_locSpeedLimitLabel = "camera_window.debug.traffic.speed_limit_label";
    m_locCurrentSpeedLabel = "camera_window.debug.traffic.current_speed_label";
    m_locAccelerationLabel = "camera_window.debug.traffic.acceleration_label";
    m_locStatusUserControlled = "camera_window.debug.traffic.status_user_controlled";
    m_locCaptureSelectedVehicle = "camera_window.debug.traffic.capture_selected_vehicle";
}

const char* CameraWindow::GetWindowTitle() const {
    return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str();
}

void CameraWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  
  // --- Standardized UI Helpers ---
  
  auto drawContainerBg = [&](float width, float height) {
      ImVec2 p_min = ImGui::GetCursorScreenPos();
      ImVec2 p_max = ImVec2(p_min.x + width, p_min.y + height);
      ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, ImColor(255, 255, 255, 12), 4.0f);
  };

  const float GLOBAL_SLIDER_WIDTH_FACTOR = 0.65f;
  const float GLOBAL_COMBO_WIDTH_FACTOR = 0.60f;
  const float GLOBAL_CONTAINER_WIDTH_FACTOR = 0.75f;

  // 1. Float Slider Helper (Slider centered, label on the right, in a container)
  auto drawFloat = [&](const std::string& label, auto getter, auto setter, float min_val, float max_val, const char* format = "%.3f", std::optional<float> default_val = std::nullopt) {
    float current_value;
    if (getter(&current_value)) {
      ImGui::PushID(label.c_str());
      float availWidth = ImGui::GetContentRegionAvail().x;
      float containerWidth = availWidth * GLOBAL_CONTAINER_WIDTH_FACTOR;
      float containerHeight = ImGui::GetFrameHeight() + 8.0f;
      float startX = (availWidth - containerWidth) * 0.5f;

      ImGui::Dummy(ImVec2(0, 2.0f)); // Top margin
      ImGui::SetCursorPosX(startX);
      drawContainerBg(containerWidth, containerHeight);
      
      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f); // Padding

      float sliderWidth = containerWidth * GLOBAL_SLIDER_WIDTH_FACTOR;
      float sliderStartX = startX + (containerWidth - sliderWidth) * 0.5f;

      // Draw default value hint on the left (Aligned left with offset 10)
      if (default_val.has_value()) {
          char val_buf[32];
          snprintf(val_buf, sizeof(val_buf), format, default_val.value());
          std::string def_text = loc.Get(m_locDefaultValuePrefix) + ": " + val_buf;
          float iconWidth = ImGui::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x;

          ImGui::SetCursorPosX(startX + 25.0f);
          
          if (Button(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::Regular().Color(Colors::LIGHT_GRAY).HoverColor(Colors::GOLD), ImVec2(iconWidth + 8.0f, 0))) {
              setter(default_val.value());
          }
          if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc.Get(m_locResetToDefaults).c_str());

          ImGui::SameLine();
          Typography::Text(TextStyle::Regular().Disabled(), "%s", def_text.c_str());
          ImGui::SameLine();
      }

      ImGui::SetCursorPosX(sliderStartX);
      ImGui::SetNextItemWidth(sliderWidth);
      std::string id = "##" + label;
      if (ImGui::SliderFloat(id.c_str(), &current_value, min_val, max_val, format)) setter(current_value);
      
      ImGui::SameLine();
      Typography::Text(TextStyle::Regular(), "%s", label.c_str());
      
      ImGui::EndGroup();
      ImGui::Dummy(ImVec2(0, 2.0f)); // Bottom margin
      ImGui::PopID();
    } else {
      Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", label.c_str(), loc.Get(m_locDataNotFound).c_str());
    }
  };

  // 2. Vector3 Helper (Stacked vertically in ONE container, labels on the right)
  auto drawVector3 = [&](const std::string& header, const char* label_x, const char* label_y, const char* label_z, auto getter, auto setter, float min_val, float max_val, std::optional<ImVec4> default_vals = std::nullopt, const char* format = "%.3f") {
    float x, y, z;
    if (getter(&x, &y, &z)) {
      ImGui::PushID(header.c_str());
      bool changed = false;
      float availWidth = ImGui::GetContentRegionAvail().x;
      float containerWidth = availWidth * GLOBAL_CONTAINER_WIDTH_FACTOR;
      float rowHeight = ImGui::GetFrameHeight() + 4.0f;
      float containerHeight = (rowHeight * 3.0f) + 12.0f;
      float startX = (availWidth - containerWidth) * 0.5f;

      ImGui::Dummy(ImVec2(0, 2.0f));
      ImGui::SetCursorPosX(startX);
      drawContainerBg(containerWidth, containerHeight);
      
      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f); // Top Padding

      auto renderComponent = [&](const char* label, float* val, std::optional<float> def_val, int comp_idx) {
          ImGui::PushID(comp_idx);
          float sliderWidth = containerWidth * GLOBAL_SLIDER_WIDTH_FACTOR;
          float sliderStartX = startX + (containerWidth - sliderWidth) * 0.5f;

          if (def_val.has_value()) {
              char val_buf[32];
              snprintf(val_buf, sizeof(val_buf), format, def_val.value());
              std::string def_text = loc.Get(m_locDefaultValuePrefix) + ": " + val_buf;
              float iconWidth = ImGui::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x;

              ImGui::SetCursorPosX(startX + 25.0f);
              
              if (Button(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::Regular().Color(Colors::LIGHT_GRAY).HoverColor(Colors::GOLD), ImVec2(iconWidth + 8.0f, 0))) {
                  *val = def_val.value();
                  setter(x, y, z);
              }
              if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc.Get(m_locResetToDefaults).c_str());

              ImGui::SameLine();
              Typography::Text(TextStyle::Regular().Disabled(), "%s", def_text.c_str());
              ImGui::SameLine();
          }

          ImGui::SetCursorPosX(sliderStartX);
          ImGui::SetNextItemWidth(sliderWidth);
          bool res = ImGui::SliderFloat("##slider", val, min_val, max_val, format);
          ImGui::SameLine();
          Typography::Text(TextStyle::Regular(), "%s", label);
          ImGui::PopID();
          return res;
      };

      changed |= renderComponent(label_x, &x, default_vals ? std::optional<float>(default_vals->x) : std::nullopt, 0);
      changed |= renderComponent(label_y, &y, default_vals ? std::optional<float>(default_vals->y) : std::nullopt, 1);
      changed |= renderComponent(label_z, &z, default_vals ? std::optional<float>(default_vals->z) : std::nullopt, 2);

      ImGui::EndGroup();
      ImGui::Dummy(ImVec2(0, 2.0f));
      
      if (changed) setter(x, y, z);
      ImGui::PopID();
    } else {
      Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", header.c_str(), loc.Get(m_locDataNotFound).c_str());
    }
  };

  // 3. Vector2 Helper (Stacked vertically in ONE container, labels on the right)
  auto drawVector2 = [&](const std::string& header, const char* label_1, const char* label_2, auto getter, auto setter, float min_val, float max_val, bool is_rotation = false, std::optional<ImVec2> default_vals = std::nullopt, const char* format = "%.1f") {
    float v1, v2;
    if (getter(&v1, &v2)) {
      ImGui::PushID(header.c_str());
      bool changed = false;
      
      if (is_rotation) {
        v1 = v1 * 57.29578f;
        v2 = v2 * 57.29578f;
      }

      float availWidth = ImGui::GetContentRegionAvail().x;
      float containerWidth = availWidth * GLOBAL_CONTAINER_WIDTH_FACTOR;
      float rowHeight = ImGui::GetFrameHeight() + 4.0f;
      float containerHeight = (rowHeight * 2.0f) + 12.0f;
      float startX = (availWidth - containerWidth) * 0.5f;

      ImGui::Dummy(ImVec2(0, 2.0f));
      ImGui::SetCursorPosX(startX);
      drawContainerBg(containerWidth, containerHeight);
      
      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6.0f); // Top Padding

      auto renderComponent = [&](const char* label, float* val, std::optional<float> def_val, int comp_idx) {
          ImGui::PushID(comp_idx);
          float sliderWidth = containerWidth * GLOBAL_SLIDER_WIDTH_FACTOR;
          float sliderStartX = startX + (containerWidth - sliderWidth) * 0.5f;

          if (def_val.has_value()) {
              float d_val = def_val.value();
              if (is_rotation) d_val *= 57.29578f;
              char val_buf[32];
              snprintf(val_buf, sizeof(val_buf), format, d_val);
              std::string def_text = loc.Get(m_locDefaultValuePrefix) + ": " + val_buf;
              float iconWidth = ImGui::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x;

              ImGui::SetCursorPosX(startX + 25.0f);
              
              if (Button(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::Regular().Color(Colors::LIGHT_GRAY).HoverColor(Colors::GOLD), ImVec2(iconWidth + 8.0f, 0))) {
                  *val = d_val;
                  if (is_rotation) {
                      setter(v1 * 0.0174533f, v2 * 0.0174533f);
                  } else {
                      setter(v1, v2);
                  }
              }
              if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", loc.Get(m_locResetToDefaults).c_str());

              ImGui::SameLine();
              Typography::Text(TextStyle::Regular().Disabled(), "%s", def_text.c_str());
              ImGui::SameLine();
          }

          ImGui::SetCursorPosX(sliderStartX);
          ImGui::SetNextItemWidth(sliderWidth);
          bool res = ImGui::SliderFloat("##slider", val, min_val, max_val, format);
          ImGui::SameLine();
          Typography::Text(TextStyle::Regular(), "%s", label);
          ImGui::PopID();
          return res;
      };

      changed |= renderComponent(label_1, &v1, default_vals ? std::optional<float>(default_vals->x) : std::nullopt, 0);
      changed |= renderComponent(label_2, &v2, default_vals ? std::optional<float>(default_vals->y) : std::nullopt, 1);

      ImGui::EndGroup();
      ImGui::Dummy(ImVec2(0, 2.0f));
      
      if (changed) {
        if (is_rotation) {
          setter(v1 * 0.0174533f, v2 * 0.0174533f);
        } else {
          setter(v1, v2);
        }
      }
      ImGui::PopID();
    } else {
      Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", header.c_str(), loc.Get(m_locDataNotFound).c_str());
    }
  };

  // 4. Boolean Checkbox Helper
  auto drawBool = [&](const std::string& label, auto getter, auto setter) {
    bool current_state;
    if (getter(&current_state)) {
      float availWidth = ImGui::GetContentRegionAvail().x;
      float containerWidth = availWidth * GLOBAL_CONTAINER_WIDTH_FACTOR;
      float containerHeight = ImGui::GetFrameHeight() + 8.0f;
      float startX = (availWidth - containerWidth) * 0.5f;

      ImGui::Dummy(ImVec2(0, 2.0f));
      ImGui::SetCursorPosX(startX);
      drawContainerBg(containerWidth, containerHeight);

      ImGui::BeginGroup();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

      float checkWidth = ImGui::GetFrameHeight();
      float labelWidth = ImGui::CalcTextSize(label.c_str()).x;
      float totalWidth = checkWidth + ImGui::GetStyle().ItemInnerSpacing.x + labelWidth;
      
      ImGui::SetCursorPosX(startX + (containerWidth - totalWidth) * 0.5f);
      if (ImGui::Checkbox(label.c_str(), &current_state)) setter(current_state);
      
      ImGui::EndGroup();
      ImGui::Dummy(ImVec2(0, 2.0f));
    } else {
      Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", label.c_str(), loc.Get(m_locDataNotFound).c_str());
    }
  };

  // 5. Centered Combo Helper
  auto drawCenteredCombo = [&](const char* id, const std::string& current_label, auto render_items) {
    float availWidth = ImGui::GetContentRegionAvail().x;
    float comboWidth = availWidth * GLOBAL_COMBO_WIDTH_FACTOR;
    ImGui::SetCursorPosX((availWidth - comboWidth) * 0.5f);
    ImGui::SetNextItemWidth(comboWidth);
    if (ImGui::BeginCombo(id, current_label.c_str())) {
        render_items();
        ImGui::EndCombo();
    }
  };

  // 5. Read-Only Data Helper
  auto drawReadOnly = [&](const std::string& label, const char* format, auto getter) {
    float value_1, value_2 = 0.0f;
    if (getter(&value_1, &value_2)) {
      Typography::Text(TextStyle::Regular().Align(TextAlign::Center), format, value_1, value_2);
    } else {
      Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", label.c_str(), loc.Get(m_locDataNotFound).c_str());
    }
  };

  // 6. Centered Section Header
  auto drawHeader = [&](const std::string& label) {
    ImGui::Spacing();
    Typography::Text(TextStyle::Bold().Color(Colors::LIGHT_BLUE).Align(TextAlign::Center).Separator(true), "%s", label.c_str());
    ImGui::Spacing();
  };

  auto current_cam_type = m_gameCameraService.GetCurrentCameraType();
  ImGui::Text(loc.Get(m_locCurrentCamera).c_str(), GameCameraTypeToString(current_cam_type), static_cast<int>(current_cam_type));

  Vector3* pCameraWorldCoords = reinterpret_cast<Vector3*>(gameData.GetCameraWorldCoordinatesPtr());
  if (pCameraWorldCoords) {
    ImGui::Text(loc.Get(m_locCameraWorldCoordinates).c_str(), pCameraWorldCoords->x, pCameraWorldCoords->y, pCameraWorldCoords->z);
  } else {
    ImGui::TextDisabled("%s", loc.Get(m_locCameraWorldCoordinatesNotFound).c_str());
  }

  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("%s", loc.Get(m_locSelectCamera).c_str());
  ImGui::Separator();

  if (Button(loc.Get(m_locInterior).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::InteriorCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::InteriorCamera;
  }
  // ImGui::SameLine(0.0f, 5.0f); //for Photo Camera
  // if (Button(loc.Get(m_locPhoto).c_str())) {
  //   m_gameCameraService.SwitchTo(GameCameraType::PhotoCamera);
  //   m_needsTabSwitch = true;
  //   m_activeTabType = GameCameraType::PhotoCamera;
  // }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locBehind).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::BehindCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::BehindCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locTop).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::TopCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::TopCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locCabin).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::CabinCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::CabinCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locWindow).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::WindowCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::WindowCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locBumper).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::BumperCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::BumperCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locWheel).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::WheelCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::WheelCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locTV).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::TVCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::TVCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (Button(loc.Get(m_locDeveloperFreeCamera).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::DeveloperFreeCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::DeveloperFreeCamera;
  }

  ImGui::Separator();
  ImGui::Separator();
  ImGui::Spacing();

  if (ImGui::BeginTabBar("CameraTabs")) {
    ImGuiTabItemFlags interiorTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::InteriorCamera) {
      interiorTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabInteriorCamera).c_str(), nullptr, interiorTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::InteriorCamera);
      if (auto* interiorCam = dynamic_cast<GameCameraInterior*>(pCamera)) {
        auto& defaults = interiorCam->GetDefaults();
        
        drawHeader(loc.Get(m_locFovZoom));
        drawFloat(loc.Get(m_locBaseFov), [&](float* fov){ return interiorCam->GetFov(fov); }, [&](float fov){ interiorCam->SetFov(fov); }, 20.0f, 120.0f, "%.1f", defaults.fov_base);
        drawReadOnly(loc.Get(m_locFinalHFov), "H=%.1f, V=%.1f", [&](float* h_fov, float* v_fov){ return interiorCam->GetFinalFov(h_fov, v_fov); });

        drawHeader(loc.Get(m_locSeatPosition));
        drawVector3(loc.Get(m_locSeatPosition), loc.Get(m_locSeatLr).c_str(), loc.Get(m_locSeatUd).c_str(), loc.Get(m_locSeatFb).c_str(),
                   [&](float* seat_x, float* seat_y, float* seat_z){ return interiorCam->GetSeatPosition(seat_x, seat_y, seat_z); },
                   [&](float seat_x, float seat_y, float seat_z){ interiorCam->SetSeatPosition(seat_x, seat_y, seat_z); }, -1.5f, 1.5f, ImVec4(defaults.seat_pos_x, defaults.seat_pos_y, defaults.seat_pos_z, 0));

        drawHeader(loc.Get(m_locHeadRotation));
        drawVector2(loc.Get(m_locHeadRotation), loc.Get(m_locYawLr).c_str(), loc.Get(m_locPitchUd).c_str(),
                   [&](float* head_yaw, float* head_pitch){ return interiorCam->GetHeadRotation(head_yaw, head_pitch); },
                   [&](float head_yaw, float head_pitch){ interiorCam->SetHeadRotation(head_yaw, head_pitch); }, -180.0f, 180.0f, true, ImVec2(defaults.yaw, defaults.pitch));

        drawHeader(loc.Get(m_locMouseRotationLimits));
        drawVector2("Horizontal Limits", loc.Get(m_locLeftLimit).c_str(), loc.Get(m_locRightLimit).c_str(),
                   [&](float* left, float* right){ float u, d; return interiorCam->GetRotationLimits(left, right, &u, &d); },
                   [&](float left, float right){ float l, r, u, d; interiorCam->GetRotationLimits(&l, &r, &u, &d); interiorCam->SetRotationLimits(left, right, u, d); }, -360.0f, 360.0f, false, ImVec2(defaults.limit_left, defaults.limit_right));
        drawVector2("Vertical Limits", loc.Get(m_locUpLimit).c_str(), loc.Get(m_locDownLimit).c_str(),
                   [&](float* up, float* down){ float l, r; return interiorCam->GetRotationLimits(&l, &r, up, down); },
                   [&](float up, float down){ float l, r, u, d; interiorCam->GetRotationLimits(&l, &r, &u, &d); interiorCam->SetRotationLimits(l, r, up, down); }, -180.0f, 180.0f, false, ImVec2(defaults.limit_up, defaults.limit_down));

        drawHeader(loc.Get(m_locRotationDefaults));
        drawVector2(loc.Get(m_locRotationDefaults), loc.Get(m_locDefaultLr).c_str(), loc.Get(m_locDefaultUd).c_str(),
                   [&](float* default_lr, float* default_ud){ return interiorCam->GetRotationDefaults(default_lr, default_ud); },
                   [&](float default_lr, float default_ud){ interiorCam->SetRotationDefaults(default_lr, default_ud); }, -360.0f, 360.0f, false, ImVec2(defaults.mouse_lr_default, defaults.mouse_ud_default));

        drawHeader(loc.Get(m_locAdvancedCoreSettings));
        drawFloat(loc.Get(m_locNearPlane), [&](float* near_plane){ return interiorCam->GetNearPlane(near_plane); }, [&](float near_plane){ interiorCam->SetNearPlane(near_plane); }, 0.01f, 10.0f, "%.3f", defaults.near_plane);
        drawFloat(loc.Get(m_locFarPlane), [&](float* far_plane){ return interiorCam->GetFarPlane(far_plane); }, [&](float far_plane){ interiorCam->SetFarPlane(far_plane); }, 100.0f, 10000.0f, "%.0f", defaults.far_plane);
        drawFloat(loc.Get(m_locMouseSensitivity), [&](float* sensitivity){ return interiorCam->GetMouseSensitivity(sensitivity); }, [&](float sensitivity){ interiorCam->SetMouseSensitivity(sensitivity); }, 0.0f, 10.0f, "%.3f", defaults.mouse_sensitivity);

        drawHeader(loc.Get(m_locInteriorLogicSettings));
        drawFloat(loc.Get(m_locZoomFovFactor), [&](float* zoom_factor){ return interiorCam->GetZoomFovFactor(zoom_factor); }, [&](float zoom_factor){ interiorCam->SetZoomFovFactor(zoom_factor); }, 0.0f, 1.0f, "%.3f", defaults.zoom_fov_factor);
        drawFloat(loc.Get(m_locZoomSpeedInterior), [&](float* zoom_speed){ return interiorCam->GetZoomSpeed(zoom_speed); }, [&](float zoom_speed){ interiorCam->SetZoomSpeed(zoom_speed); }, 0.0f, 10.0f, "%.3f", defaults.zoom_speed);

        drawHeader(loc.Get(m_locAzimuthOverrides));
        size_t azimuth_count = interiorCam->GetAzimuthOverridesCount();
        if (azimuth_count > 0) {
            static int selected_azimuth_index = 0;
            if (selected_azimuth_index >= (int)azimuth_count) selected_azimuth_index = 0;

            const GameCameraInterior::CameraData::AzimuthRangeData* az_defaults = nullptr;
            if (selected_azimuth_index < (int)defaults.azimuth_overrides_defaults.size()) {
                az_defaults = &defaults.azimuth_overrides_defaults[selected_azimuth_index];
            }

            std::string combo_label = loc.Get(m_locSelectRange) + " " + std::to_string(selected_azimuth_index);
            float start_azimuth, end_azimuth;
            if (interiorCam->GetAzimuthOverrideStartAzimuth(selected_azimuth_index, &start_azimuth) && interiorCam->GetAzimuthOverrideEndAzimuth(selected_azimuth_index, &end_azimuth)) {
                combo_label += " [" + std::to_string((int)start_azimuth) + " : " + std::to_string((int)end_azimuth) + "]";
            }

            drawCenteredCombo("##AzimuthCombo", combo_label, [&](){
                for (size_t i = 0; i < azimuth_count; ++i) {
                    std::string item_name = loc.Get(m_locSelectRange) + " " + std::to_string(i);
                    float cur_start, cur_end;
                    if (interiorCam->GetAzimuthOverrideStartAzimuth(i, &cur_start) && interiorCam->GetAzimuthOverrideEndAzimuth(i, &cur_end)) {
                        item_name += " (" + std::to_string((int)cur_start) + " to " + std::to_string((int)cur_end) + ")";
                    }
                    if (ImGui::Selectable(item_name.c_str(), selected_azimuth_index == (int)i)) selected_azimuth_index = (int)i;
                }
            });

            drawVector2(loc.Get(m_locAzimuthOverrides), loc.Get(m_locRangeStartAzimuth).c_str(), loc.Get(m_locRangeEndAzimuth).c_str(),
                       [&](float* start, float* end){ bool r1 = interiorCam->GetAzimuthOverrideStartAzimuth(selected_azimuth_index, start); bool r2 = interiorCam->GetAzimuthOverrideEndAzimuth(selected_azimuth_index, end); return r1 && r2; },
                       [&](float start, float end){ interiorCam->SetAzimuthOverrideStartAzimuth(selected_azimuth_index, start); interiorCam->SetAzimuthOverrideEndAzimuth(selected_azimuth_index, end); }, -180.0f, 180.0f, false,
                       az_defaults ? std::optional<ImVec2>(ImVec2(az_defaults->start_azimuth, az_defaults->end_azimuth)) : std::nullopt);
            
            drawBool(loc.Get(m_locZoneIsOutside), [&](bool* val){ return interiorCam->GetAzimuthOverrideOutside(selected_azimuth_index, val); }, [&](bool val){ interiorCam->SetAzimuthOverrideOutside(selected_azimuth_index, val); });
            
            drawVector2("Up Limits", loc.Get(m_locStartUpLimit).c_str(), loc.Get(m_locEndUpLimit).c_str(),
                       [&](float* start, float* end){ bool r1 = interiorCam->GetAzimuthOverrideStartUpLimit(selected_azimuth_index, start); bool r2 = interiorCam->GetAzimuthOverrideEndUpLimit(selected_azimuth_index, end); return r1 && r2; },
                       [&](float start, float end){ interiorCam->SetAzimuthOverrideStartUpLimit(selected_azimuth_index, start); interiorCam->SetAzimuthOverrideEndUpLimit(selected_azimuth_index, end); }, -90.0f, 90.0f, false,
                       az_defaults ? std::optional<ImVec2>(ImVec2(az_defaults->start_up_limit, az_defaults->end_up_limit)) : std::nullopt);
            drawVector2("Down Limits", loc.Get(m_locStartDownLimit).c_str(), loc.Get(m_locEndDownLimit).c_str(),
                       [&](float* start, float* end){ bool r1 = interiorCam->GetAzimuthOverrideStartDownLimit(selected_azimuth_index, start); bool r2 = interiorCam->GetAzimuthOverrideEndDownLimit(selected_azimuth_index, end); return r1 && r2; },
                       [&](float start, float end){ interiorCam->SetAzimuthOverrideStartDownLimit(selected_azimuth_index, start); interiorCam->SetAzimuthOverrideEndDownLimit(selected_azimuth_index, end); }, -90.0f, 90.0f, false,
                       az_defaults ? std::optional<ImVec2>(ImVec2(az_defaults->start_down_limit, az_defaults->end_down_limit)) : std::nullopt);

            drawVector2("Up-Down Defaults", loc.Get(m_locStartUpDownDefault).c_str(), loc.Get(m_locEndUpDownDefault).c_str(),
                       [&](float* start, float* end){ bool r1 = interiorCam->GetAzimuthOverrideStartUpDownDefault(selected_azimuth_index, start); bool r2 = interiorCam->GetAzimuthOverrideEndUpDownDefault(selected_azimuth_index, end); return r1 && r2; },
                       [&](float start, float end){ interiorCam->SetAzimuthOverrideStartUpDownDefault(selected_azimuth_index, start); interiorCam->SetAzimuthOverrideEndUpDownDefault(selected_azimuth_index, end); }, -90.0f, 90.0f, false,
                       az_defaults ? std::optional<ImVec2>(ImVec2(az_defaults->start_up_down_default, az_defaults->end_up_down_default)) : std::nullopt);
            drawVector2("Left-Right Defaults", loc.Get(m_locStartLeftRightDefault).c_str(), loc.Get(m_locEndLeftRightDefault).c_str(),
                       [&](float* start, float* end){ bool r1 = interiorCam->GetAzimuthOverrideStartLeftRightDefault(selected_azimuth_index, start); bool r2 = interiorCam->GetAzimuthOverrideEndLeftRightDefault(selected_azimuth_index, end); return r1 && r2; },
                       [&](float start, float end){ interiorCam->SetAzimuthOverrideStartLeftRightDefault(selected_azimuth_index, start); interiorCam->SetAzimuthOverrideEndLeftRightDefault(selected_azimuth_index, end); }, -360.0f, 360.0f, false,
                       az_defaults ? std::optional<ImVec2>(ImVec2(az_defaults->start_left_right_default, az_defaults->end_left_right_default)) : std::nullopt);

            drawVector3(loc.Get(m_locStartHeadOffset), "X", "Y", "Z",
                       [&](float* x, float* y, float* z){ return interiorCam->GetAzimuthOverrideStartHeadOffset(selected_azimuth_index, x, y, z); },
                       [&](float x, float y, float z){ interiorCam->SetAzimuthOverrideStartHeadOffset(selected_azimuth_index, x, y, z); }, -1.5f, 1.5f,
                       az_defaults ? std::optional<ImVec4>(ImVec4(az_defaults->start_head_x, az_defaults->start_head_y, az_defaults->start_head_z, 0)) : std::nullopt);
            
            drawVector3(loc.Get(m_locEndHeadOffset), "X", "Y", "Z",
                       [&](float* x, float* y, float* z){ return interiorCam->GetAzimuthOverrideEndHeadOffset(selected_azimuth_index, x, y, z); },
                       [&](float x, float y, float z){ interiorCam->SetAzimuthOverrideEndHeadOffset(selected_azimuth_index, x, y, z); }, -1.5f, 1.5f,
                       az_defaults ? std::optional<ImVec4>(ImVec4(az_defaults->end_head_x, az_defaults->end_head_y, az_defaults->end_head_z, 0)) : std::nullopt);
        } else {
            Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", loc.Get(m_locAzimuthOverrides).c_str(), loc.Get(m_locDataNotFound).c_str());
        }

        drawHeader(loc.Get(m_locShakeSettings));
        drawFloat(loc.Get(m_locShakeAnimStep), [&](float* shake_step){ return interiorCam->GetShakeAnimStep(shake_step); }, [&](float shake_step){ interiorCam->SetShakeAnimStep(shake_step); }, 0.0f, 1.0f, "%.4f", defaults.shake_step);
        drawVector2("Shake Scale Range", loc.Get(m_locShakeAnimScaleMin).c_str(), loc.Get(m_locShakeAnimScaleMax).c_str(),
                   [&](float* s_min, float* s_max){ bool r1 = interiorCam->GetShakeAnimScaleMin(s_min); bool r2 = interiorCam->GetShakeAnimScaleMax(s_max); return r1 && r2; },
                   [&](float s_min, float s_max){ interiorCam->SetShakeAnimScaleMin(s_min); interiorCam->SetShakeAnimScaleMax(s_max); }, 0.0f, 0.1f, false, ImVec2(defaults.shake_min, defaults.shake_max), "%.4f");
        drawFloat(loc.Get(m_locHandShakeLimit), [&](float* shake_limit){ return interiorCam->GetHandShakeLimit(shake_limit); }, [&](float shake_limit){ interiorCam->SetHandShakeLimit(shake_limit); }, 0.0f, 1.0f, "%.4f", defaults.hand_shake_limit);
        drawFloat(loc.Get(m_locHandShakeSpeed), [&](float* shake_speed){ return interiorCam->GetHandShakeSpeed(shake_speed); }, [&](float shake_speed){ interiorCam->SetHandShakeSpeed(shake_speed); }, 0.0f, 10.0f, "%.3f", defaults.hand_shake_speed);
        
        drawHeader(loc.Get(m_locShakeAnimationArray));
        size_t shake_count = interiorCam->GetShakeAnimCount();
        if (shake_count > 0) {
            static int selected_shake_index = 0;
            if (selected_shake_index >= (int)shake_count) selected_shake_index = 0;

            const GameCameraInterior::CameraData::Vec3* shake_def = nullptr;
            if (selected_shake_index < (int)defaults.shake_anim_defaults.size()) {
                shake_def = &defaults.shake_anim_defaults[selected_shake_index];
            }

            std::string shake_label = loc.Get(m_locSelectFrame) + " " + std::to_string(selected_shake_index);
            drawCenteredCombo("##ShakeCombo", shake_label, [&](){
                for (size_t i = 0; i < shake_count; ++i) {
                    if (ImGui::Selectable((loc.Get(m_locSelectFrame) + " " + std::to_string(i)).c_str(), selected_shake_index == (int)i)) selected_shake_index = (int)i;
                }
            });

            drawVector3(loc.Get(m_locSelectFrame), loc.Get(m_locPointX).c_str(), loc.Get(m_locPointY).c_str(), loc.Get(m_locPointZ).c_str(),
                       [&](float* x, float* y, float* z){ return interiorCam->GetShakeAnim(selected_shake_index, x, y, z); },
                       [&](float x, float y, float z){ interiorCam->SetShakeAnim(selected_shake_index, x, y, z); }, -5.0f, 5.0f,
                       shake_def ? std::optional<ImVec4>(ImVec4(shake_def->x, shake_def->y, shake_def->z, 0)) : std::nullopt, "%.5f");
        } else {
            Typography::Text(TextStyle::Regular().Disabled().Align(TextAlign::Center), "%s: %s", loc.Get(m_locShakeAnimationArray).c_str(), loc.Get(m_locDataNotFound).c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          interiorCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locInteriorCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    // ImGuiTabItemFlags photoTabFlags = ImGuiTabItemFlags_None;  //for Photo Camera
    // if (m_needsTabSwitch && m_activeTabType == GameCameraType::PhotoCamera) {
    //   photoTabFlags = ImGuiTabItemFlags_SetSelected;
    // }
    // if (ImGui::BeginTabItem(loc.Get(m_locTabPhotoCamera).c_str(), nullptr, photoTabFlags)) {
    //   auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::PhotoCamera);
    //   if (auto* photoCam = dynamic_cast<GameCameraPhoto*>(pCamera)) {
    //     auto& defaults = photoCam->GetDefaults();

    //     drawHeader(loc.Get(m_locLiveState));
    //     drawFloat(loc.Get(m_locLivePitch), [&](float* p){ float y, r, z; return photoCam->GetLiveState(p, &y, &r, &z); }, [&](float p){ float cp, y, r, z; photoCam->GetLiveState(&cp, &y, &r, &z); photoCam->SetLiveState(p, y, r, z); }, -1.57f, 1.57f, "%.4f", defaults.live_pitch);
    //     drawFloat(loc.Get(m_locLiveYaw), [&](float* y){ float p, r, z; return photoCam->GetLiveState(&p, y, &r, &z); }, [&](float y){ float p, cy, r, z; photoCam->GetLiveState(&p, &cy, &r, &z); photoCam->SetLiveState(p, y, r, z); }, -3.14f, 3.14f, "%.4f", defaults.live_yaw);
    //     drawFloat(loc.Get(m_locRollFreeCam), [&](float* r){ float p, y, z; return photoCam->GetLiveState(&p, &y, r, &z); }, [&](float r){ float p, y, cr, z; photoCam->GetLiveState(&p, &y, &cr, &z); photoCam->SetLiveState(p, y, r, z); }, -1.57f, 1.57f, "%.4f", defaults.live_roll);
    //     drawFloat(loc.Get(m_locLiveZoom), [&](float* z){ float p, y, r; return photoCam->GetLiveState(&p, &y, &r, z); }, [&](float z){ float p, y, r, cz; photoCam->GetLiveState(&p, &y, &r, &cz); photoCam->SetLiveState(p, y, r, z); }, 0.0f, 100.0f, "%.1f", defaults.live_zoom);

    //     drawHeader(loc.Get(m_locPositionFreeCam));
    //     drawVector3(loc.Get(m_locPositionFreeCam), "X", "Y", "Z",
    //                [&](float* x, float* y, float* z){ return photoCam->GetPosition(x, y, z); },
    //                [&](float x, float y, float z){ photoCam->SetPosition(x, y, z); }, -50000.0f, 50000.0f, ImVec4(defaults.pos_x, defaults.pos_y, defaults.pos_z, 0));

    //     drawHeader(loc.Get(m_locFovZoom));
    //     drawFloat(loc.Get(m_locBaseFov), [&](float* fov){ return photoCam->GetFov(fov); }, [&](float fov){ photoCam->SetFov(fov); }, 1.0f, 120.0f, "%.1f", defaults.camera_fov);

    //     ImGui::Spacing();
    //     ImGui::Separator();
    //     if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
    //       photoCam->ResetToDefaults();
    //     }
    //   } else {
    //     ImGui::TextDisabled("%s", loc.Get(m_locBehindCameraNotAvailable).c_str());
    //   }
    //   ImGui::EndTabItem();
    // }

    ImGuiTabItemFlags behindTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::BehindCamera) {
      behindTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabBehindCamera).c_str(), nullptr, behindTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::BehindCamera);
      if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
        auto& defaults = behindCam->GetDefaults();

        drawHeader(loc.Get(m_locFovZoom));
        drawFloat(loc.Get(m_locBaseFovBehind), [&](float* fov){ return behindCam->GetFov(fov); }, [&](float fov){ behindCam->SetFov(fov); }, 20.0f, 120.0f, "%.1f", defaults.fov_base);
        drawReadOnly(loc.Get(m_locFinalHFov), "H=%.1f, V=%.1f", [&](float* h_fov, float* v_fov){ return behindCam->GetFinalFov(h_fov, v_fov); });
        
        drawHeader(loc.Get(m_locLiveState));
        drawFloat(loc.Get(m_locLivePitch), [&](float* p){ float y, z; return behindCam->GetLiveState(p, &y, &z); }, [&](float p){ float cp, y, z; behindCam->GetLiveState(&cp, &y, &z); behindCam->SetLiveState(p, y, z); }, -1.57f, 1.57f, "%.4f", defaults.live_pitch);
        drawFloat(loc.Get(m_locLiveYaw), [&](float* y){ float p, z; return behindCam->GetLiveState(&p, y, &z); }, [&](float y){ float p, cy, z; behindCam->GetLiveState(&p, &cy, &z); behindCam->SetLiveState(p, y, z); }, -3.14f, 3.14f, "%.4f", defaults.live_yaw);
        drawFloat(loc.Get(m_locLiveZoom), [&](float* z){ float p, y; return behindCam->GetLiveState(&p, &y, z); }, [&](float z){ float p, y, cz; behindCam->GetLiveState(&p, &y, &cz); behindCam->SetLiveState(p, y, z); }, 0.0f, 50.0f, "%.1f", defaults.live_zoom);

        drawHeader(loc.Get(m_locDistanceZoomSettings));
        drawFloat(loc.Get(m_locMinDistance), [&](float* v){ float mx, tm, d, td, s, l; return behindCam->GetDistanceSettings(v, &mx, &tm, &d, &td, &s, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(v, mx, tm, d, td, s, l); }, 0.0f, 50.0f, "%.1f", defaults.distance_min);
        drawFloat(loc.Get(m_locMaxDistance), [&](float* v){ float mi, tm, d, td, s, l; return behindCam->GetDistanceSettings(&mi, v, &tm, &d, &td, &s, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, v, tm, d, td, s, l); }, 0.0f, 50.0f, "%.1f", defaults.distance_max);
        drawFloat(loc.Get(m_locTrailerMaxOffset), [&](float* v){ float mi, mx, d, td, s, l; return behindCam->GetDistanceSettings(&mi, &mx, v, &d, &td, &s, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, mx, v, d, td, s, l); }, 0.0f, 10.0f, "%.1f", defaults.distance_trailer_max_offset);
        drawFloat(loc.Get(m_locDefaultDistance), [&](float* v){ float mi, mx, tm, td, s, l; return behindCam->GetDistanceSettings(&mi, &mx, &tm, v, &td, &s, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, mx, tm, v, td, s, l); }, 0.0f, 50.0f, "%.1f", defaults.distance_default);
        drawFloat(loc.Get(m_locTrailerDefaultDist), [&](float* v){ float mi, mx, tm, d, s, l; return behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, v, &s, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, mx, tm, d, v, s, l); }, 0.0f, 50.0f, "%.1f", defaults.distance_trailer_default);
        drawFloat(loc.Get(m_locZoomSpeed), [&](float* v){ float mi, mx, tm, d, td, l; return behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, v, &l); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, mx, tm, d, td, v, l); }, 0.1f, 5.0f, "%.2f", defaults.distance_change_speed);
        drawFloat(loc.Get(m_locDistanceLaziness), [&](float* v){ float mi, mx, tm, d, td, s; return behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, v); }, [&](float v){ float mi, mx, tm, d, td, s, l; behindCam->GetDistanceSettings(&mi, &mx, &tm, &d, &td, &s, &l); behindCam->SetDistanceSettings(mi, mx, tm, d, td, s, v); }, 0.1f, 10.0f, "%.2f", defaults.distance_laziness_speed);

        drawHeader(loc.Get(m_locElevationPitchSettings));
        drawFloat(loc.Get(m_locAzimuthLaziness), [&](float* v){ float mi, mx, d, td, h; return behindCam->GetElevationSettings(v, &mi, &mx, &d, &td, &h); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(v, mi, mx, d, td, h); }, 0.1f, 10.0f, "%.2f", defaults.azimuth_laziness_speed);
        drawFloat(loc.Get(m_locMinElevation), [&](float* v){ float al, mx, d, td, h; return behindCam->GetElevationSettings(&al, v, &mx, &d, &td, &h); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(al, v, mx, d, td, h); }, -90.0f, 90.0f, "%.1f", defaults.elevation_min);
        drawFloat(loc.Get(m_locMaxElevation), [&](float* v){ float al, mi, d, td, h; return behindCam->GetElevationSettings(&al, &mi, v, &d, &td, &h); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(al, mi, v, d, td, h); }, 0.0f, 90.0f, "%.1f", defaults.elevation_max);
        drawFloat(loc.Get(m_locDefaultElevation), [&](float* v){ float al, mi, mx, td, h; return behindCam->GetElevationSettings(&al, &mi, &mx, v, &td, &h); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(al, mi, mx, v, td, h); }, 0.0f, 90.0f, "%.1f", defaults.elevation_default);
        drawFloat(loc.Get(m_locTrailerDefaultElev), [&](float* v){ float al, mi, mx, d, h; return behindCam->GetElevationSettings(&al, &mi, &mx, &d, v, &h); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(al, mi, mx, d, v, h); }, 0.0f, 90.0f, "%.1f", defaults.elevation_trailer_default);
        drawFloat(loc.Get(m_locHeightLimit), [&](float* v){ float al, mi, mx, d, td; return behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, v); }, [&](float v){ float al, mi, mx, d, td, h; behindCam->GetElevationSettings(&al, &mi, &mx, &d, &td, &h); behindCam->SetElevationSettings(al, mi, mx, d, td, v); }, 0.0f, 50.0f, "%.1f", defaults.height_limit);

        drawHeader(loc.Get(m_locPivotOffset));
        drawVector3(loc.Get(m_locPivotOffset), loc.Get(m_locPivotX).c_str(), loc.Get(m_locPivotY).c_str(), loc.Get(m_locPivotZ).c_str(),
                   [&](float* x, float* y, float* z){ return behindCam->GetPivot(x, y, z); },
                   [&](float x, float y, float z){ behindCam->SetPivot(x, y, z); }, -5.0f, 5.0f, ImVec4(defaults.pivot_x, defaults.pivot_y, defaults.pivot_z, 0));

        drawHeader(loc.Get(m_locDynamicOffset));
        drawFloat(loc.Get(m_locMaxDynamicOffset), [&](float* v){ float s1, s2, l; return behindCam->GetDynamicOffset(v, &s1, &s2, &l); }, [&](float v){ float m, s1, s2, l; behindCam->GetDynamicOffset(&m, &s1, &s2, &l); behindCam->SetDynamicOffset(v, s1, s2, l); }, 0.0f, 10.0f, "%.2f", defaults.dynamic_offset_max);
        drawFloat(loc.Get(m_locDynOffsetSpeedMin), [&](float* v){ float m, s2, l; return behindCam->GetDynamicOffset(&m, v, &s2, &l); }, [&](float v){ float m, s1, s2, l; behindCam->GetDynamicOffset(&m, &s1, &s2, &l); behindCam->SetDynamicOffset(m, v, s2, l); }, 0.0f, 100.0f, "%.1f", defaults.dynamic_offset_speed_min);
        drawFloat(loc.Get(m_locDynOffsetSpeedMax), [&](float* v){ float m, s1, l; return behindCam->GetDynamicOffset(&m, &s1, v, &l); }, [&](float v){ float m, s1, s2, l; behindCam->GetDynamicOffset(&m, &s1, &s2, &l); behindCam->SetDynamicOffset(m, s1, v, l); }, 0.0f, 100.0f, "%.1f", defaults.dynamic_offset_speed_max);
        drawFloat(loc.Get(m_locDynOffsetLaziness), [&](float* v){ float m, s1, s2; return behindCam->GetDynamicOffset(&m, &s1, &s2, v); }, [&](float v){ float m, s1, s2, l; behindCam->GetDynamicOffset(&m, &s1, &s2, &l); behindCam->SetDynamicOffset(m, s1, s2, v); }, 0.1f, 5.0f, "%.2f", defaults.dynamic_offset_laziness_speed);

        drawHeader(loc.Get(m_locBehindCollisionSettings));
        drawBool(loc.Get(m_locBehindValidation), [&](bool* v){ return behindCam->GetValidation(v); }, [&](bool v){ behindCam->SetValidation(v); });
        drawFloat(loc.Get(m_locBehindValidationRadius), [&](float* v){ float s1, s2; return behindCam->GetValidationSettings(v, &s1, &s2); }, [&](float v){ float r, s1, s2; behindCam->GetValidationSettings(&r, &s1, &s2); behindCam->SetValidationSettings(v, s1, s2); }, 0.0f, 5.0f, "%.3f", defaults.validation_radius);
        drawFloat(loc.Get(m_locBehindValidationSpeedPos), [&](float* v){ float r, s2; return behindCam->GetValidationSettings(&r, v, &s2); }, [&](float v){ float r, s1, s2; behindCam->GetValidationSettings(&r, &s1, &s2); behindCam->SetValidationSettings(r, v, s2); }, 0.0f, 10.0f, "%.3f", defaults.validation_speed_positive);
        drawFloat(loc.Get(m_locBehindValidationSpeedNeg), [&](float* v){ float r, s1; return behindCam->GetValidationSettings(&r, &s1, v); }, [&](float v){ float r, s1, s2; behindCam->GetValidationSettings(&r, &s1, &s2); behindCam->SetValidationSettings(r, s1, v); }, 0.0f, 10.0f, "%.3f", defaults.validation_speed_negative);
        drawFloat(loc.Get(m_locBehindSpeedFovFactor), [&](float* v){ return behindCam->GetSpeedFovChangeFactor(v); }, [&](float v){ behindCam->SetSpeedFovChangeFactor(v); }, 0.0f, 1.0f, "%.4f", defaults.speed_fov_change_factor);

        drawHeader(loc.Get(m_locBehindShakeSettings));
        drawFloat(loc.Get(m_locBehindShakeAnimStep), [&](float* v){ return behindCam->GetShakeAnimStep(v); }, [&](float v){ behindCam->SetShakeAnimStep(v); }, 0.0f, 1.0f, "%.4f", defaults.shake_anim_step);
        drawVector2("Shake Scale Range", loc.Get(m_locBehindShakeAnimScaleMin).c_str(), loc.Get(m_locBehindShakeAnimScaleMax).c_str(),
                   [&](float* s_min, float* s_max){ bool r1 = behindCam->GetShakeAnimScaleMin(s_min); bool r2 = behindCam->GetShakeAnimScaleMax(s_max); return r1 && r2; },
                   [&](float s_min, float s_max){ behindCam->SetShakeAnimScaleMin(s_min); behindCam->SetShakeAnimScaleMax(s_max); }, 0.0f, 0.1f, false, ImVec2(defaults.shake_anim_scale_min, defaults.shake_anim_scale_max), "%.4f");

        drawHeader(loc.Get(m_locBehindShakeAnimationArray));
        size_t shake_count = behindCam->GetShakeAnimCount();
        if (shake_count > 0) {
            static int selected_shake_index_behind = 0;
            if (selected_shake_index_behind >= (int)shake_count) selected_shake_index_behind = 0;

            std::string shake_label = loc.Get(m_locSelectFrame) + " " + std::to_string(selected_shake_index_behind);
            drawCenteredCombo("##ShakeComboBehind", shake_label, [&](){
                for (size_t i = 0; i < shake_count; ++i) {
                    if (ImGui::Selectable((loc.Get(m_locSelectFrame) + " " + std::to_string(i)).c_str(), selected_shake_index_behind == (int)i)) selected_shake_index_behind = (int)i;
                }
            });

            drawVector3(loc.Get(m_locSelectFrame), loc.Get(m_locPointX).c_str(), loc.Get(m_locPointY).c_str(), loc.Get(m_locPointZ).c_str(),
                       [&](float* x, float* y, float* z){ behindCam->GetShakeAnim(selected_shake_index_behind, *x, *y, *z); return true; },
                       [&](float x, float y, float z){ behindCam->SetShakeAnim(selected_shake_index_behind, x, y, z); }, -5.0f, 5.0f, std::nullopt, "%.5f");
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          behindCam->ResetToDefaults();
    }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locBehindCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags topTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::TopCamera) {
      topTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabTopCamera).c_str(), nullptr, topTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::TopCamera);
      if (auto* topCam = dynamic_cast<GameCameraTop*>(pCamera)) {
        auto& defaults = topCam->GetDefaults();

        drawHeader(loc.Get(m_locFovZoom));
        drawFloat(loc.Get(m_locBaseFovTop), [&](float* fov){ return topCam->GetFov(fov); }, [&](float fov){ topCam->SetFov(fov); }, 20.0f, 120.0f, "%.1f", defaults.fov_base);
        drawReadOnly(loc.Get(m_locFinalHFov), "H=%.1f, V=%.1f", [&](float* h_fov, float* v_fov){ return topCam->GetFinalFov(h_fov, v_fov); });

        drawHeader(loc.Get(m_locHeightZoom));
        drawFloat(loc.Get(m_locMinimumHeight), [&](float* v){ float mx; return topCam->GetHeight(v, &mx); }, [&](float v){ float mi, mx; topCam->GetHeight(&mi, &mx); topCam->SetHeight(v, mx); }, 1.0f, 50.0f, "%.1f", defaults.minimum_height);
        drawFloat(loc.Get(m_locMaximumHeight), [&](float* v){ float mi; return topCam->GetHeight(&mi, v); }, [&](float v){ float mi, mx; topCam->GetHeight(&mi, &mx); topCam->SetHeight(mi, v); }, 1.0f, 100.0f, "%.1f", defaults.maximum_height);

        drawHeader(loc.Get(m_locMovement));
        drawFloat(loc.Get(m_locMovementSpeed), [&](float* speed){ return topCam->GetSpeed(speed); }, [&](float speed){ topCam->SetSpeed(speed); }, 0.1f, 10.0f, "%.2f", defaults.speed);

        drawHeader(loc.Get(m_locDynamicOffsetTop));
        drawFloat(loc.Get(m_locForwardOffsetX), [&](float* v){ float b; return topCam->GetOffsets(v, &b); }, [&](float v){ float f, b; topCam->GetOffsets(&f, &b); topCam->SetOffsets(v, b); }, -20.0f, 20.0f, "%.2f", defaults.x_offset_forward);
        drawFloat(loc.Get(m_locBackwardOffsetX), [&](float* v){ float f; return topCam->GetOffsets(&f, v); }, [&](float v){ float f, b; topCam->GetOffsets(&f, &b); topCam->SetOffsets(f, v); }, -20.0f, 20.0f, "%.2f", defaults.x_offset_backward);

        drawHeader(loc.Get(m_locTopDistanceSettings));
        drawFloat(loc.Get(m_locForwardOffsetZ), [&](float* v){ float b; return topCam->GetOffsetsZ(v, &b); }, [&](float v){ float f, b; topCam->GetOffsetsZ(&f, &b); topCam->SetOffsetsZ(v, b); }, -20.0f, 20.0f, "%.2f", defaults.offset_forward);
        drawFloat(loc.Get(m_locBackwardOffsetZ), [&](float* v){ float f; return topCam->GetOffsetsZ(&f, v); }, [&](float v){ float f, b; topCam->GetOffsetsZ(&f, &b); topCam->SetOffsetsZ(f, v); }, -20.0f, 20.0f, "%.2f", defaults.offset_backward);

        drawHeader(loc.Get(m_locTopAdaptiveSettings));
        drawBool(loc.Get(m_locTopUseAdaptive), [&](bool* v){ float f; return topCam->GetAdaptiveSettings(&f, v); }, [&](bool v){ float f; bool u; topCam->GetAdaptiveSettings(&f, &u); topCam->SetAdaptiveSettings(f, v); });
        drawFloat(loc.Get(m_locTopHeightFactor), [&](float* v){ bool u; return topCam->GetAdaptiveSettings(v, &u); }, [&](float v){ float f; bool u; topCam->GetAdaptiveSettings(&f, &u); topCam->SetAdaptiveSettings(v, u); }, 0.0f, 5.0f, "%.2f", defaults.camera_height_factor);

        drawHeader(loc.Get(m_locAdvancedCoreSettings));
        drawFloat(loc.Get(m_locTopNearPlane), [&](float* v){ float f; return topCam->GetPlaneSettings(v, &f); }, [&](float v){ float n, f; topCam->GetPlaneSettings(&n, &f); topCam->SetPlaneSettings(v, f); }, 0.01f, 10.0f, "%.3f", defaults.near_plane);
        drawFloat(loc.Get(m_locTopFarPlane), [&](float* v){ float n; return topCam->GetPlaneSettings(&n, v); }, [&](float v){ float n, f; topCam->GetPlaneSettings(&n, &f); topCam->SetPlaneSettings(n, v); }, 10.0f, 1000.0f, "%.1f", defaults.far_plane);

        drawHeader(loc.Get(m_locTopCollisionSettings));
        drawBool(loc.Get(m_locTopValidation), [&](bool* v){ return topCam->GetValidation(v); }, [&](bool v){ topCam->SetValidation(v); });
        drawFloat(loc.Get(m_locTopValidationSpeedPos), [&](float* v){ float s2; return topCam->GetValidationSettings(v, &s2); }, [&](float v){ float s1, s2; topCam->GetValidationSettings(&s1, &s2); topCam->SetValidationSettings(v, s2); }, 0.0f, 10.0f, "%.3f", defaults.validation_speed_positive);
        drawFloat(loc.Get(m_locTopValidationSpeedNeg), [&](float* v){ float s1; return topCam->GetValidationSettings(&s1, v); }, [&](float v){ float s1, s2; topCam->GetValidationSettings(&s1, &s2); topCam->SetValidationSettings(s1, v); }, 0.0f, 10.0f, "%.3f", defaults.validation_speed_negative);

        drawHeader(loc.Get(m_locTopShakeSettings));
        drawFloat(loc.Get(m_locTopShakeAnimStep), [&](float* v){ return topCam->GetShakeAnimStep(v); }, [&](float v){ topCam->SetShakeAnimStep(v); }, 0.0f, 1.0f, "%.4f", defaults.shake_anim_step);
        drawVector2("Shake Scale Range", loc.Get(m_locTopShakeAnimScaleMin).c_str(), loc.Get(m_locTopShakeAnimScaleMax).c_str(),
                   [&](float* s_min, float* s_max){ bool r1 = topCam->GetShakeAnimScaleMin(s_min); bool r2 = topCam->GetShakeAnimScaleMax(s_max); return r1 && r2; },
                   [&](float s_min, float s_max){ topCam->SetShakeAnimScaleMin(s_min); topCam->SetShakeAnimScaleMax(s_max); }, 0.0f, 0.1f, false, ImVec2(defaults.shake_anim_scale_min, defaults.shake_anim_scale_max), "%.4f");

        drawHeader(loc.Get(m_locTopShakeAnimationArray));
        size_t shake_count = topCam->GetShakeAnimCount();
        if (shake_count > 0) {
            static int selected_shake_index_top = 0;
            if (selected_shake_index_top >= (int)shake_count) selected_shake_index_top = 0;

            std::string shake_label = loc.Get(m_locSelectFrame) + " " + std::to_string(selected_shake_index_top);
            drawCenteredCombo("##ShakeComboTop", shake_label, [&](){
                for (size_t i = 0; i < shake_count; ++i) {
                    if (ImGui::Selectable((loc.Get(m_locSelectFrame) + " " + std::to_string(i)).c_str(), selected_shake_index_top == (int)i)) selected_shake_index_top = (int)i;
                }
            });

            drawVector3(loc.Get(m_locSelectFrame), loc.Get(m_locPointX).c_str(), loc.Get(m_locPointY).c_str(), loc.Get(m_locPointZ).c_str(),
                       [&](float* x, float* y, float* z){ topCam->GetShakeAnim(selected_shake_index_top, *x, *y, *z); return true; },
                       [&](float x, float y, float z){ topCam->SetShakeAnim(selected_shake_index_top, x, y, z); }, -5.0f, 5.0f, std::nullopt, "%.5f");
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          topCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locTopCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags cabinTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::CabinCamera) {
      cabinTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabCabinCamera).c_str(), nullptr, cabinTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::CabinCamera);
      if (auto* cabinCam = dynamic_cast<GameCameraCabin*>(pCamera)) {
        auto& defaults = cabinCam->GetDefaults();

        drawHeader(loc.Get(m_locFovZoom));
        drawFloat(loc.Get(m_locBaseFovCabin), [&](float* fov){ return cabinCam->GetFov(fov); }, [&](float fov){ cabinCam->SetFov(fov); }, 20.0f, 120.0f, "%.1f", defaults.fov_base);
        drawReadOnly(loc.Get(m_locFinalHFov), "H=%.1f, V=%.1f", [&](float* h_fov, float* v_fov){ return cabinCam->GetFinalFov(h_fov, v_fov); });

        drawHeader(loc.Get(m_locCabinShakeSettings));
        drawFloat(loc.Get(m_locCabinShakeAnimStep), [&](float* v){ return cabinCam->GetShakeAnimStep(v); }, [&](float v){ cabinCam->SetShakeAnimStep(v); }, 0.0f, 1.0f, "%.4f", defaults.shake_anim_step);
        drawVector2("Shake Scale Range", loc.Get(m_locCabinShakeAnimScaleMin).c_str(), loc.Get(m_locCabinShakeAnimScaleMax).c_str(),
                   [&](float* s_min, float* s_max){ bool r1 = cabinCam->GetShakeAnimScaleMin(s_min); bool r2 = cabinCam->GetShakeAnimScaleMax(s_max); return r1 && r2; },
                   [&](float s_min, float s_max){ cabinCam->SetShakeAnimScaleMin(s_min); cabinCam->SetShakeAnimScaleMax(s_max); }, 0.0f, 0.1f, false, ImVec2(defaults.shake_anim_scale_min, defaults.shake_anim_scale_max), "%.4f");

        drawHeader(loc.Get(m_locCabinShakeAnimationArray));
        size_t shake_count = cabinCam->GetShakeAnimCount();
        if (shake_count > 0) {
            static int selected_shake_index_cabin = 0;
            if (selected_shake_index_cabin >= (int)shake_count) selected_shake_index_cabin = 0;

            std::string shake_label = loc.Get(m_locSelectFrame) + " " + std::to_string(selected_shake_index_cabin);
            drawCenteredCombo("##ShakeComboCabin", shake_label, [&](){
                for (size_t i = 0; i < shake_count; ++i) {
                    if (ImGui::Selectable((loc.Get(m_locSelectFrame) + " " + std::to_string(i)).c_str(), selected_shake_index_cabin == (int)i)) selected_shake_index_cabin = (int)i;
                }
            });

            drawVector3(loc.Get(m_locSelectFrame), loc.Get(m_locPointX).c_str(), loc.Get(m_locPointY).c_str(), loc.Get(m_locPointZ).c_str(),
                       [&](float* x, float* y, float* z){ cabinCam->GetShakeAnim(selected_shake_index_cabin, *x, *y, *z); return true; },
                       [&](float x, float y, float z){ cabinCam->SetShakeAnim(selected_shake_index_cabin, x, y, z); }, -5.0f, 5.0f, std::nullopt, "%.5f");
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          cabinCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locCabinCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags windowTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::WindowCamera) {
      windowTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabWindowCamera).c_str(), nullptr, windowTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::WindowCamera);
      if (auto* windowCam = dynamic_cast<GameCameraWindow*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locHeadOffset).c_str());
        float head_offset_x, head_offset_y, head_offset_z;
        if (windowCam->GetHeadOffset(&head_offset_x, &head_offset_y, &head_offset_z)) {
          bool headChanged = false;
          headChanged |= ImGui::SliderFloat(loc.Get(m_locHeadXWindow).c_str(), &head_offset_x, -1.0f, 1.0f, "%.3f");
          headChanged |= ImGui::SliderFloat(loc.Get(m_locHeadYWindow).c_str(), &head_offset_y, -1.0f, 1.0f, "%.3f");
          headChanged |= ImGui::SliderFloat(loc.Get(m_locHeadZWindow).c_str(), &head_offset_z, -1.0f, 1.0f, "%.3f");
          if (headChanged) {
            windowCam->SetHeadOffset(head_offset_x, head_offset_y, head_offset_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locHeadOffsetNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locLiveRotation).c_str());
        float live_yaw, live_pitch;
        if (windowCam->GetLiveRotation(&live_yaw, &live_pitch)) {
          bool rotChanged = false;
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locLiveYawWindow).c_str(), &live_yaw, -3.14159f, 3.14159f, "%.3f");
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locLivePitchWindow).c_str(), &live_pitch, -1.57079f, 1.57079f, "%.3f");
          if (rotChanged) {
            windowCam->SetLiveRotation(live_yaw, live_pitch);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locLiveRotationNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locMouseRotationLimitsDefaults).c_str());
        float mouse_left_limit, mouse_right_limit, mouse_up_limit, mouse_down_limit;
        if (windowCam->GetRotationLimits(&mouse_left_limit, &mouse_right_limit, &mouse_up_limit, &mouse_down_limit)) {
          bool limitsChanged = false;
          limitsChanged |= ImGui::SliderFloat(loc.Get(m_locLeftLimitWindow).c_str(), &mouse_left_limit, -360.0f, 360.0f, "%.1f");
          limitsChanged |= ImGui::SliderFloat(loc.Get(m_locRightLimitWindow).c_str(), &mouse_right_limit, -360.0f, 360.0f, "%.1f");
          limitsChanged |= ImGui::SliderFloat(loc.Get(m_locUpLimitWindow).c_str(), &mouse_up_limit, -90.0f, 90.0f, "%.1f");
          limitsChanged |= ImGui::SliderFloat(loc.Get(m_locDownLimitWindow).c_str(), &mouse_down_limit, -180.0f, 90.0f, "%.1f");
          if (limitsChanged) {
            windowCam->SetRotationLimits(mouse_left_limit, mouse_right_limit, mouse_up_limit, mouse_down_limit);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locRotationLimitsNotFoundWindow).c_str());
        }
        float mouse_lr_default, mouse_ud_default;
        if (windowCam->GetRotationDefaults(&mouse_lr_default, &mouse_ud_default)) {
          bool defaultsChanged = false;
          defaultsChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultLrWindow).c_str(), &mouse_lr_default, 0.0f, 360.0f, "%.1f");
          defaultsChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultUdWindow).c_str(), &mouse_ud_default, -90.0f, 90.0f, "%.1f");
          if (defaultsChanged) {
            windowCam->SetRotationDefaults(mouse_lr_default, mouse_ud_default);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locRotationDefaultsNotFoundWindow).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (windowCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovWindow).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            windowCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (windowCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          windowCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locWindowCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags bumperTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::BumperCamera) {
      bumperTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabBumperCamera).c_str(), nullptr, bumperTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::BumperCamera);
      if (auto* bumperCam = dynamic_cast<GameCameraBumper*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locOffsetBumper).c_str());
        float offset_x, offset_y, offset_z;
        if (bumperCam->GetOffset(&offset_x, &offset_y, &offset_z)) {
          bool offsetChanged = false;
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetXBumper).c_str(), &offset_x, -5.0f, 5.0f, "%.2f");
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetYBumper).c_str(), &offset_y, -5.0f, 5.0f, "%.2f");
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetZBumper).c_str(), &offset_z, -5.0f, 5.0f, "%.2f");
          if (offsetChanged) {
            bumperCam->SetOffset(offset_x, offset_y, offset_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locOffsetNotFoundBumper).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (bumperCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovBumper).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            bumperCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (bumperCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          bumperCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locBumperCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags wheelTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::WheelCamera) {
      wheelTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabWheelCamera).c_str(), nullptr, wheelTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::WheelCamera);
      if (auto* wheelCam = dynamic_cast<GameCameraWheel*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locOffsetWheel).c_str());
        float offset_x, offset_y, offset_z;
        if (wheelCam->GetOffset(&offset_x, &offset_y, &offset_z)) {
          bool offsetChanged = false;
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetXWheel).c_str(), &offset_x, -5.0f, 5.0f, "%.2f");
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetYWheel).c_str(), &offset_y, -5.0f, 5.0f, "%.2f");
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locOffsetZWheel).c_str(), &offset_z, -5.0f, 5.0f, "%.2f");
          if (offsetChanged) {
            wheelCam->SetOffset(offset_x, offset_y, offset_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locOffsetNotFoundWheel).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (wheelCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovWheel).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            wheelCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (wheelCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          wheelCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locWheelCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags tvTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::TVCamera) {
      tvTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabTVCamera).c_str(), nullptr, tvTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::TVCamera);
      if (auto* tvCam = dynamic_cast<GameCameraTV*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locDistanceTV).c_str());
        float max_distance;
        if (tvCam->GetMaxDistance(&max_distance)) {
          if (ImGui::SliderFloat(loc.Get(m_locMaxDistanceTV).c_str(), &max_distance, 0.0f, 100.0f, "%.1f")) {
            tvCam->SetMaxDistance(max_distance);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locDistanceNotFoundTV).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locPrefabUpliftTV).c_str());
        float prefab_uplift_x, prefab_uplift_y, prefab_uplift_z;
        if (tvCam->GetPrefabUplift(&prefab_uplift_x, &prefab_uplift_y, &prefab_uplift_z)) {
          bool prefabChanged = false;
          prefabChanged |= ImGui::SliderFloat(loc.Get(m_locPrefabUpliftXTV).c_str(), &prefab_uplift_x, -10.0f, 10.0f, "%.2f");
          prefabChanged |= ImGui::SliderFloat(loc.Get(m_locPrefabUpliftYTV).c_str(), &prefab_uplift_y, -10.0f, 10.0f, "%.2f");
          prefabChanged |= ImGui::SliderFloat(loc.Get(m_locPrefabUpliftZTV).c_str(), &prefab_uplift_z, -10.0f, 10.0f, "%.2f");
          if (prefabChanged) {
            tvCam->SetPrefabUplift(prefab_uplift_x, prefab_uplift_y, prefab_uplift_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locPrefabUpliftNotFoundTV).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locRoadUpliftTV).c_str());
        float road_uplift_x, road_uplift_y, road_uplift_z;
        if (tvCam->GetRoadUplift(&road_uplift_x, &road_uplift_y, &road_uplift_z)) {
          bool roadChanged = false;
          roadChanged |= ImGui::SliderFloat(loc.Get(m_locRoadUpliftXTV).c_str(), &road_uplift_x, -10.0f, 10.0f, "%.2f");
          roadChanged |= ImGui::SliderFloat(loc.Get(m_locRoadUpliftYTV).c_str(), &road_uplift_y, -10.0f, 10.0f, "%.2f");
          roadChanged |= ImGui::SliderFloat(loc.Get(m_locRoadUpliftZTV).c_str(), &road_uplift_z, -10.0f, 10.0f, "%.2f");
          if (roadChanged) {
            tvCam->SetRoadUplift(road_uplift_x, road_uplift_y, road_uplift_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locRoadUpliftNotFoundTV).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (tvCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovTV).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            tvCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (tvCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          tvCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locTVCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags freeCamTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::DeveloperFreeCamera) {
      freeCamTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabFreeCamera).c_str(), nullptr, freeCamTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::DeveloperFreeCamera);
      if (auto* freeCam = dynamic_cast<GameCameraFree*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locPositionFreeCam).c_str());
        float pos_x, pos_y, pos_z;
        if (freeCam->GetPosition(&pos_x, &pos_y, &pos_z)) {
          bool posChanged = false;
          posChanged |= ImGui::SliderFloat(loc.Get(m_locPositionXFreeCam).c_str(), &pos_x, pos_x - 200.0f, pos_x + 200.0f, "%.2f");
          posChanged |= ImGui::SliderFloat(loc.Get(m_locPositionYFreeCam).c_str(), &pos_y, pos_y - 200.0f, pos_y + 200.0f, "%.2f");
          posChanged |= ImGui::SliderFloat(loc.Get(m_locPositionZFreeCam).c_str(), &pos_z, pos_z - 200.0f, pos_z + 200.0f, "%.2f");
          if (posChanged) {
            freeCam->SetPosition(pos_x, pos_y, pos_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locPositionNotFoundFreeCam).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locOrientationFreeCam).c_str());
        float mouse_x, mouse_y, roll;
        if (freeCam->GetOrientation(&mouse_x, &mouse_y, &roll)) {
          bool rotChanged = false;
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locMouseHorizontalFreeCam).c_str(), &mouse_x, 0.0f, 6.28318f, "%.4f");
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locMouseVerticalFreeCam).c_str(), &mouse_y, 0.0f, 6.28318f, "%.4f");
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locRollFreeCam).c_str(), &roll, -1.57079f, 1.57079f, "%.4f");
          if (rotChanged) {
            freeCam->SetOrientation(mouse_x, mouse_y, roll);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locOrientationNotFoundFreeCam).c_str());
        }

        ImGui::Text("%s", loc.Get(m_locQuaternionFreeCam).c_str());
        float qx, qy, qz, qw;
        if (freeCam->GetQuaternion(&qx, &qy, &qz, &qw)) {
          ImGui::Text(loc.Get(m_locQuaternionXFreeCam).c_str(), qx);
          ImGui::Text(loc.Get(m_locQuaternionYFreeCam).c_str(), qy);
          ImGui::Text(loc.Get(m_locQuaternionZFreeCam).c_str(), qz);
          ImGui::Text(loc.Get(m_locQuaternionWFreeCam).c_str(), qw);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locQuaternionNotFoundFreeCam).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (freeCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovFreeCam).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            freeCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (freeCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locMovementSpeedFreeCam).c_str());
        float speed;
        if (freeCam->GetSpeed(&speed)) {
          if (ImGui::SliderFloat(loc.Get(m_locSpeedFreeCam).c_str(), &speed, 0.1f, 500.0f, "%.1f")) {
            freeCam->SetSpeed(speed);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locMovementSpeedNotFoundFreeCam).c_str());
        }

        ImGui::Separator();
        if (Button(loc.Get(m_locResetToDefaults).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
          freeCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locFreeCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(loc.Get(m_locTabDebug).c_str())) {
      auto* debugCam = m_gameCameraService.GetDebugCamera();
      if (debugCam) {
        DebugCameraMode currentMode;
        if (debugCam->GetCurrentMode(&currentMode)) {
          ImGui::Text(loc.Get(m_locCurrentModeDebug).c_str(), DebugCameraModeToString(currentMode), static_cast<int>(currentMode));
        } else {
          ImGui::Text("%s", loc.Get(m_locCurrentModeNADebug).c_str());
        }
        ImGui::Separator();

        bool isEnabled = false;  // Default to false
        if (debugCam->GetEnabled(&isEnabled)) {
          if (ImGui::Checkbox(loc.Get(m_locEnableDebugCamera).c_str(), &isEnabled)) {
            debugCam->SetEnabled(isEnabled);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locEnableDebugCameraNotFound).c_str());
        }

        ImGui::SameLine(0, 20);

        bool isGameUiVisible;
        if (debugCam->GetGameUiVisible(&isGameUiVisible)) {
          if (ImGui::Checkbox(loc.Get(m_locCleanUI).c_str(), &isGameUiVisible)) {
            debugCam->SetGameUiVisible(isGameUiVisible);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locCleanUINotFound).c_str());
        }

        ImGui::SameLine(0, 20);

        bool isHudVisible;
        if (debugCam->GetHudVisible(&isHudVisible)) {
          if (ImGui::Checkbox(loc.Get(m_locShowDebugHUD).c_str(), &isHudVisible)) {
            debugCam->SetHudVisible(isHudVisible);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locShowDebugHUDNotFound).c_str());
        }

        ImGui::Separator();

        if (isEnabled)  // Use the value we already fetched
        {
          if (ImGui::BeginTabBar("DebugModeTabs")) {
            if (ImGui::BeginTabItem(loc.Get(m_locSimpleDebug).c_str())) {
              DebugCameraMode currentMode;
              if (debugCam->GetCurrentMode(&currentMode) && currentMode != DebugCameraMode::SIMPLE) {
                debugCam->SetMode(DebugCameraMode::SIMPLE);
              }
              ImGui::Text("%s", loc.Get(m_locBasicDebugCameraMode).c_str());
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locVideoDebug).c_str())) {
              DebugCameraMode currentMode;
              if (debugCam->GetCurrentMode(&currentMode) && currentMode != DebugCameraMode::VIDEO) {
                debugCam->SetMode(DebugCameraMode::VIDEO);
              }

              ImGui::Text("%s", loc.Get(m_locSelectionLocks).c_str());
              bool posLock, rotLock, orbit;
              if (debugCam->GetPosLock(&posLock)) {
                if (ImGui::Checkbox(loc.Get(m_locPosLock).c_str(), &posLock)) debugCam->SetPosLock(posLock);
              }
              ImGui::SameLine();
              if (debugCam->GetRotLock(&rotLock)) {
                if (ImGui::Checkbox(loc.Get(m_locRotLock).c_str(), &rotLock)) debugCam->SetRotLock(rotLock);
              }
              ImGui::SameLine();
              if (debugCam->GetOrbitMode(&orbit)) {
                if (ImGui::Checkbox(loc.Get(m_locOrbitMode).c_str(), &orbit)) debugCam->SetOrbitMode(orbit);
              }

              float orbitSpeed;
              if (debugCam->GetOrbitSpeed(&orbitSpeed)) {
                if (ImGui::SliderFloat(loc.Get(m_locOrbitZoomSpeed).c_str(), &orbitSpeed, -100.0f, 100.0f, "%.2f")) {
                  debugCam->SetOrbitSpeed(orbitSpeed);
                }
              }

              // ImGui::Spacing();
              // ImGui::Checkbox("Show PiP Texture Window", &m_showPipWindow);

              // // --- NEW: Scanned Dynamic Textures Selector ---
              // ImGui::Separator();
              // ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Scanned Dynamic Textures (Auto-scan):");
              
              // auto scannedTextures = debugCam->GetAvailableTextures();
              // static int selectedScanIdx = -1;

              // std::string comboPreview = "Select a texture to preview...";
              // if (selectedScanIdx >= 0 && selectedScanIdx < (int)scannedTextures.size()) {
              //     comboPreview = scannedTextures[selectedScanIdx].name;
              // }

              // if (ImGui::BeginCombo("##ScannedTexturesCombo", comboPreview.c_str())) {
              //   for (int i = 0; i < (int)scannedTextures.size(); ++i) {
              //     bool isSelected = (selectedScanIdx == i);
              //     char label[1024];
              //     sprintf_s(label, "[ID: %d] %s (%dx%d)",
              //         scannedTextures[i].dx11Id,
              //         scannedTextures[i].name.c_str(),
              //         scannedTextures[i].width,
              //         scannedTextures[i].height);

              //     if (ImGui::Selectable(label, isSelected)) {
              //       selectedScanIdx = i;
              //       debugCam->SetSelectedTextureId(scannedTextures[i].dx11Id);
              //     }
              //     if (isSelected) ImGui::SetItemDefaultFocus();
              //   }
              //   ImGui::EndCombo();
              // }

              // // --- DIAGNOSTIC: Automatic Methods Testing ---
              // ImGui::Spacing();
              // ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Direct Access (Experimental):");
              // if (ImGui::Button("Find GPS (Auto)")) {
              //     uintptr_t srv = debugCam->GetGpsTextureSrv();
              // }
              // ImGui::SameLine();
              // if (ImGui::Button("Find Map (Path)")) {
              //     debugCam->GetTextureSrvByPath("/material/ui/dashboard/bare_map.tobj");
              // }
              // ImGui::SameLine();
              // if (ImGui::Button("Find Dash (Path)")) {
              //     debugCam->GetTextureSrvByPath("/vehicle/truck/share/dashboard.tobj");
              // }

              // if (ImGui::TreeNode("Mirrors Diagnostics")) {
              //     for (int m = 0; m < 7; m++) {
              //         char mLabel[32];
              //         sprintf_s(mLabel, "Mirror %d", m);
              //         if (ImGui::Button(mLabel)) {
              //             debugCam->GetMirrorTextureSrv(m);
              //         }
              //         if (m < 6) ImGui::SameLine();
              //     }
              //     ImGui::TreePop();
              // }

              // ImGui::Separator();
              // // Texture ID Selection with Buttons
              // int textureCount = debugCam->GetTextureCount();
              // int currentId = debugCam->GetSelectedTextureId();
              
              // ImGui::Text("Texture ID: %d / %d", currentId, textureCount);
              // if (ImGui::Button("-", ImVec2(30, 0))) {
              //   if (currentId > 0) debugCam->SetSelectedTextureId(currentId - 1);
              // }
              // ImGui::SameLine();
              // if (ImGui::Button("+", ImVec2(30, 0))) {
              //   if (currentId < textureCount - 1) debugCam->SetSelectedTextureId(currentId + 1);
              // }
              // ImGui::SameLine();
              
              // static bool skipEmpty = true;
              // ImGui::Checkbox("Skip Empty", &skipEmpty);
              
              // ImGui::SameLine();
              // static bool filterRtvSrv = false;
              // ImGui::Checkbox("Filter RTV+SRV", &filterRtvSrv);

              // if (skipEmpty) {
              //   ImGui::SameLine();
              //   if (ImGui::Button("Next Active")) {
              //     for (int i = currentId + 1; i < textureCount; ++i) {
              //       if (filterRtvSrv) {
              //         if (debugCam->IsRenderTargetImage(i)) {
              //           debugCam->SetSelectedTextureId(i);
              //           break;
              //         }
              //       } else if (debugCam->GetPipTextureSrv() != 0) {
              //         debugCam->SetSelectedTextureId(i);
              //         break;
              //       }
              //     }
              //   }
              // }

              // std::string pipComboLabel = "Select ID: " + std::to_string(currentId);
              // if (ImGui::BeginCombo("##PipIDCombo", pipComboLabel.c_str())) {
              //   for (int i = 0; i < textureCount; ++i) {
              //     if (filterRtvSrv && !debugCam->IsRenderTargetImage(i)) continue;
                  
              //     bool isSelected = (currentId == i);
              //     std::string name = "ID " + std::to_string(i);
              //     if (ImGui::Selectable(name.c_str(), isSelected)) {
              //       debugCam->SetSelectedTextureId(i);
              //     }
              //   }
              //   ImGui::EndCombo();
              // }

              ImGui::Separator();
              uintptr_t selectedObj = debugCam->GetSelectedObjectPtr();
              uintptr_t hoveredObj = debugCam->GetHoveredObjectPtr();

              ImGui::Text((loc.Get(m_locHoveredActor) + "  0x%p").c_str(), (void*)hoveredObj);
              ImGui::Text((loc.Get(m_locSelectedActor) + " 0x%p").c_str(), (void*)selectedObj);

              ImGui::Spacing();
              
              // Simple list selection
              auto& objectService = SPF::Data::GameData::GameObjectVehicleService::GetInstance();
              const auto vehicles = objectService.GetAllVehiclesFullInfo();
              static int listIdx = -1;
              
              std::string comboLabel = (listIdx != -1 && listIdx < vehicles.size()) ? loc.Get(m_locTabWheelCamera).c_str() + std::to_string(vehicles[listIdx].id) : loc.Get(m_locSelectFromList).c_str();
              if (ImGui::BeginCombo(loc.Get(m_locTrafficVehicles).c_str(), comboLabel.c_str())) {
                for (int i = 0; i < vehicles.size(); ++i) {
                  if (ImGui::Selectable((loc.Get(m_locTabWheelCamera).c_str() + std::to_string(vehicles[i].id)).c_str(), listIdx == i)) {
                    listIdx = i;
                  }
                }
                ImGui::EndCombo();
              }

              uintptr_t toCapture = (hoveredObj != 0) ? hoveredObj : ((listIdx != -1 && listIdx < vehicles.size()) ? vehicles[listIdx].pointer : 0);

              if (toCapture != 0) {
                const char* btnText = (hoveredObj != 0) ? loc.Get(m_locCaptureHovered).c_str() : loc.Get(m_locCaptureSelected).c_str();
                if (Button(btnText, TextStyle::DefaultButton(), ImVec2(-1, 0))) {
                  debugCam->SetSelectedObjectPtr(toCapture);
                }
              } else {
                ImGui::BeginDisabled();
                Button(loc.Get(m_locNoActorToCapture).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0));
                ImGui::EndDisabled();
              }

              ImGui::Spacing();
              uintptr_t myTruck = objectService.GetPlayerVehiclePtr();
              if (myTruck != 0) {
                if (Button(loc.Get(m_locCaptureMyTruck).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
                  debugCam->SetSelectedObjectPtr(myTruck);
                }
              } else {
                ImGui::BeginDisabled();
                Button(loc.Get(m_locMyTruckNotFound).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0));
                ImGui::EndDisabled();
              }

              ImGui::Separator();
              ImGui::Text("%s", loc.Get(m_locHUDPositionDebug).c_str());
              if (Button(loc.Get(m_locTopLeftDebug).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
                debugCam->SetHudPosition(DebugHudPosition::TopLeft);
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locBottomLeftDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::BottomLeft);
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locTopRightDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::TopRight);
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locBottomRightDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::BottomRight);
              }

              ImGui::SameLine();
              DebugHudPosition pos;
              if (debugCam->GetHudPosition(&pos)) {
                ImGui::Text(loc.Get(m_locCurrentDebug).c_str(), DebugHudPositionToString(pos));
              } else {
                ImGui::Text("%s", loc.Get(m_locCurrentNADebug).c_str());
              }
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locTrafficDebug).c_str())) {
              DebugCameraMode currentMode;
              if (debugCam->GetCurrentMode(&currentMode) && currentMode != DebugCameraMode::TRAFFIC) {
                debugCam->SetMode(DebugCameraMode::TRAFFIC);
              }
              ImGui::Text("%s", loc.Get(m_locCameraFocusesTraffic).c_str());
              ImGui::Separator();

              // 1. Prepare data (My Truck + Traffic)
              auto& objectService = SPF::Data::GameData::GameObjectVehicleService::GetInstance();
              uintptr_t myTruckPtr = objectService.GetPlayerVehiclePtr();
              auto trafficVehicles = objectService.GetAllVehiclesFullInfo();
              
              // Sort traffic by ID
              std::sort(trafficVehicles.begin(), trafficVehicles.end(), [](const auto& a, const auto& b) {
                return a.id < b.id;
              });

              struct Entry { int id; uintptr_t ptr; std::string label; bool is_mine; };
              std::vector<Entry> allEntries;
              if (myTruckPtr) allEntries.push_back({ -1, myTruckPtr, "ID: [MY TRUCK]", true });
              for (const auto& v : trafficVehicles) {
                allEntries.push_back({ v.id, v.pointer, "ID: " + std::to_string(v.id), false });
              }

              // UI state for selection
              static uintptr_t selectedPtr = 0;
              std::string preview = "None";
              int selectedIdx = -1;

              for (int i = 0; i < allEntries.size(); ++i) {
                if (allEntries[i].ptr == selectedPtr) {
                  preview = allEntries[i].label;
                  selectedIdx = i;
                  break;
                }
              }

              // 2. Render Selection Combo
              if (ImGui::BeginCombo(loc.Get(m_locSelectVehicle).c_str(), preview.c_str())) {
                for (int i = 0; i < allEntries.size(); ++i) {
                  if (ImGui::Selectable(allEntries[i].label.c_str(), selectedIdx == i)) {
                    selectedPtr = allEntries[i].ptr;
                  }
                }
                ImGui::EndCombo();
              }

              // 3. Display details and Capture button
              if (selectedPtr != 0) {
                ImGui::Separator();
                
                // Find if it's a traffic vehicle to show details
                const SPF::Data::GameData::GameObjectVehicleService::VehicleFullInfo* vInfo = nullptr;
                for (const auto& v : trafficVehicles) { if (v.pointer == selectedPtr) { vInfo = &v; break; } }

                if (vInfo) {
                  ImGui::Text(loc.Get(m_locVehicleDetailsTraffic).c_str(), vInfo->id);
                  ImGui::Text((loc.Get(m_locPointerLabel) + " 0x%p").c_str(), (void*)vInfo->pointer);
                  ImGui::Text((loc.Get(m_locPatienceLabel) + " %.2f").c_str(), vInfo->patience);
                  ImGui::Text((loc.Get(m_locSafetyLabel) + " %.2f").c_str(), vInfo->safety);
                  ImGui::Text((loc.Get(m_locTargetSpeedLabel) + " %.2f mph").c_str(), vInfo->target_speed * 2.23694f);
                  ImGui::Text((loc.Get(m_locSpeedLimitLabel) + " %.2f mph").c_str(), vInfo->speed_limit * 2.23694f);
                  ImGui::Text((loc.Get(m_locCurrentSpeedLabel) + " %.2f mph").c_str(), vInfo->current_speed * 2.23694f);
                  ImGui::Text((loc.Get(m_locAccelerationLabel) + " %.2f").c_str(), vInfo->acceleration);
                } else if (selectedPtr == myTruckPtr) {
                  ImGui::Text("%s", loc.Get(m_locVehicleDetailsMine).c_str());
                  ImGui::Text((loc.Get(m_locPointerLabel) + " 0x%p").c_str(), (void*)myTruckPtr);
                  
                  // Try to read speed/acceleration for player truck using VTable logic
                  float mySpeed = 0.0f;
                  float myAccel = 0.0f;
                  uintptr_t vtable_ptr = *(uintptr_t*)(myTruckPtr + 16);
                  if (vtable_ptr) {
                    uintptr_t* vtable = (uintptr_t*)vtable_ptr;
                    auto GetFloatFn = reinterpret_cast<float (*)(void*)>(vtable[1]);
                    auto GetAccelFn = reinterpret_cast<float (*)(void*)>(vtable[2]);
                    if (GetFloatFn) mySpeed = GetFloatFn((void*)(myTruckPtr + 16));
                    if (GetAccelFn) myAccel = GetAccelFn((void*)(myTruckPtr + 16));
                  }
                  
                  ImGui::Text((loc.Get(m_locCurrentSpeedLabel) + " %.2f mph").c_str(), mySpeed * 2.23694f);
                  ImGui::Text((loc.Get(m_locAccelerationLabel) + " %.2f").c_str(), myAccel);
                  ImGui::Text("%s", loc.Get(m_locStatusUserControlled).c_str());
                }
                
                ImGui::Spacing();
                
                if (Button(loc.Get(m_locCaptureSelectedVehicle).c_str(), TextStyle::DefaultButton(), ImVec2(-1, 0))) {
                  debugCam->SetSelectedObjectPtr(selectedPtr);
                }
              }
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locCinematicDebug).c_str())) {
              DebugCameraMode currentMode;
              if (debugCam->GetCurrentMode(&currentMode) && currentMode != DebugCameraMode::CINEMATIC) {
                debugCam->SetMode(DebugCameraMode::CINEMATIC);
              }
              ImGui::Text("%s", loc.Get(m_locCinematicCameraMode).c_str());
              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locAnimatedDebug).c_str())) {
              ImGui::Text("%s", loc.Get(m_locCreatePlayAnimations).c_str());
              ImGui::Separator();

              // Button to explicitly switch to this mode
              if (Button(loc.Get(m_locActivateGameAnimatedMode).c_str())) {
                debugCam->SetMode(DebugCameraMode::ANIMATED);
              }

              ImGui::Separator();
              ImGui::TextUnformatted(loc.Get(m_locCustomAnimationControls).c_str());

              // These static variables hold the UI state for this tab
              static float timeline_pos = 0.0f;
              static int current_item_index = 0;

              if (auto* animController = m_gameCameraService.GetDebugAnimationController()) {
                // --- State Synchronization ---
                // The controller is the single source of truth for the current position.
                // We only stop updating the UI from it when the user is actively dragging the slider.
                if (!ImGui::IsItemActive()) {
                  timeline_pos = static_cast<float>(animController->GetCurrentFrame()) + animController->GetCurrentFrameProgress();
                  current_item_index = animController->GetCurrentFrame();
                }

                // --- Controls ---
                const char* statusText;
                auto state = animController->GetPlaybackState();
                if (state == GameCamera::GameCameraDebugAnimation::PlaybackState::Playing) {
                  statusText = loc.Get(m_locPlayingStatus).c_str();
                  if (Button(loc.Get(m_locPauseButton).c_str())) {
                    animController->Pause();
                  }
                } else {
                  statusText = (state == GameCamera::GameCameraDebugAnimation::PlaybackState::Paused) ? loc.Get(m_locPausedStatus).c_str() : loc.Get(m_locStoppedStatus).c_str();
                  if (Button(loc.Get(m_locPlayButton).c_str())) {
                    animController->Play(current_item_index);
                  }
                }
                ImGui::SameLine();
                if (Button(loc.Get(m_locStopButton).c_str())) {
                  animController->Stop();
                  // After stopping, immediately sync the UI to the reset state
                  timeline_pos = 0.0f;
                  current_item_index = 0;
                }
                ImGui::SameLine();
                ImGui::Text(loc.Get(m_locStatusLabel).c_str(), statusText);

                static bool reverse = false;
                if (ImGui::Checkbox(loc.Get(m_locReversePlayback).c_str(), &reverse)) {
                  animController->SetReverse(reverse);
                }
              }

              ImGui::Separator();

              // Timeline/Scrubber
              if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                int stateCount = stateCam->GetStateCount();
                if (stateCount > 1) {
                  ImGui::Text("%s", loc.Get(m_locTimelineLabel).c_str());
                  ImGui::PushItemWidth(-1);
                  if (ImGui::SliderFloat("##Timeline", &timeline_pos, 0.0f, static_cast<float>(stateCount - 1) - 0.001f, "%.3f")) {
                    if (auto* animController = m_gameCameraService.GetDebugAnimationController()) {
                      animController->ScrubTo(timeline_pos);
                      current_item_index = static_cast<int>(timeline_pos);
                    }
                  }
                  ImGui::PopItemWidth();
                }
              }

              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locStateCameraDebug).c_str())) {
              static GameCamera::GameCameraDebugState::CameraState newState = {};
              static int current_item = 0;

              ImGui::Text("%s", loc.Get(m_locCreateStateCamera).c_str());
              ImGui::Separator();

              if (Button(loc.Get(m_locSaveKeyframe).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->SaveState();
                }
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locReloadFromFile).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->ReloadStatesFromFile();
                }
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locClearAllMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->ClearAllStatesInMemory();
                }
              }

              ImGui::Separator();
              ImGui::TextUnformatted(loc.Get(m_locAnimationControls).c_str());

              ImGui::Separator();

              // Continuously update the editor fields with live data from the free camera
              if (auto* freeCam = dynamic_cast<GameCameraFree*>(m_gameCameraService.GetCamera(GameCameraType::DeveloperFreeCamera))) {
                freeCam->GetPosition(&newState.pos_x, &newState.pos_y, &newState.pos_z);
                freeCam->GetFreecamMysteryFloat(&newState.internal_value);
                freeCam->GetQuaternion(&newState.q_x, &newState.q_y, &newState.q_z, &newState.q_w);
                freeCam->GetFov(&newState.fov);
              }

              ImGui::TextUnformatted(loc.Get(m_locAddEditState).c_str());
              ImGui::InputFloat3(loc.Get(m_locPositionXYZ).c_str(), &newState.pos_x);
              ImGui::InputFloat(loc.Get(m_locInternalValue).c_str(), &newState.internal_value);
              ImGui::InputFloat4(loc.Get(m_locQuaternionXYZW).c_str(), &newState.q_x);
              ImGui::InputFloat(loc.Get(m_locFOVLabel).c_str(), &newState.fov);
              if (Button(loc.Get(m_locAddStateMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->AddStateToMemory(newState);
                }
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locUpdateStateMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->EditStateInMemory(current_item, newState);
                }
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locDeleteStateMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->DeleteStateInMemory(current_item);
                  // Clamp current_item to be safe after deletion
                  int stateCount = stateCam->GetStateCount();
                  if (current_item >= stateCount && stateCount > 0) {
                    current_item = stateCount - 1;
                  } else if (stateCount == 0) {
                    current_item = 0;
                  }
                }
              }
              ImGui::Separator();

              if (Button(loc.Get(m_locPreviousState).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->CycleState(0);
                }
              }
              ImGui::SameLine();
              if (Button(loc.Get(m_locNextState).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->CycleState(1);
                }
              }

              ImGui::Separator();

              if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                if (auto* animController = m_gameCameraService.GetDebugAnimationController()) {
                  int stateCount = stateCam->GetStateCount();
                  int currentIndex = animController->GetCurrentFrame();

                  if (currentIndex >= 0 && currentIndex < stateCount) {
                    GameCamera::GameCameraDebugState::CameraState state_data;
                    if (stateCam->GetState(currentIndex, state_data)) {
                      ImGui::Text(loc.Get(m_locActiveStateLabel).c_str(), currentIndex);
                      ImGui::Text(loc.Get(m_locPosLabel).c_str(), state_data.pos_x, state_data.pos_y, state_data.pos_z);
                      ImGui::Text(loc.Get(m_locInternalLabel).c_str(), state_data.internal_value);
                      ImGui::Text(loc.Get(m_locQuatLabel).c_str(), state_data.q_x, state_data.q_y, state_data.q_z, state_data.q_w);
                      ImGui::Text(loc.Get(m_locFOVValueLabel).c_str(), state_data.fov);
                    }
                  } else {
                    ImGui::Text("%s", loc.Get(m_locActiveStateNone).c_str());
                  }
                }
              }

              ImGui::Separator();

              if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                int stateCount = stateCam->GetStateCount();
                ImGui::Text(loc.Get(m_locSavedStatesLabel).c_str(), stateCount);

                if (stateCount > 0) {
                  // Ensure current_item is valid if states were deleted
                  if (current_item >= stateCount) {
                      current_item = stateCount - 1;
                  }
                  std::string combo_label = loc.Get(m_locStatesComboLabel).c_str() + std::to_string(current_item);
                  if (ImGui::BeginCombo(loc.Get(m_locStatesComboLabel).c_str(), combo_label.c_str())) {
                    for (int i = 0; i < stateCount; ++i) {
                      bool is_selected = (current_item == i);
                      std::string item_name = loc.Get(m_locStateItemLabel).c_str() + std::to_string(i);
                      if (ImGui::Selectable(item_name.c_str(), is_selected)) {
                        current_item = i;
                        if (auto* animController = m_gameCameraService.GetDebugAnimationController()) {
                          animController->GoToFrame(i);
                        }
                      }
                      if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                      }
                    }
                    ImGui::EndCombo();
                  }
                } else {
                  // Display a disabled combo box or text if no states are saved
                  ImGui::TextDisabled("%s", loc.Get(m_locNoStatesSaved).c_str());
                }
              }

              // TODO: Add other controls like Apply, Cycle, Play/Pause here later

              ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.Get(m_locOversizeDebug).c_str())) {
              DebugCameraMode currentMode;
              if (debugCam->GetCurrentMode(&currentMode) && currentMode != DebugCameraMode::OVERSIZE) {
                debugCam->SetMode(DebugCameraMode::OVERSIZE);
              }
              ImGui::Text("%s", loc.Get(m_locCameraOversizedTrailers).c_str());
              ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locEnableDebugCameraToSelectMode).c_str());
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locDebugCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }
  }
  ImGui::EndTabBar();

  // if (m_showPipWindow) {
  //   auto* debugCam = m_gameCameraService.GetDebugCamera();
  //   if (debugCam) {
  //     uintptr_t srv = debugCam->GetPipTextureSrv();
  //     if (srv) {
  //       if (ImGui::Begin("PiP Texture Viewer", &m_showPipWindow)) {
  //         ImGui::Image((ImTextureID)srv, ImVec2(400, 225));
  //       }
  //       ImGui::End();
  //     }
  //   }
  // }

  m_needsTabSwitch = false;
}
}  // namespace UI
SPF_NS_END
