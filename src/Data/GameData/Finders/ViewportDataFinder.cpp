#include "SPF/Data/GameData/Finders/ViewportDataFinder.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
bool ViewportDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(GetName());
  logger->Info("Searching for Viewport data (Dynamic Search for v1.59+)...");

  bool all_found = true;

  /*
   * ANCHOR: Viewport Parameters Access (Version 1.59+)
   * 
   * This signature targets a block in FUN_14044d3e0 where the game loads 
   * a global game object pointer, applies a negative adjustment (e.g., -0x450), 
   * and then reads viewport coordinate offsets (X1, X2, Y1, Y2).
   *
   * 1.59 Ghidra Example:
   * 14044d518: 48 8b 1d 91 39 f9 02    MOV RBX, qword ptr [DAT_1433e0eb0]
   * 14044d51f: 48 85 db                 TEST RBX, RBX
   * 14044d522: 74 09                    JZ ...
   * 14044d524: 48 81 c3 b0 fb ff ff    ADD RBX, -0x450  <-- Base Adjustment
   * 14044d52b: eb 03                    JMP ...
   * 14044d52d: 49 8b de                 MOV RBX, R14
   * 14044d530: f3 0f 10 83 84 08 00 00  MOVSS XMM0, dword ptr [RBX + 0x884] -> Y2
   * 14044d538: f3 0f 5c 83 88 08 00 00  SUBSS XMM0, dword ptr [RBX + 0x888] -> Y1
   * 14044d540: f3 0f 10 8b 80 08 00 00  MOVSS XMM1, dword ptr [RBX + 0x880] -> X2
   * 14044d548: f3 0f 5c 8b 7c 08 00 00  SUBSS XMM1, dword ptr [RBX + 0x87c] -> X1
   */
  const char* VIEWPORT_159_SIG = "48 8B 1D ?? ?? ?? ?? 48 ?? ?? ?? ?? 48 81 ?? ?? ?? ?? ?? EB ?? 49 ?? ?? F3";
  uintptr_t anchor = Utils::PatternFinder::Find(VIEWPORT_159_SIG);

  if (anchor) {
    logger->Debug("Viewport Anchor (1.59+) found at: 0x{:X}", anchor);

    // 1. Resolve the Global Game Pointer address
    uintptr_t pGamePtrAddr = Utils::PatternFinder::GetRipAddress(anchor, 3, 7);
    if (pGamePtrAddr) {
      uintptr_t pGameObject = *reinterpret_cast<uintptr_t*>(pGamePtrAddr);
      
      // 2. Extract the dynamic base adjustment (e.g., -0x450)
      // The instruction is ADD RBX, imm32 (48 81 C3 XX XX XX XX) starting at anchor + 12.
      // The opcode is 3 bytes (48 81 C3), and the 4-byte immediate value follows.
      int32_t adjustment = Utils::PatternFinder::ReadInt32(anchor + 15);

      // Final Base for viewport parameters
      uintptr_t pCameraParamsObject = pGameObject ? (pGameObject + (intptr_t)adjustment) : 0;
      owner.SetCameraParamsObjectPtr(pCameraParamsObject);
      
      logger->Debug("Viewport Base: GamePtr=0x{:X}, Adj=0x{:X} ({}), Final=0x{:X}", 
                   pGamePtrAddr, (uint32_t)adjustment, adjustment, pCameraParamsObject);

      // 3. Extract Viewport Coordinate Offsets
      // Instructions start 24 bytes after the anchor (7 + 5 + 7 + 2 + 3 = 24).
      // Each instruction is 8 bytes long, with the offset being an int32 at byte 4.
      int32_t off_y2 = Utils::PatternFinder::ReadInt32(anchor + 24 + 4);
      int32_t off_y1 = Utils::PatternFinder::ReadInt32(anchor + 32 + 4);
      int32_t off_x2 = Utils::PatternFinder::ReadInt32(anchor + 40 + 4);
      int32_t off_x1 = Utils::PatternFinder::ReadInt32(anchor + 48 + 4);

      if (Utils::PatternFinder::IsSaneOffset(off_x1)) owner.SetViewportX1Offset(off_x1);
      else { logger->Error("Viewport: X1 INVALID (0x{:X})", off_x1); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off_x2)) owner.SetViewportX2Offset(off_x2);
      else { logger->Error("Viewport: X2 INVALID (0x{:X})", off_x2); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off_y1)) owner.SetViewportY1Offset(off_y1);
      else { logger->Error("Viewport: Y1 INVALID (0x{:X})", off_y1); all_found = false; }

      if (Utils::PatternFinder::IsSaneOffset(off_y2)) owner.SetViewportY2Offset(off_y2);
      else { logger->Error("Viewport: Y2 INVALID (0x{:X})", off_y2); all_found = false; }

      if (all_found) {
        logger->Info("Successfully extracted all dynamic viewport offsets for v1.59.");
        logger->Debug("Final Viewport Offsets: X1:0x{:X}, X2:0x{:X}, Y1:0x{:X}, Y2:0x{:X}", 
                     off_x1, off_x2, off_y1, off_y2);
        m_isReady = true;
      }
    } else {
      logger->Error("FAILED to resolve RIP address for GamePtr in 1.59 search.");
      all_found = false;
    }
  } else {
    logger->Warn("1.59 Viewport signature not found. Attempting legacy 1.58 search...");
    
    // --- LEGACY SEARCH (v1.58) ---
    const char* VIEWPORT_ANCHOR_158 = "48 ?? ?? ?? ?? ?? ?? ?? F3 ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F ?? ?? ?? ?? ?? ?? ?? ?? C6 81";
    uintptr_t anchor158 = Utils::PatternFinder::Find(VIEWPORT_ANCHOR_158);
    if (anchor158) {
      uintptr_t mov_addr = Utils::PatternFinder::Find(anchor158, 200, "48 8B 05");
      if (mov_addr) {
          uintptr_t pointer_to_global_ptr = Utils::PatternFinder::GetRipAddress(mov_addr, 3, 7);
          if (pointer_to_global_ptr) {
            uintptr_t pCameraParamsObject = *reinterpret_cast<uintptr_t*>(pointer_to_global_ptr);
            owner.SetCameraParamsObjectPtr(pCameraParamsObject);
          }
      }
      
      const char* viewport_sig_158 = "F3 0F 10 83 ?? ?? ?? ?? F3 0F 5C 83 ?? ?? ?? ?? F3 0F 10 8B ?? ?? ?? ?? F3 0F 5C 8B ? ? ? ? 8b";
      uintptr_t off_anchor = Utils::PatternFinder::Find(viewport_sig_158);
      if (off_anchor) {
        owner.SetViewportY2Offset(Utils::PatternFinder::ReadInt32(off_anchor + 4));
        owner.SetViewportY1Offset(Utils::PatternFinder::ReadInt32(off_anchor + 12));
        owner.SetViewportX2Offset(Utils::PatternFinder::ReadInt32(off_anchor + 20));
        owner.SetViewportX1Offset(Utils::PatternFinder::ReadInt32(off_anchor + 28));
        m_isReady = true;
        logger->Info("Successfully found legacy (v1.58) viewport data.");
      }
    }
  }

  if (!m_isReady) {
    logger->Error("Failed to find any compatible Viewport data structures.");
  }

  return m_isReady;
}

}  // namespace Data::GameData::Finders
SPF_NS_END
