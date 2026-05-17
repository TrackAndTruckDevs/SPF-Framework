#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/**
 * ApplyBehindCameraConfiguration (Ghidra: FUN_140dff890)
 * Signature for the function that applies saved configuration values to the camera.
 */
const char* APPLY_CONFIG_SIG = "48 8B C4 55 48 83 EC 50 48 83 3D ? ? ? ? ? 48 8B E9";

/**
 * UpdateCameraInput (Ghidra: FUN_140999050)
 * Signature for the function that handles camera input (zoom, rotation).
 */
const char* UPDATE_INPUT_SIG = "40 53 48 83 EC ? 48 8B 01 48 8B D9 0F 29 7C 24 30 F3 0F 10 79 ? 44";

/**
 * ActivateBehindCamera (Ghidra: FUN_1409994a0)
 * Signature for the function that initializes the camera state upon activation.
 */
const char* ACTIVATE_CAMERA_SIG = "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC ? 0F B6 F2 48 8B D9";

/**
 * FindBestFocusPoint (Ghidra: FUN_140947940)
 * Signature for the function that finds a focus point for the camera.
 */
const char* FIND_FOCUS_POINT_SIG = "48 8B C4 48 89 58 08 57 48 81 EC ? ? ? ? F2 41 0F ? ? 48 8B FA F3 0F 10 99";
}  // namespace

bool BehindCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Starting search for Behind Camera memory structures...");

  uintptr_t pfnApplyConfig = Utils::PatternFinder::Find(APPLY_CONFIG_SIG);
  uintptr_t pfnUpdateInput = Utils::PatternFinder::Find(UPDATE_INPUT_SIG);
  uintptr_t pfnActivateCamera = Utils::PatternFinder::Find(ACTIVATE_CAMERA_SIG);
  uintptr_t pfnFindFocusPoint = Utils::PatternFinder::Find(FIND_FOCUS_POINT_SIG);

  if (!pfnApplyConfig || !pfnUpdateInput || !pfnActivateCamera || !pfnFindFocusPoint) {
    logger->Warn("Critical: One or more anchor functions not found. Postponing search.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 2048;

  // Helper lambda to validate and set offsets safely
  auto validateAndSet = [&](int32_t offset, const char* label, void (GameDataCameraService::*setter)(intptr_t)) {
      if (Utils::PatternFinder::IsSaneOffset(offset)) {
          (owner.*setter)(static_cast<intptr_t>(offset));
          return true;
      }
      logger->Error("INVALID offset found for {}: 0x{:X}", label, offset);
      return false;
  };

  // --- 1. Find Configuration Offsets (from ApplyBehindCameraConfiguration) ---
  {
    /*
     * Anchor #1: Azimuth Laziness & Elevation (Min/Max/Default)
     * Sequence: LEA RCX, [PTR...]; CALL GetAndCacheValue; LEA RCX, [PTR...]; MOV [RBP+48C], EAX; MOV [RBP+490], EAX
     */
    const char* BEHIND_CONFIG_BLOCK_SIG = "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
    uintptr_t addr1 = Utils::PatternFinder::Find(pfnApplyConfig, SEARCH_RANGE, BEHIND_CONFIG_BLOCK_SIG);
    if (addr1) {
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr1 + 21), "distChangeSpd", &GameDataCameraService::SetBehindDistanceChangeSpeedOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr1 + 27), "distLazinessSpd", &GameDataCameraService::SetBehindDistanceLazinessSpeedOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr1 + 45), "azimLazinessSpd", &GameDataCameraService::SetBehindAzimuthLazinessSpeedOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr1 + 51), "elevMin", &GameDataCameraService::SetBehindElevationMinOffset);

      // Anchor #2: Elevation Max & Elevation Default
      const char* BEHIND_ELEV_SIG = "E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
      uintptr_t addr2 = Utils::PatternFinder::Find(addr1 + 55, 256, BEHIND_ELEV_SIG);
      if (addr2) {
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr2 + 14), "elevMax", &GameDataCameraService::SetBehindElevationMaxOffset);
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr2 + 20), "elevDef", &GameDataCameraService::SetBehindElevationDefaultOffset);

        // Anchor #2 Ext: Elevation Trailer Default & Height Limit
        const char* BEHIND_ELEV_EXT_SIG = "E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 48 8D";
        uintptr_t addr2_ext = Utils::PatternFinder::Find(addr2 + 5, 512, BEHIND_ELEV_EXT_SIG);
        if (addr2_ext) {
          all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr2_ext + 14), "elevTrDef", &GameDataCameraService::SetBehindElevationTrailerDefaultOffset);
          all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr2_ext + 20), "heightLim", &GameDataCameraService::SetBehindHeightLimitOffset);
        } else { logger->Error("FAILED to find Anchor #2 Ext (Elevation storage block)"); all_found = false; }
      } else { logger->Error("FAILED to find Anchor #2 (Elevation logic)"); all_found = false; }

      // ANCHOR #3: Dynamic speed/laziness logic
      const char* DYNAMIC_LIMITS_LOGIC_SIG = "0F 47 C1 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
      uintptr_t addr3 = Utils::PatternFinder::Find(pfnApplyConfig, SEARCH_RANGE, DYNAMIC_LIMITS_LOGIC_SIG);
      if (addr3) {
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr3 + 36), "dynMax", &GameDataCameraService::SetBehindDynamicOffsetMaxOffset);
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr3 + 42), "dynSpdMin", &GameDataCameraService::SetBehindDynamicOffsetSpeedMinOffset);

        /**
         * ANCHOR #3 EXT: Dynamic Speed and Laziness Storage
         * 
         * Ghidra Analysis (v1.59.2):
         * 140e3e6a5 [0]: f3 0f 11 85 c0 04 00 00  MOVSS dword ptr [RBP + 0x4c0], XMM0 <-- dSpdMax (+4)
         * 140e3e6ad [8]: f3 0f 11 85 c4 04 00 00  MOVSS dword ptr [RBP + 0x4c4], XMM0 <-- dLaziness (+12)
         * 140e3e6b5 [16]: e8 ?? ?? ?? ??           CALL GetAndCache_DefaultFOV
         * 140e3e6ba [21]: 48 8b 05 ?? ?? ?? ??     MOV RAX, qword ptr [DAT_...] (Changed from 4C 8B in 1.58)
         */
        const char* DYNAMIC_SPEED_STORAGE_SIG = "F3 0F 11 85 ?? ?? ?? ?? F3 0F 11 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ??";
        uintptr_t addr3_ext = Utils::PatternFinder::Find(addr3 + 45, 512, DYNAMIC_SPEED_STORAGE_SIG);
        if (addr3_ext) {
          all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr3_ext + 4), "dynSpdMax", &GameDataCameraService::SetBehindDynamicOffsetSpeedMaxOffset);
          all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr3_ext + 12), "dynLaz", &GameDataCameraService::SetBehindDynamicOffsetLazinessSpeedOffset);
        } else { logger->Error("FAILED to find Anchor #3 Ext (Dynamic storage block)"); all_found = false; }
      } else { logger->Error("FAILED to find Anchor #3 (Dynamic logic)"); all_found = false; }

      // Anchor #9: Pivot Points
      const char* PIVOT_SIG = "f3 41 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? 4c";
      uintptr_t addr9 = Utils::PatternFinder::Find(PIVOT_SIG);
      if (addr9) {
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr9 + 14), "PivotX", &GameDataCameraService::SetBehindPivotXOffset);
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr9 + 23), "PivotY", &GameDataCameraService::SetBehindPivotYOffset);
        all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr9 + 32), "PivotZ", &GameDataCameraService::SetBehindPivotZOffset);
      } else { logger->Error("FAILED to find Anchor #9 (Pivot points)"); all_found = false; }

    } else { logger->Error("FAILED to find Anchor #1 (Config base block)"); all_found = false; }
  }

  // --- 2. Find Distance Offsets (from ActivateBehindCamera) ---
  {
    const char* DIST_OFFSETS_SIG = "F3 0F 10 83 ?? ?? ?? ?? 40 B7 01 F3 0F 58 83 ?? ?? ?? ?? F3 0F 10 8B ?? ?? ?? ?? EB 13 F3 0F 10 8B ?? ?? ?? ?? 40 32 FF F3 0F 10 83 ?? ?? ?? ?? F3 0F 10 93 ?? ?? ?? ??";
    uintptr_t addr4 = Utils::PatternFinder::Find(pfnActivateCamera, SEARCH_RANGE, DIST_OFFSETS_SIG);
    if (addr4) {
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr4 + 4), "distTrailerMax", &GameDataCameraService::SetBehindDistanceTrailerMaxOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr4 + 15), "distMax", &GameDataCameraService::SetBehindDistanceMaxOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr4 + 23), "distTrailerDef", &GameDataCameraService::SetBehindDistanceTrailerDefaultOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr4 + 33), "distDef", &GameDataCameraService::SetBehindDistanceDefaultOffset);
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr4 + 52), "distMin", &GameDataCameraService::SetBehindDistanceMinOffset);
    } else { logger->Error("FAILED to find Anchor #4 (Distances)"); all_found = false; }
  }

  // --- 3. Find Runtime Input Offsets (from UpdateCameraInput) ---
  {
    // Anchor #5: Live Pitch
    const char* LIVE_PITCH_SIG = "48 8B D9 0F 29 7C 24 30 F3 0F 10 79 ?? 44 0F 29 44 24 20";
    uintptr_t addr5 = Utils::PatternFinder::Find(pfnUpdateInput, 128, LIVE_PITCH_SIG);
    if (addr5) {
      int8_t livePitch = Utils::PatternFinder::ReadInt8(addr5 + 12);
      if (livePitch > 0 && livePitch < 0x7F) { // Reasonable check for 8-bit offset
          owner.SetBehindLivePitchOffset(static_cast<intptr_t>(livePitch));
      } else { logger->Error("Anchor #5: livePitch offset 0x{:X} is suspicious", (uint8_t)livePitch); all_found = false; }
    } else { logger->Error("FAILED to find Anchor #5 (Live Pitch)"); all_found = false; }

    // Anchor #6: Live Yaw
    const char* LIVE_YAW_SIG = "F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 59 05";
    uintptr_t addr6 = Utils::PatternFinder::Find(pfnUpdateInput, SEARCH_RANGE, LIVE_YAW_SIG);
    if (addr6) {
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr6 + 4), "liveYaw", &GameDataCameraService::SetBehindLiveYawOffset);
    } else { logger->Error("FAILED to find Anchor #6 (Live Yaw)"); all_found = false; }

    // Anchor #7: Live Zoom
    const char* LIVE_ZOOM_SIG = "E8 ?? ?? ?? ?? 84 C0 74 18 F3 0F 10 83 ?? ?? ?? ?? F3 0F 5C 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ??";
    uintptr_t addr7 = Utils::PatternFinder::Find(pfnUpdateInput, SEARCH_RANGE, LIVE_ZOOM_SIG);
    if (addr7) {
      all_found &= validateAndSet(Utils::PatternFinder::ReadInt32(addr7 + 13), "liveZoom", &GameDataCameraService::SetBehindLiveZoomOffset);
    } else { logger->Error("FAILED to find Anchor #7 (Live Zoom)"); all_found = false; }
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully initialized all Behind Camera offsets.");
  } else {
    logger->Warn("Partial failure during Behind Camera offset search.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
