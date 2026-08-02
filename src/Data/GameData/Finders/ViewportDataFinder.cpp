#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

bool ViewportDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  FinderLog log(GetName());

  uintptr_t anchor = 0;

  // ── Phase 1: Viewport Parameters Access ──
  {
    auto phase = log.MakePhase("Viewport Parameters Access");

    /**
     * Direct Viewport Parameters Access.
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404c0340[1404c0340]) ---/
     * 1404c0474  48 8B 1D 05 48 09 03          MOV RBX,qword ptr [0x143554c80]   <-- Global Pointer
     * 1404c047b  8B C6                         MOV EAX,ESI
     * 1404c047d  F3 0F 10 83 1C 09 00 00       MOVSS XMM0,dword ptr [RBX + 0x91c] <-- Y2 (Offset at +13)
     * 1404c0485  F3 0F 5C 83 20 09 00 00       SUBSS XMM0,dword ptr [RBX + 0x920] <-- Y1 (Offset at +21)
     * 1404c048d  F3 0F 10 8B 18 09 00 00       MOVSS XMM1,dword ptr [RBX + 0x918] <-- X2 (Offset at +29)
     * 1404c0495  F3 0F 5C 8B 14 09 00 00       SUBSS XMM1,dword ptr [RBX + 0x914] <-- X1 (Offset at +37)
     */
    const char* VIEWPORT_PARAMS_ACCESS_SIG = "[MOV r64, [rip+off32]] [MOV r32, r32] [MOVSS xmm, [r64+off32]] F3 0F ? ? ? ? ? ? [MOVSS xmm, [r64+off32]] F3 0F";
    anchor = PatternFinder::Find(VIEWPORT_PARAMS_ACCESS_SIG);
    if (phase.Step(anchor, "Viewport access signature", "RT")) {
      // 1. Resolve Global Pointer address via RIP-relative displacement (3 bytes opcode + 4 bytes displacement)
      uintptr_t pGamePtrAddr = PatternFinder::GetRipAddress(anchor, 3, 7);
      if (phase.Step(pGamePtrAddr, "Game pointer address", "DATA")) {
        // 2. Set camera parameters base (In v1.59.2, RBX points to the object directly)
        uintptr_t pGameObject = *reinterpret_cast<uintptr_t*>(pGamePtrAddr);
        if (phase.Step(pGameObject, "Camera params object", "DATA")) {
          owner.SetCameraParamsObjectPtr(pGameObject);
        }
      }
    }
  }

  // ── Phase 2: Viewport Coordinate Offsets ──
  if (anchor) {
    auto phase = log.MakePhase("Viewport Coordinate Offsets");

    // 3. Extract Coordinate Offsets from instruction bytes (32-bit offsets)
    int32_t off_y2 = PatternFinder::ReadInt32(anchor + 13);
    if (phase.StepOffset(off_y2, "Y2 offset", "OFF")) {
      owner.SetViewportY2Offset(static_cast<intptr_t>(off_y2));
    }

    int32_t off_y1 = PatternFinder::ReadInt32(anchor + 21);
    if (phase.StepOffset(off_y1, "Y1 offset", "OFF")) {
      owner.SetViewportY1Offset(static_cast<intptr_t>(off_y1));
    }

    int32_t off_x2 = PatternFinder::ReadInt32(anchor + 29);
    if (phase.StepOffset(off_x2, "X2 offset", "OFF")) {
      owner.SetViewportX2Offset(static_cast<intptr_t>(off_x2));
    }

    int32_t off_x1 = PatternFinder::ReadInt32(anchor + 37);
    if (phase.StepOffset(off_x1, "X1 offset", "OFF")) {
      owner.SetViewportX1Offset(static_cast<intptr_t>(off_x1));
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetViewportX1Offset() != 0 && owner.GetViewportX2Offset() != 0 && owner.GetViewportY1Offset() != 0 && owner.GetViewportY2Offset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
