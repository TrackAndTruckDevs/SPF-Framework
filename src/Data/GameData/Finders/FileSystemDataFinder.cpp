#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>


SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/**
 * @brief Signature to find the UFS manager accessor logic.
 * Matches the start of the function which validates the manager index and loads the array.
 *
 * Target Code Snippet (Verified for Game Version 1.60):
 * 14014fc50 48 83 ec 48                SUB        RSP,0x48
 * 14014fc54 48 63 d1                   MOVSXD     RDX,ECX
 * 14014fc57 48 3b 15 d2 48 49 02       CMP        RDX,qword ptr [g_UfsManagersCount]  <-- Offset +0x07
 * 14014fc5e 73 10                      JNC        LAB_14014fc70
 * 14014fc60 48 8b 05 c1 48 49 02       MOV        RAX,qword ptr [g_UfsManagersArray]  <-- Offset +0x10
 *
 * Strategy:
 * Use value ranges for flexible stack allocations, register usage, and branch offsets.
 */
const char* UFS_GET_MANAGER_ACCESSOR_SIG = "48 83 ec [00-80] [0-8?] 48 63 [c0-ff] [0-8?] 48 3b [05-3d]";

/**
 * @brief Unique error string to find UFS_RegisterMount entry point.
 * Verified Address (v1.60): 140155fc0
 */
const char* UFS_REGISTER_MOUNT_STR = "[ufs] The table of UFS mounted devices is full";

/**
 * @brief Signature for Node Structure (DevicePtr and VirtualPath).
 *
 * Ghidra 1.60 Analysis:
 * 1401560fd 48 89 48 10                MOV  qword ptr [RAX + 0x10], RCX
 * 140156101 48 8d 48 18                LEA  RCX, [RAX + 0x18]
 */
const char* MOUNT_NODE_STRUCT_SIG = "[48-4F] 89 [40-BF] ?? [48-4F] 8D [40-BF]";

/**
 * @brief Signature for StringBuffer offset within node registration.
 *
 * Ghidra 1.60 Analysis:
 * 140156110 48 89 41 08                MOV  qword ptr [RCX + 0x8], RAX
 */
const char* MOUNT_STR_BUFF_SIG = "48 89 [40-7F] ?? [4-20?] E8";

/**
 * @brief Signature for Mount List Head anchor.
 * Links the list pointer load to subsequent stack operations for uniqueness.
 *
 * Ghidra 1.59 Analysis (verified at 14027eb06):
 * 14027eb06 49 8b 5d 78                MOV  RBX, qword ptr [R13 + 0x78]
 * 14027eb0a 88 44 24 44                MOV  byte ptr [RSP + 0x44], AL
 * 14027eb0e 0f b6 85 d8 00 00 00       MOVZX EAX, byte ptr [RBP + 0xd8]
 *
 * Ghidra 1.60 Analysis (verified at 1401560c6):
 * 1401560c6 49 8b 9d 88 00 00 00       MOV  RBX, qword ptr [R13 + 0x88]
 * 1401560cd 88 44 24 54                MOV  byte ptr [RSP + 0x54], AL
 * 1401560d1 0f b6 85 f8 00 00 00       MOVZX EAX, byte ptr [RBP + 0xf8]
 */
const char* MOUNT_LIST_HEAD_SIG = "49 8B [40-BF] ?? [0-3?] 88 44";

/**
 * @brief Signature for Physical Device Path offset.
 * Based on sequence: MOV reg, [reg+off] followed by LEA.
 *
 * Ghidra 1.60 Analysis:
 * 140156336 49 8b 7f 10                MOV  RDI, qword ptr [R15 + 0x10]
 * 14015633a 48 8d 05 f7 21 f5 01       LEA  RAX, [PTR_FUN_1420a8538]
 */
const char* PHYS_PATH_SIG = "49 8B [40-7F] [0-8?] 48 8D";

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

}  // namespace

bool FileSystemDataFinder::TryFindOffsets(GameObjectFileSystemService& owner) {
  if (m_isReady) return true;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Starting high-quality dynamic search for FileSystem (UFS) structures...");

  // 1. Find the entry point of the GetManagerAccessor function.
  uintptr_t pfnGetManager = PatternFinder::Find(UFS_GET_MANAGER_ACCESSOR_SIG);
  if (pfnGetManager) {
    logger->Debug("1. GetManagerAccessor found at 0x{:X}", pfnGetManager);

    // 1.1 [DATA: Managers Count Pointer]
    uintptr_t addrCmp = PatternFinder::Find(pfnGetManager, 64, "48 3b [05-3d]");
    if (addrCmp) {
      uintptr_t countAddr = PatternFinder::GetRipAddress(addrCmp, 3, 7);
      if (countAddr) {
        owner.SetManagersCountAddr(countAddr);
        logger->Debug("1.1 [DATA: Managers Count] Found at 0x{:X}", countAddr);
      } else {
        logger->Error("1.1 [DATA: Managers Count] Failed to resolve RIP address.");
      }
    } else {
      logger->Error("1.1 [DATA: Managers Count] FAILED to find CMP instruction.");
    }

    // 1.2 [DATA: Managers Array Pointer]
    uintptr_t addrMov = PatternFinder::Find(pfnGetManager, 64, "48 8b [05-3d]");
    if (addrMov) {
      uintptr_t arrayAddr = PatternFinder::GetRipAddress(addrMov, 3, 7);
      if (arrayAddr) {
        owner.SetDevicesArrayAddr(arrayAddr);
        logger->Debug("1.2 [DATA: Managers Array] Found at 0x{:X}", arrayAddr);
      } else {
        logger->Error("1.2 [DATA: Managers Array] Failed to resolve RIP address.");
      }
    } else {
      logger->Error("1.2 [DATA: Managers Array] FAILED to find MOV instruction.");
    }
  } else {
    logger->Error("1. Failed to find GetManagerAccessor function start.");
  }

  // --- Step 2: Find Offsets via UFS_RegisterMount ---
  /**
   * SEARCH STRATEGY:
   * Locate UFS_RegisterMount using its unique error string.
   * Verified for v1.60 at 0x140155fc0.
   */
  uintptr_t pfnRegisterMount = PatternFinder::FindFunctionByString(UFS_REGISTER_MOUNT_STR, true);
  if (pfnRegisterMount) {
    logger->Debug("2. UFS_RegisterMount found at 0x{:X}", pfnRegisterMount);

    // 2.1 [OFFSETS: Node Structure]
    uintptr_t addrNode = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_NODE_STRUCT_SIG);
    if (addrNode) {
      uint8_t modrm1 = *(uint8_t*)(addrNode + 2);
      int32_t deviceOff = (modrm1 >= 0x80) ? PatternFinder::ReadInt32(addrNode + 3) : PatternFinder::ReadInt8(addrNode + 3);

      uintptr_t addrLea = PatternFinder::Find(addrNode + 2, 32, "[48-4F] 8D [40-BF]");
      if (addrLea) {
        uint8_t modrm2 = *(uint8_t*)(addrLea + 2);
        int32_t vpathOff = (modrm2 >= 0x80) ? PatternFinder::ReadInt32(addrLea + 3) : PatternFinder::ReadInt8(addrLea + 3);

        if (PatternFinder::IsSaneOffset(deviceOff) && PatternFinder::IsSaneOffset(vpathOff)) {
          owner.SetNodeDeviceOffset(deviceOff);
          owner.SetNodeVPathOffset(vpathOff);
          logger->Debug("2.1 [NODE] Dev: 0x{:X}, VPath: 0x{:X}", deviceOff, vpathOff);
        } else {
          logger->Error("2.1 [NODE] Insane offsets: Dev 0x{:X}, VPath 0x{:X}", deviceOff, vpathOff);
        }
      } else {
        logger->Error("2.1 [NODE] Failed to find VirtualPath LEA.");
      }
    } else {
      logger->Error("2.1 [NODE] Failed to find Node Structure signature.");
    }

    // StringBuffer Offset (M1.3: 140156110 48 89 41 08)
    uintptr_t addrStr = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_STR_BUFF_SIG);
    if (addrStr) {
      uint8_t modrm = *(uint8_t*)(addrStr + 2);
      int32_t strBuffOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrStr + 3) : (modrm >= 0x40) ? PatternFinder::ReadInt8(addrStr + 3) : 0;
      owner.SetStringBufferOffset(strBuffOff);
      logger->Debug("2.1 [STRBUFF] Found: 0x{:X}", strBuffOff);
    } else {
      logger->Error("2.1 [STRBUFF] Failed to find StringBuffer MOV.");
    }

    // 2.2 [OFFSET: Mount Counter]
    uintptr_t addrInc = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_LIST_HEAD_SIG);
    if (addrInc) {
      uint8_t modrm = *(uint8_t*)(addrInc + 2);
      int32_t listHeadOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrInc + 3) : PatternFinder::ReadInt8(addrInc + 3);
      if (PatternFinder::IsSaneOffset(listHeadOff)) {
        owner.SetMountListHeadOffset(listHeadOff);
        logger->Debug("2.2 [COUNTER] Found: 0x{:X}", listHeadOff);
      } else {
        logger->Error("2.2 [COUNTER] Insane offset: 0x{:X}", listHeadOff);
      }
    } else {
      logger->Error("2.2 [COUNTER] Failed to find Increment signature.");
    }

    // 2.3 [OFFSET: Physical Path]
    uintptr_t addrPhys = PatternFinder::Find(pfnRegisterMount, 4096, PHYS_PATH_SIG);
    if (addrPhys) {
      uint8_t modrm = *(uint8_t*)(addrPhys + 2);
      int32_t physOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrPhys + 3) : (modrm >= 0x40) ? PatternFinder::ReadInt8(addrPhys + 3) : 0;
      if (PatternFinder::IsSaneOffset(physOff)) {
        owner.SetPhysicalDevicePathOffset(physOff);
        logger->Debug("2.3 [PHYS PATH] Found: 0x{:X}", physOff);
      } else {
        logger->Error("2.3 [PHYS PATH] Insane offset: 0x{:X}", physOff);
      }
    } else {
      logger->Error("2.3 [PHYS PATH] Failed to find Physical Path signature.");
    }
  } else {
    logger->Error("2. FAILED to find UFS_RegisterMount entry point.");
  }

  // --- Step 3: Find Active Profile Data ---
  // REMOVED: Core profile offsets (GamePtr, ProfileHandle) are now centrally managed by SessionDataFinder.
  // FileSystemDataFinder now relies on GameObjectSessionService for these root addresses.

  // --- Final Readiness Check ---
  m_isReady = (owner.GetDevicesArrayAddr() != 0 && owner.GetManagersCountAddr() != 0 && owner.GetMountListHeadOffset() != 0 && owner.GetNodeDeviceOffset() != 0 && owner.GetNodeVPathOffset() != 0 && owner.GetStringBufferOffset() != 0 &&
               owner.GetPhysicalDevicePathOffset() != 0);

  if (m_isReady) {
    logger->Info("--- FILESYSTEM OFFSETS FOUND. FileSystemDataFinder is ready. ---");
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
