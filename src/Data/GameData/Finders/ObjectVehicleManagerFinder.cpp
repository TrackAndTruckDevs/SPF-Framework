#include "SPF/Data/GameData/Finders/ObjectVehicleManagerFinder.hpp"
#include "SPF/Data/GameData/GameObjectVehicleService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {
/*
 * @brief Signature to find the DebugCamera_RenderInfoOverlay function.
 *
 * HOW-TO-FIND:
 * 1. This function is responsible for rendering debug information on the screen, making it a stable and unique target.
 * 2. Within this function, there is a crucial instruction that loads the global pointer to the Traffic/Object Manager.
 * 3. This signature targets the prologue of the function, which is highly stable across game updates.
 * 4. The targeted instructions from the disassembly are:
 *    140496e70: 4C 8B DC           (MOV R11, RSP)
 *    140496e73: 55                 (PUSH RBP)
 *    140496e74: 41 56              (PUSH R14)
 *    140496e76: 49 8D 6B A1        (LEA RBP, [R11-0x5F])
 *    140496e7a: 48 81 EC F8 00...  (SUB RSP, 0xF8)
 *    140496e81: 80 B9 38 05...     (CMP BYTE PTR [RCX+0x538], 0x0)
 */
const char* DEBUG_CAMERA_RENDER_INFO_OVERLAY_SIG = "4C 8B DC 55 41 56 49 8D ? ? 48 81 EC ? ? ? ? 80 B9";

/*
 * @brief Signature for the instruction that loads the global TrafficManager pointer.
 *
 * HOW-TO-FIND:
 * 1. This search is performed *inside* the found `DebugCamera_RenderInfoOverlay` function.
 * 2. We are looking for the instruction that loads the g_ObjectManager (another name for TrafficManager) into a register.
 * 3. The targeted instruction from the disassembly is a RIP-relative MOV:
 *    140496fd0: 48 8B 05 A9 ED DF 02  (MOV RAX, qword ptr [g_ObjectManager])
 * 4. The signature `48 8B 05 ?? ?? ?? ??` specifically targets this `MOV RAX, [RIP + offset]` instruction,
 *    using wildcards for the 4-byte relative offset to remain stable across updates.
 */
const char* TRAFFIC_MANAGER_POINTER_LOAD_SIG = "48 8B 05 ?? ?? ?? ??";

/*
 * @brief Signature to find the ClearLocalVehicles function.
 *
 * HOW-TO-FIND:
 * 1. In a decompiler, search for a function that clears game vehicles. A good search term might be "ClearLocalVehicles" or look for loops that iterate and clear vehicle data.
 * 2. This signature targets a stable sequence of instructions at the beginning of the function, including the setup and the first couple of key MOV instructions.
 * 3. The targeted instructions from the user-provided disassembly are:
 *    1404ab170: 48 89 5C 24 08  (MOV QWORD PTR [RSP+0x8],RBX)
 *    1404ab175: 57              (PUSH RDI)
 *    1404ab176: 48 83 EC 20     (SUB RSP,0x20)
 *    1404ab17a: 48 8B D9        (MOV RBX,RCX)
 *    1404ab17d: 0F B6 FA        (MOVZX EDI,DL)
 *    1404ab180: 48 8B 89...     (MOV RCX,QWORD PTR [RCX+0xD0]) <- Contains pArrayObjectOffset
 *    1404ab187: 48 8B 83...     (MOV RAX,QWORD PTR [RBX+0xD8]) <- Contains vehicleCountOffset
 * 4. Wildcards are used for the stack adjustment (`SUB RSP, ??`) and the offsets themselves to ensure the signature is resilient to minor code shifts and recompilations.
 */
const char* CLEAR_LOCAL_VEHICLES_SIG = "48 89 5C 24 08 57 48 83 ? ? 48 8B D9 0F B6 FA 48 8B 89 ? ? ? ? 48 8B 83 ? ? ? ?";

/*
 * @brief Signature for the LEA instruction that reveals the size of the vehicle struct.
 *
 * HOW-TO-FIND:
 * 1. Inside the ClearLocalVehicles function, look for the main loop that iterates through the vehicle array.
 * 2. The instruction that increments the pointer to the next element in the array is what we need.
 * 3. This instruction is `LEA RCX,[RAX + 0x10]`. The immediate value `0x10` is the size.
 * 4. The signature targets this specific LEA instruction.
 *    1404ab1ab: 48 8D 48 10     (LEA RCX,[RAX+0x10])
 */
const char* STRUCT_SIZE_SIG = "48 8D 48 ??";

/*
 * @brief Signature to find the vehicleIdOffset.
 *
 * HOW-TO-FIND:
 * 1. Search within the `DebugCamera_RenderInfoOverlay` function (or a similar function that accesses vehicle data).
 * 2. Look for an instruction that reads a 32-bit integer from a vehicle object pointer at a specific offset.
 * 3. The targeted instructions from the user-provided disassembly are:
 *    140497503: 44 8B 86 F8 03 00 00  (MOV R8D, dword ptr [RSI + 0x3f8])
 * 4. The signature `48 85 F6 ? ? 44 8B 86 ? ? ? ?` is provided by the user.
 *    It targets the `TEST RSI, RSI` instruction followed by a conditional jump and then the `MOV R8D, dword ptr [RSI + offset]` instruction.
 *    The `? ?` wildcards make it robust against small changes in jump offsets.
 */
const char* VEHICLE_ID_OFFSET_SIG = "48 85 F6 ? ? 44 8B 86 ? ? ? ?";

/*
 * @brief Signature for the function that reads and formats many vehicle properties.
 *
 * HOW-TO-FIND:
 * 1. This is part of the large `DebugCamera_RenderInfoOverlay` function.
 * 2. This specific signature targets a block of code that handles the display of AI vehicle properties.
 * 3. The signature `48 85 D2 ...` corresponds to the prologue of a sub-section identified by the user.
 */
const char* VEHICLE_PROPERTIES_FUNC_SIG = "48 85 D2 ? ? ? ? ? ? 55 56 41 56 48 8D 6C 24 A0 48 81 EC";

/*
 * @brief Signature for the instruction reading the 'Patience' property.
 *
 * HOW-TO-FIND:
 * 1. Inside the properties function found above.
 * 2. The instruction is `MOVSS XMM9, dword ptr [RSI + 0x41c]`.
 * 3. `RSI` holds the pointer to the vehicle object. `0x41c` is the offset.
 */
const char* PATIENCE_OFFSET_SIG = "F3 44 0F 10 8E ?? ?? 00 00";

/*
 * @brief Signature for the instruction reading the 'Safety' property.
 *
 * HOW-TO-FIND:
 * 1. Inside the properties function, shortly after the 'Patience' read.
 * 2. The instruction is `MOVSS XMM10, dword ptr [RSI + 0x418]`.
 * 3. `0x418` is the offset.
 */
const char* SAFETY_OFFSET_SIG = "F3 44 0F 10 96 ?? ?? 00 00";

/*
 * @brief Signature for the instruction using the 'Target Speed' property.
 *
 * HOW-TO-FIND:
 * 1. Inside the properties function.
 * 2. The instruction is `MULSS XMM1, dword ptr [RSI + 0x40c]`. It multiplies a register by this value.
 * 3. `0x40c` is the offset.
 */
const char* TARGET_SPEED_OFFSET_SIG = "F3 0F 59 8E ?? ?? 00 00";

/*
 * @brief Signature for the instruction reading the 'Speed Limit' property.
 *
 * HOW-TO-FIND:
 * 1. This instruction is slightly separate from the main block but still in the same parent function.
 * 2. The instruction is `MOVSS XMM0, dword ptr [RSI + 0x408]`.
 * 3. `0x408` is the offset.
 */
const char* SPEED_LIMIT_OFFSET_SIG = "F3 0F 10 86 ?? ?? 00 00";

/*
 * @brief Signature for the instruction accessing the 'Lane Speed Input' property.
 *
 * HOW-TO-FIND:
 * 1. This property is read and passed as an argument to `FUN_1407f6320` which calculates the lane speed.
 * 2. The instruction is `MOV RDX, qword ptr [RSI + 0x400]`.
 * 3. `0x400` is the offset.
 */
const char* LANE_SPEED_INPUT_OFFSET_SIG = "48 8B 96 ?? ?? 00 00";

} // namespace

bool ObjectManagerFinder::TryFindOffsets(GameObjectVehicleService& owner) {
    // If we are already initialized, do nothing.
    if (m_isReady) {
        return true;
    }

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());

    // --- Step 1: Find the TrafficManager base address ---
    // This logic remains the same. We must find the TrafficManager pointer before we can find offsets within its structure.
    if (owner.GetTrafficManagerAddr() == 0) {
        logger->Info("Searching for TrafficManager address...");

        uintptr_t renderInfoOverlayFuncAddr = PatternFinder::Find(DEBUG_CAMERA_RENDER_INFO_OVERLAY_SIG);
        if (!renderInfoOverlayFuncAddr) {
            logger->Warn("Could not find DebugCamera_RenderInfoOverlay signature. The game has likely been updated.");
            return false;
        }
        logger->Info("Found DebugCamera_RenderInfoOverlay function at {0:#x}", renderInfoOverlayFuncAddr);

        uintptr_t movInstructionAddr = PatternFinder::Find(renderInfoOverlayFuncAddr, 0x300, TRAFFIC_MANAGER_POINTER_LOAD_SIG);
        if (!movInstructionAddr) {
            logger->Warn("Found the function, but could not find the TrafficManager pointer load instruction inside it.");
            return false;
        }
        logger->Info("Found TrafficManager pointer load instruction at {0:#x}", movInstructionAddr);

        uintptr_t nextInstructionAddr = movInstructionAddr + 7;
        int32_t relativeOffset = *reinterpret_cast<int32_t*>(movInstructionAddr + 3);
        uintptr_t pTrafficManagerAddr = nextInstructionAddr + relativeOffset;
        uintptr_t trafficManagerAddr = *reinterpret_cast<uintptr_t*>(pTrafficManagerAddr);

        if (!trafficManagerAddr) {
            logger->Debug("Resolved the global pointer, but it points to null. Will try again...");
            return false;
        }

        logger->Info("--- TRAFFIC MANAGER FOUND ---");
        logger->Info("Address: {0:#x}", trafficManagerAddr);
        logger->Info("-----------------------------");

        owner.SetTrafficManagerAddr(trafficManagerAddr);
    }

    // --- Step 2: Find offsets within ClearLocalVehicles ---
    // This function iterates through and clears the active vehicle array.
    // Signature approved by user: 48 89 5C ? ? 57 48 83 ? ? 48 8B D9 0F B6 FA 48 8B 89 ? ? ? ? 48 8B 83
    uintptr_t pfnClearVehicles = PatternFinder::Find("48 89 5C ? ? 57 48 83 ? ? 48 8B D9 0F B6 FA 48 8B 89 ? ? ? ? 48 8B 83");
    if (pfnClearVehicles) {
        // 2.1 Vehicle Array Offset (MOV RCX, [RCX + offset])
        uintptr_t addrArray = PatternFinder::Find(pfnClearVehicles, 128, "48 8B 89 ? ? ? ?");
        if (addrArray) {
            int32_t off = PatternFinder::ReadInt32(addrArray + 3);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetPArrayObjectOffset(off);
                logger->Info("--- Found Vehicle Array offset: 0x{:X}", off);
            } else { logger->Error("Vehicle Array offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Vehicle Array anchor"); }

        // 2.2 Vehicle Count Offset (MOV RAX, [RBX + offset])
        uintptr_t addrCount = PatternFinder::Find(pfnClearVehicles, 128, "48 8B 83 ? ? ? ?");
        if (addrCount) {
            int32_t off = PatternFinder::ReadInt32(addrCount + 3);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetVehicleCountOffset(off);
                logger->Info("--- Found Vehicle Count offset: 0x{:X}", off);
            } else { logger->Error("Vehicle Count offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Vehicle Count anchor"); }

        // 2.3 Vehicle Struct Size
        // Found via instruction: LEA RCX, [RAX + size] (the increment for the array loop)
        uintptr_t addrSize = PatternFinder::Find(pfnClearVehicles, 256, "48 8D 48 ??");
        if (addrSize) {
            uint8_t size = PatternFinder::ReadInt8(addrSize + 3);
            owner.SetSpawnedVehicleStructSize(size);
            logger->Info("--- Found Vehicle Struct Size: 0x{:X}", size);
        } else { logger->Error("FAILED to find Vehicle Struct Size anchor"); }
    } else { logger->Error("FAILED to find ClearLocalVehicles signature"); }

    // 2.4. Find and extract vehicleIdOffset
    // Found within DebugCamera_RenderInfoOverlay.
    // Logic: The game checks if the vehicle pointer is valid, then loads its ID for display.
    // Anchor: TEST RSI, RSI; JZ LAB_...; MOV R8D, dword ptr [RSI + offset]
    uintptr_t debugCameraRenderInfoOverlayFuncAddr = PatternFinder::Find(DEBUG_CAMERA_RENDER_INFO_OVERLAY_SIG);
    if (debugCameraRenderInfoOverlayFuncAddr) {
        uintptr_t addrIdLoad = PatternFinder::Find(debugCameraRenderInfoOverlayFuncAddr, 0x800, "48 85 F6 ? ? 44 8B 86 ? ? ? ?");
        if (addrIdLoad) {
            int32_t offset = PatternFinder::ReadInt32(addrIdLoad + 8);
            if (PatternFinder::IsSaneOffset(offset)) {
                owner.SetVehicleIdOffset(offset);
                logger->Info("--- Found Vehicle ID offset: 0x{:X}", offset);
            } else { logger->Error("Vehicle ID offset INVALID (0x{:X})", offset); }
        } else { logger->Error("FAILED to find Vehicle ID load anchor"); }
    } else { logger->Error("FAILED to find DebugCamera_RenderInfoOverlay for ID search"); }

    // 2.5. Find Local Player Controller Offset
    // This offset points to the main controller/player object within the manager.
    // Found in UpdateTrafficTrajectories (Anchor: MOV RCX, [R14 + offset]; TEST RCX, RCX)
    uintptr_t pfnLocalPlayerPath = PatternFinder::Find("49 8B 8E ? ? ? ? 48 85 C9 ? ? 41 f6 86 ? ? ? ? ? 75");
    if (pfnLocalPlayerPath) {
        int32_t offset = PatternFinder::ReadInt32(pfnLocalPlayerPath + 3);
        if (PatternFinder::IsSaneOffset(offset)) {
            owner.SetLocalPlayerControllerOffset(offset);
            logger->Info("--- Found LocalPlayerController offset: 0x{:X}", offset);
        } else {
            logger->Error("LocalPlayerController offset INVALID (0x{:X})", offset);
        }
    } else {
        logger->Warn("FAILED to find LocalPlayerController anchor in UpdateTrafficTrajectories");
    }

    // 2.6. Find Player Vehicle Actor Offset
    // This offset points to the actual vehicle object (Actor) within the controller.
    // Found in UpdateAllTraffic (Anchor: JNZ LAB_...; MOV RCX, [RCX + offset]; TEST RCX, RCX)
    uintptr_t pfnPlayerVehiclePath = PatternFinder::Find("75 33 48 8B ? ? 48 85 ? 74 14 48 8B ? FF 50 ?");
    if (pfnPlayerVehiclePath) {
        int32_t offset = PatternFinder::ReadInt8(pfnPlayerVehiclePath + 5);
        if (PatternFinder::IsSaneOffset(offset)) {
            owner.SetPlayerVehicleInControllerOffset(offset);
            logger->Info("--- Found PlayerVehicleInController offset: 0x{:X}", offset);
        } else {
            logger->Error("PlayerVehicle offset INVALID (0x{:X})", offset);
        }
    } else {
        logger->Warn("FAILED to find PlayerVehicle anchor in UpdateAllTraffic");
    }

    // --- Step 4: Find detailed vehicle property offsets ---
    // Found within FormatObjectDebugInfo (the function that prepares strings for the debug overlay).
    uintptr_t pfnFormatInfo = PatternFinder::Find(VEHICLE_PROPERTIES_FUNC_SIG);
    if (pfnFormatInfo) {
        logger->Info("Searching for detailed vehicle property offsets in FormatObjectDebugInfo...");

        // 4.1. Speed Limit Offset (Approved anchor: F3 0F 10 ? ? ? ? ?)
        uintptr_t addrSpeedLimit = PatternFinder::Find(pfnFormatInfo, 0x800, "F3 0F 10 ? ? ? ? ?");
        if (addrSpeedLimit) {
            int32_t off = PatternFinder::ReadInt32(addrSpeedLimit + 4);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetSpeedLimitOffset(off);
                logger->Info("--- Found Speed Limit offset: 0x{:X}", off);
            } else { logger->Error("Speed Limit offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Speed Limit anchor"); }

        // 4.2. Patience Offset (Approved anchor: F3 44 0F 10 8E ? ? ? ?)
        uintptr_t addrPatience = PatternFinder::Find(pfnFormatInfo, 0x800, "F3 44 0F 10 8E ? ? ? ?");
        if (addrPatience) {
            int32_t off = PatternFinder::ReadInt32(addrPatience + 5);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetPatienceOffset(off);
                logger->Info("--- Found Patience offset: 0x{:X}", off);
            } else { logger->Error("Patience offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Patience anchor"); }

        // 4.3. Safety Offset (Approved anchor: F3 44 0F 10 96 ? ? ? ?)
        uintptr_t addrSafety = PatternFinder::Find(pfnFormatInfo, 0x800, "F3 44 0F 10 96 ? ? ? ?");
        if (addrSafety) {
            int32_t off = PatternFinder::ReadInt32(addrSafety + 5);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetSafetyOffset(off);
                logger->Info("--- Found Safety offset: 0x{:X}", off);
            } else { logger->Error("Safety offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Safety anchor"); }

        // 4.4. Lane Speed Input Offset (Approved anchor: 48 8B 96 ? ? ? ?)
        uintptr_t addrLaneSpeed = PatternFinder::Find(pfnFormatInfo, 0x800, "48 8B 96 ? ? ? ?");
        if (addrLaneSpeed) {
            int32_t off = PatternFinder::ReadInt32(addrLaneSpeed + 3);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetLaneSpeedInputOffset(off);
                logger->Info("--- Found Lane Speed Input offset: 0x{:X}", off);
            } else { logger->Error("Lane Speed Input offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Lane Speed Input anchor"); }

        // 4.5. Target Speed Offset (Approved anchor: F3 0F 59 8E ? ? ? ?)
        uintptr_t addrTargetSpeed = PatternFinder::Find(pfnFormatInfo, 0x800, "F3 0F 59 8E ? ? ? ?");
        if (addrTargetSpeed) {
            int32_t off = PatternFinder::ReadInt32(addrTargetSpeed + 4);
            if (PatternFinder::IsSaneOffset(off)) {
                owner.SetTargetSpeedOffset(off);
                logger->Info("--- Found Target Speed offset: 0x{:X}", off);
            } else { logger->Error("Target Speed offset INVALID (0x{:X})", off); }
        } else { logger->Error("FAILED to find Target Speed anchor"); }

        // 4.6. Vehicle Sub-Object Offset (Anchor: MOV RAX, [RSI+10]; LEA RCX, [RSI+10])
        uintptr_t addrSubObj = PatternFinder::Find(pfnFormatInfo, 0x800, "48 8B 46 ? 48 8D 4E ? 41");
        if (addrSubObj) {
            uint8_t off = PatternFinder::ReadInt8(addrSubObj + 3);
            owner.SetVehicleSubObjectOffset(off);
            logger->Info("--- Found Vehicle Sub-Object offset: 0x{:X}", off);
        } else { logger->Error("FAILED to find Vehicle Sub-Object anchor"); }

        // 4.7. Acceleration VTable Offset (Anchor: CALL [RAX+10]; MOV RAX, [RSI+10]; ...)
        uintptr_t addrAccelFn = PatternFinder::Find(pfnFormatInfo, 0x800, "FF 50 ? 48 8B 46 ? 48 8D 4E ? 0F 57 F6");
        if (addrAccelFn) {
            uint8_t off = PatternFinder::ReadInt8(addrAccelFn + 2);
            owner.SetVtableGetAccelerationOffset(off);
            logger->Info("--- Found Acceleration VTable offset: 0x{:X}", off);
        } else { logger->Error("FAILED to find Acceleration VTable anchor"); }

        // 4.8. Speed VTable Offset (Anchor: CALL [RAX+08]; MOVSD [RSP+40], XMM9)
        uintptr_t addrSpeedFn = PatternFinder::Find(pfnFormatInfo, 0x800, "FF 50 ? F2 44 0F");
        if (addrSpeedFn) {
            uint8_t off = PatternFinder::ReadInt8(addrSpeedFn + 2);
            owner.SetVtableGetCurrentSpeedOffset(off);
            logger->Info("--- Found Speed VTable offset: 0x{:X}", off);
        } else { logger->Error("FAILED to find Speed VTable anchor"); }
    } else { logger->Error("FAILED to find FormatObjectDebugInfo for property search"); }

    // Final check for all offsets before declaring readiness
    if (!owner.GetTrafficManagerAddr() || !owner.GetPArrayObjectOffset() || !owner.GetVehicleCountOffset() || !owner.GetSpawnedVehicleStructSize() || !owner.GetVehicleIdOffset() ||
        !owner.GetPatienceOffset() || !owner.GetSafetyOffset() || !owner.GetTargetSpeedOffset() || 
        !owner.GetSpeedLimitOffset() || !owner.GetLaneSpeedInputOffset() || !owner.GetLocalPlayerControllerOffset() || !owner.GetPlayerVehicleInControllerOffset() ||
        !owner.GetVehicleSubObjectOffset() || !owner.GetVtableGetCurrentSpeedOffset() || !owner.GetVtableGetAccelerationOffset())
    {
        return false;
    }

    logger->Info("--- ALL OFFSETS FOUND. ObjectManagerFinder is ready. ---");
    m_isReady = true;
    return true;
}

} // namespace Data::GameData::Finders
SPF_NS_END
