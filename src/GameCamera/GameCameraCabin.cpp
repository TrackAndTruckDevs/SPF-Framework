#include "SPF/GameCamera/GameCameraCabin.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraCabin::GameCameraCabin() {}

void GameCameraCabin::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Activating Cabin Camera.");

  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
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

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
}

void GameCameraCabin::SetFov(float fov) {
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
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraCabin::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraCabin::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraCabin");
  logger->Info("Resetting camera state to defaults...");

  SetFov(m_defaultCameraData.fov_base);
}

// --- New Safe Getters ---

bool GameCameraCabin::GetFov(float* out_fov) const {
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

bool GameCameraCabin::GetFinalFov(float* out_horiz, float* out_vert) const {
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
