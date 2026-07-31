#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>
#include <string>

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

}  // namespace

bool ClimateDataFinder::TryFindOffsets(ClimateService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Climate and Weather data using provided Ghidra signatures...");

  bool all_found = true;
  uintptr_t addr = 0;

  // === SECTION 1: ENVIRONMENT STATE UPDATE ===
  // Used by: SetWeatherMode, SetRainIntensity, SetSkyboxIndex (to trigger env update)
  {
    Utils::FinderLog log(GetName());
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

  // 2. Find SetWeather function entry.
  const char* SET_WEATHER_STRING = "Restarting environment transition, possibly upcoming blending errors.";
  const char* SET_WEATHER_CONTEXT = "[MOV r32, [r64+off32]] [MOV r32, imm32] [MOVSS xmm, [r64+off32]]";
  uintptr_t SetWeatherFn = Utils::PatternFinder::FindFunctionByString(SET_WEATHER_STRING, true, SET_WEATHER_CONTEXT, 32);

  if (!SetWeatherFn) {
    logger->Error("3. Failed to find SetWeather function start.");
    all_found = false;
  } else {
    logger->Info("3. [CALL: SetWeather] Found at 0x{:X}", SetWeatherFn);
    owner.SetSetWeatherModeFnAddr(SetWeatherFn);

    // Extract weatherModeOffset from function body (for GetWeatherMode)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
    // * 1404df417  89 91 70 3E 00 00             MOV dword ptr [RCX + 0x3e70],EDX
    // * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
    addr = Utils::PatternFinder::Find(SetWeatherFn, 96, "[MOV [r64+off32], r32] [MOV [r64+off32], r32]");
    if (addr) {
      int32_t weatherModeOff = Utils::PatternFinder::ReadInt32(addr + 2);
      if (Utils::PatternFinder::IsSaneOffset(weatherModeOff)) {
        owner.SetWeatherModeOffset(weatherModeOff);
        logger->Debug("3.1 [OFFSET: Weather Mode] Found: 0x{:X}", weatherModeOff);
      }
    }

    // 2.2 Weather target offset (for GetNextWeatherMode)
    // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
    // * 1404df41d  89 91 74 3E 00 00             MOV dword ptr [RCX + 0x3e74],EDX
    uintptr_t addrTarget = Utils::PatternFinder::Find(addr + 2, 16, "[MOV [r64+off32], r32]");
    if (addrTarget) {
      int32_t weatherTargetOff = Utils::PatternFinder::ReadInt32(addrTarget + 2);
      if (Utils::PatternFinder::IsSaneOffset(weatherTargetOff)) {
        owner.SetNextWeatherModeOffset(weatherTargetOff);
        logger->Debug("3.2 [OFFSET: Weather Target] Found: 0x{:X}", weatherTargetOff);
      }
    }

    // 2.3 Remaining bad weather time offset (env+0x4570)
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
    // * 1404df451  F3 0F 11 83 70 45 00 00       MOVSS dword ptr [RBX + 0x4570],XMM0
    // * 1404df459  48 C7 83 60 45 00 00 FF FF FF FF MOV qword ptr [RBX + 0x4560],-0x1
    uintptr_t addrRemain = Utils::PatternFinder::Find(addrTarget, 96, "[MOVSS [r64+off32], xmm] [MOV qword ptr [r64+off32], imm64_32]");
    if (addrRemain) {
      int32_t remainOff = Utils::PatternFinder::ReadInt32(addrRemain + 4);
      if (Utils::PatternFinder::IsSaneOffset(remainOff)) {
        owner.SetRemainingBadWeatherOffset(remainOff);
        logger->Debug("3.3 [OFFSET: Remaining Bad Weather] Found: 0x{:X}", remainOff);
      } else {
        logger->Error("3.3 [OFFSET: Remaining Bad Weather] Offset (0x{:X}) is insane.", remainOff);
        all_found = false;
      }
    } else {
      logger->Error("3.3 [OFFSET: Remaining Bad Weather] FAILED to find signature.");
      all_found = false;
    }

    // 2.4 Env profile data pointer offset (env+0x2ae0)
    // Used by: GetEnvProfileData (env_profile reflection attributes)
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(SetWeather[1404df3e0]) ---/
    // 1404df559  48 8B 83 E0 2A 00 00          MOV RAX,qword ptr [RBX + 0x2ae0]
    // 1404df560  F3 0F 59 05 FC 7F EF 01       MULSS XMM0,dword ptr [0x1423d7564]
    uintptr_t addrEnvProfile = Utils::PatternFinder::Find(addrRemain, 300, "[MOV r64, [r64+off32]] F3");
    if (addrEnvProfile) {
      int32_t envProfileOff = Utils::PatternFinder::ReadInt32(addrEnvProfile + 3);
      if (Utils::PatternFinder::IsSaneOffset(envProfileOff)) {
        owner.SetEnvProfilePtrOffset(envProfileOff);
        logger->Debug("3.4 [OFFSET: Env Profile Ptr] Found: 0x{:X}", envProfileOff);
      } else {
        logger->Error("3.4 [OFFSET: Env Profile Ptr] Offset (0x{:X}) is insane.", envProfileOff);
      }
    } else {
      logger->Warn("3.4 [OFFSET: Env Profile Ptr] FAILED to find signature — using hardcoded 0x2ae0.");
    }
  }

  // === SECTION 4: CLIMATE PTR & UNIT ID ===
  // Used by: GetCurrentClimateName, GetAvailableClimates, GetActiveSunProfileIndex,
  //          GetNextSunProfileIndex, GetSunProfileCount, GetSunProfileName,
  //          GetTransitionProgress, SetRainIntensity,
  //          EnsureInitialKick, GetActiveProfilePtr

  // /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
  // 1404d74d6  48 8B 81 B8 2A 00 00          MOV RAX,qword ptr [RCX + 0x2ab8]
  // 1404d74dd  48 8B F9                      MOV RDI,RCX
  // 1404d74e0  4C 8B C0                      MOV R8,RAX
  // 1404d74e3  48 89 5C 24 40                MOV qword ptr [RSP + 0x40],RBX
  // 1404d74e8  48 8D 4C 24 30                LEA RCX,[RSP + 0x30]
  // 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
  // \---
  const char* UPDATE_ACTIVE_CLIMATE_SIG = "[MOV r64, [r64+off32]] [MOV r64, r64] [MOV r64, r64] [MOV [r64+off8], r64] [LEA r64, [r64+off8]] [MOV r32, [r64+off8]]";
  uintptr_t pfnUpdateActiveClimate = Utils::PatternFinder::Find(UPDATE_ACTIVE_CLIMATE_SIG);

  if (pfnUpdateActiveClimate) {
    logger->Debug("4. UpdateActiveClimate found at 0x{:X}", pfnUpdateActiveClimate);
  } else {
    logger->Error("4. UpdateActiveClimate FAILED to find function start.");
    all_found = false;
  }

  // 4.1 Climate object pointer offset (instruction 1: MOV RAX, [RCX + 0x2ab8])
  // Used by: all methods that dereference m_climatePtrOffset
  if (pfnUpdateActiveClimate) {
    int32_t climateOff = Utils::PatternFinder::ReadInt32(pfnUpdateActiveClimate + 3);
    if (Utils::PatternFinder::IsSaneOffset(climateOff)) {
      owner.SetClimatePtrOffset(climateOff);
      logger->Debug("4.1 [OFFSET: Climate Object] Found: 0x{:X}", climateOff);
    } else {
      logger->Error("4.1 [OFFSET: Climate Object] Offset (0x{:X}) is insane.", climateOff);
      all_found = false;
    }

    // 4.2 Climate UnitID offset
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateActiveClimate[1404d74d0]) ---/
    // 1404d74ed  8B 50 0C                      MOV EDX,dword ptr [RAX + 0xc]
    // \---
    // Used by: GetCurrentClimateName, GetAvailableClimates (climate + unitIdOff = UnitID)
    uintptr_t addrUnitId = Utils::PatternFinder::Find(pfnUpdateActiveClimate, 64, "[MOV r32, [r64+off8]]");
    if (addrUnitId) {
      int8_t unitIdOff = Utils::PatternFinder::ReadInt8(addrUnitId + 2);
      owner.SetClimateUnitIdOffset(static_cast<intptr_t>(unitIdOff));
      logger->Debug("4.2 [OFFSET: Climate UnitID] Found: 0x{:X}", unitIdOff);
    } else {
      logger->Error("4.2 [OFFSET: Climate UnitID] FAILED to find signature.");
      all_found = false;
    }
  } else {
    logger->Error("4.1/4.2 [OFFSET: Climate Object/UnitID] Cannot search - UpdateActiveClimate is NULL.");
    all_found = false;
  }

  // === SECTION 5: SetClimate FUNCTION ===
  // Used by: ClimateService::SetClimate

  const char* SET_CLIMATE_SIG = "Restarting environment transition, possibly upcoming blending errors.";
  uintptr_t setClimateFn = Utils::PatternFinder::FindFunctionByString(SET_CLIMATE_SIG, true, "48 8b 03 48", 16);
  if (setClimateFn) {
    owner.SetSetClimateFnAddr(setClimateFn);
    logger->Info("5. [CALL: SetClimate] Found at: 0x{:X}", setClimateFn);
  } else {
    logger->Warn("5. [CALL: SetClimate] NOT found using flexible signature.");
  }

  // === SECTION 6: CLIMATE ARRAY (GetAvailableClimates) ===
  // Used by: GetAvailableClimates

  // 6. Find FindClimateByToken via its error string
  const char* FIND_CLIMATE_STR = "Unknown climate %s";
  uintptr_t pfnFindClimateByToken = Utils::PatternFinder::FindFunctionByString(FIND_CLIMATE_STR, true);

  if (pfnFindClimateByToken) {
    logger->Debug("6. FindClimateByToken found at 0x{:X}", pfnFindClimateByToken);
  } else {
    logger->Error("6. FindClimateByToken FAILED to find function start.");
    all_found = false;
  }

  if (pfnFindClimateByToken) {
    // 6.1 Climate array pointer offset (param_1 + 0x130)
    uintptr_t addrArr = Utils::PatternFinder::Find(pfnFindClimateByToken, 32, "[MOV r64, [r64+off32]]");
    if (addrArr) {
      int32_t arrOff = Utils::PatternFinder::ReadInt32(addrArr + 3);
      if (Utils::PatternFinder::IsSaneOffset(arrOff)) {
        owner.SetClimateArrayOffset(arrOff);
        logger->Debug("6.1 [OFFSET: Climate Array] Found: 0x{:X}", arrOff);

        // 6.2 Climate count offset (param_1 + 0x138) — search past addrArr to avoid re-finding instr 1
        uintptr_t addrCount = Utils::PatternFinder::Find(addrArr + 1, 32, "[MOV r64, [r64+off32]]");
        if (addrCount) {
          int32_t countOff = Utils::PatternFinder::ReadInt32(addrCount + 3);
          if (Utils::PatternFinder::IsSaneOffset(countOff)) {
            owner.SetClimateCountOffset(countOff);
            logger->Debug("6.2 [OFFSET: Climate Count] Found: 0x{:X}", countOff);
          } else {
            logger->Error("6.2 [OFFSET: Climate Count] Offset (0x{:X}) is insane.", countOff);
            all_found = false;
          }
        } else {
          logger->Error("6.2 [OFFSET: Climate Count] FAILED to find signature.");
          all_found = false;
        }
      } else {
        logger->Error("6.1 [OFFSET: Climate Array] Offset (0x{:X}) is insane.", arrOff);
        all_found = false;
      }
    } else {
      logger->Error("6.1 [OFFSET: Climate Array] FAILED to find signature.");
      all_found = false;
    }
  }

  // === SECTION 7: SUN PROFILE ACTIVE/NEXT INDICES ===
  // Used by: GetActiveSunProfileIndex, GetNextSunProfileIndex, GetTransitionProgress

  // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
  // 1404e4940  44 88 44 24 18                MOV byte ptr [RSP + 0x18],R8B
  // 1404e4945  41 54                         PUSH R12
  // 1404e4947  41 56                         PUSH R14
  // 1404e4949  41 57                         PUSH R15
  // 1404e494b  48 83 EC 60                   SUB RSP,0x60
  // 1404e494f  4C 8B F1                      MOV R14,RCX
  // 1404e4952  48 81 C1 E8 2A 00 00          ADD RCX,0x2ae8
  // 1404e4959  E8 92 E9 E2 00                CALL 0x1413132f0
  // 1404e495e  45 33 FF                      XOR R15D,R15D
  // \---
  const char* SUN_PROFILE_UPDATE_SIG = "44 [MOV [r64+off8], r8] [PUSH R8-R15] [PUSH R8-R15] [PUSH R8-R15] [SUB r64, imm8] [MOV r64, r64] [ADD r64, imm32] [CALL rel32] 45 [XOR r32, r32]";
  uintptr_t pfnSunProfileUpdate = Utils::PatternFinder::Find(SUN_PROFILE_UPDATE_SIG);

  if (pfnSunProfileUpdate) {
    logger->Debug("7. [CALL: SunProfileUpdate] Found at 0x{:X}", pfnSunProfileUpdate);

    // 7.1 Active profile index offset
    //--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49c9  41 8B 8E 60 45 00 00          MOV ECX,dword ptr [R14 + 0x4560]
    uintptr_t addrActive = Utils::PatternFinder::Find(pfnSunProfileUpdate, 160, "41 [MOV r32, [r64+off32]]");
    if (addrActive) {
      int32_t activeOff = Utils::PatternFinder::ReadInt32(addrActive + 3);
      if (Utils::PatternFinder::IsSaneOffset(activeOff)) {
        owner.SetActiveProfileIndexOffset(activeOff);
        logger->Debug("7.1 [OFFSET: Active Profile Index] Found: 0x{:X}", activeOff);
      } else {
        logger->Error("7.1 [OFFSET: Active Profile Index] Offset (0x{:X}) is insane.", activeOff);
        all_found = false;
      }
    } else {
      logger->Error("7.1 [OFFSET: Active Profile Index] FAILED to find signature.");
      all_found = false;
    }

    // 7.2 Next profile index offset
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49e1  41 8B 8E 64 45 00 00          MOV ECX,dword ptr [R14 + 0x4564]
    uintptr_t addrNext = Utils::PatternFinder::Find(addrActive + 1, 32, "41 [MOV r32, [r64+off32]]");
    if (addrNext) {
      int32_t nextOff = Utils::PatternFinder::ReadInt32(addrNext + 3);
      if (Utils::PatternFinder::IsSaneOffset(nextOff)) {
        owner.SetNextProfileIndexOffset(nextOff);
        logger->Debug("7.2 [OFFSET: Next Profile Index] Found: 0x{:X}", nextOff);
      } else {
        logger->Error("7.2 [OFFSET: Next Profile Index] Offset (0x{:X}) is insane.", nextOff);
        all_found = false;
      }
    } else {
      logger->Error("7.2 [OFFSET: Next Profile Index] FAILED to find signature.");
      all_found = false;
    }

    // 7.3 Nice container offset (0xd0 — first set of sun profiles, "nice" variants)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49be  B8 D0 00 00 00                MOV EAX,0xd0
    uintptr_t addrNice = Utils::PatternFinder::FindBackward(addrNext, 64, "[MOV r32, imm32]");
    if (addrNice) {
      int32_t niceOff = Utils::PatternFinder::ReadInt32(addrNice + 1);
      if (Utils::PatternFinder::IsSaneOffset(niceOff)) {
        owner.SetContainerNiceOffset(niceOff);
        logger->Debug("7.4 [OFFSET: Nice Container] Found: 0x{:X}", niceOff);
      } else {
        logger->Error("7.4 [OFFSET: Nice Container] Offset (0x{:X}) is insane.", niceOff);
        all_found = false;
      }
    } else {
      logger->Error("7.4 [OFFSET: Nice Container] FAILED to find signature.");
      all_found = false;
    }

    // 7.5 Bad container offset (0x120 — second set of sun profiles, "bad" variants)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e498a  BA 20 01 00 00                MOV EDX,0x120
    //* 1404e498f  4D 3B BE 38 01 00 00          CMP R15,qword ptr [R14 + 0x138]
    uintptr_t addrBad = Utils::PatternFinder::FindBackward(addrNice, 64, "[MOV r32, imm32] [CMP r64, [r64+off32]]");
    if (addrBad) {
      int32_t badOff = Utils::PatternFinder::ReadInt32(addrBad + 1);
      if (Utils::PatternFinder::IsSaneOffset(badOff)) {
        owner.SetContainerBadOffset(badOff);
        logger->Debug("7.5 [OFFSET: Bad Container] Found: 0x{:X}", badOff);
      } else {
        logger->Error("7.5 [OFFSET: Bad Container] Offset (0x{:X}) is insane.", badOff);
        all_found = false;
      }
    } else {
      logger->Error("7.5 [OFFSET: Bad Container] FAILED to find signature.");
      all_found = false;
    }

    // 7.6 Container count offset (0x10 — number of profiles in container)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49d0  48 8B 50 10                   MOV RDX,qword ptr [RAX + 0x10]
    //* 1404e49d4  48 3B CA                      CMP RCX,RDX
    uintptr_t addrContainerCount = Utils::PatternFinder::Find(addrNice, 32, "[MOV r64, [r64+off8]] [CMP r64, r64]");
    if (addrContainerCount) {
      int8_t countOff = Utils::PatternFinder::ReadInt8(addrContainerCount + 3);
      owner.SetContainerCountOffset(countOff);
      logger->Debug("7.6 [OFFSET: Container Count] Found: 0x{:X}", countOff);
    } else {
      logger->Error("7.6 [OFFSET: Container Count] FAILED to find signature.");
      all_found = false;
    }

    // 7.7 Profiles array offset (0x08 — pointer to profile pointers array in container)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49d9  48 8B 78 08                   MOV RDI,qword ptr [RAX + 0x8]
    uintptr_t addrProfilesArray = Utils::PatternFinder::Find(addrContainerCount + 1, 16, "[MOV r64, [r64+off8]]");
    if (addrProfilesArray) {
      int8_t arrOff = Utils::PatternFinder::ReadInt8(addrProfilesArray + 3);
      owner.SetProfilesArrayOffset(arrOff);
      logger->Debug("7.7 [OFFSET: Profiles Array] Found: 0x{:X}", arrOff);
    } else {
      logger->Error("7.7 [OFFSET: Profiles Array] FAILED to find signature.");
      all_found = false;
    }

    // 7.8 Sun angle offset (0x3F20 — current sun angle in env)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e4a5e  F3 41 0F 10 96 20 3F 00 00    MOVSS XMM2,dword ptr [R14 + 0x3f20]
    //* 1404e4a67  48 89 6C 24 20                MOV qword ptr [RSP + 0x20],RBP
    uintptr_t addrSunAngle = Utils::PatternFinder::Find(addrNice, 180, "F3 41 0F 10 96 ? ? ? ? [MOV [r64+off8], r64]");
    if (addrSunAngle) {
      int32_t sunAngleOff = Utils::PatternFinder::ReadInt32(addrSunAngle + 5);
      if (Utils::PatternFinder::IsSaneOffset(sunAngleOff)) {
        owner.SetSunAngleOffset(sunAngleOff);
        logger->Debug("7.8 [OFFSET: Sun Angle] Found: 0x{:X}", sunAngleOff);
      } else {
        logger->Error("7.8 [OFFSET: Sun Angle] Offset (0x{:X}) is insane.", sunAngleOff);
        all_found = false;
      }
    } else {
      logger->Error("7.8 [OFFSET: Sun Angle] FAILED to find signature.");
      all_found = false;
    }
  } else {
    logger->Error("7. [CALL: SunProfileUpdate] FAILED to find function start.");
    all_found = false;
  }

  // === SECTION 8: WEATHER BLEND PROGRESS ===
  // Used by: GetWeatherBlendProgress, SetTransitionDuration
  // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d0920[1404d0920]) ---/
  // * 1404d0920  44 8B 81 78 3E 00 00       MOV R8D,dword ptr [RCX + 0x3e78]
  // * 1404d0927  B8 6D C1 16 6C             MOV EAX,0x6c16c16d
  // * 1404d092c  F3 0F 10 81 7C 3E 00 00    MOVSS XMM0,dword ptr [RCX + 0x3e7c]
  // * 1404d0934  0F 57 C9                   XORPS XMM1,XMM1
  // * 1404d0937  F3 0F 59 05 FD 82 F0 01    MULSS XMM0,dword ptr [0x1423d8c3c]
  const char* WEATHER_BLEND_PROGRESS_PATTERN = "44 [MOV r32, [r64+off32]] [MOV r32, imm32] [MOVSS xmm, [r64+off32]] [XORPS xmm, xmm] F3 0F 59 05";
  uintptr_t pfnWeatherBlend = Utils::PatternFinder::Find(WEATHER_BLEND_PROGRESS_PATTERN);

  if (!pfnWeatherBlend) {
    logger->Error("8. Failed to find WeatherBlendProgress function.");
    all_found = false;
  } else {
    logger->Debug("8. [CALL: WeatherBlendProgress] Found at 0x{:X}", pfnWeatherBlend);
    owner.SetWeatherBlendProgressFnAddr(pfnWeatherBlend);

    // 8.1 Transition duration global (for SetTransitionDuration)
    // * 1404d09bb  48 F7 2D E6 5E 73 02    IMUL qword ptr [0x142c068a8]
    uintptr_t addrDuration = Utils::PatternFinder::Find(pfnWeatherBlend, 160, "48 F7 2D");
    if (addrDuration) {
      uintptr_t durGlobal = Utils::PatternFinder::GetRipAddress(addrDuration, 3, 7);
      if (durGlobal) {
        owner.SetTransitionDurationAddr(durGlobal);
        logger->Debug("8.1 [PTR: Transition Duration] Found at 0x{:X} (val: {})", durGlobal, *(uint32_t*)durGlobal);
      }
    }
  }

  // === SECTION 9: BAD WEATHER FACTOR (g_bad_weather_factor) ===
  // Used by: GetBadWeatherFactor, SetBadWeatherFactor
  //
  // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
  // * 1404d7620  40 53                         PUSH RBX
  // * 1404d7622  48 83 EC 20                   SUB RSP,0x20
  // * 1404d7626  48 8B 05 13 D6 07 03          MOV RAX,qword ptr [0x143554c40]
  // * 1404d762d  48 8B D9                      MOV RBX,RCX
  // * 1404d7630  80 B8 EC 08 00 00 00          CMP byte ptr [RAX + 0x8ec],0x0
  // * 1404d7637  0F 85 94 01 00 00             JNZ 0x1404d77d1
  // * 1404d763d  48 8B 0D 8C D6 07 03          MOV RCX,qword ptr [0x143554cd0]
  const char* BAD_WEATHER_FN_SIG = "40 [PUSH r64] [SUB r64, imm8] [MOV r64, [rip+off32]] [MOV r64, r64] [CMP byte ptr [r64+off32], imm8] [JNE rel32] [MOV r64, [rip+off32]]";
  uintptr_t pfnBadWeather = Utils::PatternFinder::Find(BAD_WEATHER_FN_SIG);

  if (pfnBadWeather) {
    logger->Debug("9. [CALL: BadWeatherUpdate] Found at 0x{:X}", pfnBadWeather);

    // 9.1 PTR_PTR_142be8a30 — контейнер g_bad_weather_factor
    // /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404d7620[1404d7620]) ---/
    // * 1404d7689  48 8D 0D A0 13 71 02       LEA RCX,[0x142be8a30]
    uintptr_t addrLea = Utils::PatternFinder::Find(pfnBadWeather, 128, "[LEA r64, [rip+off32]]");
    if (addrLea) {
      uintptr_t ptrPtr = Utils::PatternFinder::GetRipAddress(addrLea, 3, 7);
      if (ptrPtr) {
        owner.SetBadWeatherFactorPtr(ptrPtr);
        logger->Debug("9.1 [DATA: BadWeatherFactor] PTR=0x{:X}", ptrPtr);
      } else {
        logger->Error("9.1 [DATA: BadWeatherFactor] FAILED to resolve RIP address.");
        all_found = false;
      }
    } else {
      logger->Error("9.1 [DATA: BadWeatherFactor] FAILED to find LEA pattern.");
      all_found = false;
    }
  } else {
    logger->Error("9. [CALL: BadWeatherUpdate] FAILED to find function start.");
    all_found = false;
  }

  // === SECTION 10: SUN PROFILE REFLECTION ATTRIBUTES ===
  // Resolved via SCS reflection (FindAttributeOffset), independent of hardcoded game version.
  // Best-effort: individual attributes may fail without affecting overall finder readiness.
  {
    Utils::FinderLog log(GetName());
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
    Utils::FinderLog log(GetName());
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

  m_isReady = all_found;
  if (m_isReady) logger->Info("ClimateDataFinder: All weather/climate offsets found. Service is READY.");

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
