#include "SPF/Data/GameData/Finders/FovDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool FovDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for shared FOV offsets (Dynamic Pattern Search)...");

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  auto pfnUpdateCameraProjection = cameraHooks.GetUpdateCameraProjectionFunc();

  if (!pfnUpdateCameraProjection) {
    logger->Warn("Cannot find FOV offsets: UpdateCameraProjection function pointer is not ready. Will retry...");
    return false;
  }

  bool all_found = true;
  const size_t SEARCH_RANGE = 512;

  /*
   * ANCHOR #1: Base FOV (UpdateCameraProjection initialization)
   *
   * Expected offset for Game Version 1.58: 0x20
   *
   * Signature breakdown:
   * 0F B6 41 2C           | MOVZX EAX, byte ptr [RCX + 0x2c]
   * 48 8D 51 3C           | LEA RDX, [RCX + 0x3c]
   * F3 0F 5E CA           | DIVSS XMM1, XMM2
   * 48 8B D9              | MOV RBX, RCX
   * ?? ?? ?? ??           | MOV [RSP + offset], AL (88 44 24 20)
   * F3 0F 10 51 ??        | MOVSS XMM2, dword ptr [RCX + 0x20] -> Base FOV
   */
  const char* p_base_fov = "0F B6 41 2C 48 8D 51 3C F3 0F 5E CA 48 8B D9 ?? ?? ?? ?? F3 0F 10 51 ??";
  uintptr_t addr = Utils::PatternFinder::Find((uintptr_t)pfnUpdateCameraProjection, SEARCH_RANGE, p_base_fov);
  if (addr) {
    int8_t baseFovOffset = Utils::PatternFinder::ReadInt8(addr + 23);
    if (Utils::PatternFinder::IsSaneOffset(static_cast<int32_t>(baseFovOffset))) {
      owner.SetFovBaseOffset(baseFovOffset);
      logger->Debug("FOV Anchor #1 found: BaseFovOffset=0x{:X}", (uint8_t)baseFovOffset);
    } else {
      logger->Error("FOV Anchor #1: BaseFov INVALID (0x{:X})", (uint8_t)baseFovOffset);
      all_found = false;
    }
  } else {
    logger->Error("FAILED to find FOV Anchor #1 (Base FOV) in UpdateCameraProjection");
    all_found = false;
  }

  /*
   * ANCHOR #2: Horizontal FOV Final (Limit check block)
   *
   * Expected offset for Game Version 1.58: 0x38
   *
   * Signature breakdown:
   * F3 0F 10 5B ??        | MOVSS XMM3, dword ptr [RBX + 0x30]
   * 0F 57 C9              | XORPS XMM1, XMM1
   * 0F 2F D9              | COMISS XMM3, XMM1
   * 76 07                 | JBE ...
   * F3 0F 11 5B ??        | MOVSS dword ptr [RBX + 0x38], XMM3 -> Horizontal FOV Final
   */
  const char* p_horiz_fov = "F3 0F 10 5B ?? 0F 57 C9 0F 2F D9 76 07 F3 0F 11 5B ??";
  addr = Utils::PatternFinder::Find((uintptr_t)pfnUpdateCameraProjection, SEARCH_RANGE, p_horiz_fov);
  if (addr) {
    int8_t horizFovOffset = Utils::PatternFinder::ReadInt8(addr + 17);
    if (Utils::PatternFinder::IsSaneOffset(static_cast<int32_t>(horizFovOffset))) {
      owner.SetFovHorizFinalOffset(horizFovOffset);
      logger->Debug("FOV Anchor #2 found: HorizFovOffset=0x{:X}", (uint8_t)horizFovOffset);
    } else {
      logger->Error("FOV Anchor #2: HorizFov INVALID (0x{:X})", (uint8_t)horizFovOffset);
      all_found = false;
    }
  } else {
    logger->Error("FAILED to find FOV Anchor #2 (Horizontal FOV) in UpdateCameraProjection");
    all_found = false;
  }

  /*
   * ANCHOR #3: Vertical FOV Final (Limit check block)
   *
   * Expected offset for Game Version 1.58: 0x3C
   *
   * Signature breakdown:
   * F3 0F 10 43 ??        | MOVSS XMM0, dword ptr [RBX + 0x34]
   * 0F 2F C1              | COMISS XMM0, XMM1
   * 76 07                 | JBE ...
   * F3 0F 11 43 ??        | MOVSS dword ptr [RBX + 0x3C], XMM0 -> Vertical FOV Final
   */
  const char* p_vert_fov = "F3 0F 10 43 ?? 0F 2F C1 76 07 F3 0F 11 43 ??";
  addr = Utils::PatternFinder::Find((uintptr_t)pfnUpdateCameraProjection, SEARCH_RANGE, p_vert_fov);
  if (addr) {
    int8_t vertFovOffset = Utils::PatternFinder::ReadInt8(addr + 14);
    if (Utils::PatternFinder::IsSaneOffset(static_cast<int32_t>(vertFovOffset))) {
      owner.SetFovVertFinalOffset(vertFovOffset);
      logger->Debug("FOV Anchor #3 found: VertFovOffset=0x{:X}", (uint8_t)vertFovOffset);
    } else {
      logger->Error("FOV Anchor #3: VertFov INVALID (0x{:X})", (uint8_t)vertFovOffset);
      all_found = false;
    }
  } else {
    logger->Error("FAILED to find FOV Anchor #3 (Vertical FOV) in UpdateCameraProjection");
    all_found = false;
  }

  m_isReady = all_found;
  if (all_found) {
    logger->Info("Successfully found all shared FOV offsets dynamically.");
  } else {
    logger->Error("Failed to find one or more shared FOV offsets.");
  }

  return m_isReady;
}
}  // namespace Data::GameData::Finders
SPF_NS_END
