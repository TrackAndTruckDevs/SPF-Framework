#include "SPF/Data/GameData/Finders/CoreCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/Vec3.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;
namespace {
// Signature for the start of the InitializeCamera function, used to find the standard manager pointer.
// const char* INITIALIZE_CAMERA_SIG = "48 81 EC 90 00 00 00 48 8B 1D ? ? ? ? 8B FA 48 8B F1";
// Signature for the world coordinates pointer.
const char* CAMERA_WORLD_COORDINATES_PTR_SIG = "F2 0F 11 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 83 BF";
}  // namespace

bool CoreCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  bool success = true;

  // 1. Find StandardManagerPtrAddr
  {
    auto& cameraHooks = Hooks::CameraHooks::GetInstance();
    uintptr_t initCamAddr = reinterpret_cast<uintptr_t>(cameraHooks.GetInitializeCameraFunc());

    if (!initCamAddr) {
      logger->Error("Cannot find StandardManagerPtrAddr: InitializeCamera function pointer is null.");
      success = false;
    } else {
      // The full signature from CameraHooks finds the start of the function.
      // The MOV RBX, [rip+offset] instruction is at a fixed offset from there.
      const int mov_rbx_offset = 18;  // 5+5+1+7 bytes from the function start
      uintptr_t mov_instruction_address = initCamAddr + mov_rbx_offset;

      // Perform the RIP-relative calculation to find the pointer's address
      int32_t rip_offset = *(int32_t*)(mov_instruction_address + 3);
      uintptr_t rip_base = mov_instruction_address + 7;
      uintptr_t pStandardManagerPtrAddr = rip_base + rip_offset;

      if (pStandardManagerPtrAddr) {
        owner.SetStandardManagerPtrAddr(pStandardManagerPtrAddr);
        logger->Info("Found 'standardManagerPtr' address: {:#x}", pStandardManagerPtrAddr);
      } else {
        logger->Error("Failed to calculate 'standardManagerPtr' address.");
        success = false;
      }
    }
  }

  // 2. Find ActiveCameraIdOffset
  {
    auto& cameraHooks = Hooks::CameraHooks::GetInstance();
    uintptr_t initCamAddr = reinterpret_cast<uintptr_t>(cameraHooks.GetInitializeCameraFunc());

    if (!initCamAddr) {
      logger->Error("Cannot find ActiveCameraIdOffset: InitializeCamera function pointer is null.");
      success = false;
    } else {
      // Scan for `CMP dword ptr [RBX + ??], 0x0E` (opcode 83 7B ?? 0E)
      const unsigned char pattern[] = {0x83, 0x7B};  // CMP [RBX+disp8], imm8
      const uint8_t immediate_value = 0x0E;
      bool found = false;
      for (int i = 0; i < 200; ++i) {
        uintptr_t instr_ptr = initCamAddr + i;
        if (memcmp((void*)instr_ptr, pattern, sizeof(pattern)) == 0 && *(uint8_t*)(instr_ptr + 3) == immediate_value) {
          intptr_t activeCameraIdOffset = *(int8_t*)(instr_ptr + 2);
          owner.SetActiveCameraIdOffset(activeCameraIdOffset);
          logger->Info("Dynamically found camera ID offset via CMP instruction: {:#x}", activeCameraIdOffset);
          found = true;
          break;
        }
      }
      if (!found) {
        logger->Error("Failed to find camera ID offset pattern (CMP [RBX+??]).");
        success = false;
      }
    }
  }

  // 3. Find World Coordinates Pointer
  {
    uintptr_t movsd_instruction_address = Utils::PatternFinder::Find(CAMERA_WORLD_COORDINATES_PTR_SIG);
    if (movsd_instruction_address) {
      uintptr_t rip = movsd_instruction_address + 8;                // Address of the next instruction
      int32_t offset = *(int32_t*)(movsd_instruction_address + 4);  // Read 4-byte offset
      owner.SetCameraWorldCoordinatesPtr(reinterpret_cast<uintptr_t*>(rip + offset));
      logger->Info("Found Camera World Coordinates pointer at: {:#x}", (uintptr_t)owner.GetCameraWorldCoordinatesPtr());
    } else {
      logger->Error("FAILED to find signature for World Coordinates pointer.");
      success = false;
    }
  }

  if (success) {
    m_isReady = true;
    owner.SetCoreOffsetsFound(true);
    logger->Info("Successfully found all core camera data.");
  }
  return success;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
