#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

// Include the new finder implementations
#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/FreeCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/InteriorCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/FovDataFinder.hpp"
#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"
#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/TopCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/CabinCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/BumperCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/WheelCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraStateDataFinder.hpp"
#include "SPF/Data/GameData/Finders/DebugCameraAnimationDataFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData {
GameDataCameraService::GameDataCameraService() = default;

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
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraStateDataFinder>());
  m_dataFinders.push_back(std::make_unique<Finders::DebugCameraAnimationDataFinder>());
}

void GameDataCameraService::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  logger->Info("Initializing Game Data (Camera) Service...");

  RegisterFinders();

  m_isInitialized = false;     // Initially not ready, will be set by TryFindAllOffsets
  m_coreOffsetsFound = false;  // Reset core offsets status
  logger->Info("Game Data (Camera) Service initialization finished. Waiting for critical offsets.");
}

bool GameDataCameraService::TryFindAllOffsets() {
  if (m_isInitialized) return true;  // Already found everything

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
  bool all_critical_found_this_pass = true;

  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady())  // Only try to find if not already ready
    {
      if (finder->TryFindOffsets(*this)) {
        logger->Info("-> Finder '{}' succeeded.", finder->GetName());
      } else {
        logger->Warn("-> Finder '{}' failed. Will retry.", finder->GetName());
        if (strcmp(finder->GetName(), "CoreCameraDataFinder") == 0) {
          all_critical_found_this_pass = false;  // Core finder is critical
        }
      }
    }
  }

  if (all_critical_found_this_pass && m_coreOffsetsFound) {  // Check m_coreOffsetsFound explicitly
    m_isInitialized = true;
    logger->Info("All critical camera offsets found. Service is fully initialized.");
    return true;
  }

  return m_isInitialized;
}

bool GameDataCameraService::IsFinderReady(const char* name) const {
  for (const auto& finder : m_dataFinders) {
    if (strcmp(finder->GetName(), name) == 0) {
      return finder->IsReady();
    }
  }
  return false;  // Finder with that name not found
}

bool GameDataCameraService::AreAllFindersReady() const {
  for (const auto& finder : m_dataFinders) {
    if (!finder->IsReady()) {
      return false;
    }
  }
  return true;
}

void GameDataCameraService::Shutdown() {
  if (m_isInitialized) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameDataCameraService");
    logger->Info("Uninstalling Game Data (Camera) Service...");
    m_isInitialized = false;

    // Clear all data members to their initial state
    m_pStandardManagerPtrAddr = 0;
    m_activeCameraIdOffset = 0;
    m_pFreecamGlobalObjectPtr = nullptr;
    m_freecamContextOffset = 0;
    m_interior_seat_x_offset = 0;
    m_interior_seat_y_offset = 0;
    m_interior_seat_z_offset = 0;
    m_interior_yaw_offset = 0;
    m_interior_pitch_offset = 0;
    m_interior_limit_left_offset = 0;
    m_interior_limit_right_offset = 0;
    m_interior_limit_up_offset = 0;
    m_interior_limit_down_offset = 0;
    m_fov_base_offset = 0;
    m_fov_horiz_final_offset = 0;
    m_fov_vert_final_offset = 0;
    m_interior_mouse_lr_default = 0;
    m_interior_mouse_ud_default = 0;

    m_freecam_pos_x_offset = 0;
    m_freecam_pos_y_offset = 0;
    m_freecam_pos_z_offset = 0;
    m_freecam_mouse_x_offset = 0;
    m_freecam_mouse_y_offset = 0;
    m_freecam_roll_offset = 0;
    m_pFreeCamSpeed = nullptr;

    m_pCameraWorldCoordinatesPtr = nullptr;

    m_pCameraParamsObject = 0;
    m_viewport_x1_offset = 0;
    m_viewport_x2_offset = 0;
    m_viewport_y1_offset = 0;
    m_viewport_y2_offset = 0;

    m_top_min_height_offset = 0;
    m_top_max_height_offset = 0;
    m_top_speed_offset = 0;
    m_top_x_offset_forward_offset = 0;
    m_top_x_offset_backward_offset = 0;

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

    m_pDebugCameraContext = 0;
    m_pfnSetDebugCameraMode = nullptr;

    m_pfnSetHudVisibility = nullptr;
    m_pfnSetDebugHudPosition = nullptr;
    m_hudVisibleOffset = 0;
    m_hudPositionOffset = 0;
    m_gameUiVisibleOffset = 0;

    // Clear state data
    m_pfnAddCameraState = nullptr;
    m_stateContextOffset = 0;
    m_pfnCycleSavedState = nullptr;
    m_pfnApplyState = nullptr;
    m_stateArrayOffset = 0;
    m_stateCountOffset = 0;
    m_stateCurrentIndexOffset = 0;

    // Clear animation data
    m_pfnUpdateAnimatedFlight = nullptr;
    m_animationTimerOffset = 0;

    // Clear the finders vector
    m_dataFinders.clear();
  }
}

}  // namespace Data::GameData
SPF_NS_END