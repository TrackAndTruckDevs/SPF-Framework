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

  // --- Final Readiness Check ---
  m_isReady = owner.GetBankListLockOffset() != 0 && owner.GetBankListHeadOffset() != 0 && owner.GetBankEventListHeadOffset() != 0 && owner.GetBankPathStringOffset() != 0 && owner.GetEventListTerminatorOffset() != 0 && owner.GetEventPathOffset() != 0 && owner.GetEventGuidOffset() != 0;

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
