#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "fmt/format.h"

#include <cstdint>
#include <string>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

bool TVCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  FinderLog log(GetName());

  const char* CLASS_NAME_TV = "vehicle_tv_camera";
  const char* CLASS_NAME_CORE_CAMERA = "core_camera";

  // ── Phase 1: vehicle_tv_camera SII Attributes ──
  {
    auto phase = log.MakePhase("vehicle_tv_camera Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

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
  }

  // ── Phase 2: core_camera SII Attributes ──
  {
    auto phase = log.MakePhase("core_camera Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

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
  }

  // --- Final Readiness Check ---
  m_isReady = (owner.GetTVMaxDistanceOffset() != 0 && owner.GetCameraFovOffset() != 0 && owner.GetShakeAnimStepOffset() != 0 && owner.GetShakeAnimScaleMinOffset() != 0 && owner.GetShakeAnimScaleMaxOffset() != 0 && owner.GetShakeAnimOffset() != 0);

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
