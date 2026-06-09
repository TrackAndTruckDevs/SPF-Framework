/**                                                                                               
 * @file ClimateDataFinder.cpp                                                                          
 * @brief Implementation of dynamic pattern searching for Climate and Weather data using EXACT Ghidra signatures.
 */ 

#include "SPF/Data/GameData/Finders/ClimateDataFinder.hpp"
#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

bool ClimateDataFinder::TryFindOffsets(ClimateService& owner) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Searching for Climate and Weather data using provided Ghidra signatures...");

    bool all_found = true;
    const size_t SEARCH_RANGE = 4096; 
    uintptr_t addr = 0;

    // --- SECTION 1: CORE ENVIRONMENT ANCHORS ---

    // 1. Find the entry point of the UpdateTimeAdvance function.

    /**
     * SEARCH STRATEGY (Verified for Game Version 1.60):
     * This function (UpdateTimeAdvance) handles time scrubbing during events 
     * like resting, using the ferry, or using Photo Mode. 
     * We locate it by searching for the time format string "%u:%02u".
     *
     * Since this common string might be used in multiple UI functions, we use a 
     * context signature that matches the specific math block where seconds are 
     * calculated (using the magic constant 0x88888889 for division by 60).
     *
     * Target Code Snippet (Ghidra 1.60):
     * 1415f8a64 b8 89 88 88 88     MOV        EAX,0x88888889
     * 1415f8a69 4c 8d 05 48 ad...  LEA        R8,[s_%u:%02u_1421637b8]
     * 1415f8a70 41 f7 e6           MUL        R14D
     * 1415f8a73 48 8d 4c 24 40     LEA        RCX=>local_48,[RSP + 0x40]
     *
     * After finding the string reference (Xref), we use GetFunctionStart (.pdata)
     * to accurately find the function's entry point:
     * 1415f8920 48 8b c4           MOV        RAX,RSP
     * 1415f8923 55                 PUSH       RBP
     * 1415f8924 48 81 ec 80...     SUB        RSP,0x80
     */
    const char* UPDATE_TIME_STRING = "%u:%02u";
    // Context Strategy: Match magic constant 0x88888889 followed by a flexible LEA [RIP+...]
    // 89 88 88 88      -> Magic constant (partial for 0x88888889)
    // [0-16?]          -> Gap for compiler reordering (allows 0 to 16 bytes)
    // [48-4F] 8D       -> REX (48-4F) + LEA opcode (8D)
    // [05-3D]          -> ModR/M for RIP-relative addressing (any register)
    const char* UPDATE_TIME_CONTEXT = "89 88 88 88 [0-16?] [48-4F] 8D [05-3D]";
    uintptr_t pfnUpdateTimeAdvance = Utils::PatternFinder::FindFunctionByString(UPDATE_TIME_STRING, true, UPDATE_TIME_CONTEXT);

    if (!pfnUpdateTimeAdvance) {
        logger->Error("Failed to find UpdateTimeAdvance function start.");
        return false;
    }
    logger->Debug("1. UpdateTimeAdvance found at 0x{:X}", pfnUpdateTimeAdvance);

    /*
     * 1.1 [OFFSET: Environment Object]
     * This offset points to the actual environment object within the manager.
     * Verified as 0x990 in v1.60.
     *
     * Ghidra 1.60 Analysis:
     * 1415f8973 [48-4F] 8B [80-BF] 90 09 00 00  MOV RBX, qword ptr [RBX + 0x990]
     * 1415f897a 49 8B C9                        MOV RCX, R9 (Marker)
     *
     * Strategy: Match MOV reg, [reg + offset32] followed by the RCX/R9 register bridge.
     */
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

    /*
     * 1.2 [DATA: Global Managers] (Environment)
     * Strategy: Use the stable string "[used_vehicles]..." as an anchor in UpdateGameSession.
     * In version 1.60+, Environment pointer is loaded from global memory.
     * 
     * Ghidra 1.60 Analysis (UpdateGameSession):
     * 1408527e6 48 8d 0d ...       LEA RCX, [s_[used_vehicles]...]
     * ...
     * 1408527f5 48 8b 05 d0 24...  MOV RAX, qword ptr [DAT_143554c40] (Environment)
     */
    const char* UNIQUE_LOG_STR = "[used_vehicles] %Iu used truck offers have expired";
    uintptr_t usedVehiclesXref = Utils::PatternFinder::FindFunctionByString(UNIQUE_LOG_STR, false);
    if (usedVehiclesXref) {
        // 1.2.1 [DATA: Environment Manager Pointer]
        addr = Utils::PatternFinder::Find(usedVehiclesXref, 64, "[48-4F] 8B [05-3D] ?? ?? ?? ??");
        if (addr) {
            uintptr_t envPtr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
            if (envPtr) {
                owner.SetEnvironmentBasePtr(envPtr);
                logger->Debug("1.2.1 [DATA: Environment Manager] Found at 0x{:X}", envPtr);

                // 1.2.2 Adjustment detection: look for LEA reg, [reg + adjustment]
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

    // --- SECTION 2: ENVIRONMENT STATE UPDATE ---

    // 2. Find the entry point of the UpdateSimulationTime function.
    /**
     * SEARCH STRATEGY (Updated for Game Version 1.60):
     * This function manages the in-game clock and simulation timing.
     * Between version 1.5x and 1.60, the compiler changed the prologue:
     * 
     * v1.5x (Old):
     * 140418320 40 56          PUSH RSI
     * 140418322 48 83 ec 60    SUB  RSP, 0x60
     * 
     * v1.60 (New):
     * 14048a3e0 40 55          PUSH RBP
     * 14048a3e2 56             PUSH RSI
     * 14048a3e3 48 81 ec a8... SUB  RSP, 0xa8
     * 
     * We use a "Flexible Signature" to match both versions:
     * 40 [0-1?] 56             -> Optional PUSH RBP + PUSH RSI
     * 48 [81-83] ec [1-4?]     -> Match SUB RSP with either 1-byte or 4-byte operand
     * [3-30?]                  -> Skip variable initialization logic
     * e8 ?? ?? ?? ??           -> CALL IsSimulationPaused
     */
    const char* UPDATE_SIM_TIME_SIG = "40 [0-1?] 56 48 [81-83] ec [1-4?] [3-30?] e8 ?? ?? ?? ?? 84 c0 0f 85 ?? ?? ?? ?? [40-4F] 8b [05-3D]";
    uintptr_t pfnUpdateSimTime = Utils::PatternFinder::Find(UPDATE_SIM_TIME_SIG);

    if (pfnUpdateSimTime) {
        logger->Debug("2. [CALL: UpdateSimulationTime] Found at 0x{:X}", pfnUpdateSimTime);
    } else {
        logger->Error("2. Failed to find UpdateSimulationTime function start.");
        return false;
    }

    /*
     * 2.1 [CALL: UpdateEnvironmentState]
     * This triggers the environment state refresh.
     * 
     * Ghidra 1.60 Analysis (Address: 14048a5c9):
     * 14048a5c9 f3 41 0f 11 b9 7c 3e 00 00  MOVSS dword ptr [R9 + 0x3e7c], XMM7
     * 14048a5d2 41 89 81 78 3e 00 00        MOV dword ptr [R9 + 0x3e78], EAX
     * 14048a5d9 e8 12 9e 04 00              CALL UpdateEnvironmentState
     *
     * Chain Strategy:
     * 1. MOVSS [reg+off], XMM -> F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??
     * 2. MOV   [reg+off], EAX -> [40-4F] 89 [80-BF] ?? ?? ?? ??
     * 3. CALL  UpdateState    -> E8
     */
    const std::vector<std::string> timeUpdateChain = {
        "F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??", 
        "[40-4F] 89 [80-BF] ?? ?? ?? ??", 
        "E8 ?? ?? ?? ??"
    };
    
    addr = Utils::PatternFinder::FindChain(timeUpdateChain, 16, pfnUpdateSimTime);
    if (addr) {
        // Find the CALL instruction (14048a5d9)
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

    /*
     * 3. [OFFSET: Skybox Auto-update] (Eternal Signature)
     * Strategy: Match the load of Environment object into R9 followed by a comparison.
     * v1.59 uses CMP [R9+off], 0 (83 B9), v1.60 uses CMP [R9+off], reg (39 B1).
     * 
     * Ghidra 1.60 Analysis:
     * 14048a5ab 4C 8B 88 90 09 00 00  MOV R9, qword ptr [RAX + 0x990]
     * 14048a5b2 45 39 B1 0C 47 00 00  CMP dword ptr [R9 + 0x470C], R14D
     */
    // const char* p_skybox_sig = "4C 8B [80-BF] ?? ?? ?? ?? [41-45] [39-83] [80-BF]";
    // addr = Utils::PatternFinder::Find(pfnUpdateSimTime, 1024, p_skybox_sig);
    // if (addr) {
    //     // Find the start of the CMP instruction (it follows the 7-byte MOV)
    //     uintptr_t addrCmp = addr + 7;
    //     int32_t skyboxOff = Utils::PatternFinder::ReadInt32(addrCmp + 3);
    //     if (Utils::PatternFinder::IsSaneOffset(skyboxOff)) {
    //         owner.SetSkyboxAutoUpdateOffset(skyboxOff);
    //         logger->Debug("3. [OFFSET: Skybox Auto-update] Found: 0x{:X}", skyboxOff);
    //     } else {
    //         logger->Error("3. [OFFSET: Skybox Auto-update] Offset (0x{:X}) is insane.", skyboxOff);
    //         all_found = false;
    //     }
    // } else {
    //     logger->Error("3. [OFFSET: Skybox Auto-update] FAILED to find signature.");
    //     all_found = false;
    // }

    // --- SECTION 4: WEATHER CONTROL ---

    // 4. Find the entry point of the SetWeather function (Verified for v1.60).
    /**
     * SEARCH STRATEGY (Updated for Game Version 1.60):
     * This function forces a weather change and manages transition states.
     * We locate it via the log string "Restarting environment transition".
     *
     * To disambiguate from other functions using the same string, we use a 
     * tight context search for the simulation time magic constant (0x6C16C16D).
     *
     * Ghidra 1.60 Analysis:
     * 1404df4dc 48 8d 0d ...  LEA RCX, [s_Restarting_environment_transitio...]
     * 1404df4ee b8 6d c1 16 6c     MOV EAX, 0x6C16C16D  <- Context anchor
     */
    const char* SET_WEATHER_STRING = "Restarting environment transition";
    const char* SET_WEATHER_CONTEXT = "6D C1 16 6C";
    uintptr_t pfnSetWeather = Utils::PatternFinder::FindFunctionByString(SET_WEATHER_STRING, true, SET_WEATHER_CONTEXT, 64);

    if (!pfnSetWeather) {
        logger->Error("4. Failed to find SetWeather function start.");
        all_found = false;
    } else {
        logger->Debug("4. [CALL: SetWeather] Found at 0x{:X}", pfnSetWeather);
    }

    /*
     * 4.1 [OFFSET: Weather Indexes] (Verified for v1.60)
     * Ghidra 1.60 Analysis (Address: 1404df417):
     * 1404df417 89 91 70 3e 00 00  MOV dword ptr [RCX + 0x3e70], EDX
     * 1404df41d 89 91 74 3e 00 00  MOV dword ptr [RCX + 0x3e74], EDX
     */
    const std::vector<std::string> weatherChain = {
        "89 91 ?? ?? ?? ??", 
        "89 91 ?? ?? ?? ??"
    };

    addr = Utils::PatternFinder::FindChain(weatherChain, 10, pfnSetWeather);
    if (addr) {
        int32_t weatherModeOff = Utils::PatternFinder::ReadInt32(addr + 2);
        int32_t weatherTargetOff = Utils::PatternFinder::ReadInt32(addr + 8);

        if (Utils::PatternFinder::IsSaneOffset(weatherModeOff) && Utils::PatternFinder::IsSaneOffset(weatherTargetOff)) {
            owner.SetWeatherModeOffset(weatherModeOff);
            owner.SetWeatherTargetOffset(weatherTargetOff);
            logger->Debug("4.1 [OFFSET: Weather Indexes] Found Current: 0x{:X}, Target: 0x{:X}", 
                weatherModeOff, weatherTargetOff);
        }

        // 4.2.1 Duration (Verified as 0x45E0 in v1.60)
        // Ghidra 1.60: 1404df434 83 b9 74 45 00 00 02  CMP State, 2
        //              1404df43b f3 0f 11 81 e0 45...  MOVSS Duration, XMM0
        const std::vector<std::string> durChain = { "83 [B8-BF] ?? ?? ?? ?? 02", "F3 0F 11 [80-BF] ?? ?? ?? ??" };
        uintptr_t addrDur = Utils::PatternFinder::FindChain(durChain, 20, pfnSetWeather);
        if (addrDur) {
            uintptr_t movAddr = Utils::PatternFinder::Find(addrDur, 32, "F3 0F 11 [80-BF]");
            if (movAddr) {
                int32_t durOff = Utils::PatternFinder::ReadInt32(movAddr + 4);
                if (Utils::PatternFinder::IsSaneOffset(durOff)) {
                    owner.SetWeatherTransDurationOffset(durOff);
                    logger->Debug("4.2.1 [OFFSET: Weather Duration] Found: 0x{:X}", durOff);
                }
            }
        }

        // 4.2.2 Blending Factor (Verified as 0x4570 in v1.60)
        // Ghidra 1.60: 1404df451 f3 0f 11 83 70 45 00 00  MOVSS [RBX + 0x4570], XMM0
        //              1404df459 48 c7 83 60 45 00 00...  MOV [RBX + 0x4560], -1
        uintptr_t addrBlend = Utils::PatternFinder::Find(pfnSetWeather, 1024, "F3 0F 11 [80-BF] ?? ?? ?? ?? 48 C7 [80-BF] ?? ?? ?? ?? FF FF FF FF");
        if (addrBlend) {
            int32_t blendOff = Utils::PatternFinder::ReadInt32(addrBlend + 4);
            if (Utils::PatternFinder::IsSaneOffset(blendOff)) {
                owner.SetWeatherBlendingFactorOffset(blendOff);
                logger->Debug("4.2.2 [OFFSET: Blending Factor] Found: 0x{:X}", blendOff);
            }
        }

        // 4.2.3 Transition State & Start Time (Verified as 0x4574 and 0x4578 in v1.60)
        // Ghidra 1.60: 1404df4ee b8 6d c1 16 6c           MOV EAX, 0x6C16C16D
        //              ...
        //              1404df512 c7 83 74 45 00 00 01... MOV State, 1
        //              1404df536 44 89 83 78 45 00 00     MOV StartTime, R8D
        const std::vector<std::string> timingChain = { 
            "B8 6D C1 16 6C",                // Anchor: Time constant
            "C7 [80-BF] ?? ?? ?? ?? 01 00 00 00", // Target 1: State = 1
            "44 89 [80-BF] ?? ?? ?? ??"      // Target 2: StartTime = R8D
        };
        uintptr_t addrTiming = Utils::PatternFinder::FindChain(timingChain, 100, pfnSetWeather);
        if (addrTiming) {
            uintptr_t addrState = Utils::PatternFinder::Find(addrTiming, 100, "C7 [80-BF]");
            uintptr_t addrStart = Utils::PatternFinder::Find(addrTiming, 120, "44 89 [80-BF]");
            if (addrState && addrStart) {
                int32_t stateOff = Utils::PatternFinder::ReadInt32(addrState + 2);
                int32_t startOff = Utils::PatternFinder::ReadInt32(addrStart + 3);
                if (Utils::PatternFinder::IsSaneOffset(stateOff) && Utils::PatternFinder::IsSaneOffset(startOff)) {
                    owner.SetWeatherTransitionOffset(stateOff);
                    owner.SetWeatherTransStartTimeOffset(startOff);
                    logger->Debug("4.2.3 [OFFSET: Weather Timing] Found State: 0x{:X}, StartTime: 0x{:X}", stateOff, startOff);
                }
            }
        }
    } else {
        logger->Error("4. [SET_WEATHER] FAILED to find logic chain.");
        all_found = false;
    }

    // --- SECTION 5: ENVIRONMENT PARAMETERS ---

    // 5. Find the entry point of the UpdateEnvironmentValues function (Verified for v1.60).
    /**
     * SEARCH STRATEGY (Updated for Game Version 1.60):
     * This function calculates final parameters for Rain, Fog, and Sun position.
     * We locate it via the unique log string "[env] Sun elevation %.2f, azimut".
     *
     * Ghidra 1.60 Analysis:
     * 1404d4827 48 8d 0d ...  LEA RCX, [s_[env]_Sun_elevation_...]
     */
    const char* UPDATE_ENV_VALS_STRING = "[env] Sun elevation %.2f, azimut";
    uintptr_t pfnUpdateEnvVals = Utils::PatternFinder::FindFunctionByString(UPDATE_ENV_VALS_STRING, true);

    if (!pfnUpdateEnvVals) {
        logger->Error("5. Failed to find UpdateEnvironmentValues function start.");
        all_found = false;
    } else {
        logger->Debug("5. [CALL: UpdateEnvironmentValues] Found at 0x{:X}", pfnUpdateEnvVals);
    }

    /*
     * 5.1 [OFFSET: Environment Runtime Parameters] (Updated for v1.60)
     * These parameters change in real-time based on simulation logic.
     * Strategy: Match specific logic blocks within UpdateEnvironmentValues for each parameter.
     */

    // 5.1.1 Rain Intensity & Road Wetness (Verified as 0x3F34 and 0x3F38 in v1.60)
    // Ghidra 1.60: 1404d503a f3 0f 11 83 34 3f 00 00  MOVSS [RBX + 0x3f34], XMM0
    //              1404d5042 85 c9                    TEST ECX, ECX
    const std::vector<std::string> rainChain = {
        "F3 [0-1?] 0F 11 [80-BF] ?? ?? ?? ??", // 1. MOVSS [reg + rain_off], XMM
        "85 C9"                                // 2. TEST ECX, ECX (Anchor)
    };

    addr = Utils::PatternFinder::FindChain(rainChain, 16, pfnUpdateEnvVals);
    if (addr) {
        // Offset position depends on presence of REX prefix
        int offPos = (*(uint8_t*)(addr + 1) == 0x0F) ? 4 : 5;
        int32_t rainOff = Utils::PatternFinder::ReadInt32(addr + offPos);
        if (Utils::PatternFinder::IsSaneOffset(rainOff)) {
            owner.SetRainIntensityOffset(rainOff);
            owner.SetRoadWetnessOffset(rainOff + 4); // Road wetness is consistently at +4
            logger->Debug("5.1.1 [OFFSET: Rain/Wetness] Found Rain: 0x{:X}, Wetness: 0x{:X}", rainOff, rainOff + 4);
        } else {
            logger->Error("5.1.1 [OFFSET: Rain Intensity] Offset (0x{:X}) is insane.", rainOff);
            all_found = false;
        }
    } else {
        logger->Error("5.1.1 [OFFSET: Rain Intensity] FAILED to find logic chain.");
        all_found = false;
    }

    // 5.1.2 Lightning Enabled (Verified as 0x3F11 in v1.60)
    // Ghidra 1.60: 1404d509f E8 ?? ?? ?? ??           CALL FUN_...
    //              1404d50a4 80 BB 11 3F 00 00 00     CMP byte ptr [RBX + 0x3f11], 0x0
    //              1404d50ab 0F 28 [C0-FF]            MOVAPS XMM?, XMM?
    const std::vector<std::string> lightChain = {
        "E8 ?? ?? ?? ??",               // 1. CALL (Anchor)
        "80 [80-BF] ?? ?? ?? ?? ??",    // 2. CMP byte ptr [reg + off32], imm8 (Target)
        "0F 28 [C0-FF]"                 // 3. MOVAPS (Anchor)
    };

    addr = Utils::PatternFinder::FindChain(lightChain, 32, pfnUpdateEnvVals);
    if (addr) {
        // Find the CMP instruction within the chain (it's after E8)
        uintptr_t addrCmp = Utils::PatternFinder::Find(addr, 32, "80 [80-BF] ?? ?? ?? ?? ??");
        if (addrCmp) {
            int32_t lightOff = Utils::PatternFinder::ReadInt32(addrCmp + 2);
            if (Utils::PatternFinder::IsSaneOffset(lightOff)) {
                owner.SetLightningEnabledOffset(lightOff);
                logger->Debug("5.1.2 [OFFSET: Lightning Enabled] Found: 0x{:X}", lightOff);
            } else {
                logger->Error("5.1.2 [OFFSET: Lightning Enabled] Offset (0x{:X}) is insane.", lightOff);
                all_found = false;
            }
        }
    } else {
        logger->Error("5.1.2 [OFFSET: Lightning Enabled] FAILED to find logic chain.");
        all_found = false;
    }

    // 5.1.3 Temperature & Lightning Intensity (Verified as 0x2A4 and 0x2A0 in v1.60)
    // Strategy: Use the unique math sequence (Time Load -> Lightning MOVSS -> DIVSS -> Temperature MOVSS)
    // Ghidra 1.60: 1404d50da 8b 93 78 3e 00 00           MOV EDX, [RBX + 0x3e78]
    //              1404d50e4 f3 44 0f 11 bb a0 02 00 00  MOVSS [RBX + 0x2a0], XMM15
    //              1404d5100 f3 0f 5e 88 f0 09 00 00     DIVSS XMM1, [RAX + 0x9f0]
    //              1404d5110 f3 0f 11 8b a4 02 00 00     MOVSS [RBX + 0x2a4], XMM1
    const std::vector<std::string> tempChain = {
        "8B [80-BF] ?? ?? ?? ??",             // 1. Anchor: MOV EDX, [reg + 0x3E78]
        "F3 [40-4F] 0F 11 [80-BF] ?? ?? ?? ??", // 2. Target: MOVSS [reg + lightning_off], XMM
        "F3 0F 11 [80-BF] ?? ?? ?? ??"        // 4. Target: MOVSS [reg + temp_off], XMM
    };

    addr = Utils::PatternFinder::FindChain(tempChain, 64, pfnUpdateEnvVals);
    if (addr) {
        // Extract Lightning Intensity (at index 1 in chain, usually ~6 bytes after anchor 1)
        uintptr_t addrLight = Utils::PatternFinder::Find(addr, 32, "F3 [40-4F] 0F 11 [80-BF]");
        // Extract Temperature (at index 3 in chain, usually ~40 bytes after anchor 1)
        uintptr_t addrTemp = Utils::PatternFinder::Find(addr + 30, 64, "F3 0F 11 [80-BF]");

        if (addrLight && addrTemp) {
            int32_t lightIntOff = Utils::PatternFinder::ReadInt32(addrLight + 5);
            int32_t tempOff = Utils::PatternFinder::ReadInt32(addrTemp + 4);
            
            if (Utils::PatternFinder::IsSaneOffset(lightIntOff) && Utils::PatternFinder::IsSaneOffset(tempOff)) {
                owner.SetLightningIntensityOffset(lightIntOff);
                owner.SetTemperatureOffset(tempOff);
                logger->Debug("5.1.3 [OFFSET: Temp/Light Intensity] Found Temp: 0x{:X}, Light: 0x{:X}", tempOff, lightIntOff);
            } else {
                logger->Error("5.1.3 [OFFSET: Temp/Light] Offsets (0x{:X}, 0x{:X}) are insane.", tempOff, lightIntOff);
                all_found = false;
            }
        }
    } else {
        logger->Error("5.1.3 [OFFSET: Temp/Light Intensity] FAILED to find logic chain.");
        all_found = false;
    }

    /*
     * 5.2 [OFFSET: Fog Parameters] (Verified for v1.60)
     * Strategy: Match the load of fog color and density parameters within UpdateEnvironmentValues.
     * 
     * Ghidra 1.60 Analysis:
     * 1404d4762 f3 0f 10 83 3c 3f 00 00  MOVSS XMM0, dword ptr [RBX + 0x3f3c] ; Density
     * ...
     * 1404d47f6 48 8d 93 30 3f 00 00     LEA RDX, [RBX + 0x3f30]              ; Color RGB
     */
    
    // 5.2.1 Fog Density
    const std::vector<std::string> fogDensityChain = {
        "F3 0F 10 [80-BF] ?? ?? ?? ??", // 1. MOVSS XMM?, [reg + density_off]
        "48 8D [80-BF] ?? ?? ?? ??"     // 2. LEA RDX, [reg + color_off]
    };

    addr = Utils::PatternFinder::FindChain(fogDensityChain, 160, pfnUpdateEnvVals);
    if (addr) {
        int32_t densityOff = Utils::PatternFinder::ReadInt32(addr + 4);
        uintptr_t addrColor = Utils::PatternFinder::Find(addr, 160, "48 8D [80-BF]");
        if (addrColor) {
            int32_t colorOff = Utils::PatternFinder::ReadInt32(addrColor + 3);
            if (Utils::PatternFinder::IsSaneOffset(densityOff) && Utils::PatternFinder::IsSaneOffset(colorOff)) {
                owner.SetFogDensityOffset(densityOff);
                owner.SetFogColorOffset(colorOff);
                logger->Debug("5.2 [OFFSET: Fog] Found Color: 0x{:X}, Density: 0x{:X}", colorOff, densityOff);
            }
        }
    } else {
        logger->Error("5.2 [OFFSET: Fog] FAILED to find logic chain.");
        all_found = false;
    }

    // --- SECTION 6: CLIMATE & PROFILES ---

    // 6. [CALL: ProcessSunProfiles]
    /**
     * SEARCH STRATEGY (Verified for Game Version 1.60):
     * This function manages the sun profiles array and is critical for Climate pointer.
     *
     * Ghidra 1.60 Analysis (Address: 1404d1100):
     * 1404d1100 48 89 5c 24 20     MOV  qword ptr [RSP + 0x20], RBX
     * 1404d1105 55                 PUSH RBP
     * 1404d1109 48 83 ec 40        SUB  RSP, 0x40
     * 1404d110d 48 8b 81 b8 2a...  MOV  RAX, qword ptr [RCX + 0x2ab8]
     * 1404d1117 48 85 c0           TEST RAX, RAX
     *
     * Flexible Signature Explanation:
     * 48 89 5c 24 [10-20]     -> Initial MOV RBX with variable stack offset
     * [4-16?]                 -> Skip variable register saving (MOVs or PUSHs)
     * 48 83 ec 40             -> Common SUB RSP 0x40
     * 48 8b 81 ?? ?? 00 00    -> MOV RAX, [RCX + offset] (Object Load)
     * [1-30?]                 -> Skip variable initialization logic
     * 48 85 c0                -> TEST RAX, RAX
     */
    const char* PROCESS_SUN_PROFILES_SIG = "48 89 5c 24 [10-20] [4-16?] 48 83 ec 40 48 8b 81 ?? ?? ?? ?? [1-30?] 48 85 c0";
    uintptr_t pfnProcessSunProfiles = Utils::PatternFinder::Find(PROCESS_SUN_PROFILES_SIG);

    if (pfnProcessSunProfiles) {
        logger->Debug("6. ProcessSunProfiles found at 0x{:X}", pfnProcessSunProfiles);
    } else {
        logger->Error("Failed to find ProcessSunProfiles function start.");
        all_found = false;
    }

    /*
     * 6.1 [OFFSET: Climate Object] (Eternal Signature)
     * This offset points to the climate data structure within the manager.
     * Strategy: Match the load of the climate object followed by the profile limit constant.
     * v1.59 uses limit 0xC0 (BE C0), v1.60 uses limit 0xD0 (BE D0).
     * 
     * Ghidra 1.60 Analysis (Address: 1404d110d):
     * 1404d110d 48 8b 81 b8 2a 00 00  MOV RAX, qword ptr [RCX + 0x2ab8]
     * ...
     * 1404d112c be d0 00 00 00        MOV ESI, 0xd0  <- Profile limit anchor
     */
    if (pfnProcessSunProfiles) {
        const std::vector<std::string> climateChain = {
            "48 8B 81 ?? ?? ?? ??", // 1. MOV RAX, [RCX + offset]
            "BE [80-FF] ?? ?? ??"   // 2. MOV ESI, limit (C0/D0/etc)
        };

        addr = Utils::PatternFinder::FindChain(climateChain, 32, pfnProcessSunProfiles);
        if (addr) {
            // Extract 32-bit offset: 48 8B 81 [OFFSET] (Offset is at +3)
            int32_t climateOff = Utils::PatternFinder::ReadInt32(addr + 3);
            if (Utils::PatternFinder::IsSaneOffset(climateOff)) {
                owner.SetClimatePtrOffset(climateOff);
                logger->Debug("6.1 [OFFSET: Climate Object] Found: 0x{:X}", climateOff);
            } else {
                logger->Error("6.1 [OFFSET: Climate Object] Offset (0x{:X}) is insane.", climateOff);
                all_found = false;
            }
        } else {
            logger->Error("6.1 [OFFSET: Climate Object] FAILED to find logic chain.");
            all_found = false;
        }
    } else {
        logger->Error("6.1 [OFFSET: Climate Object] Cannot search - ProcessSunProfiles is NULL.");
        all_found = false;
    }

    // --- SECTION 7: API FUNCTIONS ---

    // 7.1 [CALL: SetClimate] (Updated for v1.60)
    /**
     * SEARCH STRATEGY: Match the flexible function prologue.
     * Ghidra 1.60 Address: 14022a700 (or similar)
     */
    const char* SET_CLIMATE_SIG = "48 89 5C 24 ?? 48 89 [68-74] 24 ?? 57 48 83 EC [30-50] 0F 29 [70-7C] 24";
    uintptr_t setClimateFn = Utils::PatternFinder::Find(SET_CLIMATE_SIG);
    if (setClimateFn) {
        owner.SetSetClimateFnAddr(setClimateFn);
        logger->Info("7.1 [CALL: SetClimate] Found at: 0x{:X}", setClimateFn);
    } else {
        logger->Warn("7.1 [CALL: SetClimate] NOT found using flexible signature.");
    }

    m_isReady = all_found;
    if (m_isReady) logger->Info("ClimateDataFinder: All weather/climate offsets found. Service is READY.");

    return all_found;
}

} // namespace Data::GameData::Finders
SPF_NS_END
