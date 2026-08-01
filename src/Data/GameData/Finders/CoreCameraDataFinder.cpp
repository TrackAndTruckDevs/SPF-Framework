#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Windows.hpp"

#include "fmt/format.h"

#include <cstdint>

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
 *    Reads the camera context array pointer directly from StandardManager.
 *    Implementations: `CoreCameraDataFinder` (array offset discovery +
 *    inventory) and `DebugCameraDataFinder` (debug camera context pointer).
 *
 * StandardManager STRUCTURE:
 * +0x10: [uint32] ActiveCameraID
 * +0x38: [pointer] pCameraContextArray (array of camera context pointers;
 *        the camera object sits at context + 0x0)
 *
 * NOTE: Offsets are NOT hardcoded. ActiveCameraID and the camera context
 * array offsets are discovered dynamically via byte-pattern signatures inside
 * `InitializeCamera` / `GetCameraObject` (0x10 / 0x38 are fallbacks for
 * pre-1.60 builds). See ARRAY_SCAN_SIG and ACTIVE_CAMERA_ID_SIG below.
 */

namespace {
/*
 * Anchor #1: Camera Count Offset
 * Inside GetCameraObjectByID: the bounds check CMP R8, qword ptr [RCX + off8].
 * It reads the number of camera slots directly from the manager before the
 * array base/sub adjustment. We mask the ModRM / displacement bytes to stay
 * register-independent.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
 * 1404f0197  4C 3B 41 40                   CMP R8,qword ptr [RCX + 0x40]
 */
const char* CAMERA_COUNT_SIG = "[CMP r64, [r64+off8]]";

/*
 * Anchor #1a: Camera Array Base Offset
 * Inside GetCameraObjectByID: ADD RCX, imm8 followed by CMP against [RCX + off8].
 * We mask the ModRM / displacement bytes to stay register-independent.
 * This is searched FROM the CAMERA_COUNT_SIG match (count logic) onward.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
 * 1404f01a4  48 83 C1 30                   ADD RCX,0x30
 * 1404f01a8  4C 3B 41 10                   CMP R8,qword ptr [RCX + 0x10]
 */
const char* ARRAY_BASE_SIG = "[ADD r64, imm8] [CMP r64, [r64+off8]]";

/*
 * Anchor #1b: Camera Array Sub Offset
 * Inside GetCameraObjectByID: MOV RAX, qword ptr [RCX + off8].
 * The final array offset is baseOff + subOff.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
 * 1404f01ae  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
 */
const char* ARRAY_SUB_SIG = "[MOV r64, [r64+off8]]";

/*
 * Anchor #2: Active Camera ID Offset
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(InitializeCamera[1405c09e0]) ---/
 * 1405c09fe  83 7B 10 0E                   CMP dword ptr [RBX + 0x10],0xe
 * 1405c0a02  89 53 14                      MOV dword ptr [RBX + 0x14],EDX
 */
const char* ACTIVE_CAMERA_ID_SIG = "83 [78-7F] ?? ?? [MOV [r64+off8], r32]";

/*
 * Anchor #3: World Coordinates Pointer (Global)
 * We find the global variables that store the current camera's world coordinates (X, Y, Z).
 * Instead of simple MOV instructions, we use a robust mathematical block (SUBSS/MULSS).
 * We mask the ModRM bytes [00-FF] to stay register-independent (XMM0-XMM15).
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140485220[140485220]) ---/
 * 14048526d  F3 0F 5C 3D BB 3A 0B 03       SUBSS XMM7,dword ptr [0x143538d30]
 * 140485275  F3 0F 59 C6                   MULSS XMM0,XMM6
 * 140485279  F3 0F 5C 0D B3 3A 0B 03       SUBSS XMM1,dword ptr [0x143538d34]
 * 140485281  F3 0F 5C 05 AF 3A 0B 03       SUBSS XMM0,dword ptr [0x143538d38]
 */
const char* WORLD_COORDINATES_SIG =
  "F3 0F 5C [00-FF] ? ? ? ? "  // SUBSS (X)
  "[MULSS xmm, xmm] "              // MULSS
  "F3 0F 5C [00-FF] ? ? ? ? "  // SUBSS (Y)
  "F3 0F 5C [00-FF]";              // SUBSS (Z)
}  // namespace

bool CoreCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  Utils::FinderLog log(GetName());
  log.Info("Searching for Core Camera Data...");

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  uintptr_t pfnInitializeCamera = reinterpret_cast<uintptr_t>(cameraHooks.GetInitializeCameraFunc());
  uintptr_t pfnGetCamObj = reinterpret_cast<uintptr_t>(cameraHooks.GetGetCameraObjectFunc());

  if (!pfnInitializeCamera || !pfnGetCamObj) {
    log.Error("Required engine functions not found. Discovery failed.");
    return log.Finish(false);
  }

  bool all_found = true;

  // ── Phase 1: Camera Count & Array Offset ──
  // Analyze 'GetCameraObjectByID' to find the offsets used for array access.
  // First locate the count bounds check (search length 16), then use it as the
  // anchor to find the array base (ADD) and sub (MOV) offsets.
  {
    auto phase = log.MakePhase("Camera Count & Array Offset");

    uintptr_t addrCount = Utils::PatternFinder::Find(pfnGetCamObj, 16, CAMERA_COUNT_SIG);
    if (phase.Step(addrCount, "Camera count logic", "RT")) {
      int8_t countOff = Utils::PatternFinder::ReadInt8(addrCount + 3);
      if (phase.StepOffset(countOff, "Camera count offset", "OFF")) {
        owner.SetCameraCountOffset(static_cast<intptr_t>(countOff));
      } else {
        all_found = false;
      }

      uintptr_t addrBase = Utils::PatternFinder::Find(addrCount, 32, ARRAY_BASE_SIG);
      if (phase.Step(addrBase, "Array base logic", "RT")) {
        int8_t baseOff = Utils::PatternFinder::ReadInt8(addrBase + 3);
        phase.StepOffset(baseOff, "Base offset", "OFF");

        uintptr_t addrSub = Utils::PatternFinder::Find(addrBase, 32, ARRAY_SUB_SIG);
        if (phase.Step(addrSub, "Array sub logic", "RT")) {
          int8_t subOff = Utils::PatternFinder::ReadInt8(addrSub + 3);
          phase.StepOffset(subOff, "Sub offset", "OFF");

          int32_t finalArrayOffset = static_cast<int32_t>(baseOff) + static_cast<int32_t>(subOff);
          if (phase.StepOffset(finalArrayOffset, "Final array offset", "OFF")) {
            owner.SetCameraArrayOffset(finalArrayOffset);
          } else {
            all_found = false;
          }
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    } else {
      all_found = false;
    }
  }

  // ── Phase 2: Active Camera ID Offset ──
  {
    auto phase = log.MakePhase("Active Camera ID Offset");

    uintptr_t addrId = Utils::PatternFinder::Find(pfnInitializeCamera, 64, ACTIVE_CAMERA_ID_SIG);
    if (phase.Step(addrId, "Active ID logic", "RT")) {
      int8_t offset = Utils::PatternFinder::ReadInt8(addrId + 2);
      if (phase.StepOffset(offset, "ID offset", "OFF")) {
        owner.SetActiveCameraIdOffset(static_cast<intptr_t>(offset));
      } else {
        all_found = false;
      }
    } else {
      all_found = false;
    }
  }

  // ── Phase 3: Initial Array Inventory & Registration ──
  // Best-effort: registers discovered camera addresses early. Does not gate
  // readiness — the array may still be empty before a world is loaded.
  // The array and count offsets come back from Phase 1 via the owner getters.
  {
    auto phase = log.MakePhase("Initial Array Inventory");

    intptr_t arrayOff = owner.GetCameraArrayOffset();
    intptr_t countOff = owner.GetCameraCountOffset();
    uintptr_t managerAddr = owner.GetCameraManager();
    if (phase.StepOptional(managerAddr, "Camera manager", "RT") && arrayOff) {
      // Read the camera slot count from the manager (bounds check at +0x40).
      // Fall back to 15 slots when the offset is unavailable or the value is absurd.
      uint64_t camCount = 15;
      if (countOff) {
        uint64_t rawCount = *reinterpret_cast<uint64_t*>(managerAddr + countOff);
        if (rawCount != 0 && rawCount <= 64) {
          camCount = rawCount;
        }
      }

      uintptr_t pArray = *reinterpret_cast<uintptr_t*>(managerAddr + arrayOff);
      if (pArray && !IsBadReadPtr(reinterpret_cast<void*>(pArray), camCount * 8)) {
        for (uint64_t i = 0; i < camCount; ++i) {
          uintptr_t camAddr = *reinterpret_cast<uintptr_t*>(pArray + (i * 8));
          if (camAddr) {
            owner.RegisterDiscoveredAddress(static_cast<int>(i), camAddr);
            phase.StepOptional(camAddr, fmt::format("Slot {:2}", i), "CAM");
          }
        }
      }
    }
  }

  // ── Phase 4: World Coordinates ──
  // Best-effort global world coordinate block. Not required for core
  // readiness; consumers already null-check GetCameraWorldCoordinatesPtr().
  {
    auto phase = log.MakePhase("World Coordinates");

    /*
     * We extract the address of the global world coordinates block.
     * Logic: Our signature matches a SUBSS instruction (8 bytes total).
     * Format: [F3 0F 5C] [ModRM] [DISP32]
     * Displacement (relative address) starts at byte 4.
     * instructionSize = 8 bytes.
     */
    uintptr_t addrWorld = Utils::PatternFinder::Find(WORLD_COORDINATES_SIG);
    if (phase.Step(addrWorld, "World coordinates pattern found", "RT")) {
      uintptr_t pWorldCoords = Utils::PatternFinder::GetRipAddress(addrWorld, 4, 8);
      if (phase.Step(pWorldCoords, "RIP address resolved", "RT")) {
        owner.SetCameraWorldCoordinatesPtr(reinterpret_cast<uintptr_t*>(pWorldCoords));
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && owner.GetCameraArrayOffset() != 0 && owner.GetActiveCameraIdOffset() != 0 && owner.GetCameraCountOffset() != 0;
  owner.SetCoreOffsetsFound(m_isReady);

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
