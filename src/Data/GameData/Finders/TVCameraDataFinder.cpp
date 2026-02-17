#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * ConfigureTVCamera (Ghidra: FUN_14099bd60)
 * Signature provided by user for the TV camera configuration function.
 */
const char* CONFIGURE_TV_CAMERA_SIG = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 48 8D 68 A1 48 81 EC D0 00 00 00 48 8B B9 E0 03 00";
}  // namespace

bool TVCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for TV Camera offsets (Dynamic Search)...");

  uintptr_t pfnConfigure = Utils::PatternFinder::Find(CONFIGURE_TV_CAMERA_SIG);
  if (!pfnConfigure) {
    logger->Error("CRITICAL: Failed to find signature for ConfigureTVCamera function.");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 4096;

  /*
   * ANCHOR #1: TV Camera Parameters Block
   * We find the base offset (max_distance = 0x478) and calculate others as they are sequential.
   * Expected offsets for v1.58: 478, 47C, 480, 484, 488, 48C, 490
   * 
   * Sequence: ADDSS XMM6, XMM0; MOVSS XMM0, [RBX + 0x478]; MULSS XMM0, XMM0
   */
  const char* p_tv_base = "F3 0F 58 F0 F3 0F 10 83 ?? ?? ?? ?? F3 0F 59 C0";
  uintptr_t addr = Utils::PatternFinder::Find(pfnConfigure, SEARCH_RANGE, p_tv_base);
  
  if (addr) {
    int32_t base = Utils::PatternFinder::ReadInt32(addr + 8); // 478

    if (Utils::PatternFinder::IsSaneOffset(base)) {
      // 1. Max Distance (0x478)
      owner.SetTVMaxDistanceOffset(base);
      
      // 2. Prefab Uplift X, Y, Z (0x47C, 0x480, 0x484)
      owner.SetTVPrefabUpliftXOffset(base + 4);
      owner.SetTVPrefabUpliftYOffset(base + 8);
      owner.SetTVPrefabUpliftZOffset(base + 12);

      // 3. Road Uplift X, Y, Z (0x488, 0x48C, 0x490)
      owner.SetTVRoadUpliftXOffset(base + 16);
      owner.SetTVRoadUpliftYOffset(base + 20);
      owner.SetTVRoadUpliftZOffset(base + 24);

      logger->Debug("TV Anchors: MaxDist=0x{:X}, PrefabX=0x{:X}, PrefabY=0x{:X}, PrefabZ=0x{:X}, RoadX=0x{:X}, RoadY=0x{:X}, RoadZ=0x{:X}", 
                   base, base + 4, base + 8, base + 12, base + 16, base + 20, base + 24);
    } else {
      logger->Error("TV Camera base offset INVALID (0x{:X})", base);
      all_found = false;
    }
  } else {
    logger->Error("FAILED to find TV offsets anchor block in ConfigureTVCamera.");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all 7 TV Camera offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more TV Camera offsets.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
