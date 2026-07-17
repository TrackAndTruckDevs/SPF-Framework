#include "SPF/GameCamera/GameCameraTop.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstddef>
#include <cstdint>


SPF_NS_BEGIN
namespace GameCamera {
GameCameraTop::GameCameraTop() {
  // Constructor
}

void GameCameraTop::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
  logger->Info("Activating Top Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  // GetCameraManager() handles pointer dereferencing and version-specific adjustments.
  uintptr_t pStandardManager = gameData.GetCameraManager();

  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }
}

void GameCameraTop::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
  logger->Info("Deactivating Top Camera.");
  m_pCameraObject = nullptr;
}

void GameCameraTop::Update(float dt) {
  if (!m_pCameraObject) return;
}

void GameCameraTop::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float min_h, max_h;
  if (GetHeight(&min_h, &max_h)) {
    m_defaultCameraData.minimum_height = min_h;
    m_defaultCameraData.maximum_height = max_h;
  }

  float speed_val;
  if (GetSpeed(&speed_val)) m_defaultCameraData.speed = speed_val;

  float offset_f, offset_b;
  if (GetOffsets(&offset_f, &offset_b)) {
    m_defaultCameraData.x_offset_forward = offset_f;
    m_defaultCameraData.x_offset_backward = offset_b;
  }

  float off_z_f, off_z_b;
  if (GetOffsetsZ(&off_z_f, &off_z_b)) {
    m_defaultCameraData.offset_forward = off_z_f;
    m_defaultCameraData.offset_backward = off_z_b;
  }

  float height_factor;
  bool use_adaptive;
  if (GetAdaptiveSettings(&height_factor, &use_adaptive)) {
    m_defaultCameraData.camera_height_factor = height_factor;
    m_defaultCameraData.use_adaptive_camera_height = use_adaptive;
  }

  float near_p, far_p;
  if (GetPlaneSettings(&near_p, &far_p)) {
    m_defaultCameraData.near_plane = near_p;
    m_defaultCameraData.far_plane = far_p;
  }

  bool validation;
  if (GetValidation(&validation)) m_defaultCameraData.validation = validation;

  float val_speed_pos, val_speed_neg;
  if (GetValidationSettings(&val_speed_pos, &val_speed_neg)) {
    m_defaultCameraData.validation_speed_positive = val_speed_pos;
    m_defaultCameraData.validation_speed_negative = val_speed_neg;
  }

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  float shake_step, shake_min, shake_max;
  if (GetShakeAnimStep(&shake_step)) m_defaultCameraData.shake_anim_step = shake_step;
  if (GetShakeAnimScaleMin(&shake_min)) m_defaultCameraData.shake_anim_scale_min = shake_min;
  if (GetShakeAnimScaleMax(&shake_max)) m_defaultCameraData.shake_anim_scale_max = shake_max;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraTop::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
  logger->Info("Resetting camera state to defaults...");

  SetHeight(m_defaultCameraData.minimum_height, m_defaultCameraData.maximum_height);
  SetSpeed(m_defaultCameraData.speed);
  SetOffsets(m_defaultCameraData.x_offset_forward, m_defaultCameraData.x_offset_backward);
  SetOffsetsZ(m_defaultCameraData.offset_forward, m_defaultCameraData.offset_backward);
  SetAdaptiveSettings(m_defaultCameraData.camera_height_factor, m_defaultCameraData.use_adaptive_camera_height);
  SetPlaneSettings(m_defaultCameraData.near_plane, m_defaultCameraData.far_plane);
  SetFov(m_defaultCameraData.fov_base);
  SetValidation(m_defaultCameraData.validation);
  SetValidationSettings(m_defaultCameraData.validation_speed_positive, m_defaultCameraData.validation_speed_negative);
  SetShakeAnimStep(m_defaultCameraData.shake_anim_step);
  SetShakeAnimScaleMin(m_defaultCameraData.shake_anim_scale_min);
  SetShakeAnimScaleMax(m_defaultCameraData.shake_anim_scale_max);
}

void GameCameraTop::SetHeight(float min, float max) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto min_offset = gameData.GetTopMinHeightOffset();
  auto max_offset = gameData.GetTopMaxHeightOffset();
  if (min_offset && max_offset) {
    *reinterpret_cast<float*>(pCam + min_offset) = min;
    *reinterpret_cast<float*>(pCam + max_offset) = max;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set height: one or more offsets are missing.");
  }
}

void GameCameraTop::SetSpeed(float speed) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto speed_offset = gameData.GetTopSpeedOffset();
  if (speed_offset) {
    *reinterpret_cast<float*>(pCam + speed_offset) = speed;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set speed: offset is missing.");
  }
}

void GameCameraTop::SetOffsets(float forward, float backward) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto forward_offset = gameData.GetTopXOffsetForwardOffset();
  auto backward_offset = gameData.GetTopXOffsetBackwardOffset();
  if (forward_offset && backward_offset) {
    *reinterpret_cast<float*>(pCam + forward_offset) = forward;
    *reinterpret_cast<float*>(pCam + backward_offset) = backward;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set lateral offsets: one or more offsets are missing.");
  }
}

void GameCameraTop::SetOffsetsZ(float forward, float backward) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fwd_off = gameData.GetTopOffsetForwardOffset();
  auto bwd_off = gameData.GetTopOffsetBackwardOffset();
  if (fwd_off && bwd_off) {
    *reinterpret_cast<float*>(pCam + fwd_off) = forward;
    *reinterpret_cast<float*>(pCam + bwd_off) = backward;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set longitudinal offsets: one or more offsets are missing.");
  }
}

void GameCameraTop::SetAdaptiveSettings(float factor, bool use_adaptive) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fact_off = gameData.GetTopCameraHeightFactorOffset();
  auto use_off = gameData.GetTopUseAdaptiveCameraHeightOffset();
  if (fact_off && use_off) {
    *reinterpret_cast<float*>(pCam + fact_off) = factor;
    *reinterpret_cast<bool*>(pCam + use_off) = use_adaptive;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set adaptive settings: one or more offsets are missing.");
  }
}

void GameCameraTop::SetPlaneSettings(float near_p, float far_p) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto near_off = gameData.GetTopNearPlaneOffset();
  auto far_off = gameData.GetTopFarPlaneOffset();
  if (near_off && far_off) {
    *reinterpret_cast<float*>(pCam + near_off) = near_p;
    *reinterpret_cast<float*>(pCam + far_off) = far_p;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set plane settings: one or more offsets are missing.");
  }
}

void GameCameraTop::SetFov(float fov) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fov_base_offset = gameData.GetFovBaseOffset();
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_offset = gameData.GetViewportX1Offset();
  auto x2_offset = gameData.GetViewportX2Offset();
  auto y1_offset = gameData.GetViewportY1Offset();
  auto y2_offset = gameData.GetViewportY2Offset();

  if (fov_base_offset && pfnUpdateCameraProjection && pCameraParamsObject && x1_offset && x2_offset && y1_offset && y2_offset) {
    *reinterpret_cast<float*>(pCam + fov_base_offset) = fov;
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraTop::SetValidation(bool enabled) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetTopValidationOffset();
  if (offset) {
    *reinterpret_cast<bool*>(pCam + offset) = enabled;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set validation: offset is missing.");
  }
}

void GameCameraTop::SetValidationSettings(float speed_pos, float speed_neg) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto speed_pos_offset = gameData.GetTopValidationSpeedPositiveOffset();
  auto speed_neg_offset = gameData.GetTopValidationSpeedNegativeOffset();
  if (speed_pos_offset && speed_neg_offset) {
    *reinterpret_cast<float*>(pCam + speed_pos_offset) = speed_pos;
    *reinterpret_cast<float*>(pCam + speed_neg_offset) = speed_neg;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set validation settings: one or more offsets are missing.");
  }
}

void GameCameraTop::SetShakeAnimStep(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimStepOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set shake animation step: offset is missing.");
  }
}

void GameCameraTop::SetShakeAnimScaleMin(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMinOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set shake animation scale min: offset is missing.");
  }
}

void GameCameraTop::SetShakeAnimScaleMax(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMaxOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set shake animation scale max: offset is missing.");
  }
}

void GameCameraTop::SetShakeAnim(size_t index, float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set shake animation point: offset is missing.");
    return;
  }
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + offset + 8);
  if (!pData) return;
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  pVec[0] = x;
  pVec[1] = y;
  pVec[2] = z;
}

bool GameCameraTop::GetHeight(float* out_min, float* out_max) const {
  if (!out_min || !out_max || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto min_offset = gameData.GetTopMinHeightOffset();
  auto max_offset = gameData.GetTopMaxHeightOffset();
  if (min_offset && max_offset) {
    *out_min = *reinterpret_cast<float*>(pCam + min_offset);
    *out_max = *reinterpret_cast<float*>(pCam + max_offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetSpeed(float* out_speed) const {
  if (!out_speed || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto speed_offset = gameData.GetTopSpeedOffset();
  if (speed_offset) {
    *out_speed = *reinterpret_cast<float*>(pCam + speed_offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetOffsets(float* out_forward, float* out_backward) const {
  if (!out_forward || !out_backward || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto forward_offset = gameData.GetTopXOffsetForwardOffset();
  auto backward_offset = gameData.GetTopXOffsetBackwardOffset();
  if (forward_offset && backward_offset) {
    *out_forward = *reinterpret_cast<float*>(pCam + forward_offset);
    *out_backward = *reinterpret_cast<float*>(pCam + backward_offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetOffsetsZ(float* out_forward, float* out_backward) const {
  if (!out_forward || !out_backward || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fwd_off = gameData.GetTopOffsetForwardOffset();
  auto bwd_off = gameData.GetTopOffsetBackwardOffset();
  if (fwd_off && bwd_off) {
    *out_forward = *reinterpret_cast<float*>(pCam + fwd_off);
    *out_backward = *reinterpret_cast<float*>(pCam + bwd_off);
    return true;
  }
  return false;
}

bool GameCameraTop::GetAdaptiveSettings(float* out_factor, bool* out_use_adaptive) const {
  if (!out_factor || !out_use_adaptive || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fact_off = gameData.GetTopCameraHeightFactorOffset();
  auto use_off = gameData.GetTopUseAdaptiveCameraHeightOffset();
  if (fact_off && use_off) {
    *out_factor = *reinterpret_cast<float*>(pCam + fact_off);
    *out_use_adaptive = *reinterpret_cast<bool*>(pCam + use_off);
    return true;
  }
  return false;
}

bool GameCameraTop::GetPlaneSettings(float* out_near, float* out_far) const {
  if (!out_near || !out_far || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto near_off = gameData.GetTopNearPlaneOffset();
  auto far_off = gameData.GetTopFarPlaneOffset();
  if (near_off && far_off) {
    *out_near = *reinterpret_cast<float*>(pCam + near_off);
    *out_far = *reinterpret_cast<float*>(pCam + far_off);
    return true;
  }
  return false;
}

bool GameCameraTop::GetFov(float* out_fov) const {
  if (!out_fov || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto fov_base_offset = gameData.GetFovBaseOffset();
  if (fov_base_offset) {
    *out_fov = *reinterpret_cast<float*>(pCam + fov_base_offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetFinalFov(float* out_horiz, float* out_vert) const {
  if (!out_horiz || !out_vert || !m_pCameraObject) return false;
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

bool GameCameraTop::GetValidation(bool* out_enabled) const {
  if (!out_enabled || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetTopValidationOffset();
  if (offset) {
    *out_enabled = *reinterpret_cast<bool*>(pCam + offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetValidationSettings(float* out_speed_pos, float* out_speed_neg) const {
  if (!out_speed_pos || !out_speed_neg || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto speed_pos_offset = gameData.GetTopValidationSpeedPositiveOffset();
  auto speed_neg_offset = gameData.GetTopValidationSpeedNegativeOffset();
  if (speed_pos_offset && speed_neg_offset) {
    *out_speed_pos = *reinterpret_cast<float*>(pCam + speed_pos_offset);
    *out_speed_neg = *reinterpret_cast<float*>(pCam + speed_neg_offset);
    return true;
  }
  return false;
}

bool GameCameraTop::GetShakeAnimStep(float* out_val) const {
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

bool GameCameraTop::GetShakeAnimScaleMin(float* out_val) const {
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

bool GameCameraTop::GetShakeAnimScaleMax(float* out_val) const {
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

size_t GameCameraTop::GetShakeAnimCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimOffset();
  if (offset) {
    return *reinterpret_cast<size_t*>(pCam + offset + 16);
  }
  return 0;
}

void GameCameraTop::GetShakeAnim(size_t index, float& x, float& y, float& z) const {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) return;
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + offset + 8);
  if (!pData) return;
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  x = pVec[0];
  y = pVec[1];
  z = pVec[2];
}
}  // namespace GameCamera
SPF_NS_END
