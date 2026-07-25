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
  //          GetSkyboxCount, GetSkyboxIndex, SetSkyboxIndex, GetCurrentClimateName,
  //          GetAvailableClimates, GetActiveProfileName, GetActiveProfileIndex,
  //          SetClimate, GetTemperature, SetTemperature, GetWeight, SetWeight,
  //          EnsureInitialKick, GetActiveProfilePtr)

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
  }

  // 3.1 Weather mode and target offsets
  // Used by: GetWeatherMode (mode), SetWeatherMode (mode + target),
  //          GetActiveProfileName (mode), GetActiveProfilePtr (mode)
  const std::vector<std::string> weatherChain = {"89 91 ?? ?? ?? ??", "89 91 ?? ?? ?? ??"};

  addr = Utils::PatternFinder::FindChain(weatherChain, 10, SetWeatherFn);
  if (addr) {
    int32_t weatherModeOff = Utils::PatternFinder::ReadInt32(addr + 2);
    int32_t weatherTargetOff = Utils::PatternFinder::ReadInt32(addr + 8);

    if (Utils::PatternFinder::IsSaneOffset(weatherModeOff) && Utils::PatternFinder::IsSaneOffset(weatherTargetOff)) {
      owner.SetWeatherModeOffset(weatherModeOff);
      owner.SetWeatherTargetOffset(weatherTargetOff);
      logger->Debug("3.1 [OFFSET: Weather Indexes] Found Current: 0x{:X}, Target: 0x{:X}", weatherModeOff, weatherTargetOff);
    }

    // 3.2 Blending factor
    // Used by: SetWeatherMode (force blending state)
    uintptr_t addrBlend = Utils::PatternFinder::Find(SetWeatherFn, 1024, "F3 0F 11 [80-BF] ?? ?? ?? ?? 48 C7 [80-BF] ?? ?? ?? ?? FF FF FF FF");
    if (addrBlend) {
      int32_t blendOff = Utils::PatternFinder::ReadInt32(addrBlend + 4);
      if (Utils::PatternFinder::IsSaneOffset(blendOff)) {
        owner.SetWeatherBlendingFactorOffset(blendOff);
        logger->Debug("3.2 [OFFSET: Blending Factor] Found: 0x{:X}", blendOff);
      }
    }

    // 3.3 Transition state and start time
    // Used by: SetWeatherMode (transition state + blending timing)
    const std::vector<std::string> timingChain = {
      "B8 6D C1 16 6C",
      "C7 [80-BF] ?? ?? ?? ?? 01 00 00 00",
      "44 89 [80-BF] ?? ?? ?? ??"
    };
    uintptr_t addrTiming = Utils::PatternFinder::FindChain(timingChain, 100, SetWeatherFn);
    if (addrTiming) {
      uintptr_t addrState = Utils::PatternFinder::Find(addrTiming, 100, "C7 [80-BF]");
      uintptr_t addrStart = Utils::PatternFinder::Find(addrTiming, 120, "44 89 [80-BF]");
      if (addrState && addrStart) {
        int32_t stateOff = Utils::PatternFinder::ReadInt32(addrState + 2);
        int32_t startOff = Utils::PatternFinder::ReadInt32(addrStart + 3);
        if (Utils::PatternFinder::IsSaneOffset(stateOff) && Utils::PatternFinder::IsSaneOffset(startOff)) {
          owner.SetWeatherTransitionOffset(stateOff);
          owner.SetWeatherTransStartTimeOffset(startOff);
          logger->Debug("3.3 [OFFSET: Weather Timing] Found State: 0x{:X}, StartTime: 0x{:X}", stateOff, startOff);
        }
      }
    }
  } else {
    logger->Error("3. [SET_WEATHER] FAILED to find logic chain.");
    all_found = false;
  }

  // === SECTION 4: CLIMATE PTR & UNIT ID ===
  // Used by: GetCurrentClimateName, GetAvailableClimates, GetActiveProfileName,
  //          SetSkyboxIndex, SetRainIntensity, GetSkyboxCount, GetSkyboxIndex,
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

  m_isReady = all_found;
  if (m_isReady) logger->Info("ClimateDataFinder: All weather/climate offsets found. Service is READY.");

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
