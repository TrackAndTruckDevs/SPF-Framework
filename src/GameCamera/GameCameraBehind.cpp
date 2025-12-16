#include "SPF/GameCamera/GameCameraBehind.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraBehind::GameCameraBehind() {
  // Constructor
}

void GameCameraBehind::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
  logger->Info("Activating Behind Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }
}

void GameCameraBehind::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
  logger->Info("Deactivating Behind Camera.");
  m_pCameraObject = nullptr;
}

void GameCameraBehind::Update(float dt) {
  if (!m_pCameraObject) return;

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
}

void GameCameraBehind::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float pitch, yaw, zoom;
  if (GetLiveState(&pitch, &yaw, &zoom)) {
    m_defaultCameraData.live_pitch = pitch;
    m_defaultCameraData.live_yaw = yaw;
    m_defaultCameraData.live_zoom = zoom;
  }

  float dist_min, dist_max, dist_trailer_max, dist_def, dist_trailer_def, dist_speed, dist_lazy;
  if (GetDistanceSettings(&dist_min, &dist_max, &dist_trailer_max, &dist_def, &dist_trailer_def, &dist_speed, &dist_lazy)) {
    m_defaultCameraData.distance_min = dist_min;
    m_defaultCameraData.distance_max = dist_max;
    m_defaultCameraData.distance_trailer_max_offset = dist_trailer_max;
    m_defaultCameraData.distance_default = dist_def;
    m_defaultCameraData.distance_trailer_default = dist_trailer_def;
    m_defaultCameraData.distance_change_speed = dist_speed;
    m_defaultCameraData.distance_laziness_speed = dist_lazy;
  }

  float elev_azimuth_lazy, elev_min, elev_max, elev_def, elev_trailer_def, elev_height_limit;
  if (GetElevationSettings(&elev_azimuth_lazy, &elev_min, &elev_max, &elev_def, &elev_trailer_def, &elev_height_limit)) {
    m_defaultCameraData.azimuth_laziness_speed = elev_azimuth_lazy;
    m_defaultCameraData.elevation_min = elev_min;
    m_defaultCameraData.elevation_max = elev_max;
    m_defaultCameraData.elevation_default = elev_def;
    m_defaultCameraData.elevation_trailer_default = elev_trailer_def;
    m_defaultCameraData.height_limit = elev_height_limit;
  }

  float pivot_x, pivot_y, pivot_z;
  if (GetPivot(&pivot_x, &pivot_y, &pivot_z)) {
    m_defaultCameraData.pivot_x = pivot_x;
    m_defaultCameraData.pivot_y = pivot_y;
    m_defaultCameraData.pivot_z = pivot_z;
  }

  float dyn_max, dyn_speed_min, dyn_speed_max, dyn_lazy;
  if (GetDynamicOffset(&dyn_max, &dyn_speed_min, &dyn_speed_max, &dyn_lazy)) {
    m_defaultCameraData.dynamic_offset_max = dyn_max;
    m_defaultCameraData.dynamic_offset_speed_min = dyn_speed_min;
    m_defaultCameraData.dynamic_offset_speed_max = dyn_speed_max;
    m_defaultCameraData.dynamic_offset_laziness_speed = dyn_lazy;
  }

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraBehind::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
  logger->Info("Resetting camera state to defaults...");

  SetLiveState(m_defaultCameraData.live_pitch, m_defaultCameraData.live_yaw, m_defaultCameraData.live_zoom);
  SetDistanceSettings(m_defaultCameraData.distance_min,
                      m_defaultCameraData.distance_max,
                      m_defaultCameraData.distance_trailer_max_offset,
                      m_defaultCameraData.distance_default,
                      m_defaultCameraData.distance_trailer_default,
                      m_defaultCameraData.distance_change_speed,
                      m_defaultCameraData.distance_laziness_speed);
  SetElevationSettings(m_defaultCameraData.azimuth_laziness_speed,
                       m_defaultCameraData.elevation_min,
                       m_defaultCameraData.elevation_max,
                       m_defaultCameraData.elevation_default,
                       m_defaultCameraData.elevation_trailer_default,
                       m_defaultCameraData.height_limit);
  SetPivot(m_defaultCameraData.pivot_x, m_defaultCameraData.pivot_y, m_defaultCameraData.pivot_z);
  SetDynamicOffset(m_defaultCameraData.dynamic_offset_max,
                   m_defaultCameraData.dynamic_offset_speed_min,
                   m_defaultCameraData.dynamic_offset_speed_max,
                   m_defaultCameraData.dynamic_offset_laziness_speed);
  SetFov(m_defaultCameraData.fov_base);
}

void GameCameraBehind::SetLiveState(float pitch, float yaw, float zoom) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto pitch_offset = gameData.GetBehindLivePitchOffset();
  auto yaw_offset = gameData.GetBehindLiveYawOffset();
  auto zoom_offset = gameData.GetBehindLiveZoomOffset();

  if (pitch_offset && yaw_offset && zoom_offset) {
    *reinterpret_cast<float*>(pCam + pitch_offset) = pitch;
    *reinterpret_cast<float*>(pCam + yaw_offset) = yaw;
    *reinterpret_cast<float*>(pCam + zoom_offset) = zoom;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set live state: one or more offsets are missing.");
  }
}

void GameCameraBehind::SetDistanceSettings(float min, float max, float trailer_max_offset, float def, float trailer_def, float change_speed, float laziness) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto min_offset = gameData.GetBehindDistanceMinOffset();
  auto max_offset = gameData.GetBehindDistanceMaxOffset();
  auto trailer_max_offset_val = gameData.GetBehindDistanceTrailerMaxOffset();
  auto def_offset = gameData.GetBehindDistanceDefaultOffset();
  auto trailer_def_offset = gameData.GetBehindDistanceTrailerDefaultOffset();
  auto change_speed_offset = gameData.GetBehindDistanceChangeSpeedOffset();
  auto laziness_offset = gameData.GetBehindDistanceLazinessSpeedOffset();

  if (min_offset && max_offset && trailer_max_offset_val && def_offset && trailer_def_offset && change_speed_offset && laziness_offset) {
    *reinterpret_cast<float*>(pCam + min_offset) = min;
    *reinterpret_cast<float*>(pCam + max_offset) = max;
    *reinterpret_cast<float*>(pCam + trailer_max_offset_val) = trailer_max_offset;
    *reinterpret_cast<float*>(pCam + def_offset) = def;
    *reinterpret_cast<float*>(pCam + trailer_def_offset) = trailer_def;
    *reinterpret_cast<float*>(pCam + change_speed_offset) = change_speed;
    *reinterpret_cast<float*>(pCam + laziness_offset) = laziness;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set distance settings: one or more offsets are missing.");
  }
}

void GameCameraBehind::SetElevationSettings(float azimuth_laziness, float min, float max, float def, float trailer_def, float height_limit) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto azimuth_laziness_offset = gameData.GetBehindAzimuthLazinessSpeedOffset();
  auto min_offset = gameData.GetBehindElevationMinOffset();
  auto max_offset = gameData.GetBehindElevationMaxOffset();
  auto def_offset = gameData.GetBehindElevationDefaultOffset();
  auto trailer_def_offset = gameData.GetBehindElevationTrailerDefaultOffset();
  auto height_limit_offset = gameData.GetBehindHeightLimitOffset();

  if (azimuth_laziness_offset && min_offset && max_offset && def_offset && trailer_def_offset && height_limit_offset) {
    *reinterpret_cast<float*>(pCam + azimuth_laziness_offset) = azimuth_laziness;
    *reinterpret_cast<float*>(pCam + min_offset) = min;
    *reinterpret_cast<float*>(pCam + max_offset) = max;
    *reinterpret_cast<float*>(pCam + def_offset) = def;
    *reinterpret_cast<float*>(pCam + trailer_def_offset) = trailer_def;
    *reinterpret_cast<float*>(pCam + height_limit_offset) = height_limit;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set elevation settings: one or more offsets are missing.");
  }
}

void GameCameraBehind::SetPivot(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetBehindPivotXOffset();
  auto y_offset = gameData.GetBehindPivotYOffset();
  auto z_offset = gameData.GetBehindPivotZOffset();

  if (x_offset && y_offset && z_offset) {
    *reinterpret_cast<float*>(pCam + x_offset) = x;
    *reinterpret_cast<float*>(pCam + y_offset) = y;
    *reinterpret_cast<float*>(pCam + z_offset) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set pivot: one or more offsets are missing.");
  }
}

void GameCameraBehind::SetDynamicOffset(float max, float speed_min, float speed_max, float laziness) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto max_offset = gameData.GetBehindDynamicOffsetMaxOffset();
  auto speed_min_offset = gameData.GetBehindDynamicOffsetSpeedMinOffset();
  auto speed_max_offset = gameData.GetBehindDynamicOffsetSpeedMaxOffset();
  auto laziness_offset = gameData.GetBehindDynamicOffsetLazinessSpeedOffset();

  if (max_offset && speed_min_offset && speed_max_offset && laziness_offset) {
    *reinterpret_cast<float*>(pCam + max_offset) = max;
    *reinterpret_cast<float*>(pCam + speed_min_offset) = speed_min;
    *reinterpret_cast<float*>(pCam + speed_max_offset) = speed_max;
    *reinterpret_cast<float*>(pCam + laziness_offset) = laziness;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set dynamic offset: one or more offsets are missing.");
  }
}

void GameCameraBehind::SetFov(float fov) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  // Get all required data first
  auto fov_base_offset = gameData.GetFovBaseOffset();  // Re-uses interior FOV offset
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_offset = gameData.GetViewportX1Offset();
  auto x2_offset = gameData.GetViewportX2Offset();
  auto y1_offset = gameData.GetViewportY1Offset();
  auto y2_offset = gameData.GetViewportY2Offset();

  // Check if everything is available
  if (fov_base_offset && pfnUpdateCameraProjection && pCameraParamsObject && x1_offset && x2_offset && y1_offset && y2_offset) {
    // 1. Set the base FOV value
    *reinterpret_cast<float*>(pCam + fov_base_offset) = fov;

    // 2. Calculate viewport parameters
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);

    // 3. Call the game's function to make the FOV change take effect
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraBehind");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

// --- New Safe Getters ---

bool GameCameraBehind::GetLiveState(float* out_pitch, float* out_yaw, float* out_zoom) const {
  if (!out_pitch || !out_yaw || !out_zoom) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto pitch_offset = gameData.GetBehindLivePitchOffset();
  auto yaw_offset = gameData.GetBehindLiveYawOffset();
  auto zoom_offset = gameData.GetBehindLiveZoomOffset();

  if (pitch_offset && yaw_offset && zoom_offset) {
    *out_pitch = *reinterpret_cast<float*>(pCam + pitch_offset);
    *out_yaw = *reinterpret_cast<float*>(pCam + yaw_offset);
    *out_zoom = *reinterpret_cast<float*>(pCam + zoom_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetDistanceSettings(float* out_min, float* out_max, float* out_trailer_max_offset, float* out_def, float* out_trailer_def, float* out_change_speed,
                                           float* out_laziness) const {
  if (!out_min || !out_max || !out_trailer_max_offset || !out_def || !out_trailer_def || !out_change_speed || !out_laziness) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto min_offset = gameData.GetBehindDistanceMinOffset();
  auto max_offset = gameData.GetBehindDistanceMaxOffset();
  auto trailer_max_offset_val = gameData.GetBehindDistanceTrailerMaxOffset();
  auto def_offset = gameData.GetBehindDistanceDefaultOffset();
  auto trailer_def_offset = gameData.GetBehindDistanceTrailerDefaultOffset();
  auto change_speed_offset = gameData.GetBehindDistanceChangeSpeedOffset();
  auto laziness_offset = gameData.GetBehindDistanceLazinessSpeedOffset();

  if (min_offset && max_offset && trailer_max_offset_val && def_offset && trailer_def_offset && change_speed_offset && laziness_offset) {
    *out_min = *reinterpret_cast<float*>(pCam + min_offset);
    *out_max = *reinterpret_cast<float*>(pCam + max_offset);
    *out_trailer_max_offset = *reinterpret_cast<float*>(pCam + trailer_max_offset_val);
    *out_def = *reinterpret_cast<float*>(pCam + def_offset);
    *out_trailer_def = *reinterpret_cast<float*>(pCam + trailer_def_offset);
    *out_change_speed = *reinterpret_cast<float*>(pCam + change_speed_offset);
    *out_laziness = *reinterpret_cast<float*>(pCam + laziness_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetElevationSettings(float* out_azimuth_laziness, float* out_min, float* out_max, float* out_def, float* out_trailer_def, float* out_height_limit) const {
  if (!out_azimuth_laziness || !out_min || !out_max || !out_def || !out_trailer_def || !out_height_limit) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto azimuth_laziness_offset = gameData.GetBehindAzimuthLazinessSpeedOffset();
  auto min_offset = gameData.GetBehindElevationMinOffset();
  auto max_offset = gameData.GetBehindElevationMaxOffset();
  auto def_offset = gameData.GetBehindElevationDefaultOffset();
  auto trailer_def_offset = gameData.GetBehindElevationTrailerDefaultOffset();
  auto height_limit_offset = gameData.GetBehindHeightLimitOffset();

  if (azimuth_laziness_offset && min_offset && max_offset && def_offset && trailer_def_offset && height_limit_offset) {
    *out_azimuth_laziness = *reinterpret_cast<float*>(pCam + azimuth_laziness_offset);
    *out_min = *reinterpret_cast<float*>(pCam + min_offset);
    *out_max = *reinterpret_cast<float*>(pCam + max_offset);
    *out_def = *reinterpret_cast<float*>(pCam + def_offset);
    *out_trailer_def = *reinterpret_cast<float*>(pCam + trailer_def_offset);
    *out_height_limit = *reinterpret_cast<float*>(pCam + height_limit_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetPivot(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetBehindPivotXOffset();
  auto y_offset = gameData.GetBehindPivotYOffset();
  auto z_offset = gameData.GetBehindPivotZOffset();

  if (x_offset && y_offset && z_offset) {
    *out_x = *reinterpret_cast<float*>(pCam + x_offset);
    *out_y = *reinterpret_cast<float*>(pCam + y_offset);
    *out_z = *reinterpret_cast<float*>(pCam + z_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetDynamicOffset(float* out_max, float* out_speed_min, float* out_speed_max, float* out_laziness) const {
  if (!out_max || !out_speed_min || !out_speed_max || !out_laziness) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto max_offset = gameData.GetBehindDynamicOffsetMaxOffset();
  auto speed_min_offset = gameData.GetBehindDynamicOffsetSpeedMinOffset();
  auto speed_max_offset = gameData.GetBehindDynamicOffsetSpeedMaxOffset();
  auto laziness_offset = gameData.GetBehindDynamicOffsetLazinessSpeedOffset();

  if (max_offset && speed_min_offset && speed_max_offset && laziness_offset) {
    *out_max = *reinterpret_cast<float*>(pCam + max_offset);
    *out_speed_min = *reinterpret_cast<float*>(pCam + speed_min_offset);
    *out_speed_max = *reinterpret_cast<float*>(pCam + speed_max_offset);
    *out_laziness = *reinterpret_cast<float*>(pCam + laziness_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetFov(float* out_fov) const {
  if (!out_fov) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto fov_base_offset = gameData.GetFovBaseOffset();  // Re-uses interior fov

  if (fov_base_offset) {
    *out_fov = *reinterpret_cast<float*>(pCam + fov_base_offset);
    return true;
  }
  return false;
}

bool GameCameraBehind::GetFinalFov(float* out_horiz, float* out_vert) const {
  if (!out_horiz || !out_vert) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto horiz_offset = gameData.GetFovHorizFinalOffset();  // Shared offset
  auto vert_offset = gameData.GetFovVertFinalOffset();    // Shared offset

  if (horiz_offset && vert_offset) {
    *out_horiz = *reinterpret_cast<float*>(pCam + horiz_offset);
    *out_vert = *reinterpret_cast<float*>(pCam + vert_offset);
    return true;
  }
  return false;
}
}  // namespace GameCamera
SPF_NS_END
