#pragma once

#include "SPF/Namespace.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

SPF_NS_BEGIN
namespace Utils {

// Inline definition of pattern templates used by ReplaceTemplates.
inline const std::unordered_map<std::string, std::string> kPatternTemplates = {
  // =========================================================================
  // ===							MOV INSTRUCTIONS                         ===
  // =========================================================================
  //
  // ModRM byte (Mod /Reg /R/M) — upper 2 bits = Mod addressing mode:
  //   Mod=00 [00-3f] — [r64]                 (no displacement)
  //   Mod=01 [40-7f] — [r64+disp8]           (8-bit displacement)
  //   Mod=10 [80-bf] — [r64+disp32]          (32-bit displacement)
  //   Mod=11 [c0-ff] — register-register
  //   R/M=100 (0x4)  — SIB byte follows      (index*scale + base)
  //   R/M=101 (0x5)  — RIP-relative (Mod=00) or disp32/64 (Mod=00)
  //
  // SIB byte (Scale:Index:Base):
  //   Scale  — [7:6] = 00(x1) 01(x2) 10(x4) 11(x8)
  //   Index  — [5:3] = register or 100=RSP (no index)
  //   Base   — [2:0] = register or 101=RBP (no base, disp follows)
  //
  // REX prefix: byte 0x40-0x4F, bit W (bit 3) = 1 → 64-bit operand

  // --- MOV: Register ← Immediate ---
  {"[MOV r8, imm8]",           "[b0-b7] ?"},
  {"[MOV r32, imm32]",         "4[0-f]? [b8-bf] ? ? ? ?"},
  {"[MOV r64, imm64]",         "4[8-f] [b8-bf] ? ? ? ? ? ? ? ?"},
  {"[MOV r64, imm32]",         "4[8-f] c7 [c0-ff] ? ? ? ?"},

  // --- MOV: Register ← Register ---
  {"[MOV r32, r32]",           "8b [c0-ff]"},
  {"[MOV r64, r64]",           "4[8-f] 8[b|9] [c0-ff]"},

  // --- MOV: Register ← Memory (RIP-Relative) ---
  {"[MOV r8, [rip+off32]]",    "8a 05 ? ? ? ?"},
  {"[MOV r32, [rip+off32]]",   "8b [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOV r64, [rip+off32]]",   "4[8-f] 8b [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- MOV: Register ← Memory (r64 base + disp32) ---
  {"[MOV r8, [r64+off32]]",    "8a [80-bf] [SIB?] ? ? ? ?"},
  {"[MOV r32, [r64+off32]]",   "8b [80-bf] [SIB?] ? ? ? ?"},
  {"[MOV r64, [r64+off32]]",   "4[8-f] 8b [80-bf] [SIB?] ? ? ? ?"},

  // --- MOV: Register ← Memory (r64 base + disp8) ---
  {"[MOV r8, [r64+off8]]",     "8a [40-7f] [SIB?] ?"},
  {"[MOV r32, [r64+off8]]",    "8b [40-7f] [SIB?] ?"},
  {"[MOV r64, [r64+off8]]",    "4[8-f] 8b [40-7f] [SIB?] ?"},

  // --- MOV: Register ← Memory (r64 base, no displacement) ---
  {"[MOV r8, [r64]]",          "8a [00-3f] [SIB?]"},
  {"[MOV r32, [r64]]",         "8b [00-3f] [SIB?]"},
  {"[MOV r64, [r64]]",         "4[8-f] 8b [00-3f] [SIB?]"},

  // --- MOV: Memory (RIP-Relative) ← Register ---
  {"[MOV [rip+off32], r8]",    "88 05 ? ? ? ?"},
  {"[MOV [rip+off32], r32]",   "4[0-f]? 89 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOV [rip+off32], r64]",   "4[8-f] 89 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- MOV: Memory (RIP-Relative) ← Immediate ---
  {"[MOV byte ptr [rip+off32], imm8]",   "c6 05 ? ? ? ? ?"},
  {"[MOV dword ptr [rip+off32], imm32]", "c7 05 ? ? ? ? ? ? ? ?"},

  // --- MOV: Memory (r64 base + disp32) ← Register ---
  {"[MOV [r64+off32], r8]",    "88 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOV [r64+off32], r32]",   "89 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOV [r64+off32], r64]",   "4[8-f] 89 [80-bf] [SIB?] ? ? ? ?"},

  // --- MOV: Memory (r64 base + disp8) ← Register ---
  {"[MOV [r64+off8], r8]",     "88 [40-7f] [SIB?] ?"},
  {"[MOV [r64+off8], r32]",    "89 [40-7f] [SIB?] ?"},
  {"[MOV [r64+off8], r64]",    "4[8-f] 89 [40-7f] [SIB?] ?"},

  // --- MOV: Memory (r64 base, no displacement) ← Register ---
  {"[MOV [r64], r8]",          "88 [00-3f] [SIB?]"},
  {"[MOV [r64], r32]",         "89 [00-3f] [SIB?]"},
  {"[MOV [r64], r64]",         "4[8-f] 89 [00-3f] [SIB?]"},

  // --- MOV: Memory (r64 base + disp32) ← Immediate ---
  {"[MOV byte ptr [r64+off32], imm8]",     "c6 [80-bf] [SIB?] ? ? ? ? ?"},
  {"[MOV dword ptr [r64+off32], imm32]",   "c7 [80-bf] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[MOV qword ptr [r64+off32], imm64_32]", "4[8-f] c7 [80-bf] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- MOV: Memory (r64 base + disp8) ← Immediate ---
  {"[MOV byte ptr [r64+off8], imm8]",      "c6 [40-7f] [SIB?] ? ?"},
  {"[MOV dword ptr [r64+off8], imm32]",    "c7 [40-7f] [SIB?] ? ? ? ? ?"},
  {"[MOV qword ptr [r64+off8], imm64_32]",  "4[8-f] c7 [40-7f] [SIB?] ? ? ? ? ?"},

  // --- MOV: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[MOV r8, [r64+sib+off32]]",    "8a [84-bc] ? ? ? ? ?"},
  {"[MOV r8, [r64+sib+off8]]",     "8a [44-7c] ? ? ?"},
  {"[MOV r8, [r64+sib]]",          "8a [04-3c] ?"},
  {"[MOV r32, [r64+sib+off32]]",   "8b [84-bc] ? ? ? ? ?"},
  {"[MOV r32, [r64+sib+off8]]",    "8b [44-7c] ? ? ?"},
  {"[MOV r32, [r64+sib]]",         "8b [04-3c] ?"},
  {"[MOV r64, [r64+sib+off32]]",   "4[8-f] 8b [84-bc] ? ? ? ? ?"},
  {"[MOV r64, [r64+sib+off8]]",    "4[8-f] 8b [44-7c] ? ? ?"},
  {"[MOV r64, [r64+sib]]",         "4[8-f] 8b [04-3c] ?"},

  // --- MOV: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[MOV [r64+sib+off32], r8]",    "88 [84-bc] ? ? ? ? ?"},
  {"[MOV [r64+sib+off8], r8]",     "88 [44-7c] ? ? ?"},
  {"[MOV [r64+sib], r8]",          "88 [04-3c] ?"},
  {"[MOV [r64+sib+off32], r32]",   "89 [84-bc] ? ? ? ? ?"},
  {"[MOV [r64+sib+off8], r32]",    "89 [44-7c] ? ? ?"},
  {"[MOV [r64+sib], r32]",         "89 [04-3c] ?"},
  {"[MOV [r64+sib+off32], r64]",   "4[8-f] 89 [84-bc] ? ? ? ? ?"},
  {"[MOV [r64+sib+off8], r64]",    "4[8-f] 89 [44-7c] ? ? ?"},
  {"[MOV [r64+sib], r64]",         "4[8-f] 89 [04-3c] ?"},

  // --- MOV: Zero Extend (MOVZX) ---
  {"[MOVZX r32, [r64]]",          "0f b6 [00-3f] [SIB?]"},
  {"[MOVZX r32, [r64+off8]]",     "0f b6 [40-7f] [SIB?] ?"},
  {"[MOVZX r32, [r64+off32]]",    "0f b6 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVZX r64, [r64+off8]]",     "4[8-f] 0f b6 [40-7f] [SIB?] ?"},
  {"[MOVZX r64, [r64+off32]]",    "4[8-f] 0f b6 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVZX r32, w[r64]]",        "0f b7 [00-3f] [SIB?]"},
  {"[MOVZX r32, w[r64+off8]]",   "0f b7 [40-7f] [SIB?] ?"},
  {"[MOVZX r32, w[r64+off32]]",  "0f b7 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVZX r32, r8]",            "0f b6 [c0-ff]"},
  {"[MOVZX r64, r8]",            "4[8-f] 0f b6 [c0-ff]"},

  // --- MOV: Sign Extend (MOVSXD) ---
  {"[MOVSXD r64, r32]",            "4[8-f] 63 [c0-ff]"},
  {"[MOVSXD r64, [r64+off8]]",     "4[8-f] 63 [40-7f] [SIB?] ?"},
  {"[MOVSXD r64, [r64+off32]]",    "4[8-f] 63 [80-bf] [SIB?] ? ? ? ?"},

  // --- MOVSX: Sign Extend (Byte/Word to Dword/Qword) ---
  {"[MOVSX r32, [r64]]",           "0f be [00-3f] [SIB?]"},
  {"[MOVSX r32, [r64+off8]]",      "0f be [40-7f] [SIB?] ?"},
  {"[MOVSX r32, [r64+off32]]",     "0f be [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSX r64, [r64+off8]]",      "4[8-f] 0f be [40-7f] [SIB?] ?"},
  {"[MOVSX r64, [r64+off32]]",     "4[8-f] 0f be [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSX r32, w[r64]]",         "0f bf [00-3f] [SIB?]"},
  {"[MOVSX r32, w[r64+off8]]",    "0f bf [40-7f] [SIB?] ?"},
  {"[MOVSX r32, w[r64+off32]]",   "0f bf [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSX r64, w[r64+off8]]",    "4[8-f] 0f bf [40-7f] [SIB?] ?"},
  {"[MOVSX r64, w[r64+off32]]",   "4[8-f] 0f bf [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSX r32, r8]",             "0f be [c0-ff]"},
  {"[MOVSX r64, r8]",             "4[8-f] 0f be [c0-ff]"},
  {"[MOVSX r32, r16]",            "0f bf [c0-ff]"},
  {"[MOVSX r64, r16]",            "4[8-f] 0f bf [c0-ff]"},

  // =========================================================================
  // ===						LEA (LOAD EFFECTIVE ADDRESS)                 ===
  // =========================================================================

  // --- LEA: Register ← Address (64-bit operand) ---
  {"[LEA r64, [rip+off32]]",   "4[8-f] 8d [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[LEA r64, [r64+off32]]",   "4[8-f] 8d [80-bf] [SIB?] ? ? ? ?"},
  {"[LEA r64, [r64+off8]]",    "4[8-f] 8d [40-7f] [SIB?] ?"},
  {"[LEA r64, [r64]]",         "4[8-f] 8d [00-3f] [SIB?]"},

  // --- LEA: Register ← Address (32-bit operand) ---
  {"[LEA r32, [rip+off32]]",   "8d [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[LEA r32, [r64+off32]]",   "8d [80-bf] [SIB?] ? ? ? ?"},
  {"[LEA r32, [r64+off8]]",    "8d [40-7f] [SIB?] ?"},
  {"[LEA r32, [r64]]",         "8d [00-3f] [SIB?]"},

  // --- LEA: Register ← Address (16-bit operand) ---
  {"[LEA r16, [rip+off32]]",   "66 8d [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[LEA r16, [r64+off8]]",    "66 8d [40-7f] [SIB?] ?"},
  {"[LEA r16, [r64]]",         "66 8d [00-3f] [SIB?]"},

  // --- LEA: SIB-specific (ModRM R/M=100) ---
  {"[LEA r64, [r64+sib+off32]]", "4[8-f] 8d [84-bc] ? ? ? ? ?"},
  {"[LEA r64, [r64+sib+off8]]",  "4[8-f] 8d [44-7c] ? ? ?"},
  {"[LEA r64, [r64+sib]]",       "4[8-f] 8d [04-3c] ?"},
  {"[LEA r32, [r64+sib+off32]]", "8d [84-bc] ? ? ? ? ?"},
  {"[LEA r32, [r64+sib+off8]]",  "8d [44-7c] ? ? ?"},
  {"[LEA r32, [r64+sib]]",       "8d [04-3c] ?"},

  // =========================================================================
  // ===							ARITHMETIC & LOGIC                       ===
  // =========================================================================

  // --- ADD: Register ← Register ---
  {"[ADD r64, r64]",             "4[8-f] [01|03] [c0-ff]"},
  {"[ADD r32, r32]",             "[01|03] [c0-ff]"},
  {"[ADD r8, r8]",               "[00|02] [c0-ff]"},

  // --- ADD: Register ← Immediate ---
  {"[ADD r64, imm32]",           "4[8-f] 81 [c0-c7] ? ? ? ?"},
  {"[ADD r32, imm32]",           "81 [c0-c7] ? ? ? ?"},
  {"[ADD r8, imm8]",             "80 [c0-c7] ?"},
  {"[ADD r64, imm8]",            "4[8-f] 83 [c0-c7] ?"},
  {"[ADD r32, imm8]",            "83 [c0-c7] ?"},

  // --- ADD: Register ← Memory (RIP-Relative) ---
  {"[ADD r8, [rip+off32]]",      "02 05 ? ? ? ?"},
  {"[ADD r32, [rip+off32]]",     "03 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[ADD r64, [rip+off32]]",     "4[8-f] 03 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- ADD: Register ← Memory (r64 base + disp32) ---
  {"[ADD r8, [r64+off32]]",      "02 [80-bf] [SIB?] ? ? ? ?"},
  {"[ADD r32, [r64+off32]]",     "03 [80-bf] [SIB?] ? ? ? ?"},
  {"[ADD r64, [r64+off32]]",     "4[8-f] 03 [80-bf] [SIB?] ? ? ? ?"},

  // --- ADD: Register ← Memory (r64 base + disp8) ---
  {"[ADD r8, [r64+off8]]",       "02 [40-7f] [SIB?] ?"},
  {"[ADD r32, [r64+off8]]",      "03 [40-7f] [SIB?] ?"},
  {"[ADD r64, [r64+off8]]",      "4[8-f] 03 [40-7f] [SIB?] ?"},

  // --- ADD: Register ← Memory (r64 base, no displacement) ---
  {"[ADD r8, [r64]]",            "02 [00-3f] [SIB?]"},
  {"[ADD r32, [r64]]",           "03 [00-3f] [SIB?]"},
  {"[ADD r64, [r64]]",           "4[8-f] 03 [00-3f] [SIB?]"},

  // --- ADD: Memory (RIP-Relative) ← Register ---
  {"[ADD [rip+off32], r8]",      "00 05 ? ? ? ?"},
  {"[ADD [rip+off32], r32]",     "01 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[ADD [rip+off32], r64]",     "4[8-f] 01 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- ADD: Memory (r64 base + disp32) ← Register ---
  {"[ADD [r64+off32], r8]",      "00 [80-bf] [SIB?] ? ? ? ?"},
  {"[ADD [r64+off32], r32]",     "01 [80-bf] [SIB?] ? ? ? ?"},
  {"[ADD [r64+off32], r64]",     "4[8-f] 01 [80-bf] [SIB?] ? ? ? ?"},

  // --- ADD: Memory (r64 base + disp8) ← Register ---
  {"[ADD [r64+off8], r8]",       "00 [40-7f] [SIB?] ?"},
  {"[ADD [r64+off8], r32]",      "01 [40-7f] [SIB?] ?"},
  {"[ADD [r64+off8], r64]",      "4[8-f] 01 [40-7f] [SIB?] ?"},

  // --- ADD: Memory (r64 base, no displacement) ← Register ---
  {"[ADD [r64], r8]",            "00 [00-3f] [SIB?]"},
  {"[ADD [r64], r32]",           "01 [00-3f] [SIB?]"},
  {"[ADD [r64], r64]",           "4[8-f] 01 [00-3f] [SIB?]"},

  // --- ADD: Memory (r64 base + disp32) ← Immediate ---
  {"[ADD byte ptr [r64+off32], imm8]",     "80 [80-87] [SIB?] ? ? ? ? ?"},
  {"[ADD dword ptr [r64+off32], imm32]",   "81 [80-87] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[ADD qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [80-87] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- ADD: Memory (r64 base + disp8) ← Immediate ---
  {"[ADD byte ptr [r64+off8], imm8]",      "80 [40-47] [SIB?] ? ?"},
  {"[ADD dword ptr [r64+off8], imm32]",    "81 [40-47] [SIB?] ? ? ? ? ?"},
  {"[ADD qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [40-47] [SIB?] ? ? ? ? ?"},

  // --- ADD: Memory (RIP-Relative) ← Immediate ---
  {"[ADD byte ptr [rip+off32], imm8]",     "80 05 ? ? ? ? ?"},
  {"[ADD dword ptr [rip+off32], imm32]",   "81 05 ? ? ? ? ? ? ? ?"},
  {"[ADD qword ptr [rip+off32], imm64_32]", "4[8-f] 81 05 ? ? ? ? ? ? ? ?"},

  // --- ADD: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[ADD r8, [r64+sib+off32]]",    "02 [84-bc] ? ? ? ? ?"},
  {"[ADD r8, [r64+sib+off8]]",     "02 [44-7c] ? ? ?"},
  {"[ADD r8, [r64+sib]]",          "02 [04-3c] ?"},
  {"[ADD r32, [r64+sib+off32]]",   "03 [84-bc] ? ? ? ? ?"},
  {"[ADD r32, [r64+sib+off8]]",    "03 [44-7c] ? ? ?"},
  {"[ADD r32, [r64+sib]]",         "03 [04-3c] ?"},
  {"[ADD r64, [r64+sib+off32]]",   "4[8-f] 03 [84-bc] ? ? ? ? ?"},
  {"[ADD r64, [r64+sib+off8]]",    "4[8-f] 03 [44-7c] ? ? ?"},
  {"[ADD r64, [r64+sib]]",         "4[8-f] 03 [04-3c] ?"},

  // --- ADD: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[ADD [r64+sib+off32], r8]",    "00 [84-bc] ? ? ? ? ?"},
  {"[ADD [r64+sib+off8], r8]",     "00 [44-7c] ? ? ?"},
  {"[ADD [r64+sib], r8]",          "00 [04-3c] ?"},
  {"[ADD [r64+sib+off32], r32]",   "01 [84-bc] ? ? ? ? ?"},
  {"[ADD [r64+sib+off8], r32]",    "01 [44-7c] ? ? ?"},
  {"[ADD [r64+sib], r32]",         "01 [04-3c] ?"},
  {"[ADD [r64+sib+off32], r64]",   "4[8-f] 01 [84-bc] ? ? ? ? ?"},
  {"[ADD [r64+sib+off8], r64]",    "4[8-f] 01 [44-7c] ? ? ?"},
  {"[ADD [r64+sib], r64]",         "4[8-f] 01 [04-3c] ?"},

  // --- SUB: Register ← Register ---
  {"[SUB r64, r64]",             "4[8-f] [29|2b] [c0-ff]"},
  {"[SUB r32, r32]",             "[29|2b] [c0-ff]"},
  {"[SUB r8, r8]",               "[28|2a] [c0-ff]"},

  // --- SUB: Register ← Immediate ---
  {"[SUB r64, imm32]",           "4[8-f] 81 [e8-ef] ? ? ? ?"},
  {"[SUB r32, imm32]",           "81 [e8-ef] ? ? ? ?"},
  {"[SUB r8, imm8]",             "80 [e8-ef] ?"},
  {"[SUB r64, imm8]",            "4[8-f] 83 [e8-ef] ?"},
  {"[SUB r32, imm8]",            "83 [e8-ef] ?"},

  // --- SUB: Register ← Memory (RIP-Relative) ---
  {"[SUB r8, [rip+off32]]",      "2a 05 ? ? ? ?"},
  {"[SUB r32, [rip+off32]]",     "2b [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[SUB r64, [rip+off32]]",     "4[8-f] 2b [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- SUB: Register ← Memory (r64 base + disp32) ---
  {"[SUB r8, [r64+off32]]",      "2a [80-bf] [SIB?] ? ? ? ?"},
  {"[SUB r32, [r64+off32]]",     "2b [80-bf] [SIB?] ? ? ? ?"},
  {"[SUB r64, [r64+off32]]",     "4[8-f] 2b [80-bf] [SIB?] ? ? ? ?"},

  // --- SUB: Register ← Memory (r64 base + disp8) ---
  {"[SUB r8, [r64+off8]]",       "2a [40-7f] [SIB?] ?"},
  {"[SUB r32, [r64+off8]]",      "2b [40-7f] [SIB?] ?"},
  {"[SUB r64, [r64+off8]]",      "4[8-f] 2b [40-7f] [SIB?] ?"},

  // --- SUB: Register ← Memory (r64 base, no displacement) ---
  {"[SUB r8, [r64]]",            "2a [00-3f] [SIB?]"},
  {"[SUB r32, [r64]]",           "2b [00-3f] [SIB?]"},
  {"[SUB r64, [r64]]",           "4[8-f] 2b [00-3f] [SIB?]"},

  // --- SUB: Memory (RIP-Relative) ← Register ---
  {"[SUB [rip+off32], r8]",      "28 2d ? ? ? ?"},
  {"[SUB [rip+off32], r32]",     "29 2d ? ? ? ?"},
  {"[SUB [rip+off32], r64]",     "4[8-f] 29 2d ? ? ? ?"},

  // --- SUB: Memory (r64 base + disp32) ← Register ---
  {"[SUB [r64+off32], r8]",      "28 [a8-af] [SIB?] ? ? ? ?"},
  {"[SUB [r64+off32], r32]",     "29 [a8-af] [SIB?] ? ? ? ?"},
  {"[SUB [r64+off32], r64]",     "4[8-f] 29 [a8-af] [SIB?] ? ? ? ?"},

  // --- SUB: Memory (r64 base + disp8) ← Register ---
  {"[SUB [r64+off8], r8]",       "28 [68-6f] [SIB?] ?"},
  {"[SUB [r64+off8], r32]",      "29 [68-6f] [SIB?] ?"},
  {"[SUB [r64+off8], r64]",      "4[8-f] 29 [68-6f] [SIB?] ?"},

  // --- SUB: Memory (r64 base, no displacement) ← Register ---
  {"[SUB [r64], r8]",            "28 [28-2f] [SIB?]"},
  {"[SUB [r64], r32]",           "29 [28-2f] [SIB?]"},
  {"[SUB [r64], r64]",           "4[8-f] 29 [28-2f] [SIB?]"},

  // --- SUB: Memory (r64 base + disp32) ← Immediate ---
  {"[SUB byte ptr [r64+off32], imm8]",     "80 [a8-af] [SIB?] ? ? ? ? ?"},
  {"[SUB dword ptr [r64+off32], imm32]",   "81 [a8-af] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[SUB qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [a8-af] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- SUB: Memory (r64 base + disp8) ← Immediate ---
  {"[SUB byte ptr [r64+off8], imm8]",      "80 [68-6f] [SIB?] ? ?"},
  {"[SUB dword ptr [r64+off8], imm32]",    "81 [68-6f] [SIB?] ? ? ? ? ?"},
  {"[SUB qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [68-6f] [SIB?] ? ? ? ? ?"},

  // --- SUB: Memory (RIP-Relative) ← Immediate ---
  {"[SUB byte ptr [rip+off32], imm8]",     "80 2d ? ? ? ? ?"},
  {"[SUB dword ptr [rip+off32], imm32]",   "81 2d ? ? ? ? ? ? ? ?"},
  {"[SUB qword ptr [rip+off32], imm64_32]", "4[8-f] 81 2d ? ? ? ? ? ? ? ?"},

  // --- SUB: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[SUB r8, [r64+sib+off32]]",    "2a [84-bc] ? ? ? ? ?"},
  {"[SUB r8, [r64+sib+off8]]",     "2a [44-7c] ? ? ?"},
  {"[SUB r8, [r64+sib]]",          "2a [04-3c] ?"},
  {"[SUB r32, [r64+sib+off32]]",   "2b [84-bc] ? ? ? ? ?"},
  {"[SUB r32, [r64+sib+off8]]",    "2b [44-7c] ? ? ?"},
  {"[SUB r32, [r64+sib]]",         "2b [04-3c] ?"},
  {"[SUB r64, [r64+sib+off32]]",   "4[8-f] 2b [84-bc] ? ? ? ? ?"},
  {"[SUB r64, [r64+sib+off8]]",    "4[8-f] 2b [44-7c] ? ? ?"},
  {"[SUB r64, [r64+sib]]",         "4[8-f] 2b [04-3c] ?"},

  // --- SUB: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[SUB [r64+sib+off32], r8]",    "28 [84-bc] ? ? ? ? ?"},
  {"[SUB [r64+sib+off8], r8]",     "28 [44-7c] ? ? ?"},
  {"[SUB [r64+sib], r8]",          "28 [04-3c] ?"},
  {"[SUB [r64+sib+off32], r32]",   "29 [84-bc] ? ? ? ? ?"},
  {"[SUB [r64+sib+off8], r32]",    "29 [44-7c] ? ? ?"},
  {"[SUB [r64+sib], r32]",         "29 [04-3c] ?"},
  {"[SUB [r64+sib+off32], r64]",   "4[8-f] 29 [84-bc] ? ? ? ? ?"},
  {"[SUB [r64+sib+off8], r64]",    "4[8-f] 29 [44-7c] ? ? ?"},
  {"[SUB [r64+sib], r64]",         "4[8-f] 29 [04-3c] ?"},

  // --- XOR: Register ← Register ---
  {"[XOR r64, r64]",             "4[8-f] [31|33] [c0-ff]"},
  {"[XOR r32, r32]",             "[31|33] [c0-ff]"},
  {"[XOR r16, r16]",             "66 [31|33] [c0-ff]"},
  {"[XOR r8, r8]",               "[30|32] [c0-ff]"},

  // --- XOR: Register ← Immediate ---
  {"[XOR r64, imm32]",           "4[8-f] 81 [f0-f7] ? ? ? ?"},
  {"[XOR r32, imm32]",           "81 [f0-f7] ? ? ? ?"},
  {"[XOR r8, imm8]",             "80 [f0-f7] ?"},
  {"[XOR r64, imm8]",            "4[8-f] 83 [f0-f7] ?"},
  {"[XOR r32, imm8]",            "83 [f0-f7] ?"},

  // --- XOR: Register ← Memory (RIP-Relative) ---
  {"[XOR r8, [rip+off32]]",      "32 35 ? ? ? ?"},
  {"[XOR r32, [rip+off32]]",     "33 35 ? ? ? ?"},
  {"[XOR r64, [rip+off32]]",     "4[8-f] 33 35 ? ? ? ?"},

  // --- XOR: Register ← Memory (r64 base + disp32) ---
  {"[XOR r8, [r64+off32]]",      "32 [80-bf] [SIB?] ? ? ? ?"},
  {"[XOR r32, [r64+off32]]",     "33 [80-bf] [SIB?] ? ? ? ?"},
  {"[XOR r64, [r64+off32]]",     "4[8-f] 33 [80-bf] [SIB?] ? ? ? ?"},

  // --- XOR: Register ← Memory (r64 base + disp8) ---
  {"[XOR r8, [r64+off8]]",       "32 [40-7f] [SIB?] ?"},
  {"[XOR r32, [r64+off8]]",      "33 [40-7f] [SIB?] ?"},
  {"[XOR r64, [r64+off8]]",      "4[8-f] 33 [40-7f] [SIB?] ?"},

  // --- XOR: Register ← Memory (r64 base, no displacement) ---
  {"[XOR r8, [r64]]",            "32 [00-3f] [SIB?]"},
  {"[XOR r32, [r64]]",           "33 [00-3f] [SIB?]"},
  {"[XOR r64, [r64]]",           "4[8-f] 33 [00-3f] [SIB?]"},

  // --- XOR: Memory (RIP-Relative) ← Register ---
  {"[XOR [rip+off32], r8]",      "30 35 ? ? ? ?"},
  {"[XOR [rip+off32], r32]",     "31 35 ? ? ? ?"},
  {"[XOR [rip+off32], r64]",     "4[8-f] 31 35 ? ? ? ?"},

  // --- XOR: Memory (r64 base + disp32) ← Register ---
  {"[XOR [r64+off32], r8]",      "30 [b0-b7] [SIB?] ? ? ? ?"},
  {"[XOR [r64+off32], r32]",     "31 [b0-b7] [SIB?] ? ? ? ?"},
  {"[XOR [r64+off32], r64]",     "4[8-f] 31 [b0-b7] [SIB?] ? ? ? ?"},

  // --- XOR: Memory (r64 base + disp8) ← Register ---
  {"[XOR [r64+off8], r8]",       "30 [70-77] [SIB?] ?"},
  {"[XOR [r64+off8], r32]",      "31 [70-77] [SIB?] ?"},
  {"[XOR [r64+off8], r64]",      "4[8-f] 31 [70-77] [SIB?] ?"},

  // --- XOR: Memory (r64 base, no displacement) ← Register ---
  {"[XOR [r64], r8]",            "30 [30-37] [SIB?]"},
  {"[XOR [r64], r32]",           "31 [30-37] [SIB?]"},
  {"[XOR [r64], r64]",           "4[8-f] 31 [30-37] [SIB?]"},

  // --- XOR: Memory (r64 base + disp32) ← Immediate ---
  {"[XOR byte ptr [r64+off32], imm8]",     "80 [b0-b7] [SIB?] ? ? ? ? ?"},
  {"[XOR dword ptr [r64+off32], imm32]",   "81 [b0-b7] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[XOR qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [b0-b7] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- XOR: Memory (r64 base + disp8) ← Immediate ---
  {"[XOR byte ptr [r64+off8], imm8]",      "80 [70-77] [SIB?] ? ?"},
  {"[XOR dword ptr [r64+off8], imm32]",    "81 [70-77] [SIB?] ? ? ? ? ?"},
  {"[XOR qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [70-77] [SIB?] ? ? ? ? ?"},

  // --- XOR: Memory (RIP-Relative) ← Immediate ---
  {"[XOR byte ptr [rip+off32], imm8]",     "80 35 ? ? ? ? ?"},
  {"[XOR dword ptr [rip+off32], imm32]",   "81 35 ? ? ? ? ? ? ? ?"},
  {"[XOR qword ptr [rip+off32], imm64_32]", "4[8-f] 81 35 ? ? ? ? ? ? ? ?"},

  // --- XOR: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[XOR r8, [r64+sib+off32]]",    "32 [84-bc] ? ? ? ? ?"},
  {"[XOR r8, [r64+sib+off8]]",     "32 [44-7c] ? ? ?"},
  {"[XOR r8, [r64+sib]]",          "32 [04-3c] ?"},
  {"[XOR r32, [r64+sib+off32]]",   "33 [84-bc] ? ? ? ? ?"},
  {"[XOR r32, [r64+sib+off8]]",    "33 [44-7c] ? ? ?"},
  {"[XOR r32, [r64+sib]]",         "33 [04-3c] ?"},
  {"[XOR r64, [r64+sib+off32]]",   "4[8-f] 33 [84-bc] ? ? ? ? ?"},
  {"[XOR r64, [r64+sib+off8]]",    "4[8-f] 33 [44-7c] ? ? ?"},
  {"[XOR r64, [r64+sib]]",         "4[8-f] 33 [04-3c] ?"},

  // --- XOR: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[XOR [r64+sib+off32], r8]",    "30 [84-bc] ? ? ? ? ?"},
  {"[XOR [r64+sib+off8], r8]",     "30 [44-7c] ? ? ?"},
  {"[XOR [r64+sib], r8]",          "30 [04-3c] ?"},
  {"[XOR [r64+sib+off32], r32]",   "31 [84-bc] ? ? ? ? ?"},
  {"[XOR [r64+sib+off8], r32]",    "31 [44-7c] ? ? ?"},
  {"[XOR [r64+sib], r32]",         "31 [04-3c] ?"},
  {"[XOR [r64+sib+off32], r64]",   "4[8-f] 31 [84-bc] ? ? ? ? ?"},
  {"[XOR [r64+sib+off8], r64]",    "4[8-f] 31 [44-7c] ? ? ?"},
  {"[XOR [r64+sib], r64]",         "4[8-f] 31 [04-3c] ?"},

  // --- AND: Register ← Register ---
  {"[AND r64, r64]",             "4[8-f] [21|23] [c0-ff]"},
  {"[AND r32, r32]",             "[21|23] [c0-ff]"},
  {"[AND r8, r8]",               "[20|22] [c0-ff]"},

  // --- AND: Register ← Immediate ---
  {"[AND r64, imm32]",           "4[8-f] 81 [e0-e7] ? ? ? ?"},
  {"[AND r32, imm32]",           "81 [e0-e7] ? ? ? ?"},
  {"[AND r8, imm8]",             "80 [e0-e7] ?"},
  {"[AND r64, imm8]",            "4[8-f] 83 [e0-e7] ?"},
  {"[AND r32, imm8]",            "83 [e0-e7] ?"},
  {"[AND AL, imm8]",             "24 ?"},
  {"[AND EAX, imm32]",           "25 ? ? ? ?"},

  // --- AND: Register ← Memory (RIP-Relative) ---
  {"[AND r8, [rip+off32]]",      "22 25 ? ? ? ?"},
  {"[AND r32, [rip+off32]]",     "23 25 ? ? ? ?"},
  {"[AND r64, [rip+off32]]",     "4[8-f] 23 25 ? ? ? ?"},

  // --- AND: Register ← Memory (r64 base + disp32) ---
  {"[AND r8, [r64+off32]]",      "22 [80-bf] [SIB?] ? ? ? ?"},
  {"[AND r32, [r64+off32]]",     "23 [80-bf] [SIB?] ? ? ? ?"},
  {"[AND r64, [r64+off32]]",     "4[8-f] 23 [80-bf] [SIB?] ? ? ? ?"},

  // --- AND: Register ← Memory (r64 base + disp8) ---
  {"[AND r8, [r64+off8]]",       "22 [40-7f] [SIB?] ?"},
  {"[AND r32, [r64+off8]]",      "23 [40-7f] [SIB?] ?"},
  {"[AND r64, [r64+off8]]",      "4[8-f] 23 [40-7f] [SIB?] ?"},

  // --- AND: Register ← Memory (r64 base, no displacement) ---
  {"[AND r8, [r64]]",            "22 [00-3f] [SIB?]"},
  {"[AND r32, [r64]]",           "23 [00-3f] [SIB?]"},
  {"[AND r64, [r64]]",           "4[8-f] 23 [00-3f] [SIB?]"},

  // --- AND: Memory (RIP-Relative) ← Register ---
  {"[AND [rip+off32], r8]",      "20 25 ? ? ? ?"},
  {"[AND [rip+off32], r32]",     "21 25 ? ? ? ?"},
  {"[AND [rip+off32], r64]",     "4[8-f] 21 25 ? ? ? ?"},

  // --- AND: Memory (r64 base + disp32) ← Register ---
  {"[AND [r64+off32], r8]",      "20 [a0-a7] [SIB?] ? ? ? ?"},
  {"[AND [r64+off32], r32]",     "21 [a0-a7] [SIB?] ? ? ? ?"},
  {"[AND [r64+off32], r64]",     "4[8-f] 21 [a0-a7] [SIB?] ? ? ? ?"},

  // --- AND: Memory (r64 base + disp8) ← Register ---
  {"[AND [r64+off8], r8]",       "20 [60-67] [SIB?] ?"},
  {"[AND [r64+off8], r32]",      "21 [60-67] [SIB?] ?"},
  {"[AND [r64+off8], r64]",      "4[8-f] 21 [60-67] [SIB?] ?"},

  // --- AND: Memory (r64 base, no displacement) ← Register ---
  {"[AND [r64], r8]",            "20 [20-27] [SIB?]"},
  {"[AND [r64], r32]",           "21 [20-27] [SIB?]"},
  {"[AND [r64], r64]",           "4[8-f] 21 [20-27] [SIB?]"},

  // --- AND: Memory (r64 base + disp32) ← Immediate ---
  {"[AND byte ptr [r64+off32], imm8]",     "80 [a0-a7] [SIB?] ? ? ? ? ?"},
  {"[AND dword ptr [r64+off32], imm32]",   "81 [a0-a7] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[AND qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [a0-a7] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- AND: Memory (r64 base + disp8) ← Immediate ---
  {"[AND byte ptr [r64+off8], imm8]",      "80 [60-67] [SIB?] ? ?"},
  {"[AND dword ptr [r64+off8], imm32]",    "81 [60-67] [SIB?] ? ? ? ? ?"},
  {"[AND qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [60-67] [SIB?] ? ? ? ? ?"},

  // --- AND: Memory (RIP-Relative) ← Immediate ---
  {"[AND byte ptr [rip+off32], imm8]",     "80 25 ? ? ? ? ?"},
  {"[AND dword ptr [rip+off32], imm32]",   "81 25 ? ? ? ? ? ? ? ?"},
  {"[AND qword ptr [rip+off32], imm64_32]", "4[8-f] 81 25 ? ? ? ? ? ? ? ?"},

  // --- AND: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[AND r8, [r64+sib+off32]]",    "22 [84-bc] ? ? ? ? ?"},
  {"[AND r8, [r64+sib+off8]]",     "22 [44-7c] ? ? ?"},
  {"[AND r8, [r64+sib]]",          "22 [04-3c] ?"},
  {"[AND r32, [r64+sib+off32]]",   "23 [84-bc] ? ? ? ? ?"},
  {"[AND r32, [r64+sib+off8]]",    "23 [44-7c] ? ? ?"},
  {"[AND r32, [r64+sib]]",         "23 [04-3c] ?"},
  {"[AND r64, [r64+sib+off32]]",   "4[8-f] 23 [84-bc] ? ? ? ? ?"},
  {"[AND r64, [r64+sib+off8]]",    "4[8-f] 23 [44-7c] ? ? ?"},
  {"[AND r64, [r64+sib]]",         "4[8-f] 23 [04-3c] ?"},

  // --- AND: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[AND [r64+sib+off32], r8]",    "20 [84-bc] ? ? ? ? ?"},
  {"[AND [r64+sib+off8], r8]",     "20 [44-7c] ? ? ?"},
  {"[AND [r64+sib], r8]",          "20 [04-3c] ?"},
  {"[AND [r64+sib+off32], r32]",   "21 [84-bc] ? ? ? ? ?"},
  {"[AND [r64+sib+off8], r32]",    "21 [44-7c] ? ? ?"},
  {"[AND [r64+sib], r32]",         "21 [04-3c] ?"},
  {"[AND [r64+sib+off32], r64]",   "4[8-f] 21 [84-bc] ? ? ? ? ?"},
  {"[AND [r64+sib+off8], r64]",    "4[8-f] 21 [44-7c] ? ? ?"},
  {"[AND [r64+sib], r64]",         "4[8-f] 21 [04-3c] ?"},

  // --- OR: Register ← Register ---
  {"[OR r64, r64]",              "4[8-f] [09|0b] [c0-ff]"},
  {"[OR r32, r32]",              "[09|0b] [c0-ff]"},
  {"[OR r8, r8]",                "[08|0a] [c0-ff]"},

  // --- OR: Register ← Immediate ---
  {"[OR r64, imm32]",            "4[8-f] 81 [c8-cf] ? ? ? ?"},
  {"[OR r32, imm32]",            "81 [c8-cf] ? ? ? ?"},
  {"[OR r8, imm8]",              "80 [c8-cf] ?"},
  {"[OR r64, imm8]",             "4[8-f] 83 [c8-cf] ?"},
  {"[OR r32, imm8]",             "83 [c8-cf] ?"},

  // --- OR: Register ← Memory (RIP-Relative) ---
  {"[OR r8, [rip+off32]]",       "0a 0d ? ? ? ?"},
  {"[OR r32, [rip+off32]]",      "0b 0d ? ? ? ?"},
  {"[OR r64, [rip+off32]]",      "4[8-f] 0b 0d ? ? ? ?"},

  // --- OR: Register ← Memory (r64 base + disp32) ---
  {"[OR r8, [r64+off32]]",       "0a [80-bf] [SIB?] ? ? ? ?"},
  {"[OR r32, [r64+off32]]",      "0b [80-bf] [SIB?] ? ? ? ?"},
  {"[OR r64, [r64+off32]]",      "4[8-f] 0b [80-bf] [SIB?] ? ? ? ?"},

  // --- OR: Register ← Memory (r64 base + disp8) ---
  {"[OR r8, [r64+off8]]",        "0a [40-7f] [SIB?] ?"},
  {"[OR r32, [r64+off8]]",       "0b [40-7f] [SIB?] ?"},
  {"[OR r64, [r64+off8]]",       "4[8-f] 0b [40-7f] [SIB?] ?"},

  // --- OR: Register ← Memory (r64 base, no displacement) ---
  {"[OR r8, [r64]]",             "0a [00-3f] [SIB?]"},
  {"[OR r32, [r64]]",            "0b [00-3f] [SIB?]"},
  {"[OR r64, [r64]]",            "4[8-f] 0b [00-3f] [SIB?]"},

  // --- OR: Memory (RIP-Relative) ← Register ---
  {"[OR [rip+off32], r8]",       "08 0d ? ? ? ?"},
  {"[OR [rip+off32], r32]",      "09 0d ? ? ? ?"},
  {"[OR [rip+off32], r64]",      "4[8-f] 09 0d ? ? ? ?"},

  // --- OR: Memory (r64 base + disp32) ← Register ---
  {"[OR [r64+off32], r8]",       "08 [88-8f] [SIB?] ? ? ? ?"},
  {"[OR [r64+off32], r32]",      "09 [88-8f] [SIB?] ? ? ? ?"},
  {"[OR [r64+off32], r64]",      "4[8-f] 09 [88-8f] [SIB?] ? ? ? ?"},

  // --- OR: Memory (r64 base + disp8) ← Register ---
  {"[OR [r64+off8], r8]",        "08 [48-4f] [SIB?] ?"},
  {"[OR [r64+off8], r32]",       "09 [48-4f] [SIB?] ?"},
  {"[OR [r64+off8], r64]",       "4[8-f] 09 [48-4f] [SIB?] ?"},

  // --- OR: Memory (r64 base, no displacement) ← Register ---
  {"[OR [r64], r8]",             "08 [08-0f] [SIB?]"},
  {"[OR [r64], r32]",            "09 [08-0f] [SIB?]"},
  {"[OR [r64], r64]",            "4[8-f] 09 [08-0f] [SIB?]"},

  // --- OR: Memory (r64 base + disp32) ← Immediate ---
  {"[OR byte ptr [r64+off32], imm8]",     "80 [88-8f] [SIB?] ? ? ? ? ?"},
  {"[OR dword ptr [r64+off32], imm32]",   "81 [88-8f] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[OR qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [88-8f] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- OR: Memory (r64 base + disp8) ← Immediate ---
  {"[OR byte ptr [r64+off8], imm8]",      "80 [48-4f] [SIB?] ? ?"},
  {"[OR dword ptr [r64+off8], imm32]",    "81 [48-4f] [SIB?] ? ? ? ? ?"},
  {"[OR qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [48-4f] [SIB?] ? ? ? ? ?"},

  // --- OR: Memory (RIP-Relative) ← Immediate ---
  {"[OR byte ptr [rip+off32], imm8]",     "80 0d ? ? ? ? ?"},
  {"[OR dword ptr [rip+off32], imm32]",   "81 0d ? ? ? ? ? ? ? ?"},
  {"[OR qword ptr [rip+off32], imm64_32]", "4[8-f] 81 0d ? ? ? ? ? ? ? ?"},

  // --- OR: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[OR r8, [r64+sib+off32]]",     "0a [84-bc] ? ? ? ? ?"},
  {"[OR r8, [r64+sib+off8]]",      "0a [44-7c] ? ? ?"},
  {"[OR r8, [r64+sib]]",           "0a [04-3c] ?"},
  {"[OR r32, [r64+sib+off32]]",    "0b [84-bc] ? ? ? ? ?"},
  {"[OR r32, [r64+sib+off8]]",     "0b [44-7c] ? ? ?"},
  {"[OR r32, [r64+sib]]",          "0b [04-3c] ?"},
  {"[OR r64, [r64+sib+off32]]",    "4[8-f] 0b [84-bc] ? ? ? ? ?"},
  {"[OR r64, [r64+sib+off8]]",     "4[8-f] 0b [44-7c] ? ? ?"},
  {"[OR r64, [r64+sib]]",          "4[8-f] 0b [04-3c] ?"},

  // --- OR: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[OR [r64+sib+off32], r8]",     "08 [84-bc] ? ? ? ? ?"},
  {"[OR [r64+sib+off8], r8]",      "08 [44-7c] ? ? ?"},
  {"[OR [r64+sib], r8]",           "08 [04-3c] ?"},
  {"[OR [r64+sib+off32], r32]",    "09 [84-bc] ? ? ? ? ?"},
  {"[OR [r64+sib+off8], r32]",     "09 [44-7c] ? ? ?"},
  {"[OR [r64+sib], r32]",          "09 [04-3c] ?"},
  {"[OR [r64+sib+off32], r64]",    "4[8-f] 09 [84-bc] ? ? ? ? ?"},
  {"[OR [r64+sib+off8], r64]",     "4[8-f] 09 [44-7c] ? ? ?"},
  {"[OR [r64+sib], r64]",          "4[8-f] 09 [04-3c] ?"},

  // --- INC / DEC Instructions ---
  {"[INC r64]",                    "4[8-f] ff [c0-c7]"},
  {"[INC r32]",                    "ff [c0-c7]"},
  {"[DEC r64]",                    "4[8-f] ff [c8-cf]"},
  {"[DEC r32]",                    "ff [c8-cf]"},

  // --- NOT Instructions ---
  {"[NOT r64]",                    "4[8-f] f7 [d0-d7]"},
  {"[NOT r32]",                    "f7 [d0-d7]"},

  // --- TEST Instructions ---
  {"[TEST r64, r64]",              "4[8-f] 85 [c0-ff]"},
  {"[TEST r32, r32]",              "85 [c0-ff]"},
  {"[TEST r8, r8]",                "84 [c0-ff]"},

  // --- SHIFT by Immediate (Group 2, opcode C1) ---
  {"[SHL r32, imm8]",            "c1 [e0-e7] ?"},
  {"[SHL r64, imm8]",            "4[8-f] c1 [e0-e7] ?"},
  {"[SHR r32, imm8]",            "c1 [e8-ef] ?"},
  {"[SHR r64, imm8]",            "4[8-f] c1 [e8-ef] ?"},
  {"[SAR r32, imm8]",            "c1 [f8-ff] ?"},
  {"[SAR r64, imm8]",            "4[8-f] c1 [f8-ff] ?"},

  // =========================================================================
  // ===							COMPARISONS (CMP)                        ===
  // =========================================================================

  // --- CMP: Register ← Register ---
  {"[CMP r64, r64]",             "4[8-f] 3[9|b] [c0-ff]"},
  {"[CMP r32, r32]",             "3[9|b] [c0-ff]"},
  {"[CMP r16, r16]",             "66 3[9|b] [c0-ff]"},
  {"[CMP r8, r8]",               "3[8|a] [c0-ff]"},

  // --- CMP: Register ← Immediate ---
  {"[CMP r64, imm32]",           "4[8-f] 81 [f8-ff] ? ? ? ?"},
  {"[CMP r32, imm32]",           "81 [f8-ff] ? ? ? ?"},
  {"[CMP r16, imm16]",           "66 81 [f8-ff] ? ?"},
  {"[CMP r8, imm8]",             "80 [f8-ff] ?"},
  {"[CMP r64, imm8]",            "4[8-f] 83 [f8-ff] ?"},
  {"[CMP r32, imm8]",            "83 [f8-ff] ?"},
  {"[CMP r16, imm8]",            "66 83 [f8-ff] ?"},

  // --- CMP: Register ← Memory (RIP-Relative) ---
  {"[CMP r8, [rip+off32]]",      "3a 3d ? ? ? ?"},
  {"[CMP r32, [rip+off32]]",     "3b 3d ? ? ? ?"},
  {"[CMP r64, [rip+off32]]",     "4[8-f] 3b 3d ? ? ? ?"},
  {"[CMP r16, [rip+off32]]",     "66 3b 3d ? ? ? ?"},

  // --- CMP: Register ← Memory (r64 base + disp32) ---
  {"[CMP r8, [r64+off32]]",      "3a [80-bf] [SIB?] ? ? ? ?"},
  {"[CMP r32, [r64+off32]]",     "3b [80-bf] [SIB?] ? ? ? ?"},
  {"[CMP r64, [r64+off32]]",     "4[8-f] 3b [80-bf] [SIB?] ? ? ? ?"},

  // --- CMP: Register ← Memory (r64 base + disp8) ---
  {"[CMP r8, [r64+off8]]",       "3a [40-7f] [SIB?] ?"},
  {"[CMP r32, [r64+off8]]",      "3b [40-7f] [SIB?] ?"},
  {"[CMP r64, [r64+off8]]",      "4[8-f] 3b [40-7f] [SIB?] ?"},

  // --- CMP: Register ← Memory (r64 base, no displacement) ---
  {"[CMP r8, [r64]]",            "3a [00-3f] [SIB?]"},
  {"[CMP r32, [r64]]",           "3b [00-3f] [SIB?]"},
  {"[CMP r64, [r64]]",           "4[8-f] 3b [00-3f] [SIB?]"},

  // --- CMP: Memory (RIP-Relative) ← Register ---
  {"[CMP [rip+off32], r8]",      "38 3d ? ? ? ?"},
  {"[CMP [rip+off32], r32]",     "39 3d ? ? ? ?"},
  {"[CMP [rip+off32], r64]",     "4[8-f] 39 3d ? ? ? ?"},
  {"[CMP [rip+off32], r16]",     "66 39 3d ? ? ? ?"},

  // --- CMP: Memory (r64 base + disp32) ← Register ---
  {"[CMP [r64+off32], r8]",      "38 [80-bf] [SIB?] ? ? ? ?"},
  {"[CMP [r64+off32], r32]",     "39 [80-bf] [SIB?] ? ? ? ?"},
  {"[CMP [r64+off32], r64]",     "4[8-f] 39 [80-bf] [SIB?] ? ? ? ?"},

  // --- CMP: Memory (r64 base + disp8) ← Register ---
  {"[CMP [r64+off8], r8]",       "38 [40-7f] [SIB?] ?"},
  {"[CMP [r64+off8], r32]",      "39 [40-7f] [SIB?] ?"},
  {"[CMP [r64+off8], r64]",      "4[8-f] 39 [40-7f] [SIB?] ?"},

  // --- CMP: Memory (r64 base, no displacement) ← Register ---
  {"[CMP [r64], r8]",            "38 [00-3f] [SIB?]"},
  {"[CMP [r64], r32]",           "39 [00-3f] [SIB?]"},
  {"[CMP [r64], r64]",           "4[8-f] 39 [00-3f] [SIB?]"},

  // --- CMP: Memory (r64 base + disp32) ← Immediate ---
  {"[CMP byte ptr [r64+off32], imm8]",     "80 [b8-bf] [SIB?] ? ? ? ? ?"},
  {"[CMP dword ptr [r64+off32], imm32]",   "81 [b8-bf] [SIB?] ? ? ? ? ? ? ? ?"},
  {"[CMP qword ptr [r64+off32], imm64_32]", "4[8-f] 81 [b8-bf] [SIB?] ? ? ? ? ? ? ? ?"},

  // --- CMP: Memory (r64 base + disp8) ← Immediate ---
  {"[CMP byte ptr [r64+off8], imm8]",      "80 [78-7f] [SIB?] ? ?"},
  {"[CMP dword ptr [r64+off8], imm32]",    "81 [78-7f] [SIB?] ? ? ? ? ?"},
  {"[CMP qword ptr [r64+off8], imm64_32]",  "4[8-f] 81 [78-7f] [SIB?] ? ? ? ? ?"},

  // --- CMP: Memory (RIP-Relative) ← Immediate ---
  {"[CMP byte ptr [rip+off32], imm8]",     "80 3d ? ? ? ? ?"},
  {"[CMP dword ptr [rip+off32], imm32]",   "81 3d ? ? ? ? ? ? ? ?"},
  {"[CMP qword ptr [rip+off32], imm64_32]", "4[8-f] 81 3d ? ? ? ? ? ? ? ?"},

  // --- CMP: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[CMP r8, [r64+sib+off32]]",    "3a [84-bc] ? ? ? ? ?"},
  {"[CMP r8, [r64+sib+off8]]",     "3a [44-7c] ? ? ?"},
  {"[CMP r8, [r64+sib]]",          "3a [04-3c] ?"},
  {"[CMP r32, [r64+sib+off32]]",   "3b [84-bc] ? ? ? ? ?"},
  {"[CMP r32, [r64+sib+off8]]",    "3b [44-7c] ? ? ?"},
  {"[CMP r32, [r64+sib]]",         "3b [04-3c] ?"},
  {"[CMP r64, [r64+sib+off32]]",   "4[8-f] 3b [84-bc] ? ? ? ? ?"},
  {"[CMP r64, [r64+sib+off8]]",    "4[8-f] 3b [44-7c] ? ? ?"},
  {"[CMP r64, [r64+sib]]",         "4[8-f] 3b [04-3c] ?"},

  // --- CMP: SIB-specific Store (ModRM R/M=100, memory ← register) ---
  {"[CMP [r64+sib+off32], r8]",    "38 [84-bc] ? ? ? ? ?"},
  {"[CMP [r64+sib+off8], r8]",     "38 [44-7c] ? ? ?"},
  {"[CMP [r64+sib], r8]",          "38 [04-3c] ?"},
  {"[CMP [r64+sib+off32], r32]",   "39 [84-bc] ? ? ? ? ?"},
  {"[CMP [r64+sib+off8], r32]",    "39 [44-7c] ? ? ?"},
  {"[CMP [r64+sib], r32]",         "39 [04-3c] ?"},
  {"[CMP [r64+sib+off32], r64]",   "4[8-f] 39 [84-bc] ? ? ? ? ?"},
  {"[CMP [r64+sib+off8], r64]",    "4[8-f] 39 [44-7c] ? ? ?"},
  {"[CMP [r64+sib], r64]",         "4[8-f] 39 [04-3c] ?"},

  // =========================================================================
  // ===						CONDITIONAL MOVES (CMOV)                     ===
  // =========================================================================

  // --- CMOV: Register ← Register (32-bit and 64-bit) ---
  {"[CMOVO r64, r64]",     "4[8-f] 0f 40 [c0-ff]"},
  {"[CMOVNO r64, r64]",    "4[8-f] 0f 41 [c0-ff]"},
  {"[CMOVB r64, r64]",     "4[8-f] 0f 42 [c0-ff]"},
  {"[CMOVNB r64, r64]",    "4[8-f] 0f 43 [c0-ff]"},
  {"[CMOVE r64, r64]",     "4[8-f] 0f 44 [c0-ff]"},
  {"[CMOVNE r64, r64]",    "4[8-f] 0f 45 [c0-ff]"},
  {"[CMOVBE r64, r64]",    "4[8-f] 0f 46 [c0-ff]"},
  {"[CMOVA r64, r64]",     "4[8-f] 0f 47 [c0-ff]"},
  {"[CMOVS r64, r64]",     "4[8-f] 0f 48 [c0-ff]"},
  {"[CMOVNS r64, r64]",    "4[8-f] 0f 49 [c0-ff]"},
  {"[CMOVP r64, r64]",     "4[8-f] 0f 4a [c0-ff]"},
  {"[CMOVNP r64, r64]",    "4[8-f] 0f 4b [c0-ff]"},
  {"[CMOVL r64, r64]",     "4[8-f] 0f 4c [c0-ff]"},
  {"[CMOVGE r64, r64]",    "4[8-f] 0f 4d [c0-ff]"},
  {"[CMOVLE r64, r64]",    "4[8-f] 0f 4e [c0-ff]"},
  {"[CMOVG r64, r64]",     "4[8-f] 0f 4f [c0-ff]"},

  {"[CMOVO r32, r32]",     "0f 40 [c0-ff]"},
  {"[CMOVNO r32, r32]",    "0f 41 [c0-ff]"},
  {"[CMOVB r32, r32]",     "0f 42 [c0-ff]"},
  {"[CMOVNB r32, r32]",    "0f 43 [c0-ff]"},
  {"[CMOVE r32, r32]",     "0f 44 [c0-ff]"},
  {"[CMOVNE r32, r32]",    "0f 45 [c0-ff]"},
  {"[CMOVBE r32, r32]",    "0f 46 [c0-ff]"},
  {"[CMOVA r32, r32]",     "0f 47 [c0-ff]"},
  {"[CMOVS r32, r32]",     "0f 48 [c0-ff]"},
  {"[CMOVNS r32, r32]",    "0f 49 [c0-ff]"},
  {"[CMOVP r32, r32]",     "0f 4a [c0-ff]"},
  {"[CMOVNP r32, r32]",    "0f 4b [c0-ff]"},
  {"[CMOVL r32, r32]",     "0f 4c [c0-ff]"},
  {"[CMOVGE r32, r32]",    "0f 4d [c0-ff]"},
  {"[CMOVLE r32, r32]",    "0f 4e [c0-ff]"},
  {"[CMOVG r32, r32]",     "0f 4f [c0-ff]"},

  // --- CMOV: Register ← Memory (generic addressing) ---
  {"[CMOVcc r64, [r64+off32]]",     "4[8-f] 0f 4[0-f] [80-bf] [SIB?] ? ? ? ?"},
  {"[CMOVcc r64, [r64+off8]]",      "4[8-f] 0f 4[0-f] [40-7f] [SIB?] ?"},
  {"[CMOVcc r64, [r64]]",           "4[8-f] 0f 4[0-f] [00-3f] [SIB?]"},
  {"[CMOVcc r64, [rip+off32]]",     "4[8-f] 0f 4[0-f] [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[CMOVcc r32, [r64+off32]]",     "0f 4[0-f] [80-bf] [SIB?] ? ? ? ?"},
  {"[CMOVcc r32, [r64+off8]]",      "0f 4[0-f] [40-7f] [SIB?] ?"},
  {"[CMOVcc r32, [r64]]",           "0f 4[0-f] [00-3f] [SIB?]"},
  {"[CMOVcc r32, [rip+off32]]",     "0f 4[0-f] [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},

  // --- CMOV: SIB-specific Load (ModRM R/M=100, register ← memory) ---
  {"[CMOVcc r64, [r64+sib+off32]]", "4[8-f] 0f 4[0-f] [84-bc] ? ? ? ? ?"},
  {"[CMOVcc r64, [r64+sib+off8]]",  "4[8-f] 0f 4[0-f] [44-7c] ? ? ?"},
  {"[CMOVcc r64, [r64+sib]]",       "4[8-f] 0f 4[0-f] [04-3c] ?"},
  {"[CMOVcc r32, [r64+sib+off32]]", "0f 4[0-f] [84-bc] ? ? ? ? ?"},
  {"[CMOVcc r32, [r64+sib+off8]]",  "0f 4[0-f] [44-7c] ? ? ?"},
  {"[CMOVcc r32, [r64+sib]]",       "0f 4[0-f] [04-3c] ?"},

  // =========================================================================
  // ===					CALL / RET / LEAVE / JMP / Jcc                  ===
  // =========================================================================

  // --- CALL ---
  {"[CALL rel32]",                "e8 ? ? ? ?"},
  {"[CALL r64]",                  "ff [d0-d7]"},
  {"[CALL r32]",                  "ff [d0-d7]"},
  {"[CALL [r64+off32]]",          "ff [90-97] [SIB?] ? ? ? ?"},
  {"[CALL [r64+off8]]",           "ff [50-57] [SIB?] ?"},
  {"[CALL [r64]]",                "ff [10-17] [SIB?]"},
  {"[CALL [rip+off32]]",          "ff 15 ? ? ? ?"},
  {"[CALL [rip+sib+off32]]",      "ff 14 25 ? ? ? ?"},

  // --- CALL: SIB-specific ---
  {"[CALL [r64+sib+off32]]",      "ff 94 ? ? ? ? ?"},
  {"[CALL [r64+sib+off8]]",       "ff 54 ? ? ?"},
  {"[CALL [r64+sib]]",            "ff 14 ? ?"},

  // --- RET / LEAVE ---
  {"[RET]",                       "c3"},
  {"[RET imm16]",                 "c2 ? ?"},
  {"[LEAVE]",                     "c9"},

  // --- JMP ---
  {"[JMP rel32]",                 "e9 ? ? ? ?"},
  {"[JMP rel8]",                  "eb ?"},
  {"[JMP r64]",                   "ff [e0-e7]"},
  {"[JMP r32]",                   "ff [e0-e7]"},
  {"[JMP [r64+off32]]",           "ff [a0-a7] [SIB?] ? ? ? ?"},
  {"[JMP [r64+off8]]",            "ff [60-67] [SIB?] ?"},
  {"[JMP [r64]]",                 "ff [20-27] [SIB?]"},
  {"[JMP [rip+off32]]",           "ff 25 ? ? ? ?"},
  {"[JMP [rip+sib+off32]]",       "ff 24 25 ? ? ? ?"},

  // --- JMP: SIB-specific ---
  {"[JMP [r64+sib+off32]]",       "ff a4 ? ? ? ? ?"},
  {"[JMP [r64+sib+off8]]",        "ff 64 ? ? ?"},
  {"[JMP [r64+sib]]",             "ff 24 ? ?"},

  // --- Jcc rel32 ---
  {"[JO rel32]",                  "0f 80 ? ? ? ?"},
  {"[JNO rel32]",                 "0f 81 ? ? ? ?"},
  {"[JB rel32]",                  "0f 82 ? ? ? ?"},
  {"[JAE rel32]",                 "0f 83 ? ? ? ?"},
  {"[JE rel32]",                  "0f 84 ? ? ? ?"},
  {"[JNE rel32]",                 "0f 85 ? ? ? ?"},
  {"[JBE rel32]",                 "0f 86 ? ? ? ?"},
  {"[JA rel32]",                  "0f 87 ? ? ? ?"},
  {"[JS rel32]",                  "0f 88 ? ? ? ?"},
  {"[JNS rel32]",                 "0f 89 ? ? ? ?"},
  {"[JP rel32]",                  "0f 8a ? ? ? ?"},
  {"[JNP rel32]",                 "0f 8b ? ? ? ?"},
  {"[JL rel32]",                  "0f 8c ? ? ? ?"},
  {"[JGE rel32]",                 "0f 8d ? ? ? ?"},
  {"[JLE rel32]",                 "0f 8e ? ? ? ?"},
  {"[JG rel32]",                  "0f 8f ? ? ? ?"},

  // --- Jcc rel8 ---
  {"[JO rel8]",                   "70 ?"},
  {"[JNO rel8]",                  "71 ?"},
  {"[JB rel8]",                   "72 ?"},
  {"[JAE rel8]",                  "73 ?"},
  {"[JE rel8]",                   "74 ?"},
  {"[JNE rel8]",                  "75 ?"},
  {"[JBE rel8]",                  "76 ?"},
  {"[JA rel8]",                   "77 ?"},
  {"[JS rel8]",                   "78 ?"},
  {"[JNS rel8]",                  "79 ?"},
  {"[JP rel8]",                   "7a ?"},
  {"[JNP rel8]",                  "7b ?"},
  {"[JL rel8]",                   "7c ?"},
  {"[JGE rel8]",                  "7d ?"},
  {"[JLE rel8]",                  "7e ?"},
  {"[JG rel8]",                   "7f ?"},

  // =========================================================================
  // ===						XMM / FLOATING-POINT                         ===
  // =========================================================================
  // --- MOVSS: Scalar Single-Precision (load) ---
  {"[MOVSS xmm, [rip+off32]]",     "f3 0f 10 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVSS xmm, [r64+off32]]",     "f3 0f 10 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSS xmm, [r64+off8]]",      "f3 0f 10 [40-7f] [SIB?] ?"},
  {"[MOVSS xmm, [r64]]",           "f3 0f 10 [00-3f] [SIB?]"},
  {"[MOVSS xmm, xmm]",             "f3 0f 10 [c0-ff]"},

  // --- MOVSS: Scalar Single-Precision (store) ---
  {"[MOVSS [rip+off32], xmm]",     "f3 0f 11 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVSS [r64+off32], xmm]",     "f3 0f 11 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSS [r64+off8], xmm]",      "f3 0f 11 [40-7f] [SIB?] ?"},
  {"[MOVSS [r64], xmm]",           "f3 0f 11 [00-3f] [SIB?]"},

  // --- MOVSD: Scalar Double-Precision (load) ---
  {"[MOVSD xmm, [rip+off32]]",     "f2 0f 10 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVSD xmm, [r64+off32]]",     "f2 0f 10 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSD xmm, [r64+off8]]",      "f2 0f 10 [40-7f] [SIB?] ?"},
  {"[MOVSD xmm, [r64]]",           "f2 0f 10 [00-3f] [SIB?]"},
  {"[MOVSD xmm, xmm]",             "f2 0f 10 [c0-ff]"},

  // --- MOVSD: Scalar Double-Precision (store) ---
  {"[MOVSD [rip+off32], xmm]",     "f2 0f 11 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVSD [r64+off32], xmm]",     "f2 0f 11 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVSD [r64+off8], xmm]",      "f2 0f 11 [40-7f] [SIB?] ?"},
  {"[MOVSD [r64], xmm]",           "f2 0f 11 [00-3f] [SIB?]"},

  // --- MOVSS/MOVSD: SIB-specific load ---
  {"[MOVSS xmm, [r64+sib+off32]]", "f3 0f 10 [84-bc] ? ? ? ? ?"},
  {"[MOVSS xmm, [r64+sib+off8]]",  "f3 0f 10 [44-7c] ? ? ?"},
  {"[MOVSS xmm, [r64+sib]]",       "f3 0f 10 [04-3c] ?"},
  {"[MOVSD xmm, [r64+sib+off32]]", "f2 0f 10 [84-bc] ? ? ? ? ?"},
  {"[MOVSD xmm, [r64+sib+off8]]",  "f2 0f 10 [44-7c] ? ? ?"},
  {"[MOVSD xmm, [r64+sib]]",       "f2 0f 10 [04-3c] ?"},

  // --- MOVSS/MOVSD: SIB-specific store ---
  {"[MOVSS [r64+sib+off32], xmm]", "f3 0f 11 [84-bc] ? ? ? ? ?"},
  {"[MOVSS [r64+sib+off8], xmm]",  "f3 0f 11 [44-7c] ? ? ?"},
  {"[MOVSS [r64+sib], xmm]",       "f3 0f 11 [04-3c] ?"},
  {"[MOVSD [r64+sib+off32], xmm]", "f2 0f 11 [84-bc] ? ? ? ? ?"},
  {"[MOVSD [r64+sib+off8], xmm]",  "f2 0f 11 [44-7c] ? ? ?"},
  {"[MOVSD [r64+sib], xmm]",       "f2 0f 11 [04-3c] ?"},

  // --- MOVUPS: Unaligned Packed Single (load) ---
  {"[MOVUPS xmm, [rip+off32]]",    "0f 10 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVUPS xmm, [r64+off32]]",    "0f 10 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVUPS xmm, [r64+off8]]",     "0f 10 [40-7f] [SIB?] ?"},
  {"[MOVUPS xmm, [r64]]",          "0f 10 [00-3f] [SIB?]"},

  // --- MOVUPS: Unaligned Packed Single (store) ---
  {"[MOVUPS [rip+off32], xmm]",    "0f 11 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVUPS [r64+off32], xmm]",    "0f 11 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVUPS [r64+off8], xmm]",     "0f 11 [40-7f] [SIB?] ?"},
  {"[MOVUPS [r64], xmm]",          "0f 11 [00-3f] [SIB?]"},

  // --- MOVUPD: Unaligned Packed Double (load) ---
  {"[MOVUPD xmm, [rip+off32]]",    "66 0f 10 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVUPD xmm, [r64+off32]]",    "66 0f 10 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVUPD xmm, [r64+off8]]",     "66 0f 10 [40-7f] [SIB?] ?"},
  {"[MOVUPD xmm, [r64]]",          "66 0f 10 [00-3f] [SIB?]"},

  // --- MOVUPD: Unaligned Packed Double (store) ---
  {"[MOVUPD [rip+off32], xmm]",    "66 0f 11 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVUPD [r64+off32], xmm]",    "66 0f 11 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVUPD [r64+off8], xmm]",     "66 0f 11 [40-7f] [SIB?] ?"},
  {"[MOVUPD [r64], xmm]",          "66 0f 11 [00-3f] [SIB?]"},

  // --- MOVAPS: Aligned Packed Single (load) ---
  {"[MOVAPS xmm, [rip+off32]]",    "0f 28 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVAPS xmm, [r64+off32]]",    "0f 28 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVAPS xmm, [r64+off8]]",     "0f 28 [40-7f] [SIB?] ?"},
  {"[MOVAPS xmm, [r64]]",          "0f 28 [00-3f] [SIB?]"},

  // --- MOVAPS: Aligned Packed Single (store) ---
  {"[MOVAPS [rip+off32], xmm]",    "0f 29 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVAPS [r64+off32], xmm]",    "0f 29 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVAPS [r64+off8], xmm]",     "0f 29 [40-7f] [SIB?] ?"},
  {"[MOVAPS [r64], xmm]",          "0f 29 [00-3f] [SIB?]"},

  // --- MOVAPS: SIB-specific ---
  {"[MOVAPS xmm, [r64+sib+off32]]",  "0f 28 [84-bc] ? ? ? ? ?"},
  {"[MOVAPS xmm, [r64+sib+off8]]",   "0f 28 [44-7c] ? ? ?"},
  {"[MOVAPS xmm, [r64+sib]]",        "0f 28 [04-3c] ?"},
  {"[MOVAPS [r64+sib+off32], xmm]",  "0f 29 [84-bc] ? ? ? ? ?"},
  {"[MOVAPS [r64+sib+off8], xmm]",   "0f 29 [44-7c] ? ? ?"},
  {"[MOVAPS [r64+sib], xmm]",        "0f 29 [04-3c] ?"},

  // --- MOVAPD: Aligned Packed Double (load) ---
  {"[MOVAPD xmm, [rip+off32]]",    "66 0f 28 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVAPD xmm, [r64+off32]]",    "66 0f 28 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVAPD xmm, [r64+off8]]",     "66 0f 28 [40-7f] [SIB?] ?"},
  {"[MOVAPD xmm, [r64]]",          "66 0f 28 [00-3f] [SIB?]"},

  // --- MOVAPD: Aligned Packed Double (store) ---
  {"[MOVAPD [rip+off32], xmm]",    "66 0f 29 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVAPD [r64+off32], xmm]",    "66 0f 29 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVAPD [r64+off8], xmm]",     "66 0f 29 [40-7f] [SIB?] ?"},
  {"[MOVAPD [r64], xmm]",          "66 0f 29 [00-3f] [SIB?]"},

  // --- MOVAPD: SIB-specific ---
  {"[MOVAPD xmm, [r64+sib+off32]]",  "66 0f 28 [84-bc] ? ? ? ? ?"},
  {"[MOVAPD xmm, [r64+sib+off8]]",   "66 0f 28 [44-7c] ? ? ?"},
  {"[MOVAPD xmm, [r64+sib]]",        "66 0f 28 [04-3c] ?"},
  {"[MOVAPD [r64+sib+off32], xmm]",  "66 0f 29 [84-bc] ? ? ? ? ?"},
  {"[MOVAPD [r64+sib+off8], xmm]",   "66 0f 29 [44-7c] ? ? ?"},
  {"[MOVAPD [r64+sib], xmm]",        "66 0f 29 [04-3c] ?"},

  // --- MOVDQA: Aligned Double Quadword (load) ---
  {"[MOVDQA xmm, [rip+off32]]",    "66 0f 6f [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVDQA xmm, [r64+off32]]",    "66 0f 6f [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVDQA xmm, [r64+off8]]",     "66 0f 6f [40-7f] [SIB?] ?"},
  {"[MOVDQA xmm, [r64]]",          "66 0f 6f [00-3f] [SIB?]"},

  // --- MOVDQA: Aligned Double Quadword (store) ---
  {"[MOVDQA [rip+off32], xmm]",    "66 0f 7f [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVDQA [r64+off32], xmm]",    "66 0f 7f [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVDQA [r64+off8], xmm]",     "66 0f 7f [40-7f] [SIB?] ?"},
  {"[MOVDQA [r64], xmm]",          "66 0f 7f [00-3f] [SIB?]"},

  // --- MOVDQA: SIB-specific ---
  {"[MOVDQA xmm, [r64+sib+off32]]",  "66 0f 6f [84-bc] ? ? ? ? ?"},
  {"[MOVDQA xmm, [r64+sib+off8]]",   "66 0f 6f [44-7c] ? ? ?"},
  {"[MOVDQA xmm, [r64+sib]]",        "66 0f 6f [04-3c] ?"},
  {"[MOVDQA [r64+sib+off32], xmm]",  "66 0f 7f [84-bc] ? ? ? ? ?"},
  {"[MOVDQA [r64+sib+off8], xmm]",   "66 0f 7f [44-7c] ? ? ?"},
  {"[MOVDQA [r64+sib], xmm]",        "66 0f 7f [04-3c] ?"},

  // --- MOVDQU: Unaligned Double Quadword (load) ---
  {"[MOVDQU xmm, [rip+off32]]",    "f3 0f 6f [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVDQU xmm, [r64+off32]]",    "f3 0f 6f [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVDQU xmm, [r64+off8]]",     "f3 0f 6f [40-7f] [SIB?] ?"},
  {"[MOVDQU xmm, [r64]]",          "f3 0f 6f [00-3f] [SIB?]"},

  // --- MOVDQU: Unaligned Double Quadword (store) ---
  {"[MOVDQU [rip+off32], xmm]",    "f3 0f 7f [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVDQU [r64+off32], xmm]",    "f3 0f 7f [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVDQU [r64+off8], xmm]",     "f3 0f 7f [40-7f] [SIB?] ?"},
  {"[MOVDQU [r64], xmm]",          "f3 0f 7f [00-3f] [SIB?]"},

  // --- MOVDQU: SIB-specific ---
  {"[MOVDQU xmm, [r64+sib+off32]]",  "f3 0f 6f [84-bc] ? ? ? ? ?"},
  {"[MOVDQU xmm, [r64+sib+off8]]",   "f3 0f 6f [44-7c] ? ? ?"},
  {"[MOVDQU xmm, [r64+sib]]",        "f3 0f 6f [04-3c] ?"},
  {"[MOVDQU [r64+sib+off32], xmm]",  "f3 0f 7f [84-bc] ? ? ? ? ?"},
  {"[MOVDQU [r64+sib+off8], xmm]",   "f3 0f 7f [44-7c] ? ? ?"},
  {"[MOVDQU [r64+sib], xmm]",        "f3 0f 7f [04-3c] ?"},

  // --- MOVD: Doubleword between GP and XMM ---
  {"[MOVD xmm, r32]",               "66 0f 6e [c0-ff]"},
  {"[MOVD xmm, [r64+off32]]",       "66 0f 6e [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVD xmm, [r64+off8]]",        "66 0f 6e [40-7f] [SIB?] ?"},
  {"[MOVD xmm, [r64]]",             "66 0f 6e [00-3f] [SIB?]"},
  {"[MOVD r32, xmm]",               "66 0f 7e [c0-ff]"},
  {"[MOVD [r64+off32], xmm]",       "66 0f 7e [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVD [r64+off8], xmm]",        "66 0f 7e [40-7f] [SIB?] ?"},
  {"[MOVD [r64], xmm]",             "66 0f 7e [00-3f] [SIB?]"},

  // --- MOVQ: Quadword between GP and XMM (REX.W MOVD encoding) ---
  {"[MOVQ xmm, r64]",               "66 4[8-f] 0f 6e [c0-ff]"},
  {"[MOVQ xmm, [r64+off8]]",        "66 4[8-f] 0f 6e [40-7f] [SIB?] ?"},
  {"[MOVQ xmm, [r64+off32]]",       "66 4[8-f] 0f 6e [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVQ xmm, [r64]]",             "66 4[8-f] 0f 6e [00-3f] [SIB?]"},
  {"[MOVQ r64, xmm]",               "66 4[8-f] 0f 7e [c0-ff]"},
  {"[MOVQ [r64+off8], xmm]",        "66 4[8-f] 0f 7e [40-7f] [SIB?] ?"},
  {"[MOVQ [r64+off32], xmm]",       "66 4[8-f] 0f 7e [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVQ [r64], xmm]",             "66 4[8-f] 0f 7e [00-3f] [SIB?]"},

  // --- MOVQ: Lower 64-bit XMM transfer ---
  {"[MOVQ xmm, xmm]",               "f3 0f 7e [c0-ff]"},
  {"[MOVQ xmm, [r64+off8]]",        "f3 0f 7e [40-7f] [SIB?] ?"},
  {"[MOVQ xmm, [r64+off32]]",       "f3 0f 7e [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVQ xmm, [r64]]",             "f3 0f 7e [00-3f] [SIB?]"},
  {"[MOVQ xmm, [rip+off32]]",       "f3 0f 7e [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[MOVQ [r64+off8], xmm]",        "66 0f d6 [40-7f] [SIB?] ?"},
  {"[MOVQ [r64+off32], xmm]",       "66 0f d6 [80-bf] [SIB?] ? ? ? ?"},
  {"[MOVQ [r64], xmm]",             "66 0f d6 [00-3f] [SIB?]"},

  // --- XORPS / XORPD ---
  {"[XORPS xmm, xmm]",             "0f 57 [c0-ff]"},
  {"[XORPS xmm, [r64+off32]]",     "0f 57 [80-bf] [SIB?] ? ? ? ?"},
  {"[XORPS xmm, [r64+off8]]",      "0f 57 [40-7f] [SIB?] ?"},
  {"[XORPS xmm, [r64]]",           "0f 57 [00-3f] [SIB?]"},
  {"[XORPS xmm, [rip+off32]]",     "0f 57 [05|0d|15|1d|25|2d|35|3d] ? ? ? ?"},
  {"[XORPD xmm, xmm]",             "66 0f 57 [c0-ff]"},

  // --- Scalar Arithmetic (Register ← Register) ---
  {"[ADDSS xmm, xmm]",             "f3 0f 58 [c0-ff]"},
  {"[ADDSD xmm, xmm]",             "f2 0f 58 [c0-ff]"},
  {"[SUBSS xmm, xmm]",             "f3 0f 5c [c0-ff]"},
  {"[SUBSD xmm, xmm]",             "f2 0f 5c [c0-ff]"},
  {"[MULSS xmm, xmm]",             "f3 0f 59 [c0-ff]"},
  {"[MULSD xmm, xmm]",             "f2 0f 59 [c0-ff]"},
  {"[DIVSS xmm, xmm]",             "f3 0f 5e [c0-ff]"},
  {"[DIVSD xmm, xmm]",             "f2 0f 5e [c0-ff]"},
  {"[SQRTSS xmm, xmm]",            "f3 0f 51 [c0-ff]"},
  {"[SQRTSD xmm, xmm]",            "f2 0f 51 [c0-ff]"},

  // --- Scalar Arithmetic (Register ← Memory) ---
  {"[ADDSS xmm, [r64+off32]]",     "f3 0f 58 [80-bf] [SIB?] ? ? ? ?"},
  {"[ADDSD xmm, [r64+off32]]",     "f2 0f 58 [80-bf] [SIB?] ? ? ? ?"},
  {"[MULSS xmm, [r64+off32]]",     "f3 0f 59 [80-bf] [SIB?] ? ? ? ?"},
  {"[MULSD xmm, [r64+off32]]",     "f2 0f 59 [80-bf] [SIB?] ? ? ? ?"},

  // --- Comparisons ---
  {"[COMISS xmm, xmm]",            "0f 2f [c0-ff]"},
  {"[COMISD xmm, xmm]",            "66 0f 2f [c0-ff]"},
  {"[UCOMISS xmm, xmm]",           "0f 2e [c0-ff]"},
  {"[UCOMISD xmm, xmm]",           "66 0f 2e [c0-ff]"},

  // --- Conversions: Integer → Float ---
  {"[CVTSI2SS xmm, r32]",          "f3 0f 2a [c0-ff]"},
  {"[CVTSI2SS xmm, r64]",          "f3 4[8-f] 0f 2a [c0-ff]"},
  {"[CVTSI2SD xmm, r32]",          "f2 0f 2a [c0-ff]"},
  {"[CVTSI2SD xmm, r64]",          "f2 4[8-f] 0f 2a [c0-ff]"},

  // --- Conversions: Float → Integer ---
  {"[CVTSS2SI r32, xmm]",          "f3 0f 2d [c0-ff]"},
  {"[CVTSS2SI r64, xmm]",          "f3 4[8-f] 0f 2d [c0-ff]"},
  {"[CVTSD2SI r32, xmm]",          "f2 0f 2d [c0-ff]"},
  {"[CVTSD2SI r64, xmm]",          "f2 4[8-f] 0f 2d [c0-ff]"},

  // --- Conversions: Truncated Float → Integer ---
  {"[CVTTSS2SI r32, xmm]",         "f3 0f 2c [c0-ff]"},
  {"[CVTTSS2SI r64, xmm]",         "f3 4[8-f] 0f 2c [c0-ff]"},
  {"[CVTTSD2SI r32, xmm]",         "f2 0f 2c [c0-ff]"},
  {"[CVTTSD2SI r64, xmm]",         "f2 4[8-f] 0f 2c [c0-ff]"},

  // =========================================================================
  // ===					STACK & MISC SYSTEM OPERATIONS                   ===
  // =========================================================================

  // --- PUSH (Register) ---
  {"[PUSH r64]",                    "[50-57]"},
  {"[PUSH R8-R15]",                 "41 [50-57]"},

  // --- PUSH (Immediate) ---
  {"[PUSH imm32]",                  "68 ? ? ? ?"},
  {"[PUSH imm8]",                   "6a ?"},

  // --- PUSH (Memory) ---
  {"[PUSH [r64+off8]]",             "ff [70-77] [SIB?] ?"},
  {"[PUSH [r64+off32]]",            "ff [b0-b7] [SIB?] ? ? ? ?"},
  {"[PUSH [r64]]",                  "ff [30-37] [SIB?]"},
  {"[PUSH [rip+off32]]",            "ff 35 ? ? ? ?"},

  // --- POP (Register) ---
  {"[POP r64]",                     "[58-5f]"},
  {"[POP R8-R15]",                  "41 [58-5f]"},

  // --- POP (Memory) ---
  {"[POP [r64+off8]]",              "8f [40-47] [SIB?] ?"},
  {"[POP [r64+off32]]",             "8f [80-87] [SIB?] ? ? ? ?"},
  {"[POP [r64]]",                   "8f [00-07] [SIB?]"},

  // --- INC / DEC (Memory) ---
  {"[INC dword ptr [r64+off8]]",    "ff [40-47] [SIB?] ?"},
  {"[INC dword ptr [r64+off32]]",   "ff [80-87] [SIB?] ? ? ? ?"},
  {"[INC dword ptr [r64]]",         "ff [00-07] [SIB?]"},
  {"[INC qword ptr [r64+off8]]",    "4[8-f] ff [40-47] [SIB?] ?"},
  {"[INC qword ptr [r64+off32]]",   "4[8-f] ff [80-87] [SIB?] ? ? ? ?"},
  {"[INC qword ptr [r64]]",         "4[8-f] ff [00-07] [SIB?]"},
  {"[DEC dword ptr [r64+off8]]",    "ff [48-4f] [SIB?] ?"},
  {"[DEC dword ptr [r64+off32]]",   "ff [88-8f] [SIB?] ? ? ? ?"},
  {"[DEC dword ptr [r64]]",         "ff [08-0f] [SIB?]"},
  {"[DEC qword ptr [r64+off8]]",    "4[8-f] ff [48-4f] [SIB?] ?"},
  {"[DEC qword ptr [r64+off32]]",   "4[8-f] ff [88-8f] [SIB?] ? ? ? ?"},
  {"[DEC qword ptr [r64]]",         "4[8-f] ff [08-0f] [SIB?]"},

  // --- NOT / NEG (Memory) ---
  {"[NOT dword ptr [r64+off8]]",    "f7 [50-57] [SIB?] ?"},
  {"[NOT dword ptr [r64+off32]]",   "f7 [90-97] [SIB?] ? ? ? ?"},
  {"[NOT dword ptr [r64]]",         "f7 [10-17] [SIB?]"},
  {"[NOT qword ptr [r64+off8]]",    "4[8-f] f7 [50-57] [SIB?] ?"},
  {"[NOT qword ptr [r64+off32]]",   "4[8-f] f7 [90-97] [SIB?] ? ? ? ?"},
  {"[NOT qword ptr [r64]]",         "4[8-f] f7 [10-17] [SIB?]"},
  {"[NEG dword ptr [r64+off8]]",    "f7 [58-5f] [SIB?] ?"},
  {"[NEG dword ptr [r64+off32]]",   "f7 [98-9f] [SIB?] ? ? ? ?"},
  {"[NEG dword ptr [r64]]",         "f7 [18-1f] [SIB?]"},
  {"[NEG qword ptr [r64+off8]]",    "4[8-f] f7 [58-5f] [SIB?] ?"},
  {"[NEG qword ptr [r64+off32]]",   "4[8-f] f7 [98-9f] [SIB?] ? ? ? ?"},
  {"[NEG qword ptr [r64]]",         "4[8-f] f7 [18-1f] [SIB?]"},

  // --- NOP / Multi-byte NOP ---
  {"[NOP]",                         "90"},
  {"[NOP 3-byte]",                  "0f 1f 00"},
  {"[NOP 4-byte]",                  "0f 1f 40 00"},
  {"[NOP 5-byte]",                  "0f 1f 44 00 00"},
  {"[NOP 6-byte]",                  "66 0f 1f 44 00 00"},
  {"[NOP 7-byte]",                  "0f 1f 80 00 00 00 00"},
  {"[NOP 8-byte]",                  "0f 1f 84 00 00 00 00 00"},
  {"[NOP 9-byte]",                  "66 0f 1f 84 00 00 00 00 00"},
};

// Helper that replaces template placeholders in a signature string.
// Templates are sorted by key length (longer first) so that more specific
// names (e.g. [MOV r64, [r64+off32]]) are matched before shorter ones
// (e.g. [MOV r64, [r64]]).
inline std::string ReplaceTemplates(std::string signature) {
  std::vector<std::pair<std::string, std::string>> sorted(
      kPatternTemplates.begin(), kPatternTemplates.end());
  std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });

  for (const auto& [name, replacement] : sorted) {
    size_t pos = 0;
    while ((pos = signature.find(name, pos)) != std::string::npos) {
      signature.replace(pos, name.length(), replacement);
      pos += replacement.length();
    }
  }
  return signature;
}

}  // namespace Utils
SPF_NS_END
