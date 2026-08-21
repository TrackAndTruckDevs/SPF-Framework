#include "SPF/Data/GameData/GameDataCameraService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/BumperCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/CabinCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraAnimationDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraStateDataFinder.hpp"
#include "SPF/Data/GameData/Finders/FovDataFinder.hpp"
#include "SPF/Data/GameData/Finders/FreeCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/InteriorCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/TopCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"
#include "SPF/Data/GameData/Finders/WheelCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"
#include "SPF/Data/GameData/ManagerCoreService.hpp"
#include "SPF/Data/GameData/WorldServiceRegistry.hpp"
#include "SPF/GameCamera/GameCameraType.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>
#include <cstring>
#include <memory>

SPF_NS_BEGIN
namespace Data::GameData {

GameDataCameraService::GameDataCameraService() { WorldServiceRegistry::Get().Register(this); }

GameDataCameraService& GameDataCameraService::GetInstance() {
  static GameDataCameraService instance;
  return instance;
}

void GameDataCameraService::RegisterFinders() {
  m_dataFinders.push_back(std::make_unique<Finders::CoreCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::FreeCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::InteriorCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::FovDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::ViewportDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::BehindCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::TopCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::CabinCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::WindowCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::BumperCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::WheelCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::TVCameraDataFinder>());
  // m_dataFinders.push_back(std::make_unique<Finders::PhotoCameraDataFinder>());  //for Photo Camera
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraStateDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraAnimationDataFinder>());
}

void GameDataCameraService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  logger->Info("Initializing Camera Data Service...");

  m_verifiedCameras.clear();
  RegisterFinders();

  m_isInitialized = false;
  m_coreOffsetsFound = false;

  logger->Info("Camera Data Service logic registered. Starting pattern scan sequence.");
}

bool GameDataCameraService::TryFindAllOffsets() {
  if (m_isInitialized) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  bool all_critical_found = true;

  // GameDataCameraService depends on CameraManager (resolved by ManagerCoreService).
  // Do not resolve offsets until the manager is available to avoid null dereferences.
  if (!ManagerCoreService::GetInstance().IsCameraManagerReady()) {
    logger->Warn("GameDataCameraService: CameraManager not resolved yet. Waiting for ManagerCoreService.");
    return false;
  }

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("[Success] Finder '{}' completed successfully.", finder->GetName());
      } else {
        // Log failure for every finder so we know exactly what's missing
        logger->Warn("[Failed] Finder '{}' could not resolve all patterns. Will retry on next tick.", finder->GetName());

        // CoreCameraDataFinder is mandatory. Without it, we can't even find the manager.
        if (strcmp(finder->GetName(), "CoreCameraDataFinder") == 0) {
          logger->Critical("CRITICAL FAILURE: CoreCameraDataFinder is missing! Camera system is offline.");
          all_critical_found = false;
        }
      }
    }
  }

  // We are ready only if the core system is found
  if (all_critical_found && m_coreOffsetsFound) {
    m_isInitialized = true;
    logger->Info("Camera Data Service successfully initialized (Core found).");
    return true;
  } else {
    // Report detailed failure status at the end of the pass
    logger->Error("Camera Data Service NOT ready. Status -> CriticalFinders: {}, CoreOffsetsFlag: {}", all_critical_found ? "OK" : "FAILED", m_coreOffsetsFound ? "OK" : "FALSE");
  }

  return m_isInitialized;
}

void GameDataCameraService::RegisterDiscoveredAddress(int slotIndex, uintptr_t address) { m_discoveredAddresses[slotIndex] = address; }

uintptr_t GameDataCameraService::GetDiscoveredAddress(int slotIndex) const {
  auto it = m_discoveredAddresses.find(slotIndex);
  if (it != m_discoveredAddresses.end()) {
    return it->second;
  }
  return 0;
}

void GameDataCameraService::RegisterVerifiedCamera(GameCamera::GameCameraType type, uintptr_t address) { m_verifiedCameras[type] = address; }

uintptr_t GameDataCameraService::GetVerifiedCamera(GameCamera::GameCameraType type) const {
  auto it = m_verifiedCameras.find(type);
  if (it != m_verifiedCameras.end()) {
    return it->second;
  }
  return 0;
}

void GameDataCameraService::UpdateFinders() {
  if (m_isInitialized) return;
  TryFindAllOffsets();
}

bool GameDataCameraService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;
}

bool GameDataCameraService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) return false;
  }
  return true;
}

void GameDataCameraService::Shutdown() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  logger->Info("Shutting down Camera Data Service and clearing all cache...");

  m_isInitialized = false;
  m_coreOffsetsFound = false;
  m_verifiedCameras.clear();
  m_dataFinders.clear();

  // Reset ALL internal pointers and offsets to 0
  m_cameraArrayOffset = 0;
  m_activeCameraIdOffset = 0;
  m_pFreeCamSpeed = nullptr;
  m_pCameraWorldCoordinatesPtr = nullptr;
  m_camera_fov_offset = 0;
  m_near_plane_offset = 0;
  m_far_plane_offset = 0;
  m_mouse_sensitivity_offset = 0;
  m_shake_anim_step_offset = 0;
  m_shake_anim_scale_min_offset = 0;
  m_shake_anim_scale_max_offset = 0;
  m_shake_anim_offset = 0;
  m_hand_shake_limit_offset = 0;
  m_hand_shake_speed_offset = 0;
  m_fov_base_offset = 0;
  m_fov_horiz_final_offset = 0;
  m_fov_vert_final_offset = 0;
  m_interior_seat_x_offset = 0;
  m_interior_seat_y_offset = 0;
  m_interior_seat_z_offset = 0;
  m_interior_yaw_offset = 0;
  m_interior_pitch_offset = 0;
  m_interior_limit_left_offset = 0;
  m_interior_limit_right_offset = 0;
  m_interior_limit_up_offset = 0;
  m_interior_limit_down_offset = 0;
  m_interior_outside_offset = 0;
  m_interior_mouse_lr_default = 0;
  m_interior_mouse_ud_default = 0;
  m_interior_azimuth_overrides_offset = 0;
  m_zoom_fov_factor_offset = 0;
  m_zoom_speed_offset = 0;
  m_azimuth_range_outside_offset = 0;
  m_azimuth_range_start_azimuth_offset = 0;
  m_azimuth_range_end_azimuth_offset = 0;
  m_azimuth_range_start_up_limit_offset = 0;
  m_azimuth_range_end_up_limit_offset = 0;
  m_azimuth_range_start_down_limit_offset = 0;
  m_azimuth_range_end_down_limit_offset = 0;
  m_azimuth_range_start_up_down_default_offset = 0;
  m_azimuth_range_end_up_down_default_offset = 0;
  m_azimuth_range_start_left_right_default_offset = 0;
  m_azimuth_range_end_left_right_default_offset = 0;
  m_azimuth_range_start_head_offset_offset = 0;
  m_azimuth_range_end_head_offset_offset = 0;
  m_pCameraParamsObject = 0;
  m_viewport_x1_offset = 0;
  m_viewport_x2_offset = 0;
  m_viewport_y1_offset = 0;
  m_viewport_y2_offset = 0;
  m_behind_live_pitch_offset = 0;
  m_behind_live_yaw_offset = 0;
  m_behind_live_zoom_offset = 0;
  m_behind_distance_min_offset = 0;
  m_behind_distance_max_offset = 0;
  m_behind_distance_trailer_max_offset = 0;
  m_behind_distance_default_offset = 0;
  m_behind_distance_trailer_default_offset = 0;
  m_behind_distance_change_speed_offset = 0;
  m_behind_distance_laziness_speed_offset = 0;
  m_behind_azimuth_laziness_speed_offset = 0;
  m_behind_elevation_min_offset = 0;
  m_behind_elevation_max_offset = 0;
  m_behind_elevation_default_offset = 0;
  m_behind_elevation_trailer_default_offset = 0;
  m_behind_height_limit_offset = 0;
  m_behind_pivot_x_offset = 0;
  m_behind_pivot_y_offset = 0;
  m_behind_pivot_z_offset = 0;
  m_behind_dynamic_offset_max_offset = 0;
  m_behind_dynamic_offset_speed_min_offset = 0;
  m_behind_dynamic_offset_speed_max_offset = 0;
  m_behind_dynamic_offset_laziness_speed_offset = 0;
  m_behind_validation_offset = 0;
  m_behind_validation_speed_positive_offset = 0;
  m_behind_validation_speed_negative_offset = 0;
  m_behind_validation_radius_offset = 0;
  m_behind_speed_fov_change_factor_offset = 0;
  m_top_min_height_offset = 0;
  m_top_max_height_offset = 0;
  m_top_speed_offset = 0;
  m_top_x_offset_forward_offset = 0;
  m_top_x_offset_backward_offset = 0;
  m_top_offset_forward_offset = 0;
  m_top_offset_backward_offset = 0;
  m_top_camera_height_factor_offset = 0;
  m_top_use_adaptive_camera_height_offset = 0;
  m_top_near_plane_offset = 0;
  m_top_far_plane_offset = 0;
  m_top_validation_offset = 0;
  m_top_validation_speed_positive_offset = 0;
  m_top_validation_speed_negative_offset = 0;
  m_window_head_offset_x = 0;
  m_window_head_offset_y = 0;
  m_window_head_offset_z = 0;
  m_window_live_yaw = 0;
  m_window_live_pitch = 0;
  m_window_mouse_left_limit = 0;
  m_window_mouse_right_limit = 0;
  m_window_mouse_lr_default = 0;
  m_window_mouse_up_limit = 0;
  m_window_mouse_down_limit = 0;
  m_window_mouse_ud_default = 0;
  m_bumper_offset_x = 0;
  m_bumper_offset_y = 0;
  m_bumper_offset_z = 0;
  m_wheel_offset_x = 0;
  m_wheel_offset_y = 0;
  m_wheel_offset_z = 0;
  m_tv_max_distance = 0;
  m_tv_prefab_uplift_x = 0;
  m_tv_prefab_uplift_y = 0;
  m_tv_prefab_uplift_z = 0;
  m_tv_road_uplift_x = 0;
  m_tv_road_uplift_y = 0;
  m_tv_road_uplift_z = 0;
  // m_photo_live_pitch_offset = 0;
  // m_photo_live_yaw_offset = 0;
  // m_photo_live_roll_offset = 0;
  // m_photo_live_zoom_offset = 0;
  // m_photo_pos_x_offset = 0;
  // m_photo_pos_y_offset = 0;
  // m_photo_pos_z_offset = 0;
  m_freecam_pos_x_offset = 0;
  m_freecam_pos_y_offset = 0;
  m_freecam_pos_z_offset = 0;
  m_freecam_quat_x_offset = 0;
  m_freecam_quat_y_offset = 0;
  m_freecam_quat_z_offset = 0;
  m_freecam_quat_w_offset = 0;
  m_freecam_internal_value_offset = 0;
  m_freecam_mouse_x_offset = 0;
  m_freecam_mouse_y_offset = 0;
  m_freecam_roll_offset = 0;
  m_pDebugCameraContext = 0;
  m_pfnSetDebugCameraMode = nullptr;
  m_pfnSetSelectedActor = nullptr;
  m_pfnSetPositionLock = nullptr;
  m_pfnSetRotationLock = nullptr;
  m_pfnSetOrbitMode = nullptr;
  m_pCacheableCvarObject = 0;
  m_cvarValueOffset = 0;
  m_debugCameraModeOffset = 0;
  m_debugPosLockOffset = 0;
  m_debugRotLockOffset = 0;
  m_debugOrbitOffset = 0;
  m_debugSelectedObjectPtrOffset = 0;
  m_debugOrbitSpeedOffset = 0;
  m_debugHoveredObjectPtrOffset = 0;
  m_pfnSetHudVisibility = nullptr;
  m_pfnSetDebugHudPosition = nullptr;
  m_hudVisibleOffset = 0;
  m_hudPositionOffset = 0;
  m_gameUiVisibleOffset = 0;
  m_pfnAddCameraState = nullptr;
  m_stateContextOffset = 0;
  m_pfnCycleSavedState = nullptr;
  m_pfnApplyState = nullptr;
  m_pfnLoadStatesFromFile = nullptr;
  m_pfnOpenFileForCameraState = nullptr;
  m_pfnFormatAndWriteCameraState = nullptr;
  m_stateArrayOffset = 0;
  m_stateCountOffset = 0;
  m_stateCurrentIndexOffset = 0;
  m_pfnUpdateAnimatedFlight = nullptr;
  m_animationTimerOffset = 0;
}

void GameDataCameraService::Reset() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  logger->Info("Resetting Camera Data Service for world reload...");

  m_isInitialized = false;
  m_coreOffsetsFound = false;
  m_verifiedCameras.clear();
  m_discoveredAddresses.clear();

  // Clear cached world pointers (they become dangling after a reload).
  // Static offsets and function pointers stay valid and are re-found by
  // the finders during the next TryFindAllOffsets() pass anyway.
  m_pFreeCamSpeed = nullptr;
  m_pCameraWorldCoordinatesPtr = nullptr;
  m_pDebugCameraContext = 0;
  m_pCacheableCvarObject = 0;
  m_pCameraParamsObject = 0;

  // Reset finder readiness so every finder re-scans in the new world.
  for (const auto& finder : m_dataFinders) {
    finder->Reset();
  }
}
}  // namespace Data::GameData
SPF_NS_END
