/**
 * @file ClimateDataFinder.cpp
 *
 * @todo REFACTOR (next branch): Introduce Data::GameData::CoreDataService + CoreDataFinder
 *       (rename CoreCameraDataFinder -> CoreDataFinder). One service for all shared DAT_* globals.
 *
 *       Currently 3 finders independently resolve DAT_143554ca0 (GameplayManager):
 *         - ClimateDataFinder / GameWorldDataFinder via [48-4F] 8B [05-3D] near [used_vehicles]
 *         - FreeCameraDataFinder  via 48 8B [2-12?] 0F near [used_vehicles]  ← BUG!
 *
 *       FreeCameraDataFinder stores the result in SetFreecamGlobalObjectPtr, but
 *       DAT_143554ca0 is the GameplayManager, NOT a freecam global. The pattern
 *       matches by coincidence (MOV RDI, [RIP+...] ~10 bytes after the anchor).
 *       Fix: implement CoreDataFinder, remove duplicate searches, and correct
 *       FreeCameraDataFinder to search for the real freecam global.
 */

#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

bool ClimateDataFinder::TryFindOffsets(ClimateService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Climate and Weather data using provided Ghidra signatures...");

  bool all_found = true;
  const size_t SEARCH_RANGE = 4096;
  uintptr_t addr = 0;

  // === SECTION 1: CORE ENVIRONMENT ANCHORS ===
  // Used by: all ClimateService methods (GetWeatherMode, SetWeatherMode, SetRainIntensity,
  //          GetCurrentClimateName, GetAvailableClimates, GetActiveSunProfileIndex,
  //          GetNextSunProfileIndex, GetSunProfileCount, GetSunProfileName,
  //          GetTransitionProgress, SetClimate, GetTemperature, SetTemperature,
  //          GetWeight, SetWeight, EnsureInitialKick, GetActiveProfilePtr)

  // 1. Find the entry point of the UpdateTimeAdvance function.
  const char* UPDATE_TIME_STRING = "%u:%02u";
  const char* UPDATE_TIME_CONTEXT = "89 88 88 88 [0-16?] [48-4F] 8D [05-3D]";
  uintptr_t pfnUpdateTimeAdvance = Utils::PatternFinder::FindFunctionByString(UPDATE_TIME_STRING, true, UPDATE_TIME_CONTEXT);

  if (!pfnUpdateTimeAdvance) {
    logger->Error("Failed to find UpdateTimeAdvance function start.");
    return false;
  }
  logger->Debug("1. UpdateTimeAdvance found at 0x{:X}", pfnUpdateTimeAdvance);

  // 1.1 Environment Object offset
  // Used by: all methods reading env object (GetWeatherMode, SetWeatherMode, etc.)
  const char* p_obj_offset = "[48-4F] 8B [80-BF] ?? ?? ?? ?? 49 8B [C0-CF]";
  uintptr_t addrObj = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_obj_offset);
  if (addrObj) {
    int32_t envOffset = Utils::PatternFinder::ReadInt32(addrObj + 3);
    if (Utils::PatternFinder::IsSaneOffset(envOffset)) {
      owner.SetEnvObjectOffset(envOffset);
      logger->Debug("1.1 [OFFSET: Environment Object] Found: 0x{:X}", envOffset);
    } else {
      logger->Error("1.1 [OFFSET: Environment Object] Offset (0x{:X}) is insane.", envOffset);
      all_found = false;
    }
  } else {
    logger->Error("1.1 [OFFSET: Environment Object] FAILED to find signature.");
    all_found = false;
  }

  // 1.2 Environment Manager base pointer
  // Used by: all ClimateService methods that dereference m_environmentBasePtr
  const char* UNIQUE_LOG_STR = "[used_vehicles] %Iu used truck offers have expired";
  uintptr_t usedVehiclesXref = Utils::PatternFinder::FindFunctionByString(UNIQUE_LOG_STR, false);
  if (usedVehiclesXref) {
    addr = Utils::PatternFinder::Find(usedVehiclesXref, 64, "[48-4F] 8B [05-3D] ?? ?? ?? ??");
    if (addr) {
      uintptr_t envPtr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
      if (envPtr) {
        owner.SetEnvironmentBasePtr(envPtr);
        logger->Debug("1.2.1 [DATA: Environment Manager] Found at 0x{:X}", envPtr);

        uintptr_t addrLea = Utils::PatternFinder::Find(addr, 64, "48 8D [40-BF]");
        if (addrLea) {
          int8_t imm8 = Utils::PatternFinder::ReadInt8(addrLea + 3);
          owner.SetEnvironmentAdjustment(static_cast<intptr_t>(imm8));
          logger->Info("1.2.2 [DATA: Environment Adjustment] Detected: {} (via LEA)", imm8);
        }
      } else {
        logger->Error("1.2.1 [DATA: Environment Manager] Failed to resolve RIP address.");
        all_found = false;
      }
    } else {
      logger->Error("1.2.1 [DATA: Environment Manager] FAILED to find signature.");
      all_found = false;
    }
  } else {
    logger->Error("1.2 [DATA: Global Managers] FAILED to find unique string reference.");
    all_found = false;
  }

  // === SECTION 2: ENVIRONMENT STATE UPDATE ===
  // Used by: SetWeatherMode, SetRainIntensity, SetSkyboxIndex

  // 2. Find the entry point of UpdateSimulationTime.
  const char* UPDATE_SIM_TIME_SIG = "40 [0-1?] 56 48 [81-83] ec [1-4?] [3-30?] e8 ?? ?? ?? ?? 84 c0 0f 85 ?? ?? ?? ?? [40-4F] 8b [05-3D]";
  uintptr_t pfnUpdateSimTime = Utils::PatternFinder::Find(UPDATE_SIM_TIME_SIG);

  if (pfnUpdateSimTime) {
    logger->Debug("2. [CALL: UpdateSimulationTime] Found at 0x{:X}", pfnUpdateSimTime);
  } else {
    logger->Error("2. Failed to find UpdateSimulationTime function start.");
    return false;
  }

  // 2.1 UpdateEnvironmentState function pointer
  // Used by: SetWeatherMode, SetRainIntensity, SetSkyboxIndex (to trigger env update)
  const std::vector<std::string> timeUpdateChain = {"F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??", "[40-4F] 89 [80-BF] ?? ?? ?? ??", "E8 ?? ?? ?? ??"};

  addr = Utils::PatternFinder::FindChain(timeUpdateChain, 16, pfnUpdateSimTime);
  if (addr) {
    uintptr_t addrCall = Utils::PatternFinder::Find(addr, 32, "E8");
    if (addrCall) {
      uintptr_t pfnUpdateEnv = Utils::PatternFinder::GetRipAddress(addrCall, 1, 5);
      if (pfnUpdateEnv) {
        owner.SetUpdateFnAddr(pfnUpdateEnv);
        logger->Debug("2.1 [CALL: UpdateEnvironmentState] Found at 0x{:X}", pfnUpdateEnv);
      }
    }
  } else {
    logger->Error("2.1 [CALL: UpdateEnvironmentState] FAILED to find logic chain.");
    all_found = false;
  }

  // === SECTION 3: WEATHER CONTROL ===
  // Used by: GetWeatherMode, SetWeatherMode, GetActiveProfileName, GetActiveProfilePtr

  // 3. Find SetWeather function entry.
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

    // 3.2 Weather target offset (for GetNextWeatherMode)
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

    // 7.3 Container selector offset (env+0x3E70 — chooses nice vs bad container)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49b7  41 39 9E 70 3E 00 00          CMP dword ptr [R14 + 0x3e70],EBX
    uintptr_t addrSelector = Utils::PatternFinder::Find(pfnSunProfileUpdate, 128, "41 [CMP [r64+off32], r32]");
    if (addrSelector) {
      int32_t selectorOff = Utils::PatternFinder::ReadInt32(addrSelector + 3);
      if (Utils::PatternFinder::IsSaneOffset(selectorOff)) {
        owner.SetContainerSelectorOffset(selectorOff);
        logger->Debug("7.3 [OFFSET: Container Selector (nice/bad)] Found: 0x{:X}", selectorOff);
      } else {
        logger->Error("7.3 [OFFSET: Container Selector] Offset (0x{:X}) is insane.", selectorOff);
        all_found = false;
      }
    } else {
      logger->Error("7.3 [OFFSET: Container Selector] FAILED to find signature.");
      all_found = false;
    }

    // 7.4 Nice container offset (0xd0 — first set of sun profiles, "nice" variants)
    //* /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_1404e4940[1404e4940]) ---/
    //* 1404e49be  B8 D0 00 00 00                MOV EAX,0xd0
    uintptr_t addrNice = Utils::PatternFinder::Find(addrSelector, 16, "[MOV r32, imm32]");
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
    uintptr_t addrBad = Utils::PatternFinder::FindBackward(addrSelector, 64, "[MOV r32, imm32] [CMP r64, [r64+off32]]");
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
  const char* WEATHER_BLEND_PROGRESS_PATTERN =
      "44 [MOV r32, [r64+off32]] [MOV r32, imm32] [MOVSS xmm, [r64+off32]] [XORPS xmm, xmm] F3 0F 59 05";
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

  m_isReady = all_found;
  if (m_isReady) logger->Info("ClimateDataFinder: All weather/climate offsets found. Service is READY.");

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
