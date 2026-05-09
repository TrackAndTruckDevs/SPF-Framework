#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"
#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Signature to find the UFS manager accessor logic.
 * Matches the start of FUN_14026c440 which validates the manager index and loads the array.
 * 14026c440: 48 83 ? ?           SUB RSP, ?
 * 14026c444: 48 63 ?             MOVSXD RDX, ?
 * 14026c447: 48 3b ? ? ? ? ?     CMP RDX, [g_UfsManagersCount]  <-- Offset +0x07
 * 14026c44e: 73 ?                JNC ...
 * 14026c450: 48 8b ? ? ? ? ?     MOV RAX, [g_UfsManagersArray]  <-- Offset +0x10
 */
const char* GET_MANAGER_PFN_SIG = "48 83 ? ? 48 63 ? 48 3b ? ? ? ? ? 73 ? 48 8b ? ? ? ? ? 48 8b";

/**
 * @brief Signature to find the UFS_RegisterMount function.
 * This is the primary function for registering virtual paths to physical devices.
 */
const char* REGISTER_MOUNT_FUNC_SIG = "48 89 5c ? ? 48 89 74 ? ? 55 57 41 ? 41 ? 41 ? 48 8d ? ? ? 48 81 ec ? ? ? ? 48 8b 3d ? ? ? ? 4c";

/**
 * @brief Signature for extracting list head anchor offset from UFS_RegisterMount.
 */
const char* MOUNT_OFFSETS_SIG = "49 8d ? ? 49 8b ? ? 88";

/**
 * @brief Signature for extracting node structure and string buffer offsets.
 * Matches: 
 * MOV [RSI + 0x10], RCX (NodeDeviceOffset)
 * LEA RCX, [RSI + 0x18] (NodeVPathOffset)
 * MOV [RCX + 0x08], RAX (StringBufferOffset)
 */
const char* MOUNT_NODE_STRUCTURE_SIG = "48 89 ? ? 48 8d ? ? 48 89";

/**
 * @brief Signature for extracting the physical path offset from a device object.
 */
const char* PHYS_PATH_OFFSET_SIG = "49 8b ? ? 48 8d ? ? ? ? ? 48 89 ? ? ? 48 8d";

/**
 * @brief Unique signature to find the middle of SelectProfile function.
 * Matches stack saving operations and a test on DL:
 * 1.59 Ghidra Example:
 * 140eab70c: 0f 29 70 b8           MOVAPS xmmword ptr [RAX + -0x48], XMM6
 * 140eab710: 0f 29 78 a8           MOVAPS xmmword ptr [RAX + -0x58], XMM7
 * 140eab714: 44 0f 29 40 98        MOVAPS xmmword ptr [RAX + -0x68], XMM8
 * 140eab719: 84 d2                 TEST DL, DL
 * 140eab71b: 0f 84 ...             JZ ...
 */
const char* PROFILE_LOGIC_ANCHOR_SIG = "0F ?? ?? ?? 0F ?? ?? ?? 44 0F ?? ?? ?? 84 D2 0F";

} // namespace

bool FileSystemDataFinder::TryFindOffsets(GameObjectFileSystemService& owner) {
    if (m_isReady) return true;

    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
    logger->Info("Starting high-quality dynamic search for FileSystem (UFS) structures...");

    // --- Step 1: Find Managers Array & Count via GET_MANAGER_PFN_SIG ---
    uintptr_t pfnGetManager = PatternFinder::Find(GET_MANAGER_PFN_SIG);
    if (pfnGetManager) {
        logger->Debug("Anchor #1: Found UFS Manager accessor logic at {0:#x}", pfnGetManager);
        
        uintptr_t countAddr = PatternFinder::GetRipAddress(pfnGetManager + 0x07, 3, 7);
        if (countAddr) {
            owner.SetManagersCountAddr(countAddr);
            logger->Debug("  -> Found ManagersCountAddr: 0x{:X}", countAddr);
        } else { logger->Error("  !! FAILED to extract ManagersCount address."); }

        uintptr_t arrayAddr = PatternFinder::GetRipAddress(pfnGetManager + 0x10, 3, 7);
        if (arrayAddr) {
            owner.SetDevicesArrayAddr(arrayAddr);
            logger->Debug("  -> Found ManagersArrayAddr: 0x{:X}", arrayAddr);
        } else { logger->Error("  !! FAILED to extract ManagersArray address."); }
    } else { logger->Warn("Anchor #1: FAILED to find GET_MANAGER_PFN signature."); }

    // --- Step 2: Find Offsets via UFS_RegisterMount ---
    uintptr_t pfnRegisterMount = PatternFinder::Find(REGISTER_MOUNT_FUNC_SIG);
    if (pfnRegisterMount) {
        logger->Debug("Anchor #2: Found UFS_RegisterMount at {0:#x}", pfnRegisterMount);
        
        // 2.1. Mount List Head Anchor Offset (Typically 0x70)
        uintptr_t sigAddrMountManager = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_OFFSETS_SIG);
        if (sigAddrMountManager) {
            uint8_t listHeadOff = PatternFinder::ReadInt8(sigAddrMountManager + 7);
            if (PatternFinder::IsSaneOffset(listHeadOff)) {
                owner.SetMountListHeadOffset(listHeadOff);
                logger->Debug("  -> Found MountListHeadOffset: 0x{:X}", listHeadOff);
            } else { logger->Error("  !! Mount List Head offset INVALID (0x{:X})", listHeadOff); }
        } else { logger->Error("  !! FAILED to find Mount Manager offsets anchor."); }

        // 2.2. Node structure and String Buffer offsets
        uintptr_t sigAddrNode = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_NODE_STRUCTURE_SIG);
        if (sigAddrNode) {
            uint8_t deviceOff = PatternFinder::ReadInt8(sigAddrNode + 3);
            uint8_t vpathOff = PatternFinder::ReadInt8(sigAddrNode + 7);
            uint8_t stringBuffOff = PatternFinder::ReadInt8(sigAddrNode + 11);
            if (PatternFinder::IsSaneOffset(deviceOff) && PatternFinder::IsSaneOffset(vpathOff)) {
                owner.SetNodeDeviceOffset(deviceOff);
                owner.SetNodeVPathOffset(vpathOff);
                owner.SetStringBufferOffset(stringBuffOff);
                logger->Debug("  -> Found NodeDeviceOffset: 0x{:X}", deviceOff);
                logger->Debug("  -> Found NodeVPathOffset: 0x{:X}", vpathOff);
                logger->Debug("  -> Found StringBufferOffset: 0x{:X}", stringBuffOff);
            } else { logger->Error("  !! Node offsets INVALID (Dev:0x{:X}, VP:0x{:X}, SB:0x{:X})", deviceOff, vpathOff, stringBuffOff); }
        } else { logger->Error("  !! FAILED to find Mount Node structure anchor."); }

        // 2.3. Physical Device Path Offset (Typically 0x10)
        uintptr_t sigAddrPhys = PatternFinder::Find(pfnRegisterMount, 4096, PHYS_PATH_OFFSET_SIG);
        if (sigAddrPhys) {
            uint8_t physOff = PatternFinder::ReadInt8(sigAddrPhys + 3);
            if (PatternFinder::IsSaneOffset(physOff)) {
                owner.SetPhysicalDevicePathOffset(physOff);
                logger->Debug("  -> Found PhysicalDevicePathOffset: 0x{:X}", physOff);
            } else { logger->Error("  !! PhysicalPathOffset INVALID (0x{:X})", physOff); }
        } else { logger->Error("  !! FAILED to find Physical Path offset anchor."); }
    } else { logger->Warn("Anchor #2: FAILED to find UFS_RegisterMount signature."); }

    // --- Step 3: Find Active Profile Data via New v1.59 Signature ---
    const char* PROFILE_V159_SIG = "48 8B 05 ? ? ? ? 48 85 C0 74 08 48 05 ? ? ? ? EB 03 48 8B C5 4C 8B A0";
    uintptr_t profileBlockAddr = PatternFinder::Find(PROFILE_V159_SIG);

    if (profileBlockAddr) {
        logger->Debug("Found profile data block at {0:#x}", profileBlockAddr);

        // 3.1. Find global gamePtr: 48 8b 05 [RIP_OFF]
        uintptr_t gamePtr = PatternFinder::GetRipAddress(profileBlockAddr, 3, 7);
        if (gamePtr) {
            owner.SetGamePtrAddr(gamePtr);
            logger->Debug("  -> Found GamePtrAddr: 0x{:X}", gamePtr);
        } else { logger->Error("  !! FAILED to resolve RIP address for GamePtr."); }

        // 3.2. Find base adjustment: 48 05 [IMM32]
        int32_t adj = PatternFinder::ReadInt32(profileBlockAddr + 14);
        owner.SetGamePtrAdjustment(adj);
        logger->Debug("  -> Found GamePtr Adjustment: {}", adj);

        // 3.3. Find profile offset: 4c 8b a0 [OFF32]
        uint32_t profileOff = PatternFinder::ReadInt32(profileBlockAddr + 26);
        if (PatternFinder::IsSaneOffset(profileOff)) {
            owner.SetProfileHandleOffset(profileOff);
            logger->Debug("  -> Found ProfileHandleOffset: 0x{:X}", profileOff);
        } else { logger->Error("  !! ProfileHandleOffset is INVALID (0x{:X}).", profileOff); }

    } else {
        logger->Warn("FAILED to find PROFILE_V159 signature.");
    }

    // --- Final Readiness Check ---
    m_isReady = (owner.GetDevicesArrayAddr() != 0 && 
                 owner.GetManagersCountAddr() != 0 &&
                 owner.GetMountListHeadOffset() != 0 &&
                 owner.GetNodeDeviceOffset() != 0 &&
                 owner.GetNodeVPathOffset() != 0 &&
                 owner.GetStringBufferOffset() != 0 &&
                 owner.GetPhysicalDevicePathOffset() != 0);

    if (m_isReady) {
        logger->Info("--- FILESYSTEM OFFSETS FOUND. FileSystemDataFinder is ready. ---");
    }

    return m_isReady;
}

} // namespace Data::GameData::Finders
SPF_NS_END
