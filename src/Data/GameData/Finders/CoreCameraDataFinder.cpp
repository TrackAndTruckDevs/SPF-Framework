#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

namespace {
/*
 * Anchor #1: Standard Manager Pointer
 * Inside InitializeCamera: MOV RBX, qword ptr [StandardManagerPtr]; MOV EDI, EDX
 * Signature targets the RIP-relative MOV and the subsequent MOV EDI, EDX.
 */
const char* STANDARD_MANAGER_SIG = "48 8B 1D ?? ?? ?? ?? ?? ?? 48";

/*
 * Anchor #2: Active Camera ID Offset
 * Inside InitializeCamera: CMP dword ptr [RBX + offset], imm8; MOV dword ptr [RBX + offset+4], EDX
 * Signature targets the structure of the check while masking volatile values.
 */
const char* ACTIVE_CAMERA_ID_SIG = "83 7B ?? ?? 89";

/*
 * Anchor #3: World Coordinates Pointer
 * Global search for the MOVSD instruction that writes camera world coordinates.
 */
const char* WORLD_COORDINATES_SIG = "F2 0F ?? ?? ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 83";
}  // namespace

bool CoreCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Core Camera offsets (Dynamic Pattern Search)...");

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnInitializeCamera = reinterpret_cast<uintptr_t>(cameraHooks.GetInitializeCameraFunc());

  if (!pfnInitializeCamera) {
    logger->Error("CRITICAL: InitializeCamera function pointer is NULL. Core offsets cannot be found.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 512;

  // --- 1. Find StandardManagerPtrAddr ---
  uintptr_t addrManager = Utils::PatternFinder::Find(pfnInitializeCamera, SEARCH_RANGE, STANDARD_MANAGER_SIG);
  if (addrManager) {
    uintptr_t pStandardManagerPtrAddr = Utils::PatternFinder::GetRipAddress(addrManager, 3, 7);
    if (pStandardManagerPtrAddr) {
      owner.SetStandardManagerPtrAddr(pStandardManagerPtrAddr);
      logger->Debug("Anchor #1: StandardManagerPtrAddr = 0x{:X}", pStandardManagerPtrAddr);

      // --- 1.1 Dynamic Pointer Adjustment Detection (v1.59+ support) ---
      /*
       * In newer game versions (starting from 1.59), the global pointer does not point
       * to the start of the StandardManager object. Instead, the game manually adjusts
       * the pointer after loading it.
       * 
       * We look for instructions like:
       * ADD RBX, imm8 (48 83 C3 XX)
       * SUB RBX, imm8 (48 83 EB XX)
       * 
       * We scan a small window after the initial load to find this correction logic.
       */
      intptr_t adjustment = 0;
      constexpr size_t ADJUSTMENT_SCAN_RANGE = 64;
      
      // Search for ADD RBX, imm8 (48 83 C3)
      uintptr_t addrAdd = Utils::PatternFinder::Find(addrManager, ADJUSTMENT_SCAN_RANGE, "48 83 C3");
      if (addrAdd) {
        int8_t imm8 = Utils::PatternFinder::ReadInt8(addrAdd + 3);
        adjustment = static_cast<intptr_t>(imm8);
        logger->Info("Detected StandardManager pointer adjustment: {} (via ADD RBX)", adjustment);
      } else {
        // Search for SUB RBX, imm8 (48 83 EB)
        uintptr_t addrSub = Utils::PatternFinder::Find(addrManager, ADJUSTMENT_SCAN_RANGE, "48 83 EB");
        if (addrSub) {
          int8_t imm8 = Utils::PatternFinder::ReadInt8(addrSub + 3);
          adjustment = -static_cast<intptr_t>(imm8);
          logger->Info("Detected StandardManager pointer adjustment: {} (via SUB RBX)", adjustment);
        }
      }

      if (adjustment != 0) {
        owner.SetStandardManagerAdjustment(adjustment);
      } else {
        logger->Debug("No StandardManager pointer adjustment detected (likely pre-1.59 version).");
      }

    } else {
      logger->Error("Anchor #1: FAILED to resolve RIP address for StandardManagerPtrAddr");
      all_found = false;
    }
  } else {
    logger->Error("Anchor #1: FAILED to find StandardManager signature in InitializeCamera");
    all_found = false;
  }

  // --- 2. Find ActiveCameraIdOffset ---
  uintptr_t addrId = Utils::PatternFinder::Find(pfnInitializeCamera, SEARCH_RANGE, ACTIVE_CAMERA_ID_SIG);
  if (addrId) {
    // Offset is at byte 2 of the instruction: 83 7B [OFFSET]
    int8_t offset = Utils::PatternFinder::ReadInt8(addrId + 2);
    if (Utils::PatternFinder::IsSaneOffset(static_cast<int32_t>(offset))) {
      owner.SetActiveCameraIdOffset(static_cast<intptr_t>(offset));
      logger->Debug("Anchor #2: ActiveCameraIdOffset = 0x{:X}", (uint8_t)offset);
    } else {
      logger->Error("Anchor #2: ActiveCameraIdOffset INVALID (0x{:X})", (uint8_t)offset);
      all_found = false;
    }
  } else {
    logger->Error("Anchor #2: FAILED to find ActiveCameraId signature in InitializeCamera");
    all_found = false;
  }

  // --- 3. Find World Coordinates Pointer ---
  uintptr_t addrWorld = Utils::PatternFinder::Find(WORLD_COORDINATES_SIG);
  if (addrWorld) {
    // Instruction: F2 0F 11 05 [RIP_OFFSET] (MOVSD [RIP+...], XMM0)
    // Opcode is F2 0F 11 05 (4 bytes), then 4 bytes of RIP displacement. Total 8 bytes.
    uintptr_t pWorldCoords = Utils::PatternFinder::GetRipAddress(addrWorld, 4, 8);
    if (pWorldCoords) {
      owner.SetCameraWorldCoordinatesPtr(reinterpret_cast<uintptr_t*>(pWorldCoords));
      logger->Debug("Anchor #3: WorldCoordinatesPtr = 0x{:X}", pWorldCoords);
    } else {
      logger->Error("Anchor #3: FAILED to resolve RIP address for WorldCoordinatesPtr");
      all_found = false;
    }
  } else {
    logger->Error("Anchor #3: FAILED to find WorldCoordinates signature globally");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    owner.SetCoreOffsetsFound(true);
    logger->Debug("Successfully found all Core Camera offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more Core Camera offsets. Plugin stability is compromised.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
