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

    // --- Step 2: Dynamically find offsets for vehicle data ---
    logger->Info("Searching for Vehicle Data Offsets...");

    uintptr_t sigMatchAddr = PatternFinder::Find(CLEAR_LOCAL_VEHICLES_SIG);
    if (!sigMatchAddr) {
        logger->Warn("Could not find ClearLocalVehicles function signature. This is critical for finding vehicle data offsets.");
        return false;
    }
    logger->Info("Found ClearLocalVehicles signature at {0:#x}", sigMatchAddr);

    // 2.1. Extract pArrayObjectOffset from the signature match.
    // The signature string is: "48 89 5C 24 08 57 48 83 ? ? 48 8B D9 0F B6 FA 48 8B 89 ? ? ? ?"
    // The part we care about is `48 8B 89` which is `MOV RCX, [RCX + offset]`.
    // The offset itself starts 19 bytes after the beginning of the signature match.
    uintptr_t pArrayObjectOffset = *reinterpret_cast<uint32_t*>(sigMatchAddr + 19);
    owner.SetPArrayObjectOffset(pArrayObjectOffset);
    logger->Info("Found pArrayObjectOffset: {0:#x}", pArrayObjectOffset);

    // 2.2. Extract vehicleCountOffset from the signature match.
    // The signature continues with `... 48 8B 83 ? ? ? ?`
    // This corresponds to `MOV RAX, [RBX + offset]`.
    // The offset starts 29 bytes after the beginning of the signature match.
    uintptr_t vehicleCountOffset = *reinterpret_cast<uint32_t*>(sigMatchAddr + 26);
    owner.SetVehicleCountOffset(vehicleCountOffset);
    logger->Info("Found vehicleCountOffset: {0:#x}", vehicleCountOffset);

    // 2.3. Find and extract spawnedVehicleStructSize by searching for the LEA instruction.
    // We search from the beginning of the matched function signature within a reasonable range.
    uintptr_t structSizeInstructionAddr = PatternFinder::Find(sigMatchAddr, 0x100, STRUCT_SIZE_SIG);
    if (!structSizeInstructionAddr) {
        logger->Warn("Could not find spawnedVehicleStructSize instruction signature (LEA) inside ClearLocalVehicles.");
        return false;
    }
    // The instruction is `48 8D 48 ??` (LEA RCX, [RAX + size]). The size is a single byte,
    // located 3 bytes after the start of the instruction.
    uintptr_t spawnedVehicleStructSize = *reinterpret_cast<uint8_t*>(structSizeInstructionAddr + 3);
    owner.SetSpawnedVehicleStructSize(spawnedVehicleStructSize);
    logger->Info("Found spawnedVehicleStructSize: {0:#x}", spawnedVehicleStructSize);

    // Sanity check to ensure we found valid, non-zero offsets.
    if (!pArrayObjectOffset || !vehicleCountOffset || !spawnedVehicleStructSize) {
        logger->Warn("Found the function but failed to extract one or more offsets (they were zero). Will retry.");
        return false;
    }

    // 2.4. Find and extract vehicleIdOffset
    // This offset is found within DebugCamera_RenderInfoOverlay function.
    uintptr_t debugCameraRenderInfoOverlayFuncAddr = PatternFinder::Find(DEBUG_CAMERA_RENDER_INFO_OVERLAY_SIG);
    if (!debugCameraRenderInfoOverlayFuncAddr) {
        logger->Warn("Could not find DebugCamera_RenderInfoOverlay function for vehicleIdOffset search. Will retry.");
        return false;
    }

    // Search for the VEHICLE_ID_OFFSET_SIG within a reasonable range (e.g., 0x600 bytes)
    // from the beginning of DebugCamera_RenderInfoOverlay.
    uintptr_t vehicleIdInstructionAddr = PatternFinder::Find(debugCameraRenderInfoOverlayFuncAddr, 0x800, VEHICLE_ID_OFFSET_SIG);
    if (!vehicleIdInstructionAddr) {
        logger->Warn("Could not find vehicleIdOffset instruction signature inside DebugCamera_RenderInfoOverlay. Will retry.");
        return false;
    }

    // The signature is "48 85 F6 ? ? 44 8B 86 ? ? ? ?"
    // The offset for the vehicle ID is a 4-byte value located 8 bytes after the start of this pattern.
    // Example: 44 8B 86 F8 03 00 00 -> F8 03 00 00 is the offset (0x3F8)
    uintptr_t vehicleIdOffset = *reinterpret_cast<uint32_t*>(vehicleIdInstructionAddr + 8);
    owner.SetVehicleIdOffset(vehicleIdOffset);
    logger->Info("Found vehicleIdOffset: {0:#x}", vehicleIdOffset);

    // Final sanity check for vehicleIdOffset.
    if (!vehicleIdOffset) {
        logger->Warn("Failed to extract vehicleIdOffset (it was zero). Will retry.");
        return false;
    }

    // --- Step 4: Find detailed vehicle property offsets using an efficient chained search ---
    if (owner.GetPatienceOffset() == 0) { // Only search if not already found
        logger->Info("Searching for detailed vehicle property offsets...");
        uintptr_t propsFuncAddr = PatternFinder::Find(VEHICLE_PROPERTIES_FUNC_SIG);
        if (!propsFuncAddr) {
            logger->Warn("Could not find vehicle properties function signature. Cannot find detailed offsets.");
            return false;
        }
        logger->Info("Found vehicle properties function at {0:#x}", propsFuncAddr);

        uintptr_t searchBase = propsFuncAddr;
        const size_t searchRange = 0x400;

        // 4.1. Speed Limit Offset (This appears earliest)
        uintptr_t speedLimitAddr = PatternFinder::Find(searchBase, searchRange, SPEED_LIMIT_OFFSET_SIG);
        if (speedLimitAddr) {
            uintptr_t offset = *reinterpret_cast<uint32_t*>(speedLimitAddr + 4);
            owner.SetSpeedLimitOffset(offset);
            logger->Info("Found Speed Limit Offset: {0:#x}", offset);
            searchBase = speedLimitAddr + 1; // Continue search from here
        } else {
            logger->Warn("Could not find Speed Limit Offset signature.");
        }

        // 4.2. Patience Offset
        uintptr_t patienceAddr = PatternFinder::Find(searchBase, searchRange - (searchBase - propsFuncAddr), PATIENCE_OFFSET_SIG);
        if (patienceAddr) {
            uintptr_t offset = *reinterpret_cast<uint32_t*>(patienceAddr + 5);
            owner.SetPatienceOffset(offset);
            logger->Info("Found Patience Offset: {0:#x}", offset);
            searchBase = patienceAddr + 1;
        } else {
            logger->Warn("Could not find Patience Offset signature.");
        }

        // 4.3. Safety Offset
        uintptr_t safetyAddr = PatternFinder::Find(searchBase, searchRange - (searchBase - propsFuncAddr), SAFETY_OFFSET_SIG);
        if (safetyAddr) {
            uintptr_t offset = *reinterpret_cast<uint32_t*>(safetyAddr + 5);
            owner.SetSafetyOffset(offset);
            logger->Info("Found Safety Offset: {0:#x}", offset);
            searchBase = safetyAddr + 1;
        } else {
            logger->Warn("Could not find Safety Offset signature.");
        }
        
        // 4.4. Lane Speed Input Offset
        uintptr_t laneSpeedInputAddr = PatternFinder::Find(searchBase, searchRange - (searchBase - propsFuncAddr), LANE_SPEED_INPUT_OFFSET_SIG);
        if (laneSpeedInputAddr) {
            uintptr_t offset = *reinterpret_cast<uint32_t*>(laneSpeedInputAddr + 3);
            owner.SetLaneSpeedInputOffset(offset);
            logger->Info("Found Lane Speed Input Offset: {0:#x}", offset);
            searchBase = laneSpeedInputAddr + 1;
        } else {
            logger->Warn("Could not find Lane Speed Input Offset signature.");
        }

        // 4.5. Target Speed Offset
        uintptr_t targetSpeedAddr = PatternFinder::Find(searchBase, searchRange - (searchBase - propsFuncAddr), TARGET_SPEED_OFFSET_SIG);
        if (targetSpeedAddr) {
            uintptr_t offset = *reinterpret_cast<uint32_t*>(targetSpeedAddr + 4);
            owner.SetTargetSpeedOffset(offset);
            logger->Info("Found Target Speed Offset: {0:#x}", offset);
        } else {
            logger->Warn("Could not find Target Speed Offset signature.");
        }
    }

    // Final check for all offsets before declaring readiness
    if (!owner.GetTrafficManagerAddr() || !owner.GetPArrayObjectOffset() || !owner.GetVehicleCountOffset() || !owner.GetSpawnedVehicleStructSize() || !owner.GetVehicleIdOffset() ||
        !owner.GetPatienceOffset() || !owner.GetSafetyOffset() || !owner.GetTargetSpeedOffset() || 
        !owner.GetSpeedLimitOffset() || !owner.GetLaneSpeedInputOffset())
    {
        // Don't log a warning here every frame, just fail silently and retry on the next tick.
        return false;
    }

    logger->Info("--- ALL OFFSETS FOUND. ObjectManagerFinder is ready. ---");
    m_isReady = true;
    return true;
}

} // namespace Data::GameData::Finders
SPF_NS_END
