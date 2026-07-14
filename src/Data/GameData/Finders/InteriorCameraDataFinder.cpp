#include "SPF/Data/GameData/Finders/InteriorCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <chrono>
#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Chained pattern for Live Yaw and Pitch within UpdateInteriorCameraOrientation.
 * We match a sequence of MULSS, MOVSS, COMISS, a conditional jump, MOVAPS, MINSS, LEA, and MOVSS.
 * This pattern targets the live orientation write-back instructions inside the camera update.
 *
 * Target Code Snippet (Verified for Game Version 1.60 at 140876eea):
 * 140876eea f3 0f 59 d3                MULSS      XMM2,XMM3
 * 140876eee f3 0f 11 83 98 05 00 00    MOVSS      dword ptr [RBX + 0x598],XMM0   <-- Live Yaw (Offset +0x08)
 * 140876ef6 0f 2f d1                   COMISS     XMM2,XMM1
 * 140876ef9 72 07                      JC         LAB_140876f02                  <-- Conditional Jump (Offset +0x0F)
 * 140876efb 0f 28 cc                   MOVAPS     XMM1,XMM4
 * 140876efe f3 0f 5d ca                MINSS      XMM1,XMM2
 * 140876f02 48 8d 8b 28 03 00 00       LEA        RCX,[RBX + 0x328]              <-- LEA (Offset +0x1B)
 * 140876f09 f3 0f 11 8b 9c 05 00 00    MOVSS      dword ptr [RBX + 0x59c],XMM1   <-- Live Pitch (Offset +0x29)
 *
 * Strategy:
 * Use value ranges for registers ([C0-FF], [80-8F], [40-4F]) and variable wildcards [2-6?]
 * for the Jcc to prevent compiler optimizations or register allocation changes from breaking the match.
 */
const char* LIVE_YAW_PITCH_SIG =
  "F3 0F 59 [C0-FF] "
  "F3 0F 11 [80-8F] ?? ?? ?? ??"
  " 0F 2F [C0-FF] "
  "[2-6?] "
  "0F 28 [C0-FF] "
  "F3 0F 5D [C0-FF] "
  "[40-4F] 8D [80-8F] ?? ?? ?? ??"
  " F3 0F 11 [80-8F] ?? ?? ?? ??";

}  // namespace

bool InteriorCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING INTERIOR CAMERA OFFSET SEARCH ---");

  const char* CLASS_NAME_INTERIOR = "vehicle_interior_camera";
  const char* CLASS_NAME_AZIMUTH = "camera_azimuth_range";
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

  // --- Step 1: Find interior_camera SII Attributes via Reflection Table ---
  // We fetch static configuration offsets directly from the game's registration table.

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

  // --- Step 2: Find camera_azimuth_range Attributes via Reflection Table ---
  // Core camera attributes (search only, setters will be added later)
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

  // --- 1.1 Azimuth Range (NESTED) ---
  // We fetch range-specific offset attributes for managing sub-azimuth settings.

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

  // --- Step 3: Find Runtime State (Live Yaw/Pitch) ---
  // These variables are updated dynamically at runtime and do not exist in the reflection table.
  // We locate them by scanning the code of the camera orientation update function globally.

  uintptr_t addr = PatternFinder::Find(LIVE_YAW_PITCH_SIG);
  if (addr) {
    logger->Debug("3. Live Yaw/Pitch signature found at 0x{:X}", addr);

    // The first MOVSS instruction is at addr + 4 (MULSS is 4 bytes).
    // The 32-bit displacement displacement for Yaw offset starts at offset +4 of the instruction.
    int32_t liveYaw = PatternFinder::ReadInt32(addr + 4 + 4);

    // To locate Live Pitch, we scan forward starting after the first MOVSS instruction.
    // We search dynamically for the MOVSS instruction pattern "F3 0F 11 [80-8F]" to handle variable jump sizes.
    uintptr_t searchStart = addr + 12;
    uintptr_t movss2Addr = PatternFinder::Find(searchStart, 48, "F3 0F 11 [80-8F]");
    if (movss2Addr) {
      // The 32-bit displacement for Pitch offset starts at offset +4 of the second MOVSS instruction.
      int32_t livePitch = PatternFinder::ReadInt32(movss2Addr + 4);

      if (PatternFinder::IsSaneOffset(liveYaw) && PatternFinder::IsSaneOffset(livePitch)) {
        owner.SetInteriorYawOffset(liveYaw);
        owner.SetInteriorPitchOffset(livePitch);
        logger->Debug("3.1 [RUNTIME] LiveYaw: 0x{:X}, LivePitch: 0x{:X}", liveYaw, livePitch);
      } else {
        logger->Error("3.1 [RUNTIME] Insane offsets found: LiveYaw 0x{:X}, LivePitch 0x{:X}", liveYaw, livePitch);
        all_found = false;
      }
    } else {
      logger->Error("3.1 [RUNTIME] Failed to find the second MOVSS (Live Pitch) instruction.");
      all_found = false;
    }
  } else {
    logger->Error("3. FAILED to find Live Yaw/Pitch instruction chain.");
    all_found = false;
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && (owner.GetInteriorSeatXOffset() != 0 && owner.GetInteriorSeatYOffset() != 0 && owner.GetInteriorSeatZOffset() != 0 && owner.GetInteriorLimitLeftOffset() != 0 && owner.GetInteriorLimitRightOffset() != 0 &&
                            owner.GetInteriorLimitUpOffset() != 0 && owner.GetInteriorLimitDownOffset() != 0 && owner.GetInteriorOutsideOffset() != 0 && owner.GetInteriorMouseLRDefaultOffset() != 0 && owner.GetInteriorMouseUDDefaultOffset() != 0 &&
                            owner.GetInteriorYawOffset() != 0 && owner.GetInteriorPitchOffset() != 0 && owner.GetInteriorAzimuthOverridesOffset() != 0 && owner.GetAzimuthRangeOutsideOffset() != 0 && owner.GetAzimuthRangeStartAzimuthOffset() != 0 &&
                            owner.GetAzimuthRangeEndAzimuthOffset() != 0 && owner.GetAzimuthRangeStartUpLimitOffset() != 0 && owner.GetAzimuthRangeEndUpLimitOffset() != 0 && owner.GetAzimuthRangeStartDownLimitOffset() != 0 &&
                            owner.GetAzimuthRangeEndDownLimitOffset() != 0 && owner.GetAzimuthRangeStartUpDownDefaultOffset() != 0 && owner.GetAzimuthRangeEndUpDownDefaultOffset() != 0 && owner.GetAzimuthRangeStartLeftRightDefaultOffset() != 0 &&
                            owner.GetAzimuthRangeEndLeftRightDefaultOffset() != 0 && owner.GetAzimuthRangeStartHeadOffsetOffset() != 0 && owner.GetAzimuthRangeEndHeadOffsetOffset() != 0 && owner.GetZoomFovFactorOffset() != 0 &&
                            owner.GetZoomSpeedOffset() != 0 && owner.GetCameraFovOffset() != 0 && owner.GetNearPlaneOffset() != 0 && owner.GetFarPlaneOffset() != 0 && owner.GetMouseSensitivityOffset() != 0 && owner.GetShakeAnimStepOffset() != 0 &&
                            owner.GetShakeAnimScaleMinOffset() != 0 && owner.GetShakeAnimScaleMaxOffset() != 0 && owner.GetHandShakeLimitOffset() != 0 && owner.GetHandShakeSpeedOffset() != 0 && owner.GetShakeAnimOffset() != 0);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- INTERIOR CAMERA OFFSETS FOUND. InteriorCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize one or more Interior Camera offsets. ({} ms)", duration);
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
