#include "SPF/Data/GameData/Finders/SoundDataFinder.hpp"

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

namespace SPF::Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Unique string anchor inside SoundBank_Load to locate the function.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
 * 14027ae3e  48 8D 0D 13 79 E6 01          LEA RCX,[0x1420e2758] = "[sound] cannot open fmod guids file %s"
 */
const char* SOUNDBANK_LOAD_STR = "[sound] cannot open fmod guids file %s";

/**
 * @brief Unique string anchor inside SoundBank_AddEventEntry to locate the function.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_AddEventEntry[140284710]) ---/
 * 1402847cb  48 8D 0D E6 F6 E5 01          LEA RCX,[0x1420e3eb8] = "[fmod] invalid guid in bank %s:%s"
 */
const char* ADDEVENTENTRY_STR = "[fmod] invalid guid in bank %s:%s";

/**
 * @brief Unique string anchor inside SoundSystem_Init to locate the Studio::System pointer.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundSystem_Init[1402304b0]) ---/
 * 140230648  48 8D 0D 39 4D EA 01          LEA RCX,[0x1420d5388] = "[sound] Failed to create 'fmod studio system'."
 */
const char* SOUNDSYSTEM_INIT_STR = "[sound] Failed to create 'fmod studio system'.";

}  // namespace

bool SoundDataFinder::TryFindOffsets(SoundService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());
  log.Info("Searching for Sound (FMOD) data using provided Ghidra signatures...");

  uintptr_t pfnSoundBankLoad = 0;

  // ── Phase 1: SoundBank_Load Function ──
  /*
   * SEARCH STRATEGY (Game Version 1.60):
   * Locate SoundBank_Load via a unique log string inside the function, then
   * walk to its prologue.
   *
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
   * 14027ae3e  48 8D 0D 13 79 E6 01          LEA RCX,[0x1420e2758] = "[sound] cannot open fmod guids file %s"
   */
  {
    auto phase = log.MakePhase("SoundBank_Load Function");

    uintptr_t strAddr = PatternFinder::FindFunctionByString(SOUNDBANK_LOAD_STR, false);
    if (phase.Step(strAddr, "SoundBank_Load string anchor", "REF")) {
      pfnSoundBankLoad = PatternFinder::GetFunctionStart(strAddr);
      phase.Step(pfnSoundBankLoad, "SoundBank_Load", "FN");
    }
  }

  // ── Phase 2: Bank List Sentinel + Lock Offset (/0xB0 use /0xC0 SRWLock) ──
  /*
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
   * 14027ac40  48 81 EC B0 00 00 00          SUB RSP,0xb0
   */
  {
    auto phase = log.MakePhase("Bank List Sentinel Offset");

    uintptr_t addrSub = PatternFinder::Find(pfnSoundBankLoad, 32, "[SUB r64, imm32]");
    if (phase.Step(addrSub, "Bank List Sentinel SUB", "RT")) {
      int32_t sentinelOffset = PatternFinder::ReadInt32(addrSub + 3);
      if (phase.StepOffset(sentinelOffset, "Bank List Sentinel Offset", "OFF")) {
        owner.SetBankListSentinelOffset(sentinelOffset);
      }
    }
  }

  // ── Phase 3: Bank List Lock Offset (SRWLock) ──
  /*
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
   * 14027ac4d  48 81 C1 C0 00 00 00          ADD RCX,0xc0
   */
  {
    auto phase = log.MakePhase("Bank List Lock Offset");

    uintptr_t addrAdd = PatternFinder::Find(pfnSoundBankLoad, 64, "[ADD r64, imm32]");
    if (phase.Step(addrAdd, "Bank List Lock ADD", "RT")) {
      int32_t lockOffset = PatternFinder::ReadInt32(addrAdd + 3);
      if (phase.StepOffset(lockOffset, "Bank List Lock Offset", "OFF")) {
        owner.SetBankListLockOffset(lockOffset);
      }
    }
  }

  // ── Phase 4: Bank List Head Offset ──
  /*
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
   * 14027ac5d  49 8B 9D A0 00 00 00          MOV RBX,qword ptr [R13 + 0xa0]
   */
  {
    auto phase = log.MakePhase("Bank List Head Offset");

    uintptr_t addrMov = PatternFinder::Find(pfnSoundBankLoad, 128, "[MOV r64, [r64+off32]]");
    if (phase.Step(addrMov, "Bank List Head MOV", "RT")) {
      int32_t headOffset = PatternFinder::ReadInt32(addrMov + 3);
      if (phase.StepOffset(headOffset, "Bank List Head Offset", "OFF")) {
        owner.SetBankListHeadOffset(headOffset);
      }
    }
  }

  // ── Phase 5: Bank Event List Head + Bank Path String ──
  /*
   * Single signature: three consecutive instructions within 512 bytes of function start.
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_Load[14027ac30]) ---/
   * 14027adaf  49 8D 56 40                   LEA RDX,[R14 + 0x40]
   * 14027adb3  4C 89 62 18                   MOV qword ptr [RDX + 0x18],R12
   * 14027adb7  48 8D 4A 10                   LEA RCX,[RDX + 0x10]
   */
  {
    auto phase = log.MakePhase("Bank Structure Offsets");

    const char* sig = "[LEA r64, [r64+off8]] [MOV [r64+off8], r64] [LEA r64, [r64+off8]]";
    uintptr_t addrLea = PatternFinder::Find(pfnSoundBankLoad, 512, sig);
    if (phase.Step(addrLea, "Bank structure chain", "REF")) {
      // 0x40 from first LEA: LEA RDX,[R14 + 0x40]
      int32_t eventListOffset = (int32_t)(int8_t)*(uint8_t*)(addrLea + 3);
      if (phase.StepOffset(eventListOffset, "Event List Head Offset", "OFF")) {
        owner.SetBankEventListHeadOffset(eventListOffset);
      }

      // 0x18 from MOV: second Find from addrLea, length 8
      uintptr_t addrMov = PatternFinder::Find(addrLea, 8, "[MOV [r64+off8], r64]");
      if (phase.Step(addrMov, "Bank Path String MOV", "RT")) {
        int32_t pathStringOffset = (int32_t)(int8_t)*(uint8_t*)(addrMov + 3);
        if (phase.StepOffset(pathStringOffset, "Bank Path String Offset", "OFF")) {
          owner.SetBankPathStringOffset(pathStringOffset);
        }
      }

      // 0x10 from second LEA: LEA RCX,[RDX + 0x10] (relative offset from event list head to terminator)
      uintptr_t addrLea2 = PatternFinder::Find(addrMov, 8, "[LEA r64, [r64+off8]]");
      if (phase.Step(addrLea2, "Event List Terminator LEA", "RT")) {
        int32_t terminatorRelativeOffset = (int32_t)(int8_t)*(uint8_t*)(addrLea2 + 3);
        if (phase.StepOffset(terminatorRelativeOffset, "Event List Terminator Relative Offset", "OFF")) {
          owner.SetEventListTerminatorOffset(terminatorRelativeOffset);
        }
      }
    }
  }

  // ── Phase 6: Event Entry Offsets (+0x50 path, +0x70 GUID) ──
  {
    auto phase = log.MakePhase("Event Entry Offsets");
    /*
     * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_AddEventEntry[140284710]) ---/
     * 1402847cb  48 8D 0D E6 F6 E5 01          LEA RCX,[0x1420e3eb8] = "[fmod] invalid guid in bank %s:%s"
     */
    uintptr_t addEventEntryStr = PatternFinder::FindFunctionByString(ADDEVENTENTRY_STR, false);
    if (phase.Step(addEventEntryStr, "AddEventEntry string anchor", "REF")) {
      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_AddEventEntry[140284710]) ---/
       * 140284894  0F 11 43 70                   MOVUPS xmmword ptr [RBX + 0x70],XMM0
       */
      uintptr_t addrMovups = PatternFinder::Find(addEventEntryStr, 256, "[MOVUPS [r64+off8], xmm]");
      if (phase.Step(addrMovups, "AddEventEntry MOVUPS", "RT")) {
        int32_t guidOffset = (int32_t)(int8_t)*(uint8_t*)(addrMovups + 3);
        if (phase.StepOffset(guidOffset, "Event GUID Offset", "OFF")) {
          owner.SetEventGuidOffset(guidOffset);
        }
      }

      /*
       * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundBank_AddEventEntry[140284710]) ---/
       * 140284868  48 89 43 58                   MOV qword ptr [RBX + 0x58],RAX
       * 14028486c  48 8D 05 3D 41 E2 01          LEA RAX,[0x1420a89b0]
       * 140284873  48 89 7B 08                   MOV qword ptr [RBX + 0x8],RDI
       */
      uintptr_t addrChain = PatternFinder::FindBackward(addrMovups, 64, "[MOV [r64+off8], r64] [LEA r64, [rip+off32]] [MOV [r64+off8], r64]");
      if (phase.Step(addrChain, "AddEventEntry MOV chain", "RT")) {
        int32_t pathOffset = (int32_t)(int8_t)*(uint8_t*)(addrChain + 3);
        if (phase.StepOffset(pathOffset, "Event Path Offset", "OFF")) {
          owner.SetEventPathOffset(pathOffset);
        }
      }
    }
  }

  // ── Phase 7: Studio::System Offset inside SoundManager (+0x1c8) ──
  /*
   * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SoundSystem_Init[1402304b0]) ---/
   * 140230654  48 8B 8B C8 01 00 00          MOV RCX,qword ptr [RBX + 0x1c8]
   */
  {
    auto phase = log.MakePhase("Studio::System Offset");

    uintptr_t strAddr = PatternFinder::FindFunctionByString(SOUNDSYSTEM_INIT_STR, false);
    if (phase.Step(strAddr, "SoundSystem_Init string anchor", "REF")) {
      uintptr_t addrMov = PatternFinder::Find(strAddr, 32, "[MOV r64, [r64+off32]]");
      if (phase.Step(addrMov, "Studio::System pointer MOV", "RT")) {
        int32_t studioSystemOffset = PatternFinder::ReadInt32(addrMov + 3);
        if (phase.StepOffset(studioSystemOffset, "Studio::System Offset", "OFF")) {
          owner.SetStudioSystemOffset(studioSystemOffset);
        }
      }
    }
  }

  // ── Phase 8: SoundManager Sound Event List Head (+0x88) ──
  /*
   * Project-wide search (not anchored to a function).
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
   * 1402856d0  48 8B 8D 88 00 00 00          MOV RCX,qword ptr [RBP + 0x88]
   * 1402856d7  48 8D 53 08                   LEA RDX,[RBX + 0x8]
   * 1402856db  48 8B 01                      MOV RAX,qword ptr [RCX]
   * 1402856de  48 89 02                      MOV qword ptr [RDX],RAX
   * 1402856e1  41 8B C6                      MOV EAX,R14D
   */
  {
    auto phase = log.MakePhase("Sound Event List Head Offset");

    const char* sig = "[MOV r64, [r64+off32]] [LEA r64, [r64+off8]] [MOV r64, [r64]] [MOV [r64], r64] 41? [MOV r32, r32]";
    uintptr_t addrMov = PatternFinder::Find(sig);
    if (phase.Step(addrMov, "Sound Event List Head chain", "RT")) {
      int32_t eventListHeadOffset = PatternFinder::ReadInt32(addrMov + 3);
      if (phase.StepOffset(eventListHeadOffset, "Sound Event List Head Offset", "OFF")) {
        owner.SetSoundEventListHeadOffset(eventListHeadOffset);
      }

      // node offset: LEA immediately after MOV
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
       * 1402856d7  48 8D 53 08                   LEA RDX,[RBX + 0x8]
       */
      uintptr_t addrLea = addrMov + 7;
      int8_t nodeOffset = PatternFinder::ReadInt8(addrLea + 3);
      if (phase.StepOffset(static_cast<int32_t>(nodeOffset), "SoundEvent node offset", "OFF")) {
        owner.SetSoundEventNodeOffset(static_cast<uint32_t>(nodeOffset));
      }

      // path prism offset: function start → LEA [r64+off8] within 64 bytes
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
       * 1402855c0  48 89 5C 24 20                MOV qword ptr [RSP + 0x20],RBX
       * 1402855e6  48 8D 4A 38                   LEA RCX,[RDX + 0x38]
       */
      uintptr_t fnStart = PatternFinder::GetFunctionStart(addrMov);
      if (phase.Step(fnStart, "SoundEvent_CreateAndInsert start", "RT")) {
        uintptr_t addrPathLea = PatternFinder::Find(fnStart, 64, "[LEA r64, [r64+off8]]");
        if (phase.Step(addrPathLea, "SoundEvent path LEA", "RT")) {
          int8_t pathOffset = PatternFinder::ReadInt8(addrPathLea + 3);
          if (phase.StepOffset(static_cast<int32_t>(pathOffset), "SoundEvent path offset", "OFF")) {
            owner.SetSoundEventPathOffset(static_cast<uint32_t>(pathOffset));
          }
        }
      }

      const char* sigState = "[MOV r32, [r64+off32]] [CMP r32, imm8]";
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
       * 1402856fa  8B 8D C0 01 00 00             MOV ECX,dword ptr [RBP + 0x1c0]
       * 140285700  83 F9 02                      CMP ECX,0x2
       */
      uintptr_t addrState = PatternFinder::Find(addrMov, 64, sigState);
      if (phase.Step(addrState, "SoundEvent state CMP", "RT")) {
        int32_t stateOffset = PatternFinder::ReadInt32(addrState + 2);
        if (phase.StepOffset(stateOffset, "SoundEvent state offset", "OFF")) {
          owner.SetSoundEventStateOffset(stateOffset);
        }

        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
         * 140285700  83 F9 02                      CMP ECX,0x2
         */
        uintptr_t addrBoundCmp = PatternFinder::Find(addrState, 16, "[CMP r32, imm8]");
        if (phase.Step(addrBoundCmp, "SoundEvent bound state CMP", "RT")) {
          int32_t boundState = PatternFinder::ReadInt8(addrBoundCmp + 2);
          if (phase.StepOffset(boundState, "SoundEvent bound state", "CNT")) {
            owner.SetSoundEventBoundState(static_cast<uint32_t>(boundState));
          }
        }

        const char* sigLock = "[LEA r64, [r64+off32]]";
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_CreateAndInsert[1402855c0]) ---/
         * 14028574e  48 8D 8D D8 01 00 00          LEA RCX,[RBP + 0x1d8]
         */
        uintptr_t addrLock = PatternFinder::Find(addrState, 96, sigLock);
        if (phase.Step(addrLock, "SoundEvent bound lock LEA", "RT")) {
          int32_t lockOffset = PatternFinder::ReadInt32(addrLock + 3);
          if (phase.StepOffset(lockOffset, "SoundEvent bound lock offset", "OFF")) {
            owner.SetSoundEventBoundLockOffset(lockOffset);
          }
        }
      }
    }
  }

  // ── Phase 9: Global sound_event create SRWLock (DAT) ──
  /*
   * String anchor → CALL to SoundEvent_Create → LEA to global lock DAT.
   */
  {
    auto phase = log.MakePhase("SoundEvent Create SRWLock");

    const char* soundRefStr = "/sound/ui/cbradio/cb_radio_start.soundref";
    /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1406bd780[1406bd780]) ---/
     * 1406bd7a5  48 8D 05 6C 7F BD 01          LEA RAX,[0x142295718] = "/sound/ui/cbradio/cb_radio_start.soundref"
     */
    uintptr_t strAddr = PatternFinder::FindFunctionByString(soundRefStr, false);
    if (phase.Step(strAddr, "soundref string anchor", "REF")) {
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1406bd780[1406bd780]) ---/
       * 1406bd7c5  E8 36 E6 FF FF                CALL 0x1406bbe00 - SoundEvent_Create[1406bbe00]
       */
      uintptr_t addrCall = PatternFinder::Find(strAddr, 64, "[CALL rel32]");
      if (phase.Step(addrCall, "SoundEvent_Create CALL", "RT")) {
        uintptr_t pfnSoundEventCreate = PatternFinder::GetRipAddress(addrCall, 1, 5);
        if (phase.Step(pfnSoundEventCreate, "SoundEvent_Create", "FN")) {
          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_Create[1406bbe00]) ---/
           * 1406bbe1e  48 8D 0D 23 DE 99 02          LEA RCX,[0x143059c48]
           */
          uintptr_t addrLea = PatternFinder::Find(pfnSoundEventCreate, 64, "[LEA r64, [rip+off32]]");
          if (phase.Step(addrLea, "SRWLock LEA", "RT")) {
            uintptr_t lockAddr = PatternFinder::GetRipAddress(addrLea, 3, 7);
            if (phase.Step(lockAddr, "SRWLock DAT", "ADR")) {
              owner.SetSoundEventCreateLockAddr(lockAddr);
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_Create[1406bbe00]) ---/
           * 1406bbe9e  48 8D 05 5B 1E B5 01          LEA RAX,[0x14220dd00]
           * 1406bbea5  48 89 4F 40                   MOV qword ptr [RDI + 0x40],RCX
           * 1406bbea9  48 89 4F 68                   MOV qword ptr [RDI + 0x68],RCX
           * 1406bbead  48 8D 8F B0 00 00 00          LEA RCX,[RDI + 0xb0]
           */
          const char* sigVtable = "[LEA r64, [rip+off32]] [MOV [r64+off8], r64] [MOV [r64+off8], r64] [LEA r64, [r64+off32]]";
          uintptr_t addrVtableLea = PatternFinder::Find(pfnSoundEventCreate, 256, sigVtable);
          if (phase.Step(addrVtableLea, "sound_event vtable LEA", "RT")) {
            uintptr_t vtableAddr = PatternFinder::GetRipAddress(addrVtableLea, 3, 7);
            if (phase.Step(vtableAddr, "sound_event vtable PTR_FUN", "ADR")) {
              owner.SetSoundEventVtableAddr(vtableAddr);
            }
          }
        }
      }
    }
  }

  // ── Phase 10: SoundEvent_ActivateFromSource Function ──
  /*
   * Project-wide search for mid-function prologue pattern, then GetFunctionStart.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_ActivateFromSource[14028f370]) ---/
   * 14028f3d8  49 8B D7                      MOV RDX,R15
   * 14028f3db  48 89 7C 24 30                MOV qword ptr [RSP + 0x30],RDI
   * 14028f3e0  48 89 54 24 28                MOV qword ptr [RSP + 0x28],RDX
   * 14028f3e5  48 89 5C 24 20                MOV qword ptr [RSP + 0x20],RBX
   * 14028f3ea  48 89 7C 24 38                MOV qword ptr [RSP + 0x38],RDI
   * 14028f3ef  40 38 39                      CMP byte ptr [RCX],DIL
   */
  {
    auto phase = log.MakePhase("SoundEvent_ActivateFromSource");

    const char* sig = "[MOV r64, r64] [MOV [r64+off8], r64] [MOV [r64+off8], r64] [MOV [r64+off8], r64] [MOV [r64+off8], r64] 40 [CMP [r64], r8]";
    uintptr_t addrMatch = PatternFinder::Find(sig);
    if (phase.Step(addrMatch, "ActivateFromSource pattern", "RT")) {
      uintptr_t pfnActivate = PatternFinder::GetFunctionStart(addrMatch);
      if (phase.Step(pfnActivate, "SoundEvent_ActivateFromSource", "FN")) {
        owner.SetSoundEventActivateFn(pfnActivate);

        // source prism offset
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_ActivateFromSource[14028f370]) ---/
         * 14028f412  48 8D 56 60                   LEA RDX,[RSI + 0x60]
         * 14028f416  48 83 F8 FF                   CMP RAX,-0x1
         */
        const char* sigSource = "[LEA r64, [r64+off8]] [CMP r64, imm8]";
        uintptr_t addrSourceLea = PatternFinder::Find(pfnActivate, 256, sigSource);
        if (phase.Step(addrSourceLea, "SoundEvent source LEA", "RT")) {
          int8_t sourceOffset = PatternFinder::ReadInt8(addrSourceLea + 3);
          if (phase.StepOffset(static_cast<int32_t>(sourceOffset), "SoundEvent source offset", "OFF")) {
            owner.SetSoundEventSourceOffset(static_cast<uint32_t>(sourceOffset));
          }
        }
      }
    }
  }

  // ── Phase 11: SoundRef_LoadConfig Function ──
  /*
   * FindFunctionByString with getFunctionStart=true returns function start.
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundRef_LoadConfig[1402852a0]) ---/
   * 140285326  48 8D 0D 4B 57 F8 01          LEA RCX,[0x14220aa78] = "[sound] Cannot open sound configuration file %s"
   */
  {
    auto phase = log.MakePhase("SoundRef_LoadConfig");

    uintptr_t pfnLoadConfig = PatternFinder::FindFunctionByString("[sound] Cannot open sound configuration file %s", true);
    if (phase.Step(pfnLoadConfig, "SoundRef_LoadConfig", "FN")) {
      owner.SetSoundRefLoadConfigFn(pfnLoadConfig);
    }
  }

  // ── Phase 12: UI SoundRef / Voice-nav Config Table Addresses ──
  /*
   * disp32 in SIB MOV is an RVA from image base (not a structure offset).
   * Resolve: moduleBase + disp32, validate as absolute address ("ADR").
   */
  {
    auto phase = log.MakePhase("UI SoundRef / Voice-nav Tables");

    uintptr_t moduleBase = PatternFinder::GetModuleBase();
    if (phase.Step(moduleBase, "module base", "ADR")) {
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
       * 1404421d3  48 8D 0D 36 21 E0 01          LEA RCX,[0x142244310]
       */
      uintptr_t strAddr = PatternFinder::FindFunctionByString("[sound] Failed to initialize internal sound system.", false);
      if (phase.Step(strAddr, "sound init string LEA", "REF")) {
        const char* sig = "[MOV r64, [r64+sib+off32]] 45 [MOV r32, [r64+off32]]";
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
         * 140442518  49 8B 94 07 08 CE 6D 02       MOV RDX,qword ptr [R15 + RAX*0x1 + 0x26dce08]
         * 140442520  45 8B 84 07 10 CE 6D 02       MOV R8D,dword ptr [R15 + RAX*0x1 + 0x26dce10]
         */
        uintptr_t addrMov = PatternFinder::Find(strAddr, 1000, sig);
        if (phase.Step(addrMov, "config table SIB MOV", "RT")) {
          int32_t tableRva = PatternFinder::ReadInt32(addrMov + 4);
          uintptr_t tableAddr = moduleBase + static_cast<uintptr_t>(tableRva);
          if (phase.Step(tableAddr, "UI SoundRef table", "ADR")) {
            owner.SetUiSoundRefTableAddr(tableAddr);
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 140442520  45 8B 84 07 10 CE 6D 02       MOV R8D,dword ptr [R15 + RAX*0x1 + 0x26dce10]
           */
          uintptr_t addrUiCatMov = PatternFinder::Find(addrMov, 16, "45 [MOV r32, [r64+off32]]");
          if (phase.Step(addrUiCatMov, "UI SoundRef category MOV", "RT")) {
            int32_t uiCatOff = PatternFinder::ReadInt32(addrUiCatMov + 4) - tableRva;
            if (phase.StepOffset(uiCatOff, "UI SoundRef category offset", "OFF")) {
              owner.SetUiSoundRefCategoryOffset(static_cast<uint32_t>(uiCatOff));
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 1404424f2  41 80 BC 0F 14 CE 6D 02 00    CMP byte ptr [R15 + RCX*0x1 + 0x26dce14],0x0
           */
          uintptr_t addrUiEnMov = PatternFinder::FindBackward(addrMov, 64, "41 [CMP byte ptr [r64+off32], imm8]");
          if (phase.Step(addrUiEnMov, "UI SoundRef enabled CMP", "RT")) {
            int32_t uiEnOff = PatternFinder::ReadInt32(addrUiEnMov + 4) - tableRva;
            if (phase.StepOffset(uiEnOff, "UI SoundRef enabled offset", "OFF")) {
              owner.SetUiSoundRefEnabledOffset(static_cast<uint32_t>(uiEnOff));
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 14044249e  48 C7 47 20 21 00 00 00       MOV qword ptr [RDI + 0x20],0x21
           * 1404424a6  EB 27                         JMP 0x1404424cf
           */
          uintptr_t addrUiCountInit =
              PatternFinder::FindBackward(addrMov, 150, "[MOV qword ptr [r64+off8], imm64_32] [JMP rel8]");
          if (phase.Step(addrUiCountInit, "UI SoundRef count init", "RT")) {
            int32_t uiCount = PatternFinder::ReadInt32(addrUiCountInit + 4);
            if (phase.StepOffset(uiCount, "UI SoundRef count", "CNT")) {
              owner.SetUiSoundRefCount(static_cast<uint32_t>(uiCount));
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 14044257b  49 83 C7 18                   ADD R15,0x18
           */
          uintptr_t addrUiEntryAdd = PatternFinder::Find(addrMov, 128, "[ADD r64, imm8]");
          if (phase.Step(addrUiEntryAdd, "UI SoundRef entry ADD", "RT")) {
            int32_t uiEntrySize = PatternFinder::ReadInt8(addrUiEntryAdd + 3);
            if (phase.StepOffset(uiEntrySize, "UI SoundRef entry size", "SZ")) {
              owner.SetUiSoundRefEntrySize(static_cast<uint32_t>(uiEntrySize));
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 14044255c  48 3B 77 20                   CMP RSI,qword ptr [RDI + 0x20]
           */
          uintptr_t addrCountCmp = PatternFinder::Find(addrMov, 128, "[CMP r64, [r64+off8]]");
          if (phase.Step(addrCountCmp, "UI wrapper count CMP", "RT")) {
            int8_t countOffset = PatternFinder::ReadInt8(addrCountCmp + 3);
            if (phase.StepOffset(static_cast<int32_t>(countOffset), "UI wrapper count offset", "OFF")) {
              owner.SetUiWrapperArrayCountOffset(static_cast<uint32_t>(countOffset));
            }

            /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
             * 140442566  48 8B 47 18                   MOV RAX,qword ptr [RDI + 0x18]
             */
            uintptr_t addrBufMov = PatternFinder::Find(addrCountCmp, 64, "[MOV r64, [r64+off8]]");
            if (phase.Step(addrBufMov, "UI wrapper buffer MOV", "RT")) {
              int8_t bufOffset = PatternFinder::ReadInt8(addrBufMov + 3);
              if (phase.StepOffset(static_cast<int32_t>(bufOffset), "UI wrapper buffer offset", "OFF")) {
                owner.SetUiWrapperArrayBufferOffset(static_cast<uint32_t>(bufOffset));
              }

              /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
               * 14044278a  48 8D 9F D8 01 00 00          LEA RBX,[RDI + 0x1d8]
               * 140442791  48 8B 43 10                   MOV RAX,qword ptr [RBX + 0x10]
               */
              uintptr_t addrVnLea = PatternFinder::Find(addrBufMov, 600, "[LEA r64, [r64+off32]] [MOV r64, [r64+off8]]");
              if (phase.Step(addrVnLea, "voice-nav array LEA", "RT")) {
                int32_t vnBase = PatternFinder::ReadInt32(addrVnLea + 3);
                if (phase.StepOffset(vnBase, "voice-nav array base", "OFF")) {
                  /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                   * 140442791  48 8B 43 10                   MOV RAX,qword ptr [RBX + 0x10]
                   */
                  uintptr_t addrVnCountMov = PatternFinder::Find(addrVnLea, 16, "[MOV r64, [r64+off8]]");
                  if (phase.Step(addrVnCountMov, "voice-nav count MOV", "RT")) {
                    int32_t vnCountOffset = vnBase + PatternFinder::ReadInt8(addrVnCountMov + 3);
                    if (phase.StepOffset(vnCountOffset, "voice-nav count offset", "OFF")) {
                      owner.SetVoiceNavArrayCountOffset(static_cast<uint32_t>(vnCountOffset));
                    }

                    /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                     * 140442795  48 83 F8 26                   CMP RAX,0x26
                     */
                    uintptr_t addrVnCountCmp = PatternFinder::Find(addrVnCountMov, 16, "[CMP r64, imm8]");
                    if (phase.Step(addrVnCountCmp, "voice-nav count CMP", "RT")) {
                      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                       * 1404427a1  48 8B 4B 08                   MOV RCX,qword ptr [RBX + 0x8]
                       */
                      uintptr_t addrVnBufMov = PatternFinder::Find(addrVnCountCmp, 16, "[MOV r64, [r64+off8]]");
                      if (phase.Step(addrVnBufMov, "voice-nav buffer MOV", "RT")) {
                        int32_t vnBufOffset = vnBase + PatternFinder::ReadInt8(addrVnBufMov + 3);
                        if (phase.StepOffset(vnBufOffset, "voice-nav buffer offset", "OFF")) {
                          owner.SetVoiceNavArrayBufferOffset(static_cast<uint32_t>(vnBufOffset));
                        }

                        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                         * 1404427bd  48 C7 43 10 26 00 00 00       MOV qword ptr [RBX + 0x10],0x26
                         */
                        uintptr_t addrVnMaxMov = PatternFinder::Find(addrVnBufMov, 64, "[MOV qword ptr [r64+off8], imm64_32]");
                        if (phase.Step(addrVnMaxMov, "voice-nav max MOV", "RT")) {
                          int32_t vnMax = PatternFinder::ReadInt32(addrVnMaxMov + 4);
                          if (phase.StepOffset(vnMax, "voice-nav max entries", "CNT")) {
                            owner.SetVoiceNavMaxEntries(static_cast<uint32_t>(vnMax));
                          }

                          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                           * 1404427ea  4D 8B 84 02 B0 B2 D7 02       MOV R8,qword ptr [R10 + RAX*0x1 + 0x2d7b2b0]
                           */
                          uintptr_t addrVnPathMov = PatternFinder::Find(addrVnMaxMov, 64, "[MOV r64, [r64+sib+off32]]");
                          if (phase.Step(addrVnPathMov, "voice-nav path SIB MOV", "RT")) {
                            int32_t vnPathDisp = PatternFinder::ReadInt32(addrVnPathMov + 4);

                            /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                             * 1404427f2  41 0F B6 94 02 A8 B2 D7 02    MOVZX EDX,byte ptr [R10 + RAX*0x1 + 0x2d7b2a8]
                             */
                            uintptr_t addrVnEnMov = PatternFinder::Find(addrVnPathMov, 32, "41 [MOVZX r32, [r64+off32]]");
                            if (phase.Step(addrVnEnMov, "voice-nav enabled MOVZX", "RT")) {
                              int32_t vnEnDisp = PatternFinder::ReadInt32(addrVnEnMov + 5);

                              /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                               * 1404427fe  41 8B 8C 02 A4 B2 D7 02       MOV ECX,dword ptr [R10 + RAX*0x1 + 0x2d7b2a4]
                               * 140442806  41 8B 84 02 A0 B2 D7 02       MOV EAX,dword ptr [R10 + RAX*0x1 + 0x2d7b2a0]
                               */
                              uintptr_t addrVnPair =
                                  PatternFinder::Find(addrVnEnMov, 32, "41 [MOV r32, [r64+off32]] 41 [MOV r32, [r64+off32]]");
                              if (phase.Step(addrVnPair, "voice-nav index+category MOV pair", "RT")) {
                                int32_t vnIdxDisp = PatternFinder::ReadInt32(addrVnPair + 4);   // 1404427fe: index field RVA
                                int32_t vnCatDisp = PatternFinder::ReadInt32(addrVnPair + 12);  // 140442806: category field RVA == table base

                                int32_t vnPathOff = vnPathDisp - vnCatDisp;
                                int32_t vnEnOff = vnEnDisp - vnCatDisp;
                                int32_t vnIdxOff = vnIdxDisp - vnCatDisp;
                                if (phase.StepOffset(vnPathOff, "voice-nav path offset", "OFF")) {
                                  owner.SetVoiceNavPathOffset(static_cast<uint32_t>(vnPathOff));
                                }
                                if (phase.StepOffset(vnEnOff, "voice-nav enabled offset", "OFF")) {
                                  owner.SetVoiceNavEnabledOffset(static_cast<uint32_t>(vnEnOff));
                                }
                                if (phase.StepOffset(vnIdxOff, "voice-nav index offset", "OFF")) {
                                  owner.SetVoiceNavIndexOffset(static_cast<uint32_t>(vnIdxOff));
                                }
                                owner.SetVoiceNavCategoryOffset(static_cast<uint32_t>(vnCatDisp - vnCatDisp));
                              }
                            }
                          }

                          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                           * 14044282b  49 83 C2 18                   ADD R10,0x18
                           * 14044282f  49 83 C3 28                   ADD R11,0x28
                           */
                          uintptr_t addrVnSizeAdd =
                              PatternFinder::Find(addrVnMaxMov, 128, "[ADD r64, imm8] [ADD r64, imm8]");
                          if (phase.Step(addrVnSizeAdd, "voice-nav entry ADD pair", "RT")) {
                            int32_t vnEntrySize = PatternFinder::ReadInt8(addrVnSizeAdd + 3);
                            if (phase.StepOffset(vnEntrySize, "voice-nav entry size", "SZ")) {
                              owner.SetVoiceNavEntrySize(static_cast<uint32_t>(vnEntrySize));
                            }
                            int32_t vnStride = PatternFinder::ReadInt8(addrVnSizeAdd + 4 + 3);
                            if (phase.StepOffset(vnStride, "voice-nav entry stride", "SZ")) {
                              owner.SetVoiceNavEntryStride(static_cast<uint32_t>(vnStride));
                            }

                            /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
                             * 1404429f3  4C 89 62 20                   MOV qword ptr [RDX + 0x20],R12
                             * 1404429f7  48 83 C2 28                   ADD RDX,0x28
                             */
                            uintptr_t addrVnEventMov =
                                PatternFinder::Find(addrVnSizeAdd, 500, "[MOV [r64+off8], r64] [ADD r64, imm8]");
                            if (phase.Step(addrVnEventMov, "voice-nav event MOV", "RT")) {
                              int32_t vnEventOff = PatternFinder::ReadInt8(addrVnEventMov + 3);
                              if (phase.StepOffset(vnEventOff, "voice-nav event offset", "OFF")) {
                                owner.SetVoiceNavEntryEventOffset(static_cast<uint32_t>(vnEventOff));
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 140442532  49 C7 44 24 08 00 00 00 00    MOV qword ptr [R12 + 0x8],0x0
           */
          uintptr_t addrEventInit = PatternFinder::Find(addrMov, 64, "[MOV qword ptr [r64+off8], imm64_32]");
          if (phase.Step(addrEventInit, "UI wrapper event MOV", "RT")) {
            int8_t eventOffset = PatternFinder::ReadInt8(addrEventInit + 4);
            if (phase.StepOffset(static_cast<int32_t>(eventOffset), "UI wrapper event offset", "OFF")) {
              owner.SetUiWrapperEventOffset(static_cast<uint32_t>(eventOffset));
            }
          }

          const char* sigNav = "41 [MOV r32, [r64+off32]] 43 [MOV [r64], r32]";
          /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_140442180[140442180]) ---/
           * 140442806  41 8B 84 02 A0 B2 D7 02       MOV EAX,dword ptr [R10 + RAX*0x1 + 0x2d7b2a0]
           * 14044280e  43 89 04 0B                   MOV dword ptr [R11 + R9*0x1],EAX
           */
          uintptr_t addrNav = PatternFinder::Find(addrMov, 800, sigNav);
          if (phase.Step(addrNav, "voice-nav SIB MOV", "RT")) {
            int32_t navRva = PatternFinder::ReadInt32(addrNav + 4);
            uintptr_t navAddr = moduleBase + static_cast<uintptr_t>(navRva);
            if (phase.Step(navAddr, "Voice-nav table", "ADR")) {
              owner.SetVoiceNavTableAddr(navAddr);
            }
          }
        }
      }
    }
  }

  // ── Phase 13: sound_event instance / playback-state / bound field offsets ──
  /*
   * Offsets observed in SoundEvent_Stop / SoundEvent_PlaybackControl decompilation.
   * Drives the rebind fix (stop -> release -> activate) and the L2 game-event API.
   */
  {
    auto phase = log.MakePhase("SoundEvent instance/state/bound offsets");

    uintptr_t stopFn = owner.GetSoundEventStopFn();
    uintptr_t playbackFn = owner.GetSoundEventPlaybackControlFn();
    if (phase.Step(stopFn, "SoundEvent_Stop", "FN")) {
      if (phase.Step(playbackFn, "SoundEvent_PlaybackControl", "FN")) {
        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_Stop[14028fb70]) ---/
         * 14028fb7f  48 8B 89 A0 00 00 00          MOV RCX,qword ptr [RCX + 0xa0]
         */
        uintptr_t addrInstance = PatternFinder::Find(stopFn, 32, "[MOV r64, [r64+off32]]");
        if (phase.Step(addrInstance, "SoundEvent instance MOV", "RT")) {
          uint32_t instanceOffset = static_cast<uint32_t>(PatternFinder::ReadInt32(addrInstance + 3));
          if (phase.StepOffset(static_cast<int32_t>(instanceOffset), "SoundEvent instance offset", "OFF")) {
            owner.SetSoundEventInstanceOffset(instanceOffset);
          }
        }

        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_Stop[14028fb70]) ---/
         * 14028fb86  C7 83 30 00 00 00 00 00 00 00  MOV dword ptr [RBX + 0x30],0x0
         * 14028fb94  C7 87 30 00 00 00 00 00 00 00  MOV dword ptr [RDI + 0x30],0x0
         * /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_PlaybackControl[14028f7f0]) ---/
         * 14028f826  89 7F 30                        MOV dword ptr [RDI + 0x30],EBX
         */
        uintptr_t addrState = PatternFinder::Find(stopFn, 64, "[MOV dword ptr [r64+off8], imm32]");
        if (phase.Step(addrState, "SoundEvent playback-state MOV", "RT")) {
          uint32_t stateOffset = static_cast<uint32_t>(PatternFinder::ReadInt8(addrState + 2));
          if (phase.StepOffset(static_cast<int32_t>(stateOffset), "SoundEvent playback-state offset", "OFF")) {
            owner.SetSoundEventPlaybackStateOffset(stateOffset);
          }
        }

        /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(SoundEvent_PlaybackControl[14028f7f0]) ---/
         * 14028f81b  8B 81 2C 00 00 00             MOV EAX,dword ptr [RCX + 0x2c]
         * 14028f81e  83 F8 02                      CMP EAX,0x2
         */
        uintptr_t addrBound = PatternFinder::Find(playbackFn, 32, "[MOV r32, [r64+off8]]");
        if (phase.Step(addrBound, "SoundEvent bound-field MOV", "RT")) {
          uint32_t boundOffset = static_cast<uint32_t>(PatternFinder::ReadInt8(addrBound + 2));
          if (phase.StepOffset(static_cast<int32_t>(boundOffset), "SoundEvent bound-field offset", "OFF")) {
            owner.SetSoundEventBoundFieldOffset(boundOffset);
          }
        }

        // Vtable order-shift guard: slot +0x10 must equal the pattern-found activate fn.
        if (owner.GetSoundEventVtableAddr() != 0 && owner.ReadSoundEventVtableSlot(2) != owner.GetSoundEventActivateFn()) {
          log.Error("vtable slot +0x10 != SoundEvent_ActivateFromSource - vtable order shifted");
        }
      }
    }
  }

  // --- Final Readiness Check ---
  // Required: without these the core SoundRef path is unsafe or non-functional —
  // state/bound-lock/bound-state gate the SRW lock around prism_string writes
  // (unlocked write races the game thread); UI wrapper offsets are the primary
  // LIVE source for FindSoundEventsByPath, so register would silently bind nothing.
  // Optional (not checked): UI table count/entry size and the whole voice-nav stack —
  // a zero there degrades catalog/VN sections only, all such loops are guarded.
  m_isReady = owner.GetBankListLockOffset() != 0 && owner.GetBankListHeadOffset() != 0 && owner.GetBankEventListHeadOffset() != 0 && owner.GetBankPathStringOffset() != 0 && owner.GetEventListTerminatorOffset() != 0 &&
              owner.GetEventPathOffset() != 0 && owner.GetEventGuidOffset() != 0 && owner.GetSoundEventListHeadOffset() != 0 && owner.GetSoundEventCreateLockAddr() != 0 && owner.GetSoundEventActivateFn() != 0 &&
              owner.GetSoundRefLoadConfigFn() != 0 && owner.GetUiSoundRefTableAddr() != 0 && owner.GetStudioSystemOffset() != 0 && owner.GetSoundEventNodeOffset() != 0 && owner.GetSoundEventPathOffset() != 0 &&
               owner.GetSoundEventSourceOffset() != 0 && owner.GetSoundEventStateOffset() != 0 && owner.GetSoundEventBoundLockOffset() != 0 && owner.GetSoundEventBoundState() != 0 &&
               owner.GetSoundEventInstanceOffset() != 0 && owner.GetSoundEventPlaybackStateOffset() != 0 && owner.GetSoundEventBoundFieldOffset() != 0 &&
              owner.GetUiWrapperArrayBufferOffset() != 0 && owner.GetUiWrapperArrayCountOffset() != 0 && owner.GetUiWrapperEventOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
