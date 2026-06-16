#include "SPF/Data/GameData/Finders/TopCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

bool TopCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING TOP CAMERA OFFSET SEARCH (Reflection) ---");

  const char* CLASS_NAME_TOP = "vehicle_top_camera";
  const char* CLASS_NAME_VEHICLE_CAM = "vehicle_camera";
  const char* CLASS_NAME_CORE_CAMERA = "core_camera";
  bool all_found = true;

  // Lambda helper to safely extract, validate, and log offsets from the SCS Reflection Table
  auto getAttr = [&](const char* className, const char* name) -> uintptr_t {
    uintptr_t off = PatternFinder::FindAttributeOffset(className, name);
    if (off && PatternFinder::IsSaneOffset(static_cast<int32_t>(off))) {
      logger->Debug("1.[REFLECTION] Verified '{}'::'{}' at offset 0x{:X}", className, name, off);
      return off;
    }
    logger->Error("1.[REFLECTION] FAILED to find or validate '{}'::'{}' (Offset: 0x{:X})", className, name, off);
    all_found = false;
    return 0;
  };

  // --- Step 1: Find vehicle_top_camera SII Attributes ---
  
  uintptr_t minH = getAttr(CLASS_NAME_TOP, "minimum_height");
  if (minH) owner.SetTopMinHeightOffset(static_cast<intptr_t>(minH));

  uintptr_t maxH = getAttr(CLASS_NAME_TOP, "maximum_height");
  if (maxH) owner.SetTopMaxHeightOffset(static_cast<intptr_t>(maxH));

  uintptr_t speed = getAttr(CLASS_NAME_TOP, "speed");
  if (speed) owner.SetTopSpeedOffset(static_cast<intptr_t>(speed));

  uintptr_t xFwd = getAttr(CLASS_NAME_TOP, "x_offset_forward");
  if (xFwd) owner.SetTopXOffsetForwardOffset(static_cast<intptr_t>(xFwd));

  uintptr_t xBwd = getAttr(CLASS_NAME_TOP, "x_offset_backward");
  if (xBwd) owner.SetTopXOffsetBackwardOffset(static_cast<intptr_t>(xBwd));

  uintptr_t fwd = getAttr(CLASS_NAME_TOP, "offset_forward");
  if (fwd) owner.SetTopOffsetForwardOffset(static_cast<intptr_t>(fwd));

  uintptr_t bwd = getAttr(CLASS_NAME_TOP, "offset_backward");
  if (bwd) owner.SetTopOffsetBackwardOffset(static_cast<intptr_t>(bwd));

  uintptr_t heightFactor = getAttr(CLASS_NAME_TOP, "camera_height_factor");
  if (heightFactor) owner.SetTopCameraHeightFactorOffset(static_cast<intptr_t>(heightFactor));

  uintptr_t useAdaptive = getAttr(CLASS_NAME_TOP, "use_adaptive_camera_height");
  if (useAdaptive) owner.SetTopUseAdaptiveCameraHeightOffset(static_cast<intptr_t>(useAdaptive));

  // --- Step 2: Find vehicle_camera SII Attributes ---

  uintptr_t validation = getAttr(CLASS_NAME_VEHICLE_CAM, "validation");
  if (validation) owner.SetTopValidationOffset(static_cast<intptr_t>(validation));

  uintptr_t valSpdPos = getAttr(CLASS_NAME_VEHICLE_CAM, "validation_speed_positive");
  if (valSpdPos) owner.SetTopValidationSpeedPositiveOffset(static_cast<intptr_t>(valSpdPos));

  uintptr_t valSpdNeg = getAttr(CLASS_NAME_VEHICLE_CAM, "validation_speed_negative");
  if (valSpdNeg) owner.SetTopValidationSpeedNegativeOffset(static_cast<intptr_t>(valSpdNeg));

  // --- Step 3: Find core_camera SII Attributes ---

  uintptr_t nearP = getAttr(CLASS_NAME_CORE_CAMERA, "near_plane");
  if (nearP) owner.SetTopNearPlaneOffset(static_cast<intptr_t>(nearP));

  uintptr_t farP = getAttr(CLASS_NAME_CORE_CAMERA, "far_plane");
  if (farP) owner.SetTopFarPlaneOffset(static_cast<intptr_t>(farP));

  uintptr_t camFov = getAttr(CLASS_NAME_CORE_CAMERA, "camera_fov");
  if (camFov) owner.SetCameraFovOffset(static_cast<intptr_t>(camFov));

  uintptr_t mouseSens = getAttr(CLASS_NAME_CORE_CAMERA, "mouse_sensitivity");
  if (mouseSens) owner.SetMouseSensitivityOffset(static_cast<intptr_t>(mouseSens));

  uintptr_t shakeAnimStep = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_step");
  if (shakeAnimStep) owner.SetShakeAnimStepOffset(static_cast<intptr_t>(shakeAnimStep));

  uintptr_t shakeAnimScaleMin = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_scale_min");
  if (shakeAnimScaleMin) owner.SetShakeAnimScaleMinOffset(static_cast<intptr_t>(shakeAnimScaleMin));

  uintptr_t shakeAnimScaleMax = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_scale_max");
  if (shakeAnimScaleMax) owner.SetShakeAnimScaleMaxOffset(static_cast<intptr_t>(shakeAnimScaleMax));

  uintptr_t shakeAnim = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim");
  if (shakeAnim) owner.SetShakeAnimOffset(static_cast<intptr_t>(shakeAnim));

  // --- Final Readiness Check ---
  m_isReady = all_found && (owner.GetTopMinHeightOffset() != 0 &&
                           owner.GetTopMaxHeightOffset() != 0 &&
                           owner.GetTopSpeedOffset() != 0 &&
                           owner.GetTopXOffsetForwardOffset() != 0 &&
                           owner.GetTopXOffsetBackwardOffset() != 0 &&
                           owner.GetTopOffsetForwardOffset() != 0 &&
                           owner.GetTopOffsetBackwardOffset() != 0 &&
                           owner.GetTopCameraHeightFactorOffset() != 0 &&
                           owner.GetTopUseAdaptiveCameraHeightOffset() != 0 &&
                           owner.GetTopNearPlaneOffset() != 0 &&
                           owner.GetTopFarPlaneOffset() != 0 &&
                           owner.GetTopValidationOffset() != 0 &&
                           owner.GetTopValidationSpeedPositiveOffset() != 0 &&
                           owner.GetTopValidationSpeedNegativeOffset() != 0 &&
                           owner.GetCameraFovOffset() != 0 &&
                           owner.GetMouseSensitivityOffset() != 0 &&
                           owner.GetShakeAnimStepOffset() != 0 &&
                           owner.GetShakeAnimScaleMinOffset() != 0 &&
                           owner.GetShakeAnimScaleMaxOffset() != 0 &&
                           owner.GetShakeAnimOffset() != 0);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- TOP CAMERA OFFSETS FOUND. TopCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize one or more Top Camera offsets. ({} ms)", duration);
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
