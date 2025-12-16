#include "SPF/Data/GameData/Finders/TVCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the ConfigureTVCamera function (FUN_14093d3b0)
 * Provided by user. This is the entry point for finding all TV camera offsets.
 */
const char* CONFIGURE_TV_CAMERA_SIG = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 48 8D 68 A1 48 81 EC D0 00 00 00 48 8B B9 E0 03 00";

/*
 * Signature for the instruction that reads the MaxDistance offset (0x470).
 * Found inside ConfigureTVCamera.
 */
const char* TV_MAX_DISTANCE_SIG = "F3 0F 58 F0 F3 0F 10 83 ? ? ? ? F3 0F 59 C0";

/*
 * Signature for the CALL to UpdateTVCamera within ConfigureTVCamera.
 */
const char* CALL_UPDATE_TV_CAMERA_SIG = "48 8B CB E8 ? ? ? ? 0F 10 4D D7";
}  // namespace

bool TVCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for TV Camera offsets...");

  // Step 1: Find the anchor function `ConfigureTVCamera`
  uintptr_t pfnConfigureTVCamera = Utils::PatternFinder::Find(CONFIGURE_TV_CAMERA_SIG);
  if (!pfnConfigureTVCamera) {
    logger->Error("CRITICAL: Failed to find signature for ConfigureTVCamera function. Cannot proceed.");
    return false;
  }
  logger->Info("Found ConfigureTVCamera function at: {:#x}", pfnConfigureTVCamera);

  bool all_found = true;

  // Step 2: Find MaxDistance offset
  uintptr_t max_dist_addr = Utils::PatternFinder::Find(pfnConfigureTVCamera, 2048, TV_MAX_DISTANCE_SIG);
  if (max_dist_addr) {
    int32_t offset = *(int32_t*)(max_dist_addr + 8);
    owner.SetTVMaxDistanceOffset(offset);
    logger->Info("-> Found TVMaxDistanceOffset: {:#x}", offset);
  } else {
    logger->Warn("-> FAILED to find signature for TVMaxDistanceOffset.");
    all_found = false;
  }

  // Step 3: Find the UpdateTVCamera function address from the CALL instruction
  uintptr_t pfnUpdateTVCamera = 0;
  uintptr_t call_addr = Utils::PatternFinder::Find(pfnConfigureTVCamera, 2048, CALL_UPDATE_TV_CAMERA_SIG);
  if (call_addr) {
    uintptr_t call_instr = call_addr + 3;
    uintptr_t rip = call_instr + 5;
    pfnUpdateTVCamera = rip + *(int32_t*)(call_instr + 1);
    logger->Info("-> Found UpdateTVCamera function at: {:#x}", pfnUpdateTVCamera);
  } else {
    logger->Warn("-> FAILED to find signature for CALL to UpdateTVCamera.");
    all_found = false;
  }

  // Step 4: Find remaining offsets inside UpdateTVCamera
  if (pfnUpdateTVCamera) {
    const unsigned char prefab_x_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x8D, 0x74, 0x04, 0x00, 0x00};  // MOVSS XMM1,[R13+474h]
    const unsigned char prefab_y_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x9D, 0x78, 0x04, 0x00, 0x00};  // MOVSS XMM3,[R13+478h]
    const unsigned char prefab_z_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x8D, 0x7C, 0x04, 0x00, 0x00};  // MOVSS XMM1,[R13+47Ch]
    const unsigned char road_x_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x8D, 0x80, 0x04, 0x00, 0x00};    // MOVSS XMM1,[R13+480h]
    const unsigned char road_y_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x9D, 0x84, 0x04, 0x00, 0x00};    // MOVSS XMM3,[R13+484h]
    const unsigned char road_z_pattern[] = {0xF3, 0x41, 0x0F, 0x10, 0x8D, 0x88, 0x04, 0x00, 0x00};    // MOVSS XMM1,[R13+488h]

    bool found_px = false, found_py = false, found_pz = false, found_rx = false, found_ry = false, found_rz = false;

    for (int i = 0; i < 4096; ++i) {
      uintptr_t addr = pfnUpdateTVCamera + i;
      if (!found_px && memcmp((void*)addr, prefab_x_pattern, sizeof(prefab_x_pattern)) == 0) {
        owner.SetTVPrefabUpliftXOffset(*(int32_t*)(addr + 5));
        found_px = true;
        logger->Info("-> Found TVPrefabUpliftXOffset: {:#x}", owner.GetTVPrefabUpliftXOffset());
        continue;
      }
      if (!found_py && memcmp((void*)addr, prefab_y_pattern, sizeof(prefab_y_pattern)) == 0) {
        owner.SetTVPrefabUpliftYOffset(*(int32_t*)(addr + 5));
        found_py = true;
        logger->Info("-> Found TVPrefabUpliftYOffset: {:#x}", owner.GetTVPrefabUpliftYOffset());
        continue;
      }
      if (!found_pz && memcmp((void*)addr, prefab_z_pattern, sizeof(prefab_z_pattern)) == 0) {
        owner.SetTVPrefabUpliftZOffset(*(int32_t*)(addr + 5));
        found_pz = true;
        logger->Info("-> Found TVPrefabUpliftZOffset: {:#x}", owner.GetTVPrefabUpliftZOffset());
        continue;
      }
      if (!found_rx && memcmp((void*)addr, road_x_pattern, sizeof(road_x_pattern)) == 0) {
        owner.SetTVRoadUpliftXOffset(*(int32_t*)(addr + 5));
        found_rx = true;
        logger->Info("-> Found TVRoadUpliftXOffset: {:#x}", owner.GetTVRoadUpliftXOffset());
        continue;
      }
      if (!found_ry && memcmp((void*)addr, road_y_pattern, sizeof(road_y_pattern)) == 0) {
        owner.SetTVRoadUpliftYOffset(*(int32_t*)(addr + 5));
        found_ry = true;
        logger->Info("-> Found TVRoadUpliftYOffset: {:#x}", owner.GetTVRoadUpliftYOffset());
        continue;
      }
      if (!found_rz && memcmp((void*)addr, road_z_pattern, sizeof(road_z_pattern)) == 0) {
        owner.SetTVRoadUpliftZOffset(*(int32_t*)(addr + 5));
        found_rz = true;
        logger->Info("-> Found TVRoadUpliftZOffset: {:#x}", owner.GetTVRoadUpliftZOffset());
        continue;
      }
      if (found_px && found_py && found_pz && found_rx && found_ry && found_rz) break;
    }
    if (!(found_px && found_py && found_pz && found_rx && found_ry && found_rz)) {
      logger->Warn("-> FAILED to find all offsets inside UpdateTVCamera.");
      all_found = false;
    }
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all TV Camera offsets.");
  } else {
    logger->Error("Failed to find one or more TV Camera offsets.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
