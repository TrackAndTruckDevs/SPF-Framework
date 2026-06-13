#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Anchor signature for the UpdateCameraInput function (Behind Camera).
 * Ghidra Address: 140a6f190
 * Logic: Matches the function prologue and the initial state setup.
 * Sequence: PUSH RBX; SUB RSP, 0x40; MOV RAX, [RCX]; MOV RBX, RCX.
 */
const char* UPDATE_INPUT_ANCHOR_SIG = "40 53 48 83 EC [20-80] 48 8B [00-07] 48 8B [D8-DF] 0F 29 [3-6?] F3 0F ? ? ? 44";

/**
 * @brief Logic chain for Live Yaw (Horizontal rotation).
 * Ghidra Address: 140a6f1d5 - 140a6f1da
 * Logic: MULSS (delta from input) -> [skip intermediate] -> ADDSS (add to current Yaw).
 * Pattern: F3 0F 59 [40-7F] ?? F3 0F 58 [80-BF]
 */
const char* YAW_LOGIC_SIG = "F3 0F 59 [40-7F] [1-4?] F3 0F 58 [80-BF]";

/**
 * @brief Logic chain for Live Pitch (Vertical rotation).
 * Ghidra Address: 140a6f24a
 * Logic: Final MOVSS to Pitch offset (8-bit) before the next action trigger check.
 * Pattern: F3 0F 11 [40-7F] [1-4?] B2 01 E8
 */
const char* PITCH_LOGIC_SIG = "F3 0F 11 [40-7F] [1-4?] B2 01 E8";

/**
 * @brief Logic chain for Live Zoom (Distance) with safety verification.
 * Ghidra Address: 140a6f25a - 140a6f262
 * Logic: MOVSS (current Zoom) -> [skip] -> SUBSS (Distance Change Speed).
 * We verify the second offset against 'distance_change_speed' from reflection.
 * Pattern: F3 0F 10 [80-BF] [1-8?] F3 0F 5C [80-BF]
 */
const char* ZOOM_LOGIC_SIG = "F3 0F 10 [80-BF] [1-8?] F3 0F 5C [80-BF]";

}  // namespace

bool BehindCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING BEHIND CAMERA OFFSET SEARCH ---");

  const char* CLASS_NAME_BEHIND = "vehicle_behind_rotation_camera";
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

  // --- Step 1: Find vehicle_behind_rotation_camera SII Attributes via Reflection Table ---

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

  // --- Step 2: Find vehicle_camera SII Attributes via Reflection Table ---
  
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

  // --- Step 3: Find core_camera SII Attributes via Reflection Table ---

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

  // --- Step 4: Find Runtime State (Live Yaw/Pitch/Zoom) ---
  // These are updated dynamically and aren't in reflection tables.
  // We locate them within the UpdateCameraInput function using robust logic chains.

  uintptr_t pfnUpdateInput = PatternFinder::Find(UPDATE_INPUT_ANCHOR_SIG);
  if (pfnUpdateInput) {
    logger->Debug("4.[RUNTIME] UpdateCameraInput anchor found at 0x{:X}", pfnUpdateInput);
    const size_t FUNC_RANGE = 512;

    // 4.1 Live Yaw (Horizontal)
    // Chain: MULSS (input delta) -> [skip] -> ADDSS (update Yaw).
    uintptr_t yawBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, YAW_LOGIC_SIG);
    if (yawBlock) {
      uintptr_t addssAddr = PatternFinder::Find(yawBlock, 32, "F3 0F 58 [80-BF]");
      if (addssAddr) {
        int32_t liveYaw = PatternFinder::ReadInt32(addssAddr + 4);
        if (PatternFinder::IsSaneOffset(liveYaw)) {
          owner.SetBehindLiveYawOffset(liveYaw);
          logger->Debug("4.1 [RUNTIME] LiveYaw offset found: 0x{:X}", liveYaw);
        } else { logger->Error("4.1 [RUNTIME] Insane LiveYaw offset: 0x{:X}", liveYaw); all_found = false; }
      } else { logger->Error("4.1 [RUNTIME] FAILED to extract ADDSS Yaw instruction."); all_found = false; }
    } else { logger->Error("4.1 [RUNTIME] FAILED to find LiveYaw logic chain."); all_found = false; }

    // 4.2 Live Pitch (Vertical)
    // Chain: MOVSS (Pitch writeback) -> [skip] -> MOV DL, 1 (Next logic block anchor).
    uintptr_t pitchBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, PITCH_LOGIC_SIG);
    if (pitchBlock) {
      int8_t livePitch = PatternFinder::ReadInt8(pitchBlock + 4);
      if (livePitch > 0 && livePitch < 0x7F) {
        owner.SetBehindLivePitchOffset(static_cast<intptr_t>(livePitch));
        logger->Debug("4.2 [RUNTIME] LivePitch offset found: 0x{:X}", livePitch);
      } else { logger->Error("4.2 [RUNTIME] Insane LivePitch offset: 0x{:X}", livePitch); all_found = false; }
    } else { logger->Error("4.2 [RUNTIME] FAILED to find LivePitch logic chain."); all_found = false; }

    // 4.3 Live Zoom (Distance)
    // Chain: MOVSS (current Zoom) -> [skip] -> SUBSS (Distance Change Speed).
    // We verify the SUBSS offset against the already found distChgSpd (0x494).
    uintptr_t zoomBlock = PatternFinder::Find(pfnUpdateInput, FUNC_RANGE, ZOOM_LOGIC_SIG);
    if (zoomBlock) {
      int32_t liveZoom = PatternFinder::ReadInt32(zoomBlock + 4);
      uintptr_t subssAddr = PatternFinder::Find(zoomBlock, 32, "F3 0F 5C [80-BF]");
      if (subssAddr) {
        int32_t verifyOff = PatternFinder::ReadInt32(subssAddr + 4);
        if (verifyOff == static_cast<int32_t>(distChgSpd) && PatternFinder::IsSaneOffset(liveZoom)) {
          owner.SetBehindLiveZoomOffset(liveZoom);
          logger->Debug("4.3 [RUNTIME] LiveZoom offset verified: 0x{:X} (via distChgSpd 0x{:X})", liveZoom, verifyOff);
        } else { logger->Error("4.3 [RUNTIME] Zoom verification FAILED. Offset: 0x{:X}, Expected Verification Offset: 0x{:X}", liveZoom, verifyOff); all_found = false; }
      } else { logger->Error("4.3 [RUNTIME] FAILED to extract SUBSS Zoom instruction."); all_found = false; }
    } else { logger->Error("4.3 [RUNTIME] FAILED to find LiveZoom logic chain."); all_found = false; }

  } else {
    logger->Error("4.[RUNTIME] FAILED to find UpdateCameraInput function anchor.");
    all_found = false;
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && (owner.GetBehindDistanceMinOffset() != 0 &&
                           owner.GetBehindDistanceMaxOffset() != 0 &&
                           owner.GetBehindDistanceTrailerMaxOffset() != 0 &&
                           owner.GetBehindDistanceDefaultOffset() != 0 &&
                           owner.GetBehindDistanceTrailerDefaultOffset() != 0 &&
                           owner.GetBehindDistanceChangeSpeedOffset() != 0 &&
                           owner.GetBehindDistanceLazinessSpeedOffset() != 0 &&
                           owner.GetBehindAzimuthLazinessSpeedOffset() != 0 &&
                           owner.GetBehindElevationMinOffset() != 0 &&
                           owner.GetBehindElevationMaxOffset() != 0 &&
                           owner.GetBehindElevationDefaultOffset() != 0 &&
                           owner.GetBehindElevationTrailerDefaultOffset() != 0 &&
                           owner.GetBehindHeightLimitOffset() != 0 &&
                           owner.GetBehindPivotXOffset() != 0 &&
                           owner.GetBehindPivotYOffset() != 0 &&
                           owner.GetBehindPivotZOffset() != 0 &&
                           owner.GetBehindDynamicOffsetMaxOffset() != 0 &&
                           owner.GetBehindDynamicOffsetSpeedMinOffset() != 0 &&
                           owner.GetBehindDynamicOffsetSpeedMaxOffset() != 0 &&
                           owner.GetBehindDynamicOffsetLazinessSpeedOffset() != 0 &&
                           owner.GetBehindLiveYawOffset() != 0 &&
                           owner.GetBehindLivePitchOffset() != 0 &&
                           owner.GetBehindLiveZoomOffset() != 0 &&
                           owner.GetBehindValidationOffset() != 0 &&
                           owner.GetBehindValidationSpeedPositiveOffset() != 0 &&
                           owner.GetBehindValidationSpeedNegativeOffset() != 0 &&
                           owner.GetBehindValidationRadiusOffset() != 0 &&
                           owner.GetBehindSpeedFovChangeFactorOffset() != 0 &&
                           owner.GetCameraFovOffset() != 0 &&
                           owner.GetMouseSensitivityOffset() != 0 &&
                           owner.GetShakeAnimStepOffset() != 0 &&
                           owner.GetShakeAnimScaleMinOffset() != 0 &&
                           owner.GetShakeAnimScaleMaxOffset() != 0 &&
                           owner.GetShakeAnimOffset() != 0);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- BEHIND CAMERA OFFSETS FOUND. BehindCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize one or more Behind Camera offsets. ({} ms)", duration);
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
