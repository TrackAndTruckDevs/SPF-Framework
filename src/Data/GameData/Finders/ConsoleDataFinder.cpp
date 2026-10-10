/**
 * @file ConsoleDataFinder.cpp
 * @brief Implementation of pattern searcher for console command registries.
 *
 * Walks ConsoleCompleteCommand (verified against Ghidra 1.61) and extracts
 * the three command sources: console command list, script command list,
 * and input constants array.
 */

#include "SPF/Data/GameData/Finders/ConsoleDataFinder.hpp"

#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>

namespace SPF::Data::GameData::Finders {

using namespace SPF::Utils;

namespace {
/*
 * ConsoleCompleteCommand anchor.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
 * 141525deb  49 3B FD                      CMP RDI,R13
 * 141525dee  0F 84 64 01 00 00             JZ 0x141525f58
 * 141525df4  49 8B 46 08                   MOV RAX,qword ptr [R14 + 0x8]
 * 141525df8  0F B6 08                      MOVZX ECX,byte ptr [RAX]
 */
const char* CONSOLE_COMPLETE_SIG = "[CMP r64, r64] [JE rel32] [MOV r64, [r64+off8]] [MOVZX r32, [r64]]";

constexpr size_t HEAD_LOAD_WINDOW = 64;     // backward from anchor
constexpr size_t SENTINEL_LEA_WINDOW = 16;  // from head load
constexpr size_t NAME_CMP_WINDOW = 32;      // from anchor
constexpr size_t NAME_COPY_WINDOW = 64;     // from name CMP
constexpr size_t NEXT1_WINDOW = 312;        // from name copy
constexpr size_t LOOP_EXIT_WINDOW = 32;     // from list advance
constexpr size_t HEAD2_LOAD_WINDOW = 32;    // from loop exit
constexpr size_t NAME2_LEA_WINDOW = 32;     // from head2 load
constexpr size_t NEXT2_WINDOW = 312;        // from name2 LEA
constexpr size_t HEAD3_LOAD_WINDOW = 32;    // from list2 advance
constexpr size_t DATA3_LOAD_WINDOW = 64;    // from head3 load
constexpr size_t ELEMENT_WINDOW = 32;       // from data3 load
constexpr size_t SUGGESTIONS_WINDOW = 352;  // from element match
constexpr size_t ARRAY_BASE_WINDOW = 1200;  // from suggestions data
}  // namespace

bool ConsoleDataFinder::TryFind() {
  if (m_isReady) return true;

  FinderLog log(GetName());

  // ── Phase 1: ConsoleCompleteCommand anchor ──
  uintptr_t addrAnchor = 0;
  {
    auto phase = log.MakePhase("ConsoleCompleteCommand Anchor");

    addrAnchor = PatternFinder::Find(CONSOLE_COMPLETE_SIG);
    if (phase.Step(addrAnchor, "CMP/JE/MOV/MOVZX anchor", "RT")) {
      // Step 12 target sits later in the chain; anchor validated above.
    }
  }
  if (!addrAnchor) {
    log.Error("ConsoleCompleteCommand anchor not found.");
    return log.Finish(false);
  }

  // ── Phase 2: Console command list (head + sentinel + node layout) ──
  uintptr_t addrLoopExit = 0;
  {
    auto phase = log.MakePhase("Console Command List");

    /*
     * Head load: search BACKWARD from the anchor.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 141525db8  48 8B 3D 41 C6 21 01          MOV RDI,qword ptr [0x142742400]
     */
    uintptr_t addrHeadLoad = PatternFinder::FindBackward(addrAnchor, HEAD_LOAD_WINDOW, "[MOV r64, [rip+off32]]");
    if (phase.Step(addrHeadLoad, "list1 head load", "RT")) {
      m_consoleListHeadAddr = PatternFinder::GetRipAddress(addrHeadLoad, 3, 7);
      phase.Step(m_consoleListHeadAddr, "console command list head", "DATA");
    }

    /*
     * Sentinel LEA right after the head load.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 141525dbf  4C 8D 2D 4A C6 21 01          LEA R13,[0x142742410]
     */
    if (addrHeadLoad) {
      uintptr_t addrSentinel = PatternFinder::Find(addrHeadLoad, SENTINEL_LEA_WINDOW, "[LEA r64, [rip+off32]]");
      if (phase.Step(addrSentinel, "list1 sentinel LEA", "RT")) {
        m_consoleListSentinelAddr = PatternFinder::GetRipAddress(addrSentinel, 3, 7);
        phase.Step(m_consoleListSentinelAddr, "console command list sentinel", "DATA");
      }
    }

    /*
     * Name field: MOV r64, [r64+off8] followed by SUB (length compare).
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 141525dff  48 8B 57 10                   MOV RDX,qword ptr [RDI + 0x10]
     * 141525e03  48 2B D0                      SUB RDX,RAX
     */
    uintptr_t addrNameCmp = PatternFinder::Find(addrAnchor, NAME_CMP_WINDOW, "[MOV r64, [r64+off8]] [SUB r64, r64]");
    if (phase.Step(addrNameCmp, "list1 name compare", "RT")) {
      int len = 0;
      m_consoleNameOffset = PatternFinder::ReadInstructionDisp(addrNameCmp, len);
      phase.StepOffset(m_consoleNameOffset, "list1 name offset", "OFF");

      /*
       * Name copy: MOV r64, [r64+off8] followed by MOV r64, r64.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
       * 141525e1a  48 8B 47 10                   MOV RAX,qword ptr [RDI + 0x10]
       * 141525e1e  49 8B D9                      MOV RBX,R9
       */
      uintptr_t addrNameCopy = PatternFinder::Find(addrNameCmp, NAME_COPY_WINDOW, "[MOV r64, [r64+off8]] [MOV r64, r64]");
      if (phase.Step(addrNameCopy, "list1 name copy", "RT")) {
        int lenCopy = 0;
        int32_t nameCopyOffset = PatternFinder::ReadInstructionDisp(addrNameCopy, lenCopy);
        phase.StepOffset(nameCopyOffset, "list1 name offset (copy)", "OFF");
      }

      /*
       * List advance: bare [r64] next pointer, then vtable LEA.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
       * 141525f29  48 8B 3F                      MOV RDI,qword ptr [RDI]
       * 141525f2c  4C 8D 05 9D B9 CA 00          LEA R8,[0x1421d18d0]
       */
      uintptr_t addrAdvance = PatternFinder::Find(addrNameCmp, NEXT1_WINDOW, "[MOV r64, [r64]] [LEA r64, [rip+off32]]");
      if (phase.Step(addrAdvance, "list1 advance (next @ +0x00, no disp)", "NODE")) {
        m_consoleNextOffset = 0;  // pattern [MOV r64, [r64]] has no displacement

        /*
         * Loop exit: CMP r64, r64 against the sentinel.
         *
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
         * 141525f41  49 3B FD                      CMP RDI,R13
         */
        addrLoopExit = PatternFinder::Find(addrAdvance, LOOP_EXIT_WINDOW, "[CMP r64, r64]");
        phase.Step(addrLoopExit, "list1 loop exit CMP", "RT");
      }
    }
  }
  if (!addrLoopExit) {
    log.Error("Console command list discovery incomplete.");
    return log.Finish(false);
  }

  // ── Phase 3: Script command list ──
  uintptr_t addrNext2 = 0;
  {
    auto phase = log.MakePhase("Script Command List");

    /*
     * Head load of the second list.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 141525f58  4C 8B 2D 79 2D 16 02          MOV R13,qword ptr [0x143688cd8]
     */
    uintptr_t addrHead2 = PatternFinder::Find(addrLoopExit, HEAD2_LOAD_WINDOW, "[MOV r64, [rip+off32]]");
    if (phase.Step(addrHead2, "list2 head load", "RT")) {
      m_scriptListHeadAddr = PatternFinder::GetRipAddress(addrHead2, 3, 7);
      phase.Step(m_scriptListHeadAddr, "script command list head", "DATA");

      /*
       * Inline name LEA.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
       * 141525f70  49 8D 45 08                   LEA RAX,[R13 + 0x8]
       */
      uintptr_t addrName2 = PatternFinder::Find(addrHead2, NAME2_LEA_WINDOW, "[LEA r64, [r64+off8]]");
      if (phase.Step(addrName2, "list2 name LEA", "RT")) {
        int len = 0;
        m_scriptNameOffset = PatternFinder::ReadInstructionDisp(addrName2, len);
        phase.StepOffset(m_scriptNameOffset, "list2 name offset", "OFF");

        /*
         * List2 advance: next at +0x130, then sentinel LEA.
         *
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
         * 141526094  4D 8B AD 30 01 00 00          MOV R13,qword ptr [R13 + 0x130]
         * 14152609b  4C 8D 0D 8C B3 8C 00          LEA R9,[0x141df142e]
         */
        addrNext2 = PatternFinder::Find(addrName2, NEXT2_WINDOW, "[MOV r64, [r64+off32]] [LEA r64, [rip+off32]]");
        if (phase.Step(addrNext2, "list2 advance", "NODE")) {
          int lenNext = 0;
          m_scriptNextOffset = PatternFinder::ReadInstructionDisp(addrNext2, lenNext);
          phase.StepOffset(m_scriptNextOffset, "list2 next offset", "OFF");
        }
      }
    }
  }
  if (!addrNext2) {
    log.Error("Script command list discovery incomplete.");
    return log.Finish(false);
  }

  // ── Phase 4: Input constants array ──
  uintptr_t addrElement = 0;
  {
    auto phase = log.MakePhase("Input Constants");

    /*
     * Array size global.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 1415260ab  48 8B 05 FE CD 21 01          MOV RAX,qword ptr [0x142742eb0]
     */
    uintptr_t addrSize = PatternFinder::Find(addrNext2, HEAD3_LOAD_WINDOW, "[MOV r64, [rip+off32]]");
    if (phase.Step(addrSize, "input array size load", "RT")) {
      m_inputConstSizeAddr = PatternFinder::GetRipAddress(addrSize, 3, 7);
      phase.Step(m_inputConstSizeAddr, "input constants size", "DATA");

      /*
       * Array data global.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
       * 1415260d8  48 8B 05 C9 CD 21 01          MOV RAX,qword ptr [0x142742ea8]
       * 1415260df  48 8D 4D 40                   LEA RCX,[RBP + 0x40]
       */
      uintptr_t addrData = PatternFinder::Find(addrSize, DATA3_LOAD_WINDOW, "[MOV r64, [rip+off32]] [LEA r64, [r64+off8]]");
      if (phase.Step(addrData, "input array data load", "RT")) {
        m_inputConstDataAddr = PatternFinder::GetRipAddress(addrData, 3, 7);
        phase.Step(m_inputConstDataAddr, "input constants data", "DATA");

        /*
         * Element access: [RAX + R12*0x8], name at +0x08, copy helper.
         *
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
         * 1415260e3  4A 8B 14 E0                   MOV RDX,qword ptr [RAX + R12*0x8]
         * 1415260e7  48 83 C2 08                   ADD RDX,0x8
         * 1415260eb  E8 D0 F5 CE FE                CALL 0x1402156c0
         */
        addrElement = PatternFinder::Find(addrData, ELEMENT_WINDOW, "[MOV r64, [r64+sib]] [ADD r64, imm8] [CALL rel32]");
        if (phase.Step(addrElement, "input array element access", "RT")) {
          m_inputConstNameOffset = PatternFinder::ReadInt8(addrElement + 7);  // imm8 of ADD at 1415260ea
          phase.StepOffset(m_inputConstNameOffset, "input element name offset", "OFF");

          m_inputConstCopyNameFnAddr = PatternFinder::GetRipAddress(addrElement + 8, 1, 5);  // CALL at 1415260eb
          phase.Step(m_inputConstCopyNameFnAddr, "input element name copy", "FN");
        }
      }
    }
  }
  if (!addrElement) {
    log.Error("Input constants discovery incomplete.");
    return log.Finish(false);
  }

  // ── Phase 5: Suggestions buffer (assert path) ──
  {
    auto phase = log.MakePhase("Suggestions Buffer");

    /*
     * Suggestions data + count globals.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
     * 14152622a  48 8B 0D AF 33 8E 01          MOV RCX,qword ptr [0x142e095e0]
     * 141526231  48 8B 05 B0 33 8E 01          MOV RAX,qword ptr [0x142e095e8]
     */
    uintptr_t addrSugg = PatternFinder::Find(addrElement, SUGGESTIONS_WINDOW, "[MOV r64, [rip+off32]] [MOV r64, [rip+off32]]");
    if (phase.Step(addrSugg, "suggestions globals", "RT")) {
      m_suggestionsDataAddr = PatternFinder::GetRipAddress(addrSugg, 3, 7);
      phase.Step(m_suggestionsDataAddr, "suggestions data", "DATA");

      m_suggestionsCountAddr = PatternFinder::GetRipAddress(addrSugg + 7, 3, 7);  // second MOV at 141526231
      phase.Step(m_suggestionsCountAddr, "suggestions count", "DATA");

      /*
       * Array base (assert path; optional).
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(ConsoleCompleteCommand[141525d60]) ---/
       * 1415266b0  48 8D 0D E9 C7 21 01          LEA RCX,[0x142742ea0]
       * 1415266b7  E8 44 E2 BC FE                CALL 0x1400f4900
       * 1415266bc  48 89 44 24 30                MOV qword ptr [RSP + 0x30],RAX
       */
      uintptr_t addrArrayBase = PatternFinder::Find(addrSugg, ARRAY_BASE_WINDOW, "[LEA r64, [rip+off32]] [CALL rel32] [MOV [r64+off8], r64]");
      if (phase.StepOptional(addrArrayBase, "input array base LEA", "RT")) {
        m_inputConstArrayBaseAddr = PatternFinder::GetRipAddress(addrArrayBase, 3, 7);
        phase.StepOptional(m_inputConstArrayBaseAddr, "input array base", "DATA");
      }
    }

    m_isReady = (m_consoleListHeadAddr != 0) && (m_consoleListSentinelAddr != 0) && (m_consoleNameOffset != 0) && (m_scriptListHeadAddr != 0) && (m_scriptNameOffset != 0) && (m_scriptNextOffset != 0) && (m_inputConstSizeAddr != 0) &&
                (m_inputConstDataAddr != 0) && (m_inputConstNameOffset != 0) && (m_inputConstCopyNameFnAddr != 0) && (m_suggestionsDataAddr != 0) && (m_suggestionsCountAddr != 0);

    if (m_isReady) {
      log.Info(
        "Console registries: list1 head {:#x} sentinel {:#x} name +{:#x}; list2 head {:#x} name +{:#x} next +{:#x}; "
        "input size {:#x} data {:#x} name +{:#x}",
        m_consoleListHeadAddr,
        m_consoleListSentinelAddr,
        m_consoleNameOffset,
        m_scriptListHeadAddr,
        m_scriptNameOffset,
        m_scriptNextOffset,
        m_inputConstSizeAddr,
        m_inputConstDataAddr,
        m_inputConstNameOffset);
    } else {
      log.Error("Console command registry discovery incomplete. Command autocomplete list unavailable.");
    }
  }

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
