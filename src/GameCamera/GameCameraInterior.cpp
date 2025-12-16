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
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
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
    // 1. Set the base FOV value
    *reinterpret_cast<float*>(pCam + fov_base_offset) = fov;

    // 2. Calculate viewport parameters
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);

    // 3. Call the game's function to make the FOV change take effect
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);
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

  // Mark defaults as saved
  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraInterior::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) {
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraInterior");
  logger->Info("Resetting camera state to defaults...");

  // Use the existing Set... methods to apply the default values
  SetSeatPosition(m_defaultCameraData.seat_pos_x, m_defaultCameraData.seat_pos_y, m_defaultCameraData.seat_pos_z);
  SetHeadRotation(m_defaultCameraData.yaw, m_defaultCameraData.pitch);
  SetFov(m_defaultCameraData.fov_base);
  SetRotationLimits(m_defaultCameraData.limit_left, m_defaultCameraData.limit_right, m_defaultCameraData.limit_up, m_defaultCameraData.limit_down);
  SetRotationDefaults(m_defaultCameraData.mouse_lr_default, m_defaultCameraData.mouse_ud_default);
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
}  // namespace GameCamera
SPF_NS_END
