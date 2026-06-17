#include "SPF/Data/GameData/Finders/ObjectVehicleManagerFinder.hpp"
#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include <chrono>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {
// String anchor for finding TrafficManager and ClearLocalVehicles.
// Found inside DebugCamera_HandleInput (v1.60+ @ 14053f7b6).
// 14053f7b6 48 8d 0d ab 02 bf 01    LEA RCX,[s_Macro_created_%Iu_vehicles_from_%_142129a68]

const char* MACRO_CREATED_VEHICLES_STR = "Macro created %Iu vehicles from %Iu loaded";

// String anchor for finding the vehicle ID offset.
// Found inside DebugCamera_RenderInfoOverlay (v1.60+ @ 140542352).
// 140542553 48 8d 15 0e e1 be 01  LEA RDX,[s_state:_%s<br>_142130668]
const char* VEHICLE_ID_STR_ANCHOR = "state: %s<br>";

// String anchor for finding vehicle property offsets.
// Found inside FormatObjectDebugInfo (v1.60+ @ 14054921e).
// 14054921e 48 8d 15 1b 78 be 01    LEA RDX,[s_[remote_player_vehicle]_142130a40]
const char* REMOTE_PLAYER_VEHICLE_STR = "[remote_player_vehicle]";

} // namespace

bool ObjectManagerFinder::TryFindOffsets(GameObjectVehicleService& owner) {
    if (m_isReady) return true;

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    auto start = std::chrono::high_resolution_clock::now();
    logger->Info("Searching for Object/Vehicle Manager data (Dynamic Search)...");
    
    uintptr_t pfnClear = 0;

    // --- BLOCK 1: ---
    /*
     * Logic:
     * 1. Directly find the XREF of the "Macro created %Iu vehicles..." string using FindFunctionByString.
     * 2. From this XREF (the LEA instruction), search forward for the MOV instruction that loads g_ObjectManager.
     * 3. Immediately after, find the CALL instruction to resolve ClearLocalVehicles function address.
     */
    uintptr_t scanPos = PatternFinder::FindFunctionByString(MACRO_CREATED_VEHICLES_STR, false);
    if (scanPos) {
        logger->Info("--- [Block 1] Found Macro string XREF at: 0x{:X}", scanPos);

        // Find TrafficManager pointer load (MOV REG, [RIP+offset])
        // Ghidra Reference (v1.60+ @ 14053f7e4):
        // 14053f7e4 48 8b 0d f5 54 01 03    MOV RCX,qword ptr [g_ObjectManager]
        // Pattern: 48 8B [05-3D] (covers RAX-RDI targets)
        uintptr_t addrMov = PatternFinder::Find(scanPos, 128, "48 8B [05-3D]");
        if (addrMov) {
            uintptr_t pTrafficManager = PatternFinder::GetRipAddress(addrMov, 3, 7);
            uintptr_t trafficManagerAddr = *reinterpret_cast<uintptr_t*>(pTrafficManager);
            owner.SetTrafficManagerAddr(trafficManagerAddr);
            logger->Info("--- [Block 1] Found TrafficManager address at: 0x{:X}", trafficManagerAddr);

            // Check for pointer adjustment (ADD REG, imm8)
            // Ghidra Reference (v1.59+ adjustment logic):
            // 1404c973c 48 83 c0 e8     ADD RAX,-0x18
            // Pattern: 48 83 [C0-C7] (covers RAX-RDI targets)
            uintptr_t addrAdd = PatternFinder::Find(addrMov, 32, "48 83 [C0-C7]");
            if (addrAdd) {
                int8_t imm8 = PatternFinder::ReadInt8(addrAdd + 3);
                owner.SetTrafficManagerAdjustment(static_cast<intptr_t>(imm8));
                logger->Info("--- [Block 1] Detected TrafficManager adjustment: {}", imm8);
            }

            // Find ClearLocalVehicles call (CALL rel32)

            // Ghidra Reference (v1.60+ @ 14053f7ed):
            // 14053f7ed e8 0e 71 01 00          CALL ClearLocalVehicles
            // Pattern: E8 (standard CALL opcode)
            uintptr_t addrCall = PatternFinder::Find(addrMov, 32, "E8");
            if (addrCall) {
                pfnClear = PatternFinder::GetRipAddress(addrCall, 1, 5);
                logger->Info("--- [Block 1] Found ClearLocalVehicles at: 0x{:X}", pfnClear);
            } else { logger->Warn("[Block 1] FAILED to find ClearLocalVehicles call near 0x{:X}", addrMov); }
        } else { logger->Warn("[Block 1] FAILED to find TrafficManager pointer load near 0x{:X}", scanPos); }
    } else { logger->Warn("[Block 1] FAILED to find Macro string anchor '{}' or its XREF", MACRO_CREATED_VEHICLES_STR); }
    // --- END OF BLOCK 1 ---

    // --- BLOCK 1.1: Extracting offsets from ClearLocalVehicles (v1.60+) ---
    /*
     * Logic:
     * Inside ClearLocalVehicles, we find the offsets for the vehicle array, count, and struct size.
     * 
     * Ghidra Reference (v1.60+ @ 140556910):
     * 140556910 48 8b 89 f8 00 00 00    MOV RCX,qword ptr [RCX + 0xf8]   <-- Array Offset
     * 140556917 48 8b 83 00 01 00 00    MOV RAX,qword ptr [RBX + 0x100]  <-- Count Offset
     * ...
     * 14055693b 48 8d 48 10             LEA RCX,[RAX + 0x10]             <-- Struct Size
     */
    if (pfnClear) {
        logger->Info("--- [Block 1.1] Extracting offsets from ClearLocalVehicles at 0x{:X}...", pfnClear);

        // 1. Vehicle Array Offset (MOV REG, [REG + 32-bit displacement])
        // Pattern: 48 8B [80-BF]
        uintptr_t addrArray = PatternFinder::Find(pfnClear, 64, "48 8B [80-BF]");
        if (addrArray) {
            int32_t off = PatternFinder::ReadInt32(addrArray + 3);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetPArrayObjectOffset(off);
                logger->Info("--- [Block 1.1] Found Vehicle Array offset: 0x{:X}", off);

                // 2. Vehicle Count Offset (The next MOV after the array load)
                uintptr_t addrCount = PatternFinder::Find(addrArray + 7, 32, "48 8B [80-BF]");
                if (addrCount) {
                    int32_t offCount = PatternFinder::ReadInt32(addrCount + 3);
                    if (PatternFinder::IsSaneOffset(offCount)) {
                        owner.SetVehicleCountOffset(offCount);
                        logger->Info("--- [Block 1.1] Found Vehicle Count offset: 0x{:X}", offCount);
                    } else { logger->Error("[Block 1.1] Vehicle Count offset INVALID (0x{:X})", offCount); }
                } else { logger->Error("[Block 1.1] FAILED to find Vehicle Count anchor"); }
            } else { logger->Error("[Block 1.1] Vehicle Array offset INVALID (0x{:X})", off); }
        } else { logger->Error("[Block 1.1] FAILED to find Vehicle Array anchor"); }

        // 3. Vehicle Struct Size (LEA REG, [REG + 8-bit displacement])
        // Pattern: 48 8D [40-7F] (Covers all destination registers with disp8)
        uintptr_t addrSize = PatternFinder::Find(pfnClear, 128, "48 8D [40-7F]");
        if (addrSize) {
            uint8_t size = PatternFinder::ReadInt8(addrSize + 3);
            owner.SetSpawnedVehicleStructSize(size);
            logger->Info("--- [Block 1.1] Found Vehicle Struct Size: 0x{:X}", size);
        } else { logger->Error("[Block 1.1] FAILED to find Vehicle Struct Size anchor"); }
    }
    // --- BLOCK 2: Vehicle ID Offset (v1.60+) ---
    /*
     * Logic:
     * 1. Find the unique string anchor used for rendering the vehicle ID overlay.
     * 2. From its XREF (where it's loaded as a format string), scan backward for the MOV instruction that loads the ID.
     * 
     * Ghidra Reference (v1.60+ @ 140542343):
     * 14054233e 48 85 f6                   TEST       RSI,RSI
     * 140542341 74 09                      JZ         LAB_14054234c
     * 140542343 44 8b 86 20 04 00 00       MOV        R8D,dword ptr [RSI + 0x420]  <-- ID Offset
     * 14054234a eb 06                      JMP        LAB_140542352
     * ...
     * 140542553 48 8d 15 0e e1 be 01  LEA RDX,[s_state:_%s<br>_142130668]
     */
    uintptr_t idStrXref = PatternFinder::FindFunctionByString(VEHICLE_ID_STR_ANCHOR, false);
    if (idStrXref) {
        logger->Info("--- [Block 2] Found ID string XREF at: 0x{:X}", idStrXref);

        // Scan backward for MOV REG, [REG + offset]
        // Pattern: 44 8B [80-BF] (covers R8D-R15D targets with 32-bit displacement)
        uintptr_t addrIdLoad = PatternFinder::FindBackward(idStrXref, 1000, "44 8B [80-BF]");
        if (addrIdLoad) {
            int32_t offset = PatternFinder::ReadInt32(addrIdLoad + 3);
            if (PatternFinder::IsSaneOffset(offset)) {
                owner.SetVehicleIdOffset(offset);
                logger->Info("--- [Block 2] Found Vehicle ID offset: 0x{:X}", offset);
            } else { logger->Error("[Block 2] Vehicle ID offset INVALID (0x{:X})", offset); }
        } else { logger->Error("[Block 2] FAILED to find Vehicle ID load anchor near 0x{:X}", idStrXref); }
    } else { logger->Warn("[Block 2] FAILED to find ID string anchor '{}'", VEHICLE_ID_STR_ANCHOR); }
    // --- END OF BLOCK 2 ---

    // --- BLOCK 3: Local Player Controller Offset (v1.60+) ---
    /*
     * Logic:
     * Search for the sequence where the game loads the local player controller and checks its status.
     * 
     * Ghidra Reference (v1.60+ @ 14054ff67):
     * 14054ff67 49 8b 8e f8 04 00 00       MOV        RCX,qword ptr [R14 + 0x4f8]  <-- Player Offset
     * 14054ff6e 48 85 c9                  TEST       RCX,RCX
     * 14054ff71 74 30                     JZ         LAB_14054ffa3
     * 14054ff73 41 f6 86 54 05 00 00 40    TEST       byte ptr [R14 + 0x554],0x40  <-- Flag check
     */
    // Pattern: MOV REG, [R8-15+off]; TEST REG, REG; JZ; TEST byte [R8-15+off]
    const char* PLAYER_CONTROLLER_SIG = "49 8B [80-BF] ?? ?? ?? ?? 48 85 [C0-FF] 74 ?? 41 F6 [80-BF]";
    uintptr_t addrPlayerLoad = PatternFinder::Find(PLAYER_CONTROLLER_SIG);
    if (addrPlayerLoad) {
        int32_t offset = PatternFinder::ReadInt32(addrPlayerLoad + 3);
        if (PatternFinder::IsSaneOffset(offset)) {
            owner.SetLocalPlayerControllerOffset(offset);
            logger->Info("--- [Block 3] Found LocalPlayerController offset: 0x{:X}", offset);
        } else { logger->Error("[Block 3] LocalPlayerController offset INVALID (0x{:X})", offset); }
    } else { logger->Warn("[Block 3] FAILED to find LocalPlayerController sequence"); }
    // --- END OF BLOCK 3 ---

    // --- BLOCK 3.1: Player Vehicle Actor Offset (v1.60+) ---
    /*
     * Logic:
     * 1. Find the start of the function containing the player vehicle actor logic using a unique mangled string anchor.
     * 2. From the function start, scan forward for the instruction sequence that retrieves the Actor from the Controller.
     * 
     * Ghidra Reference (v1.60+ @ 140609f89):
     * 140609f89 48 8b 49 48       MOV        RCX,qword ptr [RCX + 0x48]  <-- Actor Offset
     * 140609f8d 48 85 c9          TEST       RCX,RCX
     * 140609f90 74 14             JZ         LAB_140609fa6
     * 140609f92 48 8b 01          MOV        RAX,qword ptr [RCX]
     * 140609f95 ff 50 10          CALL       qword ptr [RAX + 0x10]
     */
    const char* SPAWNED_VEHICLE_ARRAY_STR = "??A?$array_t@Uspawned_vehicle_t@traffic_u@prism@@@prism@@QEAAAEAUspawned_vehicle_t@traffic_u@1@_K@Z";
    uintptr_t pfnUpdateAllTraffic = PatternFinder::FindFunctionByString(SPAWNED_VEHICLE_ARRAY_STR, true);
    if (pfnUpdateAllTraffic) {
        logger->Info("--- [Block 3.1] Found UpdateAllTraffic start at: 0x{:X}", pfnUpdateAllTraffic);

        // Pattern: MOV REG, [REG+disp8]; TEST REG, REG; JZ; MOV REG, [REG]; CALL [REG+off]
        const char* PLAYER_ACTOR_SIG = "48 8B [40-7F] ?? 48 85 [C0-FF] 74 ?? 48 8B [00-3F] FF";
        uintptr_t addrActorLoad = PatternFinder::Find(pfnUpdateAllTraffic, 0x1000, PLAYER_ACTOR_SIG);
        if (addrActorLoad) {
            int8_t offset = PatternFinder::ReadInt8(addrActorLoad + 3);
            if (PatternFinder::IsSaneOffset(offset)) {
                owner.SetPlayerVehicleInControllerOffset(offset);
                logger->Info("--- [Block 3.1] Found PlayerVehicleInController offset: 0x{:X}", offset);
            } else { logger->Error("[Block 3.1] PlayerVehicle offset INVALID (0x{:X})", offset); }
        } else { logger->Error("[Block 3.1] FAILED to find PlayerVehicle actor sequence inside function"); }
    } else { logger->Warn("[Block 3.1] FAILED to find string anchor '{}'", SPAWNED_VEHICLE_ARRAY_STR); }
    // --- END OF BLOCK 3.1 ---

    // --- BLOCK 4: Detailed Vehicle Property Offsets (v1.60+) ---
    /*
     * Logic:
     * 1. Find the FormatObjectDebugInfo function using the unique "[remote_player_vehicle]" string.
     * 2. Inside the function, scan for instruction sequences that load specific AI properties.
     * 
     * Ghidra Reference (v1.60+ @ 140549170):
     * ...
     * 14054939a f3 0f 10 86 30 04 00 00    MOVSS XMM0, [RSI + 0x430]      <-- Speed Limit
     * 1405493f1 f3 44 0f 10 8e 44 04 00 00 MOVSS XMM9, [RSI + 0x444]      <-- Patience
     * 1405493fd f3 44 0f 10 96 40 04 00 00 MOVSS XMM10, [RSI + 0x440]     <-- Safety
     * 140549406 48 8b 96 28 04 00 00       MOV RDX, [RSI + 0x428]         <-- Lane Speed Input
     * 14054942f f3 0f 59 8e 34 04 00 00    MULSS XMM1, [RSI + 0x434]      <-- Target Speed
     * ...
     * 14054941e 48 8b 46 10                MOV RAX, [RSI + 0x10]          <-- Sub-Object
     * 140549443 ff 50 10                   CALL [RAX + 0x10]              <-- Accel VTable
     * 140549455 ff 50 08                   CALL [RAX + 0x08]              <-- Speed VTable
     */
    uintptr_t pfnFormat = PatternFinder::FindFunctionByString(REMOTE_PLAYER_VEHICLE_STR, true);
    if (pfnFormat) {
        logger->Info("--- [Block 4] Found FormatObjectDebugInfo start at: 0x{:X}", pfnFormat);

        // 4.1 Speed Limit (MOVSS XMM, [REG+disp32])
        uintptr_t addrSpeedLimit = PatternFinder::Find(pfnFormat, 0x1000, "F3 0F 10 [80-BF]");
        if (addrSpeedLimit) {
            int32_t off = PatternFinder::ReadInt32(addrSpeedLimit + 4);
            owner.SetSpeedLimitOffset(off);
            logger->Info("--- [Block 4] Found Speed Limit offset: 0x{:X}", off);
        }

        // 4.2 Patience & Safety (MOVSS XMM, [REG+disp32])
        // Found after Speed Limit.
        uintptr_t addrPatience = PatternFinder::Find(addrSpeedLimit ? addrSpeedLimit : pfnFormat, 0x500, "F3 44 0F 10 [80-BF]");
        if (addrPatience) {
            int32_t offP = PatternFinder::ReadInt32(addrPatience + 5);
            owner.SetPatienceOffset(offP);
            logger->Info("--- [Block 4] Found Patience offset: 0x{:X}", offP);

            uintptr_t addrSafety = PatternFinder::Find(addrPatience + 9, 128, "F3 44 0F 10 [80-BF]");
            if (addrSafety) {
                int32_t offS = PatternFinder::ReadInt32(addrSafety + 5);
                owner.SetSafetyOffset(offS);
                logger->Info("--- [Block 4] Found Safety offset: 0x{:X}", offS);
            }
        }

        // 4.3 Target Speed (MULSS XMM, [REG+disp32])
        uintptr_t addrTarget = PatternFinder::Find(pfnFormat, 0x1000, "F3 0F 59 [80-BF]");
        if (addrTarget) {
            int32_t off = PatternFinder::ReadInt32(addrTarget + 4);
            owner.SetTargetSpeedOffset(off);
            logger->Info("--- [Block 4] Found Target Speed offset: 0x{:X}", off);
        }

        // 4.4 Lane Speed Input (MOV REG, [REG+disp32])
        uintptr_t addrLane = PatternFinder::Find(pfnFormat, 0x1000, "48 8B [90-BF]");
        if (addrLane) {
            int32_t off = PatternFinder::ReadInt32(addrLane + 3);
            owner.SetLaneSpeedInputOffset(off);
            logger->Info("--- [Block 4] Found Lane Speed Input offset: 0x{:X}", off);
        }

        // 4.5 Sub-Object & VTables (MOV REG, [REG+disp8] then LEA REG, [REG+disp8])
        uintptr_t addrSubObj = PatternFinder::Find(pfnFormat, 0x1000, "48 8B [40-7F] ?? 48 8D [40-7F]");
        if (addrSubObj) {
            uint8_t off = PatternFinder::ReadInt8(addrSubObj + 3);
            owner.SetVehicleSubObjectOffset(off);
            logger->Info("--- [Block 4] Found Vehicle Sub-Object offset: 0x{:X}", off);

            // Acceleration VTable (Immediately follows)
            uintptr_t addrAccel = PatternFinder::Find(addrSubObj, 64, "FF 50");
            if (addrAccel) {
                uint8_t offA = PatternFinder::ReadInt8(addrAccel + 2);
                owner.SetVtableGetAccelerationOffset(offA);
                logger->Info("--- [Block 4] Found Accel VTable offset: 0x{:X}", offA);

                // Speed VTable
                uintptr_t addrSpeed = PatternFinder::Find(addrAccel + 3, 64, "FF 50");
                if (addrSpeed) {
                    uint8_t offSp = PatternFinder::ReadInt8(addrSpeed + 2);
                    owner.SetVtableGetCurrentSpeedOffset(offSp);
                    logger->Info("--- [Block 4] Found Speed VTable offset: 0x{:X}", offSp);
                }
            }
        }
    }
    // --- END OF BLOCK 4 ---

    // Final check for all offsets before declaring readiness
    m_isReady = (owner.GetTrafficManagerAddr() != 0 && owner.GetPArrayObjectOffset() != 0 && 
                 owner.GetVehicleCountOffset() != 0 && owner.GetSpawnedVehicleStructSize() != 0 && 
                 owner.GetVehicleIdOffset() != 0 && owner.GetPatienceOffset() != 0 && 
                 owner.GetSafetyOffset() != 0 && owner.GetTargetSpeedOffset() != 0 && 
                 owner.GetSpeedLimitOffset() != 0 && owner.GetLaneSpeedInputOffset() != 0 && 
                 owner.GetLocalPlayerControllerOffset() != 0 && owner.GetPlayerVehicleInControllerOffset() != 0 &&
                 owner.GetVehicleSubObjectOffset() != 0 && owner.GetVtableGetCurrentSpeedOffset() != 0 && 
                 owner.GetVtableGetAccelerationOffset() != 0);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (m_isReady) {
        logger->Info("--- ALL OFFSETS FOUND. ObjectManagerFinder is ready. ({} ms) ---", duration);
    } else {
        logger->Error("FAILED to initialize one or more Vehicle/Object Manager offsets. ({} ms)", duration);
    }

    return m_isReady;
}

} // namespace Data::GameData::Finders
SPF_NS_END
