#include "SPF/Hooks/GameTools/PrismStringResolver.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/SEHGuard.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace SPF::Hooks::GameTools {

// ============================================================================
// Constructor / Singleton
// ============================================================================
PrismStringResolver& PrismStringResolver::GetInstance() {
  static PrismStringResolver instance;
  return instance;
}

// ============================================================================
// Signatures
// ============================================================================
namespace {
/*
 * String anchor near a prism_string_set call site.
 * Do NOT jump to the function start — we need the LEA/xref address itself.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401ddcd0[1401ddcd0]) ---/
 * 1401ddde9  48 8D 0D B0 15 02 02          LEA RCX,[0x1421ff3a0] = "[win] Window creation failed."
 */
const char* PRISM_STRING_SET_ANCHOR_STR = "[win] Window creation failed.";

/*
 * CALL rel32 to prism_string_set followed by REX + MOV [r64+off8], r8.
 * Search window: 64 bytes from the anchor xref.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_1401ddcd0[1401ddcd0]) ---/
 * 1401dddff  E8 AC 63 F1 FF                CALL 0x1400f41b0
 * 1401dde04  40 88 6F 30                   MOV byte ptr [RDI + 0x30],BPL
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(prism_string_set[1400f41b0]) ---/
 * 1400f41b0  40 53                         PUSH RBX
 */
const char* PRISM_STRING_SET_SIG = "[CALL rel32] 40 [MOV [r64+off8], r8]";
constexpr size_t PRISM_STRING_SET_SEARCH_WINDOW = 64;

/*
 * Buffer offset: MOV r64, [r64+off8] reading char* buffer at prism_string+0x08.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(prism_string_set[1400f41b0]) ---/
 * 1400f41bc  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
 */
constexpr size_t BUFFER_OFFSET_SEARCH_WINDOW = 32;
constexpr int BUFFER_OFFSET_DISP_POS = 3;  // 48 8B 41 08 → byte at +3

/*
 * Length offset: MOV [r64+off8], r32 followed by TEST r64, r64.
 * Writes uint32 length at prism_string+0x10.
 *
 * /--- Ghidra:(amtrucks_1_61.exe) Fun:(prism_string_set[1400f41b0]) ---/
 * 1400f41d5  89 53 10                      MOV dword ptr [RBX + 0x10],EDX
 * 1400f41d8  4D 85 C0                      TEST R8,R8
 */
constexpr size_t LENGTH_OFFSET_SEARCH_WINDOW = 32;
constexpr int LENGTH_OFFSET_DISP_POS = 2;  // 89 53 10 → byte at +2
}  // namespace

// ============================================================================
// Install / Uninstall / Remove
// ============================================================================
bool PrismStringResolver::Install() {
  Utils::FinderLog log(m_name);
  log.Info("Searching for prism_string_set and field offsets...");

  if (IsInstalled()) {
    log.Info("prism_string_set already found. Skipping installation.");
    return log.Finish(true);
  }

  uintptr_t setFn = 0;
  int32_t bufferOffset = 0;
  int32_t lengthOffset = 0;
  uintptr_t addrBufferMov = 0;

  // ── Phase 1: prism_string_set function ──
  {
    auto phase = log.MakePhase("prism_string_set Function");

    // 1. String anchor (LEA site, not function start)
    uintptr_t anchor = Utils::PatternFinder::FindFunctionByString(PRISM_STRING_SET_ANCHOR_STR, false);
    if (phase.Step(anchor, "prism_string_set anchor string", "REF")) {
      // 2. CALL rel32 to prism_string_set within 64 bytes of the anchor
      uintptr_t callSite = Utils::PatternFinder::Find(anchor, PRISM_STRING_SET_SEARCH_WINDOW, PRISM_STRING_SET_SIG);
      if (phase.Step(callSite, "prism_string_set CALL site", "RT")) {
        // 3. Resolve CALL rel32 target
        setFn = Utils::PatternFinder::GetRipAddress(callSite, 1, 5);
        phase.Step(setFn, "prism_string_set", "FN");
      }
    }
  }

  // ── Phase 2: Buffer Offset (char* at +0x08) ──
  /*
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(prism_string_set[1400f41b0]) ---/
   * 1400f41bc  48 8B 41 08                   MOV RAX,qword ptr [RCX + 0x8]
   *
   * Search from the start of prism_string_set.
   */
  if (setFn) {
    auto phase = log.MakePhase("Buffer Offset");

    addrBufferMov = Utils::PatternFinder::Find(setFn, BUFFER_OFFSET_SEARCH_WINDOW, "[MOV r64, [r64+off8]]");
    if (phase.Step(addrBufferMov, "Buffer MOV [r64+off8]", "RT")) {
      bufferOffset = Utils::PatternFinder::ReadInt8(addrBufferMov + BUFFER_OFFSET_DISP_POS);
      phase.StepOffset(bufferOffset, "Buffer Offset", "OFF");
    }
  }

  // ── Phase 3: Length Offset (uint32 at +0x10) ──
  /*
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(prism_string_set[1400f41b0]) ---/
   * 1400f41d5  89 53 10                      MOV dword ptr [RBX + 0x10],EDX
   * 1400f41d8  4D 85 C0                      TEST R8,R8
   *
   * Search starts from the buffer MOV address (1400f41bc), not function start.
   */
  if (setFn) {
    auto phase = log.MakePhase("Length Offset");

    uintptr_t startSearch = addrBufferMov ? addrBufferMov : setFn;
    uintptr_t addrMovLength = Utils::PatternFinder::Find(startSearch, LENGTH_OFFSET_SEARCH_WINDOW, "[MOV [r64+off8], r32] [TEST r64, r64]");
    if (phase.Step(addrMovLength, "Length MOV+TEST", "RT")) {
      lengthOffset = Utils::PatternFinder::ReadInt8(addrMovLength + LENGTH_OFFSET_DISP_POS);
      phase.StepOffset(lengthOffset, "Length Offset", "OFF");
    }
  }

  bool ready = (setFn != 0) && (bufferOffset != 0) && (lengthOffset != 0);

  if (ready) {
    m_prismStringSet = setFn;
    m_bufferOffset = bufferOffset;
    m_lengthOffset = lengthOffset;
    log.Info("Found 'prism_string_set' at {:#x}, buffer@+{:#x}, length@+{:#x}", setFn, bufferOffset, lengthOffset);
  } else {
    log.Error("prism_string_set discovery incomplete. String R/W will be unavailable.");
  }

  return log.Finish(ready);
}

void PrismStringResolver::Uninstall() {
  if (m_prismStringSet) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Disabling prism string resolver, clearing pointers.");
    m_prismStringSet = 0;
    m_bufferOffset = 0;
    m_lengthOffset = 0;
  }
}

void PrismStringResolver::Remove() { Uninstall(); }

// ============================================================================
// Public API
// ============================================================================
bool PrismStringResolver::Set(void* prismString, const char* value) {
  if (!m_prismStringSet || !prismString || !value) {
    return false;
  }

  bool calledOk = Utils::InvokeSafe([&]() {
    auto func = reinterpret_cast<PrismStringSetFunc>(m_prismStringSet);
    // Callee does MOV R8,[RDX] (Ghidra 1400f41b6) — it wants char**, not char*.
    const char* strPtr = value;
    func(prismString, &strPtr);
  });

  return calledOk;
}

const char* PrismStringResolver::GetBuffer(const void* prismString) const {
  if (!m_prismStringSet || !prismString || m_bufferOffset <= 0) {
    return nullptr;
  }

  const char* buffer = nullptr;
  bool readOk = Utils::InvokeSafe([&]() {
    auto base = static_cast<const uint8_t*>(prismString);
    buffer = *reinterpret_cast<const char* const*>(base + m_bufferOffset);
  });

  return readOk ? buffer : nullptr;
}

uint32_t PrismStringResolver::GetLength(const void* prismString) const {
  if (!m_prismStringSet || !prismString || m_lengthOffset <= 0) {
    return 0;
  }

  uint32_t length = 0;
  bool readOk = Utils::InvokeSafe([&]() {
    auto base = static_cast<const uint8_t*>(prismString);
    length = *reinterpret_cast<const uint32_t*>(base + m_lengthOffset);
  });

  return readOk ? length : 0;
}

namespace {

// Distrust thresholds for game-owned string fields: length 0 (uninitialized /
// wrong offset) or > kPrismStringDistrustLength (garbage) makes ReadString fall
// back to a manual NUL scan capped at kPrismStringScanCap bytes. Real game
// paths and config strings are a few hundred bytes.
constexpr uint32_t kPrismStringDistrustLength = 0x10000;
constexpr size_t kPrismStringScanCap = 1024;

// IsValidAddress is VirtualQuery-based and answers per 4 KB page, so probing
// every byte inside a page is redundant — validating page boundaries only is
// byte-for-byte equivalent protection with ~3 orders of magnitude fewer calls.
bool IsValidRange(uintptr_t start, size_t len) {
  if (len == 0) return true;
  if (start == 0 || len > UINTPTR_MAX - start) return false;
  uintptr_t last = start + len - 1;
  uintptr_t pageFirst = start & ~static_cast<uintptr_t>(0xFFF);
  uintptr_t pageLast = last & ~static_cast<uintptr_t>(0xFFF);
  for (uintptr_t page = pageFirst;; page += 0x1000) {
    if (!Utils::PatternFinder::IsValidAddress(page)) return false;
    if (page == pageLast) break;
  }
  return true;
}

}  // namespace

std::string PrismStringResolver::ReadString(const void* prismString) const {
  const char* buf = GetBuffer(prismString);
  if (buf == nullptr) return {};
  uintptr_t bufAddr = reinterpret_cast<uintptr_t>(buf);

  uint32_t len = GetLength(prismString);
  if (len == 0 || len > kPrismStringDistrustLength) {
    // Length field unusable — manual NUL scan with page-granular readability checks.
    if (!Utils::PatternFinder::IsValidAddress(bufAddr)) return {};
    len = 0;
    for (size_t i = 0; i < kPrismStringScanCap; ++i) {
      uintptr_t addr = bufAddr + i;
      if ((addr & 0xFFF) == 0 && !Utils::PatternFinder::IsValidAddress(addr)) return {};
      if (buf[i] == '\0') {
        len = static_cast<uint32_t>(i);
        break;
      }
    }
    if (len == 0) return {};
  } else if (!IsValidRange(bufAddr, len)) {
    return {};
  }
  return std::string(buf, len);
}

bool PrismStringResolver::IsBufferReadable(const void* prismString) const {
  const char* buf = GetBuffer(prismString);
  return buf != nullptr && Utils::PatternFinder::IsValidAddress(reinterpret_cast<uintptr_t>(buf));
}

}  // namespace SPF::Hooks::GameTools
