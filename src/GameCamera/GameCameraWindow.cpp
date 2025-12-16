#include "SPF/GameCamera/GameCameraWindow.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraWindow::GameCameraWindow() {}

void GameCameraWindow::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
  logger->Info("Activating Window Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }
}

void GameCameraWindow::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
  logger->Info("Deactivating Window Camera.");
  m_pCameraObject = nullptr;
}

void GameCameraWindow::Update(float dt) {
  if (!m_pCameraObject) return;

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
}

void GameCameraWindow::SetHeadOffset(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetWindowHeadOffsetXOffset();
  auto y_offset = gameData.GetWindowHeadOffsetYOffset();
  auto z_offset = gameData.GetWindowHeadOffsetZOffset();

  if (x_offset && y_offset && z_offset) {
    *reinterpret_cast<float*>(pCam + x_offset) = x;
    *reinterpret_cast<float*>(pCam + y_offset) = y;
    *reinterpret_cast<float*>(pCam + z_offset) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set head offset: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetLiveRotation(float yaw, float pitch) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto yaw_offset = gameData.GetWindowLiveYawOffset();
  auto pitch_offset = gameData.GetWindowLivePitchOffset();

  if (yaw_offset && pitch_offset) {
    *reinterpret_cast<float*>(pCam + yaw_offset) = yaw;
    *reinterpret_cast<float*>(pCam + pitch_offset) = pitch;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set live rotation: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetRotationLimits(float left, float right, float up, float down) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto left_offset = gameData.GetWindowMouseLeftLimitOffset();
  auto right_offset = gameData.GetWindowMouseRightLimitOffset();
  auto up_offset = gameData.GetWindowMouseUpLimitOffset();
  auto down_offset = gameData.GetWindowMouseDownLimitOffset();

  if (left_offset && right_offset && up_offset && down_offset) {
    *reinterpret_cast<float*>(pCam + left_offset) = left;
    *reinterpret_cast<float*>(pCam + right_offset) = right;
    *reinterpret_cast<float*>(pCam + up_offset) = up;
    *reinterpret_cast<float*>(pCam + down_offset) = down;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set rotation limits: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetRotationDefaults(float lr, float ud) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto lr_offset = gameData.GetWindowMouseLRDefaultOffset();
  auto ud_offset = gameData.GetWindowMouseUDDefaultOffset();

  if (lr_offset && ud_offset) {
    *reinterpret_cast<float*>(pCam + lr_offset) = lr;
    *reinterpret_cast<float*>(pCam + ud_offset) = ud;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set rotation defaults: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetFov(float fov) {
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
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraWindow::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float head_x, head_y, head_z;
  if (GetHeadOffset(&head_x, &head_y, &head_z)) {
    m_defaultCameraData.head_offset_x = head_x;
    m_defaultCameraData.head_offset_y = head_y;
    m_defaultCameraData.head_offset_z = head_z;
  }

  float live_yaw, live_pitch;
  if (GetLiveRotation(&live_yaw, &live_pitch)) {
    m_defaultCameraData.live_yaw = live_yaw;
    m_defaultCameraData.live_pitch = live_pitch;
  }

  float lim_l, lim_r, lim_u, lim_d;
  if (GetRotationLimits(&lim_l, &lim_r, &lim_u, &lim_d)) {
    m_defaultCameraData.mouse_left_limit = lim_l;
    m_defaultCameraData.mouse_right_limit = lim_r;
    m_defaultCameraData.mouse_up_limit = lim_u;
    m_defaultCameraData.mouse_down_limit = lim_d;
  }

  float lr_def, ud_def;
  if (GetRotationDefaults(&lr_def, &ud_def)) {
    m_defaultCameraData.mouse_lr_default = lr_def;
    m_defaultCameraData.mouse_ud_default = ud_def;
  }

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraWindow::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
  logger->Info("Resetting camera state to defaults...");

  SetHeadOffset(m_defaultCameraData.head_offset_x, m_defaultCameraData.head_offset_y, m_defaultCameraData.head_offset_z);
  SetLiveRotation(m_defaultCameraData.live_yaw, m_defaultCameraData.live_pitch);
  SetRotationLimits(m_defaultCameraData.mouse_left_limit, m_defaultCameraData.mouse_right_limit, m_defaultCameraData.mouse_up_limit, m_defaultCameraData.mouse_down_limit);
  SetRotationDefaults(m_defaultCameraData.mouse_lr_default, m_defaultCameraData.mouse_ud_default);
  SetFov(m_defaultCameraData.fov_base);
}

// --- New Safe Getters ---

bool GameCameraWindow::GetHeadOffset(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetWindowHeadOffsetXOffset();
  auto y_offset = gameData.GetWindowHeadOffsetYOffset();
  auto z_offset = gameData.GetWindowHeadOffsetZOffset();

  if (x_offset && y_offset && z_offset) {
    *out_x = *reinterpret_cast<float*>(pCam + x_offset);
    *out_y = *reinterpret_cast<float*>(pCam + y_offset);
    *out_z = *reinterpret_cast<float*>(pCam + z_offset);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetLiveRotation(float* out_yaw, float* out_pitch) const {
  if (!out_yaw || !out_pitch) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto yaw_offset = gameData.GetWindowLiveYawOffset();
  auto pitch_offset = gameData.GetWindowLivePitchOffset();

  if (yaw_offset && pitch_offset) {
    *out_yaw = *reinterpret_cast<float*>(pCam + yaw_offset);
    *out_pitch = *reinterpret_cast<float*>(pCam + pitch_offset);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const {
  if (!out_left || !out_right || !out_up || !out_down) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto left_offset = gameData.GetWindowMouseLeftLimitOffset();
  auto right_offset = gameData.GetWindowMouseRightLimitOffset();
  auto up_offset = gameData.GetWindowMouseUpLimitOffset();
  auto down_offset = gameData.GetWindowMouseDownLimitOffset();

  if (left_offset && right_offset && up_offset && down_offset) {
    *out_left = *reinterpret_cast<float*>(pCam + left_offset);
    *out_right = *reinterpret_cast<float*>(pCam + right_offset);
    *out_up = *reinterpret_cast<float*>(pCam + up_offset);
    *out_down = *reinterpret_cast<float*>(pCam + down_offset);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetRotationDefaults(float* out_lr, float* out_ud) const {
  if (!out_lr || !out_ud) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto lr_offset = gameData.GetWindowMouseLRDefaultOffset();
  auto ud_offset = gameData.GetWindowMouseUDDefaultOffset();

  if (lr_offset && ud_offset) {
    *out_lr = *reinterpret_cast<float*>(pCam + lr_offset);
    *out_ud = *reinterpret_cast<float*>(pCam + ud_offset);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetFov(float* out_fov) const {
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

bool GameCameraWindow::GetFinalFov(float* out_horiz, float* out_vert) const {
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