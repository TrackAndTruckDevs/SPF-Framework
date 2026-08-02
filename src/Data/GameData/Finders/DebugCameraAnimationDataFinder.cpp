#include "SPF/Data/GameData/Finders/DebugCameraAnimationDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

namespace {
/*
 * Anchor #1: Animation Timer Offset
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
 * 140544438  C7 83 F8 0D 00 00 00 00 80 BF MOV dword ptr [RBX + 0xdf8],0xbf800000
 */
const char* ANIMATION_TIMER_SIG = "[MOV dword ptr [r64+off32], imm32]";

/*
 * Anchor #2: UpdateAnimatedFlight function
 * Signature for the function prologue and initial register setup.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14054ac40[14054ac40]) ---/
 * 14054ac40  48 89 5C 24 10                MOV qword ptr [RSP + 0x10],RBX
 * 14054ac45  57                            PUSH RDI
 * 14054ac46  48 81 EC 90 00 00 00          SUB RSP,0x90
 * 14054ac4d  0F 29 B4 24 80 00 00 00       MOVAPS xmmword ptr [RSP + 0x80],XMM6
 * 14054ac55  33 FF                         XOR EDI,EDI
 */
const char* ANIMATED_FLIGHT_FUNC_SIG = "[MOV [r64+off8], r64] [PUSH r64] [SUB r64, imm32] [MOVAPS [r64+sib+off32], xmm] [XOR r32, r32]";
}  // namespace

bool DebugCameraAnimationDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  Utils::FinderLog log(GetName());
  log.Info("Searching for Debug Camera Animation data...");

  bool all_found = true;

  // ── Phase 1: Animation Timer Offset ──
  // Located inside SetDebugCameraMode. Skipped when already cached.
  {
    auto phase = log.MakePhase("Animation Timer Offset");

    bool timerCached = owner.GetAnimationTimerOffset() != 0;
    if (!timerCached) {
      uintptr_t pfnSetDebugCameraMode = reinterpret_cast<uintptr_t>(owner.GetDebugCameraModeFunc());
      if (phase.Step(pfnSetDebugCameraMode, "SetDebugCameraMode", "FN")) {
        // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
        // 140544438  C7 83 F8 0D 00 00 00 00 80 BF MOV dword ptr [RBX + 0xdf8],0xbf800000
        uintptr_t sigAddr = Utils::PatternFinder::Find(pfnSetDebugCameraMode, 300, ANIMATION_TIMER_SIG);
        if (phase.Step(sigAddr, "Animation timer signature", "RT")) {
          // Offset is at byte 2 of the instruction: C7 83 [OFFSET]
          int32_t offset = Utils::PatternFinder::ReadInt32(sigAddr + 2);
          if (phase.StepOffset(offset, "Animation timer offset", "OFF")) {
            owner.SetAnimationTimerOffset(offset);
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
      phase.StepOffset(static_cast<int32_t>(owner.GetAnimationTimerOffset()), "Animation timer offset (cached)", "OFF");
    }
  }

  // ── Phase 2: UpdateAnimatedFlight Function ──
  // Located via a global prologue signature. Skipped when already cached.
  {
    auto phase = log.MakePhase("UpdateAnimatedFlight");

    bool funcCached = owner.GetUpdateAnimatedFlightFunc() != nullptr;
    if (!funcCached) {
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14054ac40[14054ac40]) ---/
      // 14054ac40  48 89 5C 24 10                MOV qword ptr [RSP + 0x10],RBX
      // 14054ac45  57                            PUSH RDI
      // 14054ac46  48 81 EC 90 00 00 00          SUB RSP,0x90
      // 14054ac4d  0F 29 B4 24 80 00 00 00       MOVAPS xmmword ptr [RSP + 0x80],XMM6
      // 14054ac55  33 FF                         XOR EDI,EDI
      uintptr_t pfnUpdateAnimatedFlight = Utils::PatternFinder::Find(ANIMATED_FLIGHT_FUNC_SIG);
      if (phase.Step(pfnUpdateAnimatedFlight, "UpdateAnimatedFlight function", "FN")) {
        owner.SetUpdateAnimatedFlightFunc(reinterpret_cast<void*>(pfnUpdateAnimatedFlight));
      } else {
        all_found = false;
      }
    } else {
      phase.Step(reinterpret_cast<uintptr_t>(owner.GetUpdateAnimatedFlightFunc()), "UpdateAnimatedFlight (cached)", "FN");
    }
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && owner.GetAnimationTimerOffset() != 0 && owner.GetUpdateAnimatedFlightFunc() != nullptr;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
