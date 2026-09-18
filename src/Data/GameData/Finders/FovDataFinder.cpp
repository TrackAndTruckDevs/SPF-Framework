#include "SPF/Data/GameData/Finders/FovDataFinder.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Data::GameData::Finders {
using namespace Utils;

namespace {

/*
 * ANCHOR #1: Base FOV (UpdateCameraProjection initialization)
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraProjection[140771010]) ---/
 * 140771032  F3 0F 10 51 20                MOVSS XMM2,dword ptr [RCX + 0x20]
 * 140771037  48 83 C1 38                   ADD RCX,0x38
 */
const char* BASE_FOV_SIG = "[MOVSS xmm, [r64+off8]] [ADD r64, imm8]";

/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraProjection[140771010]) ---/
 * 140771050  F3 0F 11 5B 38                MOVSS dword ptr [RBX + 0x38],XMM3
 * 140771055  EB 05                         JMP 0x14077105c
 */
const char* HORIZ_FOV_SIG = "[MOVSS [r64+off8], xmm] [JMP rel8]";

/*
 * ANCHOR #3: Vertical FOV Final (Limit check block)
 *
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraProjection[140771010]) ---/
 * 140771066  F3 0F 11 43 3C                MOVSS dword ptr [RBX + 0x3c],XMM0
 * 14077106b  EB 05                         JMP 0x140771072
 * 14077106d  F3 0F 10 43 3C                MOVSS XMM0,dword ptr [RBX + 0x3c]
 */
const char* VERT_FOV_SIG = "[MOVSS [r64+off8], xmm] [JMP rel8]";

/*
 * ANCHOR #4: FOV Zoom Reference Base (native dynamic speed-FOV baseline)
 *
 * Inside the game's per-frame speed-based FOV system (not UpdateCameraProjection — a
 * separate function that recomputes BaseFovOffset from this field whenever the truck's
 * speed or the zoom toggle changes). Found via a memory write breakpoint on BaseFovOffset
 * followed by Ghidra decompilation of the writer.
 *
 * /--- Ghidra:(eurotrucks2_1.61.1.0s.exe) ---/
 * 1405e865e  F3 41 0F10 AE 5C1E0000        MOVSS XMM5,[R14 + 0x1e5c]
 * 1405e8667  F3 44 0F10 80 08040000        MOVSS XMM8,[RAX + 0x408]
 *
 * The first instruction is an exact-byte anchor (unrelated "current zoom state" field on a
 * different object, r14); the second is the target read, with its displacement wildcarded
 * so it can be re-extracted if it moves in a future game version.
 */
const char* FOV_ZOOM_BASE_SIG = "F3 41 0F 10 AE 5C 1E 00 00 F3 44 0F 10 80 ? ? ? ?";

}  // namespace

bool FovDataFinder::TryFindOffsets(GameDataCameraService& owner) {
  if (m_isReady) return true;

  FinderLog log(GetName());

  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  auto pfnUpdateCameraProjection = cameraHooks.GetUpdateCameraProjectionFunc();

  if (!pfnUpdateCameraProjection) {
    log.Warn("Cannot find FOV offsets: UpdateCameraProjection function pointer is not ready. Will retry...");
    return log.Finish(false);
  }

  // ── Phase 1: Base FOV ──
  {
    auto phase = log.MakePhase("Base FOV");

    uintptr_t addr = PatternFinder::Find((uintptr_t)pfnUpdateCameraProjection, 64, BASE_FOV_SIG);
    if (phase.Step(addr, "FOV Anchor #1 (Base FOV)", "RT")) {
      int8_t baseFovOffset = PatternFinder::ReadInt8(addr + 4);
      if (phase.StepOffset(baseFovOffset, "BaseFovOffset", "OFF")) {
        owner.SetFovBaseOffset(baseFovOffset);
      }
    }
  }

  // ── Phase 2: Horizontal FOV Final ──
  uintptr_t addr = 0;
  {
    auto phase = log.MakePhase("Horizontal FOV Final");

    addr = PatternFinder::Find((uintptr_t)pfnUpdateCameraProjection, 96, HORIZ_FOV_SIG);
    if (phase.Step(addr, "FOV Anchor #2 (Horizontal FOV)", "RT")) {
      int8_t horizFovOffset = PatternFinder::ReadInt8(addr + 4);
      if (phase.StepOffset(horizFovOffset, "HorizFovOffset", "OFF")) {
        owner.SetFovHorizFinalOffset(horizFovOffset);
      }
    }
  }

  // ── Phase 3: Vertical FOV Final ──
  {
    auto phase = log.MakePhase("Vertical FOV Final");

    if (addr) addr = PatternFinder::Find(addr + 5, 32, VERT_FOV_SIG);
    if (phase.Step(addr, "FOV Anchor #3 (Vertical FOV)", "RT")) {
      int8_t vertFovOffset = PatternFinder::ReadInt8(addr + 4);
      if (phase.StepOffset(vertFovOffset, "VertFovOffset", "OFF")) {
        owner.SetFovVertFinalOffset(vertFovOffset);
      }
    }
  }

  // ── Phase 4: FOV Zoom Reference Base ──
  // Optional: only the interior camera's manual FOV override relies on this to survive the
  // game's native speed-based FOV recompute. A miss here doesn't block FovDataFinder overall.
  {
    auto phase = log.MakePhase("FOV Zoom Reference Base");

    uintptr_t zoomAddr = PatternFinder::Find(FOV_ZOOM_BASE_SIG);
    if (phase.StepOptional(zoomAddr, "FOV Anchor #4 (Zoom Reference Base)", "RT")) {
      int32_t zoomBaseOffset = PatternFinder::ReadInt32(zoomAddr + 14);
      if (phase.StepOffsetOptional(zoomBaseOffset, "FovZoomBaseOffset", "OFF")) {
        owner.SetFovZoomBaseOffset(zoomBaseOffset);
      }
    }
  }

  // --- Final Readiness Check ---
  m_isReady = owner.GetFovBaseOffset() != 0 && owner.GetFovHorizFinalOffset() != 0 && owner.GetFovVertFinalOffset() != 0;

  return log.Finish(m_isReady);
}
}  // namespace Data::GameData::Finders
SPF_NS_END
