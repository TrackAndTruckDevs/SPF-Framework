#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "fmt/format.h"

#include <cstddef>
#include <cstdint>
#include <string>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Anchor signature for the UpdateCameraInput function (Behind Camera).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraInput[140a6f190]) ---/
 * 140a6f190  40 53                         PUSH RBX
 * 140a6f192  48 83 EC 40                   SUB RSP,0x40
 * 140a6f196  48 8B 01                      MOV RAX,qword ptr [RCX]
 * 140a6f199  48 8B D9                      MOV RBX,RCX
 * 140a6f19c  0F 29 7C 24 30                MOVAPS xmmword ptr [RSP + 0x30],XMM7
 * 140a6f1a1  F3 0F 10 79 14                MOVSS XMM7,dword ptr [RCX + 0x14]
 */
const char* UPDATE_INPUT_ANCHOR_SIG = "40 [PUSH r64] [SUB r64, imm8] [MOV r64, [r64]] [MOV r64, r64] [MOVAPS [r64+off8], xmm] [MOVSS xmm, [r64+off8]]";

/**
 * @brief Logic chain for Live Yaw (Horizontal rotation).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraInput[140a6f190]) ---/
 * 140a6f1da  F3 0F 58 83 D4 04 00 00       ADDSS XMM0,dword ptr [RBX + 0x4d4] -> [LiveYaw offset]
 * 140a6f1e2  F3 0F 11 83 D4 04 00 00       MOVSS dword ptr [RBX + 0x4d4],XMM0
 * 140a6f1ea  E8 C1 C5 AD FF                CALL 0x14054b7b0
 */
const char* YAW_LOGIC_SIG = "[ADDSS xmm, [r64+off32]] [MOVSS [r64+off32], xmm] [CALL rel32]";

/**
 * @brief Logic chain for Live Pitch (Vertical rotation).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraInput[140a6f190]) ---/
 * 140a6f24a  F3 0F 11 7B 14                MOVSS dword ptr [RBX + 0x14],XMM7 -< [LivePitch offset]
 * 140a6f24f  B2 01                         MOV DL,0x1
 */
const char* PITCH_LOGIC_SIG = "[MOVSS [r64+off8], xmm] [MOV r8, imm8]";

/**
 * @brief Logic chain for Live Zoom (Distance) with safety verification.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraInput[140a6f190]) ---/
 * 140a6f25a  F3 0F 10 83 D8 04 00 00       MOVSS XMM0,dword ptr [RBX + 0x4d8] -> [LiveZoom offset]
 * 140a6f262  F3 0F 5C 83 94 04 00 00       SUBSS XMM0,dword ptr [RBX + 0x494]
 */
const char* ZOOM_LOGIC_SIG = "[MOVSS xmm, [r64+off32]] F3 0F 5C 83";

}  // namespace

bool BehindCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  const char* CLASS_NAME_BEHIND = "vehicle_behind_rotation_camera";
  const char* CLASS_NAME_VEHICLE_CAM = "vehicle_camera";
  const char* CLASS_NAME_CORE_CAMERA = "core_camera";

  // ── Phase 1: Camera Reflection Attributes ──
  {
    auto phase = log.MakePhase("Camera Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    // Distance Settings
    uintptr_t distInterval = getAttr(CLASS_NAME_BEHIND, "distance_interval");
    if (distInterval) {
      owner.SetBehindDistanceMinOffset(static_cast<intptr_t>(distInterval));
      owner.SetBehindDistanceMaxOffset(static_cast<intptr_t>(distInterval + 4));
    }

    uintptr_t distTrlMaxOff = getAttr(CLASS_NAME_BEHIND, "distance_trailer_max_offset");
    if (distTrlMaxOff) owner.SetBehindDistanceTrailerMaxOffset(static_cast<intptr_t>(distTrlMaxOff));

    uintptr_t distDef = getAttr(CLASS_NAME_BEHIND, "distance_default");
    if (distDef) owner.SetBehindDistanceDefaultOffset(static_cast<intptr_t>(distDef));

    uintptr_t distTrlDef = getAttr(CLASS_NAME_BEHIND, "distance_trailer_default");
    if (distTrlDef) owner.SetBehindDistanceTrailerDefaultOffset(static_cast<intptr_t>(distTrlDef));

    uintptr_t distChgSpd = getAttr(CLASS_NAME_BEHIND, "distance_change_speed");
    if (distChgSpd) owner.SetBehindDistanceChangeSpeedOffset(static_cast<intptr_t>(distChgSpd));

    uintptr_t distLazSpd = getAttr(CLASS_NAME_BEHIND, "distance_laziness_speed");
    if (distLazSpd) owner.SetBehindDistanceLazinessSpeedOffset(static_cast<intptr_t>(distLazSpd));

    // Azimuth Settings
    uintptr_t azimLazSpd = getAttr(CLASS_NAME_BEHIND, "azimuth_laziness_speed");
    if (azimLazSpd) owner.SetBehindAzimuthLazinessSpeedOffset(static_cast<intptr_t>(azimLazSpd));

    // Elevation Settings
    uintptr_t elevInterval = getAttr(CLASS_NAME_BEHIND, "elevation_interval");
    if (elevInterval) {
      owner.SetBehindElevationMinOffset(static_cast<intptr_t>(elevInterval));
      owner.SetBehindElevationMaxOffset(static_cast<intptr_t>(elevInterval + 4));
    }

    uintptr_t elevDef = getAttr(CLASS_NAME_BEHIND, "elevation_default");
    if (elevDef) owner.SetBehindElevationDefaultOffset(static_cast<intptr_t>(elevDef));

    uintptr_t elevTrlDef = getAttr(CLASS_NAME_BEHIND, "elevation_trailer_default");
    if (elevTrlDef) owner.SetBehindElevationTrailerDefaultOffset(static_cast<intptr_t>(elevTrlDef));

    uintptr_t heightLim = getAttr(CLASS_NAME_BEHIND, "height_limit");
    if (heightLim) owner.SetBehindHeightLimitOffset(static_cast<intptr_t>(heightLim));

    // Pivot Offset (Vector3)
    uintptr_t pivotOff = getAttr(CLASS_NAME_BEHIND, "pivot_offset");
    if (pivotOff) {
      owner.SetBehindPivotXOffset(static_cast<intptr_t>(pivotOff));
      owner.SetBehindPivotYOffset(static_cast<intptr_t>(pivotOff + 4));
      owner.SetBehindPivotZOffset(static_cast<intptr_t>(pivotOff + 8));
    }

    // Dynamic Offset Settings
    uintptr_t dynMax = getAttr(CLASS_NAME_BEHIND, "dynamic_offset_max");
    if (dynMax) owner.SetBehindDynamicOffsetMaxOffset(static_cast<intptr_t>(dynMax));

    uintptr_t dynSpdInterval = getAttr(CLASS_NAME_BEHIND, "dynamic_offset_speed_interval");
    if (dynSpdInterval) {
      owner.SetBehindDynamicOffsetSpeedMinOffset(static_cast<intptr_t>(dynSpdInterval));
      owner.SetBehindDynamicOffsetSpeedMaxOffset(static_cast<intptr_t>(dynSpdInterval + 4));
    }

    uintptr_t dynLazSpd = getAttr(CLASS_NAME_BEHIND, "dynamic_offset_laziness_speed");
    if (dynLazSpd) owner.SetBehindDynamicOffsetLazinessSpeedOffset(static_cast<intptr_t>(dynLazSpd));

    // vehicle_camera SII Attributes
    uintptr_t validation = getAttr(CLASS_NAME_VEHICLE_CAM, "validation");
    if (validation) owner.SetBehindValidationOffset(static_cast<intptr_t>(validation));

    uintptr_t valSpdPos = getAttr(CLASS_NAME_VEHICLE_CAM, "validation_speed_positive");
    if (valSpdPos) owner.SetBehindValidationSpeedPositiveOffset(static_cast<intptr_t>(valSpdPos));

    uintptr_t valSpdNeg = getAttr(CLASS_NAME_VEHICLE_CAM, "validation_speed_negative");
    if (valSpdNeg) owner.SetBehindValidationSpeedNegativeOffset(static_cast<intptr_t>(valSpdNeg));

    uintptr_t valRadius = getAttr(CLASS_NAME_VEHICLE_CAM, "validation_radius");
    if (valRadius) owner.SetBehindValidationRadiusOffset(static_cast<intptr_t>(valRadius));

    uintptr_t speedFovFact = getAttr(CLASS_NAME_VEHICLE_CAM, "speed_fov_change_factor");
    if (speedFovFact) owner.SetBehindSpeedFovChangeFactorOffset(static_cast<intptr_t>(speedFovFact));

    // core_camera SII Attributes
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
  }

  // ── Phase 2: Runtime State ──
  // These are updated dynamically and aren't in reflection tables.
  // We locate them within the UpdateCameraInput function using robust logic chains.
  {
    auto phase = log.MakePhase("Runtime State");

    uintptr_t pfnUpdateInput = PatternFinder::Find(UPDATE_INPUT_ANCHOR_SIG);
    if (phase.Step(pfnUpdateInput, "UpdateCameraInput anchor", "RT")) {
      const size_t FUNC_RANGE = 512;

      // 2.1 Live Yaw (Horizontal)
      uintptr_t yawBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, YAW_LOGIC_SIG);
      if (phase.Step(yawBlock, "LiveYaw logic", "RT")) {
        int32_t liveYaw = PatternFinder::ReadInt32(yawBlock + 4);
        phase.StepOffset(liveYaw, "LiveYaw", "OFF");
        if (PatternFinder::IsSaneOffset(liveYaw)) {
          owner.SetBehindLiveYawOffset(liveYaw);
        }
      }

      // 2.2 Live Pitch (Vertical)
      uintptr_t pitchBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, PITCH_LOGIC_SIG);
      if (phase.Step(pitchBlock, "LivePitch logic", "RT")) {
        int8_t livePitch = PatternFinder::ReadInt8(pitchBlock + 4);
        phase.StepOffset(livePitch, "LivePitch", "OFF");
        if (PatternFinder::IsSaneOffset(livePitch)) {
          owner.SetBehindLivePitchOffset(static_cast<intptr_t>(livePitch));
        }
      }

      // 2.3 Live Zoom (Distance)
      uintptr_t zoomBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, ZOOM_LOGIC_SIG);
      if (phase.Step(zoomBlock, "LiveZoom logic", "RT")) {
        int32_t liveZoom = PatternFinder::ReadInt32(zoomBlock + 4);
        phase.StepOffset(liveZoom, "LiveZoom", "OFF");
        if (PatternFinder::IsSaneOffset(liveZoom)) {
          owner.SetBehindLiveZoomOffset(liveZoom);
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady =
    (owner.GetBehindDistanceMinOffset() != 0 && owner.GetBehindDistanceMaxOffset() != 0 && owner.GetBehindDistanceTrailerMaxOffset() != 0 && owner.GetBehindDistanceDefaultOffset() != 0 && owner.GetBehindDistanceTrailerDefaultOffset() != 0 &&
     owner.GetBehindDistanceChangeSpeedOffset() != 0 && owner.GetBehindDistanceLazinessSpeedOffset() != 0 && owner.GetBehindAzimuthLazinessSpeedOffset() != 0 && owner.GetBehindElevationMinOffset() != 0 && owner.GetBehindElevationMaxOffset() != 0 &&
     owner.GetBehindElevationDefaultOffset() != 0 && owner.GetBehindElevationTrailerDefaultOffset() != 0 && owner.GetBehindHeightLimitOffset() != 0 && owner.GetBehindPivotXOffset() != 0 && owner.GetBehindPivotYOffset() != 0 &&
     owner.GetBehindPivotZOffset() != 0 && owner.GetBehindDynamicOffsetMaxOffset() != 0 && owner.GetBehindDynamicOffsetSpeedMinOffset() != 0 && owner.GetBehindDynamicOffsetSpeedMaxOffset() != 0 &&
     owner.GetBehindDynamicOffsetLazinessSpeedOffset() != 0 && owner.GetBehindLiveYawOffset() != 0 && owner.GetBehindLivePitchOffset() != 0 && owner.GetBehindLiveZoomOffset() != 0 && owner.GetBehindValidationOffset() != 0 &&
     owner.GetBehindValidationSpeedPositiveOffset() != 0 && owner.GetBehindValidationSpeedNegativeOffset() != 0 && owner.GetBehindValidationRadiusOffset() != 0 && owner.GetBehindSpeedFovChangeFactorOffset() != 0 && owner.GetCameraFovOffset() != 0 &&
     owner.GetMouseSensitivityOffset() != 0 && owner.GetShakeAnimStepOffset() != 0 && owner.GetShakeAnimScaleMinOffset() != 0 && owner.GetShakeAnimScaleMaxOffset() != 0 && owner.GetShakeAnimOffset() != 0);

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
