#include "SPF/Data/GameData/Finders/FreeCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the UpdateCameraFlySpeed function.
 * HOW-TO-FIND:
 * 1. In a disassembler (like IDA) or memory scanner (like Cheat Engine), search for the string "Camera fly speed set to %.2f".
 * 2. Find cross-references (XREFs) to this string. This will lead you to a function.
 * 3. Go up from the string reference until you find the function's prologue (the start).
 * 4. The signature below is for the prologue and the first few unique checks inside it, making it a stable anchor.
 * EXPECTED INSTRUCTIONS:
 * 48 89 5c        MOV        qword ptr [RSP + local_res10],RBX
 * 24 10
 * 48 89 74        MOV        qword ptr [RSP + local_res18],RSI
 * 24 18
 * 57              PUSH       RDI
 * 48 81 ec        SUB        RSP,0xb0
 * b0 00 00 00
 * 80 b9 34        CMP        byte ptr [RCX + 0x434],0x0
 * 04 00 00 00
 * 48 8b da        MOV        RBX,RDX
 * 48 8b 35        MOV        RSI,qword ptr [DAT_143295d90]
 * 4d 68 73 02
 */
// const char* UPDATE_CAMERA_FLY_SPEED_SIG = "48 89 5C 24 10 48 89 74 24 18 57 48 81 EC B0 00 00 00 80 B9 34 04 00 00 00 48 8B DA 48 8B"; we have reworked now we are looking directly for the address, without looking for the function itself

/*
 * Signature for the LEA instruction that loads the pointer to the CVar object for camera speed.
 * This signature is searched for *inside* the UpdateCameraFlySpeed function.
 * HOW-TO-FIND:
 * 1. Go to the UpdateCameraFlySpeed function in a disassembler.
 * 2. Look for a call to a function like GetAndCache... (in our case, GetAndCache_DefaultFOV).
 * 3. This signature targets the LEA instruction right before that call, which is unique in this context.
 * EXPECTED INSTRUCTIONS:
 *   48 8D 0D... (LEA RCX, [rip+...]) <- The instruction we need to parse.
 *   E8...       (CALL GetAndCache_DefaultFOV)
 *   F3 0F 58 C6 (ADDSS XMM0, XMM6)
 */
const char* LEA_PCAMERAOBJ_SIG = "48 8D 0D ? ? ? ? E8 ? ? ? ? F3 0F 58 C6";

/*
 * Signature to find the offset of the cached float value within a CVar object.
 * This is searched for globally as it's inside a generic GetAndCache... function.
 * HOW-TO-FIND:
 * 1. Find the GetAndCache_DefaultFOV function (or any similar one for floats).
 * 2. Look for the part of the code that runs when the cache is INVALID.
 * 3. You will see it set a flag to 1 (cache is now valid) and then write the float value.
 *    This signature targets exactly that sequence, making it very reliable.
 * EXPECTED INSTRUCTIONS:
 *   C6 83 16 01... 01 (MOV byte ptr [rbx+116h], 1)      <- Sets cache valid flag.
 *   F3 0F 11 83...    (MOVSS dword ptr [rbx+0x118], xmm0) <- Writes cached value. This is what we need.
 */
const char* CVAR_CACHED_VALUE_OFFSET_SIG = "C6 83 16 01 00 00 01 F3 0F 11 83";

// Signature for the start of the Freecam_Move function, used to find position offsets.
const char* FREECAM_MOVE_SIG =
    "48 83 EC 48 F3 0F 10 0A 4C 8B D1 48 8D 4C 24 3C 4C 8B CA ? ? ? ? ? F3 41 0F 10 59 04 48 8D 4C 24 3E F3 41 0F 10 49 08 F3 0F 11 5C 24 34 F3 0F 11 44 24 30 "
    "? ? ? ? ? 41 0F 10 5A 40";
}  // namespace

bool FreeCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Free Camera offsets...");
  bool all_found = true;

  // 1. Find Freecam Global Object Pointer (pFreecamGlobalObjectPtr)
  /*
   * Ghidra: FUN_1407a25ac
   * Anchor: LEA RAX, [RBP-58h]; MOV RDI, [RIP+...]
   */
  /**
   * B-1: Find Freecam Global Object Pointer.
   * Logic: First find a unique anchor near the profile selection check, 
   * then find the first MOV instruction that loads the global pointer.
   * 
   * 1.59 Ghidra Example:
   * 1407cfb59: 4c 89 bc 24 b0 01 00 00  MOV qword ptr [RSP + 0x1b0], R15
   * 1407cfb61: 84 c0                    TEST AL, AL
   * 1407cfb63: 74 07                    JZ LAB_1407cfb6c
   * 1407cfb65: 32 db                    XOR BL, BL
   * ...
   * 1407cfb6c: 48 8b 05 fd 12 c1 02     MOV RAX, qword ptr [DAT_1433e0e70]
   */
  const char* FREECAM_ANCHOR_SIG = "4C 89 ?? ?? ?? ?? ?? ?? 84 C0 ?? ?? 32 DB";
  uintptr_t anchor_addr = Utils::PatternFinder::Find(FREECAM_ANCHOR_SIG);
  if (anchor_addr) {
    uintptr_t mov_addr = Utils::PatternFinder::Find(anchor_addr, 100, "48 8B");
    if (mov_addr) {
        uintptr_t* pFreecamGlobalObjectPtr = reinterpret_cast<uintptr_t*>(Utils::PatternFinder::GetRipAddress(mov_addr, 3, 7));
        if (pFreecamGlobalObjectPtr) {
          owner.SetFreecamGlobalObjectPtr(pFreecamGlobalObjectPtr);
          logger->Debug("B-1: Found 'pFreecamGlobalObjectPtr' at: {:#x}", (uintptr_t)pFreecamGlobalObjectPtr);

          // --- 1.1 Dynamic Pointer Adjustment Detection (v1.59+ support) ---
          /*
           * In newer game versions (starting from 1.59), the global pointer does not point
           * to the start of the Freecam system object. The game adjusts it immediately after loading.
           * 
           * 1.59 Ghidra Example:
           * 1407cfb6c: 48 8b 05 ...  MOV RAX, qword ptr [DAT_...]
           * 1407cfb76: 48 8d 70 f0     LEA RSI, [RAX - 0x10]  <-- This is what we need to detect.
           * 
           * FIX (1.59.2): We must be careful not to catch RIP-relative LEA instructions (like 48 8D 05).
           * We specifically look for LEA instructions with a 1-byte displacement (ModRM byte 0x40-0x7F).
           */
          intptr_t adjustment = 0;
          constexpr size_t ADJUSTMENT_SCAN_RANGE = 32;

          // Search for LEA instruction (48 8D) with a 1-byte displacement ModRM byte.
          // We mask the ModRM byte to ensure it's in the [REG + imm8] range (0x40-0x7F).
          uintptr_t addrLea = Utils::PatternFinder::Find(mov_addr, ADJUSTMENT_SCAN_RANGE, "48 8D [40-7F]");
          if (addrLea) {
            // Instruction: 48 8D [ModRM] [OFFSET] -> e.g., 48 8D 70 f0
            // Offset is at byte 3.
            int8_t imm8 = Utils::PatternFinder::ReadInt8(addrLea + 3);
            adjustment = static_cast<intptr_t>(imm8);
            logger->Info("Detected Freecam global object pointer adjustment: {} (via LEA)", adjustment);
          } else {
            logger->Debug("No Freecam pointer adjustment detected (LEA pattern not found).");
          }
          owner.SetFreecamGlobalObjectAdjustment(adjustment);

        } else {
          logger->Error("B-1: FAILED to resolve RIP for pFreecamGlobalObjectPtr");
          all_found = false;
        }
    } else {
      logger->Error("B-1: FAILED to find 48 8B instruction after anchor.");
      all_found = false;
    }
  } else {
    logger->Warn("B-1: Could not find FREECAM_ANCHOR signature.");
    all_found = false;
  }

  // 2. Find Freecam Context Offset (freecamContextOffset)
  /*
   * Ghidra: FUN_1407a26d2
   * Anchor: CALL FUN_14043c850; MOV RDX, [RDI + offset]
   */
  const char* freecamContextOffset_SIG = "E8 ? ? ? ? 48 8B ? ? ? ? ? 49 8B ? ? ? ? ? e8 ? ? ? ? 48 ? ? ? ? ? ? 48 ? ? 74";
  uintptr_t mov_rdx_addr = Utils::PatternFinder::Find(freecamContextOffset_SIG);
  if (mov_rdx_addr) {
    int32_t offset = Utils::PatternFinder::ReadInt32(mov_rdx_addr + 8); // MOV RDX, [RDI + offset]
    if (Utils::PatternFinder::IsSaneOffset(offset)) {
      owner.SetFreecamContextOffset(offset);
      logger->Debug("B-2: Found 'freecamContextOffset': 0x{:X}", offset);
    } else {
      logger->Error("B-2: freecamContextOffset INVALID (0x{:X})", offset);
      all_found = false;
    }
  } else {
    logger->Warn("B-2: Could not find signature for 'freecamContextOffset'.");
    all_found = false;
  }

  // --- Part A: Find pFreeCamSpeed ---
  if (owner.GetFreeCamSpeedPtr() == nullptr) {
    uintptr_t pCameraObject = 0;
    intptr_t speed_offset = 0;

    // Step 1 & 2: Find the CVar object pointer (`pCameraObject`) by globally searching for its usage.
    // Based on user feedback, this signature is strong enough to be found globally and points to the LEA instruction we need.
    uintptr_t lea_addr = Utils::PatternFinder::Find(LEA_PCAMERAOBJ_SIG);
    if (lea_addr) {
      pCameraObject = Utils::PatternFinder::GetRipAddress(lea_addr, 3, 7);
      logger->Debug("A-1/2: Found pCameraObject via LEA: {:#x}", pCameraObject);

      uintptr_t pfnGetter = Utils::PatternFinder::GetRipAddress(lea_addr + 7, 1, 5);
      if (pfnGetter) {
        logger->Debug("Found CVar getter function at: {:#x}", pfnGetter);
        using CVarGetter = float (*)(uintptr_t);
        auto getter = (CVarGetter)pfnGetter;
        float real_value = getter(pCameraObject);
        logger->Info("Warmed up speed CVar cache. Initial value: {}", real_value);
      }
    } else {
      logger->Warn("A-1/2: FAILED to find LEA signature for camera speed CVar.");
      all_found = false;
    }

    if (pCameraObject != 0) {
      uintptr_t movss_addr = Utils::PatternFinder::Find(CVAR_CACHED_VALUE_OFFSET_SIG);
      if (movss_addr) {
        speed_offset = Utils::PatternFinder::ReadInt32(movss_addr + 11);
        if (Utils::PatternFinder::IsSaneOffset(speed_offset)) {
          logger->Debug("A-3: Found CVar cached value offset: 0x{:X}", speed_offset);
        } else {
          logger->Error("A-3: CVar offset INVALID (0x{:X})", speed_offset);
          all_found = false;
        }
      } else {
        logger->Warn("A-3: FAILED to find signature for CVar cached value offset.");
        all_found = false;
      }
    }

    if (pCameraObject != 0 && speed_offset != 0) {
      float* pFreeCamSpeed = (float*)(pCameraObject + speed_offset);
      owner.SetFreeCamSpeedPtr(pFreeCamSpeed);
      logger->Debug("A-4: Successfully calculated pFreeCamSpeed pointer: {:#x}", (uintptr_t)pFreeCamSpeed);
    } else {
      all_found = false;
    }
  }
  // 4. Find Freecam Position Offsets
  uintptr_t pfnFreecamMove = Utils::PatternFinder::Find(FREECAM_MOVE_SIG);
  if (pfnFreecamMove) {
    /*
     * Anchor: MOVUPS XMM3, [R10 + 0x40]
     * Sequence: 41 0F 10 5A 40
     */
    uintptr_t addr = Utils::PatternFinder::Find(pfnFreecamMove, 512, "41 0F 10 5A ??");
    if (addr) {
      int32_t posX = Utils::PatternFinder::ReadInt8(addr + 4);
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

        logger->Debug("Found Freecam position offsets: x=0x{:X}, y=0x{:X}, z=0x{:X}", 
                     owner.GetFreecamPosXOffset(), owner.GetFreecamPosYOffset(), owner.GetFreecamPosZOffset());
        
        logger->Debug("Found Freecam quaternion offsets: x=0x{:X}, y=0x{:X}, z=0x{:X}, w=0x{:X}",
                     owner.GetFreecamQuatXOffset(), owner.GetFreecamQuatYOffset(), 
                     owner.GetFreecamQuatZOffset(), owner.GetFreecamQuatWOffset());

        logger->Debug("Found Freecam mystery float offset: 0x{:X}", owner.GetFreecamMysteryFloatOffset());
      } else {
        logger->Error("Freecam position offset INVALID (0x{:X})", posX);
        all_found = false;
      }
    } else {
      logger->Warn("FAILED to find position anchor inside Freecam_Move.");
      all_found = false;
    }
  } else {
    logger->Warn("FAILED to find Freecam_Move function.");
    all_found = false;
  }

  // 5. Find Mouse and Roll Offsets (Orientation)
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnHandleInput = cameraHooks.GetDebugCameraHandleInputFunc();
  if (pfnHandleInput) {
    // Yaw Anchor: MOV [reg+off], reg; LEA RDI, [RCX + offset]; MOV [reg+off], reg
    uintptr_t addrYaw = Utils::PatternFinder::Find(pfnHandleInput, 512, "48 89 ?? ?? 48 8D");
    // Pitch Anchor: MOVAPS [reg+off], XMM8; MOVSS XMM8, [RCX + offset]; MOVAPS [reg+off], XMM11
    uintptr_t addrPitch = Utils::PatternFinder::Find(pfnHandleInput, 512, "44 0F 29 ?? ?? F3 44 0F 10 41 ?? 44 0F 29 ?? ?? ?? ?? ??");

    if (addrYaw && addrPitch) {
      int32_t yaw = Utils::PatternFinder::ReadInt8(addrYaw + 7);
      int32_t pitch = Utils::PatternFinder::ReadInt8(addrPitch + 10);

      if (Utils::PatternFinder::IsSaneOffset(yaw) && Utils::PatternFinder::IsSaneOffset(pitch)) {
        owner.SetFreecamMouseXOffset(yaw);
        owner.SetFreecamMouseYOffset(pitch);
        owner.SetFreecamRollOffset(pitch + 4);
        logger->Debug("Found Freecam orientation offsets: Yaw=0x{:X}, Pitch=0x{:X}, Roll=0x{:X}", 
                     yaw, pitch, pitch + 4);
      } else {
        logger->Error("Freecam orientation offsets INVALID: Yaw=0x{:X}, Pitch=0x{:X}", yaw, pitch);
        all_found = false;
      }
    } else {
      logger->Warn("FAILED to find orientation anchors inside DebugCamera_HandleInput.");
      all_found = false;
    }
  } else {
    logger->Warn("DebugCamera_HandleInput function pointer is not ready.");
    all_found = false;
  }

  if (all_found) {
    m_isReady = true;
    logger->Info("Successfully found all free camera data.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
