#include "SPF/GameCamera/GameCameraInterior.hpp"
#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraInterior::GameCameraInterior() {
  // Constructor
}

void GameCameraInterior::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
  logger->Info("Activating Interior Camera.");

  // Get the raw camera object pointer when this camera becomes active
  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  // GetStandardManager() handles pointer dereferencing and version-specific adjustments (e.g. v1.59).
  uintptr_t pStandardManager = gameData.GetStandardManager();

  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
    if (m_pCameraObject) {
      logger->Debug("DIAGNOSTIC: Interior Camera Object Address: 0x{:X}", reinterpret_cast<uintptr_t>(m_pCameraObject));
    }
  }
}

void GameCameraInterior::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
  logger->Info("Deactivating Interior Camera.");
  m_pCameraObject = nullptr;  // Clear the pointer when not active
}

void GameCameraInterior::Update(float dt) {
  if (!m_pCameraObject) {
    return;  // Do nothing if the camera object isn't resolved
  }

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
}

void GameCameraInterior::SetSeatPosition(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetInteriorSeatXOffset();
  auto y_offset = gameData.GetInteriorSeatYOffset();
  auto z_offset = gameData.GetInteriorSeatZOffset();

  if (x_offset && y_offset && z_offset) {
    *reinterpret_cast<float*>(pCam + x_offset) = x;
    *reinterpret_cast<float*>(pCam + y_offset) = y;
    *reinterpret_cast<float*>(pCam + z_offset) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
    logger->Warn("Cannot set seat position: one or more offsets are missing.");
  }
}

void GameCameraInterior::SetHeadRotation(float yaw, float pitch) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto yaw_offset = gameData.GetInteriorYawOffset();
  auto pitch_offset = gameData.GetInteriorPitchOffset();

  if (yaw_offset && pitch_offset) {
    *reinterpret_cast<float*>(pCam + yaw_offset) = yaw;
    *reinterpret_cast<float*>(pCam + pitch_offset) = pitch;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
    logger->Warn("Cannot set head rotation: one or more offsets are missing.");
  }
}

void GameCameraInterior::SetFov(float fov) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  // Get all required data first
  auto fov_base_offset = gameData.GetFovBaseOffset();
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_offset = gameData.GetViewportX1Offset();
  auto x2_offset = gameData.GetViewportX2Offset();
  auto y1_offset = gameData.GetViewportY1Offset();
  auto y2_offset = gameData.GetViewportY2Offset();

  // Check if everything is available
  if (fov_base_offset && pfnUpdateCameraProjection && pCameraParamsObject && x1_offset && x2_offset && y1_offset && y2_offset) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");

    // 1. Set the base FOV value
    *reinterpret_cast<float*>(pCam + fov_base_offset) = fov;

    // 2. Calculate viewport parameters
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);

    // 3. Call the game's function to make the FOV change take effect
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);

    // Log final results
    // float horiz_final = *reinterpret_cast<float*>(pCam + gameData.GetFovHorizFinalOffset());
    // float vert_final = *reinterpret_cast<float*>(pCam + gameData.GetFovVertFinalOffset());

    // logger->Trace("SetFov [Result]: H={:.4f}, V={:.4f}", horiz_final, vert_final);
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraInterior::SetRotationLimits(float left, float right, float up, float down) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto left_offset = gameData.GetInteriorLimitLeftOffset();
  auto right_offset = gameData.GetInteriorLimitRightOffset();
  auto up_offset = gameData.GetInteriorLimitUpOffset();
  auto down_offset = gameData.GetInteriorLimitDownOffset();

  if (left_offset && right_offset && up_offset && down_offset) {
    *reinterpret_cast<float*>(pCam + left_offset) = left;
    *reinterpret_cast<float*>(pCam + right_offset) = right;
    *reinterpret_cast<float*>(pCam + up_offset) = up;
    *reinterpret_cast<float*>(pCam + down_offset) = down;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
    logger->Warn("Cannot set rotation limits: one or more offsets are missing.");
  }
}

void GameCameraInterior::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) {
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  float seat_x, seat_y, seat_z;
  if (GetSeatPosition(&seat_x, &seat_y, &seat_z)) {
    m_defaultCameraData.seat_pos_x = seat_x;
    m_defaultCameraData.seat_pos_y = seat_y;
    m_defaultCameraData.seat_pos_z = seat_z;
  }

  float yaw_val, pitch_val;
  if (GetHeadRotation(&yaw_val, &pitch_val)) {
    m_defaultCameraData.yaw = yaw_val;
    m_defaultCameraData.pitch = pitch_val;
  }

  float lim_l, lim_r, lim_u, lim_d;
  if (GetRotationLimits(&lim_l, &lim_r, &lim_u, &lim_d)) {
    m_defaultCameraData.limit_left = lim_l;
    m_defaultCameraData.limit_right = lim_r;
    m_defaultCameraData.limit_up = lim_u;
    m_defaultCameraData.limit_down = lim_d;
  }

  float lr_def, ud_def;
  if (GetRotationDefaults(&lr_def, &ud_def)) {
    m_defaultCameraData.mouse_lr_default = lr_def;
    m_defaultCameraData.mouse_ud_default = ud_def;
  }

  // --- Capture Advanced Camera Data (v1.2) ---
  float val;
  if (GetNearPlane(&val)) m_defaultCameraData.near_plane = val;
  if (GetFarPlane(&val)) m_defaultCameraData.far_plane = val;
  if (GetMouseSensitivity(&val)) m_defaultCameraData.mouse_sensitivity = val;
  if (GetShakeAnimStep(&val)) m_defaultCameraData.shake_step = val;
  if (GetShakeAnimScaleMin(&val)) m_defaultCameraData.shake_min = val;
  if (GetShakeAnimScaleMax(&val)) m_defaultCameraData.shake_max = val;
  
  if (GetHandShakeLimit(&val)) m_defaultCameraData.hand_shake_limit = val;
  if (GetHandShakeSpeed(&val)) m_defaultCameraData.hand_shake_speed = val;
  
  if (GetZoomFovFactor(&val)) m_defaultCameraData.zoom_fov_factor = val;
  if (GetZoomSpeed(&val)) m_defaultCameraData.zoom_speed = val;

  // --- Store Array Defaults ---
  m_defaultCameraData.azimuth_overrides_defaults.clear();
  size_t azimuth_count = GetAzimuthOverridesCount();
  for (size_t i = 0; i < azimuth_count; ++i) {
      CameraData::AzimuthRangeData range = {};
      GetAzimuthOverrideStartAzimuth(i, &range.start_azimuth);
      GetAzimuthOverrideEndAzimuth(i, &range.end_azimuth);
      GetAzimuthOverrideOutside(i, &range.outside);
      GetAzimuthOverrideStartUpLimit(i, &range.start_up_limit);
      GetAzimuthOverrideEndUpLimit(i, &range.end_up_limit);
      GetAzimuthOverrideStartDownLimit(i, &range.start_down_limit);
      GetAzimuthOverrideEndDownLimit(i, &range.end_down_limit);
      GetAzimuthOverrideStartUpDownDefault(i, &range.start_up_down_default);
      GetAzimuthOverrideEndUpDownDefault(i, &range.end_up_down_default);
      GetAzimuthOverrideStartLeftRightDefault(i, &range.start_left_right_default);
      GetAzimuthOverrideEndLeftRightDefault(i, &range.end_left_right_default);
      GetAzimuthOverrideStartHeadOffset(i, &range.start_head_x, &range.start_head_y, &range.start_head_z);
      GetAzimuthOverrideEndHeadOffset(i, &range.end_head_x, &range.end_head_y, &range.end_head_z);
      m_defaultCameraData.azimuth_overrides_defaults.push_back(range);
  }

  m_defaultCameraData.shake_anim_defaults.clear();
  size_t shake_count = GetShakeAnimCount();
  for (size_t i = 0; i < shake_count; ++i) {
      CameraData::Vec3 point = {};
      GetShakeAnim(i, &point.x, &point.y, &point.z);
      m_defaultCameraData.shake_anim_defaults.push_back(point);
  }

  // Mark defaults as saved
  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored (including arrays).");
}

void GameCameraInterior::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) {
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
  logger->Info("Resetting camera state to defaults...");

  SetSeatPosition(m_defaultCameraData.seat_pos_x, m_defaultCameraData.seat_pos_y, m_defaultCameraData.seat_pos_z);
  SetHeadRotation(m_defaultCameraData.yaw, m_defaultCameraData.pitch);
  SetFov(m_defaultCameraData.fov_base);
  SetRotationLimits(m_defaultCameraData.limit_left, m_defaultCameraData.limit_right, m_defaultCameraData.limit_up, m_defaultCameraData.limit_down);
  SetRotationDefaults(m_defaultCameraData.mouse_lr_default, m_defaultCameraData.mouse_ud_default);

  // --- Reset Advanced Data ---
  SetNearPlane(m_defaultCameraData.near_plane);
  SetFarPlane(m_defaultCameraData.far_plane);
  SetMouseSensitivity(m_defaultCameraData.mouse_sensitivity);
  SetShakeAnimStep(m_defaultCameraData.shake_step);
  SetShakeAnimScaleMin(m_defaultCameraData.shake_min);
  SetShakeAnimScaleMax(m_defaultCameraData.shake_max);
  SetHandShakeLimit(m_defaultCameraData.hand_shake_limit);
  SetHandShakeSpeed(m_defaultCameraData.hand_shake_speed);
  SetZoomFovFactor(m_defaultCameraData.zoom_fov_factor);
  SetZoomSpeed(m_defaultCameraData.zoom_speed);

  // --- Restore Array Defaults ---
  size_t current_azimuth_count = GetAzimuthOverridesCount();
  size_t saved_azimuth_count = m_defaultCameraData.azimuth_overrides_defaults.size();
  size_t azimuth_restore_count = (current_azimuth_count < saved_azimuth_count) ? current_azimuth_count : saved_azimuth_count;

  for (size_t i = 0; i < azimuth_restore_count; ++i) {
      const auto& range = m_defaultCameraData.azimuth_overrides_defaults[i];
      SetAzimuthOverrideStartAzimuth(i, range.start_azimuth);
      SetAzimuthOverrideEndAzimuth(i, range.end_azimuth);
      SetAzimuthOverrideOutside(i, range.outside);
      SetAzimuthOverrideStartUpLimit(i, range.start_up_limit);
      SetAzimuthOverrideEndUpLimit(i, range.end_up_limit);
      SetAzimuthOverrideStartDownLimit(i, range.start_down_limit);
      SetAzimuthOverrideEndDownLimit(i, range.end_down_limit);
      SetAzimuthOverrideStartUpDownDefault(i, range.start_up_down_default);
      SetAzimuthOverrideEndUpDownDefault(i, range.end_up_down_default);
      SetAzimuthOverrideStartLeftRightDefault(i, range.start_left_right_default);
      SetAzimuthOverrideEndLeftRightDefault(i, range.end_left_right_default);
      SetAzimuthOverrideStartHeadOffset(i, range.start_head_x, range.start_head_y, range.start_head_z);
      SetAzimuthOverrideEndHeadOffset(i, range.end_head_x, range.end_head_y, range.end_head_z);
  }

  size_t current_shake_count = GetShakeAnimCount();
  size_t saved_shake_count = m_defaultCameraData.shake_anim_defaults.size();
  size_t shake_restore_count = (current_shake_count < saved_shake_count) ? current_shake_count : saved_shake_count;

  for (size_t i = 0; i < shake_restore_count; ++i) {
      const auto& point = m_defaultCameraData.shake_anim_defaults[i];
      SetShakeAnim(i, point.x, point.y, point.z);
  }
}

// --- New Safe Getters ---

bool GameCameraInterior::GetSeatPosition(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetInteriorSeatXOffset();
  auto y_offset = gameData.GetInteriorSeatYOffset();
  auto z_offset = gameData.GetInteriorSeatZOffset();

  if (x_offset && y_offset && z_offset) {
    *out_x = *reinterpret_cast<float*>(pCam + x_offset);
    *out_y = *reinterpret_cast<float*>(pCam + y_offset);
    *out_z = *reinterpret_cast<float*>(pCam + z_offset);
    return true;
  }
  return false;
}

bool GameCameraInterior::GetHeadRotation(float* out_yaw, float* out_pitch) const {
  if (!out_yaw || !out_pitch) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto yaw_offset = gameData.GetInteriorYawOffset();
  auto pitch_offset = gameData.GetInteriorPitchOffset();

  if (yaw_offset && pitch_offset) {
    *out_yaw = *reinterpret_cast<float*>(pCam + yaw_offset);
    *out_pitch = *reinterpret_cast<float*>(pCam + pitch_offset);
    return true;
  }
  return false;
}

bool GameCameraInterior::GetFov(float* out_fov) const {
  if (!out_fov) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto fov_base_offset = gameData.GetFovBaseOffset();

  if (fov_base_offset) {
    *out_fov = *reinterpret_cast<float*>(pCam + fov_base_offset);
    return true;
  }
  return false;
}

bool GameCameraInterior::GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const {
  if (!out_left || !out_right || !out_up || !out_down) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto left_offset = gameData.GetInteriorLimitLeftOffset();
  auto right_offset = gameData.GetInteriorLimitRightOffset();
  auto up_offset = gameData.GetInteriorLimitUpOffset();
  auto down_offset = gameData.GetInteriorLimitDownOffset();

  if (left_offset && right_offset && up_offset && down_offset) {
    *out_left = *reinterpret_cast<float*>(pCam + left_offset);
    *out_right = *reinterpret_cast<float*>(pCam + right_offset);
    *out_up = *reinterpret_cast<float*>(pCam + up_offset);
    *out_down = *reinterpret_cast<float*>(pCam + down_offset);
    return true;
  }
  return false;
}

bool GameCameraInterior::GetFinalFov(float* out_horiz, float* out_vert) const {
  if (!out_horiz || !out_vert) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto horiz_offset = gameData.GetFovHorizFinalOffset();
  auto vert_offset = gameData.GetFovVertFinalOffset();

  if (horiz_offset && vert_offset) {
    *out_horiz = *reinterpret_cast<float*>(pCam + horiz_offset);
    *out_vert = *reinterpret_cast<float*>(pCam + vert_offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetRotationDefaults(float lr, float ud) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto lr_offset = gameData.GetInteriorMouseLRDefaultOffset();
  auto ud_offset = gameData.GetInteriorMouseUDDefaultOffset();

  if (lr_offset && ud_offset) {
    *reinterpret_cast<float*>(pCam + lr_offset) = lr;
    *reinterpret_cast<float*>(pCam + ud_offset) = ud;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
    logger->Warn("Cannot set rotation defaults: one or more offsets are missing.");
  }
}

bool GameCameraInterior::GetRotationDefaults(float* out_lr, float* out_ud) const {
  if (!out_lr || !out_ud) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto lr_offset = gameData.GetInteriorMouseLRDefaultOffset();
  auto ud_offset = gameData.GetInteriorMouseUDDefaultOffset();

  if (lr_offset && ud_offset) {
    *out_lr = *reinterpret_cast<float*>(pCam + lr_offset);
    *out_ud = *reinterpret_cast<float*>(pCam + ud_offset);
    return true;
  }
  return false;
}

bool GameCameraInterior::GetOutside(bool* out_val) const {
  if (!out_val || !m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetInteriorOutsideOffset();

  if (offset) {
    // The bool type in reflection has a size of 4 bytes (0x39), so we read it as uint32_t
    *out_val = (*reinterpret_cast<uint32_t*>(pCam + offset) != 0);
    return true;
  }
  return false;
}

void GameCameraInterior::SetOutside(bool val) {
  if (!m_pCameraObject) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetInteriorOutsideOffset();

  if (offset) {
    // Write as a 4-byte integer
    *reinterpret_cast<uint32_t*>(pCam + offset) = val ? 1 : 0;
  }
}

bool GameCameraInterior::GetNearPlane(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetNearPlaneOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetNearPlane(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetNearPlaneOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetFarPlane(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetFarPlaneOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetFarPlane(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetFarPlaneOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetMouseSensitivity(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetMouseSensitivityOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetMouseSensitivity(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetMouseSensitivityOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetShakeAnimStep(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimStepOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetShakeAnimStep(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimStepOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetShakeAnimScaleMin(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMinOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetShakeAnimScaleMin(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMinOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetShakeAnimScaleMax(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMaxOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetShakeAnimScaleMax(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMaxOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetHandShakeLimit(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetHandShakeLimitOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetHandShakeLimit(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetHandShakeLimitOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetHandShakeSpeed(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetHandShakeSpeedOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetHandShakeSpeed(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetHandShakeSpeedOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetZoomFovFactor(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetZoomFovFactorOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetZoomFovFactor(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetZoomFovFactorOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

bool GameCameraInterior::GetZoomSpeed(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetZoomSpeedOffset();
  if (offset) {
    *out_val = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

void GameCameraInterior::SetZoomSpeed(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetZoomSpeedOffset();
  if (offset) *reinterpret_cast<float*>(pCam + offset) = val;
}

// --- Public API for Azimuth Overrides ---

size_t GameCameraInterior::GetAzimuthOverridesCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetInteriorAzimuthOverridesOffset();
  if (!offset) return 0;

  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  // Number of elements lies behind the offset: azimuthOverrides + 16
  return static_cast<size_t>(*reinterpret_cast<uint64_t*>(pCam + offset + 16));
}

void* GameCameraInterior::GetAzimuthOverrideAddress(size_t index) const {
  if (!m_pCameraObject) return nullptr;
  if (index >= GetAzimuthOverridesCount()) return nullptr;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetInteriorAzimuthOverridesOffset();
  if (!offset) return nullptr;

  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  // The pointer to the beginning of the array lies at the offset: azimuthOverrides + 8
  void** data = *reinterpret_cast<void***>(pCam + offset + 8);
  if (!data) return nullptr;

  return data[index];
}

//start_azimuth
bool GameCameraInterior::GetAzimuthOverrideStartAzimuth(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartAzimuthOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartAzimuth(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartAzimuthOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//end_azimuth
bool GameCameraInterior::GetAzimuthOverrideEndAzimuth(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndAzimuthOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndAzimuth(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndAzimuthOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//outside
bool GameCameraInterior::GetAzimuthOverrideOutside(size_t index, bool* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeOutsideOffset();
  if (!offset) return false;

  // The bool type in reflection has a size of 4 bytes (0x39), so we read it as uint32_t
  *out_val = (*reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(addr) + offset) != 0);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideOutside(size_t index, bool val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeOutsideOffset();
  if (!offset) return;

  // Write as a 4-byte integer
  *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(addr) + offset) = val ? 1 : 0;
}

//start_up_limit
bool GameCameraInterior::GetAzimuthOverrideStartUpLimit(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartUpLimitOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartUpLimit(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartUpLimitOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//end_up_limit
bool GameCameraInterior::GetAzimuthOverrideEndUpLimit(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndUpLimitOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndUpLimit(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndUpLimitOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//start_down_limit
bool GameCameraInterior::GetAzimuthOverrideStartDownLimit(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartDownLimitOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartDownLimit(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartDownLimitOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//end_down_limit
bool GameCameraInterior::GetAzimuthOverrideEndDownLimit(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndDownLimitOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndDownLimit(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndDownLimitOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//start_up_down_default
bool GameCameraInterior::GetAzimuthOverrideStartUpDownDefault(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartUpDownDefaultOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartUpDownDefault(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartUpDownDefaultOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//end_up_down_default
bool GameCameraInterior::GetAzimuthOverrideEndUpDownDefault(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndUpDownDefaultOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndUpDownDefault(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndUpDownDefaultOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//start_left_right_default
bool GameCameraInterior::GetAzimuthOverrideStartLeftRightDefault(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartLeftRightDefaultOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartLeftRightDefault(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartLeftRightDefaultOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//end_left_right_default
bool GameCameraInterior::GetAzimuthOverrideEndLeftRightDefault(size_t index, float* out_val) const {
  if (!out_val) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndLeftRightDefaultOffset();
  if (!offset) return false;

  *out_val = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndLeftRightDefault(size_t index, float val) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndLeftRightDefaultOffset();
  if (!offset) return;

  *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(addr) + offset) = val;
}

//start_head_offset_offset
bool GameCameraInterior::GetAzimuthOverrideStartHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartHeadOffsetOffset();
  if (!offset) return false;

  uintptr_t ptr = reinterpret_cast<uintptr_t>(addr) + offset;
  *out_x = *reinterpret_cast<float*>(ptr);
  *out_y = *reinterpret_cast<float*>(ptr + 4);
  *out_z = *reinterpret_cast<float*>(ptr + 8);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideStartHeadOffset(size_t index, float x, float y, float z) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeStartHeadOffsetOffset();
  if (!offset) return;

  uintptr_t ptr = reinterpret_cast<uintptr_t>(addr) + offset;
  *reinterpret_cast<float*>(ptr) = x;
  *reinterpret_cast<float*>(ptr + 4) = y;
  *reinterpret_cast<float*>(ptr + 8) = z;
}

//end_head_offset_offset
bool GameCameraInterior::GetAzimuthOverrideEndHeadOffset(size_t index, float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndHeadOffsetOffset();
  if (!offset) return false;

  uintptr_t ptr = reinterpret_cast<uintptr_t>(addr) + offset;
  *out_x = *reinterpret_cast<float*>(ptr);
  *out_y = *reinterpret_cast<float*>(ptr + 4);
  *out_z = *reinterpret_cast<float*>(ptr + 8);
  return true;
}

void GameCameraInterior::SetAzimuthOverrideEndHeadOffset(size_t index, float x, float y, float z) {
  void* addr = GetAzimuthOverrideAddress(index);
  if (!addr) return;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetAzimuthRangeEndHeadOffsetOffset();
  if (!offset) return;

  uintptr_t ptr = reinterpret_cast<uintptr_t>(addr) + offset;
  *reinterpret_cast<float*>(ptr) = x;
  *reinterpret_cast<float*>(ptr + 4) = y;
  *reinterpret_cast<float*>(ptr + 8) = z;
}

// --- Public API for Shake Animation ---

size_t GameCameraInterior::GetShakeAnimCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) return 0;

  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  // SCS vector structure: [ptr(0), capacity(8), count(16)]
  return static_cast<size_t>(*reinterpret_cast<uint64_t*>(pCam + offset + 16));
}

bool GameCameraInterior::GetShakeAnim(size_t index, float* out_x, float* out_y, float* out_z) const {
  if (!m_pCameraObject || index >= GetShakeAnimCount()) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) return false;

  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  // Based on memory dump, the data pointer is at offset + 8
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + offset + 8);
  if (!pData) return false;

  // Packed float3 (12 bytes per element)
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  
  if (out_x) *out_x = pVec[0];
  if (out_y) *out_y = pVec[1];
  if (out_z) *out_z = pVec[2];

  return true;
}

void GameCameraInterior::SetShakeAnim(size_t index, float x, float y, float z) {
  if (!m_pCameraObject || index >= GetShakeAnimCount()) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) return;

  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  // Based on memory dump, the data pointer is at offset + 8
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + offset + 8);
  if (!pData) return;

  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  pVec[0] = x;
  pVec[1] = y;
  pVec[2] = z;
}

}  // namespace GameCamera
SPF_NS_END
