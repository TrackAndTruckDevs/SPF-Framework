#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * ApplyBehindCameraConfiguration (Ghidra: FUN_140dff890)
 * Signature for the function that applies saved configuration values to the camera.
 * This function is responsible for loading values from the game's configuration (Azimuth, Elevation, etc.)
 */
const char* APPLY_CONFIG_SIG = "48 8B C4 55 48 83 EC 50 48 83 3D ? ? ? ? ? 48 8B E9";

/*
 * UpdateCameraInput (Ghidra: FUN_140999050)
 * Signature for the function that handles camera input (zoom, rotation).
 */
const char* UPDATE_INPUT_SIG = "40 53 48 83 EC ? 48 8B 01 48 8B D9 0F 29 7C 24 30 F3 0F 10 79 ? 44";

/*
 * ActivateBehindCamera (Ghidra: FUN_1409994a0)
 * Signature for the function that initializes the camera state upon activation.
 */
const char* ACTIVATE_CAMERA_SIG = "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC ? 0F B6 F2 48 8B D9";

/*
 * FindBestFocusPoint (Ghidra: FUN_140947940)
 * Signature for the function that finds a focus point for the camera, used to find laziness speed.
 */
const char* FIND_FOCUS_POINT_SIG = "48 8B C4 48 89 58 08 57 48 81 EC ? ? ? ? F2 41 0F ? ? 48 8B FA F3 0F 10 99";
}  // namespace

bool BehindCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Behind Camera offsets (Strict Dynamic Search)...");

  uintptr_t pfnApplyConfig = Utils::PatternFinder::Find(APPLY_CONFIG_SIG);
  uintptr_t pfnUpdateInput = Utils::PatternFinder::Find(UPDATE_INPUT_SIG);
  uintptr_t pfnActivateCamera = Utils::PatternFinder::Find(ACTIVATE_CAMERA_SIG);
  uintptr_t pfnFindFocusPoint = Utils::PatternFinder::Find(FIND_FOCUS_POINT_SIG);

  if (!pfnApplyConfig || !pfnUpdateInput || !pfnActivateCamera || !pfnFindFocusPoint) {
    logger->Warn("CRITICAL: FAILED to find one or more anchor functions. Will retry...");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 2048;

  // --- 1. Find Config Offsets from ApplyBehindCameraConfiguration (FUN_140dff890) ---
  {
    /*
     * Anchor #1: Azimuth Laziness & Elevation (Min/Max/Default)
     * Sequence: LEA RCX, [PTR...]; CALL GetAndCacheValue; LEA RCX, [PTR...]; MOV [RBP+48C], EAX; MOV [RBP+490], EAX
     *
     * Expected offsets for v1.58: 48C, 490, 494, 498
     */
    const char* p_anchor1 = "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
    uintptr_t addr1 = Utils::PatternFinder::Find(pfnApplyConfig, SEARCH_RANGE, p_anchor1);
    if (addr1) {
      int32_t distChangeSpd   = Utils::PatternFinder::ReadInt32(addr1 + 21); // 48C
      int32_t distLazinessSpd = Utils::PatternFinder::ReadInt32(addr1 + 27); // 490
      int32_t azimLazinessSpd = Utils::PatternFinder::ReadInt32(addr1 + 45); // 494
      int32_t elevMin         = Utils::PatternFinder::ReadInt32(addr1 + 51); // 498

      if (Utils::PatternFinder::IsSaneOffset(distChangeSpd)) {
        owner.SetBehindDistanceChangeSpeedOffset(distChangeSpd);
        logger->Debug("Anchor #1: distChangeSpd=0x{:X}", distChangeSpd);
      } else { logger->Error("Anchor #1: distChangeSpd INVALID (0x{:X})", distChangeSpd); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(distLazinessSpd)) {
        owner.SetBehindDistanceLazinessSpeedOffset(distLazinessSpd);
        logger->Debug("Anchor #1: distLazinessSpd=0x{:X}", distLazinessSpd);
      } else { logger->Error("Anchor #1: distLazinessSpd INVALID (0x{:X})", distLazinessSpd); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(azimLazinessSpd)) {
        owner.SetBehindAzimuthLazinessSpeedOffset(azimLazinessSpd);
        logger->Debug("Anchor #1: azimLazinessSpd=0x{:X}", azimLazinessSpd);
      } else { logger->Error("Anchor #1: azimLazinessSpd INVALID (0x{:X})", azimLazinessSpd); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(elevMin)) {
        owner.SetBehindElevationMinOffset(elevMin);
        logger->Debug("Anchor #1: elevMin=0x{:X}", elevMin);
      } else { logger->Error("Anchor #1: elevMin INVALID (0x{:X})", elevMin); all_found = false; }

      /*
       * Anchor #2: Trailer Default & Height Limit
       * Continues from Anchor #1
       *
       * Expected offsets for v1.58: 49C, 4A0
       */
      const char* p_anchor2 = "E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
      uintptr_t addr2 = Utils::PatternFinder::Find(addr1 + 55, 256, p_anchor2);
      if (addr2) {
        int32_t elevMax = Utils::PatternFinder::ReadInt32(addr2 + 14); // 49C
        int32_t elevDef = Utils::PatternFinder::ReadInt32(addr2 + 20); // 4A0

        if (Utils::PatternFinder::IsSaneOffset(elevMax)) {
          owner.SetBehindElevationMaxOffset(elevMax);
          logger->Debug("Anchor #2: elevMax=0x{:X}", elevMax);
        } else { logger->Error("Anchor #2: elevMax INVALID (0x{:X})", elevMax); all_found = false; }

        if (Utils::PatternFinder::IsSaneOffset(elevDef)) {
          owner.SetBehindElevationDefaultOffset(elevDef);
          logger->Debug("Anchor #2: elevDef=0x{:X}", elevDef);
        } else { logger->Error("Anchor #2: elevDef INVALID (0x{:X})", elevDef); all_found = false; }

        /*
         * Anchor #2 Ext: Elevation Trailer Default (4A4) & Height Limit (4A8)
         */
        const char* p_anchor2_ext = "E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 85 C0 48 8D";
        uintptr_t addr2_ext = Utils::PatternFinder::Find(addr2 + 5, 512, p_anchor2_ext);
        if (addr2_ext) {
          int32_t eTrDef = Utils::PatternFinder::ReadInt32(addr2_ext + 14); // 4A4
          int32_t hLim   = Utils::PatternFinder::ReadInt32(addr2_ext + 20); // 4A8
          if (Utils::PatternFinder::IsSaneOffset(eTrDef)) {
            owner.SetBehindElevationTrailerDefaultOffset(eTrDef);
            logger->Debug("Anchor #2 ElevTrDef=0x{:X}", eTrDef);
          } else { logger->Error("Anchor #2 ElevTrDef INVALID (0x{:X})", eTrDef); all_found = false; }

          if (Utils::PatternFinder::IsSaneOffset(hLim)) {
            owner.SetBehindHeightLimitOffset(hLim);
            logger->Debug("Anchor #2 HeightLim=0x{:X}", hLim);
          } else { logger->Error("Anchor #2 HeightLim INVALID (0x{:X})", hLim); all_found = false; }
        } else {
          logger->Error("FAILED to find Anchor #2 Ext (4A4, 4A8)");
          all_found = false;
        }
      } else {
        logger->Error("FAILED to find ApplyConfig Anchor #2");
        all_found = false;
      }

      /*
       * Anchor #3: Dynamic Offsets (Max, Speed Min/Max, Laziness)
       * Distinct by CMOVA instruction
       *
       * Expected offsets for v1.58: 4B0, 4B4, 4B8, 4BC
       */
      const char* p_anchor3 = "0F 47 C1 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 89 85 ?? ?? ?? ?? 89 85 ?? ?? ?? ??";
      uintptr_t addr3 = Utils::PatternFinder::Find(pfnApplyConfig, SEARCH_RANGE, p_anchor3);
      if (addr3) {
        int32_t dynMax    = Utils::PatternFinder::ReadInt32(addr3 + 36); // 4B8
        int32_t dynSpdMin = Utils::PatternFinder::ReadInt32(addr3 + 42); // 4BC
        
        // Find continuation for 4C0 and 4C4
        const char* p_dyn_ext = "F3 0F 11 85 ?? ?? ?? ?? F3 0F 11 85 ?? ?? ?? ?? E8 ?? ?? ?? ?? 4c 8b ?? ?? ?? ?? ?? 0f";
        uintptr_t addr3_ext = Utils::PatternFinder::Find(addr3 + 45, 256, p_dyn_ext);

        if (Utils::PatternFinder::IsSaneOffset(dynMax)) {
          owner.SetBehindDynamicOffsetMaxOffset(dynMax);
          logger->Debug("Anchor #3: dynMax=0x{:X}", dynMax);
        } else { logger->Error("Anchor #3: dynMax INVALID (0x{:X})", dynMax); all_found = false; }

        if (Utils::PatternFinder::IsSaneOffset(dynSpdMin)) {
          owner.SetBehindDynamicOffsetSpeedMinOffset(dynSpdMin);
          logger->Debug("Anchor #3: dynSpdMin=0x{:X}", dynSpdMin);
        } else { logger->Error("Anchor #3: dynSpdMin INVALID (0x{:X})", dynSpdMin); all_found = false; }

        if (addr3_ext) {
          int32_t dSpdMax = Utils::PatternFinder::ReadInt32(addr3_ext + 4);  // 4C0
          int32_t dLaz    = Utils::PatternFinder::ReadInt32(addr3_ext + 12); // 4C4
          
          if (Utils::PatternFinder::IsSaneOffset(dSpdMax)) {
            owner.SetBehindDynamicOffsetSpeedMaxOffset(dSpdMax);
            logger->Debug("Anchor #3 dynSpdMax=0x{:X}", dSpdMax);
          } else { logger->Error("Anchor #3 dynSpdMax INVALID (0x{:X})", dSpdMax); all_found = false; }

          if (Utils::PatternFinder::IsSaneOffset(dLaz)) {
            owner.SetBehindDynamicOffsetLazinessSpeedOffset(dLaz);
            logger->Debug("Anchor #3 dynLaziness=0x{:X}", dLaz);
          } else { logger->Error("Anchor #3 dynLaz INVALID (0x{:X})", dLaz); all_found = false; }
        } else {
          logger->Error("FAILED to find Anchor #3 Ext (4C0, 4C4)");
          all_found = false;
        }
      } else {
        logger->Error("FAILED to find ApplyConfig Anchor #3");
        all_found = false;
      }

      /*
       * Anchor #9: Pivot X, Y, Z
       * Expected offsets: 4AC, 4B0, 4B4
       */
      const char* p_pivot_sig = "f3 41 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? f3 45 0f 11 ?? ?? ?? ?? ?? 4c";
      uintptr_t addr9 = Utils::PatternFinder::Find(p_pivot_sig);
      if (addr9) {
        int32_t px = Utils::PatternFinder::ReadInt32(addr9 + 14); // 4AC
        int32_t py = Utils::PatternFinder::ReadInt32(addr9 + 23); // 4B0
        int32_t pz = Utils::PatternFinder::ReadInt32(addr9 + 32); // 4B4

        if (Utils::PatternFinder::IsSaneOffset(px)) {
          owner.SetBehindPivotXOffset(px);
          logger->Debug("Anchor #9: PivotX=0x{:X}", px);
        } else { logger->Error("Anchor #9: PivotX INVALID (0x{:X})", px); all_found = false; }

        if (Utils::PatternFinder::IsSaneOffset(py)) {
          owner.SetBehindPivotYOffset(py);
          logger->Debug("Anchor #9: PivotY=0x{:X}", py);
        } else { logger->Error("Anchor #9: PivotY INVALID (0x{:X})", py); all_found = false; }

        if (Utils::PatternFinder::IsSaneOffset(pz)) {
          owner.SetBehindPivotZOffset(pz);
          logger->Debug("Anchor #9: PivotZ=0x{:X}", pz);
        } else { logger->Error("Anchor #9: PivotZ INVALID (0x{:X})", pz); all_found = false; }
      } else {
        logger->Error("FAILED to find Anchor #9 (Pivots)");
        all_found = false;
      }

    } else {
      logger->Error("FAILED to find ApplyConfig Anchor #1");
      all_found = false;
    }
  }

  // --- 2. Find Distance Offsets from ActivateBehindCamera (FUN_1409994a0) ---
  {
    /*
     * Anchor #4: Distance Offsets (Min, Max, Default, Trailer variants)
     * Expected offsets for v1.58: 478, 47C, 480, 484, 488
     */
    const char* p_anchor4 = "F3 0F 10 83 ?? ?? ?? ?? 40 B7 01 F3 0F 58 83 ?? ?? ?? ?? F3 0F 10 8B ?? ?? ?? ?? EB 13 F3 0F 10 8B ?? ?? ?? ?? 40 32 FF F3 0F 10 83 ?? ?? ?? ?? F3 0F 10 93 ?? ?? ?? ??";
    uintptr_t addr4 = Utils::PatternFinder::Find(pfnActivateCamera, SEARCH_RANGE, p_anchor4);
    if (addr4) {
      int32_t off480 = Utils::PatternFinder::ReadInt32(addr4 + 4);
      int32_t off47C = Utils::PatternFinder::ReadInt32(addr4 + 15);
      int32_t off488 = Utils::PatternFinder::ReadInt32(addr4 + 23);
      int32_t off484 = Utils::PatternFinder::ReadInt32(addr4 + 33);
      int32_t off478 = Utils::PatternFinder::ReadInt32(addr4 + 52);

      if (Utils::PatternFinder::IsSaneOffset(off478)) {
        owner.SetBehindDistanceMinOffset(off478);
        logger->Debug("Anchor #4: DistMin=0x{:X}", off478);
      } else { logger->Error("Anchor #4: DistMin INVALID (0x{:X})", off478); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off47C)) {
        owner.SetBehindDistanceMaxOffset(off47C);
        logger->Debug("Anchor #4: DistMax=0x{:X}", off47C);
      } else { logger->Error("Anchor #4: DistMax INVALID (0x{:X})", off47C); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off480)) {
        owner.SetBehindDistanceTrailerMaxOffset(off480);
        logger->Debug("Anchor #4: TrMaxOffset=0x{:X}", off480);
      } else { logger->Error("Anchor #4: TrMax INVALID (0x{:X})", off480); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off484)) {
        owner.SetBehindDistanceDefaultOffset(off484);
        logger->Debug("Anchor #4: DistDefault=0x{:X}", off484);
      } else { logger->Error("Anchor #4: DistDef INVALID (0x{:X})", off484); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off488)) {
        owner.SetBehindDistanceTrailerDefaultOffset(off488);
        logger->Debug("Anchor #4: TrDefault=0x{:X}", off488);
      } else { logger->Error("Anchor #4: TrDef INVALID (0x{:X})", off488); all_found = false; }
    } else {
      logger->Error("FAILED to find ActivateCamera Anchor #4 (Distances)");
      all_found = false;
    }
  }

// --- 3. Find Input Offsets from UpdateCameraInput (FUN_140999050) ---
  {
    /*
     * Anchor #5: Live Pitch
     * Expected offset for v1.58: 0x14
     */
    const char* p_anchor5 = "48 8B D9 0F 29 7C 24 30 F3 0F 10 79 ?? 44 0F 29 44 24 20";
    uintptr_t addr5 = Utils::PatternFinder::Find(pfnUpdateInput, 128, p_anchor5);
    if (addr5) {
      int8_t livePitch = Utils::PatternFinder::ReadInt8(addr5 + 12);
      owner.SetBehindLivePitchOffset(livePitch);
      logger->Debug("Anchor #5 found: LivePitch=0x{:X}", (uint8_t)livePitch);
    } else {
      logger->Error("FAILED to find Anchor #5 (Live Pitch)");
      all_found = false;
    }

            /*
             * Anchor #6: Live Yaw
             * Sequence: ADDSS XMM0, [RBX + 0x4CC]; MOVSS [RBX + 0x4CC], XMM0
             */
            const char* p_anchor6 = "F3 0F 58 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ?? E8 ?? ?? ?? ?? F3 0F 59 05";
            uintptr_t addr6 = Utils::PatternFinder::Find(pfnUpdateInput, SEARCH_RANGE, p_anchor6);
            if (addr6) {
              int32_t yaw = Utils::PatternFinder::ReadInt32(addr6 + 4);
              if (Utils::PatternFinder::IsSaneOffset(yaw)) {
                owner.SetBehindLiveYawOffset(yaw);
                logger->Debug("Anchor #6: LiveYaw=0x{:X}", yaw);
              } else {
                logger->Error("Anchor #6: LiveYaw INVALID (0x{:X})", yaw);
                all_found = false;
              }
            } else {
              logger->Error("FAILED to find UpdateInput Anchor #6 (Live Yaw)");
              all_found = false;
            }    
            /*
             * Anchor #7: Live Zoom
             * Block with IsInputActionTriggered call. 
             * Expected offset: 0x4D0
             */
            const char* p_anchor7 = "E8 ?? ?? ?? ?? 84 C0 74 18 F3 0F 10 83 ?? ?? ?? ?? F3 0F 5C 83 ?? ?? ?? ?? F3 0F 11 83 ?? ?? ?? ??";
            uintptr_t addr7 = Utils::PatternFinder::Find(pfnUpdateInput, SEARCH_RANGE, p_anchor7);
            if (addr7) {
              int32_t zoom = Utils::PatternFinder::ReadInt32(addr7 + 13);
              
              if (Utils::PatternFinder::IsSaneOffset(zoom)) {
                owner.SetBehindLiveZoomOffset(zoom);
                logger->Debug("Anchor #7: LiveZoom=0x{:X}", zoom);
              } else {
                logger->Error("Anchor #7: LiveZoom INVALID (0x{:X})", zoom);
                all_found = false;
              }
            } else {
              logger->Error("FAILED to find UpdateInput Anchor #7 (Live Zoom)");
              all_found = false;
                }
              }
            
              m_isReady = all_found;
      if (all_found) {
        logger->Info("Successfully found all Behind Camera offsets dynamically.");
      } else {
        logger->Warn("Failed to find some Behind Camera offsets. Will retry...");
      }
    
      return all_found;
    }
    
    }  // namespace Data::GameData::Finders
    SPF_NS_END
    