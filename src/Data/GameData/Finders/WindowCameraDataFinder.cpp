#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"
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
 * @brief Chained pattern for Live Yaw and Pitch within UpdateInteriorCameraOrientation.
 * Shared by Interior and Window cameras.
 * Target Code Snippet (Verified for Game Version 1.60 at 140876eea):
 * 140876eea f3 0f 59 d3                MULSS      XMM2,XMM3
 * 140876eee f3 0f 11 83 98 05 00 00    MOVSS      dword ptr [RBX + 0x598],XMM0   <-- Live Yaw (Offset +0x08)
 * 140876ef6 0f 2f d1                   COMISS     XMM2,XMM1
 * 140876ef9 72 07                      JC         LAB_140876f02                  <-- Conditional Jump (Offset +0x0F)
 * 140876efb 0f 28 cc                   MOVAPS     XMM1,XMM4
 * 140876efe f3 0f 5d ca                MINSS      XMM1,XMM2
 * 140876f02 48 8d 8b 28 03 00 00       LEA        RCX,[RBX + 0x328]              <-- LEA (Offset +0x1B)
 * 140876f09 f3 0f 11 8b 9c 05 00 00    MOVSS      dword ptr [RBX + 0x59c],XMM1   <-- Live Pitch (Offset +0x29)
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

bool WindowCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  auto start = std::chrono::high_resolution_clock::now();
  logger->Info("--- STARTING WINDOW CAMERA OFFSET SEARCH (Reflection + Pattern) ---");

  const char* CLASS_NAME_WINDOW = "vehicle_interior_camera";
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

  // --- Step 1: Find vehicle_interior_camera SII Attributes ---
  
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

  // --- Step 2: Find core_camera SII Attributes ---

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

  // --- Step 3: Find Runtime State (Live Yaw/Pitch) via Pattern ---
  
  uintptr_t addr = PatternFinder::Find(LIVE_YAW_PITCH_SIG);
  if (addr) {
    logger->Debug("2. Live Yaw/Pitch signature found at 0x{:X}", addr);

    int32_t liveYaw = PatternFinder::ReadInt32(addr + 4 + 4);
    uintptr_t searchStart = addr + 12;
    uintptr_t movss2Addr = PatternFinder::Find(searchStart, 48, "F3 0F 11 [80-8F]");
    
    if (movss2Addr) {
      int32_t livePitch = PatternFinder::ReadInt32(movss2Addr + 4);

      if (PatternFinder::IsSaneOffset(liveYaw) && PatternFinder::IsSaneOffset(livePitch)) {
        owner.SetWindowLiveYawOffset(liveYaw);
        owner.SetWindowLivePitchOffset(livePitch);
        logger->Debug("2.1 [RUNTIME] LiveYaw: 0x{:X}, LivePitch: 0x{:X}", liveYaw, livePitch);
      } else {
        logger->Error("2.1 [RUNTIME] Insane offsets found: LiveYaw 0x{:X}, LivePitch 0x{:X}", liveYaw, livePitch);
        all_found = false;
      }
    } else {
      logger->Error("2.1 [RUNTIME] Failed to find the second MOVSS (Live Pitch) instruction.");
      all_found = false;
    }
  } else {
    logger->Error("2. FAILED to find Live Yaw/Pitch instruction chain.");
    all_found = false;
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && (owner.GetWindowHeadOffsetXOffset() != 0 &&
                           owner.GetWindowMouseLeftLimitOffset() != 0 &&
                           owner.GetWindowLiveYawOffset() != 0 &&
                           owner.GetWindowRelativeHeadtrackingAzimuthOffset() != 0 &&
                           owner.GetWindowAutoCenterMoveDirectionOffset() != 0 &&
                           owner.GetCameraFovOffset() != 0 &&
                           owner.GetMouseSensitivityOffset() != 0 &&
                           owner.GetShakeAnimStepOffset() != 0 &&
                           owner.GetShakeAnimScaleMinOffset() != 0 &&
                           owner.GetShakeAnimScaleMaxOffset() != 0 &&
                           owner.GetShakeAnimOffset() != 0);

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  if (m_isReady) {
    logger->Info("--- WINDOW CAMERA OFFSETS FOUND. WindowCameraDataFinder is ready. ({} ms) ---", duration);
  } else {
    logger->Error("FAILED to initialize one or more Window Camera offsets. ({} ms)", duration);
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
