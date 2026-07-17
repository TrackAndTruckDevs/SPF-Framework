#include "SPF/GameCamera/GameCameraPhoto.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>


SPF_NS_BEGIN
namespace GameCamera {

GameCameraPhoto::GameCameraPhoto() {}

void GameCameraPhoto::OnActivate() {
  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  // GetCameraManager() handles pointer dereferencing and version-specific adjustments.
  uintptr_t pStandardManager = gameData.GetCameraManager();

  if (hooks.GetGetCameraObjectFunc() && pStandardManager) {
    m_pCameraObject = hooks.GetGetCameraObjectFunc()((void*)pStandardManager, static_cast<int>(GetType()));
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameCameraPhoto");
  if (m_pCameraObject) {
    logger->Info("Photo Camera activated at 0x{:X}", reinterpret_cast<uintptr_t>(m_pCameraObject));
  } else {
    logger->Warn("Failed to resolve Photo Camera object.");
  }
}

void GameCameraPhoto::OnDeactivate() { m_pCameraObject = nullptr; }

void GameCameraPhoto::Update(float dt) {
  if (!m_pCameraObject) return;
  // Live updates will go here
}

void GameCameraPhoto::StoreDefaultState() {
  if (!m_pCameraObject) return;
  // Read and store current values as defaults
  m_defaultsSaved = true;
}

void GameCameraPhoto::ResetToDefaults() {
  if (!m_defaultsSaved || !m_pCameraObject) return;
  // Apply defaults
}

bool GameCameraPhoto::GetLiveState(float* out_pitch, float* out_yaw, float* out_roll, float* out_zoom) const { return false; }

void GameCameraPhoto::SetLiveState(float pitch, float yaw, float roll, float zoom) {}

bool GameCameraPhoto::GetPosition(float* out_x, float* out_y, float* out_z) const { return false; }

void GameCameraPhoto::SetPosition(float x, float y, float z) {}

bool GameCameraPhoto::GetFov(float* out_fov) const { return false; }

void GameCameraPhoto::SetFov(float fov) {}

}  // namespace GameCamera
SPF_NS_END
