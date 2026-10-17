#include "SPF/Hooks/GameTools/PrismArrayResolver.hpp"

#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstddef>
#include <cstdint>

namespace SPF::Hooks::GameTools {

// ============================================================================
// Constructor / Singleton
// ============================================================================
PrismArrayResolver& PrismArrayResolver::GetInstance() {
  static PrismArrayResolver instance;
  return instance;
}

// ============================================================================
// Signatures
// ============================================================================
namespace {
/*
 * Empty control block call: CALL rel32 followed by MOV r64, imm64.
 * Search window: 256 bytes from the etalon text anchor
 * (PrismStringResolver::GetEtalonAnchorAddr — cached there, no rescan).
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(DeliverOvsNewRoutesMail[14084d030]) ---/
 * 14084d079  48 8D 05 B8 6B A7 01          LEA RAX,[0x1422c3c38] = "mailtext.ovs.new_routes"
 * 14084d12c  E8 4F 55 8A FF                CALL 0x1400f2680
 * 14084d131  48 BF 00 00 00 00 01 00 00 00 MOV RDI,0x100000000
 */
const char* EMPTY_CTRL_SIG = "[CALL rel32] [MOV r64, imm64]";
constexpr size_t EMPTY_CTRL_WINDOW = 256;

/*
 * Array vtable LEA right after the empty-ctrl CALL site.
 * Search window: 64 bytes from the empty-ctrl CALL site.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(DeliverOvsNewRoutesMail[14084d030]) ---/
 * 14084d14c  4C 8D 3D A5 8C 98 01          LEA R15,[0x1421d5df8]
 */
const char* ARRAY_VTABLE_LEA_SIG = "[LEA r64, [rip+off32]]";
constexpr size_t ARRAY_VTABLE_LEA_WINDOW = 64;

/*
 * SHL anchor between the empty-ctrl call and the push call.
 * Search window: 256 bytes from the empty-ctrl CALL site.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(DeliverOvsNewRoutesMail[14084d030]) ---/
 * 14084d1f7  48 C1 E7 05                   SHL RDI,0x5
 */
constexpr size_t PUSH_ANCHOR_WINDOW = 256;

/*
 * Push call: CALL rel32 followed by MOV r64, [r64+off8].
 * Search window: 256 bytes from the SHL anchor.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(DeliverOvsNewRoutesMail[14084d030]) ---/
 * 14084d2ae  E8 7D 9C 8E FF                CALL 0x140136f30
 * 14084d2b3  48 8B 7D 90                   MOV RDI,qword ptr [RBP + -0x70]
 */
const char* PUSH_SIG = "[CALL rel32] [MOV r64, [r64+off8]]";
constexpr size_t PUSH_WINDOW = 256;

/*
 * Array destructor pair: two consecutive CALL rel32 + LEA r64,[r64+off8].
 * Search window: 525 bytes from the push CALL site.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(DeliverOvsNewRoutesMail[14084d030]) ---/
 * 14084d498  E8 43 CF 8B FF                CALL 0x14010a3e0
 * 14084d49d  48 8D 4D 80                   LEA RCX,[RBP + -0x80]
 * 14084d4a1  E8 3A CF 8B FF                CALL 0x14010a3e0
 * 14084d4a6  48 8D 4C 24 60                LEA RCX,[RSP + 0x60]
 */
const char* ARRAY_DTOR_SIG = "[CALL rel32] [LEA r64, [r64+off8]] [CALL rel32] [LEA r64, [r64+off8]]";
constexpr size_t ARRAY_DTOR_WINDOW = 525;
}  // namespace

// ============================================================================
// Install / Uninstall / Remove
// ============================================================================
bool PrismArrayResolver::Install() {
  Utils::FinderLog log(m_name);
  log.Info("Searching for prism array building blocks...");

  if (IsInstalled()) {
    log.Info("prism array functions already found. Skipping installation.");
    return log.Finish(true);
  }

  uintptr_t emptyCtrl = 0;
  uintptr_t arrayVtable = 0;
  uintptr_t push = 0;
  uintptr_t dtor = 0;

  // ── Phase 1: Empty shared control block ──
  {
    auto phase = log.MakePhase("Empty Control Block");

    uintptr_t anchor = PrismStringResolver::GetInstance().GetEtalonAnchorAddr();
    if (phase.Step(anchor, "etalon anchor (cached)", "REF")) {
      uintptr_t addrCall = Utils::PatternFinder::Find(anchor, EMPTY_CTRL_WINDOW, EMPTY_CTRL_SIG);
      if (phase.Step(addrCall, "empty ctrl CALL site", "RT")) {
        emptyCtrl = Utils::PatternFinder::GetRipAddress(addrCall, 1, 5);
        phase.Step(emptyCtrl, "empty shared control block", "FN");

        // Array vtable LEA (from the empty-ctrl CALL site, window 64)
        uintptr_t addrVtLea = Utils::PatternFinder::Find(addrCall, ARRAY_VTABLE_LEA_WINDOW, ARRAY_VTABLE_LEA_SIG);
        if (phase.Step(addrVtLea, "array vtable LEA", "RT")) {
          arrayVtable = Utils::PatternFinder::GetRipAddress(addrVtLea, 3, 7);
          phase.Step(arrayVtable, "array vtable", "VT");
        }

        // ── Phase 2: SHL anchor + push (chained: needs addrCall) ──
        uintptr_t addrShl = Utils::PatternFinder::Find(addrCall, PUSH_ANCHOR_WINDOW, "[SHL r64, imm8]");
        if (phase.Step(addrShl, "SHL anchor", "RT")) {
          uintptr_t addrPush = Utils::PatternFinder::Find(addrShl, PUSH_WINDOW, PUSH_SIG);
          if (phase.Step(addrPush, "push CALL site", "RT")) {
            push = Utils::PatternFinder::GetRipAddress(addrPush, 1, 5);
            phase.Step(push, "array push", "FN");

            // ── Phase 3: Array destructor (chained: needs addrPush) ──
            uintptr_t addrDtor = Utils::PatternFinder::Find(addrPush, ARRAY_DTOR_WINDOW, ARRAY_DTOR_SIG);
            if (phase.Step(addrDtor, "array dtor CALL pair", "RT")) {
              dtor = Utils::PatternFinder::GetRipAddress(addrDtor, 1, 5);
              phase.Step(dtor, "array destructor", "FN");
            }
          }
        }
      }
    }
  }

  bool ready = (emptyCtrl != 0) && (arrayVtable != 0) && (push != 0) && (dtor != 0);

  if (ready) {
    m_emptyCtrl = emptyCtrl;
    m_arrayVtable = arrayVtable;
    m_push = push;
    m_dtor = dtor;
    log.Info("Found array blocks: empty ctrl {:#x}, vtable {:#x}, push {:#x}, dtor {:#x}", emptyCtrl,
             arrayVtable, push, dtor);
  } else {
    log.Error("Prism array discovery incomplete. Mail request building will be unavailable.");
  }

  return log.Finish(ready);
}

void PrismArrayResolver::Uninstall() {
  if (m_emptyCtrl || m_push || m_dtor) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Disabling prism array resolver, clearing pointers.");
    m_emptyCtrl = 0;
    m_arrayVtable = 0;
    m_push = 0;
    m_dtor = 0;
  }
}

void PrismArrayResolver::Remove() { Uninstall(); }

}  // namespace SPF::Hooks::GameTools
