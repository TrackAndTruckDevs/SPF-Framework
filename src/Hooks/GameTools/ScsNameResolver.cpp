#include "SPF/Hooks/GameTools/ScsNameResolver.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Utils/SEHGuard.hpp"

#include <cstdint>
#include <cstring>
#include <minwindef.h>
#include <string>

namespace SPF::Hooks::GameTools {

// ============================================================================
// Static members
// ============================================================================
void* ScsNameResolver::ScsStringWriter::g_vtable[3] = {nullptr, nullptr, nullptr};

// ============================================================================
// ScsStringWriter helpers
// ============================================================================
size_t ScsNameResolver::ScsStringWriter::EnsureCapacity(ScsNameResolver::ScsStringWriter* self, size_t required) {
  if (required <= static_cast<size_t>(self->capacity)) {
    return static_cast<size_t>(self->capacity);
  }
  return 0;  // Inline buffer cannot grow; callers see this as failure.
}

// ============================================================================
// Constructor / Singleton
// ============================================================================
ScsNameResolver::ScsNameResolver() { ScsStringWriter::g_vtable[2] = reinterpret_cast<void*>(ScsStringWriter::EnsureCapacity); }

ScsNameResolver& ScsNameResolver::GetInstance() {
  static ScsNameResolver instance;
  return instance;
}

// ============================================================================
// Signatures
// ============================================================================
namespace {
/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(DecodeSCSTokenToString[1400ece20]) ---/
 * 1400ece20  48 89 5C 24 10                MOV qword ptr [RSP + 0x10],RBX
 * 1400ece25  56                            PUSH RSI
 * 1400ece26  48 83 EC 30                   SUB RSP,0x30
 * 1400ece2a  48 8B F1                      MOV RSI,RCX
 * 1400ece2d  4C 8D 4C 24 20                LEA R9,[RSP + 0x20]
 * 1400ece32  48 8B 0A                      MOV RCX,qword ptr [RDX]
 * 1400ece35  4C 8D 1D A4 D6 C0 01          LEA R11,[0x141cfa4e0]
 * 1400ece3c  48 B8 FF FF FF FF FF FF FF 7F MOV RAX,0x7fffffffffffffff
 */
const char* DECODE_TOKEN_SIG = "[MOV [r64+off8], r64] [PUSH r64] [SUB r64, imm8] [MOV r64, r64] [LEA r64, [r64+off8]] [MOV r64, [r64]] [LEA r64, [rip+off32]] [MOV r64, imm64]";

/*
 * /--- Ghidra:(amtrucks_1_60.exe) Fun:(UnitIdToToken[140126d90]) ---/
 * 140126d90  4C 8B DC                      MOV R11,RSP
 * 140126d93  41 56                         PUSH R14
 * 140126d95  48 83 EC 70                   SUB RSP,0x70
 * 140126d99  4C 8B F1                      MOV R14,RCX
 * 140126d9c  85 D2                         TEST EDX,EDX
 * 140126d9e  0F 85 B0 01 00 00             JNZ 0x140126f54
 */
const char* UNIT_ID_TO_TOKEN_SIG = "[MOV r64, r64] [PUSH R8-R15] [SUB r64, imm8] [MOV r64, r64] [TEST r32, r32] [JNE rel32]";
}  // namespace

// ============================================================================
// Install / Uninstall / Remove
// ============================================================================
bool ScsNameResolver::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  if (IsInstalled()) {
    logger->Info("SCS name resolver functions already found. Skipping installation.");
    return true;
  }

  logger->Info("Searching for SCS name resolver functions...");

  // --- 1. ResolveUnitName ---
  /**
   * ResolveUnitName(builder*, name_id, unit*) — decodes name_id into the dotted
   * name inside a stack dynstring builder (etalon FUN_14049b120):
   * /--- Ghidra:(amtrucks_1_61.exe) Fun:(FUN_14049b120[14049b120]) ---/
   * 14049b3ce  48 8D 05 A3 FC DA 01          LEA RAX,[0x14224b078] = "driver.xxx"
   * FindBackward from that LEA site, [CALL rel32], window 64:
   * 14049b3c9  E8 62 07 C9 FF                CALL 0x14012bb30
   */
  uintptr_t nameLea = Utils::PatternFinder::FindFunctionByString("driver.xxx", false);
  uintptr_t resolveAddr = 0;
  if (Utils::PatternFinder::IsValidAddress(nameLea)) {
    uintptr_t nameCall = Utils::PatternFinder::FindBackward(nameLea, 64, "[CALL rel32]");
    if (Utils::PatternFinder::IsValidAddress(nameCall)) {
      resolveAddr = Utils::PatternFinder::GetRipAddress(nameCall, 1, 5);
    }
  }
  if (!Utils::PatternFinder::IsValidAddress(resolveAddr)) {
    logger->Critical("'ResolveUnitName' not found or invalid. Name resolution will be unavailable.");
    return false;
  }

  m_resolveAddr = resolveAddr;
  logger->Info("Found 'ResolveUnitName' at address: {:#x}", resolveAddr);

  // --- 2. DecodeToken ---
  uintptr_t decodeAddr = Utils::PatternFinder::Find(DECODE_TOKEN_SIG);
  // if (!Utils::PatternFinder::IsValidAddress(decodeAddr)) {
  //   logger->Critical("'DecodeToken' not found or invalid. Token decoding will be unavailable.");
  //   m_resolveAddr = 0;
  //   return false;
  // }

  m_decodeAddr = decodeAddr;
  logger->Info("Found 'DecodeToken' at address: {:#x}", decodeAddr);

  // --- 3. UnitIdToToken ---
  uintptr_t unitIdToTokenAddr = Utils::PatternFinder::Find(UNIT_ID_TO_TOKEN_SIG);
  if (!Utils::PatternFinder::IsValidAddress(unitIdToTokenAddr)) {
    logger->Critical("'UnitIdToToken' not found or invalid. Token resolution will be unavailable.");
    return false;
  }
  m_unitIdToTokenFn = unitIdToTokenAddr;
  logger->Info("Found 'UnitIdToToken' at address: {:#x}", unitIdToTokenAddr);

  ScsStringWriter::g_vtable[2] = reinterpret_cast<void*>(ScsStringWriter::EnsureCapacity);

  logger->Info("SCS name resolver functions installed successfully.");
  return true;
}

void ScsNameResolver::Uninstall() {
  if (m_resolveAddr || m_decodeAddr || m_unitIdToTokenFn) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Disabling SCS name resolver, clearing pointers.");
    m_resolveAddr = 0;
    m_decodeAddr = 0;
    m_unitIdToTokenFn = 0;
    ScsStringWriter::g_vtable[2] = nullptr;
  }
}

void ScsNameResolver::Remove() { Uninstall(); }

// ============================================================================
// Public API
// ============================================================================
std::string ScsNameResolver::ResolveUnitName(uint32_t unitId) {
  if (!m_resolveAddr) {
    return {};
  }

  ScsStringWriter writer;
  std::memset(&writer, 0, sizeof(writer));
  writer.vtable = ScsStringWriter::g_vtable;
  writer.buffer = writer.m_data;
  writer.capacity = static_cast<int32_t>(sizeof(writer.m_data));

  bool calledOk = Utils::InvokeSafe([&]() {
    auto func = reinterpret_cast<ResolveUnitNameFunc>(m_resolveAddr);
    func(&writer, unitId, 0);
  });

  if (!calledOk) {
    return {};
  }

  if (writer.length > 0 && writer.length < static_cast<int32_t>(sizeof(writer.m_data))) {
    writer.buffer[writer.length] = '\0';
    return std::string(writer.buffer);
  }
  return {};
}

std::string ScsNameResolver::DecodeToken(uint64_t token) {
  if (!m_decodeAddr) {
    return {};
  }

  ScsStringWriter writer;
  std::memset(&writer, 0, sizeof(writer));
  writer.vtable = ScsStringWriter::g_vtable;
  writer.buffer = writer.m_data;
  writer.capacity = static_cast<int32_t>(sizeof(writer.m_data));

  uint64_t mutableToken = token;
  bool decodeOk = false;
  bool calledOk = Utils::InvokeSafe([&]() {
    auto func = reinterpret_cast<DecodeTokenFunc>(m_decodeAddr);
    decodeOk = func(&writer, &mutableToken);
  });

  if (!calledOk || !decodeOk) {
    return {};
  }

  if (writer.length > 0 && writer.length < static_cast<int32_t>(sizeof(writer.m_data))) {
    writer.buffer[writer.length] = '\0';
    return std::string(writer.buffer);
  }
  return {};
}

uint64_t ScsNameResolver::ResolveUnitToken(uint32_t unitId) {
  if (!m_unitIdToTokenFn) return 0;

  typedef uint64_t*(__fastcall * UnitIdToToken_t)(uint64_t* out, uint32_t unitId, uint64_t param3);
  uint64_t result = 0;
  bool calledOk = Utils::InvokeSafe([&]() { ((UnitIdToToken_t)m_unitIdToTokenFn)(&result, unitId, 0); });

  if (!calledOk) return 0;
  return result;
}

}  // namespace SPF::Hooks::GameTools
