#include "SPF/Data/GameData/Finders/WindowCameraDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
namespace {
/*
 * Signature for the UpdateInteriorCamera function.
 * This function is responsible for updating head position, mouse limits, etc.
 * for interior-like cameras (window, cabin).
 */
const char* UPDATE_INTERIOR_CAMERA_SIG = "48 83 EC 38 F3 0F 10 2D ?? ?? ?? ?? 4C 8B C2 0F 29";

/*
 * Signature for the UpdateInteriorCameraOrientation function.
 * This function is responsible for updating the live yaw/pitch of the camera.
 */
const char* UPDATE_INTERIOR_CAMERA_ORIENTATION_SIG = "40 53 48 81 EC 80 00 00 00 80 B9 3C 01 00 00 00 48 8B D9";
}  // namespace
bool WindowCameraDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Window Camera offsets...");

  uintptr_t pfnUpdateInteriorCamera = Utils::PatternFinder::Find(UPDATE_INTERIOR_CAMERA_SIG);
  if (!pfnUpdateInteriorCamera) {
    logger->Error("CRITICAL: Failed to find signature for UpdateInteriorCamera function. Cannot proceed.");
    return false;
  }
  logger->Info("Found UpdateInteriorCamera function at: {:#x}", pfnUpdateInteriorCamera);

  bool all_found = true;

  // --- Find offsets within UpdateInteriorCamera ---
  {
    const unsigned char p_head_x[] = {0xF2, 0x0F, 0x10, 0x81, 0x88, 0x04, 0x00, 0x00};      // MOVSD xmm0, [rcx+488h]
    const unsigned char p_head_z[] = {0x8B, 0x81, 0x90, 0x04, 0x00, 0x00};                  // MOV eax, [rcx+490h]
    const unsigned char p_limit_l[] = {0xF3, 0x0F, 0x10, 0x89, 0x7C, 0x05, 0x00, 0x00};     // MOVSS xmm1, [rcx+57Ch]
    const unsigned char p_limit_r[] = {0xF3, 0x0F, 0x10, 0x81, 0x80, 0x05, 0x00, 0x00};     // MOVSS xmm0, [rcx+580h]
    const unsigned char p_default_lr[] = {0xF3, 0x0F, 0x10, 0x81, 0x84, 0x05, 0x00, 0x00};  // MOVSS xmm0, [rcx+584h]
    const unsigned char p_limit_u[] = {0xF3, 0x0F, 0x10, 0x91, 0x8C, 0x05, 0x00, 0x00};     // MOVSS xmm2, [rcx+58Ch]
    const unsigned char p_limit_d[] = {0xF3, 0x0F, 0x10, 0xA1, 0x90, 0x05, 0x00, 0x00};     // MOVSS xmm4, [rcx+590h]
    const unsigned char p_default_ud[] = {0x8B, 0x81, 0x94, 0x05, 0x00, 0x00};              // MOV eax, [rcx+594h]

    bool f[8] = {false};

    for (int i = 0; i < 2048; ++i) {
      uintptr_t addr = pfnUpdateInteriorCamera + i;
      if (!f[0] && memcmp((void*)addr, p_head_x, sizeof(p_head_x)) == 0) {
        owner.SetWindowHeadOffsetXOffset(*(int32_t*)(addr + 4));
        owner.SetWindowHeadOffsetYOffset(*(int32_t*)(addr + 4) + 4);
        f[0] = true;
      }
      if (!f[1] && memcmp((void*)addr, p_head_z, sizeof(p_head_z)) == 0) {
        owner.SetWindowHeadOffsetZOffset(*(int32_t*)(addr + 2));
        f[1] = true;
      }
      if (!f[2] && memcmp((void*)addr, p_limit_l, sizeof(p_limit_l)) == 0) {
        owner.SetWindowMouseLeftLimitOffset(*(int32_t*)(addr + 4));
        f[2] = true;
      }
      if (!f[3] && memcmp((void*)addr, p_limit_r, sizeof(p_limit_r)) == 0) {
        owner.SetWindowMouseRightLimitOffset(*(int32_t*)(addr + 4));
        f[3] = true;
      }
      if (!f[4] && memcmp((void*)addr, p_default_lr, sizeof(p_default_lr)) == 0) {
        owner.SetWindowMouseLRDefaultOffset(*(int32_t*)(addr + 4));
        f[4] = true;
      }
      if (!f[5] && memcmp((void*)addr, p_limit_u, sizeof(p_limit_u)) == 0) {
        owner.SetWindowMouseUpLimitOffset(*(int32_t*)(addr + 4));
        f[5] = true;
      }
      if (!f[6] && memcmp((void*)addr, p_limit_d, sizeof(p_limit_d)) == 0) {
        owner.SetWindowMouseDownLimitOffset(*(int32_t*)(addr + 4));
        f[6] = true;
      }
      if (!f[7] && memcmp((void*)addr, p_default_ud, sizeof(p_default_ud)) == 0) {
        owner.SetWindowMouseUDDefaultOffset(*(int32_t*)(addr + 2));
        f[7] = true;
      }
      if (f[0] && f[1] && f[2] && f[3] && f[4] && f[5] && f[6] && f[7]) break;
    }
    if (!(f[0] && f[1] && f[2] && f[3] && f[4] && f[5] && f[6] && f[7])) {
      logger->Warn("-> FAILED to find one or more offsets in UpdateInteriorCamera");
      all_found = false;
    }
  }

  // --- Find offsets within UpdateInteriorCameraOrientation ---
  uintptr_t pfnUpdateInteriorCameraOrientation = Utils::PatternFinder::Find(UPDATE_INTERIOR_CAMERA_ORIENTATION_SIG);
  if (!pfnUpdateInteriorCameraOrientation) {
    logger->Error("CRITICAL: Failed to find signature for UpdateInteriorCameraOrientation function.");
    all_found = false;
  } else {
    const unsigned char yaw_pattern[] = {0xF3, 0x0F, 0x11, 0x83, 0x70, 0x05, 0x00, 0x00};    // MOVSS [RBX+570h], XMM0
    const unsigned char pitch_pattern[] = {0xF3, 0x0F, 0x11, 0x8B, 0x74, 0x05, 0x00, 0x00};  // MOVSS [RBX+574h], XMM1
    bool found_yaw = false, found_pitch = false;
    for (int i = 0; i < 2048; ++i) {
      uintptr_t addr = pfnUpdateInteriorCameraOrientation + i;
      if (!found_yaw && memcmp((void*)addr, yaw_pattern, sizeof(yaw_pattern)) == 0) {
        owner.SetWindowLiveYawOffset(*(int32_t*)(addr + 4));
        found_yaw = true;
      }
      if (!found_pitch && memcmp((void*)addr, pitch_pattern, sizeof(pitch_pattern)) == 0) {
        owner.SetWindowLivePitchOffset(*(int32_t*)(addr + 4));
        found_pitch = true;
      }
      if (found_yaw && found_pitch) break;
    }
    if (!found_yaw || !found_pitch) {
      logger->Warn("-> FAILED to find LiveYaw/LivePitch offsets.");
      all_found = false;
    }
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all Window Camera offsets.");
  } else {
    logger->Error("Failed to find one or more Window Camera offsets.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
