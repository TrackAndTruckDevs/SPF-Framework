#include "SPF/Data/GameData/Finders/ObjectVehicleManagerFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

// String anchor for finding TrafficManager and ClearLocalVehicles.
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
// * 14053f766  48 8D 0D AB 02 BF 01          LEA RCX,[0x14212fa18]
const char* MACRO_CREATED_VEHICLES_STR = "Macro created %Iu vehicles from %Iu loaded";

// String anchor for finding the vehicle ID offset.
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
// * 140542503  48 8D 15 0E E1 BE 01          LEA RDX,[0x142130618]
const char* VEHICLE_ID_STR_ANCHOR = "state: %s<br>";

// String anchor for finding vehicle property offsets.
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
// * 1405491ce  48 8D 15 1B 78 BE 01          LEA RDX,[0x1421309f0]
const char* REMOTE_PLAYER_VEHICLE_STR = "[remote_player_vehicle]";

// Pattern: MOV REG, [R8-15+off]; TEST REG, REG; JZ; TEST byte [R8-15+off]
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateTrafficTrajectories[14054fc60]) ---/
// * 14054ff17  49 8B 8E F8 04 00 00          MOV RCX,qword ptr [R14 + 0x4f8]
// * 14054ff1e  48 85 C9                      TEST RCX,RCX
// * 14054ff21  74 30                         JZ 0x14054ff53
// * 14054ff23  41 F6 86 54 05 00 00 40       TEST byte ptr [R14 + 0x554],0x40
const char* PLAYER_CONTROLLER_SIG = "[MOV r64, [r64+off32]] [TEST r64, r64] [JE rel8] 41 F6 86 ? ? ? ? 40";

// Pattern: MOV REG, [REG+off8]; TEST REG, REG
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateAllTraffic[140609d30]) ---/
// * 140609f39  48 8B 49 48                   MOV RCX,qword ptr [RCX + 0x48]
// * 140609f3d  48 85 C9                      TEST RCX,RCX
const char* PLAYER_ACTOR_SIG = "[MOV r64, [r64+off8]] [TEST r64, r64]";

}  // namespace

bool ObjectManagerFinder::TryFindOffsets(GameObjectVehicleService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  // ── Phase 1: TrafficManager & ClearLocalVehicles ──
  uintptr_t pfnClear = 0;
  {
    auto phase = log.MakePhase("TrafficManager & ClearLocalVehicles");

    uintptr_t scanPos = PatternFinder::FindFunctionByString(MACRO_CREATED_VEHICLES_STR, false);
    if (phase.Step(scanPos, "Macro string XREF", "STR")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
      // * 14053f794  48 8B 0D F5 54 01 03          MOV RCX,qword ptr [0x143554c90]
      uintptr_t addrMov = PatternFinder::Find(scanPos, 64, "[MOV r64, [rip+off32]]");
      if (phase.Step(addrMov, "TrafficManager pointer load", "RT")) {
        uintptr_t pTrafficManager = PatternFinder::GetRipAddress(addrMov, 3, 7);
        uintptr_t trafficManagerAddr = *reinterpret_cast<uintptr_t*>(pTrafficManager);
        owner.SetTrafficManagerAddr(trafficManagerAddr);
        log.Info("--- [Phase 1] Found TrafficManager address at: {}", log.Rel(trafficManagerAddr));

        // Find ClearLocalVehicles call (CALL rel32)
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053f79d  E8 0E 71 01 00                CALL 0x1405568b0
        // * 14053f7a2  0F 28 05 87 9A E9 01          MOVAPS XMM0,xmmword ptr [0x1423d9230]
        uintptr_t addrCall = PatternFinder::Find(scanPos, 96, "[CALL rel32] [MOVAPS xmm, [rip+off32]]");
        if (addrCall) {
          pfnClear = PatternFinder::GetRipAddress(addrCall, 1, 5);
          phase.Step(pfnClear, "ClearLocalVehicles call", "RT");
        }
      }
    }
  }

  // ── Phase 2: ClearLocalVehicles Offsets ──
  {
    auto phase = log.MakePhase("ClearLocalVehicles Offsets");

    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(ClearLocalVehicles[1405568b0]) ---/
    // * 1405568b0  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
    if (pfnClear) {
      // 1. Vehicle Array Offset (MOV REG, [REG + 32-bit displacement])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(ClearLocalVehicles[1405568b0]) ---/
      // * 1405568c0  48 8B 89 F8 00 00 00          MOV RCX,qword ptr [RCX + 0xf8]
      uintptr_t addrArray = PatternFinder::Find(pfnClear, 32, "[MOV r64, [r64+off32]]");
      if (phase.Step(addrArray, "Vehicle Array load", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addrArray + 3);
        if (phase.StepOffset(off, "PArrayObjectOffset", "OFF")) {
          owner.SetPArrayObjectOffset(off);

          // 2. Vehicle Count Offset (The next MOV after the array load)
          // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(ClearLocalVehicles[1405568b0]) ---/
          // * 1405568c7  48 8B 83 00 01 00 00          MOV RAX,qword ptr [RBX + 0x100]
          uintptr_t addrCount = PatternFinder::Find(addrArray + 7, 16, "[MOV r64, [r64+off32]]");
          if (phase.Step(addrCount, "Vehicle Count load", "RT")) {
            int32_t offCount = PatternFinder::ReadInt32(addrCount + 3);
            if (phase.StepOffset(offCount, "VehicleCountOffset", "OFF")) {
              owner.SetVehicleCountOffset(offCount);
            }
          }
        }
      }

      // 3. Vehicle Struct Size (LEA REG, [REG + 8-bit displacement])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(ClearLocalVehicles[1405568b0]) ---/
      // * 1405568eb  48 8D 48 10                   LEA RCX,[RAX + 0x10]
      uintptr_t addrSize = PatternFinder::Find(pfnClear, 64, "[LEA r64, [r64+off8]]");
      if (phase.Step(addrSize, "Vehicle Struct Size LEA", "RT")) {
        int8_t size = PatternFinder::ReadInt8(addrSize + 3);
        if (phase.StepOffset(size, "SpawnedVehicleStructSize", "OFF")) {
          owner.SetSpawnedVehicleStructSize(static_cast<uint8_t>(size));
        }
      }
    }
  }

  // ── Phase 3: Vehicle ID Offset ──
  {
    auto phase = log.MakePhase("Vehicle ID Offset");

    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
    // * 140542503  48 8D 15 0E E1 BE 01          LEA RDX,[0x142130618]
    uintptr_t idStrXref = PatternFinder::FindFunctionByString(VEHICLE_ID_STR_ANCHOR, false);
    if (phase.Step(idStrXref, "ID string XREF", "STR")) {
      // Scan backward for MOV REG, [REG + offset]
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
      // * 1405422f3  44 8B 86 20 04 00 00          MOV R8D,dword ptr [RSI + 0x420]
      uintptr_t addrIdLoad = PatternFinder::FindBackward(idStrXref, 1000, "44 8B [80-BF]");
      if (phase.Step(addrIdLoad, "Vehicle ID load", "RT")) {
        int32_t offset = PatternFinder::ReadInt32(addrIdLoad + 3);
        if (phase.StepOffset(offset, "VehicleIdOffset", "OFF")) {
          owner.SetVehicleIdOffset(offset);
        }
      }
    }
  }

  // ── Phase 4: Local Player Controller Offset ──
  {
    auto phase = log.MakePhase("Local Player Controller Offset");

    uintptr_t addrPlayerLoad = PatternFinder::Find(PLAYER_CONTROLLER_SIG);
    if (phase.Step(addrPlayerLoad, "Player controller sequence", "RT")) {
      int32_t offset = PatternFinder::ReadInt32(addrPlayerLoad + 3);
      if (phase.StepOffset(offset, "LocalPlayerControllerOffset", "OFF")) {
        owner.SetLocalPlayerControllerOffset(offset);
      }
    }
  }

  // ── Phase 5: Player Vehicle Actor Offset ──
  {
    auto phase = log.MakePhase("Player Vehicle Actor Offset");

    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateAllTraffic[140609d30]) ---/
    // * 140609d30  40 55                         PUSH RBP
    const char* SPAWNED_VEHICLE_ARRAY_STR = "??A?$array_t@Uspawned_vehicle_t@traffic_u@prism@@@prism@@QEAAAEAUspawned_vehicle_t@traffic_u@1@_K@Z";
    uintptr_t pfnUpdateAllTraffic = PatternFinder::FindFunctionByString(SPAWNED_VEHICLE_ARRAY_STR, true);
    if (phase.Step(pfnUpdateAllTraffic, "UpdateAllTraffic function", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateAllTraffic[140609d30]) ---/
      // * 140609f39  48 8B 49 48                   MOV RCX,qword ptr [RCX + 0x48]
      // * 140609f3d  48 85 C9                      TEST RCX,RCX
      uintptr_t addrActorLoad = PatternFinder::Find(pfnUpdateAllTraffic, 1024, PLAYER_ACTOR_SIG);
      if (phase.Step(addrActorLoad, "Player vehicle actor load", "RT")) {
        int8_t offset = PatternFinder::ReadInt8(addrActorLoad + 3);
        if (phase.StepOffset(offset, "PlayerVehicleInControllerOffset", "OFF")) {
          owner.SetPlayerVehicleInControllerOffset(offset);
        }
      }
    }
  }

  // ── Phase 6: Detailed Vehicle Property Offsets ──
  {
    auto phase = log.MakePhase("Vehicle Property Offsets");

    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
    // * 140549120  48 85 D2                      TEST RDX,RDX
    uintptr_t pfnFormat = PatternFinder::FindFunctionByString(REMOTE_PLAYER_VEHICLE_STR, true);
    if (phase.Step(pfnFormat, "FormatObjectDebugInfo function", "RT")) {
      // 6.1 Speed Limit (MOVSS XMM, [REG+disp32])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
      // * 14054934a  F3 0F 10 86 30 04 00 00       MOVSS XMM0,dword ptr [RSI + 0x430]
      uintptr_t addrSpeedLimit = PatternFinder::Find(pfnFormat, 1000, "[MOVSS xmm, [r64+off32]]");
      if (phase.Step(addrSpeedLimit, "Speed Limit load", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addrSpeedLimit + 4);
        if (phase.StepOffset(off, "SpeedLimitOffset", "OFF")) {
          owner.SetSpeedLimitOffset(off);

          // 6.2 Patience & Safety (MOVSS XMM, [REG+disp32])
          // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
          // * 1405493a1  F3 44 0F 10 8E 44 04 00 00    MOVSS XMM9,dword ptr [RSI + 0x444]
          uintptr_t addrPatience = PatternFinder::Find(addrSpeedLimit, 128, "F3 44 0F 10 [80-BF]");
          if (phase.Step(addrPatience, "Patience load", "RT")) {
            int32_t offP = PatternFinder::ReadInt32(addrPatience + 5);
            if (phase.StepOffset(offP, "PatienceOffset", "OFF")) {
              owner.SetPatienceOffset(offP);

              // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
              // * 1405493ad  F3 44 0F 10 96 40 04 00 00    MOVSS XMM10,dword ptr [RSI + 0x440]
              uintptr_t addrSafety = PatternFinder::Find(addrPatience + 9, 64, "F3 44 0F 10 [80-BF]");
              if (phase.Step(addrSafety, "Safety load", "RT")) {
                int32_t offS = PatternFinder::ReadInt32(addrSafety + 5);
                if (phase.StepOffset(offS, "SafetyOffset", "OFF")) {
                  owner.SetSafetyOffset(offS);
                }
              }
            }
          }
        }
      }

      // 6.3 Target Speed (MULSS XMM, [REG+disp32])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
      // * 1405493df  F3 0F 59 8E 34 04 00 00       MULSS XMM1,dword ptr [RSI + 0x434]
      uintptr_t addrTarget = PatternFinder::Find(pfnFormat, 0x1000, "F3 0F 59 [80-BF]");
      if (phase.Step(addrTarget, "Target Speed MULSS", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addrTarget + 4);
        if (phase.StepOffset(off, "TargetSpeedOffset", "OFF")) {
          owner.SetTargetSpeedOffset(off);
        }
      }

      // 6.4 Lane Speed Input (MOV REG, [REG+disp32])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
      // * 1405492a4  48 8B 9E 10 04 00 00          MOV RBX,qword ptr [RSI + 0x410]
      uintptr_t addrLane = PatternFinder::Find(pfnFormat, 1000, "48 8B [90-BF]");
      if (phase.Step(addrLane, "Lane Speed Input load", "RT")) {
        int32_t off = PatternFinder::ReadInt32(addrLane + 3);
        if (phase.StepOffset(off, "LaneSpeedInputOffset", "OFF")) {
          owner.SetLaneSpeedInputOffset(off);
        }
      }

      // 6.5 Sub-Object & VTables (MOV REG, [REG+disp8] then LEA REG, [REG+disp8])
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
      // * 1405493ce  48 8B 46 10                   MOV RAX,qword ptr [RSI + 0x10]
      uintptr_t addrSubObj = PatternFinder::Find(pfnFormat, 0x1000, "48 8B [40-7F] ?? 48 8D [40-7F]");
      if (phase.Step(addrSubObj, "Vehicle Sub-Object load", "RT")) {
        int8_t off = PatternFinder::ReadInt8(addrSubObj + 3);
        if (phase.StepOffset(off, "VehicleSubObjectOffset", "OFF")) {
          owner.SetVehicleSubObjectOffset(static_cast<uint8_t>(off));

          // Acceleration VTable (Immediately follows)
          // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
          // * 1405493f3  FF 50 10                      CALL qword ptr [RAX + 0x10]
          uintptr_t addrAccel = PatternFinder::Find(addrSubObj, 64, "[CALL [r64+off8]]");
          if (phase.Step(addrAccel, "Acceleration VTable call", "RT")) {
            int8_t offA = PatternFinder::ReadInt8(addrAccel + 2);
            if (phase.StepOffset(offA, "VtableGetAccelerationOffset", "OFF")) {
              owner.SetVtableGetAccelerationOffset(static_cast<uint8_t>(offA));

              // Speed VTable
              // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FormatObjectDebugInfo[140549120]) ---/
              // * 140549405  FF 50 08                      CALL qword ptr [RAX + 0x8]
              uintptr_t addrSpeed = PatternFinder::Find(addrAccel + 3, 64, "[CALL [r64+off8]]");
              if (phase.Step(addrSpeed, "Speed VTable call", "RT")) {
                int8_t offSp = PatternFinder::ReadInt8(addrSpeed + 2);
                if (phase.StepOffset(offSp, "VtableGetCurrentSpeedOffset", "OFF")) {
                  owner.SetVtableGetCurrentSpeedOffset(static_cast<uint8_t>(offSp));
                }
              }
            }
          }
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetTrafficManagerAddr() != 0 && owner.GetPArrayObjectOffset() != 0 && owner.GetVehicleCountOffset() != 0 && owner.GetSpawnedVehicleStructSize() != 0 && owner.GetVehicleIdOffset() != 0 && owner.GetPatienceOffset() != 0 &&
              owner.GetSafetyOffset() != 0 && owner.GetTargetSpeedOffset() != 0 && owner.GetSpeedLimitOffset() != 0 && owner.GetLaneSpeedInputOffset() != 0 && owner.GetLocalPlayerControllerOffset() != 0 &&
              owner.GetPlayerVehicleInControllerOffset() != 0 && owner.GetVehicleSubObjectOffset() != 0 && owner.GetVtableGetCurrentSpeedOffset() != 0 && owner.GetVtableGetAccelerationOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
