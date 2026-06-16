#include "SPF/GameCamera/GameCameraTV.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraTV::GameCameraTV() {}

void GameCameraTV::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
  logger->Info("Activating TV Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  
  uintptr_t pStandardManager = gameData.GetCameraManager();
  
  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }
}

void GameCameraTV::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
  logger->Info("Deactivating TV Camera.");
  m_pCameraObject = nullptr;
}

void GameCameraTV::Update(float dt) {
  if (!m_pCameraObject) return;
}

void GameCameraTV::SetMaxDistance(float distance) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto offset = gameData.GetTVMaxDistanceOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = distance;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set max distance: offset is missing.");
  }
}

void GameCameraTV::SetPrefabUplift(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_off = gameData.GetTVPrefabUpliftXOffset();
  auto y_off = gameData.GetTVPrefabUpliftYOffset();
  auto z_off = gameData.GetTVPrefabUpliftZOffset();

  if (x_off && y_off && z_off) {
    *reinterpret_cast<float*>(pCam + x_off) = x;
    *reinterpret_cast<float*>(pCam + y_off) = y;
    *reinterpret_cast<float*>(pCam + z_off) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set prefab uplift: one or more offsets are missing.");
  }
}

void GameCameraTV::SetRoadUplift(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_off = gameData.GetTVRoadUpliftXOffset();
  auto y_off = gameData.GetTVRoadUpliftYOffset();
  auto z_off = gameData.GetTVRoadUpliftZOffset();

  if (x_off && y_off && z_off) {
    *reinterpret_cast<float*>(pCam + x_off) = x;
    *reinterpret_cast<float*>(pCam + y_off) = y;
    *reinterpret_cast<float*>(pCam + z_off) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set road uplift: one or more offsets are missing.");
  }
}

void GameCameraTV::SetFov(float fov) {
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
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraTV::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
  logger->Info("Storing default camera state...");

  float max_d;
  if (GetMaxDistance(&max_d)) m_defaultCameraData.max_distance = max_d;

  float px, py, pz;
  if (GetPrefabUplift(&px, &py, &pz)) {
    m_defaultCameraData.prefab_uplift_x = px;
    m_defaultCameraData.prefab_uplift_y = py;
    m_defaultCameraData.prefab_uplift_z = pz;
  }

  float rx, ry, rz;
  if (GetRoadUplift(&rx, &ry, &rz)) {
    m_defaultCameraData.road_uplift_x = rx;
    m_defaultCameraData.road_uplift_y = ry;
    m_defaultCameraData.road_uplift_z = rz;
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

void GameCameraTV::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
  logger->Info("Resetting camera state to defaults...");

  SetMaxDistance(m_defaultCameraData.max_distance);
  SetPrefabUplift(m_defaultCameraData.prefab_uplift_x, m_defaultCameraData.prefab_uplift_y, m_defaultCameraData.prefab_uplift_z);
  SetRoadUplift(m_defaultCameraData.road_uplift_x, m_defaultCameraData.road_uplift_y, m_defaultCameraData.road_uplift_z);
  SetFov(m_defaultCameraData.fov_base);
  SetShakeAnimStep(m_defaultCameraData.shake_anim_step);
  SetShakeAnimScaleMin(m_defaultCameraData.shake_anim_scale_min);
  SetShakeAnimScaleMax(m_defaultCameraData.shake_anim_scale_max);
}

bool GameCameraTV::GetMaxDistance(float* out_max_distance) const {
  if (!out_max_distance || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetTVMaxDistanceOffset();
  if (offset) {
    *out_max_distance = *reinterpret_cast<float*>(pCam + offset);
    return true;
  }
  return false;
}

bool GameCameraTV::GetPrefabUplift(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto x_off = gameData.GetTVPrefabUpliftXOffset();
  auto y_off = gameData.GetTVPrefabUpliftYOffset();
  auto z_off = gameData.GetTVPrefabUpliftZOffset();
  if (x_off && y_off && z_off) {
    *out_x = *reinterpret_cast<float*>(pCam + x_off);
    *out_y = *reinterpret_cast<float*>(pCam + y_off);
    *out_z = *reinterpret_cast<float*>(pCam + z_off);
    return true;
  }
  return false;
}

bool GameCameraTV::GetRoadUplift(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z || !m_pCameraObject) return false;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto x_off = gameData.GetTVRoadUpliftXOffset();
  auto y_off = gameData.GetTVRoadUpliftYOffset();
  auto z_off = gameData.GetTVRoadUpliftZOffset();
  if (x_off && y_off && z_off) {
    *out_x = *reinterpret_cast<float*>(pCam + x_off);
    *out_y = *reinterpret_cast<float*>(pCam + y_off);
    *out_z = *reinterpret_cast<float*>(pCam + z_off);
    return true;
  }
  return false;
}

bool GameCameraTV::GetFov(float* out_fov) const {
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

bool GameCameraTV::GetFinalFov(float* out_horiz, float* out_vert) const {
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

bool GameCameraTV::GetShakeAnimStep(float* out_val) const {
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

bool GameCameraTV::GetShakeAnimScaleMin(float* out_val) const {
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

bool GameCameraTV::GetShakeAnimScaleMax(float* out_val) const {
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

size_t GameCameraTV::GetShakeAnimCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimOffset();
  if (off) return *reinterpret_cast<size_t*>(pCam + off + 16);
  return 0;
}

void GameCameraTV::GetShakeAnim(size_t index, float& x, float& y, float& z) const {
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

void GameCameraTV::SetShakeAnimStep(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimStepOffset();
  if (off) *reinterpret_cast<float*>(pCam + off) = val;
  else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set shake animation step: offset is missing.");
  }
}

void GameCameraTV::SetShakeAnimScaleMin(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMinOffset();
  if (off) *reinterpret_cast<float*>(pCam + off) = val;
  else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set shake animation scale min: offset is missing.");
  }
}

void GameCameraTV::SetShakeAnimScaleMax(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimScaleMaxOffset();
  if (off) *reinterpret_cast<float*>(pCam + off) = val;
  else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
    logger->Warn("Cannot set shake animation scale max: offset is missing.");
  }
}

void GameCameraTV::SetShakeAnim(size_t index, float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto off = gameData.GetShakeAnimOffset();
  if (!off) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTV");
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
