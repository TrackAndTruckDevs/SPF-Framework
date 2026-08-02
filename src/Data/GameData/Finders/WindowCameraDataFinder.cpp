#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"

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
 * Shared by Interior and Window cameras.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateInteriorCameraOrientation[140876d40]) ---/
 * 140876eea  F3 0F 59 D3                   MULSS XMM2,XMM3
 * 140876eee  F3 0F 11 83 98 05 00 00       MOVSS dword ptr [RBX + 0x598],XMM0
 * 140876ef6  0F 2F D1                      COMISS XMM2,XMM1
 * 140876ef9  72 07                         JC 0x140876f02
 */
const char* LIVE_YAW_PITCH_SIG = "[MULSS xmm, xmm] [MOVSS [r64+off32], xmm] [COMISS xmm, xmm] [JB rel8]";

}  // namespace

bool WindowCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  FinderLog log(GetName());

  const char* CLASS_NAME_WINDOW = "vehicle_interior_camera";
  const char* CLASS_NAME_CORE_CAMERA = "core_camera";

  // ── Phase 1: vehicle_interior_camera SII Attributes ──
  {
    auto phase = log.MakePhase("vehicle_interior_camera Reflection Attributes");

    auto getAttr = [&phase](const char* className, const char* attrName) -> uintptr_t {
      uintptr_t off = PatternFinder::FindAttributeOffset(className, attrName);
      std::string desc = fmt::format("{}::{}", className, attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    uintptr_t headOff = getAttr(CLASS_NAME_WINDOW, "head_offset");
    if (headOff) {
      owner.SetWindowHeadOffsetXOffset(static_cast<intptr_t>(headOff));
      owner.SetWindowHeadOffsetYOffset(static_cast<intptr_t>(headOff + 4));
      owner.SetWindowHeadOffsetZOffset(static_cast<intptr_t>(headOff + 8));
    }

    uintptr_t leftLim = getAttr(CLASS_NAME_WINDOW, "mouse_left_limit");
    if (leftLim) owner.SetWindowMouseLeftLimitOffset(static_cast<intptr_t>(leftLim));

    uintptr_t rightLim = getAttr(CLASS_NAME_WINDOW, "mouse_right_limit");
    if (rightLim) owner.SetWindowMouseRightLimitOffset(static_cast<intptr_t>(rightLim));

    uintptr_t upLim = getAttr(CLASS_NAME_WINDOW, "mouse_up_limit");
    if (upLim) owner.SetWindowMouseUpLimitOffset(static_cast<intptr_t>(upLim));

    uintptr_t downLim = getAttr(CLASS_NAME_WINDOW, "mouse_down_limit");
    if (downLim) owner.SetWindowMouseDownLimitOffset(static_cast<intptr_t>(downLim));

    uintptr_t lrDef = getAttr(CLASS_NAME_WINDOW, "mouse_left_right_default");
    if (lrDef) owner.SetWindowMouseLRDefaultOffset(static_cast<intptr_t>(lrDef));

    uintptr_t udDef = getAttr(CLASS_NAME_WINDOW, "mouse_up_down_default");
    if (udDef) owner.SetWindowMouseUDDefaultOffset(static_cast<intptr_t>(udDef));

    uintptr_t relAzimuth = getAttr(CLASS_NAME_WINDOW, "relative_headtracking_azimuth");
    if (relAzimuth) owner.SetWindowRelativeHeadtrackingAzimuthOffset(static_cast<intptr_t>(relAzimuth));

    uintptr_t autoCenter = getAttr(CLASS_NAME_WINDOW, "auto_center_move_direction");
    if (autoCenter) owner.SetWindowAutoCenterMoveDirectionOffset(static_cast<intptr_t>(autoCenter));
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

  // ── Phase 3: Runtime State (Live Yaw/Pitch) ──
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
            owner.SetWindowLiveYawOffset(liveYaw);
            owner.SetWindowLivePitchOffset(livePitch);
          }
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetWindowHeadOffsetXOffset() != 0 && owner.GetWindowMouseLeftLimitOffset() != 0 && owner.GetWindowLiveYawOffset() != 0 && owner.GetWindowRelativeHeadtrackingAzimuthOffset() != 0 &&
              owner.GetWindowAutoCenterMoveDirectionOffset() != 0 && owner.GetCameraFovOffset() != 0 && owner.GetMouseSensitivityOffset() != 0 && owner.GetShakeAnimStepOffset() != 0 && owner.GetShakeAnimScaleMinOffset() != 0 &&
              owner.GetShakeAnimScaleMaxOffset() != 0 && owner.GetShakeAnimOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
