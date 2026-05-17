/**                                                                                               
 * @file WorldDataFinder.cpp                                                                          
 * @brief Implementation of dynamic pattern searching using EXACT Ghidra signatures.
 */ 

#include "SPF/Data/GameData/Finders/GameWorldDataFinder.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {

bool WorldDataFinder::TryFindOffsets(GameWorldService& owner) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Searching for GameWorld (Environment) data using provided Ghidra signatures...");

    // 1. Find the entry point of the UpdateTimeAdvance function.
    // This function is responsible for scrubbing/simulating time during events 
    // like resting, using the ferry, or changing time in Photo Mode. 
    // The signature matches the start of the function analyzed in Ghidra (UpdateTimeAdvance).
    // 48 8b c4        MOV        RAX,RSP
    // 55              PUSH       RBP
    // 48 81 ec        SUB        RSP,0x80
    // 80 00 00 00
    // 44 8b 91        MOV        R10D,dword ptr [RCX + 0x210]
    // 10 02 00 00
    // 48 8b e9        MOV        RBP,RCX    
    const char* UPDATE_TIME_ADVANCE_SIG = "48 81 ? ? ? ? ? 44 8b ? ? ? ? ? ? ? ? ? ? ? 0f 84 ? ? ? ? 48 ? ? ? ba";
    uintptr_t pfnUpdateTimeAdvance = Utils::PatternFinder::Find(UPDATE_TIME_ADVANCE_SIG);

    if (!pfnUpdateTimeAdvance) {
        logger->Error("CRITICAL: Failed to find UpdateTimeAdvance function start.");
        return false;
    }
    logger->Debug("UpdateTimeAdvance found at 0x{:X}", pfnUpdateTimeAdvance);

    // 2. Find the entry point of the UpdateSimulationTime function.
    // Signature: 40 ? 48 83 ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 84 c0 0f 85 ?? ?? ?? ?? 48 8b
    const char* UPDATE_SIM_TIME_SIG = "40 ? 48 83 ?? ?? 48 8B ?? E8 ?? ?? ?? ?? 84 c0 0f 85 ?? ?? ?? ?? 48 8b";
    uintptr_t pfnUpdateSimTime = Utils::PatternFinder::Find(UPDATE_SIM_TIME_SIG);

    if (!pfnUpdateSimTime) {
        logger->Error("CRITICAL: Failed to find UpdateSimulationTime function start.");
        return false;
    }
    logger->Debug("UpdateSimulationTime found at 0x{:X}", pfnUpdateSimTime);

    bool all_found = true;
    const size_t SEARCH_RANGE = 4096; 

    /*
     * ANCHOR #1: Global Environment Base Pointer (Updated for v1.59.2)
     * In 1.59.2, the game loads the pointer directly into RBX without the previous -0x10 adjustment.
     * Ghidra: 1414c68d0 48 8b 1d 61 f5 ef 01  MOV RBX, qword ptr [DAT_1433c5e38]
     *         1414c68d7 ba 80 00 00 00        MOV EDX, 0x80
     */
    const char* p_global_base = "48 8b 1d ?? ?? ?? ?? ba";
    uintptr_t addr = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_global_base);
    if (addr) {
        uintptr_t basePtr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
        if (basePtr) {
            owner.SetEnvironmentBasePtr(basePtr);
            logger->Debug("Anchor #1: Global Environment Base Pointer found at 0x{:X}", basePtr);

            // --- 1.1 Dynamic Pointer Adjustment Detection (v1.59+ support) ---
            /*
             * In versions before 1.59, the base pointer required a -0x10 adjustment.
             * In 1.59+, this adjustment is usually gone or changed.
             * We look for a LEA instruction (48 8D [40-7F]) that adjusts the loaded pointer.
             */
            intptr_t adjustment = 0;
            uintptr_t addrLea = Utils::PatternFinder::Find(addr, 32, "48 8D [40-7F]");
            if (addrLea) {
                int8_t imm8 = Utils::PatternFinder::ReadInt8(addrLea + 3);
                adjustment = static_cast<intptr_t>(imm8);
                logger->Info("Detected Environment pointer adjustment: {} (via LEA)", adjustment);
            } else {
                logger->Debug("No Environment pointer adjustment detected (standard 1.59+ behavior).");
            }
            owner.SetEnvironmentAdjustment(adjustment);

        } else {
            logger->Error("Anchor #1: Failed to resolve Global Base Pointer.");
            all_found = false;
        }
    } else {
        logger->Error("Anchor #1: FAILED to find Global Base signature (checked for direct MOV RBX).");
        all_found = false;
    }

    /*
     * ANCHOR #2: Environment Object Offset (Updated for v1.59.2)
     * Ghidra: 1414c68e3 48 8b 9b 50 08 00 00  MOV RBX, qword ptr [RBX + 0x850]
     *         1414c68ea 49 8b c9              MOV RCX, R9
     */
    const char* p_obj_offset = "48 ?? ?? ?? ?? ?? ?? 49";
    addr = Utils::PatternFinder::Find(pfnUpdateTimeAdvance, SEARCH_RANGE, p_obj_offset);
    if (addr) {
        int32_t envOffset = Utils::PatternFinder::ReadInt32(addr + 3);
        if (Utils::PatternFinder::IsSaneOffset(envOffset)) {
            owner.SetEnvObjectOffset(envOffset);
            logger->Debug("Anchor #2: Environment Object Offset found: 0x{:X}", envOffset);
        } else {
            logger->Error("Anchor #2: Environment Object Offset (0x{:X}) is insane.", envOffset);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #2: FAILED to find Object Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #3: Time Offset (0x3E58) & Environment Update Function Call (Updated for v1.59.2)
     * Ghidra: 140418461 f3 41 0f 11 81 5c 3e 00 00 MOVSS dword ptr [R9 + 0x3e5c], XMM0
     *         14041846a 41 89 81 58 3e 00 00  MOV dword ptr [R9 + 0x3e58], EAX
     *         140418471 e8 ea 5c 04 00        CALL UpdateEnvironmentState
     */
    const char* p_time_update = "F3 41 0F 11 81 ?? ?? ?? ?? 41 89 81 ?? ?? ?? ?? E8";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SEARCH_RANGE, p_time_update);
    if (addr) {
        // 1. Extract Time Offset (from MOV dword ptr [R9 + offset], EAX)
        // Offset 0x3E58 is at index 12 in our pattern.
        int32_t timeOffset = Utils::PatternFinder::ReadInt32(addr + 12);
        
        // 2. Extract Update Function Address (from CALL UpdateEnvironmentState)
        // CALL (E8) is at index 16. Displacement at +17.
        uintptr_t pfnUpdateEnv = Utils::PatternFinder::GetRipAddress(addr + 16, 1, 5);

        if (Utils::PatternFinder::IsSaneOffset(timeOffset)) {
            owner.SetTimeOffset(timeOffset);
            logger->Debug("Anchor #3: Time Offset found: 0x{:X}", timeOffset);
        } else {
            logger->Error("Anchor #3: Time Offset (0x{:X}) is insane.", timeOffset);
            all_found = false;
        }

        if (pfnUpdateEnv) {
            owner.SetUpdateFnAddr(pfnUpdateEnv);
            logger->Debug("Anchor #3: Update function found at 0x{:X}", pfnUpdateEnv);
        } else {
            logger->Error("Anchor #3: Failed to resolve Update function address.");
            all_found = false;
        }
    } else {
        logger->Error("Anchor #3: FAILED to find Time/Update signature.");
        all_found = false;
    }

    // 3. Find the entry point of the UpdateGameSession function.
    // Ghidra: UpdateGameSession
    // 1407bf710 40 56                      PUSH       RSI
    // 1407bf712 57                         PUSH       RDI
    // 1407bf713 48 83 ec 48                SUB        RSP,0x48
    // 1407bf717 48 8b 3d                   MOV        RDI,qword ptr [DAT_1433c5e38]
    //         1a 67 c0 02
    // 1407bf71e 48 8b f1                   MOV        RSI,RCX
    // 1407bf721 0f 29 74                   MOVAPS     xmmword ptr [RSP + local_28[0]],XMM6
    //         24 30
    // 1407bf726 48 89 5c                   MOV        qword ptr [RSP + local_res18],RBX
    //         24 70
    // 1407bf72b 48 89 6c                   MOV        qword ptr [RSP + local_18],RBP
    //         24 40    
    // Signature: 48 8b ?? ?? ?? ?? ?? 48 8b ?? 0f ?? ?? ?? ?? 48 ?? ?? ?? ?? 48 89
    const char* UPDATE_SESSION_SIG = "48 8b ?? ?? ?? ?? ?? 48 8b ?? 0f ?? ?? ?? ?? 48 ?? ?? ?? ?? 48 89";
    uintptr_t pfnUpdateSession = Utils::PatternFinder::Find(UPDATE_SESSION_SIG);
    if (pfnUpdateSession) {
        logger->Debug("UpdateGameSession found at 0x{:X}", pfnUpdateSession);
    } else {
        logger->Error("CRITICAL: Failed to find UpdateGameSession function start.");
        return false;
    }

    const size_t SIM_TIME_SEARCH_RANGE = 1024;
    const size_t SESSION_SEARCH_RANGE = 2048;

    /*
     * ANCHOR #4: Time Manager Global Pointer (DAT_142cf7668)
     *  48 8b ? ? ? ? ? 48 85 ? ? ? 32 ? 48 ? ? ? 5e
     * 
     * amtrucks.exe+1083586 - 48 8B 35 9B63C701     - mov rsi,[amtrucks.exe+2CF9928]
     * amtrucks.exe+108358D - 48 85 F6              - test rsi,rsi
     * amtrucks.exe+1083590 - 75 08                 - jne amtrucks.exe+108359A
     * amtrucks.exe+1083592 - 32 C0                 - xor al,al
     * amtrucks.exe+1083594 - 48 83 C4 70           - add rsp,70
     * amtrucks.exe+1083598 - 5E                    - pop rsi
     * amtrucks.exe+1083599 - C3                    - ret 
     */
    const char* p_time_manager_ptr = "48 8B ?? ?? ?? ?? ?? 48 85 ?? ?? ?? 32 ?? 48 ?? ?? ?? ?? c3 48 ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? ?? 48";
    addr = Utils::PatternFinder::Find(p_time_manager_ptr);
    if (addr) {
        // The instruction is 7 bytes long: 48 8B ?? [4-byte RIP displacement]
        uintptr_t timeMgrPtrAddr = Utils::PatternFinder::GetRipAddress(addr, 3, 7);
        if (timeMgrPtrAddr) {
            owner.SetTimeMgrPtrAddr(timeMgrPtrAddr);
            logger->Debug("Anchor #4: Time Manager Pointer found at 0x{:X}", timeMgrPtrAddr);
        } else {
            logger->Error("Anchor #4: Failed to resolve Time Manager Pointer.");
            all_found = false;
        }
    } else {
        logger->Error("Anchor #4: FAILED to find Time Manager signature.");
        all_found = false;
    }

    /*
     * ANCHOR #5: Simulation Time Offset (0x15C)
     * Signature: 44 8b
     * Ghidra: 140406f7b 44 8b ? ? ? ? ?   MOV R14D(?), dword ptr [RSI + 0x15c]
     */
    const char* p_sim_time_off = "44 8B";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_sim_time_off);
    if (addr) {
        int32_t simTimeOff = Utils::PatternFinder::ReadInt32(addr + 3);
        if (Utils::PatternFinder::IsSaneOffset(simTimeOff)) {
            owner.SetSimulationTimeOffset(simTimeOff);
            logger->Debug("Anchor #5: Simulation Time Offset found: 0x{:X}", simTimeOff);
        } else {
            logger->Error("Anchor #5: Simulation Time Offset (0x{:X}) is insane.", simTimeOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #5: FAILED to find Simulation Time Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #6: Sub-Minute Seconds Offset (0x160)
     * Signature: f3 0f 58 8e ? ? ? ? 0f
     * Ghidra: 140406feb f3 0f 58 8e 60 01 00 00  ADDSS XMM1, dword ptr [RSI + 0x160]
     * f3 0f 58        ADDSS      XMM1,dword ptr [RSI + 0x160]
     * 8e 60 01 00 00
     */
    const char* p_sub_sec_off = "F3 0F 58 ?? ?? ?? ?? ?? 0F ?? ?? 73";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_sub_sec_off);
    if (addr) {
        int32_t subSecOff = Utils::PatternFinder::ReadInt32(addr + 4);
        if (Utils::PatternFinder::IsSaneOffset(subSecOff)) {
            owner.SetSubMinuteSecondsOffset(subSecOff);
            logger->Debug("Anchor #6: Sub-Minute Seconds Offset found: 0x{:X}", subSecOff);
        } else {
            logger->Error("Anchor #6: Sub-Minute Seconds Offset (0x{:X}) is insane.", subSecOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #6: FAILED to find Sub-Minute Seconds Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #7: Map Scale (local.scale) Offset (0x2FA4 in 1.59.2)
     * Signature: F3 ?? 0F 59 ?? ?? ?? ?? ?? 41
     * Ghidra: 1404183ba f3 44 0f 59 88 a4 2f 00 00  MULSS XMM9, dword ptr [RAX + 0x2fa4]
     */
    const char* p_map_scale_off = "F3 ?? 0F 59 ?? ?? ?? ?? ?? 41";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_map_scale_off);
    if (addr) {
        // The 32-bit offset starts at index 5 (after F3 44 0F 59 88)
        int32_t scaleOff = Utils::PatternFinder::ReadInt32(addr + 5);
        if (Utils::PatternFinder::IsSaneOffset(scaleOff)) {
            owner.SetMapScaleOffset(scaleOff);
            logger->Debug("Anchor #7: Map Scale Offset found: 0x{:X}", scaleOff);
        } else {
            logger->Error("Anchor #7: Map Scale Offset (0x{:X}) is insane.", scaleOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #7: FAILED to find Map Scale Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #9: Skybox Auto-update Offset (0x46CC in 1.59.2)
     * Signature: 41 ?? B9
     * Ghidra: 14041844f 41 83 b9 cc 46 00 00 00  CMP dword ptr [R9 + 0x46cc], 0x0
     */
    const char* p_skybox_off = "41 ?? B9";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_skybox_off);
    if (addr) {
        int32_t skyboxOff = Utils::PatternFinder::ReadInt32(addr + 3);
        if (Utils::PatternFinder::IsSaneOffset(skyboxOff)) {
            owner.SetSkyboxAutoUpdateOffset(skyboxOff);
            logger->Debug("Anchor #9: Skybox Auto-update Offset found: 0x{:X}", skyboxOff);
        } else {
            logger->Error("Anchor #9: Skybox Auto-update Offset (0x{:X}) is insane.", skyboxOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #9: FAILED to find Skybox Auto-update Offset signature.");
        all_found = false;
    }

    /*
     * ANCHORS #10 & #11: Real Play Time (Minutes & Seconds)
     *  ff 86 ? ? ? ? f3 0f ? ? ? ? ? ? 48
     * 
     * Ghidra context:
     * 1404071b5 ff 86 e0 01 00 00  INC dword ptr [RSI + 0x1e0]
     * 1404071bb f3 0f 11 86 e4 01 00 00  MOVSS dword ptr [RSI + 0x1e4], XMM0
     * 1404071c3 48 8b 4e 10  MOV RCX, qword ptr [RSI + 0x10]
     */
    const char* p_real_play_time_sig = "FF ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? 48";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_real_play_time_sig);
    if (addr) {
        // Extract 0x1E0 (from INC dword ptr [RSI + offset])
        int32_t realTimeOff = Utils::PatternFinder::ReadInt32(addr + 2);
        // Extract 0x1E4 (from MOVSS dword ptr [RSI + offset], XMM0)
        // INC instruction is 6 bytes long, MOVSS starts at addr + 6, offset at addr + 10
        int32_t realSecOff = Utils::PatternFinder::ReadInt32(addr + 10);

        if (Utils::PatternFinder::IsSaneOffset(realTimeOff)) {
            owner.SetRealPlayTimeOffset(realTimeOff);
            logger->Debug("Anchor #10: Real Play Time Offset found: 0x{:X}", realTimeOff);
        } else {
            logger->Error("Anchor #10: Real Play Time Offset (0x{:X}) is insane.", realTimeOff);
            all_found = false;
        }

        if (Utils::PatternFinder::IsSaneOffset(realSecOff)) {
            owner.SetRealPlaySecondsOffset(realSecOff);
            logger->Debug("Anchor #11: Real Play Seconds Offset found: 0x{:X}", realSecOff);
        } else {
            logger->Error("Anchor #11: Real Play Seconds Offset (0x{:X}) is insane.", realSecOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchors #10 & #11: FAILED to find Real Play Time combined signature.");
        all_found = false;
    }

    // 4. Find the entry point of the CoreEngine_UpdateLoop function.
    //  48 8b c4 55 53 56 57 41 54 41 55 41 56 41 57 48 8d ? ? ? ? ? 48 81 ? ? ? ? ? 0f 29 ? ? 4c
    const char* CORE_ENGINE_LOOP_SIG = "48 8B C4 55 53 56 57 41 54 41 55 41 56 41 57 48 8D ?? ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? 0F 29 ?? ?? 4C";
    uintptr_t pfnCoreEngineLoop = Utils::PatternFinder::Find(CORE_ENGINE_LOOP_SIG);

    if (!pfnCoreEngineLoop) {
        logger->Error("CRITICAL: Failed to find CoreEngine_UpdateLoop function start.");
        return false;
    }
    logger->Debug("CoreEngine_UpdateLoop found at 0x{:X}", pfnCoreEngineLoop);

    const size_t CORE_SEARCH_RANGE = 4096;

    /*
     * ANCHOR #12: Global Warp Offset (0x66C)
     *  f3 41 ? ? ? ? ? ? ? f3 44
     * 
     * Ghidra context:
     * 14039a3d2 f3 41 0f 10 97 6c 06 00 00  MOVSS XMM2, dword ptr [R15 + 0x66c]
     * 14039a3db f3 44 0f 10 0d ac 61 e6 01  MOVSS XMM9, dword ptr [DAT_142200590]
     */
    const char* p_global_warp_sig = "F3 41 0F 10 ?? ?? ?? ?? ?? F3 44 ?? ?? ?? ?? ?? ?? ?? 41 ?? ?? ?? 49";
    addr = Utils::PatternFinder::Find(pfnCoreEngineLoop, CORE_SEARCH_RANGE, p_global_warp_sig);
    if (addr) {
        int32_t warpOff = Utils::PatternFinder::ReadInt32(addr + 5);
        if (Utils::PatternFinder::IsSaneOffset(warpOff)) {
            owner.SetGlobalWarpOffset(warpOff);
            logger->Debug("Anchor #12: Global Warp Offset found: 0x{:X}", warpOff);
        } else {
            logger->Error("Anchor #12: Global Warp Offset (0x{:X}) is insane.", warpOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #12: FAILED to find Global Warp Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #13: Pause Status Offset (0x859)
     *  41 38 ? ? ? ? ? ? ? ? ? ? 41 88
     * 
     * Ghidra context:
     * 14039a3ba 41 38 97 59 08 00 00  CMP byte ptr [R15 + 0x859], DL
     * 14039a3c6 41 88 97 59 08 00 00  MOV byte ptr [R15 + 0x859], DL
     */
    const char* p_pause_status_sig = "41 38 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 41 88 ?? ?? ?? ?? ?? e8 ?? ?? ?? ?? f3";
    addr = Utils::PatternFinder::Find(pfnCoreEngineLoop, CORE_SEARCH_RANGE, p_pause_status_sig);
    if (addr) {
        int32_t pauseOff = Utils::PatternFinder::ReadInt32(addr + 3);
        if (Utils::PatternFinder::IsSaneOffset(pauseOff)) {
            owner.SetPauseStatusOffset(pauseOff);
            logger->Debug("Anchor #13: Pause Status Offset found: 0x{:X}", pauseOff);
        } else {
            logger->Error("Anchor #13: Pause Status Offset (0x{:X}) is insane.", pauseOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #13: FAILED to find Pause Status Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #14: Frame Counter Offset (0x19C)
     *  8b 81 ? ? ? ? 83
     * 
     * Ghidra context:
     * 140399f65 8b 81 9c 01 00 00  MOV EAX, dword ptr [RCX + 0x19c]
     */
    const char* p_frame_counter_sig = "8B ?? ?? ?? ?? ?? 0f ?? ?? ?? 44";
    addr = Utils::PatternFinder::Find(pfnCoreEngineLoop, CORE_SEARCH_RANGE, p_frame_counter_sig);
    if (addr) {
        int32_t frameCounterOff = Utils::PatternFinder::ReadInt32(addr + 2);
        if (Utils::PatternFinder::IsSaneOffset(frameCounterOff)) {
            owner.SetFrameCounterOffset(frameCounterOff);
            logger->Debug("Anchor #14: Frame Counter Offset found: 0x{:X}", frameCounterOff);
        } else {
            logger->Error("Anchor #14: Frame Counter Offset (0x{:X}) is insane.", frameCounterOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #14: FAILED to find Frame Counter Offset signature.");
        all_found = false;
    }

    /*
     * ANCHOR #15: Real Delta Time Offset (0x8E8)
     *  f2 48 ? ? ? ? ? ? ? 44 8b
     * 
     * Ghidra context:
     * 140406f72 f2 48 0f 2a 80 e8 08 00 00  CVTSI2SD XMM0, qword ptr [RAX + 0x8e8]
     * 140406f7b 44 8b b6 5c 01 00 00  MOV R14D, dword ptr [RSI + 0x15c]
     */
    const char* p_delta_time_off_sig = "F2 48 0f";
    addr = Utils::PatternFinder::Find(pfnUpdateSimTime, SIM_TIME_SEARCH_RANGE, p_delta_time_off_sig);
    if (addr) {
        // Offset 0x8E8 is at addr + 5 (after F2 48 0F 2A 80)
        int32_t deltaTimeOff = Utils::PatternFinder::ReadInt32(addr + 5);
        if (Utils::PatternFinder::IsSaneOffset(deltaTimeOff)) {
            owner.SetRealDeltaTimeOffset(deltaTimeOff);
            logger->Debug("Anchor #15: Real Delta Time Offset found: 0x{:X}", deltaTimeOff);
        } else {
            logger->Error("Anchor #15: Real Delta Time Offset (0x{:X}) is insane.", deltaTimeOff);
            all_found = false;
        }
    } else {
        logger->Error("Anchor #15: FAILED to find Real Delta Time Offset signature.");
        all_found = false;
    }

    m_isReady = all_found;
    return all_found;
}

} // namespace Data::GameData::Finders
SPF_NS_END
