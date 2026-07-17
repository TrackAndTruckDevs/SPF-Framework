#include "SPF/GameCamera/GameCameraCabin.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstddef>
#include <cstdint>


SPF_NS_BEGIN
namespace GameCamera {
GameCameraCabin::GameCameraCabin() {}

void GameCameraCabin::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Activating Cabin Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  // GetCameraManager() handles pointer dereferencing and version-specific adjustments.
  uintptr_t pStandardManager = gameData.GetCameraManager();

  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }
}

void GameCameraCabin::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Deactivating Cabin Camera.");
  m_pCameraObject = nullptr;
}

void GameCameraCabin::Update(float dt) {
  if (!m_pCameraObject) return;
}

void GameCameraCabin::SetFov(float fov) {
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
  }
}

void GameCameraCabin::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Storing default camera state...");

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  float shake_step, shake_min, shake_max;
  if (GetShakeAnimStep(&shake_step)) m_defaultCameraData.shake_anim_step = shake_step;
  if (GetShakeAnimScaleMin(&shake_min)) m_defaultCameraData.shake_anim_scale_min = shake_min;
  if (GetShakeAnimScaleMax(&shake_max)) m_defaultCameraData.shake_anim_scale_max = shake_max;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraCabin::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Resetting camera state to defaults...");

  SetFov(m_defaultCameraData.fov_base);
  SetShakeAnimStep(m_defaultCameraData.shake_anim_step);
  SetShakeAnimScaleMin(m_defaultCameraData.shake_anim_scale_min);
  SetShakeAnimScaleMax(m_defaultCameraData.shake_anim_scale_max);
}

bool GameCameraCabin::GetFov(float* out_fov) const {
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

bool GameCameraCabin::GetFinalFov(float* out_horiz, float* out_vert) const {
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

bool GameCameraCabin::GetShakeAnimStep(float* out_val) const {
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

bool GameCameraCabin::GetShakeAnimScaleMin(float* out_val) const {
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

bool GameCameraCabin::GetShakeAnimScaleMax(float* out_val) const {
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

size_t GameCameraCabin::GetShakeAnimCount() const {
  if (!m_pCameraObject) return 0;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimOffset();
  if (offset) {
    return *reinterpret_cast<size_t*>(pCam + offset + 16);
  }
  return 0;
}

void GameCameraCabin::GetShakeAnim(size_t index, float& x, float& y, float& z) const {
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

void GameCameraCabin::SetShakeAnimStep(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimStepOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  }
}

void GameCameraCabin::SetShakeAnimScaleMin(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMinOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  }
}

void GameCameraCabin::SetShakeAnimScaleMax(float val) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimScaleMaxOffset();
  if (offset) {
    *reinterpret_cast<float*>(pCam + offset) = val;
  }
}

void GameCameraCabin::SetShakeAnim(size_t index, float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);
  auto offset = gameData.GetShakeAnimOffset();
  if (!offset) return;
  uintptr_t pData = *reinterpret_cast<uintptr_t*>(pCam + offset + 8);
  if (!pData) return;
  float* pVec = reinterpret_cast<float*>(pData + (index * 12));
  pVec[0] = x;
  pVec[1] = y;
  pVec[2] = z;
}
}  // namespace GameCamera
SPF_NS_END
