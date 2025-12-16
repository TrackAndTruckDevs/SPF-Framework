#include "SPF/GameCamera/GameCameraFree.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

SPF_NS_BEGIN
namespace GameCamera {
GameCameraFree::GameCameraFree() {
  // Constructor
}

void GameCameraFree::OnActivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
  logger->Info("Activating Free Camera.");

  // Get the raw camera object pointer when this camera becomes active
  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManager = *(uintptr_t*)gameData.GetStandardManagerPtrAddr();
  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
    if (m_pCameraObject) {
      logger->Info("!!! GameCameraFree object base address successfully obtained: {:#x}", (uintptr_t)m_pCameraObject);
    }
  }
}

void GameCameraFree::OnDeactivate() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
  logger->Info("Deactivating Free Camera.");
  m_pCameraObject = nullptr;  // Clear the pointer when not active
}

void GameCameraFree::Update(float dt) {
  if (!m_pCameraObject) {
    return;  // Do nothing if the camera object isn't resolved
  }

  // The new design reads data directly in the Get... methods,
  // so this per-frame update is no longer necessary for populating local data.
  // It can be used for other per-frame logic if needed in the future.
}

void GameCameraFree::SetPosition(float x, float y, float z) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetFreecamPosXOffset();
  auto y_offset = gameData.GetFreecamPosYOffset();
  auto z_offset = gameData.GetFreecamPosZOffset();

  if (x_offset && y_offset && z_offset) {
    *reinterpret_cast<float*>(pCam + x_offset) = x;
    *reinterpret_cast<float*>(pCam + y_offset) = y;
    *reinterpret_cast<float*>(pCam + z_offset) = z;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
    logger->Warn("Cannot set position: one or more offsets are missing.");
  }
}

void GameCameraFree::SetOrientation(float mouse_x, float mouse_y, float roll) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto mouse_x_offset = gameData.GetFreecamMouseXOffset();
  auto mouse_y_offset = gameData.GetFreecamMouseYOffset();
  auto roll_offset = gameData.GetFreecamRollOffset();

  if (mouse_x_offset && mouse_y_offset && roll_offset) {
    *reinterpret_cast<float*>(pCam + mouse_x_offset) = mouse_x;
    *reinterpret_cast<float*>(pCam + mouse_y_offset) = mouse_y;
    *reinterpret_cast<float*>(pCam + roll_offset) = roll;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
    logger->Warn("Cannot set orientation: one or more offsets are missing.");
  }
}

void GameCameraFree::SetFov(float fov) {
  if (!m_pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  // Get all required data first
  auto fov_offset = gameData.GetFovBaseOffset();
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_offset = gameData.GetViewportX1Offset();
  auto x2_offset = gameData.GetViewportX2Offset();
  auto y1_offset = gameData.GetViewportY1Offset();
  auto y2_offset = gameData.GetViewportY2Offset();

  // Check if everything is available
  if (fov_offset && pfnUpdateCameraProjection && pCameraParamsObject && x1_offset && x2_offset && y1_offset && y2_offset) {
    // 1. Set the base FOV value
    *reinterpret_cast<float*>(pCam + fov_offset) = fov;

    // 2. Calculate viewport parameters
    float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
    float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);

    // 3. Call the game's function to make the FOV change take effect
    pfnUpdateCameraProjection(m_pCameraObject, param_width, param_height);
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
    logger->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
  }
}

void GameCameraFree::SetSpeed(float speed) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  float* speed_ptr = gameData.GetFreeCamSpeedPtr();
  if (speed_ptr) {
    *speed_ptr = speed;
  } else {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
    logger->Warn("Cannot set speed: speed pointer is missing.");
  }
}

void GameCameraFree::StoreDefaultState() {
  if (m_defaultsSaved || !m_pCameraObject) {
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
  logger->Info("Storing default camera state...");

  // Populate m_defaultCameraData directly from game memory using safe getters
  float pos_x, pos_y, pos_z;
  if (GetPosition(&pos_x, &pos_y, &pos_z)) {
    m_defaultCameraData.pos_x = pos_x;
    m_defaultCameraData.pos_y = pos_y;
    m_defaultCameraData.pos_z = pos_z;
  }

  float mouse_x, mouse_y, roll;
  if (GetOrientation(&mouse_x, &mouse_y, &roll)) {
    m_defaultCameraData.mouse_x = mouse_x;
    m_defaultCameraData.mouse_y = mouse_y;
    m_defaultCameraData.roll = roll;
  }

  float fov_val;
  if (GetFov(&fov_val)) m_defaultCameraData.fov_base = fov_val;

  float speed_val;
  if (GetSpeed(&speed_val)) m_defaultCameraData.speed = speed_val;

  m_defaultsSaved = true;
  logger->Info("Default camera state has been stored.");
}

void GameCameraFree::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) {
    return;
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraFree");
  logger->Info("Resetting camera state to defaults...");

  SetPosition(m_defaultCameraData.pos_x, m_defaultCameraData.pos_y, m_defaultCameraData.pos_z);
  SetOrientation(m_defaultCameraData.mouse_x, m_defaultCameraData.mouse_y, m_defaultCameraData.roll);
  SetFov(m_defaultCameraData.fov_base);
  SetSpeed(m_defaultCameraData.speed);
}

// --- New Safe Getters ---

bool GameCameraFree::GetPosition(float* out_x, float* out_y, float* out_z) const {
  if (!out_x || !out_y || !out_z) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetFreecamPosXOffset();
  auto y_offset = gameData.GetFreecamPosYOffset();
  auto z_offset = gameData.GetFreecamPosZOffset();

  if (x_offset && y_offset && z_offset) {
    *out_x = *reinterpret_cast<float*>(pCam + x_offset);
    *out_y = *reinterpret_cast<float*>(pCam + y_offset);
    *out_z = *reinterpret_cast<float*>(pCam + z_offset);
    return true;
  }
  return false;
}

bool GameCameraFree::GetQuaternion(float* out_x, float* out_y, float* out_z, float* out_w) const {
  if (!out_x || !out_y || !out_z || !out_w) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto x_offset = gameData.GetFreecamQuatXOffset();
  auto y_offset = gameData.GetFreecamQuatYOffset();
  auto z_offset = gameData.GetFreecamQuatZOffset();
  auto w_offset = gameData.GetFreecamQuatWOffset();

  if (x_offset && y_offset && z_offset && w_offset) {
    *out_x = *reinterpret_cast<float*>(pCam + x_offset);
    *out_y = *reinterpret_cast<float*>(pCam + y_offset);
    *out_z = *reinterpret_cast<float*>(pCam + z_offset);
    *out_w = *reinterpret_cast<float*>(pCam + w_offset);
    return true;
  }
  return false;
}

bool GameCameraFree::GetOrientation(float* out_mouse_x, float* out_mouse_y, float* out_roll) const {
  if (!out_mouse_x || !out_mouse_y || !out_roll) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto mouse_x_offset = gameData.GetFreecamMouseXOffset();
  auto mouse_y_offset = gameData.GetFreecamMouseYOffset();
  auto roll_offset = gameData.GetFreecamRollOffset();

  if (mouse_x_offset && mouse_y_offset && roll_offset) {
    *out_mouse_x = *reinterpret_cast<float*>(pCam + mouse_x_offset);
    *out_mouse_y = *reinterpret_cast<float*>(pCam + mouse_y_offset);
    *out_roll = *reinterpret_cast<float*>(pCam + roll_offset);
    return true;
  }
  return false;
}

bool GameCameraFree::GetFov(float* out_fov) const {
  if (!out_fov) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto fov_offset = gameData.GetFovBaseOffset();

  if (fov_offset) {
    *out_fov = *reinterpret_cast<float*>(pCam + fov_offset);
    return true;
  }
  return false;
}

bool GameCameraFree::GetSpeed(float* out_speed) const {
  if (!out_speed) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  float* speed_ptr = gameData.GetFreeCamSpeedPtr();

  if (speed_ptr) {
    *out_speed = *speed_ptr;
    return true;
  }
  return false;
}

bool GameCameraFree::GetFinalFov(float* out_horiz, float* out_vert) const {
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

bool GameCameraFree::GetFreecamMysteryFloat(float* out_mystery) const {
  if (!out_mystery) return false;
  if (!m_pCameraObject) return false;

  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(m_pCameraObject);

  auto mystery_offset = gameData.GetFreecamMysteryFloatOffset();

  if (mystery_offset) {
    *out_mystery = *reinterpret_cast<float*>(pCam + mystery_offset);
    return true;
  }
  return false;
}
}  // namespace GameCamera
SPF_NS_END
