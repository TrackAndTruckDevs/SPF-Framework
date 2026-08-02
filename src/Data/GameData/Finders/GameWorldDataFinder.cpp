#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

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

}  // namespace

bool WorldDataFinder::TryFindOffsets(GameWorldService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());
  log.Info("Searching for GameWorld (Engine & Time) data using provided Ghidra signatures...");

  uintptr_t addr = 0;              // General purpose address variable
  uintptr_t pfnUpdateSimTime = 0;  // Entry point of UpdateSimulationTime
  uintptr_t pfnCoreEngineLoop = 0; // Entry point of CoreEngine_UpdateLoop

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

  // --- Final Readiness Check ---
  m_isReady = owner.GetTimeOffset() != 0 && owner.GetUpdateFnAddr() != 0 && owner.GetSimulationTimeOffset() != 0 && owner.GetSubMinuteSecondsOffset() != 0 && owner.GetMapScaleOffset() != 0 &&
              owner.GetRealPlayTimeOffset() != 0 && owner.GetRealPlaySecondsOffset() != 0 && owner.GetGlobalWarpOffset() != 0 && owner.GetPauseStatusOffset() != 0 && owner.GetRealDeltaTimeOffset() != 0 &&
              owner.GetSkyboxAutoUpdateOffset() != 0 && owner.GetGlobalHaltOffset() != 0 && owner.GetSimulationHaltOffset() != 0 && owner.GetTrafficHaltOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
