#include "SPF/Data/GameData/Finders/WheelCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the universal function that updates camera positions for Wheel, Bumper, etc.
 * HOW-TO-FIND:
 * Search for usage of the hardcoded offsets (e.g., 0x470, 0x474). This will lead to this function.
 * The signature is for the function's prologue, provided by the user.
 */
const char* UPDATE_BUMPER_WHEEL_CAMERA_POS_SIG = "48 8B C4 48 89 58 08 48 89 78 10 55 48 8D 68 A1 48 81 EC D0 00 00 00 48 8B 99 E0 03 00";
}  // namespace

bool WheelCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Wheel Camera offsets...");

  uintptr_t pfnUpdateBumperWheelCam = Utils::PatternFinder::Find(UPDATE_BUMPER_WHEEL_CAMERA_POS_SIG);
  if (!pfnUpdateBumperWheelCam) {
    logger->Error("CRITICAL: Failed to find signature for UpdateBumperWheelCameraPosition function.");
    return false;
  }
  logger->Info("Found UpdateBumperWheelCameraPosition function at: {:#x}", pfnUpdateBumperWheelCam);

  // --- Optimized Search: Single loop for all offsets ---
  const unsigned char x_pattern[] = {0xF3, 0x0F, 0x10, 0xAF, 0x70, 0x04, 0x00, 0x00};  // MOVSS XMM5, [RDI+470h]
  const unsigned char y_pattern[] = {0xF3, 0x0F, 0x10, 0xB7, 0x74, 0x04, 0x00, 0x00};  // MOVSS XMM6, [RDI+474h]
  const unsigned char z_pattern[] = {0xF3, 0x0F, 0x10, 0xA7, 0x78, 0x04, 0x00, 0x00};  // MOVSS XMM4, [RDI+478h]

  bool found_x = false;
  bool found_y = false;
  bool found_z = false;

  for (int i = 0; i < 1024; ++i) {
    uintptr_t addr = pfnUpdateBumperWheelCam + i;
    if (!found_x && memcmp((void*)addr, x_pattern, sizeof(x_pattern)) == 0) {
      owner.SetWheelOffsetXOffset(*(int32_t*)(addr + 4));
      found_x = true;
      logger->Info("-> Found WheelOffsetXOffset: {:#x}", owner.GetWheelOffsetXOffset());
    }
    if (!found_y && memcmp((void*)addr, y_pattern, sizeof(y_pattern)) == 0) {
      owner.SetWheelOffsetYOffset(*(int32_t*)(addr + 4));
      found_y = true;
      logger->Info("-> Found WheelOffsetYOffset: {:#x}", owner.GetWheelOffsetYOffset());
    }
    if (!found_z && memcmp((void*)addr, z_pattern, sizeof(z_pattern)) == 0) {
      owner.SetWheelOffsetZOffset(*(int32_t*)(addr + 4));
      found_z = true;
      logger->Info("-> Found WheelOffsetZOffset: {:#x}", owner.GetWheelOffsetZOffset());
    }
    if (found_x && found_y && found_z) break;
  }

  bool all_found = found_x && found_y && found_z;
  m_isReady = all_found;

  if (all_found) {
    logger->Info("Successfully found all Wheel Camera offsets.");
  } else {
    logger->Error("Failed to find one or more Wheel Camera offsets.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
