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
  
  uintptr_t pStandardManager = gameData.GetCameraManager();
  
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
}

void GameCameraWindow::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
  logger->Info("Storing default camera state...");

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

  bool rel_azimuth;
  if (GetRelativeHeadtrackingAzimuth(&rel_azimuth)) m_defaultCameraData.relative_headtracking_azimuth = rel_azimuth;

  int32_t auto_center;
  if (GetAutoCenterMoveDirection(&auto_center)) m_defaultCameraData.auto_center_move_direction = auto_center;

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  float shake_step, shake_min, shake_max;
  if (GetShakeAnimStep(&shake_step)) m_defaultCameraData.shake_anim_step = shake_step;
  if (GetShakeAnimScaleMin(&shake_min)) m_defaultCameraData.shake_anim_scale_min = shake_min;
  if (GetShakeAnimScaleMax(&shake_max)) m_defaultCameraData.shake_anim_scale_max = shake_max;

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
  SetRelativeHeadtrackingAzimuth(m_defaultCameraData.relative_headtracking_azimuth);
  SetAutoCenterMoveDirection(m_defaultCameraData.auto_center_move_direction);
  SetFov(m_defaultCameraData.fov_base);
  SetShakeAnimStep(m_defaultCameraData.shake_anim_step);
  SetShakeAnimScaleMin(m_defaultCameraData.shake_anim_scale_min);
  SetShakeAnimScaleMax(m_defaultCameraData.shake_anim_scale_max);
}

bool GameCameraWindow::GetHeadOffset(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto x_off = gameData.GetWindowHeadOffsetXOffset();
  auto y_off = gameData.GetWindowHeadOffsetYOffset();
  auto z_off = gameData.GetWindowHeadOffsetZOffset();
  if (x_off && y_off && z_off) {
    *out_x = *reinterpret_cast<float*>(pCam + x_off);
    *out_y = *reinterpret_cast<float*>(pCam + y_off);
    *out_z = *reinterpret_cast<float*>(pCam + z_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetLiveRotation(float* out_yaw, float* out_pitch) const {
  if (!out_yaw || !out_pitch || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto yaw_off = gameData.GetWindowLiveYawOffset();
  auto pitch_off = gameData.GetWindowLivePitchOffset();
  if (yaw_off && pitch_off) {
    *out_yaw = *reinterpret_cast<float*>(pCam + yaw_off);
    *out_pitch = *reinterpret_cast<float*>(pCam + pitch_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetRotationLimits(float* out_left, float* out_right, float* out_up, float* out_down) const {
  if (!out_left || !out_right || !out_up || !out_down || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto l_off = gameData.GetWindowMouseLeftLimitOffset();
  auto r_off = gameData.GetWindowMouseRightLimitOffset();
  auto u_off = gameData.GetWindowMouseUpLimitOffset();
  auto d_off = gameData.GetWindowMouseDownLimitOffset();
  if (l_off && r_off && u_off && d_off) {
    *out_left = *reinterpret_cast<float*>(pCam + l_off);
    *out_right = *reinterpret_cast<float*>(pCam + r_off);
    *out_up = *reinterpret_cast<float*>(pCam + u_off);
    *out_down = *reinterpret_cast<float*>(pCam + d_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetRotationDefaults(float* out_lr, float* out_ud) const {
  if (!out_lr || !out_ud || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto lr_off = gameData.GetWindowMouseLRDefaultOffset();
  auto ud_off = gameData.GetWindowMouseUDDefaultOffset();
  if (lr_off && ud_off) {
    *out_lr = *reinterpret_cast<float*>(pCam + lr_off);
    *out_ud = *reinterpret_cast<float*>(pCam + ud_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetRelativeHeadtrackingAzimuth(bool* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetWindowRelativeHeadtrackingAzimuthOffset();
  if (off) {
    *out_val = *reinterpret_cast<bool*>(pCam + off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetAutoCenterMoveDirection(int32_t* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetWindowAutoCenterMoveDirectionOffset();
  if (off) {
    *out_val = *reinterpret_cast<int32_t*>(pCam + off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetFov(float* out_fov) const {
  if (!out_fov || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fov_off = gameData.GetFovBaseOffset();
  if (fov_off) {
    *out_fov = *reinterpret_cast<float*>(pCam + fov_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetFinalFov(float* out_horiz, float* out_vert) const {
  if (!out_horiz || !out_vert || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto horiz_off = gameData.GetFovHorizFinalOffset();
  auto vert_off = gameData.GetFovVertFinalOffset();
  if (horiz_off && vert_off) {
    *out_horiz = *reinterpret_cast<float*>(pCam + horiz_off);
    *out_vert = *reinterpret_cast<float*>(pCam + vert_off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetShakeAnimStep(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimStepOffset();
  if (off) {
    *out_val = *reinterpret_cast<float*>(pCam + off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetShakeAnimScaleMin(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMinOffset();
  if (off) {
    *out_val = *reinterpret_cast<float*>(pCam + off);
    return true;
  }
  return false;
}

bool GameCameraWindow::GetShakeAnimScaleMax(float* out_val) const {
  if (!out_val || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMaxOffset();
  if (off) {
    *out_val = *reinterpret_cast<float*>(pCam + off);
    return true;
  }
  return false;
}

size_t GameCameraWindow::GetShakeAnimCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimOffset();
  if (off) return *reinterpret_cast<size_t*>(pCam + off + 16);
  return 0;
}

void GameCameraWindow::GetShakeAnim(size_t index, float& x, float& y, float& z) const {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimOffset();
  if (!off) return;
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + off + 8);
  if (!pData) return;
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  x = pVec[0]; y = pVec[1]; z = pVec[2];
}

void GameCameraWindow::SetHeadOffset(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto x_off = gameData.GetWindowHeadOffsetXOffset();
  auto y_off = gameData.GetWindowHeadOffsetYOffset();
  auto z_off = gameData.GetWindowHeadOffsetZOffset();
  if (x_off && y_off && z_off) {
    *reinterpret_cast<float*>(pCam + x_off) = x;
    *reinterpret_cast<float*>(pCam + y_off) = y;
    *reinterpret_cast<float*>(pCam + z_off) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set head offset: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetLiveRotation(float yaw, float pitch) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto yaw_off = gameData.GetWindowLiveYawOffset();
  auto pitch_off = gameData.GetWindowLivePitchOffset();
  if (yaw_off && pitch_off) {
    *reinterpret_cast<float*>(pCam + yaw_off) = yaw;
    *reinterpret_cast<float*>(pCam + pitch_off) = pitch;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set live rotation: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetRotationLimits(float left, float right, float up, float down) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto l_off = gameData.GetWindowMouseLeftLimitOffset();
  auto r_off = gameData.GetWindowMouseRightLimitOffset();
  auto u_off = gameData.GetWindowMouseUpLimitOffset();
  auto d_off = gameData.GetWindowMouseDownLimitOffset();
  if (l_off && r_off && u_off && d_off) {
    *reinterpret_cast<float*>(pCam + l_off) = left;
    *reinterpret_cast<float*>(pCam + r_off) = right;
    *reinterpret_cast<float*>(pCam + u_off) = up;
    *reinterpret_cast<float*>(pCam + d_off) = down;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set rotation limits: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetRotationDefaults(float lr, float ud) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto lr_off = gameData.GetWindowMouseLRDefaultOffset();
  auto ud_off = gameData.GetWindowMouseUDDefaultOffset();
  if (lr_off && ud_off) {
    *reinterpret_cast<float*>(pCam + lr_off) = lr;
    *reinterpret_cast<float*>(pCam + ud_off) = ud;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set rotation defaults: one or more offsets are missing.");
  }
}

void GameCameraWindow::SetRelativeHeadtrackingAzimuth(bool val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetWindowRelativeHeadtrackingAzimuthOffset();
  if (off) {
    *reinterpret_cast<bool*>(pCam + off) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set relative headtracking azimuth: offset is missing.");
  }
}

void GameCameraWindow::SetAutoCenterMoveDirection(int32_t val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetWindowAutoCenterMoveDirectionOffset();
  if (off) {
    *reinterpret_cast<int32_t*>(pCam + off) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set auto center move direction: offset is missing.");
  }
}

void GameCameraWindow::SetFov(float fov) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fov_off = gameData.GetFovBaseOffset();
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_off = gameData.GetViewportX1Offset();
  auto x2_off = gameData.GetViewportX2Offset();
  auto y1_off = gameData.GetViewportY1Offset();
  auto y2_off = gameData.GetViewportY2Offset();

  if (fov_off && pfnUpdateCameraProjection && pCameraParamsObject && x1_off && x2_off && y1_off && y2_off) {
    *reinterpret_cast<float*>(pCam + fov_off) = fov;
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_off) - *reinterpret_cast<float*>(pCameraParamsObject + x1_off);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_off) - *reinterpret_cast<float*>(pCameraParamsObject + y1_off);
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraWindow::SetShakeAnimStep(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimStepOffset();
  if (off) {
    *reinterpret_cast<float*>(pCam + off) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set shake animation step: offset is missing.");
  }
}

void GameCameraWindow::SetShakeAnimScaleMin(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMinOffset();
  if (off) {
    *reinterpret_cast<float*>(pCam + off) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set shake animation scale min: offset is missing.");
  }
}

void GameCameraWindow::SetShakeAnimScaleMax(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMaxOffset();
  if (off) {
    *reinterpret_cast<float*>(pCam + off) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set shake animation scale max: offset is missing.");
  }
}

void GameCameraWindow::SetShakeAnim(size_t index, float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimOffset();
  if (!off) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraWindow");
    logger->Warn("Cannot set shake animation point: offset is missing.");
    return;
  }
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + off + 8);
  if (!pData) return;
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  pVec[0] = x; pVec[1] = y; pVec[2] = z;
}
}  // namespace GameCamera
SPF_NS_END
