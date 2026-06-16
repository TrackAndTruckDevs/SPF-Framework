#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

/*
 * ARCHITECTURE NOTE: CAMERA ACCESS METHODS
 * 
 * 1. FUNCTION CALL:
 *    Uses the internal game function `GetCameraObject(manager, ID)`.
 *    Implementation: `Hooks::CameraHooks`.
 * 
 * 2. DIRECT ACCESS:
 *    Uses the raw array of camera pointers at `StandardManager + 0x38`.
 *    Implementation: `Data::GameData::Finders::DebugCameraDataFinder`.
 * 
 * StandardManager STRUCTURE:
 * +0x10: [uint32] ActiveCameraID
 * +0x38: [pointer] pCameraArray (Pointer to an array of camera pointers)
 */

namespace {
/*
 * Anchor #1: Camera Manager Pointer
 * Inside InitializeCamera: MOV RBX, qword ptr [DAT_143554ca8]; MOV EDI, EDX; MOV RSI, RCX
 * We mask the registers to stay robust against compiler changes.
 * Ghidra: 1405c09f2 48 8b 1d af 42 f9 02
 */
const char* CAMERA_MANAGER_SIG = "48 8B 1D ?? ?? ?? ?? 8B [C0-FF] 48 8B [C0-FF]";

/*
 * Anchor #2: Active Camera ID Offset
 * Inside InitializeCamera: CMP dword ptr [RBX + 0x10], 0xE
 * Masking the ModRM byte [78-7F] to support different registers (RBX, RSI, RDI, etc.).
 * Ghidra: 1405c09fe 83 7b 10 0e
 */
const char* ACTIVE_CAMERA_ID_SIG = "83 [78-7F] ?? ?? 89";

/*
 * Anchor #3: World Coordinates Pointer (Global)
 * We find the global variables that store the current camera's world coordinates (X, Y, Z).
 * Instead of simple MOV instructions, we use a robust mathematical block (SUBSS/MULSS).
 * We mask the ModRM bytes [00-FF] to stay register-independent (XMM0-XMM15).
 * 
 * Ghidra snippet:
 * 14048526d f3 0f 5c 3d bb 3a 0b 03  SUBSS XMM7, dword ptr [DAT_143538d30]
 * 140485275 f3 0f 59 c6              MULSS XMM0, XMM6
 * 140485279 f3 0f 5c 0d b3 3a 0b 03  SUBSS XMM1, dword ptr [DAT_143538d34]
 * 140485281 f3 0f 5c 05 af 3a 0b 03  SUBSS XMM0, dword ptr [DAT_143538d38]
 */
const char* WORLD_COORDINATES_SIG = 
    "F3 0F 5C [00-FF] ?? ?? ?? ?? " // SUBSS (X)
    "F3 0F 59 [00-FF] "             // MULSS
    "F3 0F 5C [00-FF] ?? ?? ?? ?? " // SUBSS (Y)
    "F3 0F 5C [00-FF]";             // SUBSS (Z)
}  // namespace

bool CoreCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("--- [CameraSystem] Starting Discovery Phase ---");

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnInitializeCamera = reinterpret_cast<uintptr_t>(cameraHooks.GetInitializeCameraFunc());
  uintptr_t pfnGetCamObj = reinterpret_cast<uintptr_t>(cameraHooks.GetGetCameraObjectFunc());

  if (!pfnInitializeCamera || !pfnGetCamObj) {
    logger->Error("[CameraSystem] CRITICAL: Required engine functions not found. Discovery failed.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 512;

  // --- STEP 1: Find Camera Manager ---
  uintptr_t addrManager = Utils::PatternFinder::Find(pfnInitializeCamera, SEARCH_RANGE, CAMERA_MANAGER_SIG);
  if (addrManager) {
    uintptr_t pManagerPtrAddr = Utils::PatternFinder::GetRipAddress(addrManager, 3, 7);
    if (pManagerPtrAddr) {
      owner.SetCameraManagerPtrAddr(pManagerPtrAddr);
      logger->Info("[CameraSystem] STATIC BASE POINTER: 0x{:X}", pManagerPtrAddr);
      
      // --- Dynamic Pointer Adjustment Detection ---
      /*
       * In some engine builds, the global pointer does not point directly to the start 
       * of the Camera Manager object. Instead, the game adjusts the pointer immediately 
       * after loading it into a register.
       * 
       * We scan a small window (64 bytes) following the initial load for:
       * 1. ADD register, imm8 (48 83 C3 XX) -> Pointer moves forward
       * 2. SUB register, imm8 (48 83 EB XX) -> Pointer moves backward
       * 
       * If found, we store this adjustment to ensure all subsequent offsets 
       * (like the camera array at +0x38) are calculated from the true object base.
       */
      intptr_t adjustment = 0;
      constexpr size_t ADJ_RANGE = 64;
      uintptr_t addrAdd = Utils::PatternFinder::Find(addrManager, ADJ_RANGE, "48 83 C3");
      if (addrAdd) adjustment = static_cast<intptr_t>(Utils::PatternFinder::ReadInt8(addrAdd + 3));
      else {
        uintptr_t addrSub = Utils::PatternFinder::Find(addrManager, ADJ_RANGE, "48 83 EB");
        if (addrSub) adjustment = -static_cast<intptr_t>(Utils::PatternFinder::ReadInt8(addrSub + 3));
      }
      owner.SetCameraManagerAdjustment(adjustment);
      if (adjustment != 0) {
        logger->Info("[CameraSystem] Camera Manager found at 0x{:X} (Applied Adjustment: {})", owner.GetCameraManager(), adjustment);
      } else {
        logger->Info("[CameraSystem] Camera Manager found at 0x{:X} (No adjustment detected, using direct address)", owner.GetCameraManager());
      }
    } else {
      logger->Error("[CameraSystem] FAILED to resolve RIP address for Camera Manager");
      all_found = false;
    }
  } else {
    logger->Error("[CameraSystem] FAILED to find Camera Manager signature");
    all_found = false;
  }

  // --- STEP 2: Find Camera Array Offset (Dynamic) ---
  /*
   * We analyze 'GetCameraObjectByID' to find the offsets used for array access.
   * Ghidra Reference:
   * 1404f01a4 48 83 c1 30             ADD RCX, 0x30             <-- baseOff
   * 1404f01ae 48 8b 41 08             MOV RAX, qword ptr [RCX + 0x8] <-- subOff
   */
  const char* ARRAY_SCAN_SIG = "48 83 C1 ?? ?? ?? ?? ?? 73 ?? 48 8B 41 ??";
  uintptr_t addrArrayLogic = Utils::PatternFinder::Find(pfnGetCamObj, 128, ARRAY_SCAN_SIG);
  
  int32_t finalArrayOffset = 0x38; // Standard fallback
  if (addrArrayLogic) {
    int8_t baseOff = Utils::PatternFinder::ReadInt8(addrArrayLogic + 3);
    int8_t subOff = Utils::PatternFinder::ReadInt8(addrArrayLogic + 13);
    finalArrayOffset = static_cast<int32_t>(baseOff) + static_cast<int32_t>(subOff);
    logger->Info("[CameraSystem] Detected Dynamic Array Offset: 0x{:X} (0x{:X} + 0x{:X})", finalArrayOffset, baseOff, subOff);
  } else {
    logger->Warn("[CameraSystem] Logic for Array Offset not found. Using fallback 0x38.");
  }
  owner.SetCameraArrayOffset(finalArrayOffset);

  // --- STEP 3: Find Active Camera ID Offset ---
  uintptr_t addrId = Utils::PatternFinder::Find(pfnInitializeCamera, SEARCH_RANGE, ACTIVE_CAMERA_ID_SIG);
  if (addrId) {
    int8_t offset = Utils::PatternFinder::ReadInt8(addrId + 2);
    owner.SetActiveCameraIdOffset(static_cast<intptr_t>(offset));
    logger->Info("[CameraSystem] Active ID Offset: 0x{:X}", (uint8_t)offset);
  } else {
    logger->Error("[CameraSystem] FAILED to find Active ID signature");
    all_found = false;
  }

  // --- STEP 4: Initial Array Inventory & Registration ---
  uintptr_t managerAddr = owner.GetCameraManager();
  if (managerAddr && finalArrayOffset) {
    uintptr_t pArray = *reinterpret_cast<uintptr_t*>(managerAddr + finalArrayOffset);
    if (pArray && !IsBadReadPtr((void*)pArray, 16 * 8)) {
      logger->Info("[CameraSystem] --- Performing Initial Inventory ---");
      for (int i = 0; i < 15; ++i) {
        uintptr_t camAddr = *reinterpret_cast<uintptr_t*>(pArray + (i * 8));
        if (camAddr) {
          owner.RegisterDiscoveredAddress(i, camAddr);
          logger->Info("[CameraSystem] | Slot [{:2}] | Raw Address: 0x{:X}", i, camAddr);
        }
      }
      logger->Info("[CameraSystem] -------------------------------------");
    }
  }

  // --- STEP 5: World Coordinates ---
  /*
   * We extract the address of the global world coordinates block.
   * Logic: Our signature matches a SUBSS instruction (8 bytes total).
   * Format: [F3 0F 5C] [ModRM] [DISP32]
   * Displacement (relative address) starts at byte 4. 
   * instructionSize = 8 bytes.
   */
  uintptr_t addrWorld = Utils::PatternFinder::Find(WORLD_COORDINATES_SIG);
  if (addrWorld) {
    uintptr_t pWorldCoords = Utils::PatternFinder::GetRipAddress(addrWorld, 4, 8);
    if (pWorldCoords) {
      owner.SetCameraWorldCoordinatesPtr(reinterpret_cast<uintptr_t*>(pWorldCoords));
      logger->Info("[CameraSystem] World Coordinates found at 0x{:X}", pWorldCoords);
    } else {
      logger->Error("[CameraSystem] FAILED to resolve RIP address from World Coordinates signature.");
      all_found = false;
    }
  } else {
    logger->Error("[CameraSystem] FAILED to find World Coordinates block (SUBSS pattern not found).");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    owner.SetCoreOffsetsFound(true);
    logger->Info("[CameraSystem] Core discovery completed successfully. System is READY.");
  } else {
    owner.SetCoreOffsetsFound(false);
    logger->Error("[CameraSystem] Core discovery FAILED. Some critical pointers are missing.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
