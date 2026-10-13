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
constexpr size_t CHEAT_GATE_WINDOW = 64;    // gate prologue MOV/LEA window
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
  if (!addrAnchor) return log.Finish(false);

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
  if (!addrLoopExit) return log.Finish(false);

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
  if (!addrNext2) return log.Finish(false);

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
  if (!addrElement) return log.Finish(false);

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
  }

  // ── Phase 6: Cheat dispatcher / gate / manager (optional) ──
  {
    auto phase = log.MakePhase("Cheat Dispatcher");

    /*
     * String anchor: the cheat dispatcher prints the subcommand list.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
     * anchor string: "--- Available cheats:"
     */
    uintptr_t dispatcher = PatternFinder::FindFunctionByString("--- Available cheats:", true);
    if (phase.Step(dispatcher, "cheat dispatcher (string anchor)", "FN")) {
      m_cheatDispatcherAddr = dispatcher;

      /*
       * Wrapper layout from the consumer side: dispatcher receives the wrapper
       * in RDX and copies it to R10.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
       * 1405b6303  48 83 7A 20 02                CMP qword ptr [RDX + 0x20],0x2
       */
      const uintptr_t sizeCmp = PatternFinder::Find(dispatcher, 32, "48 83 7A");
      if (phase.Step(sizeCmp, "wrapper size check (CMP RDX+0x20)", "FN")) {
        int wrapDispLen = 0;
        phase.StepOffset(PatternFinder::ReadInstructionDisp(sizeCmp, wrapDispLen), "dispatcher size slot [RDX+0x20]", "OFF");

        /*
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
         * 1405b6308  4C 8B D2                      MOV R10,RDX
         */
        const uintptr_t wrapCopy = PatternFinder::Find(sizeCmp, 8, "[MOV r64, r64]");
        if (phase.Step(wrapCopy, "wrapper copy to R10 (MOV)", "FN")) {
          /*
           * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
           * 1405b65cb  49 8B 42 18                   MOV RAX,qword ptr [R10 + 0x18]
           * 1405b65cf  48 8B 50 28                   MOV RDX,qword ptr [RAX + 0x28]
           * 1405b65d3  49 8B 40 08                   MOV RAX,qword ptr [R8 + 0x8]
           */
          const uintptr_t tokenLoads = PatternFinder::Find(wrapCopy, 750, "[MOV r64, [r64+off8]] [MOV r64, [r64+off8]] [MOV r64, [r64+off8]]");
          if (phase.Step(tokenLoads, "token field loads (MOV x3)", "FN")) {
            int tokenDispLen = 0;
            phase.StepOffset(PatternFinder::ReadInstructionDisp(tokenLoads, tokenDispLen), "dispatcher data slot [R10+0x18]", "OFF");

            /*
             * The third load of the same triple is the entry name compare source.
             *
             * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
             * 1405b65cf  48 8B 50 28                   MOV RDX,qword ptr [RAX + 0x28]
             * 1405b65d3  49 8B 40 08                   MOV RAX,qword ptr [R8 + 0x8]
             */
            int midLoadLen = 0;
            PatternFinder::ReadInstructionDisp(tokenLoads + tokenDispLen, midLoadLen);
            int nameDispLen = 0;
            const int32_t entryNameOff = PatternFinder::ReadInstructionDisp(tokenLoads + tokenDispLen + midLoadLen, nameDispLen);
            if (phase.StepOffset(entryNameOff, "cheat entry name field [R8+0x08]", "OFF")) {
              m_cheatEntryNameOff = entryNameOff;
            }

            /*
             * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
             * 1405b6608  49 83 7A 20 01                CMP qword ptr [R10 + 0x20],0x1
             */
            const uintptr_t sizeCmp10 = PatternFinder::Find(tokenLoads, 64, "49 83 7A");
            if (phase.Step(sizeCmp10, "wrapper size check (CMP R10+0x20)", "FN")) {
              int sizeDispLen = 0;
              phase.StepOffset(PatternFinder::ReadInstructionDisp(sizeCmp10, sizeDispLen), "dispatcher size slot [R10+0x20]", "OFF");

              /*
               * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
               * 1405b660d  49 8D 4A 10                   LEA RCX,[R10 + 0x10]
               */
              const uintptr_t arrayLea = PatternFinder::Find(sizeCmp10, 16, "[LEA r64, [r64+off8]]");
              if (phase.Step(arrayLea, "tokens base LEA (R10+0x10)", "FN")) {
                int leaDispLen = 0;
                const int32_t tokensOff = PatternFinder::ReadInstructionDisp(arrayLea, leaDispLen);
                m_cheatTokensOff = tokensOff;
                phase.StepOffset(tokensOff, "dispatcher tokens base [R10+0x10]", "OFF");

                /*
                 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
                 * 1405b6617  48 8B 51 08                   MOV RDX,qword ptr [RCX + 0x8]
                 */
                const uintptr_t dataLoad = PatternFinder::Find(arrayLea, 16, "[MOV r64, [r64+off8]]");
                if (phase.Step(dataLoad, "tokens data load (RCX+0x08)", "FN")) {
                  int loadDispLen = 0;
                  phase.StepOffset(PatternFinder::ReadInstructionDisp(dataLoad, loadDispLen), "dispatcher array data field [RCX+0x08]", "OFF");

                  /*
                   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
                   * 1405b6662  49 8D 4A 10                   LEA RCX,[R10 + 0x10]
                   */
                  const uintptr_t arrayLea2 = PatternFinder::Find(dataLoad, 128, "[LEA r64, [r64+off8]]");
                  if (phase.Step(arrayLea2, "tokens base LEA confirm (R10+0x10)", "FN")) {
                    int confirmDispLen = 0;
                    phase.StepOffset(PatternFinder::ReadInstructionDisp(arrayLea2, confirmDispLen), "dispatcher tokens base confirm [R10+0x10]", "OFF");
                  }
                }
              }
            }
          }

          /*
           * Shared ctrl / manager / entry / decoded-string layout from the
           * dispatcher consumer side (all in FUN_1405b62f0).
           *
           * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
           * 1405b633b  48 8B 48 10                   MOV RCX,qword ptr [RAX + 0x10]
           */
          const uintptr_t ctrlInnerLoad = PatternFinder::Find(wrapCopy, 64, "[MOV r64, [r64+off8]]");
          if (phase.Step(ctrlInnerLoad, "shared ctrl inner load (RAX+0x10)", "FN")) {
            int ctrlInnerLen = 0;
            const int32_t ctrlInnerOff = PatternFinder::ReadInstructionDisp(ctrlInnerLoad, ctrlInnerLen);
            const bool ctrlInnerOk = phase.StepOffset(ctrlInnerOff, "shared ctrl inner pointer field", "OFF");

            /*
             * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
             * 1405b6353  49 69 47 10 A8 00 00 00       IMUL RAX,qword ptr [R15 + 0x10],0xa8
             */
            const uintptr_t imulIns = PatternFinder::Find(ctrlInnerLoad, 32, "49 69 47");
            if (phase.Step(imulIns, "manager count x stride (IMUL)", "FN")) {
              int imulLen = 0;
              const int32_t managerCountOff = PatternFinder::ReadInstructionDisp(imulIns, imulLen);
              const int32_t entryStride = *reinterpret_cast<const int32_t*>(imulIns + imulLen);
              const bool countOk = phase.StepOffset(managerCountOff, "manager count field", "OFF");
              const bool strideOk = phase.StepOffset(entryStride, "cheat entry stride", "OFF");

              /*
               * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
               * 1405b635b  49 8B 77 08                   MOV RSI,qword ptr [R15 + 0x8]
               */
              const uintptr_t mgrDataLoad = PatternFinder::Find(imulIns, 16, "[MOV r64, [r64+off8]]");
              if (phase.Step(mgrDataLoad, "manager data load (R15+0x08)", "FN")) {
                int mgrDataLen = 0;
                const int32_t managerDataOff = PatternFinder::ReadInstructionDisp(mgrDataLoad, mgrDataLen);
                const bool mgrDataOk = phase.StepOffset(managerDataOff, "manager data field", "OFF");

                /*
                 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
                 * 1405b63fa  48 83 C3 20                   ADD RBX,0x20
                 */
                const uintptr_t strideAdd = PatternFinder::Find(mgrDataLoad, 256, "[ADD r64, imm8]");
                if (phase.Step(strideAdd, "decoded string stride (ADD imm8)", "FN")) {
                  int addLen = 0;
                  PatternFinder::ReadInstructionDisp(strideAdd, addLen);
                  const int32_t tempPrismSize = *reinterpret_cast<const uint8_t*>(strideAdd + addLen);
                  const bool prismOk = phase.StepOffset(tempPrismSize, "temp prism size", "OFF");

                  /*
                   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b62f0[1405b62f0]) ---/
                   * 1405b63e3  4C 89 63 08                   MOV qword ptr [RBX + 0x8],R12
                   * 1405b63e7  48 8B CB                      MOV RCX,RBX
                   * 1405b63ea  48 89 7B 10                   MOV qword ptr [RBX + 0x10],RDI
                   */
                  const uintptr_t initStores = PatternFinder::FindBackward(strideAdd, 64, "[MOV [r64+off8], r64] [MOV r64, r64] [MOV [r64+off8], r64]");
                  if (phase.Step(initStores, "decoded init stores (buffer+len)", "FN")) {
                    int bufLen = 0;
                    const int32_t decodedBufferOff = PatternFinder::ReadInstructionDisp(initStores, bufLen);
                    int midLen = 0;
                    PatternFinder::ReadInstructionDisp(initStores + bufLen, midLen);
                    int lenLen = 0;
                    const int32_t decodedLenOff = PatternFinder::ReadInstructionDisp(initStores + bufLen + midLen, lenLen);
                    const bool bufOk = phase.StepOffset(decodedBufferOff, "decoded buffer field", "OFF");
                    const bool lenOk = phase.StepOffset(decodedLenOff, "decoded length field", "OFF");

                    if (ctrlInnerOk && countOk && strideOk && mgrDataOk && prismOk && bufOk && lenOk) {
                      m_cheatCtrlInnerOff = ctrlInnerOff;
                      m_cheatManagerCountOff = managerCountOff;
                      m_cheatEntryStride = entryStride;
                      m_cheatManagerDataOff = managerDataOff;
                      m_cheatTempPrismSize = tempPrismSize;
                      m_cheatDecodedBufferOff = decodedBufferOff;
                      m_cheatDecodedLenOff = decodedLenOff;
                    }
                  }
                }
              }
            }
          }
        }
      }

      /*
       * Gate: a direct CALL to the dispatcher whose function start is
       * followed within 64 bytes by BOTH [MOV r64, [rip+off32]] (manager)
       * and [LEA r64, [rip+off32]] (g_developer cvar).
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b66f0[1405b66f0]) ---/
       * 1405b66f9  48 8D 0D 10 7E 09 03          LEA RCX,[0x14364e510]
       * 1405b6709  48 8B 0D 80 16 10 03          MOV RCX,qword ptr [0x1436b7d90]
       */
      auto xrefs = PatternFinder::FindCallXrefs(dispatcher);
      uintptr_t gateMovIns = 0;
      uintptr_t gateLeaIns = 0;
      for (uintptr_t callSite : xrefs) {
        const uintptr_t start = PatternFinder::GetFunctionStart(callSite);
        if (!start) continue;
        const uintptr_t movIns = PatternFinder::Find(start, CHEAT_GATE_WINDOW, "[MOV r64, [rip+off32]]");
        const uintptr_t leaIns = PatternFinder::Find(start, CHEAT_GATE_WINDOW, "[LEA r64, [rip+off32]]");
        if (movIns && leaIns) {
          m_cheatGateAddr = start;
          gateMovIns = movIns;
          gateLeaIns = leaIns;
          break;
        }
      }
      if (phase.Step(m_cheatGateAddr, "cheat gate (MOV+LEA within 64)", "FN")) {
        m_cheatManagerPtrAddr = PatternFinder::GetRipAddress(gateMovIns, 3, 7);
        phase.Step(m_cheatManagerPtrAddr, "cheat manager pointer", "DATA");

        m_developerCvarAddr = PatternFinder::GetRipAddress(gateLeaIns, 3, 7);
        phase.Step(m_developerCvarAddr, "g_developer cvar", "DATA");

        /*
         * GetAndCacheValue: first CALL inside the gate (cvar reader).
         *
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1405b66f0[1405b66f0]) ---/
         * 1405b6700  E8 5B 69 C1 FF                CALL 0x1401cd060
         */
        const uintptr_t getAndCacheCall = PatternFinder::Find(m_cheatGateAddr, 64, "[CALL rel32]");
        if (phase.Step(getAndCacheCall, "GetAndCacheValue (CALL from gate)", "FN")) {
          m_getAndCacheValueAddr = PatternFinder::GetRipAddress(getAndCacheCall, 1, 5);
          phase.Step(m_getAndCacheValueAddr, "GetAndCacheValue", "FN");
        }
      }
    }
  }

  // ── Phase 7: Cheat token array layout (Table A: FUN_1401e1f10) ──
  {
    auto phase = log.MakePhase("Cheat Array Layout");

    /*
     * String anchor: the unknown-command printer builds the token array.
     *
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
     * anchor string: "[cmd] '%s' - unknown command"
     */
    uintptr_t cmdFn = PatternFinder::FindFunctionByString("[cmd] '%s' - unknown command", true);
    if (phase.Step(cmdFn, "command token builder (string anchor)", "FN")) {
      // Step 1 (len 256): refcount increment imm64, ctrl frame slot, refcount field.
      /*
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
       * 1401e1fb7 48 B8 00 00 00 00 01 00 00 00 MOV RAX,0x100000000
       * 1401e1fc1 48 89 4D 80 MOV qword ptr [RBP + -0x80],RCX
       * 1401e1fc5 F0 48 0F C1 41 58 XADD.LOCK qword ptr [RCX + 0x58],RAX
       */
      bool ctrlOk = false;
      int32_t ctrlFrameSlot = 0;
      const uintptr_t ins1 = PatternFinder::Find(cmdFn, 256, "[MOV r64, imm64] [MOV [r64+off8], r64]");
      if (phase.Step(ins1, "ctrl init (imm64 + store + XADD)", "FN")) {
        m_ctrlRefcountInc = static_cast<uint64_t>(*reinterpret_cast<const int64_t*>(ins1 + 2));
        // [MOV r64, imm64] = 10 bytes → the ctrl store follows; XADD.LOCK is next.
        const uintptr_t ins2 = ins1 + 10;
        int ctrlStoreLen = 0;
        ctrlFrameSlot = PatternFinder::ReadInstructionDisp(ins2, ctrlStoreLen);
        const uintptr_t xadd = ins2 + ctrlStoreLen;
        int xaddLen = 0;
        // ReadInstructionDisp starts at the C1 opcode: F0 48 0F | C1 41 58.
        const int32_t refcountOff = (*reinterpret_cast<const uint8_t*>(xadd) == 0xF0) ? PatternFinder::ReadInstructionDisp(xadd + 3, xaddLen) : 0;
        ctrlOk = phase.StepOffset(refcountOff, "ctrl refcount field", "OFF");
        if (ctrlOk) {
          m_ctrlRefcountOff = refcountOff;
        }
      }

      // Step 2 (len 64, from the ctrl store): array vtable, element vtable, array base slot.
      /*
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
       * 1401e1fda 48 8D 0D 17 3E FF 01 LEA RCX,[0x1421d5df8]
       * 1401e1fe1 48 89 4C 24 60 MOV qword ptr [RSP + 0x60],RCX
       * 1401e1fe6 4C 8D 35 E3 F8 FE 01 LEA R14,[0x1421d18d0]
       * 1401e1fed 48 89 5D 98 MOV qword ptr [RBP + -0x68],RBX
       * 1401e1ff1 4C 89 75 88 MOV qword ptr [RBP + -0x78],R14
       */
      bool vtableOk = false;
      int32_t arrayBaseSlot = 0;
      uintptr_t vtIns = 0;
      uintptr_t nodeIns = 0;
      int32_t nodeSlot = 0;
      int32_t idSlot = 0;
      if (ctrlOk) {
        vtIns = PatternFinder::Find(ins1 + 10,
                                    64,
                                    "[LEA r64, [rip+off32]] [MOV [r64+off8], r64] "
                                    "[LEA r64, [rip+off32]] [MOV [r64+off8], r64] [MOV [r64+off8], r64]");
        if (phase.Step(vtIns, "array/element vtable stores", "FN")) {
          m_arrayVtableAddr = PatternFinder::GetRipAddress(vtIns, 3, 7);
          phase.Step(m_arrayVtableAddr, "array vtable", "DATA");

          int baseSlotLen = 0;
          arrayBaseSlot = PatternFinder::ReadInstructionDisp(vtIns + 7, baseSlotLen);
          vtableOk = phase.StepOffset(arrayBaseSlot, "array base slot", "OFF");
          if (vtableOk) {
            m_elementVtableAddr = PatternFinder::GetRipAddress(vtIns + 7 + baseSlotLen, 3, 7);
            phase.Step(m_elementVtableAddr, "element vtable", "DATA");
          }
        }
      }

      /*
       * Wrapper node/id stores — node slot is the wrapper base in the RSP frame.
       *
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
       * 1401e2102  4C 89 44 24 50                MOV qword ptr [RSP + 0x50],R8
       * 1401e2107  44 89 6C 24 58                MOV dword ptr [RSP + 0x58],R13D
       */
      if (vtableOk) {
        nodeIns = PatternFinder::Find(vtIns, 312, "[MOV [r64+off8], r64] 44 [MOV [r64+off8], r32]");
        if (phase.Step(nodeIns, "wrapper node/id stores (MOV+MOV)", "FN")) {
          int nodeDispLen = 0;
          nodeSlot = PatternFinder::ReadInstructionDisp(nodeIns, nodeDispLen);
          idSlot = PatternFinder::ReadInstructionDisp(nodeIns + nodeDispLen, nodeDispLen);
          phase.StepOffset(nodeSlot, "wrapper node slot", "OFF");
          phase.StepOffset(idSlot, "wrapper id slot", "OFF");
        }
      }

      // Steps 3-6: size, cap, base confirmation and data slots of the grow check.
      /*
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
       * 1401e21a1 48 8B 44 24 70 MOV RAX,qword ptr [RSP + 0x70]
       * 1401e21a6 48 8D 50 01 LEA RDX,[RAX + 0x1]
       * 1401e21aa 48 3B 54 24 78 CMP RDX,qword ptr [RSP + 0x78]
       */
      bool sizeOk = false;
      bool capOk = false;
      bool dataOk = false;
      int32_t sizeSlot = 0;
      int32_t capSlot = 0;
      int32_t dataSlot = 0;
      if (vtableOk) {
        // Step 3 (len 256 from the wrapper node store): size slot — the first MOV.
        const uintptr_t sizeIns = PatternFinder::Find(nodeIns, 256, "[MOV r64, [r64+off8]] [LEA r64, [r64+off8]] [CMP r64, [r64+off8]]");
        if (phase.Step(sizeIns, "size load (MOV/LEA/CMP)", "FN")) {
          int dispLen = 0;
          sizeSlot = PatternFinder::ReadInstructionDisp(sizeIns, dispLen);
          sizeOk = phase.StepOffset(sizeSlot, "array size slot", "OFF");

          if (sizeOk) {
            // Step 4 (len 16): cap slot — the CMP operand.
            const uintptr_t capIns = PatternFinder::Find(sizeIns, 16, "[CMP r64, [r64+off8]]");
            if (phase.Step(capIns, "cap load (grow CMP)", "FN")) {
              capSlot = PatternFinder::ReadInstructionDisp(capIns, dispLen);
              capOk = phase.StepOffset(capSlot, "array cap slot", "OFF");

              if (capOk) {
                // Step 5 (len 32): base confirmation — LEA of the array base.
                /*
                 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
                 * 1401e21b6 48 8D 4C 24 60 LEA RCX,[RSP + 0x60]
                 */
                const uintptr_t baseIns = PatternFinder::Find(capIns, 32, "[LEA r64, [r64+off8]]");
                if (phase.Step(baseIns, "array base confirm (LEA)", "FN")) {
                  int baseLen = 0;
                  const int32_t baseConfirm = PatternFinder::ReadInstructionDisp(baseIns, baseLen);
                  phase.StepOffset(baseConfirm, "array base confirm slot", "OFF");

                  // Step 6 (len 32): data slot — MOV followed by LEA.
                  /*
                   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
                   * 1401e21c7 4C 8B 44 24 68 MOV R8,qword ptr [RSP + 0x68]
                   * 1401e21cc 48 8D 50 01 LEA RDX,[RAX + 0x1]
                   */
                  const uintptr_t dataIns = PatternFinder::Find(baseIns, 32, "[MOV r64, [r64+off8]] [LEA r64, [r64+off8]]");
                  if (phase.Step(dataIns, "data load (MOV)", "FN")) {
                    dataSlot = PatternFinder::ReadInstructionDisp(dataIns, dispLen);
                    dataOk = phase.StepOffset(dataSlot, "array data slot", "OFF");

                    // Step toward the wrapper→handler call at 1401e22f0 (search from the data load).
                    /*
                     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
                     * 1401e21f0  48 89 70 08                   MOV qword ptr [RAX + 0x8],RSI
                     * 1401e21f4  48 C7 40 10 00 00 00 00       MOV qword ptr [RAX + 0x10],0x0
                     */
                    const uintptr_t initIns = PatternFinder::Find(dataIns, 64, "[MOV [r64+off8], r64] [MOV qword ptr [r64+off8], imm64_32]");
                    phase.Step(initIns, "init stores (MOV + MOV imm64_32)", "FN");
                  }
                }
              }
            }
          }
        }
      }

      // Step 7 (len 32, from function start): frame delta → ctrl field offset.
      /*
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401e1f10[1401e1f10]) ---/
       * 1401e1f16 48 8D AC 24 00 F4 FF FF LEA RBP,[RSP + -0xc00]
       * 1401e1f1e 48 81 EC 00 0D 00 00 SUB RSP,0xd00
       *
       * RBP = RSP_at_lea + leaOff, then RSP -= subImm → RBP − RSP_body = leaOff + subImm.
       * ctrl field = (RBP + ctrlFrameSlot) − (RSP_body + arrayBaseSlot).
       */
      if (ctrlOk && vtableOk && sizeOk && capOk && dataOk) {
        const uintptr_t prolog = PatternFinder::Find(cmdFn, 32, "[LEA r64, [r64+sib+off32]] [SUB r64, imm32]");
        if (phase.Step(prolog, "frame delta (LEA RBP + SUB RSP)", "FN")) {
          int leaLen = 0;
          const int32_t leaOff = PatternFinder::ReadInstructionDisp(prolog, leaLen);
          // SUB r64, imm32 = REX 81 /5 modrm imm32 → imm32 sits 3 bytes past the SUB opcode.
          const int32_t subImm = *reinterpret_cast<const int32_t*>(prolog + leaLen + 3);
          if (phase.StepOffset(subImm, "frame SUB imm32", "OFF")) {
            const int32_t ctrlOff = leaOff + subImm + ctrlFrameSlot - arrayBaseSlot;
            const int32_t dataOff = dataSlot - arrayBaseSlot;
            const int32_t sizeOff = sizeSlot - arrayBaseSlot;
            const int32_t capOff = capSlot - arrayBaseSlot;
            const bool ctrlOffOk = phase.StepOffset(ctrlOff, "array ctrl field", "OFF");
            const bool dataOffOk = phase.StepOffset(dataOff, "array data field", "OFF");
            const bool sizeOffOk = phase.StepOffset(sizeOff, "array size field", "OFF");
            const bool capOffOk = phase.StepOffset(capOff, "array cap field", "OFF");
            if (ctrlOffOk && dataOffOk && sizeOffOk && capOffOk) {
              m_arrayCtrlOff = ctrlOff;
              m_arrayDataOff = dataOff;
              m_arraySizeOff = sizeOff;
              m_arrayCapOff = capOff;
            }
          }
        }

        m_cheatWrapperSize = (m_cheatTokensOff > 0 && m_arrayCtrlOff > 0) ? (static_cast<int64_t>(m_cheatTokensOff) + m_arrayCtrlOff + 8) : 0;
      }
    }
  }

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
