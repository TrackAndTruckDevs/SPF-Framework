#include "SPF/Data/GameData/Finders/SessionDataFinder.hpp"

#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

namespace SPF::Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Unique error string used to locate the profile property access logic.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
 * 140fb5f2a  48 8D 0D 2F 56 28 01          LEA RCX,[0x14223b560]
 */
const char* NEW_PROFILE_LOG_STR = "New profile selected: '%s'";

}  // namespace

bool SessionDataFinder::TryFindOffsets(GameObjectSessionService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  uintptr_t xrefSelectProfile = 0;

  // ── Phase 1: Active Profile Data (SelectProfile) ──
  {
    auto phase = log.MakePhase("SelectProfile Data");

    /**
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
     * 140fb5cf0  48 8B C4                      MOV RAX,RSP
     */
    xrefSelectProfile = PatternFinder::FindFunctionByString(NEW_PROFILE_LOG_STR, false);
    uintptr_t addrSelectProfile = PatternFinder::GetFunctionStart(xrefSelectProfile);
    if (phase.Step(addrSelectProfile, "SelectProfile function", "RT")) {
      // 1.1 & 1.2. Extract g_Game and Profile Handle Offset together
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
      // * 140fb5d3a  48 8B 05 3F EF 59 02          MOV RAX,qword ptr [0x143554c80]
      // * 140fb5d41  4C 8B A0 F8 10 00 00          MOV R12,qword ptr [RAX + 0x10f8]
      uintptr_t addrPair = PatternFinder::Find(addrSelectProfile, 128, "[MOV r64, [rip+off32]] [MOV r64, [r64+off32]]");
      if (phase.Step(addrPair, "GamePtr/ProfileHandle pair", "RT")) {
        // Extract g_Game from the first MOV
        uintptr_t gamePtr = PatternFinder::GetRipAddress(addrPair, 3, 7);
        if (phase.Step(gamePtr, "Game Pointer", "DATA")) {
          owner.SetGamePtrAddr(gamePtr);
        }

        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
        // * 140fb5d41  4C 8B A0 F8 10 00 00          MOV R12,qword ptr [RAX + 0x10f8]
        uintptr_t addrprofileOff = PatternFinder::Find(addrPair + 3, 16, "[MOV r64, [r64+off32]]");
        // Extract ProfileHandleOffset from the second MOV (starts at +7 from addrPair)
        int32_t profileOff = PatternFinder::ReadInt32(addrprofileOff + 3);
        if (phase.StepOffset(profileOff, "ProfileHandleOffset", "OFF")) {
          owner.SetProfileHandleOffset(static_cast<uint32_t>(profileOff));
        }
      }
    }
  }

  // ── Phase 2: Session Manager (Networked) ──
  {
    auto phase = log.MakePhase("Session Manager");

    /**
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140514a80[140514a80]) ---/
     * 140514a88  48 8B F9                      MOV RDI,RCX
     * 140514a8b  4C 89 74 24 40                MOV qword ptr [RSP + 0x40],R14
     * 140514a90  48 8D 0D 09 73 DE 01          LEA RCX,[0x1422fbda0]
     */
    uintptr_t addrMpFunc = PatternFinder::FindFunctionByString("[MP] Session started.", true, "[MOV r64, r64]");
    if (phase.Step(addrMpFunc, "MP session function", "RT")) {
      // Find the MOV RCX, [SessionMgr] instruction near the status check
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140514a80[140514a80]) ---/
      // * 140514de5  48 8B 0D E4 FE 03 03          MOV RCX,qword ptr [0x143554cd0]
      // * 140514dec  48 85 C9                      TEST RCX,RCX
      // * 140514def  74 13                         JZ 0x140514e04
      // * 140514df1  0F B6 41 0B                   MOVZX EAX,byte ptr [RCX + 0xb]
      uintptr_t addrMgr = PatternFinder::Find(addrMpFunc, 1000, "[MOV r64, [rip+off32]] [TEST r64, r64] [JE rel8] [MOVZX r32, [r64+off8]]");
      if (phase.Step(addrMgr, "Session Manager sequence", "RT")) {
        uintptr_t sessionMgrPtr = PatternFinder::GetRipAddress(addrMgr, 3, 7);
        if (phase.Step(sessionMgrPtr, "Session Manager pointer", "DATA")) {
          owner.SetSessionMgrPtrAddr(sessionMgrPtr);
        }

        // Extract Status Offset
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140514a80[140514a80]) ---/
        // * 140514df1  0F B6 41 0B                   MOVZX EAX,byte ptr [RCX + 0xb]
        uintptr_t addrMovzx = PatternFinder::Find(addrMgr, 32, "[MOVZX r32, [r64+off8]]");
        if (phase.Step(addrMovzx, "Convoy Status MOVZX", "RT")) {
          int8_t statusOff = PatternFinder::ReadInt8(addrMovzx + 3);
          if (phase.StepOffset(statusOff, "ConvoyStatusOffset", "OFF")) {
            owner.SetConvoyStatusOffset(static_cast<uint8_t>(statusOff));
          }
        }
      }
    }
  }

  // ── Phase 3: Profile Properties (DisplayName and Type) ──
  {
    auto phase = log.MakePhase("Profile Properties");

    /**
     * SEARCH STRATEGY:
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
     * 140fb5f2a  48 8D 0D 2F 56 28 01          LEA RCX,[0x14223b560]
     */
    if (xrefSelectProfile) {
      // 3.1. Extract DisplayName Offset
      /**
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SelectProfile[140fb5cf0]) ---/
       * 140fb5f38  48 8B 52 18                   MOV RDX,qword ptr [RDX + 0x18]
       */
      uintptr_t addrName = PatternFinder::Find(xrefSelectProfile, 64, "[MOV r64, [r64+off8]]");
      if (phase.Step(addrName, "DisplayName MOV", "RT")) {
        int8_t nameOff = PatternFinder::ReadInt8(addrName + 3);
        if (phase.StepOffset(nameOff, "ProfileDisplayNameOffset", "OFF")) {
          owner.SetProfileDisplayNameOffset(static_cast<uint8_t>(nameOff));
        }
      }

      // 3.2. Extract ProfileType Offset
      /**
       * We locate the SetProfileBasePath function via its unique path strings AND
       * its instruction prologue using flexible patterns to avoid hardcoding.
       *
       * Target: SetProfileBasePath (v1.60 verified at 1405042d0)
       * String: "/home/preview_profiles/" (v1.60 at 14212ac00)
       *
       * Ghidra 1.60 Analysis (SetProfileBasePath start):
       * 1405042e3 48 83 EC ??          SUB  RSP, <any>
       * 1405042e7 48 8B [40-7F] ??     MOV  reg, qword ptr [reg + <any disp8>]
       * 1405042eb 33 F6                XOR  ESI, ESI
       * ...
       * 1405042f3 8B 47 40             MOV  EAX, dword ptr [RDI + 0x40]
       */
      uintptr_t addrPathFunc = PatternFinder::FindFunctionByString("/home/preview_profiles/", true, "48 83 EC ?? 48 8B [40-7F] ?? 33 F6");
      if (phase.Step(addrPathFunc, "SetProfileBasePath function", "RT")) {
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetProfileBasePath[1405042d0]) ---/
        // * 1405042f3  8B 47 40                      MOV EAX,dword ptr [RDI + 0x40]
        uintptr_t addrTypeMov = PatternFinder::Find(addrPathFunc, 64, "8B [0-2?] 41");
        if (phase.Step(addrTypeMov, "Profile Type MOV", "RT")) {
          int8_t typeOff = PatternFinder::ReadInt8(addrTypeMov + 2);
          if (phase.StepOffset(typeOff, "ProfileTypeOffset", "OFF")) {
            owner.SetProfileTypeOffset(static_cast<uint8_t>(typeOff));
          }
        }
      }
    }
  }

  // ── Phase 4: Text Locale — LoadLocale + language API ──
  {
    auto phase = log.MakePhase("Mail Text Locale");

    /**
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14083e740[14083e740]) ---/
     * 14083e871  48 8D 15 A0 C5 5D 01          LEA RDX,[0x141e1ae18]
     * FindFunctionByString(anchor, true) -> function start (FUN_14083e740).
     */
    uintptr_t loadLocale = PatternFinder::FindFunctionByString("/locale/%s/mails.sii", true);
    if (phase.Step(loadLocale, "LoadLocale function", "FN")) {
      owner.SetLoadLocaleAddr(loadLocale);
    }

    /**
     * SetApplicationLanguage — located by its locale override path string.
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SetApplicationLanguage[14151dd40]) ---/
     * 14151e25e  48 89 44 24 20                MOV qword ptr [RSP + 0x20],RAX
     */
    uintptr_t xrefSetLang = PatternFinder::FindFunctionByString("/local.override.sii", false);
    if (phase.Step(xrefSetLang, "SetApplicationLanguage xref", "RT")) {
      uintptr_t setLangFn = PatternFinder::GetFunctionStart(xrefSetLang);
      phase.Step(setLangFn, "SetApplicationLanguage function", "FN");

      /**
       * PTR_PTR_142e068d0 — current language global written by SetApplicationLanguage.
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SetApplicationLanguage[14151dd40]) ---/
       * 14151e8d8  48 8D 0D F1 7F 8E 01          LEA RCX,[0x142e068d0]
       * 14151e8df  E8 2C 73 BD FE                CALL 0x1400f5c10
       * 14151e8e4  48 8D 0D 25 EC 15 02          LEA RCX,[0x14367d510]
       * 14151e8eb  FF 15 27 F2 89 00             CALL qword ptr [0x141dbdb18]
       * 14151e8f1  4D 85 ED                      TEST R13,R13
       */
      uintptr_t addrLangGlobal = PatternFinder::Find(xrefSetLang, 1750, "[LEA r64, [rip+off32]] [CALL rel32] [LEA r64, [rip+off32]] [CALL [rip+off32]] [TEST r64, r64]");
      if (phase.Step(addrLangGlobal, "Language global sequence", "RT")) {
        uintptr_t langPtr = PatternFinder::GetRipAddress(addrLangGlobal, 3, 7);
        if (phase.Step(langPtr, "Current language pointer", "DATA")) {
          owner.SetGameLanguagePtrAddr(langPtr);
        }
      }

      /**
       * Language list function — called from SetApplicationLanguage start.
       * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SetApplicationLanguage[14151dd40]) ---/
       * 14151dd40  48 89 4C 24 08                MOV qword ptr [RSP + 0x8],RCX
       * 14151dd78  E8 83 E9 FF FF                CALL 0x14151c700
       * 14151dd7d  48 8B D3                      MOV RDX,RBX
       */
      uintptr_t addrListCall = PatternFinder::Find(setLangFn, 64, "[CALL rel32] [MOV r64, r64]");
      if (phase.Step(addrListCall, "Language list CALL", "RT")) {
        uintptr_t listFn = PatternFinder::GetRipAddress(addrListCall, 1, 5);
        if (phase.Step(listFn, "Language list function", "FN")) {
          owner.SetLanguageListFuncAddr(listFn);
        }
      }
    }

    /**
     * FUN_14041c6c0 — set application language + write CVar cache in one call.
     * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1403fcff0[1403fcff0]) ---/
     * 1403fdbd6  48 8D 05 EF CE E3 01          LEA RAX,[0x14223aacc] = "en_us"
     * 1403fdbe5  E8 D6 EA 01 00                CALL 0x14041c6c0
     */
    uintptr_t xrefEnUs = PatternFinder::FindFunctionByString("en_us", false);
    if (phase.Step(xrefEnUs, "SetLanguage xref (en_us)", "RT")) {
      uintptr_t addrSetCall = PatternFinder::Find(xrefEnUs, 32, "[CALL rel32]");
      if (phase.Step(addrSetCall, "SetLanguage CALL", "RT")) {
        uintptr_t setLanguageFn = PatternFinder::GetRipAddress(addrSetCall, 1, 5);
        if (phase.Step(setLanguageFn, "SetLanguage function (set + CVar cache)", "FN")) {
          owner.SetSetLanguageFuncAddr(setLanguageFn);
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetGamePtrAddr() != 0 && owner.GetProfileHandleOffset() != 0 && owner.GetSessionMgrPtrAddr() != 0 && owner.GetConvoyStatusOffset() != 0 && owner.GetProfileDisplayNameOffset() != 0 && owner.GetProfileTypeOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
