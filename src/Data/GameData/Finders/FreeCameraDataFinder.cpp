#include "SPF/Data/GameData/Finders/FreeCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>
#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/**
 * @brief Signature for the dynamic CVar value offset.
 * Search range: Inside the GetAndCache function body.
 * 
 * Logic (FOV-style cache):
 * 1. Set the 'is_updated' flag (MOV byte ptr [reg + offset - 2], 1).
 * 2. [GAP] Potential alignment.
 * 3. Write the resolved float value (MOVSS dword ptr [reg + offset], XMM0).
 * 
 * Ghidra Reference (v1.50+ @ 1401d7521):
 * 1401d7521 c6 83 16 01 00 00 01  MOV byte ptr [RBX + 0x116], 0x1
 * 1401d7528 f3 0f 11 83 18 01 00 00 MOVSS dword ptr [RBX + 0x118], XMM0
 */
const char* CVAR_VAL_OFFSET_SIG = "C6 [80-87] [0-6?] F3";

/**
 * @brief String anchor used to find the g_flyspeed CVar object via XREFs.
 */
const char* FLY_SPEED_STRING = "Camera speed: %.2f";

/*
 * Anchor #3: Freecam Move Function (Position & Quaternions)
 * We search for the unique sequence inside Freecam_Move that reads local coordinates.
 * Using a flexible GAP [30-60?] to skip intermediate LEA/CALL instructions.
 * The filter [01-FF] ensures we find the version with an offset (Freecam), 
 * filtering out the generic one (ID 0 offset).
 * 
 * Ghidra Reference (Freecam_Move @ 140771140):
 * 140771140 48 83 ec 48                SUB RSP, 0x48
 * 140771144 f3 0f 10 0a                MOVSS XMM1, [RDX]
 * ... [GAP] ...
 * 14077117a 41 0f 10 5a 40             MOVUPS XMM3, [R10 + 0x40] <-- Our Target
 */
const char* FREECAM_MOVE_SIG = "48 83 EC 48 F3 0F 10 [08-0F] 4C 8B [D0-D7] [30-60?] 41 0F 10 [58-5F] [01-FF]";
}  // namespace

bool FreeCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto startTime = std::chrono::steady_clock::now();
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("--- [FreeCamera] Starting Stable Discovery ---");
  bool all_found = true;

  // --- STEP 1: Find Fly Speed (g_flyspeed CVar) ---
  /*
   * NEW LOGIC: String Anchor -> XREF -> Backward Scan.
   * This is much more robust than global LEA scans.
   * Logic:
   * 1. Find the unique log string "Camera speed: %.2f".
   * 2. Find the instruction that loads this string (XREF).
   * 3. From that XREF, scan BACKWARDS to find the CVar object and the caching function.
   */
  uintptr_t strAddr = Utils::PatternFinder::FindString(FLY_SPEED_STRING);
  if (strAddr) {
    logger->Debug("[FreeCamera] Found Fly Speed string at 0x{:X}", strAddr);
    auto xrefs = Utils::PatternFinder::FindXrefs(strAddr);
    if (!xrefs.empty()) {
      uintptr_t xrefAddr = xrefs[0];
      logger->Debug("[FreeCamera] Found string XREF at 0x{:X}", xrefAddr);
      
      // 1.1 Find the CVar object (LEA backward from string xref)
      // Ghidra: 14053ea86 48 8d 0d 53 38 6b 02    LEA RCX,[PTR_PTR_142bf22e0]
      uintptr_t objectLea = Utils::PatternFinder::FindBackward(xrefAddr - 1, 100, "48 8D [0D-3D]");
      
      // 1.2 Find the CVar cache function (CALL backward from string xref)
      // Ghidra: 14053ea8d e8 ee 89 c9 ff          CALL GetAndCache_DefaultFOV
      uintptr_t callAddr = Utils::PatternFinder::FindBackward(xrefAddr - 1, 100, "E8");

      if (objectLea && callAddr) {
        logger->Debug("[FreeCamera] Anchors found: LEA=0x{:X}, CALL=0x{:X}", objectLea, callAddr);
        uintptr_t pCVarObjPtrAddr = Utils::PatternFinder::GetRipAddress(objectLea, 3, 7);
        uintptr_t pfnGetAndCache = Utils::PatternFinder::GetRipAddress(callAddr, 1, 5);

        if (pCVarObjPtrAddr && pfnGetAndCache) {
          logger->Debug("[FreeCamera] Resolved internal: CVarPtrAddr=0x{:X}, CacheFunc=0x{:X}", pCVarObjPtrAddr, pfnGetAndCache);

          // 1.3 Resolve the internal value offset from the GetAndCache function body
          // We search for the MOVSS instruction that writes the float value.
          uintptr_t valWriteAddr = Utils::PatternFinder::Find(pfnGetAndCache, 300, CVAR_VAL_OFFSET_SIG);
          if (valWriteAddr) {
            logger->Debug("[FreeCamera] Found offset write instruction at 0x{:X}", valWriteAddr);
            
            // Find the F3 byte (MOVSS start) and read its displacement at +4 using ReadInt32
            uintptr_t addrF3 = Utils::PatternFinder::Find(valWriteAddr, 15, "F3");
            int32_t valOffset = Utils::PatternFinder::ReadInt32(addrF3 + 4);
            
            // The address resolved from LEA is already the object base. 
            // We do NOT dereference it.
            uintptr_t pCVarObj = pCVarObjPtrAddr;
            if (pCVarObj && Utils::PatternFinder::IsSaneOffset(valOffset)) {
              float* pFlySpeed = reinterpret_cast<float*>(pCVarObj + valOffset);
              owner.SetFlySpeedPtr(pFlySpeed);
              logger->Debug("[FreeCamera] Fly Speed resolved successfully at 0x{:X} (Offset: 0x{:X})", (uintptr_t)pFlySpeed, valOffset);
            } else {
              logger->Error("[FreeCamera] FAILED: Invalid CVar object (0x{:X}) or offset (0x{:X})", pCVarObj, valOffset);
              all_found = false;
            }
          } else {
            logger->Error("[FreeCamera] FAILED to resolve CVar value offset from function body at 0x{:X}", pfnGetAndCache);
            all_found = false;
          }
        } else {
          logger->Error("[FreeCamera] FAILED to resolve RIP addresses for LEA/CALL.");
          all_found = false;
        }
      } else {
        if (!objectLea) logger->Error("[FreeCamera] FAILED to find LEA anchor backward from 0x{:X}", xrefAddr);
        if (!callAddr) logger->Error("[FreeCamera] FAILED to find CALL anchor backward from 0x{:X}", xrefAddr);
        all_found = false;
      }
    } else {
      logger->Error("[FreeCamera] FAILED to find XREFs for Fly Speed string at 0x{:X}", strAddr);
      all_found = false;
    }
  } else {
    logger->Error("[FreeCamera] FAILED to find 'Camera speed' anchor string.");
    all_found = false;
  }

  // --- STEP 1.5: Freecam Global Object & Context Offset ---
  /*
   * NEW LOGIC: World Context Discovery via UpdateGameSession.
   * This retrieves the primary pointer and offset needed for stable activation.
   * 
   * Logic: 
   * 1. Find the unique string "[used_vehicles] %Iu used truck offers have expired (%Iu offers valid)".
   * 2. Trace back to the start of UpdateGameSession() (Ghidra: 1408523d0).
   * 3. Extract GlobalPtr and Context Offset from the prologue instructions.
   * 
   * Ghidra Reference (v1.60 @ 1408523d0):
   * 1408523dc 48 8b 3d ad 28 d0 02    MOV RDI, qword ptr [FreecamGlobalObjectPtr]
   * ...
   * 1408523f2 48 83 bf a8 31 00 00 00 CMP qword ptr [RDI + 0x31a8], 0x0
   * 
   * Targets: 
   * - FreecamGlobalObjectPtr: The RIP-relative pointer at 1408523dc.
   * - ContextOffset: The 32-bit displacement (0x31A8) at 1408523f2.
   */
  uintptr_t funcStart = Utils::PatternFinder::FindFunctionByString("[used_vehicles] %Iu used truck offers have expired (%Iu offers valid)", true);

      if (funcStart) {
        logger->Debug("[FreeCamera] Found UpdateGameSession at 0x{:X}", funcStart);
        
        // Find Global Object MOV (48 8B [2-12?] 0F)
        uintptr_t movAddr = Utils::PatternFinder::Find(funcStart, 100, "48 8B [2-12?] 0F");
        // Find Context Offset CMP (48 [4-8?] 0F)
        uintptr_t cmpAddr = Utils::PatternFinder::Find(movAddr ? movAddr + 3 : funcStart, 100, "48 [4-8?] 0F");

        if (movAddr && cmpAddr) {
          uintptr_t* pGlobalObjPtr = reinterpret_cast<uintptr_t*>(Utils::PatternFinder::GetRipAddress(movAddr, 3, 7));
          int32_t contextOff = Utils::PatternFinder::ReadInt32(cmpAddr + 3);

          if (pGlobalObjPtr && Utils::PatternFinder::IsSaneOffset(contextOff)) {
            owner.SetFreecamGlobalObjectPtr(pGlobalObjPtr);
            owner.SetFreecamContextOffset(contextOff);
            logger->Info("[FreeCamera] STATIC FREECAM POINTER: 0x{:X}", (uintptr_t)pGlobalObjPtr);
            logger->Debug("[FreeCamera] Resolved ContextOffset: 0x{:X}", contextOff);
          } else {
            logger->Error("[FreeCamera] FAILED to resolve Freecam data from UpdateGameSession.");
            all_found = false;
          }
        } else {
          if (!movAddr) logger->Error("[FreeCamera] FAILED to find Global MOV anchor.");
          if (!cmpAddr) logger->Error("[FreeCamera] FAILED to find Context CMP anchor.");
          all_found = false;
        }
  } else {
    logger->Warn("[FreeCamera] FAILED to find 'mandatory_break_soon' anchor string.");
    all_found = false;
  }

  // --- STEP 2: Position & Quaternion Offsets ---
  uintptr_t addrMove = Utils::PatternFinder::Find(FREECAM_MOVE_SIG);
  if (addrMove) {
    /*
     * Our signature matches the whole block. The offset byte is at the very end.
     * Logic: Pattern length is roughly 16-20 bytes (depending on GAP). 
     * Pattern ends with 41 0F 10 [58-5F] [OFFSET].
     * The Find result points to the START (48 83 EC 48).
     * We use a sub-search to find the MOVUPS instruction exactly.
     */
    uintptr_t movAddr = Utils::PatternFinder::Find(addrMove, 150, "41 0F 10 [58-5F] [01-FF] 41");
    if (movAddr) {
      int8_t posX = Utils::PatternFinder::ReadInt8(movAddr + 4);
      if (Utils::PatternFinder::IsSaneOffset(posX)) {
        owner.SetFreecamPosXOffset(posX);
        owner.SetFreecamPosYOffset(posX + 4);
        owner.SetFreecamPosZOffset(posX + 8);
        owner.SetFreecamMysteryFloatOffset(posX + 12);
        
        int32_t quatX = posX + 16;
        owner.SetFreecamQuatXOffset(quatX);
        owner.SetFreecamQuatYOffset(quatX + 4);
        owner.SetFreecamQuatZOffset(quatX + 8);
        owner.SetFreecamQuatWOffset(quatX + 12);

        logger->Debug("[FreeCamera] Position Offsets resolved: X=0x{:X}, QuatX=0x{:X}", posX, quatX);
      } else {
        logger->Error("[FreeCamera] Position offset 0x{:X} is INVALID.", posX);
        all_found = false;
      }
    } else {
      logger->Error("[FreeCamera] FAILED to find MOVUPS anchor inside Move function.");
      all_found = false;
    }
  } else {
    logger->Error("[FreeCamera] FAILED to find Freecam_Move function.");
    all_found = false;
  }

  // --- STEP 3: Orientation Offsets (Yaw/Pitch/Roll) ---
  /*
   * NEW LOGIC: Search near the start of DebugCamera_HandleInput for RCX-based loads.
   * Ghidra Reference (DebugCamera_HandleInput @ 14053e760):
   * 14053e7aa 48 8d 79 10          LEA RDI, [RCX + 0x10]           <-- YAW Offset
   * ...
   * 14053e7cb f3 44 0f 10 41 14    MOVSS XMM8, dword ptr [RCX+0x14] <-- PITCH Offset
   * 
   * Patterns focus on RCX as the base register to avoid catching stack setups (RAX/RBP).
   */
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnHandleInput = cameraHooks.GetDebugCameraHandleInputFunc();
  if (pfnHandleInput) {
    // 3.1 Find Yaw Anchor: LEA [reg], [RCX + offset] -> 48 8D [78-7B]
    uintptr_t addrYaw = Utils::PatternFinder::Find(pfnHandleInput, 150, "48 8D [78-7B]");
    
    // 3.2 Find Pitch Anchor: MOVSS [xmm], [RCX + offset] -> F3 44 0F 10 [40-43]
    uintptr_t addrPitch = Utils::PatternFinder::Find(pfnHandleInput, 150, "F3 44 0F 10 [40-43]");

    if (addrYaw && addrPitch) {
      int8_t yawOff = Utils::PatternFinder::ReadInt8(addrYaw + 3);
      int8_t pitchOff = Utils::PatternFinder::ReadInt8(addrPitch + 5);

      if (Utils::PatternFinder::IsSaneOffset(yawOff) && Utils::PatternFinder::IsSaneOffset(pitchOff)) {
        owner.SetFreecamMouseXOffset(yawOff);
        owner.SetFreecamMouseYOffset(pitchOff);
        owner.SetFreecamRollOffset(pitchOff + 4); // Roll (Tilt) consistently follows Pitch
        
        logger->Debug("[FreeCamera] Orientation resolved: Yaw=0x{:X}, Pitch=0x{:X}, Roll=0x{:X}", 
                     yawOff, pitchOff, (uint8_t)(pitchOff + 4));
      } else {
        logger->Error("[FreeCamera] Resolved Orientation offsets are INVALID (Yaw: 0x{:X}, Pitch: 0x{:X})", yawOff, pitchOff);
        all_found = false;
      }
    } else {
      if (!addrYaw) logger->Error("[FreeCamera] FAILED to find Yaw anchor (LEA RCX).");
      if (!addrPitch) logger->Error("[FreeCamera] FAILED to find Pitch anchor (MOVSS RCX).");
      all_found = false;
    }
  } else {
    logger->Error("[FreeCamera] CRITICAL: HandleInput function pointer is missing. Skipping orientation discovery.");
    all_found = false;
  }

  m_isReady = all_found;
  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

  if (all_found) {
    logger->Info("[FreeCamera] Discovery completed successfully. ({} ms)", duration);
  } else {
    logger->Error("[FreeCamera] Discovery FAILED. ({} ms)", duration);
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END