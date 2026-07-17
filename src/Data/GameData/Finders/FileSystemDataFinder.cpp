#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Utils/FinderLog.hpp"
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
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14014fc50[14014fc50]) ---/
 * 14014fc50  48 83 EC 48                   SUB RSP,0x48
 * 14014fc54  48 63 D1                      MOVSXD RDX,ECX
 * 14014fc57  48 3B 15 D2 48 49 02          CMP RDX,qword ptr [0x1425e4530] -> [Managers Count]
 * 14014fc5e  73 10                         JNC 0x14014fc70
 * 14014fc60  48 8B 05 C1 48 49 02          MOV RAX,qword ptr [0x1425e4528] -> [Managers Array]
 */
const char* UFS_GET_MANAGER_ACCESSOR_SIG = "[SUB r64, imm8] [MOVSXD r64, r32] 48 3B 15 ? ? ? ? [JAE rel8] [MOV r64, [rip+off32]]";

/**
 * @brief Unique error string to find UFS_RegisterMount entry point.
 * Verified Address (v1.60): 140155fc0
 */
const char* UFS_REGISTER_MOUNT_STR = "[ufs] The table of UFS mounted devices is full";

/**
 * @brief Signature for Node Structure (DevicePtr and VirtualPath).
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 1401560fd  48 89 48 10                   MOV qword ptr [RAX + 0x10],RCX -> [Device offset]
 * 140156101  48 8D 48 18                   LEA RCX,[RAX + 0x18] -> [VirtualPath]
 * 140156105  48 8D 05 12 41 BA 01          LEA RAX,[0x141cfa21e]
 * 14015610c  4C 89 61 10                   MOV qword ptr [RCX + 0x10],R12
 */
const char* MOUNT_NODE_STRUCT_SIG = "[MOV [r64+off8], r64] [LEA r64, [r64+off8]] [LEA r64, [rip+off32]] [MOV [r64+off8], r64]";

/**
 * @brief Signature for StringBuffer offset within node registration.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 140156110  48 89 41 08                   MOV qword ptr [RCX + 0x8],RAX -> [StringBuffer]
 * 140156114  48 8D 05 95 28 F5 01          LEA RAX,[0x1420a89b0]
 * 14015611b  48 89 01                      MOV qword ptr [RCX],RAX
 */
const char* MOUNT_STR_BUFF_SIG = "[MOV [r64+off8], r64] [LEA r64, [rip+off32]] [MOV [r64], r64]";

/**
 * @brief Signature for Mount List Head anchor.
 * Links the list pointer load to subsequent stack operations for uniqueness.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 1401560c6  49 8B 9D 88 00 00 00          MOV RBX,qword ptr [R13 + 0x88] -> [Mount list head]
 * 1401560cd  88 44 24 54                   MOV byte ptr [RSP + 0x54],AL
 */
const char* MOUNT_LIST_HEAD_SIG = "[MOV r64, [r64+off32]] [MOV [r64+off8], r8]";

/**
 * @brief Signature for Physical Device Path offset.
 * Based on sequence: MOV reg, [reg+off] followed by LEA.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 140156336  49 8B 7F 10                   MOV RDI,qword ptr [R15 + 0x10] -> [Physical path]
 * 14015633a  48 8D 05 F7 21 F5 01          LEA RAX,[0x1420a8538]
 */
const char* PHYS_PATH_SIG = "[MOV r64, [r64+off8]] [LEA r64, [rip+off32]]";

}  // namespace

bool FileSystemDataFinder::TryFindOffsets(GameObjectFileSystemService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  // ── Phase 1: Manager Accessor ──
  {
    auto phase = log.MakePhase("Manager Accessor");

    uintptr_t pfnGetManager = PatternFinder::Find(UFS_GET_MANAGER_ACCESSOR_SIG);
    if (phase.Step(pfnGetManager, "GetManagerAccessor")) {
      // 1.1 Managers Count Pointer
      uintptr_t addrCmp = PatternFinder::Find(pfnGetManager, 64, "48 3b [05-3d]");
      uintptr_t countAddr = addrCmp ? PatternFinder::GetRipAddress(addrCmp, 3, 7) : 0;
      if (countAddr) owner.SetManagersCountAddr(countAddr);
      phase.Step(countAddr, "Managers Count", "PTR");

      // 1.2 Managers Array Pointer
      uintptr_t addrMov = PatternFinder::Find(pfnGetManager, 64, "[MOV r64, [rip+off32]]");
      uintptr_t arrayAddr = addrMov ? PatternFinder::GetRipAddress(addrMov, 3, 7) : 0;
      if (arrayAddr) owner.SetDevicesArrayAddr(arrayAddr);
      phase.Step(arrayAddr, "Managers Array", "PTR");
    }
  }

  // ── Phase 2: Mount Registration ──
  {
    auto phase = log.MakePhase("Mount Registration");

    /**
     * SEARCH STRATEGY:
     * Locate UFS_RegisterMount using its unique error string.
     * Verified for v1.60 at 0x140155fc0.
     */
    uintptr_t pfnRegisterMount = PatternFinder::FindFunctionByString(UFS_REGISTER_MOUNT_STR, true);
    if (phase.Step(pfnRegisterMount, "UFS_RegisterMount")) {
      // 2.1 Node offsets (DevicePtr, VirtualPath)
      uintptr_t addrNode = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_NODE_STRUCT_SIG);
      if (addrNode) {
        uint8_t modrm1 = *reinterpret_cast<uint8_t*>(addrNode + 2);
        int32_t deviceOff = (modrm1 >= 0x80) ? PatternFinder::ReadInt32(addrNode + 3) : PatternFinder::ReadInt8(addrNode + 3);

        uintptr_t addrLea = PatternFinder::Find(addrNode + 2, 32, "[LEA r64, [r64+off8]]");
        if (addrLea) {
          uint8_t modrm2 = *reinterpret_cast<uint8_t*>(addrLea + 2);
          int32_t vpathOff = (modrm2 >= 0x80) ? PatternFinder::ReadInt32(addrLea + 3) : PatternFinder::ReadInt8(addrLea + 3);

          bool devOk = phase.StepOffset(deviceOff, "Device offset", "NODE");
          bool vpOk = phase.StepOffset(vpathOff, "VirtualPath", "NODE");
          if (devOk && vpOk) {
            owner.SetNodeDeviceOffset(deviceOff);
            owner.SetNodeVPathOffset(vpathOff);
          }
        } else {
          phase.StepOffset(0, "VirtualPath", "NODE");
        }
      } else {
        phase.StepOffset(0, "Node structure", "NODE");
      }

      // StringBuffer offset
      uintptr_t addrStr = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_STR_BUFF_SIG);
      if (addrStr) {
        uint8_t modrm = *reinterpret_cast<uint8_t*>(addrStr + 2);
        int32_t strBuffOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrStr + 3) : (modrm >= 0x40) ? PatternFinder::ReadInt8(addrStr + 3) : 0;
        if (phase.StepOffset(strBuffOff, "StringBuffer", "STR")) {
          owner.SetStringBufferOffset(strBuffOff);
        }
      } else {
        phase.StepOffset(0, "StringBuffer", "STR");
      }

      // Mount List Head offset
      uintptr_t addrInc = PatternFinder::Find(pfnRegisterMount, 2048, MOUNT_LIST_HEAD_SIG);
      if (addrInc) {
        uint8_t modrm = *reinterpret_cast<uint8_t*>(addrInc + 2);
        int32_t listHeadOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrInc + 3) : PatternFinder::ReadInt8(addrInc + 3);
        if (phase.StepOffset(listHeadOff, "Mount list head", "CNT")) {
          owner.SetMountListHeadOffset(listHeadOff);
        }
      } else {
        phase.StepOffset(0, "Mount list head", "CNT");
      }

      // Physical Device Path offset
      uintptr_t addrPhys = PatternFinder::Find(pfnRegisterMount, 4096, PHYS_PATH_SIG);
      if (addrPhys) {
        uint8_t modrm = *reinterpret_cast<uint8_t*>(addrPhys + 2);
        int32_t physOff = (modrm >= 0x80) ? PatternFinder::ReadInt32(addrPhys + 3) : (modrm >= 0x40) ? PatternFinder::ReadInt8(addrPhys + 3) : 0;
        if (phase.StepOffset(physOff, "Physical path", "PATH")) {
          owner.SetPhysicalDevicePathOffset(physOff);
        }
      } else {
        phase.StepOffset(0, "Physical path", "PATH");
      }
    }
  }

  // --- Step 3: Find Active Profile Data ---
  // REMOVED: Core profile offsets (GamePtr, ProfileHandle) are now centrally managed by SessionDataFinder.
  // FileSystemDataFinder now relies on GameObjectSessionService for these root addresses.

  // --- Final Readiness Check ---
  m_isReady = (owner.GetDevicesArrayAddr() != 0 && owner.GetManagersCountAddr() != 0 && owner.GetMountListHeadOffset() != 0 && owner.GetNodeDeviceOffset() != 0 && owner.GetNodeVPathOffset() != 0 && owner.GetStringBufferOffset() != 0 &&
               owner.GetPhysicalDevicePathOffset() != 0);

  return log.Finish(m_isReady);
}

}  // namespace Data::GameData::Finders
SPF_NS_END
