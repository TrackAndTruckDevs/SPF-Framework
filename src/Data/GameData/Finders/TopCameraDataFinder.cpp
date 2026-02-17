#include "SPF/Data/GameData/Finders/TopCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * TopCamera_UpdateParams (Ghidra: part of FUN_14099b7f0)
 * Unique sequence in the movement/height calculation block.
 * Expected offsets for v1.58: 480 (MinH), 484 (MaxH), 488 (Speed), 478 (Fwd), 47C (Bwd)
 */
const char* TOP_CAMERA_PARAMS_ANCHOR_SIG = "48 8D 83 ?? ?? ?? ?? 0F 2F 00 F3 0F 10 ?? ?? F3 0F ?? ?? F3 0F 5C ?? ??";
}  // namespace

bool TopCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Top Camera offsets (Strict Local Search)...");

  uintptr_t addr = Utils::PatternFinder::Find(TOP_CAMERA_PARAMS_ANCHOR_SIG);
  if (!addr) {
    logger->Error("CRITICAL: Could not find unique anchor for Top Camera offsets.");
    return false;
  }

  bool all_found = true;
  uintptr_t search_base = addr - 1024;

  // 1. Min Height is part of the anchor LEA RAX, [RBX + offset]
  int32_t minH = Utils::PatternFinder::ReadInt32(addr + 3);

  // 2. Max Height: search for the assignment block near the end of the function
  // Pattern: MOVSS reg, [RBX + offset]; COMISS; JC; MOVAPS
  uintptr_t addr_maxH = Utils::PatternFinder::Find(addr, 512, "F3 0F 10 ?? ?? ?? ?? ?? 0F 2F ??");
  int32_t maxH = addr_maxH ? Utils::PatternFinder::ReadInt32(addr_maxH + 4) : 0;

  // 3. Speed: search backward using the user-provided robust signature
  // 1.58: ... F3 44 0F 59 93 [488]
  // 1.57: ... F3 44 0F 59 93 [480]
  uintptr_t addr_speed = Utils::PatternFinder::Find(search_base, 1024, "48 8b ?? ff 90 ?? ?? ?? ?? f3 0f 10 83 ?? ?? ?? ?? f3 44 0f 59 93 ?? ?? ?? ??");
  int32_t speed = addr_speed ? Utils::PatternFinder::ReadInt32(addr_speed + 22) : 0;

  // 4. Find X-Forward (0x478/0x470)
  uintptr_t addr_fwd = Utils::PatternFinder::Find(search_base, 1024, "F3 0F 10 8B ?? ?? ?? ?? 41 0F 2E CB 75 ??");
  
  // 5. Find X-Backward (0x47C/0x474)
  uintptr_t addr_bwd = Utils::PatternFinder::Find(search_base, 1024, "41 0F 2E D3 75 ?? F3 0F 10 83 ?? ?? ?? ?? 41 0F 2E C3");

  if (Utils::PatternFinder::IsSaneOffset(minH)) {
    owner.SetTopMinHeightOffset(minH);
    logger->Debug("Top: MinHeight=0x{:X}", minH);
  } else { logger->Error("Top: MinHeight INVALID (0x{:X})", minH); all_found = false; }

  if (Utils::PatternFinder::IsSaneOffset(maxH)) {
    owner.SetTopMaxHeightOffset(maxH);
    logger->Debug("Top: MaxHeight=0x{:X}", maxH);
  } else { logger->Error("Top: MaxHeight INVALID (0x{:X})", maxH); all_found = false; }

  if (addr_speed) {
    if (Utils::PatternFinder::IsSaneOffset(speed)) {
      owner.SetTopSpeedOffset(speed);
      logger->Debug("Top: Speed=0x{:X}", speed);
    } else { logger->Error("Top: Speed INVALID (0x{:X})", speed); all_found = false; }
  } else { logger->Error("FAILED to find Speed anchor"); all_found = false; }

  if (addr_fwd) {
    int32_t fwd = Utils::PatternFinder::ReadInt32(addr_fwd + 4);
    if (Utils::PatternFinder::IsSaneOffset(fwd)) {
      owner.SetTopXOffsetForwardOffset(fwd);
      logger->Debug("Top: X-Forward=0x{:X}", fwd);
    } else { logger->Error("Top: X-Forward INVALID (0x{:X})", fwd); all_found = false; }
  } else { logger->Error("FAILED to find X-Forward anchor"); all_found = false; }

  if (addr_bwd) {
    int32_t bwd = Utils::PatternFinder::ReadInt32(addr_bwd + 10);
    if (Utils::PatternFinder::IsSaneOffset(bwd)) {
      owner.SetTopXOffsetBackwardOffset(bwd);
      logger->Debug("Top: X-Backward=0x{:X}", bwd);
    } else { logger->Error("Top: X-Backward INVALID (0x{:X})", bwd); all_found = false; }
  } else { logger->Error("FAILED to find X-Backward anchor"); all_found = false; }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all 5 Top Camera offsets dynamically.");
  } else {
    logger->Warn("Failed to find one or more Top Camera offsets accurately.");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
