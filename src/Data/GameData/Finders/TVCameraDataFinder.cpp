#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

bool TVCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING TV CAMERA OFFSET SEARCH (Reflection) ---");

  const char* CLASS_NAME_TV = "vehicle_tv_camera";
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

  // --- Step 1: Find vehicle_tv_camera SII Attributes ---
  
  uintptr_t maxDist = getAttr(CLASS_NAME_TV, "max_distance");
  if (maxDist) owner.SetTVMaxDistanceOffset(static_cast<intptr_t>(maxDist));

  uintptr_t prefabUplift = getAttr(CLASS_NAME_TV, "prefab_uplift");
  if (prefabUplift) {
    owner.SetTVPrefabUpliftXOffset(static_cast<intptr_t>(prefabUplift));
    owner.SetTVPrefabUpliftYOffset(static_cast<intptr_t>(prefabUplift + 4));
    owner.SetTVPrefabUpliftZOffset(static_cast<intptr_t>(prefabUplift + 8));
  }

  uintptr_t roadUplift = getAttr(CLASS_NAME_TV, "road_uplift");
  if (roadUplift) {
    owner.SetTVRoadUpliftXOffset(static_cast<intptr_t>(roadUplift));
    owner.SetTVRoadUpliftYOffset(static_cast<intptr_t>(roadUplift + 4));
    owner.SetTVRoadUpliftZOffset(static_cast<intptr_t>(roadUplift + 8));
  }

  // --- Step 2: Find core_camera SII Attributes ---

  uintptr_t camFov = getAttr(CLASS_NAME_CORE_CAMERA, "camera_fov");
  if (camFov) owner.SetCameraFovOffset(static_cast<intptr_t>(camFov));

  uintptr_t shakeAnimStep = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_step");
  if (shakeAnimStep) owner.SetShakeAnimStepOffset(static_cast<intptr_t>(shakeAnimStep));

  uintptr_t shakeAnimScaleMin = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_scale_min");
  if (shakeAnimScaleMin) owner.SetShakeAnimScaleMinOffset(static_cast<intptr_t>(shakeAnimScaleMin));

  uintptr_t shakeAnimScaleMax = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim_scale_max");
  if (shakeAnimScaleMax) owner.SetShakeAnimScaleMaxOffset(static_cast<intptr_t>(shakeAnimScaleMax));

  uintptr_t shakeAnim = getAttr(CLASS_NAME_CORE_CAMERA, "shake_anim");
  if (shakeAnim) owner.SetShakeAnimOffset(static_cast<intptr_t>(shakeAnim));

  // --- Final Readiness Check ---
  m_isReady = all_found && (owner.GetTVMaxDistanceOffset() != 0 &&
                           owner.GetCameraFovOffset() != 0 &&
                           owner.GetShakeAnimStepOffset() != 0 &&
                           owner.GetShakeAnimScaleMinOffset() != 0 &&
                           owner.GetShakeAnimScaleMaxOffset() != 0 &&
                           owner.GetShakeAnimOffset() != 0);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- TV CAMERA OFFSETS FOUND. TVCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize one or more TV Camera offsets. ({} ms)", duration);
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
