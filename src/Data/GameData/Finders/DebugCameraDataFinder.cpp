#include "SPF/Data/GameData/Finders/DebugCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {

// String anchor for the SetDebugCameraMode function.
// This function updates the camera mode and logs the change to the console.
// * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
// * 140544450  48 8D 0D 79 C5 BE 01          LEA RCX,[0x1421309d0]
const char* DEBUG_MODE_SET_STR = "Camera mode set to '%s'";

// String anchor for the DebugCamera_SetHUDVisibility function.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDVisibility[140541690]) ---/
//* 1405416fc  48 8D 05 85 E7 BE 01          LEA RAX,[0x14212fe88] = "debug_camera_hud"
const char* DEBUG_HUD_SCRIPT_STR = "debug_camera_hud";

// String anchor for the DebugCamera_HandleInput function.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
//* 14053ef66  48 8D 0D 6B 0D BF 01          LEA RCX,[0x14212fcd8]
const char* DEBUG_ROLL_SPEED_STR = "Camera roll speed: %.2f";

// String anchor for the DebugCamera_SetSelectedActor function.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetSelectedActor[1405441e0]) ---/
//* 14054424f  48 8D 0D 7A C6 BE 01          LEA RCX,[0x1421308d0] = "Selected actor: ID: %u"
const char* SELECTED_ACTOR_STR = "Selected actor: ID: %u";

// String anchor for the Position Lock toggle log.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
//* 140542a47  48 8D 15 D2 D7 BE 01          LEA RDX,[0x142130220] = "<color value=%02X00ffff>po.."
const char* POS_LOCK_STR = "<color value=%02X00ffff>pos lock";

// String anchor for the Orbit Mode toggle log.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
//* 140542b77  48 8D 15 FA D9 BE 01          LEA RDX,[0x142130578] = "<color value=%02X00ffff>or.."
const char* ORBIT_STR = "<color value=%02X00ffff>orbit";

// String anchor for the Orbit Speed info log.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
//* 140542c42  48 8D 15 CF D8 BE 01          LEA RDX,[0x142130518] = "<color value=ff00ffff>%.2..."
const char* ORBIT_SPEED_STR = "<color value=ff00ffff>%.2f/%.2f";

// String anchor for the Hovered Object (Actor) type.
//* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
//* 140542828  48 8D 05 B1 DB BE 01          LEA RAX,[0x1421303e0] = "Actor"
const char* JOB_TRAILER_STR = "Job trailer";

}  // namespace

bool DebugCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  Utils::FinderLog log(GetName());
  log.Info("Searching for Debug Camera data (Dynamic Search)...");

  bool all_found = true;

  // ── Phase 1: SetDebugCameraMode Function & CVar ──
  // 1.1 Find SetDebugCameraMode function
  {
    auto phase = log.MakePhase("SetDebugCameraMode & CVar");

    /*
     * Ghidra Reference (SetDebugCameraMode v1.60+ @ 1405443a0):
     * 1405444a0 48 8d 0d 79 c5 be 01    LEA RCX,[s_Camera_mode_set_to_'%s'_142130a20]
     */
    uintptr_t pfnSetMode = Utils::PatternFinder::FindFunctionByString(DEBUG_MODE_SET_STR, true);
    if (phase.Step(pfnSetMode, "SetDebugCameraMode", "FN")) {
      owner.SetDebugCameraModeFunc(reinterpret_cast<void*>(pfnSetMode));

      // 1.1.0 Find Debug Camera Mode offset inside SetDebugCameraMode
      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
       * 14054435f  39 91 5C 04 00 00             CMP dword ptr [RCX + 0x45c],EDX
       */
      uintptr_t addrModeSet = Utils::PatternFinder::Find(pfnSetMode, 32, "[CMP [r64+off32], r32]");
      if (phase.StepOptional(addrModeSet, "Debug Camera Mode offset anchor", "RT")) {
        int32_t offset = Utils::PatternFinder::ReadInt32(addrModeSet + 2);
        if (phase.StepOffsetOptional(offset, "Debug Camera Mode offset", "OFF")) {
          owner.SetDebugCameraModeOffset(offset);
        }
      }

      // 1.1.1 Find CVar object and GetAndCacheValue function inside SetDebugCameraMode
      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
       * 140544370  48 8D 0D E9 E1 6A 02          LEA RCX,[0x142bf2560]
       */
      uintptr_t cvarScan = Utils::PatternFinder::Find(pfnSetMode, 128, "[LEA r64, [rip+off32]]");
      if (phase.Step(cvarScan, "CVar object LEA anchor", "RT")) {
        uintptr_t base_ptr = Utils::PatternFinder::GetRipAddress(cvarScan, 3, 7);
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetDebugCameraMode[140544350]) ---/
        // * 140544377  E8 84 3C C8 FF                CALL 0x1401c8000
        uintptr_t getAndCacheScan = Utils::PatternFinder::Find(cvarScan, 32, "[CALL rel32]");
        uintptr_t pfnGetAndCache = Utils::PatternFinder::GetRipAddress(getAndCacheScan, 1, 5);

        if (phase.Step(base_ptr, "Cacheable CVar object", "PTR") && phase.Step(pfnGetAndCache, "GetAndCacheValue", "FN")) {
          owner.SetCacheableCvarObjectPtr(base_ptr);

          // 1.1.2 Find the internal value offset within GetAndCacheValue function
          /*
           * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetAndCacheValue[1401c8000]) ---/
           * 1401c80b7  89 87 18 01 00 00             MOV dword ptr [RDI + 0x118],EAX
           * 1401c80bd  C6 87 16 01 00 00 01          MOV byte ptr [RDI + 0x116],0x1
           */
          uintptr_t sig_off = Utils::PatternFinder::Find(pfnGetAndCache, 256, "[MOV [r64+off32], r32] [MOV byte ptr [r64+off32], imm8]");
          if (phase.Step(sig_off, "CVar value offset anchor", "RT")) {
            int32_t offset = Utils::PatternFinder::ReadInt32(sig_off + 2);
            if (phase.StepOffset(offset, "CVar value offset", "OFF")) {
              owner.SetCvarValueOffset(offset);
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

  // ── Phase 2: Game UI Visible Offset ──
  // 1.2 Find Game UI Visible (Clean UI) offset
  {
    auto phase = log.MakePhase("Game UI Visible");

    /*
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
     * 14053ed1b  88 86 58 04 00 00             MOV byte ptr [RSI + 0x458],AL
     * 14053ed21  F2 0F 10 05 DF A1 FF 02       MOVSD XMM0,qword ptr [0x143538f08]
     */
    uintptr_t rollStrAddr = Utils::PatternFinder::FindFunctionByString(DEBUG_ROLL_SPEED_STR, false);
    if (phase.Step(rollStrAddr, "Roll speed string usage", "RT")) {
      uintptr_t addrUI = Utils::PatternFinder::FindBackward(rollStrAddr, 1024, "[MOV [r64+off32], r8] [MOVSD xmm, [rip+off32]]");
      if (phase.Step(addrUI, "Game UI anchor", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(addrUI + 2);
        if (phase.StepOffset(off, "Game UI visible offset", "OFF")) {
          owner.SetGameUiVisibleOffset(off);
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

  // ── Phase 3: Lock & Orbit Toggle Functions ──
  // 1.2.2 Find PosLock, RotLock, and OrbitMode functions and offsets
  {
    auto phase = log.MakePhase("Lock & Orbit Toggle Functions");

    uintptr_t rollStrAddr = Utils::PatternFinder::FindFunctionByString(DEBUG_ROLL_SPEED_STR, false);
    if (phase.Step(rollStrAddr, "Roll speed string usage", "RT")) {
      /*
       * Logic:
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
       * 14053f112  44 38 AE E8 03 00 00          CMP byte ptr [RSI + 0x3e8],R13B ... / CALL SetPositionLock
       * 14053f136  44 38 AE E9 03 00 00          CMP byte ptr [RSI + 0x3e9],R13B / ... / CALL SetRotationLock
       * 14053f15a  44 38 AE 25 04 00 00          CMP byte ptr [RSI + 0x425],R13B / ... / CALL SetOrbitMode
       *
       * All use the same structural pattern: 44 [CMP [r64+off32], r8]
       */
      const char* togglePattern = "44 [CMP [r64+off32], r8]";

      // --- a. SetPositionLock ---
      uintptr_t posLockCall = Utils::PatternFinder::Find(rollStrAddr, 2048, togglePattern);
      if (phase.Step(posLockCall, "SetPositionLock call site", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(posLockCall + 3);
        // Locate the specific SETZ + CALL sequence within the matched block to find the function address.
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053f11f  E8 BC 3C 00 00                CALL 0x140542de0
        // * 14053f124  48 8D 8E 50 09 00 00          LEA RCX,[RSI + 0x950]
        uintptr_t addrSetzCall = Utils::PatternFinder::Find(posLockCall, 32, "[CALL rel32] [LEA r64, [r64+off32]]");
        uintptr_t pfn = Utils::PatternFinder::GetRipAddress(addrSetzCall, 1, 5);

        if (phase.Step(pfn, "SetPositionLock", "FN")) {
          if (phase.StepOffset(off, "Pos lock offset", "OFF")) {
            owner.SetDebugPosLockOffset(off);
            owner.SetSetPositionLockFunc(reinterpret_cast<void*>(pfn));
          } else {
            all_found = false;
          }
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // --- b. SetRotationLock ---
      uintptr_t rotLockCall = Utils::PatternFinder::Find(posLockCall + 7, 64, togglePattern);
      if (phase.Step(rotLockCall, "SetRotationLock call site", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(rotLockCall + 3);
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053f143  E8 58 3E 00 00                CALL 0x140542fa0
        // * 14053f148  48 8D 8E 70 0A 00 00          LEA RCX,[RSI + 0xa70]
        uintptr_t addrSetzCall = Utils::PatternFinder::Find(rotLockCall, 32, "[CALL rel32] [LEA r64, [r64+off32]]");
        uintptr_t pfn = Utils::PatternFinder::GetRipAddress(addrSetzCall, 1, 5);

        if (phase.Step(pfn, "SetRotationLock", "FN")) {
          if (phase.StepOffset(off, "Rot lock offset", "OFF")) {
            owner.SetDebugRotLockOffset(off);
            owner.SetSetRotationLockFunc(reinterpret_cast<void*>(pfn));
          } else {
            all_found = false;
          }
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // --- c. SetOrbitMode ---
      uintptr_t orbitCall = Utils::PatternFinder::Find(rotLockCall + 7, 64, togglePattern);
      if (phase.Step(orbitCall, "SetOrbitMode call site", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(orbitCall + 3);
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053f167  E8 74 3E 00 00                CALL 0x140542fe0
        // * 14053f16c  48 8D 8E 80 09 00 00          LEA RCX,[RSI + 0x980]
        uintptr_t addrSetzCall = Utils::PatternFinder::Find(orbitCall, 32, "[CALL rel32] [LEA r64, [r64+off32]]");
        uintptr_t pfn = Utils::PatternFinder::GetRipAddress(addrSetzCall, 1, 5);

        if (phase.Step(pfn, "SetOrbitMode", "FN")) {
          if (phase.StepOffset(off, "Orbit offset", "OFF")) {
            owner.SetDebugOrbitOffset(off);
            owner.SetSetOrbitModeFunc(reinterpret_cast<void*>(pfn));
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

  // ── Phase 4: SetSelectedActor Function ──
  // 2.1 Find DebugCamera_SetSelectedActor function
  {
    auto phase = log.MakePhase("SetSelectedActor");

    /*
     * Logic:
     * 1. Find the unique log string "Selected actor: ID: %u".
     * 2. Find the instruction that loads this string (XREF).
     * 3. Trace back to the start of the function (DebugCamera_SetSelectedActor).
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetSelectedActor[1405441e0]) ---/
     * 14054424f  48 8D 0D 7A C6 BE 01          LEA RCX,[0x1421308d0] = "Selected actor: ID: %u"
     */
    uintptr_t pfnSelected = Utils::PatternFinder::FindFunctionByString(SELECTED_ACTOR_STR, true);
    if (phase.Step(pfnSelected, "SetSelectedActor", "FN")) {
      owner.SetSetSelectedActorFunc(reinterpret_cast<void*>(pfnSelected));
    } else {
      all_found = false;
    }
  }

  // ── Phase 5: Pos/Rot Lock Offsets ──
  // 2.2 Find Pos/Rot Lock offsets (0x3E8/0x3E9) via string anchor
  {
    auto phase = log.MakePhase("Pos/Rot Lock Offsets");

    /*
     * Logic:
     * 1. Find the local usage of string "<color value=%02X00ffff>pos lock" inside HandleInput.
     * 2. Search backward for the CMP instruction that checks the lock state.
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
     * 140542a47  48 8D 15 D2 D7 BE 01          LEA RDX,[0x142130220] = "<color value=%02X00ffff>po.."
     */
    uintptr_t posLockUsage = Utils::PatternFinder::FindFunctionByString(POS_LOCK_STR, false);
    if (phase.Step(posLockUsage, "Pos lock string usage", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
      // * 140542a40  45 38 A6 E8 03 00 00          CMP byte ptr [R14 + 0x3e8],R12B
      uintptr_t addrLock = Utils::PatternFinder::FindBackward(posLockUsage, 64, "45 [CMP [r64+off32], r8]");
      if (phase.Step(addrLock, "CMP anchor", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(addrLock + 3);
        if (phase.StepOffset(off, "Pos/Rot lock offsets", "OFF")) {
          owner.SetDebugPosLockOffset(off);
          owner.SetDebugRotLockOffset(off + 1);
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

  // ── Phase 6: Orbit Offset ──
  // 2.4 Find Orbit offset (0x425) via string anchor
  {
    auto phase = log.MakePhase("Orbit Offset");

    /*
     * Logic:
     * 1. Find the local usage of string "<color value=%02X00ffff>orbit" inside HandleInput.
     * 2. Search backward for the CMP instruction that checks the orbit state.
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
     * 140542b77  48 8D 15 FA D9 BE 01          LEA RDX,[0x142130578] = "<color value=%02X00ffff>or.."
     */
    uintptr_t orbitUsage = Utils::PatternFinder::FindFunctionByString(ORBIT_STR, false);
    if (phase.Step(orbitUsage, "Orbit string usage", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
      // * 140542b70  45 38 A6 25 04 00 00          CMP byte ptr [R14 + 0x425],R12B
      uintptr_t addrOrbit = Utils::PatternFinder::FindBackward(orbitUsage, 64, "45 [CMP [r64+off32], r8]");
      if (phase.Step(addrOrbit, "CMP anchor", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(addrOrbit + 3);
        if (phase.StepOffset(off, "Orbit offset", "OFF")) {
          owner.SetDebugOrbitOffset(off);
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

  // ── Phase 7: Selected Object & Orbit Speed Offsets ──
  // 2.5 Find Selected Object (0x4A8) and Orbit Speed (0x420) offsets via string anchor
  {
    auto phase = log.MakePhase("Selected Object & Orbit Speed");

    /*
     * Logic:
     * 1. Find the local usage of string "<color value=ff00ffff>%.2f/%.2f" inside HandleInput.
     * 2. Search backward for the instruction sequence that loads the object and speed.
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
     * 140542c42  48 8D 15 CF D8 BE 01          LEA RDX,[0x142130518] = "<color value=ff00ffff>%.2..."
     */
    uintptr_t orbitSpeedUsage = Utils::PatternFinder::FindFunctionByString(ORBIT_SPEED_STR, false);
    if (phase.Step(orbitSpeedUsage, "Orbit speed string usage", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
      // * 140542c1a  49 8B 8E A8 04 00 00          MOV RCX,qword ptr [R14 + 0x4a8]
      uintptr_t addr = Utils::PatternFinder::FindBackward(orbitSpeedUsage, 128, "[MOV r64, [r64+off32]]");
      if (phase.Step(addr, "MOV anchor", "RT")) {
        int32_t offSel = Utils::PatternFinder::ReadInt32(addr + 3);
        int32_t offSpd = Utils::PatternFinder::ReadInt32(addr + 12);
        if (Utils::PatternFinder::IsSaneOffset(offSel) && Utils::PatternFinder::IsSaneOffset(offSpd)) {
          owner.SetDebugSelectedObjectPtrOffset(offSel);
          owner.SetDebugOrbitSpeedOffset(offSpd);
          phase.StepOffset(offSel, "Selected object offset", "OFF");
          phase.StepOffset(offSpd, "Orbit speed offset", "OFF");
        } else {
          log.Error("SelectedObj/Speed offset INVALID (0x{:X}/0x{:X})", offSel, offSpd);
          all_found = false;
        }
      } else {
        all_found = false;
      }
    } else {
      all_found = false;
    }
  }

  // ── Phase 8: Hovered Object Offset ──
  // 2.6 Find Hovered Object (Actor) offset (0x4A0) via string anchor
  {
    auto phase = log.MakePhase("Hovered Object");

    /*
     * Logic:
     * 1. Find the local usage of string "Actor" inside HandleInput.
     * 2. Search backward for the CMP instruction that checks the hovered object state.
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
     * 14054283b  48 8D 05 66 DC BE 01          LEA RAX,[0x1421304a8] = "Job trailer"
     */
    uintptr_t actorUsage = Utils::PatternFinder::FindFunctionByString(JOB_TRAILER_STR, false);
    if (phase.Step(actorUsage, "Job trailer string usage", "RT")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_RenderInfoOverlay[140541c50]) ---/
      // * 14054281e  49 83 BE A0 04 00 00 00       CMP qword ptr [R14 + 0x4a0],0x0
      // * 140542826  74 09                         JZ 0x140542831
      // * 140542828  48 8D 05 B1 DB BE 01          LEA RAX,[0x1421303e0]
      // * 14054282f  EB 31                         JMP 0x140542862
      // * 140542831  49 83 BE B0 04 00 00 00       CMP qword ptr [R14 + 0x4b0],0x0
      uintptr_t addrHover = Utils::PatternFinder::FindBackward(actorUsage, 64, "49 83 BE ? ? ? ? 00 [JE rel8] [LEA r64, [rip+off32]] [JMP rel8] 49 83 BE");
      if (phase.Step(addrHover, "CMP Job trailer", "RT")) {
        int32_t off = Utils::PatternFinder::ReadInt32(addrHover + 3);
        if (phase.StepOffset(off, "Hovered object offset", "OFF")) {
          owner.SetDebugHoveredObjectPtrOffset(off);
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

  // ── Phase 9: HUD Functions ──
  // 2.3 Find DebugCamera_SetHUDVisibility and DebugCamera_SetHUDPosition functions
  {
    auto phase = log.MakePhase("HUD Functions");

    /*
     * Logic:
     * 1. Find the unique string anchor "debug_camera_hud".
     * 2. Find the instruction that loads this string (XREF).
     * 3. Trace back to the start of the function (DebugCamera_SetHUDVisibility).
     *
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDVisibility[140541690]) ---/
     * 1405416fc  48 8D 05 85 E7 BE 01          LEA RAX,[0x14212fe88] = "debug_camera_hud"
     */
    uintptr_t pfnSetHudVis = Utils::PatternFinder::FindFunctionByString(DEBUG_HUD_SCRIPT_STR, true);
    if (phase.Step(pfnSetHudVis, "SetHudVisibility", "FN")) {
      owner.SetSetHudVisibilityFunc(reinterpret_cast<void*>(pfnSetHudVis));

      // 2.3 Find DebugCamera_SetHUDPosition function
      /*
       * Logic:
       * Inside SetHUDVisibility, there is a sequence of two calls:
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDVisibility[140541690]) ---/
       * 1405417a8  E8 33 15 00 00                CALL 0x140542ce0
       * 1405417ad  48 8B CB                      MOV RCX,RBX
       * 1405417b0  E8 9B 04 00 00                CALL 0x140541c50
       */
      uintptr_t callSite = Utils::PatternFinder::Find(pfnSetHudVis, 512, "[CALL rel32] [MOV r64, r64] [CALL rel32]");
      if (phase.StepOptional(callSite, "SetDebugHudPosition call site", "RT")) {
        uintptr_t pfnSetHudPos = Utils::PatternFinder::GetRipAddress(callSite, 1, 5);
        if (phase.Step(pfnSetHudPos, "SetDebugHudPosition", "FN")) {
          owner.SetSetDebugHudPositionFunc(reinterpret_cast<void*>(pfnSetHudPos));

          // 2.3.1 Find HUD Position Offset (0x454) inside SetDebugHudPosition
          //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDPosition[140542ce0]) ---/
          //* 140542cff  44 8B 8B 54 04 00 00          MOV R9D,dword ptr [RBX + 0x454]
          uintptr_t addrPos = Utils::PatternFinder::Find(pfnSetHudPos, 64, "44 [MOV r32, [r64+off32]]");
          if (phase.StepOptional(addrPos, "HUD position anchor", "RT")) {
            int32_t off = Utils::PatternFinder::ReadInt32(addrPos + 3);
            if (phase.StepOffsetOptional(off, "HUD position offset", "OFF")) {
              owner.SetHudPositionOffset(off);
            }
          }

          // 2.3.2 Find HUD Visibility Object Offset (0x538) inside SetDebugHudPosition
          //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDPosition[140542ce0]) ---/
          //* 140542cef  48 8B 89 38 05 00 00          MOV RCX,qword ptr [RCX + 0x538]
          uintptr_t addrVis = Utils::PatternFinder::Find(pfnSetHudPos, 32, "[MOV r64, [r64+off32]]");
          if (phase.StepOptional(addrVis, "HUD visibility object anchor", "RT")) {
            int32_t off = Utils::PatternFinder::ReadInt32(addrVis + 3);
            phase.StepOffsetOptional(off, "HUD visibility object offset", "OFF");
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

  // ── Phase 10: HUD Offsets via Call Sites ──
  // Find HUD Visibility
  {
    auto phase = log.MakePhase("HUD Call-Site Offsets");

    /*
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDVisibility[140541690]) ---/
     * 140541690  48 89 5C 24 10                MOV qword ptr [RSP + 0x10],RBX
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_SetHUDPosition[140542ce0]) ---/
     * 140542ce0  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
     */
    uintptr_t pfnSetHudVis = reinterpret_cast<uintptr_t>(owner.GetSetHudVisibilityFunc());
    uintptr_t pfnSetHudPos = reinterpret_cast<uintptr_t>(owner.GetSetDebugHudPositionFunc());

    if (phase.StepOptional(pfnSetHudVis, "SetHudVisibility", "FN") && phase.StepOptional(pfnSetHudPos, "SetDebugHudPosition", "FN")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
      // * 1405415c9  E8 C2 00 00 00                CALL 0x140541690
      auto visXrefs = Utils::PatternFinder::FindCallXrefs(pfnSetHudVis);
      if (visXrefs.empty()) {
        log.Error("HUD Vis: SetHudVisibility has NO call sites in the module");
        all_found = false;
      } else {
        bool visOffsetFound = false;
        for (uintptr_t xref : visXrefs) {
          // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
          // * 1405415bd  38 9E 40 05 00 00             CMP byte ptr [RSI + 0x540],BL
          // * 1405415c3  48 8B CE                      MOV RCX,RSI
          uintptr_t anchor = Utils::PatternFinder::FindBackward(xref, 32, "[CMP [r64+off32], r8] [MOV r64, r64]");
          if (!anchor) continue;
          if (Utils::PatternFinder::GetRipAddress(anchor + 12, 1, 5) != pfnSetHudVis) continue;
          int32_t off = Utils::PatternFinder::ReadInt32(anchor + 2);
          if (!Utils::PatternFinder::IsSaneOffset(off)) {
            log.Error("HUD Vis: offset 0x{:X} INVALID (anchor 0x{:X})", off, anchor);
            continue;
          }
          owner.SetHudVisibleOffset(off);
          visOffsetFound = true;
          log.Info("--- Found HUD Visibility offset: 0x{:X} (call 0x{:X})", off, xref);
          break;
        }
        if (!visOffsetFound) {
          log.Error("FAILED to find HUD Visible offset at SetHudVisibility call sites");
          all_found = false;
        }
      }

      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
      // * 1405415f0  E8 EB 16 00 00                CALL 0x140542ce0
      auto posXrefs = Utils::PatternFinder::FindCallXrefs(pfnSetHudPos);
      if (posXrefs.empty()) {
        log.Error("HUD Pos: SetDebugHudPosition has NO call sites in the module");
        all_found = false;
      } else {
        bool posOffsetFound = false;
        for (uintptr_t xref : posXrefs) {
          //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
          // * 1405415e2  8B 96 54 04 00 00             MOV EDX,dword ptr [RSI + 0x454]
          // * 1405415e8  48 8B CE                      MOV RCX,RSI
          uintptr_t anchor = Utils::PatternFinder::FindBackward(xref, 32, "[MOV r32, [r64+off32]] [MOV r64, r64]");
          if (!anchor) continue;
          if (Utils::PatternFinder::GetRipAddress(anchor + 14, 1, 5) != pfnSetHudPos) continue;
          int32_t off = Utils::PatternFinder::ReadInt32(anchor + 2);
          if (!Utils::PatternFinder::IsSaneOffset(off)) {
            log.Error("HUD Pos: offset 0x{:X} INVALID (anchor 0x{:X})", off, anchor);
            continue;
          }
          owner.SetHudPositionOffset(off);
          posOffsetFound = true;
          log.Info("--- Found HUD Position offset: 0x{:X} (call 0x{:X})", off, xref);
          break;
        }
        if (!posOffsetFound) {
          log.Error("FAILED to find HUD Position offset at SetDebugHudPosition call sites");
          all_found = false;
        }
      }
    } else {
      log.Error("HUD: SetHudVisibility(0x{:X}) or SetDebugHudPosition(0x{:X}) not found", pfnSetHudVis, pfnSetHudPos);
      all_found = false;
    }
  }

  // ── Phase 11: Debug Camera Context ──
  // 3. Find the pDebugCamera context pointer dynamically
  {
    auto phase = log.MakePhase("Debug Camera Context");

    auto& cameraHooks = Hooks::CameraHooks::GetInstance();
    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
    // * 1404f0190  48 83 EC 48                   SUB RSP,0x48
    uintptr_t pfnGetCamObj = reinterpret_cast<uintptr_t>(cameraHooks.GetGetCameraObjectFunc());

    // GetCameraManager() handles the pointer dereferencing and version-specific adjustments (like v1.59).
    uintptr_t pStandardManager = owner.GetCameraManager();

    if (phase.Step(pStandardManager, "Camera manager", "RT") && phase.Step(pfnGetCamObj, "GetCameraObjectByID", "FN")) {
      /*
       * HOW-TO-FIND Camera Array Offset:
       * We look inside GetCameraObjectByID function.
       * It adds a base offset to the Camera Manager and then reads the array pointer.

       * STRUCTURE OF THE CAMERA ARRAY (pDebugCameraContext at StandardManager + 0x38):
       * This is a pointer to an array of pointers (context) managed by the game engine.
       * Each offset corresponds to a GameCameraType ID:
       *
       * +0x00: DeveloperFreeCamera (ID 0)
       * +0x08: BehindCamera        (ID 1)
       * +0x10: InteriorCamera      (ID 2)
       * +0x18: BumperCamera        (ID 3)
       * +0x20: WindowCamera        (ID 4)
       * +0x28: CabinCamera         (ID 5)
       * +0x30: WheelCamera         (ID 6)
       * +0x38: TopCamera           (ID 7)
       * +0x40: ???                 (ID 8)
       * +0x48: TVCamera            (ID 9)
       * +0x50: ???                 (ID 10)
       * +0x58: ???                 (ID 11)
       * +0x60: ???                 (ID 12)
       * +0x68: PhotoCamera         (ID 13)
       */

      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
       * 1404f01a4  48 83 C1 30                   ADD RCX,0x30
       * 1404f01a8  4C 3B 41 10                   CMP R8,qword ptr [RCX + 0x10]
       */
      const char* p_array_logic = "[ADD r64, imm8] [CMP r64, [r64+off8]]";
      uintptr_t addr = Utils::PatternFinder::Find(pfnGetCamObj, 64, p_array_logic);

      if (phase.Step(addr, "Camera array base logic", "RT")) {
        int8_t baseOff = Utils::PatternFinder::ReadInt8(addr + 3);

        /*
         * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
         * 1404f01ae  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
         */
        uintptr_t addrSub = Utils::PatternFinder::Find(addr, 16, "[MOV r64, [r64+off8]]");
        if (phase.Step(addrSub, "Camera array sub logic", "RT")) {
          int8_t subOff = Utils::PatternFinder::ReadInt8(addrSub + 3);
          int32_t finalOffset = static_cast<int32_t>(baseOff) + static_cast<int32_t>(subOff);

          uintptr_t pDebugCameraContext = *reinterpret_cast<uintptr_t*>(pStandardManager + finalOffset);
          if (phase.Step(pDebugCameraContext, "pDebugCameraContext", "PTR")) {
            owner.SetDebugCameraContextPtr(pDebugCameraContext);
          } else {
            log.Error("pDebugCameraContext is NULL at 0x{:X}", pStandardManager + finalOffset);
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

  // --- Final Readiness Check ---
  m_isReady = all_found && owner.GetDebugCameraModeFunc() != nullptr && owner.GetCacheableCvarObjectPtr() != 0 && owner.GetCvarValueOffset() != 0 && owner.GetGameUiVisibleOffset() != 0 && owner.GetSetPositionLockFunc() != nullptr &&
              owner.GetSetRotationLockFunc() != nullptr && owner.GetSetOrbitModeFunc() != nullptr && owner.GetSetSelectedActorFunc() != nullptr && owner.GetDebugPosLockOffset() != 0 && owner.GetDebugRotLockOffset() != 0 &&
              owner.GetDebugOrbitOffset() != 0 && owner.GetDebugSelectedObjectPtrOffset() != 0 && owner.GetDebugOrbitSpeedOffset() != 0 && owner.GetDebugHoveredObjectPtrOffset() != 0 && owner.GetSetHudVisibilityFunc() != nullptr &&
              owner.GetSetDebugHudPositionFunc() != nullptr && owner.GetHudVisibleOffset() != 0 && owner.GetHudPositionOffset() != 0 && owner.GetDebugCameraContextPtr() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
