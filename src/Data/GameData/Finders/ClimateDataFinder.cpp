#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

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
 * @brief Unique string anchor to locate the SetWeather function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 */
const char* SET_WEATHER_STRING = "Restarting environment transition, possibly upcoming blending errors.";

/**
 * @brief Context signature to disambiguate the SetWeather string XREF
 *        (register store to [r64+off32], immediate store, then MOVSS load).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 */
const char* SET_WEATHER_CONTEXT = "[MOV r32, [r64+off32]] [MOV r32, imm32] [MOVSS xmm, [r64+off32]]";

/**
 * @brief Pattern for the two consecutive MOV dword ptr [r64+off32] stores (weather mode + target).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 * 1404df417  89 91 70 3E 00 00             MOV dword ptr [RCX + 0x3e70],EDX
 * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
 */
const char* WEATHER_MODE_OFFSET_SIG = "[MOV [r64+off32], r32] [MOV [r64+off32], r32]";

/**
 * @brief Pattern for the second MOV dword ptr [r64+off32] store (weather target).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
 */
const char* WEATHER_TARGET_OFFSET_SIG = "[MOV [r64+off32], r32]";

/**
 * @brief Pattern for the MOVSS [r64+off32] store of the remaining bad weather time
 *        followed by the MOV qword ptr [r64+off32], -0x1 reset.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 * 1404df451  F3 0F 11 83 70 45 00 00       MOVSS dword ptr [RBX + 0x4570],XMM0
 * 1404df459  48 C7 83 60 45 00 00 FF FF FF FF MOV qword ptr [RBX + 0x4560],-0x1
 */
const char* REMAINING_BAD_WEATHER_SIG = "[MOVSS [r64+off32], xmm] [MOV qword ptr [r64+off32], imm64_32]";

/**
 * @brief Pattern for the MOV r64, [r64+off32] load of the env profile data pointer
 *        followed by a MULSS against a RIP-relative constant.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
 * 1404df559  48 8B 83 E0 2A 00 00          MOV RAX,qword ptr [RBX + 0x2ae0]
 * 1404df560  F3 0F 59 05 FC 7F EF 01       MULSS XMM0,dword ptr [0x1423d7564]
 */
const char* ENV_PROFILE_PTR_SIG = "[MOV r64, [r64+off32]] F3";

/**
 * @brief Pattern to locate the UpdateActiveClimate function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
 * 1404d74d6  48 8B 81 B8 2A 00 00          MOV RAX,qword ptr [RCX + 0x2ab8]
 * 1404d74dd  48 8B F9                      MOV RDI,RCX
 * 1404d74e0  4C 8B C0                      MOV R8,RAX
 * 1404d74e3  48 89 5C 24 40                MOV qword ptr [RSP + 0x40],RBX
 * 1404d74e8  48 8D 4C 24 30                LEA RCX,[RSP + 0x30]
 * 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
 * \---
 */
const char* UPDATE_ACTIVE_CLIMATE_SIG = "[MOV r64, [r64+off32]] [MOV r64, r64] [MOV r64, r64] [MOV [r64+off8], r64] [LEA r64, [r64+off8]] [MOV r32, [r64+off8]]";

/**
 * @brief Pattern for the MOV r32, [r64+off8] load of the climate UnitID.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
 * 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
 * \---
 */
const char* CLIMATE_UNIT_ID_SIG = "[MOV r32, [r64+off8]]";

/**
 * @brief Unique string anchor to locate the SetClimate function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetClimate[1404df5f0]) ---/
 * * 1404df709  48 8D 0D 50 84 C4 01          LEA RCX,[0x142127b60] = "Restarting environment transition, possi..."
 * * 1404df710  E8 6B 8E C1 FF                CALL 0x1400f8580
 * * 1404df715  48 8B 03                      MOV RAX,qword ptr [RBX]
 */
const char* SET_CLIMATE_STRING = "Restarting environment transition, possibly upcoming blending errors.";

/**
 * @brief Context signature to disambiguate the SetClimate string XREF (MOV r64, [r64]).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetClimate[1404df5f0]) ---/
 * * 1404df715  48 8B 03                      MOV RAX,qword ptr [RBX]
 */
const char* SET_CLIMATE_CONTEXT = "[MOV r64, [r64]]";

/**
 * @brief Unique string anchor to locate the FindClimateByToken function.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
 * * 1404e45ad  48 8D 0D 9C 40 C4 01          LEA RCX,[0x142128650] = "Unknown climate %s"
 */
const char* FIND_CLIMATE_STR = "Unknown climate %s";

/**
 * @brief Pattern for the MOV r64, [r64+off32] load of the climate array pointer.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
 * * 1404e4525  48 8B B9 30 01 00 00          MOV RDI,qword ptr [RCX + 0x130]
 */
const char* CLIMATE_ARRAY_SIG = "[MOV r64, [r64+off32]]";

/**
 * @brief Pattern for the MOV r64, [r64+off32] load of the climate count.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
 * * 1404e452f  48 8B 81 38 01 00 00          MOV RAX,qword ptr [RCX + 0x138]
 */
const char* CLIMATE_COUNT_SIG = "[MOV r64, [r64+off32]]";

/**
 * @brief Pattern to locate the SunProfileUpdate function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * 1404e4940  44 88 44 24 18                MOV byte ptr [RSP + 0x18],R8B
 * 1404e4945  41 54                         PUSH R12
 * 1404e4947  41 56                         PUSH R14
 */
const char* SUN_PROFILE_UPDATE_SIG = "44 [MOV [r64+off8], r8] [PUSH R8-R15]";

/**
 * @brief Pattern for the MOV r32, [r64+off32] load of the active profile index.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e49c9  41 8B 8E 60 45 00 00          MOV ECX,dword ptr [R14 + 0x4560]
 */
const char* ACTIVE_PROFILE_INDEX_SIG = "41 [MOV r32, [r64+off32]]";

/**
 * @brief Pattern for the MOV r32, [r64+off32] load of the next profile index.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e49e1  41 8B 8E 64 45 00 00          MOV ECX,dword ptr [R14 + 0x4564]
 */
const char* NEXT_PROFILE_INDEX_SIG = "41 [MOV r32, [r64+off32]]";

/**
 * @brief Pattern for the MOV r32, imm32 store of the "nice" container offset (0xd0).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e49be  B8 D0 00 00 00                MOV EAX,0xd0
 */
const char* NICE_CONTAINER_SIG = "[MOV r32, imm32]";

/**
 * @brief Pattern for the MOV r32, imm32 store of the "bad" container offset (0x120)
 *        followed by the CMP against the container count.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e498a  BA 20 01 00 00                MOV EDX,0x120
 * * 1404e498f  4D 3B BE 38 01 00 00          CMP R15,qword ptr [R14 + 0x138]
 */
const char* BAD_CONTAINER_SIG = "[MOV r32, imm32] [CMP r64, [r64+off32]]";

/**
 * @brief Pattern for the MOV r64, [r64+off8] load of the container count (0x10).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e49d0  48 8B 50 10                   MOV RDX,qword ptr [RAX + 0x10]
 * * 1404e49d4  48 3B CA                      CMP RCX,RDX
 */
const char* CONTAINER_COUNT_SIG = "[MOV r64, [r64+off8]] [CMP r64, r64]";

/**
 * @brief Pattern for the MOV r64, [r64+off8] load of the profiles array pointer (0x08).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e49d9  48 8B 78 08                   MOV RDI,qword ptr [RAX + 0x8]
 */
const char* PROFILES_ARRAY_SIG = "[MOV r64, [r64+off8]]";

/**
 * @brief Pattern for the MOVSS load of the current sun angle (env+0x3f20).
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
 * * 1404e4a5e  F3 41 0F 10 96 20 3F 00 00    MOVSS XMM2,dword ptr [R14 + 0x3f20]
 * * 1404e4a67  48 89 6C 24 20                MOV qword ptr [RSP + 0x20],RBP
 */
const char* SUN_ANGLE_SIG = "F3 41 0F 10 96 ? ? ? ? [MOV [r64+off8], r64]";

/**
 * @brief Pattern to locate the WeatherBlendProgress function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d0920[1404d0920]) ---/
 * 1404d0920  44 8B 81 78 3E 00 00       MOV R8D,dword ptr [RCX + 0x3e78]
 * 1404d0927  B8 6D C1 16 6C             MOV EAX,0x6c16c16d
 * 1404d092c  F3 0F 10 81 7C 3E 00 00    MOVSS XMM0,dword ptr [RCX + 0x3e7c]
 * 1404d0934  0F 57 C9                   XORPS XMM1,XMM1
 */
const char* WEATHER_BLEND_PROGRESS_SIG = "44 [MOV r32, [r64+off32]] [MOV r32, imm32] [MOVSS xmm, [r64+off32]] [XORPS xmm, xmm]";

/**
 * @brief Pattern for the IMUL qword ptr [rip+off32] against the transition duration global.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d0920[1404d0920]) ---/
 * 1404d09bb  48 F7 2D E6 5E 73 02       IMUL qword ptr [0x142c068a8]
 */
const char* TRANSITION_DURATION_SIG = "48 F7 2D";

/**
 * @brief Pattern to locate the BadWeatherUpdate function entry.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
 * 1404d7620  40 53                         PUSH RBX
 * 1404d7622  48 83 EC 20                   SUB RSP,0x20
 * 1404d7626  48 8B 05 13 D6 07 03          MOV RAX,qword ptr [0x143554c40]
 * 1404d762d  48 8B D9                      MOV RBX,RCX
 * 1404d7630  80 B8 EC 08 00 00 00          CMP byte ptr [RAX + 0x8ec],0x0
 * 1404d7637  0F 85 94 01 00 00             JNZ 0x1404d77d1
 * 1404d763d  48 8B 0D 8C D6 07 03          MOV RCX,qword ptr [0x143554cd0]
 */
const char* BAD_WEATHER_FN_SIG = "40 [PUSH r64] [SUB r64, imm8] [MOV r64, [rip+off32]] [MOV r64, r64] [CMP byte ptr [r64+off32], imm8] [JNE rel32] [MOV r64, [rip+off32]]";

/**
 * @brief Pattern for the LEA r64, [rip+off32] that addresses the g_bad_weather_factor container.
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
 * 1404d7689  48 8D 0D A0 13 71 02          LEA RCX,[0x142be8a30]
 */
const char* BAD_WEATHER_FACTOR_LEA_SIG = "[LEA r64, [rip+off32]]";

}  // namespace

bool ClimateDataFinder::TryFindOffsets(ClimateService& owner) {
  Utils::FinderLog log(GetName());
  log.Info("Searching for Climate and Weather data using provided Ghidra signatures...");

  bool all_found = true;

  // === SECTION 1: ENVIRONMENT STATE UPDATE ===
  // Used by: SetWeatherMode, SetRainIntensity, SetSkyboxIndex (to trigger env update)
  {
    auto phase = log.MakePhase("Environment State Update");

    // 1. Locate the function that references the unique string anchor.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
    // 140854c76  48 8D 0D E3 2C 94 01          LEA RCX,[0x142197960] = "Missing Headquarters %s"
    uintptr_t pfnEnvOwner = Utils::PatternFinder::FindFunctionByString(UPDATE_ENV_STRING, true);
    if (phase.Step(pfnEnvOwner, "Missing Headquarters function", "FN")) {
      // 1.1 First CALL rel32 inside the function body — targets UpdateEnvironmentState.
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_140854930[140854930]) ---/
      // 14085497c  E8 6F FA C7 FF                CALL 0x1404d43f0
      uintptr_t addrCall = Utils::PatternFinder::Find(pfnEnvOwner, 128, UPDATE_ENV_CALL_SIG);
      if (phase.Step(addrCall, "UpdateEnvironmentState CALL", "RT")) {
        uintptr_t pfnUpdateEnv = Utils::PatternFinder::GetRipAddress(addrCall, 1, 5);
        if (phase.Step(pfnUpdateEnv, "UpdateEnvironmentState", "FN")) {
          owner.SetUpdateFnAddr(pfnUpdateEnv);
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

  // === SECTION 2: WEATHER CONTROL ===
  // Used by: GetWeatherMode, SetWeatherMode, GetActiveProfileName, GetActiveProfilePtr
  {
    auto phase = log.MakePhase("Weather Control");

    // 2. Find SetWeather function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
    uintptr_t setWeatherFn = Utils::PatternFinder::FindFunctionByString(SET_WEATHER_STRING, true, SET_WEATHER_CONTEXT, 32);
    if (phase.Step(setWeatherFn, "SetWeather function", "FN")) {
      owner.SetSetWeatherModeFnAddr(setWeatherFn);

      // 2.1 Extract weatherModeOffset from function body (for GetWeatherMode)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
      // * 1404df417  89 91 70 3E 00 00             MOV dword ptr [RCX + 0x3e70],EDX
      // * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
      uintptr_t addrMode = Utils::PatternFinder::Find(setWeatherFn, 96, WEATHER_MODE_OFFSET_SIG);
      if (phase.StepOptional(addrMode, "Weather mode MOV pair", "RT")) {
        int32_t weatherModeOff = Utils::PatternFinder::ReadInt32(addrMode + 2);
        if (phase.StepOffsetOptional(weatherModeOff, "Weather Mode offset", "OFF")) {
          owner.SetWeatherModeOffset(weatherModeOff);
        }
      }

      // 2.2 Weather target offset (for GetNextWeatherMode)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
      // * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
      uintptr_t addrTarget = Utils::PatternFinder::Find(addrMode + 2, 16, WEATHER_TARGET_OFFSET_SIG);
      if (phase.StepOptional(addrTarget, "Weather target MOV", "RT")) {
        int32_t weatherTargetOff = Utils::PatternFinder::ReadInt32(addrTarget + 2);
        if (phase.StepOffsetOptional(weatherTargetOff, "Weather Target offset", "OFF")) {
          owner.SetNextWeatherModeOffset(weatherTargetOff);
        }
      }

      // 2.3 Remaining bad weather time offset (env+0x4570)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
      // * 1404df451  F3 0F 11 83 70 45 00 00       MOVSS dword ptr [RBX + 0x4570],XMM0
      // * 1404df459  48 C7 83 60 45 00 00 FF FF FF FF MOV qword ptr [RBX + 0x4560],-0x1
      uintptr_t addrRemain = Utils::PatternFinder::Find(addrTarget, 96, REMAINING_BAD_WEATHER_SIG);
      if (phase.Step(addrRemain, "Remaining bad weather MOVSS", "RT")) {
        int32_t remainOff = Utils::PatternFinder::ReadInt32(addrRemain + 4);
        if (phase.StepOffset(remainOff, "Remaining Bad Weather offset", "OFF")) {
          owner.SetRemainingBadWeatherOffset(remainOff);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // 2.4 Env profile data pointer offset (env+0x2ae0)
      // Used by: GetEnvProfileData (env_profile reflection attributes)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
      // 1404df559  48 8B 83 E0 2A 00 00          MOV RAX,qword ptr [RBX + 0x2ae0]
      // 1404df560  F3 0F 59 05 FC 7F EF 01       MULSS XMM0,dword ptr [0x1423d7564]
      uintptr_t addrEnvProfile = Utils::PatternFinder::Find(addrRemain, 300, ENV_PROFILE_PTR_SIG);
      if (phase.StepOptional(addrEnvProfile, "Env profile ptr MOV", "RT")) {
        int32_t envProfileOff = Utils::PatternFinder::ReadInt32(addrEnvProfile + 3);
        if (phase.StepOffsetOptional(envProfileOff, "Env Profile Ptr offset", "OFF")) {
          owner.SetEnvProfilePtrOffset(envProfileOff);
        }
      }
    } else {
      all_found = false;
    }
  }

  // === SECTION 3: CLIMATE PTR & UNIT ID ===
  // Used by: GetCurrentClimateName, GetAvailableClimates, GetActiveSunProfileIndex,
  //          GetNextSunProfileIndex, GetSunProfileCount, GetSunProfileName,
  //          GetTransitionProgress, SetRainIntensity,
  //          EnsureInitialKick, GetActiveProfilePtr
  {
    auto phase = log.MakePhase("Climate Ptr & Unit ID");

    // 3. Find UpdateActiveClimate function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
    // 1404d74d6  48 8B 81 B8 2A 00 00          MOV RAX,qword ptr [RCX + 0x2ab8]
    // 1404d74dd  48 8B F9                      MOV RDI,RCX
    // 1404d74e0  4C 8B C0                      MOV R8,RAX
    // 1404d74e3  48 89 5C 24 40                MOV qword ptr [RSP + 0x40],RBX
    // 1404d74e8  48 8D 4C 24 30                LEA RCX,[RSP + 0x30]
    // 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
    // \---
    uintptr_t pfnUpdateActiveClimate = Utils::PatternFinder::Find(UPDATE_ACTIVE_CLIMATE_SIG);
    if (phase.Step(pfnUpdateActiveClimate, "UpdateActiveClimate", "FN")) {
      // 3.1 Climate object pointer offset (instruction 1: MOV RAX, [RCX + 0x2ab8])
      // Used by: all methods that dereference m_climatePtrOffset
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
      // 1404d74d6  48 8B 81 B8 2A 00 00          MOV RAX,qword ptr [RCX + 0x2ab8]
      int32_t climateOff = Utils::PatternFinder::ReadInt32(pfnUpdateActiveClimate + 3);
      if (phase.StepOffset(climateOff, "Climate Object offset", "OFF")) {
        owner.SetClimatePtrOffset(climateOff);
      } else {
        all_found = false;
      }

      // 3.2 Climate UnitID offset
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
      // 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
      // \---
      // Used by: GetCurrentClimateName, GetAvailableClimates (climate + unitIdOff = UnitID)
      uintptr_t addrUnitId = Utils::PatternFinder::Find(pfnUpdateActiveClimate, 32, CLIMATE_UNIT_ID_SIG);
      if (phase.Step(addrUnitId, "Climate UnitID MOV", "RT")) {
        int8_t unitIdOff = Utils::PatternFinder::ReadInt8(addrUnitId + 2);
        owner.SetClimateUnitIdOffset(static_cast<intptr_t>(unitIdOff));
      } else {
        all_found = false;
      }
    } else {
      all_found = false;
    }
  }

  // === SECTION 4: SetClimate FUNCTION ===
  // Used by: ClimateService::SetClimate
  {
    auto phase = log.MakePhase("SetClimate Function");

    // 4. Find SetClimate function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetClimate[1404df5f0]) ---/
    // * 1404df709  48 8D 0D 50 84 C4 01          LEA RCX,[0x142127b60] = "Restarting environment transition, possi..."
    // * 1404df710  E8 6B 8E C1 FF                CALL 0x1400f8580
    // * 1404df715  48 8B 03                      MOV RAX,qword ptr [RBX]
    uintptr_t setClimateFn = Utils::PatternFinder::FindFunctionByString(SET_CLIMATE_STRING, true, SET_CLIMATE_CONTEXT, 16);
    if (phase.StepOptional(setClimateFn, "SetClimate function", "FN")) {
      owner.SetSetClimateFnAddr(setClimateFn);
    }
  }

  // === SECTION 5: CLIMATE ARRAY (GetAvailableClimates) ===
  // Used by: GetAvailableClimates
  {
    auto phase = log.MakePhase("Climate Array");

    // 5. Find FindClimateByToken via its error string
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
    // * 1404e45ad  48 8D 0D 9C 40 C4 01          LEA RCX,[0x142128650] = "Unknown climate %s"
    uintptr_t pfnFindClimateByToken = Utils::PatternFinder::FindFunctionByString(FIND_CLIMATE_STR, true);
    if (phase.Step(pfnFindClimateByToken, "FindClimateByToken", "FN")) {
      // 5.1 Climate array pointer offset (param_1 + 0x130)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
      // * 1404e4525  48 8B B9 30 01 00 00          MOV RDI,qword ptr [RCX + 0x130]
      uintptr_t addrArr = Utils::PatternFinder::Find(pfnFindClimateByToken, 32, CLIMATE_ARRAY_SIG);
      if (phase.Step(addrArr, "Climate array MOV", "RT")) {
        int32_t arrOff = Utils::PatternFinder::ReadInt32(addrArr + 3);
        if (phase.StepOffset(arrOff, "Climate Array offset", "OFF")) {
          owner.SetClimateArrayOffset(arrOff);

          // 5.2 Climate count offset (param_1 + 0x138) — search past addrArr to avoid re-finding instr 1
          // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FindClimateByToken[1404e4510]) ---/
          // * 1404e452f  48 8B 81 38 01 00 00          MOV RAX,qword ptr [RCX + 0x138]
          uintptr_t addrCount = Utils::PatternFinder::Find(addrArr + 3, 32, CLIMATE_COUNT_SIG);
          if (phase.Step(addrCount, "Climate count MOV", "RT")) {
            int32_t countOff = Utils::PatternFinder::ReadInt32(addrCount + 3);
            if (phase.StepOffset(countOff, "Climate Count offset", "OFF")) {
              owner.SetClimateCountOffset(countOff);
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

  // === SECTION 6: SUN PROFILE ACTIVE/NEXT INDICES ===
  // Used by: GetActiveSunProfileIndex, GetNextSunProfileIndex, GetTransitionProgress
  {
    auto phase = log.MakePhase("Sun Profile Active/Next Indices");

    // 6. Find SunProfileUpdate function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    // 1404e4940  44 88 44 24 18                MOV byte ptr [RSP + 0x18],R8B
    // 1404e4945  41 54                         PUSH R12
    // 1404e4947  41 56                         PUSH R14
    uintptr_t pfnSunProfileUpdate = Utils::PatternFinder::Find(SUN_PROFILE_UPDATE_SIG);
    if (phase.Step(pfnSunProfileUpdate, "SunProfileUpdate", "FN")) {
      // 6.1 Active profile index offset
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e49c9  41 8B 8E 60 45 00 00          MOV ECX,dword ptr [R14 + 0x4560]
      uintptr_t addrActive = Utils::PatternFinder::Find(pfnSunProfileUpdate, 160, ACTIVE_PROFILE_INDEX_SIG);
      if (phase.Step(addrActive, "Active profile index MOV", "RT")) {
        int32_t activeOff = Utils::PatternFinder::ReadInt32(addrActive + 3);
        if (phase.StepOffset(activeOff, "Active Profile Index offset", "OFF")) {
          owner.SetActiveProfileIndexOffset(activeOff);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // 6.2 Next profile index offset
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e49e1  41 8B 8E 64 45 00 00          MOV ECX,dword ptr [R14 + 0x4564]
      uintptr_t addrNext = Utils::PatternFinder::Find(addrActive + 3, 32, NEXT_PROFILE_INDEX_SIG);
      if (phase.Step(addrNext, "Next profile index MOV", "RT")) {
        int32_t nextOff = Utils::PatternFinder::ReadInt32(addrNext + 3);
        if (phase.StepOffset(nextOff, "Next Profile Index offset", "OFF")) {
          owner.SetNextProfileIndexOffset(nextOff);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // 6.3 Nice container offset (0xd0 — first set of sun profiles, "nice" variants)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e49be  B8 D0 00 00 00                MOV EAX,0xd0
      uintptr_t addrNice = Utils::PatternFinder::FindBackward(addrActive, 32, NICE_CONTAINER_SIG);
      if (phase.Step(addrNice, "Nice container MOV", "RT")) {
        int32_t niceOff = Utils::PatternFinder::ReadInt32(addrNice + 1);
        if (phase.StepOffset(niceOff, "Nice Container offset", "OFF")) {
          owner.SetContainerNiceOffset(niceOff);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // 6.4 Bad container offset (0x120 — second set of sun profiles, "bad" variants)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e498a  BA 20 01 00 00                MOV EDX,0x120
      // * 1404e498f  4D 3B BE 38 01 00 00          CMP R15,qword ptr [R14 + 0x138]
      uintptr_t addrBad = Utils::PatternFinder::FindBackward(addrNice, 64, BAD_CONTAINER_SIG);
      if (phase.Step(addrBad, "Bad container MOV", "RT")) {
        int32_t badOff = Utils::PatternFinder::ReadInt32(addrBad + 1);
        if (phase.StepOffset(badOff, "Bad Container offset", "OFF")) {
          owner.SetContainerBadOffset(badOff);
        } else {
          all_found = false;
        }
      } else {
        all_found = false;
      }

      // 6.5 Container count offset (0x10 — number of profiles in container)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e49d0  48 8B 50 10                   MOV RDX,qword ptr [RAX + 0x10]
      // * 1404e49d4  48 3B CA                      CMP RCX,RDX
      uintptr_t addrContainerCount = Utils::PatternFinder::Find(addrNice, 32, CONTAINER_COUNT_SIG);
      if (phase.Step(addrContainerCount, "Container count MOV", "RT")) {
        int8_t countOff = Utils::PatternFinder::ReadInt8(addrContainerCount + 3);
        owner.SetContainerCountOffset(countOff);
      } else {
        all_found = false;
      }

      // 6.6 Profiles array offset (0x08 — pointer to profile pointers array in container)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e49d9  48 8B 78 08                   MOV RDI,qword ptr [RAX + 0x8]
      uintptr_t addrProfilesArray = Utils::PatternFinder::Find(addrContainerCount + 3, 16, PROFILES_ARRAY_SIG);
      if (phase.Step(addrProfilesArray, "Profiles array MOV", "RT")) {
        int8_t arrOff = Utils::PatternFinder::ReadInt8(addrProfilesArray + 3);
        owner.SetProfilesArrayOffset(arrOff);
      } else {
        all_found = false;
      }

      // 6.7 Sun angle offset (0x3F20 — current sun angle in env)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
      // * 1404e4a5e  F3 41 0F 10 96 20 3F 00 00    MOVSS XMM2,dword ptr [R14 + 0x3f20]
      // * 1404e4a67  48 89 6C 24 20                MOV qword ptr [RSP + 0x20],RBP
      uintptr_t addrSunAngle = Utils::PatternFinder::Find(addrNice, 180, SUN_ANGLE_SIG);
      if (phase.Step(addrSunAngle, "Sun angle MOVSS", "RT")) {
        int32_t sunAngleOff = Utils::PatternFinder::ReadInt32(addrSunAngle + 5);
        if (phase.StepOffset(sunAngleOff, "Sun Angle offset", "OFF")) {
          owner.SetSunAngleOffset(sunAngleOff);
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

  // === SECTION 7: WEATHER BLEND PROGRESS ===
  // Used by: GetWeatherBlendProgress, SetTransitionDuration
  {
    auto phase = log.MakePhase("Weather Blend Progress");

    // 7. Find WeatherBlendProgress function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d0920[1404d0920]) ---/
    // * 1404d0920  44 8B 81 78 3E 00 00       MOV R8D,dword ptr [RCX + 0x3e78]
    // * 1404d0927  B8 6D C1 16 6C             MOV EAX,0x6c16c16d
    // * 1404d092c  F3 0F 10 81 7C 3E 00 00    MOVSS XMM0,dword ptr [RCX + 0x3e7c]
    // * 1404d0934  0F 57 C9                   XORPS XMM1,XMM1
    uintptr_t pfnWeatherBlend = Utils::PatternFinder::Find(WEATHER_BLEND_PROGRESS_SIG);
    if (phase.Step(pfnWeatherBlend, "WeatherBlendProgress", "FN")) {
      owner.SetWeatherBlendProgressFnAddr(pfnWeatherBlend);

      // 7.1 Transition duration global (for SetTransitionDuration)
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d0920[1404d0920]) ---/
      // * 1404d09bb  48 F7 2D E6 5E 73 02    IMUL qword ptr [0x142c068a8]
      uintptr_t addrDuration = Utils::PatternFinder::Find(pfnWeatherBlend, 160, TRANSITION_DURATION_SIG);
      if (phase.StepOptional(addrDuration, "Transition duration IMUL", "RT")) {
        uintptr_t durGlobal = Utils::PatternFinder::GetRipAddress(addrDuration, 3, 7);
        if (phase.StepOptional(durGlobal, "Transition duration global", "DATA")) {
          owner.SetTransitionDurationAddr(durGlobal);
        }
      }
    } else {
      all_found = false;
    }
  }

  // === SECTION 8: BAD WEATHER FACTOR (g_bad_weather_factor) ===
  // Used by: GetBadWeatherFactor, SetBadWeatherFactor
  {
    auto phase = log.MakePhase("Bad Weather Factor");

    // 8. Find BadWeatherUpdate function entry.
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
    // * 1404d7620  40 53                         PUSH RBX
    // * 1404d7622  48 83 EC 20                   SUB RSP,0x20
    // * 1404d7626  48 8B 05 13 D6 07 03          MOV RAX,qword ptr [0x143554c40]
    // * 1404d762d  48 8B D9                      MOV RBX,RCX
    // * 1404d7630  80 B8 EC 08 00 00 00          CMP byte ptr [RAX + 0x8ec],0x0
    // * 1404d7637  0F 85 94 01 00 00             JNZ 0x1404d77d1
    // * 1404d763d  48 8B 0D 8C D6 07 03          MOV RCX,qword ptr [0x143554cd0]
    uintptr_t pfnBadWeather = Utils::PatternFinder::Find(BAD_WEATHER_FN_SIG);
    if (phase.Step(pfnBadWeather, "BadWeatherUpdate", "FN")) {
      // 8.1 Pointer-to-pointer to the g_bad_weather_factor container.
      // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
      // * 1404d7689  48 8D 0D A0 13 71 02       LEA RCX,[0x142be8a30]
      uintptr_t addrLea = Utils::PatternFinder::Find(pfnBadWeather, 128, BAD_WEATHER_FACTOR_LEA_SIG);
      if (phase.Step(addrLea, "BadWeatherFactor LEA", "RT")) {
        uintptr_t ptrPtr = Utils::PatternFinder::GetRipAddress(addrLea, 3, 7);
        if (phase.Step(ptrPtr, "BadWeatherFactor ptr-to-ptr", "DATA")) {
          owner.SetBadWeatherFactorPtr(ptrPtr);
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

  // === SECTION 10: SUN PROFILE REFLECTION ATTRIBUTES ===
  // Resolved via SCS reflection (FindAttributeOffset), independent of hardcoded game version.
  // Best-effort: individual attributes may fail without affecting overall finder readiness.
  {
    auto phase = log.MakePhase("Sun Profile Reflection Attributes");

    auto getAndSet = [&](const char* attrName, auto&& setter) -> bool {
      uintptr_t off = Utils::PatternFinder::FindAttributeOffset("sun_profile", attrName);
      bool ok = phase.StepOffset(static_cast<int32_t>(off), attrName, "REF");
      if (ok) setter(static_cast<intptr_t>(off));
      return ok;
    };

    getAndSet("low_elevation", [&](auto v) { owner.SetLowElevationOffset(v); });
    getAndSet("high_elevation", [&](auto v) { owner.SetHighElevationOffset(v); });
    getAndSet("sun_direction", [&](auto v) { owner.SetSunDirectionOffset(v); });

    getAndSet("temperature", [&](auto v) { owner.SetTemperatureOffset(v); });
    getAndSet("skybox_texture", [&](auto v) { owner.SetSkyboxTextureOffset(v); });
    getAndSet("skycloud_mask_texture", [&](auto v) { owner.SetSkycloudMaskTextureOffset(v); });
    getAndSet("lightning_mask", [&](auto v) { owner.SetLightningMaskOffset(v); });
    getAndSet("stars_texture", [&](auto v) { owner.SetStarsTextureOffset(v); });
    getAndSet("mirror_sky_texture", [&](auto v) { owner.SetMirrorSkyTextureOffset(v); });
    getAndSet("ambient", [&](auto v) { owner.SetAmbientOffset(v); });
    getAndSet("diffuse", [&](auto v) { owner.SetDiffuseOffset(v); });
    getAndSet("specular", [&](auto v) { owner.SetSpecularOffset(v); });
    getAndSet("env", [&](auto v) { owner.SetEnvOffset(v); });
    getAndSet("env_static_mod", [&](auto v) { owner.SetEnvStaticModOffset(v); });
    getAndSet("sky_color", [&](auto v) { owner.SetSkyColorOffset(v); });
    getAndSet("sky_bottom_color", [&](auto v) { owner.SetSkyBottomColorOffset(v); });
    getAndSet("starmap_color", [&](auto v) { owner.SetStarmapColorOffset(v); });
    getAndSet("stars_color", [&](auto v) { owner.SetStarsColorOffset(v); });
    getAndSet("sun_color", [&](auto v) { owner.SetSunColorOffset(v); });
    getAndSet("sun_opacity", [&](auto v) { owner.SetSunOpacityOffset(v); });
    getAndSet("sun_halo_color", [&](auto v) { owner.SetSunHaloColorOffset(v); });
    getAndSet("sun_shadow_strength", [&](auto v) { owner.SetSunShadowStrengthOffset(v); });
    getAndSet("moon_color", [&](auto v) { owner.SetMoonColorOffset(v); });
    getAndSet("moon_halo_color", [&](auto v) { owner.SetMoonHaloColorOffset(v); });
    getAndSet("moon_halo_scale", [&](auto v) { owner.SetMoonHaloScaleOffset(v); });
    getAndSet("fog_color", [&](auto v) { owner.SetFogColorOffset(v); });
    getAndSet("fog_color2", [&](auto v) { owner.SetFogColor2Offset(v); });
    getAndSet("fog_vgradient", [&](auto v) { owner.SetFogVgradientOffset(v); });
    getAndSet("fog_offset", [&](auto v) { owner.SetFogOffsetOffset(v); });
    getAndSet("fog_density", [&](auto v) { owner.SetFogDensityOffset(v); });
    getAndSet("speed_coef", [&](auto v) { owner.SetSpeedCoefOffset(v); });
    getAndSet("cloud_shadow_weight", [&](auto v) { owner.SetCloudShadowWeightOffset(v); });
    getAndSet("cloud_shadow_texture", [&](auto v) { owner.SetCloudShadowTextureOffset(v); });
    getAndSet("cloud_shadow_area_size", [&](auto v) { owner.SetCloudShadowAreaSizeOffset(v); });
    getAndSet("cloud_shadow_speed", [&](auto v) { owner.SetCloudShadowSpeedOffset(v); });
    getAndSet("rain_intensity", [&](auto v) { owner.SetRainIntensityOffset(v); });
    getAndSet("lightning_intensity", [&](auto v) { owner.SetLightningIntensityOffset(v); });
    getAndSet("rain_max_wetness", [&](auto v) { owner.SetRainMaxWetnessOffset(v); });
    getAndSet("rain_additional_ambient", [&](auto v) { owner.SetRainAdditionalAmbientOffset(v); });
    getAndSet("snow_intensity", [&](auto v) { owner.SetSnowIntensityOffset(v); });
    getAndSet("snow_flake_size_range", [&](auto v) { owner.SetSnowFlakeSizeRangeOffset(v); });
    getAndSet("snow_chaos_rate", [&](auto v) { owner.SetSnowChaosRateOffset(v); });
    getAndSet("snow_chaos_weight", [&](auto v) { owner.SetSnowChaosWeightOffset(v); });
    getAndSet("snow_additional_ambient", [&](auto v) { owner.SetSnowAdditionalAmbientOffset(v); });
    getAndSet("wind_type", [&](auto v) { owner.SetWindTypeOffset(v); });
    getAndSet("dof_start", [&](auto v) { owner.SetDofStartOffset(v); });
    getAndSet("dof_transition", [&](auto v) { owner.SetDofTransitionOffset(v); });
    getAndSet("dof_filter_size", [&](auto v) { owner.SetDofFilterSizeOffset(v); });
    getAndSet("color_balance", [&](auto v) { owner.SetColorBalanceOffset(v); });
    getAndSet("color_saturation", [&](auto v) { owner.SetColorSaturationOffset(v); });
    getAndSet("sunshaft_color", [&](auto v) { owner.SetSunshaftColorOffset(v); });
    getAndSet("sunshaft_size", [&](auto v) { owner.SetSunshaftSizeOffset(v); });
    getAndSet("low_intensity_minimum", [&](auto v) { owner.SetLowIntensityMinimumOffset(v); });
    getAndSet("low_intensity_maximum", [&](auto v) { owner.SetLowIntensityMaximumOffset(v); });
    getAndSet("low_intensity_color", [&](auto v) { owner.SetLowIntensityColorOffset(v); });
    getAndSet("min_scale", [&](auto v) { owner.SetMinScaleOffset(v); });
    getAndSet("max_scale", [&](auto v) { owner.SetMaxScaleOffset(v); });
    getAndSet("scale_override", [&](auto v) { owner.SetScaleOverrideOffset(v); });
    getAndSet("dark_adaptation_speed", [&](auto v) { owner.SetDarkAdaptationSpeedOffset(v); });
    getAndSet("bright_adaptation_speed", [&](auto v) { owner.SetBrightAdaptationSpeedOffset(v); });
    getAndSet("target_gray", [&](auto v) { owner.SetTargetGrayOffset(v); });
    getAndSet("contrast", [&](auto v) { owner.SetContrastOffset(v); });
    getAndSet("shoulder_length", [&](auto v) { owner.SetShoulderLengthOffset(v); });
    getAndSet("bloom_threshold", [&](auto v) { owner.SetBloomThresholdOffset(v); });
    getAndSet("bloom_limit", [&](auto v) { owner.SetBloomLimitOffset(v); });
    getAndSet("bloom_intensity", [&](auto v) { owner.SetBloomIntensityOffset(v); });
    getAndSet("bloom_standard_deviation", [&](auto v) { owner.SetBloomStandardDeviationOffset(v); });
    getAndSet("stability", [&](auto v) { owner.SetStabilityOffset(v); });
    getAndSet("weight", [&](auto v) { owner.SetWeightOffset(v); });
  }

  // === SECTION 10b: ENV PROFILE REFLECTION ATTRIBUTES ===
  // Fields from env_profile : env.data unit (environment-level settings, not per-sun-profile).
  {
    auto phase = log.MakePhase("Env Profile Reflection Attributes");

    auto getAndSetEnv = [&](const char* attrName, auto&& setter) -> bool {
      uintptr_t off = Utils::PatternFinder::FindAttributeOffset("env_profile", attrName);
      bool ok = phase.StepOffset(static_cast<int32_t>(off), attrName, "REF");
      if (ok) setter(static_cast<intptr_t>(off));
      return ok;
    };

    getAndSetEnv("lamps_on_elevation", [&](auto v) { owner.SetLampsOnElevationOffset(v); });
    getAndSetEnv("day_in_year", [&](auto v) { owner.SetDayInYearOffset(v); });
    getAndSetEnv("summer_time", [&](auto v) { owner.SetSummerTimeOffset(v); });
    getAndSetEnv("thunderstorm_probability", [&](auto v) { owner.SetThunderstormProbabilityOffset(v); });
  }

  m_isReady = all_found &&
              (owner.GetUpdateFnAddr() != 0 && owner.GetSetWeatherModeFnAddr() != 0 && owner.GetRemainingBadWeatherOffset() != 0 && owner.GetClimatePtrOffset() != 0 && owner.GetClimateUnitIdOffset() != 0 &&
               owner.GetClimateArrayOffset() != 0 && owner.GetClimateCountOffset() != 0 && owner.GetActiveProfileIndexOffset() != 0 && owner.GetNextProfileIndexOffset() != 0 && owner.GetContainerNiceOffset() != 0 &&
               owner.GetContainerBadOffset() != 0 && owner.GetContainerCountOffset() != 0 && owner.GetProfilesArrayOffset() != 0 && owner.GetSunAngleOffset() != 0 && owner.GetWeatherBlendProgressFnAddr() != 0 &&
               owner.GetBadWeatherFactorPtr() != 0);

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
