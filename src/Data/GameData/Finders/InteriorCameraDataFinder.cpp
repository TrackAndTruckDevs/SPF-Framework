#include "SPF/Data/GameData/Finders/InteriorCameraDataFinder.hpp"

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

namespace {

/**
 * @brief Chained pattern for Live Yaw and Pitch within UpdateInteriorCameraOrientation.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCameraOrientation[140876d40]) ---/
 * 140876eea  F3 0F 59 D3                   MULSS XMM2,XMM3
 * 140876eee  F3 0F 11 83 98 05 00 00       MOVSS dword ptr [RBX + 0x598],XMM0
 * 140876ef6  0F 2F D1                      COMISS XMM2,XMM1
 * 140876ef9  72 07                         JC 0x140876f02
 */
const char* LIVE_YAW_PITCH_SIG = "[MULSS xmm, xmm] [MOVSS [r64+off32], xmm] [COMISS xmm, xmm] [JB rel8]";

}  // namespace

bool InteriorCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  const char* CLASS_NAME_INTERIOR = "vehicle_interior_camera";
  const char* CLASS_NAME_AZIMUTH = "camera_azimuth_range";
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

    // Head Position (head_offset is a float3/Vector3 vector)
    uintptr_t headX = getAttr(CLASS_NAME_INTERIOR, "head_offset");
    if (headX) {
      owner.SetInteriorSeatXOffset(static_cast<intptr_t>(headX));
      owner.SetInteriorSeatYOffset(static_cast<intptr_t>(headX + 4));
      owner.SetInteriorSeatZOffset(static_cast<intptr_t>(headX + 8));
    }

    uintptr_t interiorOutside = getAttr(CLASS_NAME_INTERIOR, "outside");
    if (interiorOutside) owner.SetInteriorOutsideOffset(static_cast<intptr_t>(interiorOutside));

    // Azimuth Overrides Array offset
    uintptr_t azimuthOverrides = getAttr(CLASS_NAME_INTERIOR, "azimuth_overrides");
    if (azimuthOverrides) {
      owner.SetInteriorAzimuthOverridesOffset(static_cast<intptr_t>(azimuthOverrides));
    }

    // Rotation Limits from config
    uintptr_t limitLeft = getAttr(CLASS_NAME_INTERIOR, "mouse_left_limit");
    if (limitLeft) owner.SetInteriorLimitLeftOffset(static_cast<intptr_t>(limitLeft));

    uintptr_t limitRight = getAttr(CLASS_NAME_INTERIOR, "mouse_right_limit");
    if (limitRight) owner.SetInteriorLimitRightOffset(static_cast<intptr_t>(limitRight));

    // Default Positions from config
    uintptr_t defHoriz = getAttr(CLASS_NAME_INTERIOR, "mouse_left_right_default");
    if (defHoriz) owner.SetInteriorMouseLRDefaultOffset(static_cast<intptr_t>(defHoriz));

    uintptr_t limitUp = getAttr(CLASS_NAME_INTERIOR, "mouse_up_limit");
    if (limitUp) owner.SetInteriorLimitUpOffset(static_cast<intptr_t>(limitUp));

    uintptr_t limitDown = getAttr(CLASS_NAME_INTERIOR, "mouse_down_limit");
    if (limitDown) owner.SetInteriorLimitDownOffset(static_cast<intptr_t>(limitDown));

    uintptr_t defVert = getAttr(CLASS_NAME_INTERIOR, "mouse_up_down_default");
    if (defVert) owner.SetInteriorMouseUDDefaultOffset(static_cast<intptr_t>(defVert));

    uintptr_t zoomFov = getAttr(CLASS_NAME_INTERIOR, "zoom_fov_factor");
    if (zoomFov) owner.SetZoomFovFactorOffset(static_cast<intptr_t>(zoomFov));

    uintptr_t zoomSpeed = getAttr(CLASS_NAME_INTERIOR, "zoom_speed");
    if (zoomSpeed) owner.SetZoomSpeedOffset(static_cast<intptr_t>(zoomSpeed));

    // core_camera SII Attributes
    uintptr_t camFov = getAttr(CLASS_NAME_CORE_CAMERA, "camera_fov");
    if (camFov) owner.SetCameraFovOffset(static_cast<intptr_t>(camFov));

    uintptr_t nearPlane = getAttr(CLASS_NAME_CORE_CAMERA, "near_plane");
    if (nearPlane) owner.SetNearPlaneOffset(static_cast<intptr_t>(nearPlane));

    uintptr_t farPlane = getAttr(CLASS_NAME_CORE_CAMERA, "far_plane");
    if (farPlane) owner.SetFarPlaneOffset(static_cast<intptr_t>(farPlane));

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

    uintptr_t handShakeLimit = getAttr(CLASS_NAME_CORE_CAMERA, "hand_shake_limit");
    if (handShakeLimit) owner.SetHandShakeLimitOffset(static_cast<intptr_t>(handShakeLimit));

    uintptr_t handShakeSpeed = getAttr(CLASS_NAME_CORE_CAMERA, "hand_shake_speed");
    if (handShakeSpeed) owner.SetHandShakeSpeedOffset(static_cast<intptr_t>(handShakeSpeed));

    // camera_azimuth_range SII Attributes
    uintptr_t rangeOutside = getAttr(CLASS_NAME_AZIMUTH, "outside");
    if (rangeOutside) owner.SetAzimuthRangeOutsideOffset(static_cast<intptr_t>(rangeOutside));

    uintptr_t rangeStartAzimuth = getAttr(CLASS_NAME_AZIMUTH, "start_azimuth");
    if (rangeStartAzimuth) owner.SetAzimuthRangeStartAzimuthOffset(static_cast<intptr_t>(rangeStartAzimuth));

    uintptr_t rangeEndAzimuth = getAttr(CLASS_NAME_AZIMUTH, "end_azimuth");
    if (rangeEndAzimuth) owner.SetAzimuthRangeEndAzimuthOffset(static_cast<intptr_t>(rangeEndAzimuth));

    uintptr_t rangeStartUpLimit = getAttr(CLASS_NAME_AZIMUTH, "start_up_limit");
    if (rangeStartUpLimit) owner.SetAzimuthRangeStartUpLimitOffset(static_cast<intptr_t>(rangeStartUpLimit));

    uintptr_t rangeEndUpLimit = getAttr(CLASS_NAME_AZIMUTH, "end_up_limit");
    if (rangeEndUpLimit) owner.SetAzimuthRangeEndUpLimitOffset(static_cast<intptr_t>(rangeEndUpLimit));

    uintptr_t rangeStartDownLimit = getAttr(CLASS_NAME_AZIMUTH, "start_down_limit");
    if (rangeStartDownLimit) owner.SetAzimuthRangeStartDownLimitOffset(static_cast<intptr_t>(rangeStartDownLimit));

    uintptr_t rangeEndDownLimit = getAttr(CLASS_NAME_AZIMUTH, "end_down_limit");
    if (rangeEndDownLimit) owner.SetAzimuthRangeEndDownLimitOffset(static_cast<intptr_t>(rangeEndDownLimit));

    uintptr_t rangeStartUpDownDef = getAttr(CLASS_NAME_AZIMUTH, "start_up_down_default");
    if (rangeStartUpDownDef) owner.SetAzimuthRangeStartUpDownDefaultOffset(static_cast<intptr_t>(rangeStartUpDownDef));

    uintptr_t rangeEndUpDownDef = getAttr(CLASS_NAME_AZIMUTH, "end_up_down_default");
    if (rangeEndUpDownDef) owner.SetAzimuthRangeEndUpDownDefaultOffset(static_cast<intptr_t>(rangeEndUpDownDef));

    uintptr_t rangeStartLRDef = getAttr(CLASS_NAME_AZIMUTH, "start_left_right_default");
    if (rangeStartLRDef) owner.SetAzimuthRangeStartLeftRightDefaultOffset(static_cast<intptr_t>(rangeStartLRDef));

    uintptr_t rangeEndLRDef = getAttr(CLASS_NAME_AZIMUTH, "end_left_right_default");
    if (rangeEndLRDef) owner.SetAzimuthRangeEndLeftRightDefaultOffset(static_cast<intptr_t>(rangeEndLRDef));

    uintptr_t rangeStartHeadOffset = getAttr(CLASS_NAME_AZIMUTH, "start_head_offset_offset");
    if (rangeStartHeadOffset) owner.SetAzimuthRangeStartHeadOffsetOffset(static_cast<intptr_t>(rangeStartHeadOffset));

    uintptr_t rangeEndHeadOffset = getAttr(CLASS_NAME_AZIMUTH, "end_head_offset_offset");
    if (rangeEndHeadOffset) owner.SetAzimuthRangeEndHeadOffsetOffset(static_cast<intptr_t>(rangeEndHeadOffset));
  }

  // ── Phase 2: Runtime State ──
  // These variables are updated dynamically at runtime and do not exist in the reflection table.
  // We locate them by scanning the code of the camera orientation update function globally.
  {
    auto phase = log.MakePhase("Runtime State");

    uintptr_t addr = PatternFinder::Find(LIVE_YAW_PITCH_SIG);
    if (phase.Step(addr, "Live Yaw/Pitch chain", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCameraOrientation[140876d40]) ---/
      // * 140876eee  F3 0F 11 83 98 05 00 00       MOVSS dword ptr [RBX + 0x598],XMM0
      uintptr_t addrLiveYaw = PatternFinder::Find(addr, 16, "[MOVSS [r64+off32], xmm]");
      if (phase.Step(addrLiveYaw, "Live Yaw MOVSS", "RT")) {
        int32_t liveYaw = PatternFinder::ReadInt32(addrLiveYaw + 4);

        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCameraOrientation[140876d40]) ---/
        // * 140876f09  F3 0F 11 8B 9C 05 00 00       MOVSS dword ptr [RBX + 0x59c],XMM1
        uintptr_t movss2Addr = PatternFinder::Find(addrLiveYaw + 4, 48, "[MOVSS [r64+off32], xmm]");
        if (phase.Step(movss2Addr, "Live Pitch MOVSS", "RT")) {
          int32_t livePitch = PatternFinder::ReadInt32(movss2Addr + 4);

          bool okYaw = phase.StepOffset(liveYaw, "LiveYaw", "OFF");
          bool okPitch = phase.StepOffset(livePitch, "LivePitch", "OFF");
          if (okYaw && okPitch) {
            owner.SetInteriorYawOffset(liveYaw);
            owner.SetInteriorPitchOffset(livePitch);
          }
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetInteriorSeatXOffset() != 0 && owner.GetInteriorSeatYOffset() != 0 && owner.GetInteriorSeatZOffset() != 0 && owner.GetInteriorLimitLeftOffset() != 0 && owner.GetInteriorLimitRightOffset() != 0 &&
              owner.GetInteriorLimitUpOffset() != 0 && owner.GetInteriorLimitDownOffset() != 0 && owner.GetInteriorOutsideOffset() != 0 && owner.GetInteriorMouseLRDefaultOffset() != 0 && owner.GetInteriorMouseUDDefaultOffset() != 0 &&
              owner.GetInteriorYawOffset() != 0 && owner.GetInteriorPitchOffset() != 0 && owner.GetInteriorAzimuthOverridesOffset() != 0 && owner.GetAzimuthRangeOutsideOffset() != 0 && owner.GetAzimuthRangeStartAzimuthOffset() != 0 &&
              owner.GetAzimuthRangeEndAzimuthOffset() != 0 && owner.GetAzimuthRangeStartUpLimitOffset() != 0 && owner.GetAzimuthRangeEndUpLimitOffset() != 0 && owner.GetAzimuthRangeStartDownLimitOffset() != 0 &&
              owner.GetAzimuthRangeEndDownLimitOffset() != 0 && owner.GetAzimuthRangeStartUpDownDefaultOffset() != 0 && owner.GetAzimuthRangeEndUpDownDefaultOffset() != 0 && owner.GetAzimuthRangeStartLeftRightDefaultOffset() != 0 &&
              owner.GetAzimuthRangeEndLeftRightDefaultOffset() != 0 && owner.GetAzimuthRangeStartHeadOffsetOffset() != 0 && owner.GetAzimuthRangeEndHeadOffsetOffset() != 0 && owner.GetZoomFovFactorOffset() != 0 &&
              owner.GetZoomSpeedOffset() != 0 && owner.GetCameraFovOffset() != 0 && owner.GetNearPlaneOffset() != 0 && owner.GetFarPlaneOffset() != 0 && owner.GetMouseSensitivityOffset() != 0 && owner.GetShakeAnimStepOffset() != 0 &&
              owner.GetShakeAnimScaleMinOffset() != 0 && owner.GetShakeAnimScaleMaxOffset() != 0 && owner.GetHandShakeLimitOffset() != 0 && owner.GetHandShakeSpeedOffset() != 0 && owner.GetShakeAnimOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
