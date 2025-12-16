#include "SPF/UI/CameraWindow.hpp"
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

CameraWindow::CameraWindow(GameCameraManager& gameCameraService, const std::string& owner, const std::string& name)
    : BaseWindow(owner, name), m_gameCameraService(gameCameraService) {
    m_titleLocalizationKey = "camera_window.title";
    m_locCurrentCamera = "camera_window.current_camera";
    m_locCameraWorldCoordinates = "camera_window.camera_world_coordinates";
    m_locCameraWorldCoordinatesNotFound = "camera_window.camera_world_coordinates_not_found";
    m_locSelectCamera = "camera_window.select_camera";
    m_locInterior = "camera_window.interior";
    m_locBehind = "camera_window.behind";
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
    m_locTabCabinCamera = "camera_window.tabs.cabin_camera";
    m_locTabWindowCamera = "camera_window.tabs.window_camera";
    m_locTabBumperCamera = "camera_window.tabs.bumper_camera";
    m_locTabWheelCamera = "camera_window.tabs.wheel_camera";
    m_locTabTVCamera = "camera_window.tabs.tv_camera";
    m_locTabFreeCamera = "camera_window.tabs.free_camera";
    m_locTabDebug = "camera_window.tabs.debug";

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
    m_locSeatPositionNotFound = "camera_window.interior_camera.seat_position_not_found";
    m_locHeadRotation = "camera_window.interior_camera.head_rotation";
    m_locYawLr = "camera_window.interior_camera.yaw_lr";
    m_locPitchUd = "camera_window.interior_camera.pitch_ud";
    m_locHeadRotationNotFound = "camera_window.interior_camera.head_rotation_not_found";
    m_locMouseRotationLimits = "camera_window.interior_camera.mouse_rotation_limits";
    m_locLeftLimit = "camera_window.interior_camera.left_limit";
    m_locRightLimit = "camera_window.interior_camera.right_limit";
    m_locUpLimit = "camera_window.interior_camera.up_limit";
    m_locDownLimit = "camera_window.interior_camera.down_limit";
    m_locRotationLimitsNotFound = "camera_window.interior_camera.rotation_limits_not_found";
    m_locRotationDefaults = "camera_window.interior_camera.rotation_defaults";
    m_locDefaultLr = "camera_window.interior_camera.default_lr";
    m_locDefaultUd = "camera_window.interior_camera.default_ud";
    m_locRotationDefaultsNotFound = "camera_window.interior_camera.rotation_defaults_not_found";
    m_locResetToDefaults = "camera_window.interior_camera.reset_to_defaults";
    m_locInteriorCameraNotAvailable = "camera_window.interior_camera.not_available";

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
    m_locResetToDefaultsBehind = "camera_window.behind_camera.reset_to_defaults_behind";
    m_locBehindCameraNotAvailable = "camera_window.behind_camera.not_available";

    m_locHeightZoom = "camera_window.top_camera.height_zoom";
    m_locMinimumHeight = "camera_window.top_camera.minimum_height";
    m_locMaximumHeight = "camera_window.top_camera.maximum_height";
    m_locHeightZoomNotFound = "camera_window.top_camera.height_zoom_not_found";
    m_locMovement = "camera_window.top_camera.movement";
    m_locMovementSpeed = "camera_window.top_camera.movement_speed";
    m_locMovementNotFound = "camera_window.top_camera.movement_not_found";
    m_locDynamicOffsetTop = "camera_window.top_camera.dynamic_offset";
    m_locForwardOffset = "camera_window.top_camera.forward_offset";
    m_locBackwardOffset = "camera_window.top_camera.backward_offset";
    m_locDynamicOffsetNotFoundTop = "camera_window.top_camera.dynamic_offset_not_found";
    m_locBaseFovTop = "camera_window.top_camera.base_fov_top";
    m_locResetToDefaultsTop = "camera_window.top_camera.reset_to_defaults_top";
    m_locTopCameraNotAvailable = "camera_window.top_camera.not_available";

    m_locBaseFovCabin = "camera_window.cabin_camera.base_fov_cabin";
    m_locResetToDefaultsCabin = "camera_window.cabin_camera.reset_to_defaults_cabin";
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
    m_locResetToDefaultsWindow = "camera_window.window_camera.reset_to_defaults_window";
    m_locWindowCameraNotAvailable = "camera_window.window_camera.not_available";

    m_locOffsetBumper = "camera_window.bumper_camera.offset";
    m_locOffsetXBumper = "camera_window.bumper_camera.offset_x_bumper";
    m_locOffsetYBumper = "camera_window.bumper_camera.offset_y_bumper";
    m_locOffsetZBumper = "camera_window.bumper_camera.offset_z_bumper";
    m_locOffsetNotFoundBumper = "camera_window.bumper_camera.offset_not_found";
    m_locBaseFovBumper = "camera_window.bumper_camera.base_fov_bumper";
    m_locResetToDefaultsBumper = "camera_window.bumper_camera.reset_to_defaults_bumper";
    m_locBumperCameraNotAvailable = "camera_window.bumper_camera.not_available";

    m_locOffsetWheel = "camera_window.wheel_camera.offset";
    m_locOffsetXWheel = "camera_window.wheel_camera.offset_x_wheel";
    m_locOffsetYWheel = "camera_window.wheel_camera.offset_y_wheel";
    m_locOffsetZWheel = "camera_window.wheel_camera.offset_z_wheel";
    m_locOffsetNotFoundWheel = "camera_window.wheel_camera.offset_not_found";
    m_locBaseFovWheel = "camera_window.wheel_camera.base_fov_wheel";
    m_locResetToDefaultsWheel = "camera_window.wheel_camera.reset_to_defaults_wheel";
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
    m_locResetToDefaultsTV = "camera_window.tv_camera.reset_to_defaults_tv";
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
    m_locResetToDefaultsFreeCam = "camera_window.free_camera.reset_to_defaults_freecam";
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
    m_locMysteryFloat = "camera_window.debug.mystery_float";
    m_locQuaternionXYZW = "camera_window.debug.quaternion_xyzw";
    m_locFOVLabel = "camera_window.debug.fov_label";
    m_locAddStateMemory = "camera_window.debug.add_state_memory";
    m_locUpdateStateMemory = "camera_window.debug.update_state_memory";
    m_locDeleteStateMemory = "camera_window.debug.delete_state_memory";
    m_locPreviousState = "camera_window.debug.previous_state";
    m_locNextState = "camera_window.debug.next_state";
    m_locActiveStateLabel = "camera_window.debug.active_state_label";
    m_locPosLabel = "camera_window.debug.pos_label";
    m_locMysteryLabel = "camera_window.debug.mystery_label";
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
}

const char* CameraWindow::GetWindowTitle() const {
    return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str();
}

void CameraWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
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

  if (ImGui::Button(loc.Get(m_locInterior).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::InteriorCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::InteriorCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locBehind).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::BehindCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::BehindCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locTop).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::TopCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::TopCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locCabin).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::CabinCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::CabinCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locWindow).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::WindowCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::WindowCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locBumper).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::BumperCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::BumperCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locWheel).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::WheelCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::WheelCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locTV).c_str())) {
    m_gameCameraService.SwitchTo(GameCameraType::TVCamera);
    m_needsTabSwitch = true;
    m_activeTabType = GameCameraType::TVCamera;
  }
  ImGui::SameLine(0.0f, 5.0f);
  if (ImGui::Button(loc.Get(m_locDeveloperFreeCamera).c_str())) {
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
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (interiorCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFov).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            interiorCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (interiorCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locSeatPosition).c_str());
        float seat_x, seat_y, seat_z;
        if (interiorCam->GetSeatPosition(&seat_x, &seat_y, &seat_z)) {
          bool seatChanged = false;
          seatChanged |= ImGui::SliderFloat(loc.Get(m_locSeatLr).c_str(), &seat_x, -0.5f, 0.5f, "%.3f");
          seatChanged |= ImGui::SliderFloat(loc.Get(m_locSeatUd).c_str(), &seat_y, -0.2f, 0.2f, "%.3f");
          seatChanged |= ImGui::SliderFloat(loc.Get(m_locSeatFb).c_str(), &seat_z, -0.5f, 0.5f, "%.3f");
          if (seatChanged) {
            interiorCam->SetSeatPosition(seat_x, seat_y, seat_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locSeatPositionNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locHeadRotation).c_str());
        float yaw, pitch;
        if (interiorCam->GetHeadRotation(&yaw, &pitch)) {
          bool rotChanged = false;
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locYawLr).c_str(), &yaw, -3.14159f, 3.14159f, "%.3f");
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locPitchUd).c_str(), &pitch, -1.57079f, 1.57079f, "%.3f");
          if (rotChanged) {
            interiorCam->SetHeadRotation(yaw, pitch);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locHeadRotationNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locMouseRotationLimits).c_str());
        float lim_l, lim_r, lim_u, lim_d;
        if (interiorCam->GetRotationLimits(&lim_l, &lim_r, &lim_u, &lim_d)) {
          bool limitChanged = false;
          limitChanged |= ImGui::SliderFloat(loc.Get(m_locLeftLimit).c_str(), &lim_l, -360.0f, 360.0f, "%.1f");
          limitChanged |= ImGui::SliderFloat(loc.Get(m_locRightLimit).c_str(), &lim_r, -360.0f, 360.0f, "%.1f");
          limitChanged |= ImGui::SliderFloat(loc.Get(m_locUpLimit).c_str(), &lim_u, -180.0f, 180.0f, "%.1f");
          limitChanged |= ImGui::SliderFloat(loc.Get(m_locDownLimit).c_str(), &lim_d, -180.0f, 180.0f, "%.1f");
          if (limitChanged) {
            interiorCam->SetRotationLimits(lim_l, lim_r, lim_u, lim_d);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locRotationLimitsNotFound).c_str());
        }

        float mouse_lr_default, mouse_ud_default;
        if (interiorCam->GetRotationDefaults(&mouse_lr_default, &mouse_ud_default)) {
          bool defaultsChanged = false;
          defaultsChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultLr).c_str(), &mouse_lr_default, 0.0f, 360.0f, "%.1f");
          defaultsChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultUd).c_str(), &mouse_ud_default, -90.0f, 90.0f, "%.1f");
          if (defaultsChanged) {
            interiorCam->SetRotationDefaults(mouse_lr_default, mouse_ud_default);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locRotationDefaultsNotFound).c_str());
        }
        ImGui::Separator();
        if (ImGui::Button(loc.Get(m_locResetToDefaults).c_str(), ImVec2(-1, 0))) {
          interiorCam->ResetToDefaults();
        }
      } else {
        ImGui::TextDisabled("%s", loc.Get(m_locInteriorCameraNotAvailable).c_str());
      }
      ImGui::EndTabItem();
    }

    ImGuiTabItemFlags behindTabFlags = ImGuiTabItemFlags_None;
    if (m_needsTabSwitch && m_activeTabType == GameCameraType::BehindCamera) {
      behindTabFlags = ImGuiTabItemFlags_SetSelected;
    }
    if (ImGui::BeginTabItem(loc.Get(m_locTabBehindCamera).c_str(), nullptr, behindTabFlags)) {
      auto* pCamera = m_gameCameraService.GetCamera(GameCameraType::BehindCamera);
      if (auto* behindCam = dynamic_cast<GameCameraBehind*>(pCamera)) {
        ImGui::Text("%s", loc.Get(m_locLiveState).c_str());
        float pitch, yaw, zoom;
        if (behindCam->GetLiveState(&pitch, &yaw, &zoom)) {
          bool liveChanged = false;
          liveChanged |= ImGui::SliderFloat(loc.Get(m_locLivePitch).c_str(), &pitch, -1.57f, 1.57f, "%.4f");
          liveChanged |= ImGui::SliderFloat(loc.Get(m_locLiveYaw).c_str(), &yaw, -3.14f, 3.14f, "%.4f");
          liveChanged |= ImGui::SliderFloat(loc.Get(m_locLiveZoom).c_str(), &zoom, 0.0f, 50.0f, "%.1f");
          if (liveChanged) {
            behindCam->SetLiveState(pitch, yaw, zoom);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locLiveStateNotFound).c_str());
        }
        ImGui::Separator();

        ImGui::Text("%s", loc.Get(m_locDistanceZoomSettings).c_str());
        float dist_min, dist_max, dist_trailer_max, dist_def, dist_trailer_def, dist_speed, dist_lazy;
        if (behindCam->GetDistanceSettings(&dist_min, &dist_max, &dist_trailer_max, &dist_def, &dist_trailer_def, &dist_speed, &dist_lazy)) {
          bool distChanged = false;
          distChanged |= ImGui::SliderFloat(loc.Get(m_locMinDistance).c_str(), &dist_min, 0.0f, 50.0f, "%.1f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locMaxDistance).c_str(), &dist_max, 0.0f, 50.0f, "%.1f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locTrailerMaxOffset).c_str(), &dist_trailer_max, 0.0f, 10.0f, "%.1f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultDistance).c_str(), &dist_def, 0.0f, 50.0f, "%.1f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locTrailerDefaultDist).c_str(), &dist_trailer_def, 0.0f, 50.0f, "%.1f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locZoomSpeed).c_str(), &dist_speed, 0.1f, 5.0f, "%.2f");
          distChanged |= ImGui::SliderFloat(loc.Get(m_locDistanceLaziness).c_str(), &dist_lazy, 0.1f, 10.0f, "%.2f");
          if (distChanged) {
            behindCam->SetDistanceSettings(dist_min, dist_max, dist_trailer_max, dist_def, dist_trailer_def, dist_speed, dist_lazy);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locDistanceZoomSettingsNotFound).c_str());
        }
        ImGui::Separator();

        ImGui::Text("%s", loc.Get(m_locElevationPitchSettings).c_str());
        float elev_min, elev_max, elev_def, elev_trailer_def, height_limit, azimuth_lazy;
        if (behindCam->GetElevationSettings(&azimuth_lazy, &elev_min, &elev_max, &elev_def, &elev_trailer_def, &height_limit)) {
          bool elevChanged = false;
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locAzimuthLaziness).c_str(), &azimuth_lazy, 0.1f, 10.0f, "%.2f");
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locMinElevation).c_str(), &elev_min, -90.0f, 90.0f, "%.1f");
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locMaxElevation).c_str(), &elev_max, 0.0f, 90.0f, "%.1f");
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locDefaultElevation).c_str(), &elev_def, 0.0f, 90.0f, "%.1f");
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locTrailerDefaultElev).c_str(), &elev_trailer_def, 0.0f, 90.0f, "%.1f");
          elevChanged |= ImGui::SliderFloat(loc.Get(m_locHeightLimit).c_str(), &height_limit, 0.0f, 50.0f, "%.1f");
          if (elevChanged) {
            behindCam->SetElevationSettings(azimuth_lazy, elev_min, elev_max, elev_def, elev_trailer_def, height_limit);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locElevationPitchSettingsNotFound).c_str());
        }
        ImGui::Separator();

        ImGui::Text("%s", loc.Get(m_locPivotOffset).c_str());
        float pivot_x, pivot_y, pivot_z;
        if (behindCam->GetPivot(&pivot_x, &pivot_y, &pivot_z)) {
          bool pivotChanged = false;
          pivotChanged |= ImGui::SliderFloat(loc.Get(m_locPivotX).c_str(), &pivot_x, -5.0f, 5.0f, "%.2f");
          pivotChanged |= ImGui::SliderFloat(loc.Get(m_locPivotY).c_str(), &pivot_y, -5.0f, 5.0f, "%.2f");
          pivotChanged |= ImGui::SliderFloat(loc.Get(m_locPivotZ).c_str(), &pivot_z, -5.0f, 5.0f, "%.2f");
          if (pivotChanged) {
            behindCam->SetPivot(pivot_x, pivot_y, pivot_z);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locPivotOffsetNotFound).c_str());
        }
        ImGui::Separator();

        ImGui::Text("%s", loc.Get(m_locDynamicOffset).c_str());
        float dyn_max, dyn_speed_min, dyn_speed_max, dyn_lazy;
        if (behindCam->GetDynamicOffset(&dyn_max, &dyn_speed_min, &dyn_speed_max, &dyn_lazy)) {
          bool dynChanged = false;
          dynChanged |= ImGui::SliderFloat(loc.Get(m_locMaxDynamicOffset).c_str(), &dyn_max, 0.0f, 10.0f, "%.2f");
          dynChanged |= ImGui::SliderFloat(loc.Get(m_locDynOffsetSpeedMin).c_str(), &dyn_speed_min, 0.0f, 100.0f, "%.1f");
          dynChanged |= ImGui::SliderFloat(loc.Get(m_locDynOffsetSpeedMax).c_str(), &dyn_speed_max, 0.0f, 100.0f, "%.1f");
          dynChanged |= ImGui::SliderFloat(loc.Get(m_locDynOffsetLaziness).c_str(), &dyn_lazy, 0.1f, 5.0f, "%.2f");
          if (dynChanged) {
            behindCam->SetDynamicOffset(dyn_max, dyn_speed_min, dyn_speed_max, dyn_lazy);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locDynamicOffsetNotFound).c_str());
        }
        ImGui::Separator();

        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (behindCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovBehind).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            behindCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (behindCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();

        if (ImGui::Button(loc.Get(m_locResetToDefaultsBehind).c_str(), ImVec2(-1, 0))) {
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
        ImGui::Text("%s", loc.Get(m_locHeightZoom).c_str());
        float min_h, max_h;
        if (topCam->GetHeight(&min_h, &max_h)) {
          bool heightChanged = false;
          heightChanged |= ImGui::SliderFloat(loc.Get(m_locMinimumHeight).c_str(), &min_h, 1.0f, 50.0f, "%.1f");
          heightChanged |= ImGui::SliderFloat(loc.Get(m_locMaximumHeight).c_str(), &max_h, 1.0f, 100.0f, "%.1f");
          if (heightChanged) {
            topCam->SetHeight(min_h, max_h);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locHeightZoomNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locMovement).c_str());
        float speed;
        if (topCam->GetSpeed(&speed)) {
          if (ImGui::SliderFloat(loc.Get(m_locMovementSpeed).c_str(), &speed, 0.1f, 10.0f, "%.2f")) {
            topCam->SetSpeed(speed);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locMovementNotFound).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locDynamicOffsetTop).c_str());
        float offset_f, offset_b;
        if (topCam->GetOffsets(&offset_f, &offset_b)) {
          bool offsetChanged = false;
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locForwardOffset).c_str(), &offset_f, -20.0f, 20.0f, "%.2f");
          offsetChanged |= ImGui::SliderFloat(loc.Get(m_locBackwardOffset).c_str(), &offset_b, -20.0f, 20.0f, "%.2f");
          if (offsetChanged) {
            topCam->SetOffsets(offset_f, offset_b);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locDynamicOffsetNotFoundTop).c_str());
        }

        ImGui::Separator();
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (topCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovTop).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            topCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (topCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (ImGui::Button(loc.Get(m_locResetToDefaultsTop).c_str(), ImVec2(-1, 0))) {
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
        ImGui::Text("%s", loc.Get(m_locFovZoom).c_str());
        float fov;
        if (cabinCam->GetFov(&fov)) {
          if (ImGui::SliderFloat(loc.Get(m_locBaseFovCabin).c_str(), &fov, 20.0f, 120.0f, "%.1f")) {
            cabinCam->SetFov(fov);
          }
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locBaseFovNotFound).c_str());
        }
        float h_fov, v_fov;
        if (cabinCam->GetFinalFov(&h_fov, &v_fov)) {
          ImGui::Text(loc.Get(m_locFinalHFov).c_str(), h_fov);
          ImGui::Text(loc.Get(m_locFinalVFov).c_str(), v_fov);
        } else {
          ImGui::TextDisabled("%s", loc.Get(m_locFinalFovNotFound).c_str());
        }

        ImGui::Separator();
        if (ImGui::Button(loc.Get(m_locResetToDefaultsCabin).c_str(), ImVec2(-1, 0))) {
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
        if (ImGui::Button(loc.Get(m_locResetToDefaultsWindow).c_str(), ImVec2(-1, 0))) {
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
        if (ImGui::Button(loc.Get(m_locResetToDefaultsBumper).c_str(), ImVec2(-1, 0))) {
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
        if (ImGui::Button(loc.Get(m_locResetToDefaultsWheel).c_str(), ImVec2(-1, 0))) {
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
        if (ImGui::Button(loc.Get(m_locResetToDefaultsTV).c_str(), ImVec2(-1, 0))) {
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
          rotChanged |= ImGui::SliderFloat(loc.Get(m_locRollFreeCam).c_str(), &roll, -3.14159f, 3.14159f, "%.4f");
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
        if (ImGui::Button(loc.Get(m_locResetToDefaultsFreeCam).c_str(), ImVec2(-1, 0))) {
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
              ImGui::Text("%s", loc.Get(m_locHUDPositionDebug).c_str());
              if (ImGui::Button(loc.Get(m_locTopLeftDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::TopLeft);
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locBottomLeftDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::BottomLeft);
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locTopRightDebug).c_str())) {
                debugCam->SetHudPosition(DebugHudPosition::TopRight);
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locBottomRightDebug).c_str())) {
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

              // Get vehicle data once per frame
              auto& objectService = SPF::Data::GameData::GameObjectVehicleService::GetInstance();
              const auto vehicles = objectService.GetAllVehiclesFullInfo();

              // UI state for the dropdown
              static int selectedVehicleIndex = -1;
              static std::string selectedVehicleLabel; // Changed to static std::string to persist its memory.

              // Ensure index is valid if the number of vehicles changes
              if (selectedVehicleIndex != -1 && selectedVehicleIndex < vehicles.size()) {
                selectedVehicleLabel = "ID: " + std::to_string(vehicles[selectedVehicleIndex].id);
              } else {
                selectedVehicleLabel = "None"; // Default value when nothing is selected or index is invalid.
                selectedVehicleIndex = -1; // Reset index if invalid
              }

              // Create the dropdown
              if (ImGui::BeginCombo("Select Vehicle", selectedVehicleLabel.c_str())) {
                for (int i = 0; i < vehicles.size(); ++i) {
                  const bool is_selected = (selectedVehicleIndex == i);
                  std::string label = "ID: " + std::to_string(vehicles[i].id);
                  if (ImGui::Selectable(label.c_str(), is_selected)) {
                    selectedVehicleIndex = i;
                  }
                  if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                  }
                }
                ImGui::EndCombo();
              }

              // Display details for the selected vehicle
              if (selectedVehicleIndex != -1) {
                const auto& vehicle = vehicles[selectedVehicleIndex];
                ImGui::Separator();
                ImGui::Text("Selected Vehicle Details:");
                ImGui::Text("  ID: %d", vehicle.id);
                ImGui::Text("  Pointer: 0x%p", (void*)vehicle.pointer);
                ImGui::Text("  Patience: %.2f", vehicle.patience);
                ImGui::Text("  Safety: %.2f", vehicle.safety);
                ImGui::Text("  Target Speed: %.2f mph", vehicle.target_speed * 2.23694f);
                ImGui::Text("  Speed Limit: %.2f mph", vehicle.speed_limit * 2.23694f);
                ImGui::Text("  Current Speed: %.2f mph", vehicle.current_speed * 2.23694f);
                ImGui::Text("  Acceleration: %.2f", vehicle.acceleration);
                
                ImGui::Spacing();
                
                if (ImGui::Button("Select this Vehicle")) {
                    // TODO: Implement the call to DebugCamera_SetSelectedActor.
                    // We need to find the DebugCamera object pointer and the function pointer first.
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
              if (ImGui::Button(loc.Get(m_locActivateGameAnimatedMode).c_str())) {
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
                  if (ImGui::Button(loc.Get(m_locPauseButton).c_str())) {
                    animController->Pause();
                  }
                } else {
                  statusText = (state == GameCamera::GameCameraDebugAnimation::PlaybackState::Paused) ? loc.Get(m_locPausedStatus).c_str() : loc.Get(m_locStoppedStatus).c_str();
                  if (ImGui::Button(loc.Get(m_locPlayButton).c_str())) {
                    animController->Play(current_item_index);
                  }
                }
                ImGui::SameLine();
                if (ImGui::Button(loc.Get(m_locStopButton).c_str())) {
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

              if (ImGui::Button(loc.Get(m_locSaveKeyframe).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->SaveState();
                }
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locReloadFromFile).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->ReloadStatesFromFile();
                }
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locClearAllMemory).c_str())) {
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
                freeCam->GetFreecamMysteryFloat(&newState.mystery_float);
                freeCam->GetQuaternion(&newState.q_x, &newState.q_y, &newState.q_z, &newState.q_w);
                freeCam->GetFov(&newState.fov);
              }

              ImGui::TextUnformatted(loc.Get(m_locAddEditState).c_str());
              ImGui::InputFloat3(loc.Get(m_locPositionXYZ).c_str(), &newState.pos_x);
              ImGui::InputFloat(loc.Get(m_locMysteryFloat).c_str(), &newState.mystery_float);
              ImGui::InputFloat4(loc.Get(m_locQuaternionXYZW).c_str(), &newState.q_x);
              ImGui::InputFloat(loc.Get(m_locFOVLabel).c_str(), &newState.fov);
              if (ImGui::Button(loc.Get(m_locAddStateMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->AddStateToMemory(newState);
                }
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locUpdateStateMemory).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->EditStateInMemory(current_item, newState);
                }
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locDeleteStateMemory).c_str())) {
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

              if (ImGui::Button(loc.Get(m_locPreviousState).c_str())) {
                if (auto* stateCam = m_gameCameraService.GetDebugStateCamera()) {
                  stateCam->CycleState(0);
                }
              }
              ImGui::SameLine();
              if (ImGui::Button(loc.Get(m_locNextState).c_str())) {
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
                      ImGui::Text(loc.Get(m_locMysteryLabel).c_str(), state_data.mystery_float);
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
  m_needsTabSwitch = false;
}
}  // namespace UI
SPF_NS_END
