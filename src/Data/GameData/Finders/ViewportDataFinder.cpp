#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool ViewportDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Starting search for Viewport memory structures...");

  bool all_found = true;

  /**
   * PRIMARY STRATEGY: Direct Viewport Parameters Access.
   * Targets the code block in the engine where camera parameters are accessed and
   * viewport boundaries (X1, X2, Y1, Y2) are used for calculations.
   *
   * Ghidra Analysis (v1.59.2):
   * 14044ae44 [0]: 48 8b 1d 2d b0 f7 02    MOV RBX, qword ptr [DAT_1433c5e78]   <-- Global Pointer
   * 14044ae4b [7]: 8b c6                 MOV EAX, ESI
   * 14044ae4d [9]: f3 0f 10 83 84 08 00 00  MOVSS XMM0, dword ptr [RBX + 0x884] <-- Y2 (Offset at +13)
   * 14044ae55 [17]: f3 0f 5c 83 88 08 00 00  SUBSS XMM0, dword ptr [RBX + 0x888] <-- Y1 (Offset at +21)
   * 14044ae5d [25]: f3 0f 10 8b 80 08 00 00  MOVSS XMM1, dword ptr [RBX + 0x880] <-- X2 (Offset at +29)
   * 14044ae65 [33]: f3 0f 5c 8b 7c 08 00 00  SUBSS XMM1, dword ptr [RBX + 0x87c] <-- X1 (Offset at +37)
   */
  const char* VIEWPORT_PARAMS_ACCESS_SIG = "48 8B 1D ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? ?? ?? ?? F3 0F";
  uintptr_t anchor = Utils::PatternFinder::Find(VIEWPORT_PARAMS_ACCESS_SIG);

  if (anchor) {
    logger->Debug("Viewport anchor found at: 0x{:X}", anchor);

    // 1. Resolve Global Pointer address via RIP-relative displacement (3 bytes opcode + 4 bytes displacement)
    uintptr_t pGamePtrAddr = Utils::PatternFinder::GetRipAddress(anchor, 3, 7);
    if (pGamePtrAddr) {
      uintptr_t pGameObject = *reinterpret_cast<uintptr_t*>(pGamePtrAddr);

      // 2. Set camera parameters base (In v1.59.2, RBX points to the object directly)
      owner.SetCameraParamsObjectPtr(pGameObject);
      logger->Debug("Resolved Viewport Base Address: 0x{:X}", pGameObject);

      // 3. Extract Coordinate Offsets from instruction bytes (32-bit offsets)
      int32_t off_y2 = Utils::PatternFinder::ReadInt32(anchor + 13);
      int32_t off_y1 = Utils::PatternFinder::ReadInt32(anchor + 21);
      int32_t off_x2 = Utils::PatternFinder::ReadInt32(anchor + 29);
      int32_t off_x1 = Utils::PatternFinder::ReadInt32(anchor + 37);

      auto validateAndSet = [&](int32_t offset, const char* label, void (GameDataCameraService::*setter)(intptr_t)) {
        if (Utils::PatternFinder::IsSaneOffset(offset)) {
          (owner.*setter)(static_cast<intptr_t>(offset));
          return true;
        }
        logger->Error("Viewport {} offset is INVALID (0x{:X})", label, offset);
        return false;
      };

      all_found &= validateAndSet(off_x1, "X1", &GameDataCameraService::SetViewportX1Offset);
      all_found &= validateAndSet(off_x2, "X2", &GameDataCameraService::SetViewportX2Offset);
      all_found &= validateAndSet(off_y1, "Y1", &GameDataCameraService::SetViewportY1Offset);
      all_found &= validateAndSet(off_y2, "Y2", &GameDataCameraService::SetViewportY2Offset);

      if (all_found) {
        logger->Info("Successfully initialized Viewport offsets.");
        logger->Debug("Offsets: X1=0x{:X}, X2:0x{:X}, Y1:0x{:X}, Y2:0x{:X}", off_x1, off_x2, off_y1, off_y2);
        m_isReady = true;
      }
    } else {
      logger->Error("FAILED to resolve RIP address for base pointer via anchor.");
      all_found = false;
    }
  } else {
    logger->Error("Critical: Viewport access signature NOT FOUND. Camera UI may be broken.");
    all_found = false;
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
