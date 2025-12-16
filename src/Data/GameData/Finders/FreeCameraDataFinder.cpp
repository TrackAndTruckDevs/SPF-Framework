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
   * HOW-TO-FIND:
   * This pointer is the base address for the freecam context. In the function that initializes
   * the freecam, it's loaded from a global variable into a register (e.g., RDI) at the beginning.
   * We are creating a signature for that specific load instruction.
   * EXPECTED INSTRUCTIONS (from user-provided disassembly):
   *   48 8d 45 a8     - LEA RAX, [RBP-58h]
   *   48 8b 3d ...    - MOV RDI, qword ptr [rip+...]  <- This is our target
   *   4c 8d 85 ...    - LEA R8, [RBP+...]
   */
  const char* pFreecamGlobalObjectPtr_SIG = "48 8D 45 A8 48 8B 3D ? ? ? ? 4C 8D 85";
  uintptr_t mov_rdi_addr = Utils::PatternFinder::Find(pFreecamGlobalObjectPtr_SIG);
  if (mov_rdi_addr) {
    // The MOV instruction is at offset 4 of the signature
    uintptr_t mov_instr = mov_rdi_addr + 4;
    uintptr_t rip = mov_instr + 7;
    int32_t offset = *(int32_t*)(mov_instr + 3);
    uintptr_t* pFreecamGlobalObjectPtr = (uintptr_t*)(rip + offset);
    owner.SetFreecamGlobalObjectPtr(pFreecamGlobalObjectPtr);
    logger->Info("B-1: Found 'pFreecamGlobalObjectPtr' dynamically at address: {:#x}", (uintptr_t)pFreecamGlobalObjectPtr);
  } else {
    logger->Warn("B-1: Could not find signature for 'pFreecamGlobalObjectPtr'. Will retry...");
    all_found = false;
  }

  // 2. Find Freecam Context Offset (freecamContextOffset)
  /*
   * HOW-TO-FIND:
   * This offset is used to get the actual freecam context structure from the global object.
   * We found a unique place where this offset is used right before a call to a `CopyCameraState` function.
   * This signature targets the CALL instruction before our MOV, the MOV itself, and the MOV after it.
   * EXPECTED INSTRUCTIONS (from user-provided disassembly):
   *   E8 ...          - CALL ...
   *   48 8B 97 ...    - MOV RDX, qword ptr [RDI+2E60h] <- Our target
   *   49 8B 8D ...    - MOV RCX, qword ptr [R13+C8h]
   */
  const char* freecamContextOffset_SIG = "E8 ? ? ? ? 48 8B 97 ? ? ? ? 49 8B 8D";
  uintptr_t mov_rdx_addr = Utils::PatternFinder::Find(freecamContextOffset_SIG);
  if (mov_rdx_addr) {
    // The MOV instruction is at offset 5 of the signature
    uintptr_t mov_instr = mov_rdx_addr + 5;
    int32_t offset = *(int32_t*)(mov_instr + 3);
    owner.SetFreecamContextOffset(offset);
    logger->Info("B-2: Found 'freecamContextOffset' dynamically: {:#x}", owner.GetFreecamContextOffset());
  } else {
    logger->Warn("B-2: Could not find signature for 'freecamContextOffset'. Will retry...");
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
      // Find CVar Object address from the LEA instruction
      uintptr_t rip = lea_addr + 7;
      int32_t relative_offset = *(int32_t*)(lea_addr + 3);
      pCameraObject = rip + relative_offset;
      logger->Info("A-1/2: Found pCameraObject via global LEA signature at: {:#x}", pCameraObject);

      // Find and call the getter function to warm up the cache
      uintptr_t call_instr_addr = lea_addr + 7;
      uintptr_t call_rip = call_instr_addr + 5;
      int32_t call_offset = *(int32_t*)(call_instr_addr + 1);
      uintptr_t pfnGetter = call_rip + call_offset;
      
      logger->Info("Found CVar getter function at: {:#x}", pfnGetter);

      using CVarGetter = float (*)(uintptr_t);
      auto getter = (CVarGetter)pfnGetter;
      float real_value = getter(pCameraObject);
      logger->Info("Warmed up camera speed CVar cache. Initial value is now: {}", real_value);

    } else {
      logger->Warn("A-1/2: FAILED to find global LEA signature for camera speed CVar. Will retry...");
      all_found = false;
    }

    // Step 3: Find the offset of the cached value (`0x118`)
    if (pCameraObject != 0) {
      uintptr_t movss_addr = Utils::PatternFinder::Find(CVAR_CACHED_VALUE_OFFSET_SIG);
      if (movss_addr) {
        // The signature is for `C6 83 16 01 00 00 01 F3 0F 11 83`.
        // The MOVSS instruction starts at byte 7. The offset is at byte 11.
        speed_offset = *(int32_t*)(movss_addr + 11);
        logger->Info("A-3: Found CVar cached value offset dynamically: {:#x}", speed_offset);
      } else {
        logger->Warn("A-3: FAILED to find signature for CVar cached value offset. Will retry...");
        all_found = false;
      }
    }

    // Step 4: Calculate the final pointer
    if (pCameraObject != 0 && speed_offset != 0) {
      float* pFreeCamSpeed = (float*)(pCameraObject + speed_offset);
      owner.SetFreeCamSpeedPtr(pFreeCamSpeed);
      logger->Info("A-4: Successfully calculated pFreeCamSpeed pointer: {:#x}", (uintptr_t)pFreeCamSpeed);
    } else {
      all_found = false;
    }
  }
  // 4. Find Freecam Position Offsets
  uintptr_t pfnFreecamMove = Utils::PatternFinder::Find(FREECAM_MOVE_SIG);
  if (pfnFreecamMove) {
    const unsigned char pattern[] = {0x41, 0x0F, 0x10, 0x5A};  // MOVUPS XMM?, [R10 + offset]
    bool found = false;
    for (int i = 0; i < 200; ++i) {
      if (memcmp((void*)(pfnFreecamMove + i), pattern, sizeof(pattern)) == 0) {
        intptr_t pos_x_offset = *(int8_t*)(pfnFreecamMove + i + sizeof(pattern));
        owner.SetFreecamPosXOffset(pos_x_offset);
        owner.SetFreecamPosYOffset(pos_x_offset + 4);
        owner.SetFreecamPosZOffset(pos_x_offset + 8);
        logger->Info("Found Freecam position offsets: x={:#x}, y={:#x}, z={:#x}", owner.GetFreecamPosXOffset(), owner.GetFreecamPosYOffset(), owner.GetFreecamPosZOffset());

        // The quaternion data is located immediately after the position data and a 4-byte padding.
        // Position (3 floats) + Padding (1 float) = 16 bytes.
        intptr_t quat_x_offset = pos_x_offset + 16;
        owner.SetFreecamQuatXOffset(quat_x_offset);
        owner.SetFreecamQuatYOffset(quat_x_offset + 4);
        owner.SetFreecamQuatZOffset(quat_x_offset + 8);
        owner.SetFreecamQuatWOffset(quat_x_offset + 12);
        logger->Info("Found Freecam quaternion offsets: x={:#x}, y={:#x}, z={:#x}, w={:#x}",
                     owner.GetFreecamQuatXOffset(),
                     owner.GetFreecamQuatYOffset(),
                     owner.GetFreecamQuatZOffset(),
                     owner.GetFreecamQuatWOffset());

        // The mystery float is the 4th float in the position block.
        owner.SetFreecamMysteryFloatOffset(pos_x_offset + 12);
        logger->Info("Found Freecam mystery float offset: {:#x}", owner.GetFreecamMysteryFloatOffset());

        found = true;
        break;
      }
    }
    if (!found) {
      logger->Warn("FAILED to find position offsets inside Freecam_Move function. Will retry...");
      all_found = false;
    }
  } else {
    logger->Warn("FAILED to find signature for Freecam_Move function. Will retry...");
    all_found = false;
  }

  // 5. Find Mouse and Roll Offsets
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnDebugCamera_HandleInput = cameraHooks.GetDebugCameraHandleInputFunc();
  if (pfnDebugCamera_HandleInput) {
    bool found_all_orientation = false;
    const unsigned char pattern_yaw[] = {0x48, 0x8D, 0x79};  // LEA RDI, [RCX + offset]
    for (int i = 0; i < 500; ++i) {
      if (memcmp((void*)(pfnDebugCamera_HandleInput + i), pattern_yaw, sizeof(pattern_yaw)) == 0) {
        intptr_t yaw_offset = *(int8_t*)(pfnDebugCamera_HandleInput + i + sizeof(pattern_yaw));
        owner.SetFreecamMouseXOffset(yaw_offset);
        owner.SetFreecamMouseYOffset(yaw_offset + 4);
        owner.SetFreecamRollOffset(yaw_offset + 8);
        logger->Info("Found Orientation offsets: yaw={:#x}, pitch={:#x}, roll={:#x}", yaw_offset, yaw_offset + 4, yaw_offset + 8);
        found_all_orientation = true;
        break;
      }
    }
    if (!found_all_orientation) {
      logger->Warn("FAILED to find orientation offsets inside DebugCamera_HandleInput. Will retry...");
      all_found = false;
    }
  } else {
    logger->Warn("Cannot find mouse and roll offsets: DebugCamera_HandleInput function pointer is not ready. Will retry...");
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
