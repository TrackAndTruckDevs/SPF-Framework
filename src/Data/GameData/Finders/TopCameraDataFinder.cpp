#include "SPF/Data/GameData/Finders/TopCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
// This finder now uses two separate anchor functions to find all required offsets.

// --- ANCHOR 1: TopCamera_UpdateMovement ---
/*
 * Signature for the TopCamera_UpdateMovement function.
 * This function is called every frame to apply movement/speed updates for the top-down camera.
 * It's used as an anchor to find the `minimum_height` offset.
 *
 * HOW-TO-FIND:
 * 1. In a disassembler, find the value for Top Camera Minimum Height (e.g., via Cheat Engine, offset 0x478).
 * 2. Find what instruction accesses this address. This should lead you to an LEA instruction.
 * 3. Go to the beginning of the function that contains this LEA instruction.
 * 4. The signature is for the function prologue. The address in the MOVAPS instruction is
 *    wildcarded to make the signature more robust against future game updates.
 *
 * EXPECTED ASSEMBLY:
 *   48 89 5C 24 08        - MOV QWORD PTR [RSP+0x8],RBX
 *   57                    - PUSH RDI
 *   48 83 EC 60           - SUB RSP,0x60
 *   0F 28 05 ?? ?? ?? ??  - MOVAPS XMM0,QWORD PTR [rip+...]
 *   48 8B D9              - MOV RBX,RCX
 */
const char* TOP_CAMERA_UPDATE_MOVEMENT_SIG = "48 89 5C 24 08 57 48 83 EC 60 0F 28 05 ? ? ? ? 48 8B D9";


// --- ANCHOR 2: InitializeTopCamera ---
/*
 * Signature for the InitializeTopCamera function.
 * This function is used to initialize the top-down camera state and contains the offsets
 * for speed, max height, and forward/backward offsets.
 *
 * HOW-TO-FIND:
 * 1. In a disassembler, find where the `maximum_height` value (0x47c) is accessed.
 * 2. Go to the beginning of the function containing that access.
 * 3. This signature is for the function prologue and is very distinct.
 *
 * EXPECTED ASSEMBLY:
 *   48 8B C4              - MOV RAX,RSP
 *   48 89 58 08           - MOV QWORD PTR [RAX+0x8],RBX
 *   48 89 78 10           - MOV QWORD PTR [RAX+0x10],RDI
 *   55                    - PUSH RBP
 *   48 8D 68 A1           - LEA RBP,[RAX-0x5F]
 *   48 81 EC E0 00 00 00  - SUB RSP,0xE0
 */
const char* INITIALIZE_TOP_CAMERA_SIG = "48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC E0 00 00 00";

}  // namespace

bool TopCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Top Camera offsets (Rework)...");

  bool speed_found = false;
  bool min_height_found = false;
  bool max_height_found = false;
  bool x_fwd_found = false;
  bool x_bwd_found = false;

  // --- STAGE 1: Find Minimum Height offset via TopCamera_UpdateMovement ---
  uintptr_t pfnUpdateMovement = Utils::PatternFinder::Find(TOP_CAMERA_UPDATE_MOVEMENT_SIG);
  if (!pfnUpdateMovement) {
    logger->Error("CRITICAL: Could not find anchor function 'TopCamera_UpdateMovement'. Cannot find minimum height offset.");
  } else {
    logger->Info("Found 'TopCamera_UpdateMovement' function at: {:#x}", pfnUpdateMovement);
    /*
     * Signature for the instruction sequence that loads the Top Camera Minimum Height.
     * Correct offset for this property is 0x478.
     *
     * EXPECTED ASSEMBLY:
     *   F3 0F 10 43 24        - MOVSS XMM0,DWORD PTR [RBX+0x24]
     *   48 8D 83 ?? ?? ?? ??  - LEA RAX,[RBX+0x478]  <- The offset we want is here
     *   0F 2F 00              - COMISS XMM0,DWORD PTR [RAX]
     */
    const char* MIN_HEIGHT_OFFSET_SIG = "F3 0F 10 43 24 48 8D 83 ? ? ? ? 0F 2F 00";
    uintptr_t min_height_sig_addr = Utils::PatternFinder::Find(pfnUpdateMovement, 1024, MIN_HEIGHT_OFFSET_SIG);
    if (min_height_sig_addr) {
      int32_t min_height_offset = *(int32_t*)(min_height_sig_addr + 8);
      owner.SetTopMinHeightOffset(min_height_offset);
      logger->Info("-> SUCCESS: Found Top Camera Minimum Height offset: {:#x}", min_height_offset);
      min_height_found = true;
    } else {
      logger->Warn("-> FAILED: Could not find signature for Top Camera Minimum Height offset.");
    }
  }

  // --- STAGE 2: Find other offsets via InitializeTopCamera ---
  uintptr_t pfnInitialize = Utils::PatternFinder::Find(INITIALIZE_TOP_CAMERA_SIG);
  if (!pfnInitialize) {
    logger->Error("CRITICAL: Could not find anchor function 'InitializeTopCamera'. Cannot find remaining offsets.");
  } else {
    logger->Info("Found 'InitializeTopCamera' function at: {:#x}", pfnInitialize);

    // --- OFFSET: X-Offset Forward ---
    // Correct offset for this property is 0x470.
    /*
     * EXPECTED ASSEMBLY:
     *   41 0F 2E C3           - UCOMISS XMM0,XMM11
     *   75 0E                 - JNZ ...
     *   F3 0F 10 8B ?? ?? ?? ?? - MOVSS XMM1,DWORD PTR [RBX+0x470]
     */
    const char* X_FWD_OFFSET_SIG = "41 0F 2E C3 75 0E F3 0F 10 8B ? ? ? ?";
    uintptr_t x_fwd_sig_addr = Utils::PatternFinder::Find(pfnInitialize, 2048, X_FWD_OFFSET_SIG);
    if (x_fwd_sig_addr) {
      int32_t x_fwd_offset = *(int32_t*)(x_fwd_sig_addr + 10);
      owner.SetTopXOffsetForwardOffset(x_fwd_offset);
      logger->Info("-> SUCCESS: Found Top Camera X-Offset Forward: {:#x}", x_fwd_offset);
      x_fwd_found = true;
    } else {
      logger->Warn("-> FAILED: Could not find signature for Top Camera X-Offset Forward.");
    }
    
    // --- OFFSET: X-Offset Backward ---
    // Correct offset for this property is 0x474.
    /*
     * EXPECTED ASSEMBLY:
     *   75 0E                 - JNZ ...
     *   F3 0F 10 83 ?? ?? ?? ?? - MOVSS XMM0,DWORD PTR [RBX+0x474]
     *   41 0F 2E C3           - UCOMISS XMM0,XMM11
     */
    const char* X_BWD_OFFSET_SIG = "75 0E F3 0F 10 83 ? ? ? ? 41 0F 2E C3";
    uintptr_t x_bwd_sig_addr = Utils::PatternFinder::Find(pfnInitialize, 2048, X_BWD_OFFSET_SIG);
    if (x_bwd_sig_addr) {
      int32_t x_bwd_offset = *(int32_t*)(x_bwd_sig_addr + 6);
      owner.SetTopXOffsetBackwardOffset(x_bwd_offset);
      logger->Info("-> SUCCESS: Found Top Camera X-Offset Backward: {:#x}", x_bwd_offset);
      x_bwd_found = true;
    } else {
      logger->Warn("-> FAILED: Could not find signature for Top Camera X-Offset Backward.");
    }

    // --- OFFSET: Maximum Height ---
    // Correct offset for this property is 0x47C.
    /*
     * EXPECTED ASSEMBLY:
     *   F3 0F 10 83 ?? ?? ?? ?? - MOVSS XMM0,DWORD PTR [RBX+0x47c]
     *   48 8D 55 97           - LEA RDX,[RBP-0x69]
     */
    const char* MAX_HEIGHT_OFFSET_SIG = "F3 0F 10 83 ? ? ? ? 48 8D 55 97";
    uintptr_t max_height_sig_addr = Utils::PatternFinder::Find(pfnInitialize, 2048, MAX_HEIGHT_OFFSET_SIG);
    if (max_height_sig_addr) {
      int32_t max_height_offset = *(int32_t*)(max_height_sig_addr + 4);
      owner.SetTopMaxHeightOffset(max_height_offset);
      logger->Info("-> SUCCESS: Found Top Camera Maximum Height offset: {:#x}", max_height_offset);
      max_height_found = true;
    } else {
      logger->Warn("-> FAILED: Could not find signature for Top Camera Maximum Height offset.");
    }
    
    // --- OFFSET: Speed ---
    // Correct offset for this property is 0x480.
    /*
     * EXPECTED ASSEMBLY:
     *   F3 0F 10 83 94 04 00 00  - MOVSS XMM0,[RBX+0x494]
     *   F3 44 0F 59 93 ?? ?? ?? ??  - MULSS XMM10,DWORD PTR [RBX+0x480]
     *   84 C0                    - TEST AL,AL
     */
    const char* SPEED_OFFSET_SIG = "F3 0F 10 83 94 04 00 00 F3 44 0F 59 93 ? ? ? ? 84 C0";
    uintptr_t speed_sig_addr = Utils::PatternFinder::Find(pfnInitialize, 2048, SPEED_OFFSET_SIG);
    if (speed_sig_addr) {
      int32_t speed_offset = *(int32_t*)(speed_sig_addr + 13);
      owner.SetTopSpeedOffset(speed_offset);
      logger->Info("-> SUCCESS: Found Top Camera Speed offset: {:#x}", speed_offset);
      speed_found = true;
    } else {
      logger->Warn("-> FAILED: Could not find signature for Top Camera Speed offset.");
    }
  }


  // --- Finalization and Logging ---
  bool all_found = speed_found && min_height_found && max_height_found && x_fwd_found && x_bwd_found;

  if (all_found) {
    logger->Info("Successfully found all 5 Top Camera offsets.");
    m_isReady = true;
  } else {
    logger->Warn("One or more Top Camera offsets could not be found. Finder will retry.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
