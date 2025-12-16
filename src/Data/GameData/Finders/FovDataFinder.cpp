#include "SPF/Data/GameData/Finders/FovDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool FovDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for shared FOV offsets...");

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  auto pfnUpdateCameraProjection = cameraHooks.GetUpdateCameraProjectionFunc();

  if (!pfnUpdateCameraProjection) {
    logger->Warn("Cannot find FOV offsets: UpdateCameraProjection function pointer is not ready. Will retry...");
    return false;
  }

  bool found_base = false;
  bool found_horiz = false;
  bool found_vert = false;

  // Base FOV: MOVSS XMM2, dword ptr [RCX + 0x20]
  const unsigned char pattern_base[] = {0xF3, 0x0F, 0x10, 0x51};

  // Horizontal FOV: JBE -> MOVSS XMM3, dword ptr [RBX + 0x38]
  const unsigned char pattern_horiz[] = {0xEB, 0x05, 0xF3, 0x0F, 0x10, 0x5B};

  // Vertical FOV: MOVSS dword ptr [RBX + 0x3c], XMM0
  const unsigned char pattern_vert[] = {0xF3, 0x0F, 0x11, 0x43};

  // Search within the first 256 bytes of the function for our patterns.
  for (int i = 0; i < 256; ++i) {
    uintptr_t addr = (uintptr_t)pfnUpdateCameraProjection + i;

    if (!found_base && memcmp((void*)addr, pattern_base, sizeof(pattern_base)) == 0) {
      owner.SetFovBaseOffset(*(int8_t*)(addr + sizeof(pattern_base)));
      found_base = true;
    } else if (!found_horiz && memcmp((void*)addr, pattern_horiz, sizeof(pattern_horiz)) == 0) {
      // The offset is the last byte of the pattern.
      owner.SetFovHorizFinalOffset(*(int8_t*)(addr + sizeof(pattern_horiz)));
      found_horiz = true;
    } else if (!found_vert && memcmp((void*)addr, pattern_vert, sizeof(pattern_vert)) == 0) {
      owner.SetFovVertFinalOffset(*(int8_t*)(addr + sizeof(pattern_vert)));
      found_vert = true;
    }

    if (found_base && found_horiz && found_vert) break;
  }

  m_isReady = found_base && found_horiz && found_vert;

  if (m_isReady) {
    logger->Info("Found shared FOV offsets: base={:#x}, H={:#x}, V={:#x}", owner.GetFovBaseOffset(), owner.GetFovHorizFinalOffset(), owner.GetFovVertFinalOffset());
  } else {
    logger->Warn("FAILED to find all shared FOV offsets. H_found={:d}, V_found={:d}", found_horiz, found_vert);
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
