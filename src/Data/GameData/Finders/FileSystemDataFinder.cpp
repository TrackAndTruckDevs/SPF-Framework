#include "SPF/Data/GameData/Finders/FileSystemDataFinder.hpp"

#include "SPF/Data/GameData/GameObjectFileSystemService.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

namespace SPF::Data::GameData::Finders {
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
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 14015646e  4C 8D 0D 43 6B F6 01          LEA R9,[0x1420bcfb8] "[ufs] The table of UFS mo..."
 */
const char* UFS_REGISTER_MOUNT_STR = "[ufs] The table of UFS mounted devices is full";

/**
 * @brief Signature for Node Structure (DevicePtr and VirtualPath).
 * The trailing "[<instr>]?" marks an optional instruction: in v1.57 the
 * "LEA RAX,[rip]" is absent, in v1.60 it is present. The optional group lets
 * a single signature cover both versions.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 1401560fd  48 89 48 10                   MOV qword ptr [RAX + 0x10],RCX -> [Device offset]
 * 140156101  48 8D 48 18                   LEA RCX,[RAX + 0x18] -> [VirtualPath]
 * 140156105  48 8D 05 12 41 BA 01          LEA RAX,[0x141cfa21e]  <-- optional
 * 14015610c  4C 89 61 10                   MOV qword ptr [RCX + 0x10],R12
 */
const char* MOUNT_NODE_STRUCT_SIG = "[MOV [r64+off8], r64] [LEA r64, [r64+off8]] [LEA r64, [rip+off32]]? [MOV [r64+off8], r64]";

/**
 * @brief Signature for StringBuffer offset within node registration.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 140156110  48 89 41 08                   MOV qword ptr [RCX + 0x8],RAX -> [StringBuffer]
 * 140156114  48 8D 05 95 28 F5 01          LEA RAX,[0x1420a89b0] or for versions < 1.60  4C 89 61 10 MOV qword ptr [RCX + 0x10],R12
 * 14015611b  48 89 01                      MOV qword ptr [RCX],RAX
 */
const char* MOUNT_STR_BUFF_SIG = "[MOV [r64+off8], r64] {[LEA r64, [rip+off32]] | [MOV [r64+off8], r64]} [MOV [r64], r64]";
// const char* MOUNT_STR_BUFF_SIG = "48 89 41";

/**
 * @brief Signature for Mount List Head anchor.
 * Links the list pointer load to subsequent stack operations for uniqueness.
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
 * 1401560c6  49 8B 9D 88 00 00 00          MOV RBX,qword ptr [R13 + 0x88] -> [Mount list head] or for versions < 1.60 49 8B 5F 70 MOV RBX,qword ptr [R15 + 0x70]
 * 1401560cd  88 44 24 54                   MOV byte ptr [RSP + 0x54],AL
 */
const char* MOUNT_LIST_HEAD_SIG = "{[MOV r64, [r64+off32]] | [MOV r64, [r64+off8]]} [MOV [r64+off8], r8]";

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
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14014fc50[14014fc50]) ---/
      // * 14014fc57  48 3B 15 D2 48 49 02          CMP RDX,qword ptr [0x1425e4530]
      uintptr_t addrCmp = PatternFinder::Find(pfnGetManager, 64, "48 3b [05-3d]");
      uintptr_t countAddr = addrCmp ? PatternFinder::GetRipAddress(addrCmp, 3, 7) : 0;
      if (countAddr) owner.SetManagersCountAddr(countAddr);
      phase.Step(countAddr, "Managers Count", "PTR");

      // 1.2 Managers Array Pointer
      // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(FUN_14014fc50[14014fc50]) ---/
      // * 14014fc60  48 8B 05 C1 48 49 02          MOV RAX,qword ptr [0x1425e4528]
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
     * Locate UFS_RegisterMount using its unique error string.
     * Verified for v1.60 at 0x140155fc0.
     */
    uintptr_t pfnRegisterMount = PatternFinder::FindFunctionByString(UFS_REGISTER_MOUNT_STR, true);
    if (phase.Step(pfnRegisterMount, "UFS_RegisterMount")) {
      // 2.1 Node offsets (DevicePtr, VirtualPath)
      uintptr_t addrNode = PatternFinder::Find(pfnRegisterMount, 512, MOUNT_NODE_STRUCT_SIG);
      if (addrNode) {
        int nodeLen = 0;
        int32_t deviceOff = PatternFinder::ReadInstructionDisp(addrNode, nodeLen);
        // * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UFS_RegisterMount[140155fc0]) ---/
        // * 140156101  48 8D 48 18                   LEA RCX,[RAX + 0x18]
        uintptr_t addrLea = PatternFinder::Find(addrNode + nodeLen, 32, "[LEA r64, [r64+off8]]");
        if (addrLea) {
          int leaLen = 0;
          int32_t vpathOff = PatternFinder::ReadInstructionDisp(addrLea, leaLen);

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
      uintptr_t addrStr = PatternFinder::Find(pfnRegisterMount, 512, MOUNT_STR_BUFF_SIG);
      if (addrStr) {
        int strLen = 0;
        int32_t strBuffOff = PatternFinder::ReadInstructionDisp(addrStr, strLen);
        if (phase.StepOffset(strBuffOff, "StringBuffer", "STR")) {
          owner.SetStringBufferOffset(strBuffOff);
        }
      } else {
        phase.StepOffset(0, "StringBuffer", "STR");
      }

      // Mount List Head offset
      uintptr_t addrInc = PatternFinder::Find(pfnRegisterMount, 512, MOUNT_LIST_HEAD_SIG);
      if (addrInc) {
        int listLen = 0;
        int32_t listHeadOff = PatternFinder::ReadInstructionDisp(addrInc, listLen);
        if (phase.StepOffset(listHeadOff, "Mount list head", "CNT")) {
          owner.SetMountListHeadOffset(listHeadOff);
        }

        // Node order offset — anchored at the list-head instruction, forward search.
        // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(UFS_RegisterMount[14015ad50]) ---/
        // * 14015aebb  89 46 38                      MOV dword ptr [RSI + 0x38],EAX
        // * 14015aebe  0F B6 44 24 54                MOVZX EAX,byte ptr [RSP + 0x54]
        uintptr_t addrOrder = PatternFinder::Find(addrInc, 125, "[MOV [r64+off8], r32] [MOVZX r32, [r64+off8]]");
        if (addrOrder) {
          int orderLen = 0;
          int32_t orderOff = PatternFinder::ReadInstructionDisp(addrOrder, orderLen);
          if (phase.StepOffset(orderOff, "Node order", "NODE")) {
            owner.SetNodeOrderOffset(orderOff);
          }

          // Node next offset — anchored at the node-order instruction, forward search.
          // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(UFS_RegisterMount[14015ad50]) ---/
          // * 14015aed6  48 89 1E                      MOV qword ptr [RSI],RBX
          // * 14015aed9  48 8B 43 08                   MOV RAX,qword ptr [RBX + 0x8]
          uintptr_t addrNext = PatternFinder::Find(addrOrder, 64, "[MOV [r64], r64] [MOV r64, [r64+off8]]");
          if (addrNext) {
            int nextLen = 0;
            int32_t nextOff = PatternFinder::ReadInstructionDisp(addrNext, nextLen);
            owner.SetNodeNextOffset(nextOff);
            // Offset is legitimately 0 (no displacement operand) — gate on match address, not value.
            phase.Step(addrNext, "Node next", "NODE");

            // Per-pool mount count offset — anchored at the node-next instruction, forward search.
            // * /--- Ghidra:(amtrucks_1_61.exe) Fun:(UFS_RegisterMount[14015ad50]) ---/
            // * 14015aee4  49 FF 85 80 00 00 00          INC qword ptr [R13 + 0x80]
            uintptr_t addrPoolCount = PatternFinder::Find(addrNext, 32, "[INC qword ptr [r64+off32]]");
            if (addrPoolCount) {
              int poolCountLen = 0;
              int32_t poolCountOff = PatternFinder::ReadInstructionDisp(addrPoolCount, poolCountLen);
              if (phase.StepOffset(poolCountOff, "Pool mount count", "CNT")) {
                owner.SetPoolCountOffset(poolCountOff);
              }
            } else {
              phase.StepOffset(0, "Pool mount count", "CNT");
            }
          } else {
            phase.Step(0, "Node next", "NODE");
          }
        } else {
          phase.StepOffset(0, "Node order", "NODE");
        }
      } else {
        phase.StepOffset(0, "Mount list head", "CNT");
      }

      // Physical Device Path offset
      uintptr_t addrPhys = PatternFinder::Find(pfnRegisterMount, 1024, PHYS_PATH_SIG);
      if (addrPhys) {
        int physLen = 0;
        int32_t physOff = PatternFinder::ReadInstructionDisp(addrPhys, physLen);
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

  // ── Phase 3: VFS Mount API ──
  {
    auto phase = log.MakePhase("VFS Mount API");

    // 3.1 UFS_MountDevice
    /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(ufs_mount_home_dir[1400f3460]) ---/
     * 1400f34ff  48 8D 0D 82 D7 0D 02          LEA RCX,[0x1421d0c88]
     */
    uintptr_t addrMountLea = PatternFinder::FindFunctionByString("[ufs] Home directory mount failed!", false);
    if (phase.Step(addrMountLea, "ufs mount string LEA", "REF")) {
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(ufs_mount_home_dir[1400f3460]) ---/
       * 1400f34f2  E8 39 F5 FF FF                CALL 0x1400f2a30
       */
      uintptr_t addrCall = PatternFinder::FindBackward(addrMountLea, 64, "[CALL rel32]");
      if (phase.Step(addrCall, "ufs mount CALL", "RT")) {
        uintptr_t pfnMountDevice = PatternFinder::GetRipAddress(addrCall, 1, 5);
        if (phase.Step(pfnMountDevice, "UFS_MountDevice", "ADR")) {
          owner.SetUfsMountDeviceAddr(pfnMountDevice);
        }
      }
    }

    // 3.2 UFS_UnmountDevice
    /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(UFS_UnmountDevice[14015b220]) ---/
     * 14015b220  48 89 5C 24 10                MOV qword ptr [RSP+0x10],RBX
     */
    uintptr_t pfnUnmountDevice = PatternFinder::FindFunctionByString("[ufs] Error unmounting device: device not found!", true);
    if (phase.Step(pfnUnmountDevice, "UFS_UnmountDevice", "ADR")) {
      owner.SetUfsUnmountDeviceAddr(pfnUnmountDevice);
    }

    // 3.3 Pool array & pool count globals
    /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14070ef90[14070ef90]) ---/
     * 14070f289  4C 8D 05 DC CB 6B FA          LEA R8,[0x141cabe6c]
     */
    uintptr_t addrCompany = PatternFinder::FindFunctionByString("/def/company/%s/%s", false);
    if (phase.Step(addrCompany, "company string LEA", "REF")) {
      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14070ef90[14070ef90]) ---/
       * 14070f2ae  48 8B 05 73 CF 02 02          MOV RAX,qword ptr [0x14273c228]
       */
      uintptr_t addrMov = PatternFinder::Find(addrCompany, 64, "[MOV r64, [rip+off32]]");
      if (phase.Step(addrMov, "pool array MOV", "RT")) {
        uintptr_t poolArrayAddr = PatternFinder::GetRipAddress(addrMov, 3, 7);
        if (phase.Step(poolArrayAddr, "pool array global", "ADR")) {
          owner.SetPoolArrayAddr(poolArrayAddr);
        }
      }

      /** /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14070ef90[14070ef90]) ---/
       * 14070f2a0  48 83 3D 88 CF 02 02 04       CMP qword ptr [0x14273c230],0x4
       */
      uintptr_t addrCmp = PatternFinder::Find(addrCompany, 32, "48 83 3D");
      if (phase.Step(addrCmp, "pool count CMP", "RT")) {
        uintptr_t poolCountAddr = PatternFinder::GetRipAddress(addrCmp, 3, 8);
        if (phase.Step(poolCountAddr, "pool count global", "ADR")) {
          owner.SetPoolCountAddr(poolCountAddr);
        }
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = (owner.GetDevicesArrayAddr() != 0 && owner.GetManagersCountAddr() != 0 && owner.GetMountListHeadOffset() != 0 && owner.GetNodeDeviceOffset() != 0 && owner.GetNodeVPathOffset() != 0 && owner.GetStringBufferOffset() != 0 &&
                owner.GetPhysicalDevicePathOffset() != 0 && owner.GetUfsMountDeviceAddr() != 0 && owner.GetUfsUnmountDeviceAddr() != 0 && owner.GetPoolArrayAddr() != 0 && owner.GetPoolCountAddr() != 0 &&
                owner.GetNodeOrderOffset() != 0 && owner.GetPoolCountOffset() != 0);

  return log.Finish(m_isReady);
}

}  // namespace SPF::Data::GameData::Finders
