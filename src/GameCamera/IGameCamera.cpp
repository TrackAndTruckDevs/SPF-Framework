#include "SPF/GameCamera/IGameCamera.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace GameCamera {

void IGameCamera::ApplyCoreCameraFov(void* pCameraObject, float fov) {
  if (!pCameraObject) return;
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  auto& hooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pCam = reinterpret_cast<uintptr_t>(pCameraObject);

  auto fov_base_offset = gameData.GetFovBaseOffset();
  auto pfnUpdateCameraProjection = hooks.GetUpdateCameraProjectionFunc();
  uintptr_t pCameraParamsObject = gameData.GetCameraParamsObjectPtr();
  auto x1_offset = gameData.GetViewportX1Offset();
  auto x2_offset = gameData.GetViewportX2Offset();
  auto y1_offset = gameData.GetViewportY1Offset();
  auto y2_offset = gameData.GetViewportY2Offset();

  if (!(fov_base_offset && pfnUpdateCameraProjection && pCameraParamsObject && x1_offset && x2_offset && y1_offset && y2_offset)) {
    Logging::LoggerFactory::GetInstance().GetLogger("IGameCamera")->Warn("Cannot set FOV: one or more required pointers or offsets are missing.");
    return;
  }

  // 1. Live FOV field, consumed by UpdateCameraProjection below.
  *reinterpret_cast<float*>(pCam + fov_base_offset) = fov;

  // 1b. Reference field the game's native speed-FOV system reads from. Optional: not
  // required for this call's own visual effect to take place.
  auto zoomBaseOffset = gameData.GetFovZoomBaseOffset();
  if (zoomBaseOffset) {
    *reinterpret_cast<float*>(pCam + zoomBaseOffset) = fov;
  }

  // 2. Recompute horiz/vert final FOV from the live field.
  float param_width = *reinterpret_cast<float*>(pCameraParamsObject + x2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + x1_offset);
  float param_height = *reinterpret_cast<float*>(pCameraParamsObject + y2_offset) - *reinterpret_cast<float*>(pCameraParamsObject + y1_offset);
  pfnUpdateCameraProjection(pCameraObject, param_width, param_height);
}

void IGameCamera::ReassertCoreCameraFovReference(void* pCameraObject, float fov) {
  if (!pCameraObject) return;
  auto zoomBaseOffset = Data::GameData::GameDataCameraService::GetInstance().GetFovZoomBaseOffset();
  if (zoomBaseOffset) {
    *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pCameraObject) + zoomBaseOffset) = fov;
  }
}

}  // namespace GameCamera
SPF_NS_END
