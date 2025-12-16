#include "SPF/GameCamera/GameCameraTop.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

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
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
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

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
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
    logger->Warn("Cannot set offsets: one or more offsets are missing.");
  }
}

void GameCameraTop::SetFov(float fov) {
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
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraTop");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
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

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

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
  SetFov(m_defaultCameraData.fov_base);
}

// --- New Safe Getters ---

bool GameCameraTop::GetHeight(float* out_min, float* out_max) const {
  if (!out_min || !out_max) return false;
  if (!m_pCameraObject) return false;

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
  if (!out_speed) return false;
  if (!m_pCameraObject) return false;

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
  if (!out_forward || !out_backward) return false;
  if (!m_pCameraObject) return false;

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

bool GameCameraTop::GetFov(float* out_fov) const {
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

bool GameCameraTop::GetFinalFov(float* out_horiz, float* out_vert) const {
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
