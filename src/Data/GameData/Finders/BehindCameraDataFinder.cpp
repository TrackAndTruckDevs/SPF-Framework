#include "SPF/Data/GameData/Finders/BehindCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
// Signature for the function that applies saved configuration values to the camera.
const char* APPLY_CONFIG_SIG = "48 8B C4 55 48 83 EC 50 48 83 3D ? ? ? ? ? 48 8B E9";

// Signature for the function that handles camera input (zoom, rotation).
const char* UPDATE_INPUT_SIG = "40 53 48 83 EC 40 48 8B 01 48 8B D9 0F 29 7C 24 30 F3 0F 10 79 14 44";

// Signature for the function that initializes the camera state upon activation.
const char* ACTIVATE_CAMERA_SIG = "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 70 0F B6 F2 48 8B D9";

// Signature for the function that finds a focus point for the camera, used to find laziness speed.
const char* FIND_FOCUS_POINT_SIG = "48 8B C4 48 89 58 08 57 48 81 EC E0 00 00 00";
}  // namespace

bool BehindCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Behind Camera offsets using all anchor functions...");

  uintptr_t pfnApplyConfig = Utils::PatternFinder::Find(APPLY_CONFIG_SIG);
  uintptr_t pfnUpdateInput = Utils::PatternFinder::Find(UPDATE_INPUT_SIG);
  uintptr_t pfnActivateCamera = Utils::PatternFinder::Find(ACTIVATE_CAMERA_SIG);
  uintptr_t pfnFindFocusPoint = Utils::PatternFinder::Find(FIND_FOCUS_POINT_SIG);

  if (!pfnApplyConfig || !pfnUpdateInput || !pfnActivateCamera || !pfnFindFocusPoint) {
    logger->Warn("CRITICAL: FAILED to find one or more anchor functions. Will retry...");
    if (!pfnApplyConfig) logger->Warn("-> ApplyCameraConfiguration not found.");
    if (!pfnUpdateInput) logger->Warn("-> UpdateCameraInput not found.");
    if (!pfnActivateCamera) logger->Warn("-> ActivateBehindCamera not found.");
    if (!pfnFindFocusPoint) logger->Warn("-> FindBestFocusPoint not found.");
    return false;
  }

  logger->Info(
      "Found anchors: ApplyConfig={:#x}, UpdateInput={:#x}, ActivateCamera={:#x}, FindFocusPoint={:#x}", pfnApplyConfig, pfnUpdateInput, pfnActivateCamera, pfnFindFocusPoint);
  bool all_found = true;

  // --- Find Config Offsets from ApplyCameraConfiguration ---
  {
    // Azimuth Laziness & Elevation Min (0x48C, 0x490)
    // MOV dword ptr [RBP + 0x48c], EAX -> MOV dword ptr [RBP + 0x490], EAX
    const unsigned char p1[] = {0x89, 0x85, 0x8C, 0x04, 0x00, 0x00, 0x89, 0x85, 0x90, 0x04, 0x00, 0x00};
    bool f1 = false;
    for (int i = 0; i < 1024; ++i) {
      if (memcmp((void*)(pfnApplyConfig + i), p1, sizeof(p1)) == 0) {
        owner.SetBehindAzimuthLazinessSpeedOffset(*(int32_t*)(pfnApplyConfig + i + 2));
        owner.SetBehindElevationMinOffset(*(int32_t*)(pfnApplyConfig + i + 8));
        logger->Info("-> Found AzimuthLaziness/ElevationMin: {:#x}, {:#x}", owner.GetBehindAzimuthLazinessSpeedOffset(), owner.GetBehindElevationMinOffset());
        f1 = true;
        break;
      }
    }
    if (!f1) {
      logger->Warn("FAILED to find offsets for Azimuth Laziness & Elevation Min. Will retry...");
      all_found = false;
    }

    // Elevation Max & Default (0x494, 0x498)
    // MOV dword ptr [RBP + 0x494], EAX -> MOV dword ptr [RBP + 0x498], EAX
    const unsigned char p2[] = {0x89, 0x85, 0x94, 0x04, 0x00, 0x00, 0x89, 0x85, 0x98, 0x04, 0x00, 0x00};
    bool f2 = false;
    for (int i = 0; i < 1024; ++i) {
      if (memcmp((void*)(pfnApplyConfig + i), p2, sizeof(p2)) == 0) {
        owner.SetBehindElevationMaxOffset(*(int32_t*)(pfnApplyConfig + i + 2));
        owner.SetBehindElevationDefaultOffset(*(int32_t*)(pfnApplyConfig + i + 8));
        logger->Info("-> Found ElevationMax/Default: {:#x}, {:#x}", owner.GetBehindElevationMaxOffset(), owner.GetBehindElevationDefaultOffset());
        f2 = true;
        break;
      }
    }
    if (!f2) {
      logger->Warn("FAILED to find offsets for Elevation Max & Default. Will retry...");
      all_found = false;
    }

    // Elevation Trailer Default & Height Limit (0x49C, 0x4A0)
    // MOV dword ptr [RBP + 0x49C], EAX -> MOV dword ptr [RBP + 0x4A0], EAX
    const unsigned char p3[] = {0x89, 0x85, 0x9C, 0x04, 0x00, 0x00, 0x89, 0x85, 0xA0, 0x04, 0x00, 0x00};
    bool f3 = false;
    for (int i = 0; i < 1024; ++i) {
      if (memcmp((void*)(pfnApplyConfig + i), p3, sizeof(p3)) == 0) {
        owner.SetBehindElevationTrailerDefaultOffset(*(int32_t*)(pfnApplyConfig + i + 2));
        owner.SetBehindHeightLimitOffset(*(int32_t*)(pfnApplyConfig + i + 8));
        logger->Info("-> Found ElevationTrailerDefault/HeightLimit: {:#x}, {:#x}", owner.GetBehindElevationTrailerDefaultOffset(), owner.GetBehindHeightLimitOffset());
        f3 = true;
        break;
      }
    }
    if (!f3) {
      logger->Warn("FAILED to find offsets for Elevation Trailer Default & Height Limit. Will retry...");
      all_found = false;
    }

    // Dynamic Offset Max & Speed Min (0x4B0, 0x4B4)
    // MOV dword ptr [RBP + 0x4B0], EAX -> MOV dword ptr [RBP + 0x4B4], EAX
    const unsigned char p4[] = {0x89, 0x85, 0xB0, 0x04, 0x00, 0x00, 0x89, 0x85, 0xB4, 0x04, 0x00, 0x00};
    bool f4 = false;
    for (int i = 0; i < 1024; ++i) {
      if (memcmp((void*)(pfnApplyConfig + i), p4, sizeof(p4)) == 0) {
        owner.SetBehindDynamicOffsetMaxOffset(*(int32_t*)(pfnApplyConfig + i + 2));
        owner.SetBehindDynamicOffsetSpeedMinOffset(*(int32_t*)(pfnApplyConfig + i + 8));
        logger->Info("-> Found DynamicOffsetMax/SpeedMin: {:#x}, {:#x}", owner.GetBehindDynamicOffsetMaxOffset(), owner.GetBehindDynamicOffsetSpeedMinOffset());
        f4 = true;
        break;
      }
    }
    if (!f4) {
      logger->Warn("FAILED to find offsets for Dynamic Offset Max & Speed Min. Will retry...");
      all_found = false;
    }

    // Dynamic Offset Speed Max & Laziness (0x4B8, 0x4BC)
    // MOV dword ptr [RBP + 0x4B8], EAX -> MOV dword ptr [RBP + 0x4BC], EAX
    const unsigned char p5[] = {0x89, 0x85, 0xB8, 0x04, 0x00, 0x00, 0x89, 0x85, 0xBC, 0x04, 0x00, 0x00};
    bool f5 = false;
    for (int i = 0; i < 1024; ++i) {
      if (memcmp((void*)(pfnApplyConfig + i), p5, sizeof(p5)) == 0) {
        owner.SetBehindDynamicOffsetSpeedMaxOffset(*(int32_t*)(pfnApplyConfig + i + 2));
        owner.SetBehindDynamicOffsetLazinessSpeedOffset(*(int32_t*)(pfnApplyConfig + i + 8));
        logger->Info("-> Found DynamicOffsetSpeedMax/Laziness: {:#x}, {:#x}", owner.GetBehindDynamicOffsetSpeedMaxOffset(), owner.GetBehindDynamicOffsetLazinessSpeedOffset());
        f5 = true;
        break;
      }
    }
    if (!f5) {
      logger->Warn("FAILED to find offsets for Dynamic Offset Speed Max & Laziness. Will retry...");
      all_found = false;
    }
  }

  // --- Find Pivot X, Y & Z from its unique signature ---
  {
    // This signature is very specific and targets the exact sequence of three MOVSS instructions
    const char* pivot_sig = "F3 45 0F 11 87 A4 04 00 00 F3 45 0F 11 8F A8 04 00 00 F3 45 0F 11 97 AC 04 00 00";
    if (uintptr_t pPivotMovs = Utils::PatternFinder::Find(pivot_sig)) {
      owner.SetBehindPivotXOffset(*(int32_t*)(pPivotMovs + 5));
      owner.SetBehindPivotYOffset(*(int32_t*)(pPivotMovs + 14));
      owner.SetBehindPivotZOffset(*(int32_t*)(pPivotMovs + 23));
      logger->Info("-> Found Pivot offsets: x={:#x}, y={:#x}, z={:#x}", owner.GetBehindPivotXOffset(), owner.GetBehindPivotYOffset(), owner.GetBehindPivotZOffset());
    } else {
      logger->Warn("FAILED to find offsets for Pivot X, Y & Z. Will retry...");
      all_found = false;
    }
  }

  // --- Find Distance Offsets from ActivateBehindCamera ---
  {
    // MOVSS XMM0, dword ptr [RBX + 0x474]
    const unsigned char p1[] = {0xF3, 0x0F, 0x10, 0x83, 0x74, 0x04, 0x00, 0x00};
    bool f1 = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnActivateCamera + i), p1, sizeof(p1)) == 0) {
        owner.SetBehindDistanceMaxOffset(*(int32_t*)(pfnActivateCamera + i + 4));
        logger->Info("-> Found DistanceMaxOffset: {:#x}", owner.GetBehindDistanceMaxOffset());
        f1 = true;
        break;
      }
    }
    if (!f1) {
      logger->Warn("FAILED to find offset for Distance Max. Will retry...");
      all_found = false;
    }

    // MOVSS XMM1, dword ptr [RBX + 0x47c]
    const unsigned char p2[] = {0xF3, 0x0F, 0x10, 0x8B, 0x7C, 0x04, 0x00, 0x00};
    bool f2 = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnActivateCamera + i), p2, sizeof(p2)) == 0) {
        owner.SetBehindDistanceDefaultOffset(*(int32_t*)(pfnActivateCamera + i + 4));
        logger->Info("-> Found DistanceDefaultOffset: {:#x}", owner.GetBehindDistanceDefaultOffset());
        f2 = true;
        break;
      }
    }
    if (!f2) {
      logger->Warn("FAILED to find offset for Distance Default. Will retry...");
      all_found = false;
    }

    // MOVSS XMM2, dword ptr [RBX + 0x470]
    const unsigned char p3[] = {0xF3, 0x0F, 0x10, 0x93, 0x70, 0x04, 0x00, 0x00};
    bool f3 = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnActivateCamera + i), p3, sizeof(p3)) == 0) {
        owner.SetBehindDistanceMinOffset(*(int32_t*)(pfnActivateCamera + i + 4));
        logger->Info("-> Found DistanceMinOffset: {:#x}", owner.GetBehindDistanceMinOffset());
        f3 = true;
        break;
      }
    }
    if (!f3) {
      logger->Warn("FAILED to find offset for Distance Min. Will retry...");
      all_found = false;
    }

    // MOVSS XMM0, dword ptr [RBX + 0x478]
    const unsigned char p4[] = {0xF3, 0x0F, 0x10, 0x83, 0x78, 0x04, 0x00, 0x00};
    bool f4 = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnActivateCamera + i), p4, sizeof(p4)) == 0) {
        owner.SetBehindDistanceTrailerMaxOffset(*(int32_t*)(pfnActivateCamera + i + 4));
        logger->Info("-> Found DistanceTrailerMaxOffset: {:#x}", owner.GetBehindDistanceTrailerMaxOffset());
        f4 = true;
        break;
      }
    }
    if (!f4) {
      logger->Warn("FAILED to find offset for Distance Trailer Max. Will retry...");
      all_found = false;
    }

    // MOVSS XMM1, dword ptr [RBX + 0x480]
    const unsigned char p5[] = {0xF3, 0x0F, 0x10, 0x8B, 0x80, 0x04, 0x00, 0x00};
    bool f5 = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnActivateCamera + i), p5, sizeof(p5)) == 0) {
        owner.SetBehindDistanceTrailerDefaultOffset(*(int32_t*)(pfnActivateCamera + i + 4));
        logger->Info("-> Found DistanceTrailerDefaultOffset: {:#x}", owner.GetBehindDistanceTrailerDefaultOffset());
        f5 = true;
        break;
      }
    }
    if (!f5) {
      logger->Warn("FAILED to find offset for Distance Trailer Default. Will retry...");
      all_found = false;
    }
  }

  // --- Find Live & Input Offsets from UpdateCameraInput ---
  {
    // MOVSS XMM7, dword ptr [RCX + 0x14]
    const unsigned char p1[] = {0xF3, 0x0F, 0x10, 0x79, 0x14};
    bool f1 = false;
    for (int i = 0; i < 64; ++i) {
      if (memcmp((void*)(pfnUpdateInput + i), p1, sizeof(p1)) == 0) {
        owner.SetBehindLivePitchOffset(*(int8_t*)(pfnUpdateInput + i + 4));
        logger->Info("-> Found LivePitchOffset: {:#x}", owner.GetBehindLivePitchOffset());
        f1 = true;
        break;
      }
    }
    if (!f1) {
      logger->Warn("FAILED to find offset for Live Pitch. Will retry...");
      all_found = false;
    }

    // ADDSS XMM0, dword ptr [RBX + 0x4c4]
    const unsigned char p2[] = {0xF3, 0x0F, 0x58, 0x83, 0xC4, 0x04, 0x00, 0x00};
    bool f2 = false;
    for (int i = 0; i < 256; ++i) {
      if (memcmp((void*)(pfnUpdateInput + i), p2, sizeof(p2)) == 0) {
        owner.SetBehindLiveYawOffset(*(int32_t*)(pfnUpdateInput + i + 4));
        logger->Info("-> Found LiveYawOffset: {:#x}", owner.GetBehindLiveYawOffset());
        f2 = true;
        break;
      }
    }
    if (!f2) {
      logger->Warn("FAILED to find offset for Live Yaw. Will retry...");
      all_found = false;
    }

    // MOVSS XMM0, dword ptr [RBX + 0x4c8]
    const unsigned char p3[] = {0xF3, 0x0F, 0x10, 0x83, 0xC8, 0x04, 0x00, 0x00};
    bool f3 = false;
    for (int i = 0; i < 256; ++i) {
      if (memcmp((void*)(pfnUpdateInput + i), p3, sizeof(p3)) == 0) {
        owner.SetBehindLiveZoomOffset(*(int32_t*)(pfnUpdateInput + i + 4));
        logger->Info("-> Found LiveZoomOffset: {:#x}", owner.GetBehindLiveZoomOffset());
        f3 = true;
        break;
      }
    }
    if (!f3) {
      logger->Warn("FAILED to find offset for Live Zoom. Will retry...");
      all_found = false;
    }

    // SUBSS XMM0, dword ptr [RBX + 0x484]
    const unsigned char p4[] = {0xF3, 0x0F, 0x5C, 0x83, 0x84, 0x04, 0x00, 0x00};
    bool f4 = false;
    for (int i = 0; i < 256; ++i) {
      if (memcmp((void*)(pfnUpdateInput + i), p4, sizeof(p4)) == 0) {
        owner.SetBehindDistanceChangeSpeedOffset(*(int32_t*)(pfnUpdateInput + i + 4));
        logger->Info("-> Found DistanceChangeSpeedOffset: {:#x}", owner.GetBehindDistanceChangeSpeedOffset());
        f4 = true;
        break;
      }
    }
    if (!f4) {
      logger->Warn("FAILED to find offset for Distance Change Speed. Will retry...");
      all_found = false;
    }
  }

  // --- Find Laziness Speed from FindBestFocusPoint ---
  {
    // MOVSS XMM9, dword ptr [RCX + 0x488]
    const unsigned char pattern[] = {0xF3, 0x44, 0x0F, 0x10, 0x89, 0x88, 0x04, 0x00, 0x00};
    bool found = false;
    for (int i = 0; i < 512; ++i) {
      if (memcmp((void*)(pfnFindFocusPoint + i), pattern, sizeof(pattern)) == 0) {
        owner.SetBehindDistanceLazinessSpeedOffset(*(int32_t*)(pfnFindFocusPoint + i + 5));
        logger->Info("-> Found DistanceLazinessSpeedOffset: {:#x}", owner.GetBehindDistanceLazinessSpeedOffset());
        found = true;
        break;
      }
    }
    if (!found) {
      logger->Warn("FAILED to find offset for Distance Laziness Speed. Will retry...");
      all_found = false;
    }
  }

  if (all_found) {
    m_isReady = true;
    logger->Info("Successfully found all dynamic offsets for Behind Camera.");
  } else {
    logger->Warn("Failed to find some dynamic offsets for Behind Camera. Will retry...");
  }

  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END