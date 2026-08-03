#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include "fmt/format.h"

#include <cstdint>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Unique string anchor to locate the function that calls UpdateEnvironmentState.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
 * 140854c76  48 8D 0D E3 2C 94 01          LEA RCX,[0x142197960] = "Missing Headquarters %s"
 */
const char* UPDATE_ENV_STRING = "Missing Headquarters %s";

/**
 * @brief Pattern for the first CALL rel32 inside the anchor function — targets UpdateEnvironmentState.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
 * 14085497c  E8 6F FA C7 FF                CALL 0x1404d43f0
 */
const char* UPDATE_ENV_CALL_SIG = "[CALL rel32]";

/**
 * @brief Unique string anchor to locate UpdateGameSession (the function that calls UpdateSimulationTime).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
 * 1408527e6  48 8D 0D 73 5A 96 01          LEA RCX,[0x1421b8260]
 */
const char* UPDATE_GAME_SESSION_STR = "[used_vehicles] %Iu used truck offers have expired (%Iu offers valid)";

/**
 * @brief Pattern for the CALL to UpdateSimulationTime followed by the RBP setup MOV.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
 * 14085279d  E8 3E 7C C3 FF                CALL 0x14048a3e0
 * 1408527a2  48 8B AE 10 02 00 00          MOV RBP,qword ptr [RSI + 0x210]
 */
const char* UPDATE_SIM_TIME_CALL_SIG = "[CALL rel32] [MOV r64, [r64+off32]]";

/**
 * @brief Logic chain that saves the updated simulation time (ms and sec).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a5c9  F3 41 0F 11 B9 7C 3E 00 00    MOVSS dword ptr [R9 + 0x3e7c],XMM7
 * 14048a5d2  41 89 81 78 3E 00 00          MOV dword ptr [R9 + 0x3e78],EAX
 */
const char* TIME_OFF_SIG = "F3 41 0F 11 B9 ? ? ? ? 41 [MOV [r64+off32], r32]";

/**
 * @brief Signature that tracks where sub-minute seconds are added and then saved.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a4d7  F3 0F 58 8D A0 01 00 00       ADDSS XMM1,dword ptr [RBP + 0x1a0]
 */
const char* SUB_SEC_OFF_SIG = "[ADDSS xmm, [r64+off32]]";

/**
 * @brief Logic chain that loads the map scale multiplier (real seconds -> game seconds).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a490  F3 44 0F 10 96 B4 31 00 00    MOVSS XMM10,dword ptr [RSI + 0x31b4]
 * 14048a499  41 D1 EC                      SHR R12D,0x1
 */
const std::vector<std::string> MAP_SCALE_CHAIN = {"F3 [40-4F] 0F [10-59] [80-BF] ?? ?? ?? ??", "[40-4F] [C1-D1] [E8-EF]"};

/**
 * @brief Signature that reads real play time seconds.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a6fc  F3 0F 58 85 94 1B 00 00       ADDSS XMM0,dword ptr [RBP + 0x1b94]
 * 14048a704  0F 2F 05 FD E0 F4 01          COMISS XMM0,dword ptr [0x1423d8808]
 */
const char* REAL_SEC_SIG = "[ADDSS xmm, [r64+off32]] 0F 2F 05";

/**
 * @brief Signature for the Real Delta Time read.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a437  F2 48 0F 2A 80 28 0B 00 00    CVTSI2SD XMM0,qword ptr [RAX + 0xb28]
 */
const char* DELTA_SIG = "F2 48 0F 2A 80";

/**
 * @brief Signature for the Skybox auto-update flag.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
 * 14048a5b2  45 39 B1 0C 47 00 00          CMP dword ptr [R9 + 0x470c],R14D
 * 14048a5b9  75 23                         JNZ 0x14048a5de
 */
const char* SKYBOX_SIG = "45 [CMP [r64+off32], r32] [JNE rel8]";

/**
 * @brief Unique string anchor to locate the CoreEngine_UpdateLoop function.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(CoreEngine_UpdateLoop[140416a70]) ---/
 * 1404170b2  48 8D 0D C7 F0 CF 01          LEA RCX,[0x142116180]
 */
const char* CORE_LOOP_STRING = "Forcing restart_shader_time() to avoid inaccu";

/**
 * @brief Signature for the Pause Status check.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(CoreEngine_UpdateLoop[140416a70]) ---/
 * 140416f06  41 38 97 91 0A 00 00          CMP byte ptr [R15 + 0xa91],DL
 * 140416f0d  74 0F                         JZ 0x140416f1e
 */
const char* PAUSE_SIG = "41 [CMP [r64+off32], r8] [JE rel8]";

/**
 * @brief Signature for the Global Warp multiplier read.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(CoreEngine_UpdateLoop[140416a70]) ---/
 * 140416f1e  F3 41 0F 10 97 8C 08 00 00    MOVSS XMM2,dword ptr [R15 + 0x88c]
 */
const char* WARP_CHAIN = "F3 41 0F 10 [80-BF]";

/**
 * @brief Master signature for the engine halt counters (Traffic, Simulation, Global).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(ForceUnwindEngineStates[1403ea710]) ---/
 * 1403ea7a1  FF 8F AC 0A 00 00             DEC dword ptr [RDI + 0xaac]
 * 1403ea7a7  FF 8F A8 0A 00 00             DEC dword ptr [RDI + 0xaa8]
 * 1403ea7ad  83 E8 01                      SUB EAX,0x1
 * 1403ea7b0  89 87 A0 0A 00 00             MOV dword ptr [RDI + 0xaa0],EAX
 */
const char* HALT_COUNTERS_SIG = "[DEC dword ptr [r64+off32]] [DEC dword ptr [r64+off32]] [SUB r32, imm8] [MOV [r64+off32], r32]";

/**
 * @brief Unique string anchor to locate con_cmd_goto (the city navigation command).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cc26  48 8D 0D EB 9E CF 01          LEA RCX,[0x142116b18] = "goto - last position is u"
 */
const char* GOTO_LAST_POS_STRING = "goto - last position is unknown";

/**
 * @brief Loads the kdop array count into a register then stores it on the stack.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cd71  48 8B 5A 20                   MOV RBX,qword ptr [RDX + 0x20]
 * 14041cd75  48 89 9D 08 02 00 00          MOV qword ptr [RBP + 0x208],RBX
 */
const char* KDOP_COUNT_SIG = "[MOV r64, [r64+off8]] [MOV [r64+off32], r64]";

/**
 * @brief LEA that begins the embedded array_t object on the GameplayManager.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cda4  48 8D 4A 10                   LEA RCX,[RDX + 0x10]
 */
const char* KDOP_ARRAY_LEA_SIG = "[LEA r64, [r64+off8]]";

/**
 * @brief MOV that reads the array_t data pointer (buffer start).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cdae  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
 */
const char* KDOP_ARRAY_DATA_SIG = "[MOV r64, [r64+off8]]";

/**
 * @brief CMP that checks the kdop item type byte against the city item type.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cdb6  80 78 0A 0C                   CMP byte ptr [RAX + 0xa],0xc
 */
const char* KDOP_ITEM_TYPE_SIG = "[CMP byte ptr [r64+off8], imm8]";

/**
 * @brief MOVZX that loads the kdop item record pointer (city_data).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cdd2  0F B6 47 48                   MOVZX EAX,byte ptr [RDI + 0x48]
 */
const char* KDOP_RECORD_OFF_SIG = "[MOVZX r32, [r64+off8]]";

/**
 * @brief TEST that reads the kdop item flags byte.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cdeb  F6 47 34 01                   TEST byte ptr [RDI + 0x34],0x1
 */
const char* KDOP_FLAGS_SIG = "F6 47 ? 01";

/**
 * @brief MOV that loads the city record uid into a register.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041ce24  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
 * 14041ce27  4C 8B C0                      MOV R8,RAX
 */
const char* KDOP_UID_SIG = "[MOV r32, [r64+off8]] [MOV r64, r64]";

/**
 * @brief MOVSS that loads the kdop item bounding radius float.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cfae  F3 0F 10 47 54                MOVSS XMM0,dword ptr [RDI + 0x54]
 */
const char* KDOP_RADIUS_SIG = "[MOVSS xmm, [r64+off8]]";

/**
 * @brief MULSS that multiplies radius by the kdop item bounding scale float.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041cfb3  F3 0F 59 47 50                MULSS XMM0,dword ptr [RDI + 0x50]
 */
const char* KDOP_SCALE_SIG = "F3 0F 59";

/**
 * @brief Unique string anchor in con_cmd_goto used to locate the kdop item vtable slots.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041c79e  48 8D 0D BB 9F CF 01          LEA RCX,[0x142116760]
 */
const char* GOTO_CUTSCENE_ITEM_STRING = "Item/node with uid '0x%I64X' found in cutscene (%s)";

/**
 * @brief CALL through the kdop item vtable slot returning the point count.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041c7dc  FF 50 68                      CALL qword ptr [RAX + 0x68]
 */
const char* KDOP_VTABLE_POINT_COUNT_SIG = "[CALL [r64+off8]]";

/**
 * @brief CALL through the kdop item vtable slot returning the point pointer.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041c80a  FF 50 70                      CALL qword ptr [RAX + 0x70]
 */
const char* KDOP_VTABLE_GETPOINT_SIG = "[CALL [r64+off8]]";

/**
 * @brief MOVSS that loads the fixed-point 1/256 scale constant via RIP.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041c7f8  F3 0F 10 35 60 AF FB 01       MOVSS XMM6,dword ptr [0x1423d7760]
 */
const char* KDOP_POINT_SCALE_SIG = "[MOVSS xmm, [rip+off32]]";

/**
 * @brief Unique string anchor for the city-not-found message.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041d050  48 8D 0D A9 9A CF 01          LEA RCX,[0x142116b00]
 */
const char* GOTO_CITY_NOT_FOUND_STRING = "City '%s' not found!";

/**
 * @brief MOV that reads the prism::string buffer pointer.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
 * 14041d106  48 8B 50 08                   MOV RDX,qword ptr [RAX + 0x8]
 */
const char* KDOP_STRING_BUF_SIG = "[MOV r64, [r64+off8]]";

}  // namespace

bool WorldDataFinder::TryFindOffsets(GameWorldService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());
  log.Info("Searching for GameWorld (Engine & Time) data using provided Ghidra signatures...");

  uintptr_t addr = 0;              // General purpose address variable
  uintptr_t pfnUpdateSimTime = 0;  // Entry point of UpdateSimulationTime
  uintptr_t pfnCoreEngineLoop = 0; // Entry point of CoreEngine_UpdateLoop
  uintptr_t addrData = 0;          // Kdop array data MOV (Phase 15 -> 16)

  // ── Phase 1: UpdateSimulationTime Function ──
  /*
   * SEARCH STRATEGY (Game Version 1.60):
   * Instead of pattern-matching the volatile function prologue, we anchor on a
   * unique log string inside UpdateGameSession and walk backwards to the CALL
   * that invokes UpdateSimulationTime.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
   * 1408527e6  48 8D 0D 73 5A 96 01          LEA RCX,[0x1421b8260]  <- string xref
   * 14085279d  E8 3E 7C C3 FF                CALL 0x14048a3e0        <- backward scan target
   * 1408527a2  48 8B AE 10 02 00 00          MOV RBP,qword ptr [RSI + 0x210]
   */
  {
    auto phase = log.MakePhase("UpdateSimulationTime Function");

    uintptr_t strAddr = PatternFinder::FindFunctionByString(UPDATE_GAME_SESSION_STR, false);
    if (phase.Step(strAddr, "UpdateGameSession string anchor", "REF")) {
      uintptr_t callAddr = PatternFinder::FindBackward(strAddr, 128, UPDATE_SIM_TIME_CALL_SIG);
      if (phase.Step(callAddr, "UpdateSimulationTime CALL", "RT")) {
        pfnUpdateSimTime = PatternFinder::GetRipAddress(callAddr, 1, 5);
        phase.Step(pfnUpdateSimTime, "UpdateSimulationTime", "FN");
      }
    }
  }

  // ── Phase 2: Simulation Time Offset ──
  /*
   * 3.1 [DATA: Simulation Time Offset]
   * This logic block at the end of UpdateSimulationTime saves the updated
   * time (ms and sec).
   */
  {
    auto phase = log.MakePhase("Simulation Time Offset");

    uintptr_t addrChain = PatternFinder::Find(pfnUpdateSimTime, 512, TIME_OFF_SIG);
    if (phase.Step(addrChain, "Simulation Time logic chain", "RT")) {
      // Find the MOV instruction within the chain (14048a5d2)
      uintptr_t addrMov = PatternFinder::Find(addrChain, 32, "41 [MOV [r64+off32], r32]");
      if (phase.Step(addrMov, "Time Offset MOV", "RT")) {
        int32_t timeOffset = PatternFinder::ReadInt32(addrMov + 3);
        if (phase.StepOffset(timeOffset, "Time Offset", "OFF")) {
          owner.SetTimeOffset(timeOffset);
        }
      }
    }
  }

  // ── Phase 3: UpdateEnvironmentState Function ──
  {
    auto phase = log.MakePhase("UpdateEnvironmentState Function");

    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
    // 140854c76  48 8D 0D E3 2C 94 01          LEA RCX,[0x142197960] = "Missing Headquarters %s"
    uintptr_t pfnEnvOwner = PatternFinder::FindFunctionByString(UPDATE_ENV_STRING, true);
    if (phase.Step(pfnEnvOwner, "Missing Headquarters function", "FN")) {
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
      // 14085497c  E8 6F FA C7 FF                CALL 0x1404d43f0
      uintptr_t addrCall = PatternFinder::Find(pfnEnvOwner, 128, UPDATE_ENV_CALL_SIG);
      if (phase.Step(addrCall, "UpdateEnvironmentState CALL", "RT")) {
        uintptr_t pfnUpdateEnv = PatternFinder::GetRipAddress(addrCall, 1, 5);
        if (phase.Step(pfnUpdateEnv, "UpdateEnvironmentState", "FN")) {
          owner.SetUpdateFnAddr(pfnUpdateEnv);
        }
      }
    }
  }

  // ── Phase 4: Simulation Time Offset ──
  /*
   * 3.2 [OFFSET: Simulation Time]
   * This offset reads the current simulation time (seconds) from the Time object.
   * Strategy: Find the constant first, then look for the MOV instruction in the
   * immediate vicinity. Ensures compatibility with both 1.59 and 1.60 ordering.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
   * 14048a448  B8 6D C1 16 6C                MOV EAX,0x6c16c16d
   * 14048a44d  44 8B BD 9C 01 00 00          MOV R15D,dword ptr [RBP + 0x19c]
   */
  {
    auto phase = log.MakePhase("Simulation Time Offset");

    uintptr_t addrConst = PatternFinder::Find(pfnUpdateSimTime, 512, "B8 6D C1 16 6C");
    if (phase.Step(addrConst, "Simulation Time magic constant", "RT")) {
      uintptr_t addrMov = PatternFinder::Find(addrConst, 32, "44 [MOV r32, [r64+off32]]");
      if (phase.Step(addrMov, "Simulation Time MOV", "RT")) {
        int32_t simTimeOff = PatternFinder::ReadInt32(addrMov + 3);
        if (phase.StepOffset(simTimeOff, "Simulation Time Offset", "OFF")) {
          owner.SetSimulationTimeOffset(simTimeOff);
        }
      }
    }
  }

  // ── Phase 5: Sub-Minute Seconds Offset ──
  /*
   * 3.3 [OFFSET: Sub-Minute Seconds] (Eternal Signature)
   * This signature tracks where sub-minute seconds are added and then saved.
   */
  {
    auto phase = log.MakePhase("Sub-Minute Seconds Offset");

    uintptr_t addrSig = PatternFinder::Find(pfnUpdateSimTime, 512, SUB_SEC_OFF_SIG);
    if (phase.Step(addrSig, "Sub-Minute Seconds signature", "RT")) {
      int32_t subSecOff = PatternFinder::ReadInt32(addrSig + 4);
      if (phase.StepOffset(subSecOff, "Sub-Minute Seconds Offset", "OFF")) {
        owner.SetSubMinuteSecondsOffset(subSecOff);
      }
    }
  }

  // ── Phase 6: Map Scale Offset ──
  /*
   * 3.4 [OFFSET: Map Scale] (Eternal Signature)
   * This multiplier converts real seconds to game seconds (e.g. 1:19).
   */
  {
    auto phase = log.MakePhase("Map Scale Offset");

    addr = pfnUpdateSimTime ? PatternFinder::FindChain(MAP_SCALE_CHAIN, 10, pfnUpdateSimTime) : 0;
    if (phase.Step(addr, "Map Scale logic chain", "RT")) {
      // Extract 32-bit offset from MOVSS/MULSS
      // Instruction: F3 REX 0F OP ModRM [OFFSET] (Offset is at +5)
      int32_t scaleOff = PatternFinder::ReadInt32(addr + 5);
      if (phase.StepOffset(scaleOff, "Map Scale Offset", "OFF")) {
        owner.SetMapScaleOffset(scaleOff);
      }
    }
  }

  // ── Phase 7: Real Play Time Offsets ──
  /*
   * 3.5 [OFFSET: Real Play Time (Min & Sec)] (Eternal Signature)
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
   * 14048a6fc  F3 0F 58 85 94 1B 00 00       ADDSS XMM0,dword ptr [RBP + 0x1b94]  (Sec)
   * 14048a704  0F 2F 05 FD E0 F4 01          COMISS XMM0,dword ptr [0x1423d8808]
   * ...
   * 14048a740  48 8D 8D 98 1B 00 00          LEA RCX,[RBP + 0x1b98]                (Min)
   */
  {
    auto phase = log.MakePhase("Real Play Time Offsets");

    uintptr_t addrSec = PatternFinder::Find(addr, 1024, REAL_SEC_SIG);
    if (phase.Step(addrSec, "Real Play seconds signature", "RT")) {
      int32_t realSecOff = PatternFinder::ReadInt32(addrSec + 4);

      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateSimulationTime[14048a3e0]) ---/
      // * 14048a740  48 8D 8D 98 1B 00 00          LEA RCX,[RBP + 0x1b98]
      uintptr_t addrMin = PatternFinder::Find(addrSec, 128, "[LEA r64, [r64+off32]]");
      int32_t realMinOff = addrMin ? PatternFinder::ReadInt32(addrMin + 3) : 0;

      bool okMin = phase.Step(addrMin, "Real Play minute LEA", "RT");
      bool okSecOff = phase.StepOffset(realSecOff, "Real Play Seconds Offset", "OFF");
      bool okMinOff = phase.StepOffset(realMinOff, "Real Play Minutes Offset", "OFF");
      if (okMin && okSecOff && okMinOff) {
        owner.SetRealPlaySecondsOffset(realSecOff);
        owner.SetRealPlayTimeOffset(realMinOff);
      }
    }
  }

  // ── Phase 8: Real Delta Time Offset ──
  /*
   * 3.6 [OFFSET: Real Delta Time] (Updated for v1.60)
   */
  {
    auto phase = log.MakePhase("Real Delta Time Offset");

    uintptr_t addrSig = PatternFinder::Find(pfnUpdateSimTime, 128, DELTA_SIG);
    if (phase.Step(addrSig, "Real Delta Time signature", "RT")) {
      int32_t deltaTimeOff = PatternFinder::ReadInt32(addrSig + 5);
      if (phase.StepOffset(deltaTimeOff, "Real Delta Time Offset", "OFF")) {
        owner.SetRealDeltaTimeOffset(deltaTimeOff);
      }
    }
  }

  // ── Phase 9: Skybox Auto-update Offset ──
  /*
   * 3.7 [OFFSET: Skybox Auto-update] (Eternal Signature)
   */
  {
    auto phase = log.MakePhase("Skybox Auto-update Offset");

    uintptr_t addrSig = PatternFinder::Find(pfnUpdateSimTime, 512, SKYBOX_SIG);
    if (phase.Step(addrSig, "Skybox Auto-update signature", "RT")) {
      int32_t skyboxOff = PatternFinder::ReadInt32(addrSig + 3);
      if (phase.StepOffset(skyboxOff, "Skybox Auto-update Offset", "OFF")) {
        owner.SetSkyboxAutoUpdateOffset(skyboxOff);
      }
    }
  }

  // ── Phase 10: CoreEngine_UpdateLoop Function ──
  /*
   * 4. Find the entry point of the CoreEngine_UpdateLoop function (Verified for v1.60).
   * Note: This function contains critical offsets for Warp, Pause, and Delta Time.
   */
  {
    auto phase = log.MakePhase("CoreEngine_UpdateLoop Function");

    pfnCoreEngineLoop = PatternFinder::FindFunctionByString(CORE_LOOP_STRING, true);
    phase.Step(pfnCoreEngineLoop, "CoreEngine_UpdateLoop", "FN");
  }

  // ── Phase 11: Pause Status Offset ──
  /*
   * 4.1 [OFFSET: Pause Status] (Updated for v1.60)
   */
  {
    auto phase = log.MakePhase("Pause Status Offset");

    addr = PatternFinder::Find(pfnCoreEngineLoop, 1500, PAUSE_SIG);
    if (phase.Step(addr, "Pause Status signature", "RT")) {
      // Offset is at byte 3 of the first instruction (CMP)
      int32_t pauseOff = PatternFinder::ReadInt32(addr + 3);
      if (phase.StepOffset(pauseOff, "Pause Status Offset", "OFF")) {
        owner.SetPauseStatusOffset(pauseOff);
      }
    }
  }

  // ── Phase 12: Global Warp Offset ──
  /*
   * 4.2 [OFFSET: Global Warp] (Verified for v1.59 & v1.60)
   * This multiplier controls the overall speed of the game engine (console "warp").
   */
  {
    auto phase = log.MakePhase("Global Warp Offset");

    uintptr_t addrWarp = PatternFinder::Find(addr, 32, WARP_CHAIN);
    if (phase.Step(addrWarp, "Global Warp signature", "RT")) {
      int32_t warpOff = PatternFinder::ReadInt32(addrWarp + 5);
      if (phase.StepOffset(warpOff, "Global Warp Offset", "OFF")) {
        owner.SetGlobalWarpOffset(warpOff);
      }
    }
  }

  // ── Phase 13: Engine Halt Counters ──
  /*
   * 5. [OFFSET: Engine Halt Counters] (Updated for v1.60)
   * These counters control the "frozen" state of the engine sub-systems.
   */
  {
    auto phase = log.MakePhase("Engine Halt Counters");

    uintptr_t addrSig = PatternFinder::Find(HALT_COUNTERS_SIG);
    if (phase.Step(addrSig, "Engine Halt master signature", "RT")) {
      // Extract 32-bit offsets from the first instructions
      int32_t trafficOff = PatternFinder::ReadInt32(addrSig + 2);
      int32_t simOff = PatternFinder::ReadInt32(addrSig + 8);
      int32_t globalOff = PatternFinder::ReadInt32(addrSig + 17);

      bool okTraffic = phase.StepOffset(trafficOff, "Traffic Halt Offset", "OFF");
      bool okSim = phase.StepOffset(simOff, "Simulation Halt Offset", "OFF");
      bool okGlobal = phase.StepOffset(globalOff, "Global Halt Offset", "OFF");
      if (okTraffic && okSim && okGlobal) {
        owner.SetTrafficHaltOffset(trafficOff);
        owner.SetSimulationHaltOffset(simOff);
        owner.SetGlobalHaltOffset(globalOff);
      }
    }
  }

  // ── Phase 14: city_data Reflection Attributes ──
  /*
   * Harvests every city_data class attribute offset through the game's SCS
   * reflection table (FindAttributeOffset). These offsets are used to read
   * the city records behind each city kdop item in the GameplayManager array.
   */
  {
    auto phase = log.MakePhase("city_data Reflection Attributes");

    auto getAttr = [&phase, &owner](const char* attrName) {
      uintptr_t off = PatternFinder::FindAttributeOffset("city_data", attrName);
      std::string desc = fmt::format("city_data::{}", attrName);
      phase.StepOffset(static_cast<int32_t>(off), desc, "REF");
      return off;
    };

    owner.SetCityNameOffset(getAttr("city_name"));
    owner.SetCityNameLocalizedOffset(getAttr("city_name_localized"));
    owner.SetShortCityNameOffset(getAttr("short_city_name"));
    owner.SetShortCityNameLocalizedOffset(getAttr("short_city_name_localized"));
    owner.SetCityGroupOffset(getAttr("city_group"));
    owner.SetCityPinScaleFactorOffset(getAttr("city_pin_scale_factor"));
    owner.SetMapXOffsetsOffset(getAttr("map_x_offsets"));
    owner.SetMapYOffsetsOffset(getAttr("map_y_offsets"));
    owner.SetPriceCoefOffset(getAttr("price_coef"));
    owner.SetCountryOffset(getAttr("country"));
    owner.SetPopulationOffset(getAttr("population"));
    owner.SetKeyCityOffset(getAttr("key_city"));
    owner.SetTimeZoneOffset(getAttr("time_zone"));
  }

  // ── Phase 15: GameplayManager kdop array offsets ──
  /*
   * Resolves the kdop array base/count offsets on the GameplayManager object.
   * These replace the hardcoded 0x18/0x20 constants used by RefreshCityCache.
   * The array_t.data offset is derived as (array_t base) + (data field offset).
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
   * 14041cc26  48 8D 0D EB 9E CF 01          LEA RCX,[0x142116b18] = "goto - last position is u"
   * 14041cd71  48 8B 5A 20                   MOV RBX,qword ptr [RDX + 0x20]
   * 14041cd75  48 89 9D 08 02 00 00          MOV qword ptr [RBP + 0x208],RBX
   * 14041cda4  48 8D 4A 10                   LEA RCX,[RDX + 0x10]
   * 14041cdae  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
   */
  {
    auto phase = log.MakePhase("GameplayManager kdop array offsets");

    uintptr_t addrGoto = PatternFinder::FindFunctionByString(GOTO_LAST_POS_STRING, false);
    if (phase.Step(addrGoto, "con_cmd_goto string anchor", "REF")) {
      uintptr_t addrCount = PatternFinder::Find(addrGoto, 360, KDOP_COUNT_SIG);
      if (phase.Step(addrCount, "Kdop array count MOV", "RT")) {
        int32_t countOff = PatternFinder::ReadInt8(addrCount + 3);
        if (phase.StepOffset(countOff, "Kdop Count Offset", "OFF")) {
          owner.SetKdopCountOffset(countOff);
        }

        uintptr_t addrLea = PatternFinder::Find(addrCount, 64, KDOP_ARRAY_LEA_SIG);
        if (phase.Step(addrLea, "Kdop array_t LEA", "RT")) {
          int32_t arrayBaseOff = PatternFinder::ReadInt8(addrLea + 3);
          if (phase.StepOffset(arrayBaseOff, "Kdop array_t base Offset", "OFF")) {
            addrData = PatternFinder::Find(addrLea, 16, KDOP_ARRAY_DATA_SIG);
            if (phase.Step(addrData, "Kdop array data MOV", "RT")) {
              int32_t dataOff = PatternFinder::ReadInt8(addrData + 3);
              if (phase.StepOffset(dataOff, "Kdop array data Offset", "OFF")) {
                int32_t kdopArrayOffset = arrayBaseOff + dataOff;
                if (phase.StepOffset(kdopArrayOffset, "Kdop Array Offset", "OFF")) {
                  owner.SetKdopArrayOffset(kdopArrayOffset);
                }
              }
            }
          }
        }
      }
    }
  }

  // ── Phase 16: kdop item layout offsets (type, record, flags, uid, scale, radius) ──
  /*
   * Resolves the kdop_item field offsets used by RefreshCityCache: the item
   * type byte, the city_data record pointer offset, the flags byte, the city
   * record uid and the bounding scale/radius floats. Search starts from the
   * kdop array data MOV resolved in Phase 15 and walks the con_cmd_goto loop
   * forward.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
   * 14041cdb6  80 78 0A 0C                   CMP byte ptr [RAX + 0xa],0xc
   * 14041cdd2  0F B6 47 48                   MOVZX EAX,byte ptr [RDI + 0x48]
   * 14041cdeb  F6 47 34 01                   TEST byte ptr [RDI + 0x34],0x1
   * 14041ce24  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
   * 14041ce27  4C 8B C0                      MOV R8,RAX
   * 14041cfae  F3 0F 10 47 54                MOVSS XMM0,dword ptr [RDI + 0x54]
   * 14041cfb3  F3 0F 59 47 50                MULSS XMM0,dword ptr [RDI + 0x50]
   */
  {
    auto phase = log.MakePhase("Kdop item layout offsets");

    uintptr_t addrItemType = PatternFinder::Find(addrData, 32, KDOP_ITEM_TYPE_SIG);
    if (phase.Step(addrItemType, "Kdop item type CMP", "RT")) {
      int32_t itemTypeOff = PatternFinder::ReadInt8(addrItemType + 2);
      int32_t cityItemType = PatternFinder::ReadInt8(addrItemType + 3);
      if (phase.StepOffset(itemTypeOff, "Kdop Item Type Offset", "OFF")) {
        owner.SetCityItemTypeOffset(itemTypeOff);
      }
      if (phase.StepOffset(cityItemType, "City Item Type", "OFF")) {
        owner.SetCityItemType(static_cast<uint8_t>(cityItemType));
      }

      uintptr_t addrRecord = PatternFinder::Find(addrItemType, 32, KDOP_RECORD_OFF_SIG);
      if (phase.Step(addrRecord, "Kdop item record MOVZX", "RT")) {
        int32_t recordOff = PatternFinder::ReadInt8(addrRecord + 3);
        if (phase.StepOffset(recordOff, "Kdop Item Record Offset", "OFF")) {
          owner.SetCityRecordOffset(recordOff);

          uintptr_t addrFlags = PatternFinder::Find(addrRecord, 32, KDOP_FLAGS_SIG);
          if (phase.Step(addrFlags, "Kdop item flags TEST", "RT")) {
            int32_t flagsOff = PatternFinder::ReadInt8(addrFlags + 2);
            if (phase.StepOffset(flagsOff, "Kdop Item Flags Offset", "OFF")) {
              owner.SetCityFlagsOffset(flagsOff);
            }

            uintptr_t addrUid = PatternFinder::Find(addrFlags, 64, KDOP_UID_SIG);
            if (phase.Step(addrUid, "City record uid MOV", "RT")) {
              int32_t uidOff = PatternFinder::ReadInt8(addrUid + 2);
              if (phase.StepOffset(uidOff, "City Record Uid Offset", "OFF")) {
                owner.SetCityUidOffset(uidOff);
              }
            }

            uintptr_t addrRadius = PatternFinder::Find(addrUid, 512, KDOP_RADIUS_SIG);
            if (phase.Step(addrRadius, "Kdop item radius MOVSS", "RT")) {
              int32_t radiusOff = PatternFinder::ReadInt8(addrRadius + 4);
              if (phase.StepOffset(radiusOff, "Kdop Item Radius Offset", "OFF")) {
                owner.SetCityRadiusOffset(radiusOff);

                uintptr_t addrScale = PatternFinder::Find(addrRadius, 16, KDOP_SCALE_SIG);
                if (phase.Step(addrScale, "Kdop item scale MULSS", "RT")) {
                  int32_t scaleOff = PatternFinder::ReadInt8(addrScale + 4);
                  if (phase.StepOffset(scaleOff, "Kdop Item Scale Offset", "OFF")) {
                    owner.SetCityScaleOffset(scaleOff);
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // ── Phase 17: kdop item vtable slots (point count, GetPoint) ──
  /*
   * Resolves the kdop_item vtable slot offsets used by GetCityPointCount and
   * GetCityPoint: the point count function and the GetPoint function. Search
   * starts from a unique string anchor inside con_cmd_goto and walks forward
   * through the vtable CALL sites.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
   * 14041c79e  48 8D 0D BB 9F CF 01          LEA RCX,[0x142116760]
   * 14041c7dc  FF 50 68                      CALL qword ptr [RAX + 0x68]
   * 14041c7f8  F3 0F 10 35 60 AF FB 01       MOVSS XMM6,dword ptr [0x1423d7760]
   * 14041c80a  FF 50 70                      CALL qword ptr [RAX + 0x70]
   */
  {
    auto phase = log.MakePhase("Kdop item vtable slots");

    uintptr_t addrCutscene = PatternFinder::FindFunctionByString(GOTO_CUTSCENE_ITEM_STRING, false);
    if (phase.Step(addrCutscene, "con_cmd_goto cutscene anchor", "REF")) {
      uintptr_t addrCountCall = PatternFinder::Find(addrCutscene, 96, KDOP_VTABLE_POINT_COUNT_SIG);
      if (phase.Step(addrCountCall, "Kdop item point count CALL", "RT")) {
        int32_t countSlot = PatternFinder::ReadInt8(addrCountCall + 2);
        if (phase.StepOffset(countSlot, "Kdop Point Count Vtable Slot", "OFF")) {
          owner.SetCityVtablePointCountSlot(countSlot);

          uintptr_t addrScaleMovss = PatternFinder::Find(addrCountCall, 64, KDOP_POINT_SCALE_SIG);
          if (phase.Step(addrScaleMovss, "Kdop point scale MOVSS", "RT")) {
            uintptr_t addrScaleConst = PatternFinder::GetRipAddress(addrScaleMovss, 4, 8);
            if (phase.Step(addrScaleConst, "Kdop point scale constant", "DATA")) {
              float pointScale = PatternFinder::ReadFloat(addrScaleConst);
              log.Info("  point_scale={}", pointScale);
              owner.SetCityPointScale(pointScale);

              uintptr_t addrGetPointCall = PatternFinder::Find(addrScaleMovss, 64, KDOP_VTABLE_GETPOINT_SIG);
              if (phase.Step(addrGetPointCall, "Kdop item GetPoint CALL", "RT")) {
                int32_t getPointSlot = PatternFinder::ReadInt8(addrGetPointCall + 2);
                if (phase.StepOffset(getPointSlot, "Kdop GetPoint Vtable Slot", "OFF")) {
                  owner.SetCityVtableGetPointSlot(getPointSlot);
                }
              }
            }
          }
        }
      }
    }
  }

  // ── Phase 18: prism::string buffer offset ──
  /*
   * Resolves the prism::string buffer pointer offset used by CopyRecordString.
   * Search starts from a unique string anchor inside con_cmd_goto and walks
   * forward to the MOV that loads the string buffer.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(con_cmd_goto[14041c3a0]) ---/
   * 14041d050  48 8D 0D A9 9A CF 01          LEA RCX,[0x142116b00]
   * 14041d106  48 8B 50 08                   MOV RDX,qword ptr [RAX + 0x8]
   */
  {
    auto phase = log.MakePhase("Kdop string buffer offset");

    uintptr_t addrNotFound = PatternFinder::FindFunctionByString(GOTO_CITY_NOT_FOUND_STRING, false);
    if (phase.Step(addrNotFound, "con_cmd_goto city not found anchor", "REF")) {
      uintptr_t addrStrBuf = PatternFinder::Find(addrNotFound, 256, KDOP_STRING_BUF_SIG);
      if (phase.Step(addrStrBuf, "Kdop string buffer MOV", "RT")) {
        int32_t strBufOff = PatternFinder::ReadInt8(addrStrBuf + 3);
        if (phase.StepOffset(strBufOff, "Kdop String Buffer Offset", "OFF")) {
          owner.SetCityStringBufOffset(strBufOff);
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetTimeOffset() != 0 && owner.GetUpdateFnAddr() != 0 && owner.GetSimulationTimeOffset() != 0 && owner.GetSubMinuteSecondsOffset() != 0 && owner.GetMapScaleOffset() != 0 &&
              owner.GetRealPlayTimeOffset() != 0 && owner.GetRealPlaySecondsOffset() != 0 && owner.GetGlobalWarpOffset() != 0 && owner.GetPauseStatusOffset() != 0 && owner.GetRealDeltaTimeOffset() != 0 &&
              owner.GetSkyboxAutoUpdateOffset() != 0 && owner.GetGlobalHaltOffset() != 0 && owner.GetSimulationHaltOffset() != 0 && owner.GetTrafficHaltOffset() != 0 && owner.GetKdopArrayOffset() != 0 &&
              owner.GetKdopCountOffset() != 0 && owner.GetCityItemTypeOffset() != 0 && owner.GetCityRecordOffset() != 0 && owner.GetCityFlagsOffset() != 0 && owner.GetCityUidOffset() != 0 &&
              owner.GetCityScaleOffset() != 0 && owner.GetCityRadiusOffset() != 0 && owner.GetCityVtablePointCountSlot() != 0 && owner.GetCityVtableGetPointSlot() != 0 &&
              owner.GetCityPointScale() > 0.0f && owner.GetCityStringBufOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
