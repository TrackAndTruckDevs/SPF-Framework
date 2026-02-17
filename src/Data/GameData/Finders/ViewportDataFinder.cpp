#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool ViewportDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Viewport data (Dynamic Search)...");

  bool all_found = true;

  // 1. Find Camera Params Object (Global pointer via RIP-relative MOV)
  /*
   * Anchor in version 1.58:
   * 83 78 10 0B                 | CMP dword ptr [RAX + 0x10], 0xb
   * 0F 84 ?? ?? ?? ??           | JZ ...
   * 48 8B 05 ?? ?? ?? ??        | MOV RAX, qword ptr [DAT_143368680] <- TARGET
   * 83 B8 68 08 00 00 00        | CMP dword ptr [RAX + 0x868], 0x0
   */
  {
    const char* params_sig = "83 78 10 0B 0F 84 ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 83 B8";
    uintptr_t anchor = Utils::PatternFinder::Find(params_sig);
    if (!anchor) {
      logger->Error("FAILED to find Anchor for Camera Params Object.");
      all_found = false;
    } else {
      // The MOV RAX, [RIP+...] instruction starts at anchor + 10 bytes
      uintptr_t mov_addr = anchor + 10;
      uintptr_t pointer_to_global_ptr = Utils::PatternFinder::GetRipAddress(mov_addr, 3, 7);
      
      if (pointer_to_global_ptr) {
        uintptr_t pCameraParamsObject = *reinterpret_cast<uintptr_t*>(pointer_to_global_ptr);
        owner.SetCameraParamsObjectPtr(pCameraParamsObject);
        logger->Debug("Found Camera Params Object: GlobalPtrAddr=0x{:X}, ObjectAddr=0x{:X}", 
                     pointer_to_global_ptr, pCameraParamsObject);
      } else {
        logger->Error("FAILED to resolve RIP address for Camera Params Object.");
        all_found = false;
      }
    }
  }

  // 2. Find Viewport Offsets
  /*
   * Anchor in version 1.58 (Aspect Ratio calculation block):
   * F3 0F 10 83 64 06 00 00     | MOVSS XMM0, [RBX + 6E4] -> Y2
   * F3 0F 5C 83 68 06 00 00     | SUBSS XMM0, [RBX + 6E8] -> Y1
   * F3 0F 10 8B 60 06 00 00     | MOVSS XMM1, [RBX + 6E0] -> X2
   * F3 0F 5C 8B DC 06 00 00     | SUBSS XMM1, [RBX + 6DC] -> X1
   * F3 0F 5E C8                 | DIVSS XMM1, XMM0
   */
  {
    const char* viewport_sig = "F3 0F 10 83 ?? ?? ?? ?? F3 0F 5C 83 ?? ?? ?? ?? F3 0F 10 8B ?? ?? ?? ?? F3 0F 5C 8B ?? ?? ?? ?? F3 0F 5E C8";
    uintptr_t anchor = Utils::PatternFinder::Find(viewport_sig);
    if (!anchor) {
      logger->Error("FAILED to find Anchor for Viewport offsets.");
      all_found = false;
    } else {
      int32_t y2 = Utils::PatternFinder::ReadInt32(anchor + 4);
      int32_t y1 = Utils::PatternFinder::ReadInt32(anchor + 12);
      int32_t x2 = Utils::PatternFinder::ReadInt32(anchor + 20);
      int32_t x1 = Utils::PatternFinder::ReadInt32(anchor + 28);

      if (Utils::PatternFinder::IsSaneOffset(x1)) owner.SetViewportX1Offset(x1);
      else { logger->Error("Viewport: X1 INVALID (0x{:X})", x1); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(x2)) owner.SetViewportX2Offset(x2);
      else { logger->Error("Viewport: X2 INVALID (0x{:X})", x2); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(y1)) owner.SetViewportY1Offset(y1);
      else { logger->Error("Viewport: Y1 INVALID (0x{:X})", y1); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(y2)) owner.SetViewportY2Offset(y2);
      else { logger->Error("Viewport: Y2 INVALID (0x{:X})", y2); all_found = false; }

      logger->Debug("Viewport processed. All sane: {}", all_found);
    }
  }

  if (all_found) {
    m_isReady = true;
    logger->Info("Successfully found all viewport data dynamically.");
  }
  return all_found;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
