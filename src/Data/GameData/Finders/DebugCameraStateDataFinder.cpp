#include "SPF/Data/GameData/Finders/DebugCameraStateDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {
/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
 * 14054151c  48 8D 0D 9D F5 BE 01          LEA RCX,[0x142130ac0] = "CAMERA STATE SAVE: cannot open file %s""
 */
const char* OPEN_FILE_ERR_STR = "CAMERA STATE SAVE: cannot open file %s";

/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatAndWriteCameraState[1408ff2e0]) ---/
 * 1408ff2f7  48 8D 15 42 9E 8A 01          LEA RDX,[0x1421a9140] = " ; %g;%g;%g ; %g;%g;%g;%g ; %g\r\n"
 */
const char* FORMAT_AND_WRITE_STR = " ; %g;%g;%g ; %g;%g;%g;%g ; %g\r\n";

/**
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
 * 14054abd7  48 8D 0D 0A 5F BE 01          LEA RCX,[0x142130ae8] = "Camera state %Iu/%Iu"
 */
const char* CYCLE_SAVED_STATE_STR = "Camera state %Iu/%Iu";

/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
 * 140541555  48 8D 0D 4C E8 BE 01          LEA RCX,[0x14212fda8] = "Camera state saved"
 */
const char* SAVED_STATE_STR = "Camera state saved";
}  // namespace

bool DebugCameraStateDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  bool all_found = true;
  uintptr_t pfnCycleState = (uintptr_t)owner.GetCycleSavedStateFunc();

  // ── Phase 1: Save State Anchors ──
  {
    auto phase = log.MakePhase("Save State Anchors");

    // --- 1. Find AddCameraState function and StateContextOffset ---
    if (owner.GetStateContextOffset() == 0 || owner.GetAddCameraStateFunc() == nullptr) {
      // 1. Find the local usage of the error string, but stay at the instruction
      //    that loads it (do NOT climb to the function start).
      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
       * 14054151c  48 8D 0D 9D F5 BE 01          LEA RCX,[0x142130ac0]
       */
      uintptr_t errStrUsage = PatternFinder::FindFunctionByString(OPEN_FILE_ERR_STR, false);
      if (phase.Step(errStrUsage, "Save State error string usage", "REF")) {
        // 2. Walk forward to the LEA that reads the state context offset right
        //    before AddCameraState is called.
        /*
         * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
         * 140541544  48 8D 8E D0 0D 00 00          LEA RCX,[RSI + 0xdd0]
         */
        uintptr_t leaAddr = PatternFinder::Find(errStrUsage, 64, "[LEA r64, [r64+off32]]");
        if (phase.Step(leaAddr, "State context LEA", "RT")) {
          int32_t ctxOff = PatternFinder::ReadInt32(leaAddr + 3);
          if (phase.StepOffset(ctxOff, "StateContextOffset", "OFF")) {
            owner.SetStateContextOffset(ctxOff);
          } else {
            all_found = false;
          }

          // 3. Find the "Camera state saved" string usage, then walk backward to
          //    the direct CALL that invokes AddAnimatedCameraState.
          /*
           * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
           * 140541555  48 8D 0D 4C E8 BE 01          LEA RCX,[0x14212fda8] = "Camera state saved"
           * 140541550  E8 1B FC BE FF                CALL 0x140131170
           */
          uintptr_t savedStrUsage = PatternFinder::FindFunctionByString(SAVED_STATE_STR, false);
          if (phase.Step(savedStrUsage, "Camera state saved string usage", "REF")) {
            uintptr_t callAddr = PatternFinder::FindBackward(savedStrUsage, 16, "[CALL rel32]");
            if (phase.Step(callAddr, "AddAnimatedCameraState CALL", "RT")) {
              uintptr_t pAddState = PatternFinder::GetRipAddress(callAddr, 1, 5);
              if (phase.Step(pAddState, "AddAnimatedCameraState", "FN")) {
                owner.SetAddCameraStateFunc((void*)pAddState);
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
      } else {
        all_found = false;
      }
    }
  }

  // ── Phase 2: State Writer Functions ──
  {
    auto phase = log.MakePhase("State Writer Functions");

    // --- Find OpenFileForCameraState ---
    uintptr_t pfnOpenFile = (uintptr_t)owner.GetOpenFileForCameraStateFunc();
    if (!pfnOpenFile) {
      // 1. Find the local usage of the error string, but stay at the instruction
      //    that loads it (do NOT climb to the function start).
      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
       * 14054151c  48 8D 0D 9D F5 BE 01          LEA RCX,[0x142130ac0] = "CAMERA STATE SAVE: cannot...""
       */
      uintptr_t errStrUsage = PatternFinder::FindFunctionByString(OPEN_FILE_ERR_STR, false);
      if (phase.Step(errStrUsage, "OpenFile error string usage", "REF")) {
        // 2. The string is loaded right after the CALL to OpenFileForCameraState.
        //    Walk backwards to find that direct CALL site and resolve the target.
        /*
         * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
         * 140541504  E8 87 C1 BC FF                CALL 0x14010d690
         */
        uintptr_t callSite = PatternFinder::FindBackward(errStrUsage, 64, "[CALL rel32]");
        pfnOpenFile = PatternFinder::GetRipAddress(callSite, 1, 5);
      }
      phase.Step(pfnOpenFile, "OpenFileForCameraState", "FN");
      owner.SetOpenFileForCameraStateFunc((void*)pfnOpenFile);
    }

    // --- Find FormatAndWriteCameraState ---
    /*
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatAndWriteCameraState[1408ff2e0]) ---/
     * 1408ff2f7  48 8D 15 42 9E 8A 01          LEA RDX,[0x1421a9140] = " ; %g;%g;%g ; %g;%g;%g;%g ; %g\r\n"
     */
    uintptr_t pfnFormatAndWrite = (uintptr_t)owner.GetFormatAndWriteCameraStateFunc();
    if (!pfnFormatAndWrite) {
      pfnFormatAndWrite = PatternFinder::FindFunctionByString(FORMAT_AND_WRITE_STR, true);
      phase.Step(pfnFormatAndWrite, "FormatAndWriteCameraState", "FN");
      owner.SetFormatAndWriteCameraStateFunc((void*)pfnFormatAndWrite);
    }

    // --- Find CycleSavedState ---
    /**
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
     * 14054abd7  48 8D 0D 0A 5F BE 01          LEA RCX,[0x142130ae8] = "Camera state %Iu/%Iu"
     */
    if (!pfnCycleState) {
      pfnCycleState = PatternFinder::FindFunctionByString(CYCLE_SAVED_STATE_STR, false);
      if (phase.Step(pfnCycleState, "CycleSavedState string usage", "REF")) {
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
        // * 14054ab60  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
        uintptr_t pCycleStateStart = PatternFinder::GetFunctionStart(pfnCycleState);
        if (phase.Step(pCycleStateStart, "DebugCamera_CycleSavedState", "FN")) {
          owner.SetCycleSavedStateFunc((void*)pCycleStateStart);
        }
      }
    }
  }

  // ── Phase 3: CycleSavedState Internals ──
  if (pfnCycleState) {
    auto phase = log.MakePhase("CycleSavedState Internals");

    // --- Find StateArrayOffset (within CycleSavedState func) ---
    if (owner.GetStateArrayOffset() == 0) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
      // * 14054abb9  48 8B 83 D8 0D 00 00          MOV RAX,qword ptr [RBX + 0xdd8]
      // * 14054abc0  48 8D 0C D2                   LEA RCX,[RDX + RDX*0x8]
      // * 14054abc4  48 8D 14 88                   LEA RDX,[RAX + RCX*0x4]
      const char* p_arr = "[MOV r64, [r64+off32]] [LEA r64, [r64+sib]] [LEA r64, [r64+sib]]";
      uintptr_t addr = PatternFinder::FindBackward(pfnCycleState, 64, p_arr);
      if (phase.Step(addr, "StateArrayOffset anchor", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addr + 3);
        if (phase.StepOffset(off, "StateArrayOffset", "OFF")) {
          owner.SetStateArrayOffset(off);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    }

    // --- Find StateCountOffset (within CycleSavedState func) ---
    if (owner.GetStateCountOffset() == 0) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
      // * 14054abde  4C 8B 83 E0 0D 00 00          MOV R8,qword ptr [RBX + 0xde0]
      const char* p_count = "[MOV r64, [r64+off32]]";
      uintptr_t addr = PatternFinder::Find(pfnCycleState, 16, p_count);
      if (phase.Step(addr, "StateCountOffset anchor", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addr + 3);
        if (phase.StepOffset(off, "StateCountOffset", "OFF")) {
          owner.SetStateCountOffset(off);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    }

    // --- Find StateCurrentIndexOffset (within CycleSavedState func) ---
    if (owner.GetStateCurrentIndexOffset() == 0) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
      // * 14054abd0  48 8B 93 C8 0D 00 00          MOV RDX,qword ptr [RBX + 0xdc8]
      const char* p_idx = "[MOV r64, [r64+off32]]";
      uintptr_t addr = PatternFinder::FindBackward(pfnCycleState, 16, p_idx);
      if (phase.Step(addr, "StateCurrentIndexOffset anchor", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addr + 3);
        if (phase.StepOffset(off, "StateCurrentIndexOffset", "OFF")) {
          owner.SetStateCurrentIndexOffset(off);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    }

    // --- Find ApplyState (within CycleSavedState) ---
    if (owner.GetApplyStateFunc() == nullptr) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
      // * 14054abcb  E8 A0 FB FF FF                CALL 0x14054a770
      const char* p_apply = "[CALL rel32]";
      uintptr_t addr = PatternFinder::FindBackward(pfnCycleState, 32, p_apply);
      if (phase.Step(addr, "ApplyState CALL", "RT")) {
        uintptr_t pFunc = PatternFinder::GetRipAddress(addr, 1, 5);
        if (phase.Step(pFunc, "ApplyState", "FN")) {
          owner.SetApplyStateFunc((void*)pFunc);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    }

    // --- Find LoadStatesFromFile (within CycleSavedState func) ---
    if (owner.GetLoadStatesFromFileFunc() == nullptr) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_CycleSavedState[14054ab60]) ---/
      // * 14054ab81  E8 7A FC FF FF                CALL 0x14054a800
      const char* p_load = "[CALL rel32]";
      uintptr_t addr = PatternFinder::Find((uintptr_t)owner.GetCycleSavedStateFunc(), 64, p_load);
      if (phase.Step(addr, "LoadStatesFromFile CALL", "RT")) {
        uintptr_t pFunc = PatternFinder::GetRipAddress(addr, 1, 5);
        if (phase.Step(pFunc, "LoadStatesFromFile", "FN")) {
          owner.SetLoadStatesFromFileFunc((void*)pFunc);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = all_found && owner.GetStateContextOffset() != 0 && owner.GetAddCameraStateFunc() != nullptr && owner.GetOpenFileForCameraStateFunc() != nullptr && owner.GetFormatAndWriteCameraStateFunc() != nullptr && owner.GetCycleSavedStateFunc() != nullptr && owner.GetStateArrayOffset() != 0 && owner.GetStateCountOffset() != 0 && owner.GetStateCurrentIndexOffset() != 0 && owner.GetApplyStateFunc() != nullptr && owner.GetLoadStatesFromFileFunc() != nullptr;

  return log.Finish(m_isReady);
}
}  // namespace Data::GameData::Finders
SPF_NS_END
