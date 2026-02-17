#include "SPF/Data/GameData/Finders/BumperCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * UpdateBumperWheelCameraPosition (Ghidra: FUN_14099a2e0)
 * Signature for the function that calculates position for bumper and wheel cameras.
 */
const char* UPDATE_BUMPER_WHEEL_CAMERA_POS_SIG = "48 8B ?? ?? 89 58 ?? ?? 89 78 10 55 48 8D ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 99 ?? ?? ?? ?? 48 8d ?? ?? 0f 28 05 ?? ?? ?? ?? 48 8b f9 0f 29 70 ?? 48 8b cb 0f 29 78 ?? 44 0f 29";
}  // namespace

bool BumperCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Bumper Camera offsets (Dynamic Search)...");

  uintptr_t pfnUpdatePos = Utils::PatternFinder::Find(UPDATE_BUMPER_WHEEL_CAMERA_POS_SIG);
  if (!pfnUpdatePos) {
    logger->Error("CRITICAL: Failed to find signature for UpdateBumperWheelCameraPosition function.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 2048;

  /*
   * ANCHOR #1: Bumper Camera Offsets (X, Y, Z)
   * Expected offsets for v1.58: 478, 47C, 480
   * 
   * Sequence: MOVUPS [RDI+50], XMM0; SUBPS XMM11, XMM7; MOVSS XMM5, [RDI+478]; MOVSS XMM6, [RDI+47C]; MOVSS XMM4, [RDI+480]
   */
  const char* p_bumper_offsets = "0F 11 47 50 0F 10 45 C7 44 0F 5C DF 0F 11 47 40 F3 0F 10 AF ?? ?? ?? ?? 0F 28 C7 F3 0F 10 B7 ?? ?? ?? ?? 44 0F 28 CD F3 0F 10 A7 ?? ?? ?? ??";
  uintptr_t addr = Utils::PatternFinder::Find(pfnUpdatePos, SEARCH_RANGE, p_bumper_offsets);
  
  if (addr) {
    int32_t offX = Utils::PatternFinder::ReadInt32(addr + 20); // 478
    int32_t offY = Utils::PatternFinder::ReadInt32(addr + 31); // 47C
    int32_t offZ = Utils::PatternFinder::ReadInt32(addr + 43); // 480

    if (Utils::PatternFinder::IsSaneOffset(offX)) {
      owner.SetBumperOffsetXOffset(offX);
      logger->Debug("Anchor #1: BumperX=0x{:X}", offX);
    } else { logger->Error("Anchor #1: BumperX INVALID (0x{:X})", offX); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(offY)) {
      owner.SetBumperOffsetYOffset(offY);
      logger->Debug("Anchor #1: BumperY=0x{:X}", offY);
    } else { logger->Error("Anchor #1: BumperY INVALID (0x{:X})", offY); all_found = false; }

    if (Utils::PatternFinder::IsSaneOffset(offZ)) {
      owner.SetBumperOffsetZOffset(offZ);
      logger->Debug("Anchor #1: BumperZ=0x{:X}", offZ);
    } else { logger->Error("Anchor #1: BumperZ INVALID (0x{:X})", offZ); all_found = false; }

  } else {
    logger->Error("FAILED to find Bumper offsets anchor block.");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all Bumper Camera offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more Bumper Camera offsets.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
