#include "SPF/Data/GameData/Finders/FreeCameraDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {
/**
 * @brief Signature for the dynamic CVar value offset.
 * Search range: Inside the GetAndCache function body.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetAndCache[1401d7480]) ---/
 * 1401d7521  C6 83 16 01 00 00 01          MOV byte ptr [RBX + 0x116],0x1
 * 1401d7528  F3 0F 11 83 18 01 00 00       MOVSS dword ptr [RBX + 0x118],XMM0
 */
const char* CVAR_VAL_OFFSET_SIG = "[MOV byte ptr [r64+off32], imm8] [MOVSS [r64+off32], xmm]";

/**
 * @brief String anchor used to find the g_flyspeed CVar object via XREFs.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
 * 14053ea95  48 8D 0D 24 12 BF 01          LEA RCX,[0x14212fcc0] = "Camera speed: %.2f"
 */
const char* FLY_SPEED_STRING = "Camera speed: %.2f";

/*
 * Anchor #3: Freecam Move Function (Position & Quaternions)
 * We search for the unique sequence inside Freecam_Move that reads local coordinates.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(Freecam_Move[140771140]) ---/
 * 14077117a  41 0F 10 5A 40                MOVUPS XMM3,xmmword ptr [R10 + 0x40]
 * 14077117f  41 BB 01 00 00 00             MOV R11D,0x1
 * 140771185  45 33 C9                      XOR R9D,R9D
 * 140771188  0F 11 5C 24 20                MOVUPS xmmword ptr [RSP + 0x20],XMM3
 * 14077118d  F3 0F 10 4C 24 24             MOVSS XMM1,dword ptr [RSP + 0x24]
 */
const char* FREECAM_POS_SIG = "41 [MOVUPS xmm, [r64+off8]] [MOV r32, imm32] 45 [XOR r32, r32] [MOVUPS [r64+off8], xmm] [MOVSS xmm, [r64+off8]]";
}  // namespace

bool FreeCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());
  log.Info("Searching for Free Camera data...");

  // ── Phase 1: Fly Speed (g_flyspeed CVar) ──
  /*
   * NEW LOGIC: String Anchor -> XREF -> Backward Scan.
   * This is much more robust than global LEA scans.
   * Logic:
   * 1. Find the unique log string "Camera speed: %.2f".
   * 2. Find the instruction that loads this string (XREF).
   * 3. From that XREF, scan BACKWARDS to find the CVar object and the caching function.
   */
  {
    auto phase = log.MakePhase("Fly Speed (g_flyspeed CVar)");

    uintptr_t strAddr = PatternFinder::FindString(FLY_SPEED_STRING);
    if (phase.Step(strAddr, "Fly Speed string", "REF")) {
      auto xrefs = PatternFinder::FindXrefs(strAddr);
      uintptr_t xrefAddr = !xrefs.empty() ? xrefs[0] : 0;
      if (phase.Step(xrefAddr, "Fly Speed string XREF", "REF")) {
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053ea95  48 8D 0D 24 12 BF 01          LEA RCX,[0x14212fcc0]
        // 1.1 Find the CVar object (LEA backward from string xref)
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053ea86  48 8D 0D 53 38 6B 02          LEA RCX,[0x142bf22e0]
        uintptr_t objectLea = PatternFinder::FindBackward(xrefAddr - 1, 32, "[LEA r64, [rip+off32]]");

        // 1.2 Find the CVar cache function (CALL backward from string xref)
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
        // * 14053ea8d  E8 EE 89 C9 FF                CALL 0x1401d7480
        uintptr_t callAddr = PatternFinder::FindBackward(xrefAddr - 1, 32, "[CALL rel32]");

        bool okLea = phase.Step(objectLea, "CVar object LEA", "RT");
        bool okCall = phase.Step(callAddr, "GetAndCache CALL", "RT");
        if (okLea && okCall) {
          uintptr_t pCVarObjPtrAddr = PatternFinder::GetRipAddress(objectLea, 3, 7);
          uintptr_t pfnGetAndCache = PatternFinder::GetRipAddress(callAddr, 1, 5);

          bool okObjPtr = phase.Step(pCVarObjPtrAddr, "CVar object ptr", "DATA");
          bool okCache = phase.Step(pfnGetAndCache, "GetAndCache", "FN");
          if (okObjPtr && okCache) {
            // 1.3 Resolve the internal value offset from the GetAndCache function body
            // We search for the MOVSS instruction that writes the float value.
            uintptr_t valWriteAddr = PatternFinder::Find(pfnGetAndCache, 256, CVAR_VAL_OFFSET_SIG);
            if (phase.Step(valWriteAddr, "CVar value write", "RT")) {
              // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetAndCache[1401d7480]) ---/
              // * 1401d7528  F3 0F 11 83 18 01 00 00       MOVSS dword ptr [RBX + 0x118],XMM0
              uintptr_t addrF3 = PatternFinder::Find(valWriteAddr, 15, "[MOVSS [r64+off32], xmm]");
              int32_t valOffset = PatternFinder::ReadInt32(addrF3 + 4);

              // The address resolved from LEA is already the object base.
              // We do NOT dereference it.
              uintptr_t pCVarObj = pCVarObjPtrAddr;
              if (phase.StepOffset(valOffset, "CVar value offset", "OFF") && pCVarObj) {
                float* pFlySpeed = reinterpret_cast<float*>(pCVarObj + valOffset);
                owner.SetFlySpeedPtr(pFlySpeed);
              }
            }
          }
        }
      }
    }
  }

  // ── Phase 2: Freecam Global Context ──
  /*
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
   * 140852380  48 8B C4                      MOV RAX,RSP
   */
  {
    auto phase = log.MakePhase("Freecam Global Context");

    uintptr_t funcStart = PatternFinder::FindFunctionByString("[used_vehicles] %Iu used truck offers have expired (%Iu offers valid)", true);
    if (phase.Step(funcStart, "UpdateGameSession", "FN")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
      // * 14085238c  48 8B 3D AD 28 D0 02          MOV RDI,qword ptr [0x143554c40]
      uintptr_t movAddr = PatternFinder::Find(funcStart, 64, "[MOV r64, [rip+off32]]");
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateGameSession[140852380]) ---/
      // * 1408523a2  48 83 BF A8 31 00 00 00       CMP qword ptr [RDI + 0x31a8],0x0
      uintptr_t cmpAddr = PatternFinder::Find(movAddr ? movAddr + 3 : funcStart, 64, "48 83 BF ? ? ? ? 00");

      bool okMov = phase.Step(movAddr, "GameplayManager slot MOV", "RT");
      bool okCmp = phase.Step(cmpAddr, "Context offset CMP", "RT");
      if (okMov && okCmp) {
        // The MOV at movAddr loads the GameplayManager slot (FreecamGlobalObjectPtr) —
        // the same slot that ManagerCoreDataFinder resolves. It is used here only as a
        // positional anchor for the Context Offset CMP below.
        // TODO: Research — replace this anchor with an independent signature so the
        // GameplayManager search is not duplicated across finders.
        int32_t contextOff = PatternFinder::ReadInt32(cmpAddr + 3);

        if (phase.StepOffset(contextOff, "FreecamContextOffset", "OFF")) {
          owner.SetFreecamContextOffset(contextOff);
        }
      }
    }
  }

  // ── Phase 3: Position & Quaternion Offsets ──
  {
    auto phase = log.MakePhase("Position & Quaternion Offsets");

    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(Freecam_Move[140771140]) ---/
    // * 14077117a  41 0F 10 5A 40                MOVUPS XMM3,xmmword ptr [R10 + 0x40]
    // * 14077117f  41 BB 01 00 00 00             MOV R11D,0x1
    // * 140771185  45 33 C9                      XOR R9D,R9D
    // * 140771188  0F 11 5C 24 20                MOVUPS xmmword ptr [RSP + 0x20],XMM3
    // * 14077118d  F3 0F 10 4C 24 24             MOVSS XMM1,dword ptr [RSP + 0x24]
    uintptr_t movAddr = PatternFinder::Find(FREECAM_POS_SIG);
    if (phase.Step(movAddr, "Freecam_Move MOVUPS anchor", "RT")) {
      int8_t posX = PatternFinder::ReadInt8(movAddr + 4);
      if (phase.StepOffset(posX, "PosX offset", "OFF")) {
        owner.SetFreecamPosXOffset(posX);
        owner.SetFreecamPosYOffset(posX + 4);
        owner.SetFreecamPosZOffset(posX + 8);
        owner.SetFreecamMysteryFloatOffset(posX + 12);

        int32_t quatX = posX + 16;
        owner.SetFreecamQuatXOffset(quatX);
        owner.SetFreecamQuatYOffset(quatX + 4);
        owner.SetFreecamQuatZOffset(quatX + 8);
        owner.SetFreecamQuatWOffset(quatX + 12);
      }
    }
  }

  // ── Phase 4: Orientation Offsets (Yaw/Pitch/Roll) ──
  /*
   * LEA RDI, [RCX + 0x10]           <-- YAW Offset
   * ...
   * MOVSS XMM8, dword ptr [RCX+0x14] <-- PITCH Offset
   *
   * Patterns focus on RCX as the base register to avoid catching stack setups (RAX/RBP).
   */
  {
    auto phase = log.MakePhase("Orientation Offsets");

    auto& cameraHooks = Hooks::CameraHooks::GetInstance();
    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
    // * 14053e760  48 8B C4                      MOV RAX,RSP
    uintptr_t pfnHandleInput = cameraHooks.GetDebugCameraHandleInputFunc();
    if (phase.Step(pfnHandleInput, "DebugCamera_HandleInput", "FN")) {
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
      // * 14053e7aa  48 8D 79 10                   LEA RDI,[RCX + 0x10]
      uintptr_t addrYaw = PatternFinder::Find(pfnHandleInput, 150, "[LEA r64, [r64+off8]]");

      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
      // * 14053e7cb  F3 44 0F 10 41 14             MOVSS XMM8,dword ptr [RCX + 0x14]
      uintptr_t addrPitch = PatternFinder::Find(pfnHandleInput, 150, "F3 44 0F 10 [40-43]");

      bool okYaw = phase.Step(addrYaw, "Yaw LEA anchor", "RT");
      bool okPitch = phase.Step(addrPitch, "Pitch MOVSS anchor", "RT");
      if (okYaw && okPitch) {
        int8_t yawOff = PatternFinder::ReadInt8(addrYaw + 3);
        int8_t pitchOff = PatternFinder::ReadInt8(addrPitch + 5);

        bool okYawOff = phase.StepOffset(yawOff, "Yaw offset", "OFF");
        bool okPitchOff = phase.StepOffset(pitchOff, "Pitch offset", "OFF");
        if (okYawOff && okPitchOff) {
          owner.SetFreecamMouseXOffset(yawOff);
          owner.SetFreecamMouseYOffset(pitchOff);
          owner.SetFreecamRollOffset(pitchOff + 4);  // Roll (Tilt) consistently follows Pitch
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetFlySpeedPtr() != nullptr && owner.GetFreecamContextOffset() != 0 && owner.GetFreecamPosXOffset() != 0 && owner.GetFreecamPosYOffset() != 0 && owner.GetFreecamPosZOffset() != 0 &&
              owner.GetFreecamMysteryFloatOffset() != 0 && owner.GetFreecamQuatXOffset() != 0 && owner.GetFreecamQuatYOffset() != 0 && owner.GetFreecamQuatZOffset() != 0 && owner.GetFreecamQuatWOffset() != 0 &&
              owner.GetFreecamMouseXOffset() != 0 && owner.GetFreecamMouseYOffset() != 0 && owner.GetFreecamRollOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
