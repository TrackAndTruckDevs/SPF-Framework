#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <sstream>
#include <regex>

SPF_NS_BEGIN
namespace Utils {

/**
 * @brief Public entry point for scanning the entire game module.
 */
uintptr_t PatternFinder::Find(const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty()) {
    return 0;
  }
  return Find(nullptr, signatureVec);
}

/**
 * @brief Public entry point for scanning a specific memory range with a string signature.
 */
uintptr_t PatternFinder::Find(uintptr_t base, size_t size, const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty() || base == 0 || size == 0) {
    return 0;
  }

  size_t minPatternLen = 0;
  for (const auto& m : signatureVec) minPatternLen += m.minCount;

  for (uintptr_t i = 0; i < size - minPatternLen; ++i) {
    size_t dummyMatchLen = 0;
    if (MatchInternal(reinterpret_cast<uint8_t*>(base + i), signatureVec, 0, 0, dummyMatchLen)) {
      return base + i;
    }
  }
  return 0;
}

/**
 * @brief Public entry point for scanning a specific memory range with a raw byte array.
 * Note: This version supports simple wildcards via '?' character in the byte array if needed,
 * but primarily used for exact byte sequences.
 */
uintptr_t PatternFinder::Find(uintptr_t base, size_t size, const unsigned char* signature, size_t signatureSize) {
  if (!base || !size || !signature || !signatureSize) {
    return 0;
  }

  for (uintptr_t i = 0; i < size - signatureSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < signatureSize; ++j) {
      if (signature[j] != '?' && *(unsigned char*)(base + i + j) != signature[j]) {
        found = false;
        break;
      }
    }
    if (found) {
      return base + i;
    }
  }
  return 0;
}

uintptr_t PatternFinder::GetFunctionStart(uintptr_t address) {
  if (address == 0) return 0;

  // 1. Try official Windows .pdata tables first
  DWORD64 imageBase = 0;
  PRUNTIME_FUNCTION funcEntry = RtlLookupFunctionEntry(static_cast<DWORD64>(address), &imageBase, nullptr);

  uintptr_t candidate = 0;
  if (funcEntry && imageBase) {
    candidate = static_cast<uintptr_t>(imageBase + funcEntry->BeginAddress);
  } else {
    // If no .pdata entry, start backtracking from the current address
    candidate = address;
  }

  // 2. Backtrack to find the REAL start (prologue)
  // Compilers always separate functions with 'CC' (INT3) or '90' (NOP) padding.
  // We look for at least 2 padding bytes or a standard function alignment (16-byte).
  for (uintptr_t addr = candidate; addr > candidate - 0x2000; --addr) {
    if (addr <= 1) break;
    uint8_t* p = reinterpret_cast<uint8_t*>(addr);

    // If the previous bytes are padding, then 'addr' is likely the start of the function
    bool hasPaddingBefore = (p[-1] == 0xCC && p[-2] == 0xCC) || (p[-1] == 0x90 && p[-2] == 0x90);
    
    // Check if we hit a standard prologue pattern as well
    bool isPrologue = (p[0] == 0x48 && p[1] == 0x8B && p[2] == 0xC4) || // MOV RAX, RSP
                      (p[0] == 0x40 && p[1] == 0x53) ||                // PUSH RBX
                      (p[0] == 0x48 && p[1] == 0x83 && p[2] == 0xEC) || // SUB RSP, imm8
                      (p[0] == 0x48 && p[1] == 0x81 && p[2] == 0xEC);   // SUB RSP, imm32

    if (hasPaddingBefore || (isPrologue && (addr % 8 == 0))) {
      // Ensure the start is properly aligned (functions are at least 8-byte aligned)
      if (addr % 8 == 0) return addr;
      // If not 8-byte aligned, keep going back until we hit alignment or more padding
    }
  }

  return candidate;
}

uintptr_t PatternFinder::FindVTable(const char* signature, int offsetPos, int instructionSize) {
  uintptr_t instrAddr = Find(signature);
  if (!instrAddr) return 0;

  return GetRipAddress(instrAddr, offsetPos, instructionSize);
}

uintptr_t PatternFinder::GetVTableFunction(uintptr_t vtableAddr, int index) {
  if (!vtableAddr) return 0;
  return *reinterpret_cast<uintptr_t*>(vtableAddr + (index * 8));
}

uintptr_t PatternFinder::FindFunctionByConstant(uint32_t constant, bool findStart) {
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(nullptr);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return 0;
  }

  uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;

  // 1. Find the constant in the module's data sections
  uintptr_t constantAddr = 0;
  for (uintptr_t i = 0; i < size - 4; ++i) {
    if (*reinterpret_cast<uint32_t*>(base + i) == constant) {
        constantAddr = base + i;
        
        // 2. Find Xrefs to this constant
        auto xrefs = FindXrefs(constantAddr);
        if (!xrefs.empty()) {
            uintptr_t firstXref = xrefs[0];
            if (!findStart) return firstXref;
            return GetFunctionStart(firstXref);
        }
    }
  }

  return 0;
}

// Helper for recursive chain validation with backtracking
uintptr_t PatternFinder::FindChainRecursive(
    const std::vector<std::vector<ByteMatcher>>& compiledSigs,
    size_t sigIdx,
    uintptr_t currentBase,
    size_t maxGap,
    uintptr_t searchLimit) 
{
    // If we've matched all signatures, success!
    if (sigIdx == compiledSigs.size()) return 1;

    // The current pattern must start within 'maxGap' bytes from currentBase
    uintptr_t end = currentBase + maxGap;
    if (end > searchLimit) end = searchLimit;

    // Calculate minimum length of the current pattern
    size_t minLen = 0;
    for (const auto& m : compiledSigs[sigIdx]) minLen += m.minCount;

    for (uintptr_t i = currentBase; i <= end - minLen; ++i) {
        size_t matchLen = 0;
        if (MatchInternal(reinterpret_cast<uint8_t*>(i), compiledSigs[sigIdx], 0, 0, matchLen)) {
            // If this is the last signature in the chain
            if (sigIdx == compiledSigs.size() - 1) return i;

            // Try to find the next link starting AFTER this one
            if (FindChainRecursive(compiledSigs, sigIdx + 1, i + matchLen, maxGap, searchLimit)) {
                return i;
            }
        }
    }
    return 0;
}

uintptr_t PatternFinder::FindChain(const std::vector<std::string>& signatures, size_t maxGap, uintptr_t startAddress, size_t searchRange) {
  if (signatures.empty()) return 0;

  // 1. Pre-compile all signatures
  std::vector<std::vector<ByteMatcher>> compiledSigs;
  for (const auto& sig : signatures) {
    compiledSigs.push_back(SignatureToVector(sig));
  }

  // 2. Determine search boundaries
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(nullptr);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return 0;
  }

  uintptr_t moduleBase = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t moduleSize = (uintptr_t)moduleInfo.SizeOfImage;
  uintptr_t searchBase = (startAddress == 0) ? moduleBase : startAddress;
  
  // 1. Determine search limit: prioritized manual range, then default 4KB, then full module
  uintptr_t searchLimit;
  if (startAddress != 0) {
      searchLimit = startAddress + (searchRange > 0 ? searchRange : 4096);
  } else {
      searchLimit = moduleBase + moduleSize;
  }

  // 2. Clamp limit to module boundaries to avoid access violation
  if (searchLimit > moduleBase + moduleSize) searchLimit = moduleBase + moduleSize;

  // 3. Find candidates for the FIRST signature across the ENTIRE search range
  size_t firstMinLen = 0;
  for (const auto& m : compiledSigs[0]) firstMinLen += m.minCount;

  for (uintptr_t i = searchBase; i <= searchLimit - firstMinLen; ++i) {
    size_t matchLen = 0;
    if (MatchInternal(reinterpret_cast<uint8_t*>(i), compiledSigs[0], 0, 0, matchLen)) {
        // If there's only one signature, we are done
        if (compiledSigs.size() == 1) return i;

        // Otherwise, validate the rest of the chain with backtracking
        if (FindChainRecursive(compiledSigs, 1, i + matchLen, maxGap, searchLimit)) {
            return i;
        }
    }
  }

  return 0;
}

int32_t PatternFinder::ReadInt32(uintptr_t address) {
  if (address == 0) return 0;
  return *reinterpret_cast<int32_t*>(address);
}

int8_t PatternFinder::ReadInt8(uintptr_t address) {
  if (address == 0) return 0;
  return *reinterpret_cast<int8_t*>(address);
}

int64_t PatternFinder::ReadInt64(uintptr_t address) {
  if (address == 0) return 0;
  return *reinterpret_cast<int64_t*>(address);
}

float PatternFinder::ReadFloat(uintptr_t address) {
  if (address == 0) return 0.0f;
  return *reinterpret_cast<float*>(address);
}

double PatternFinder::ReadDouble(uintptr_t address) {
  if (address == 0) return 0.0;
  return *reinterpret_cast<double*>(address);
}

uintptr_t PatternFinder::GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize) {
  if (instructionAddr == 0) return 0;
  int32_t displacement = ReadInt32(instructionAddr + offsetPos);
  return instructionAddr + instructionSize + displacement;
}

bool PatternFinder::IsSaneOffset(int32_t offset) {
  return offset > 0 && offset < 0x6000;
}

/**
 * @brief Parses a signature string into a vector of ByteMatchers.
 * Handles exact bytes (XX), wildcards (??, ?), ranges ([XX-YY]), and variable wildcards ([min-max?]).
 */
std::vector<PatternFinder::ByteMatcher> PatternFinder::SignatureToVector(const std::string& signature) {
  std::vector<ByteMatcher> matchers;
  std::stringstream ss(signature);
  std::string part;

  // Regex to match value range: [XX-YY] where XX and YY are hex bytes
  static const std::regex rangeRegex(R"(\[([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})\])");
  // Regex to match count range: [min-max?] where min and max are decimal numbers
  static const std::regex countWildcardRegex(R"(\[([0-9]+)-([0-9]+)\?\])");

  while (ss >> part) {
    if (part == "?" || part == "??") {
      matchers.push_back({ByteMatcher::WILDCARD, 0, 0, 1, 1});
    } else {
      std::smatch match;
      if (std::regex_match(part, match, countWildcardRegex)) {
        int minCount = std::stoi(match[1].str());
        int maxCount = std::stoi(match[2].str());
        matchers.push_back({ByteMatcher::WILDCARD, 0, 0, minCount, maxCount});
      } else if (std::regex_match(part, match, rangeRegex)) {
        uint8_t min = static_cast<uint8_t>(std::stoi(match[1].str(), nullptr, 16));
        uint8_t max = static_cast<uint8_t>(std::stoi(match[2].str(), nullptr, 16));
        matchers.push_back({ByteMatcher::RANGE, min, max, 1, 1});
      } else {
        // Exact byte
        try {
          uint8_t val = static_cast<uint8_t>(std::stoi(part, nullptr, 16));
          matchers.push_back({ByteMatcher::EXACT, val, val, 1, 1});
        } catch (...) {
          // Handle invalid hex parts if necessary (skip or log)
        }
      }
    }
  }
  return matchers;
}

/**
 * @brief Internal helper to match a pattern starting at a specific address using backtracking.
 */
bool PatternFinder::MatchInternal(const uint8_t* data, const std::vector<ByteMatcher>& matchers, size_t dataIdx, size_t matcherIdx, size_t& matchLen) {
  if (matcherIdx == matchers.size()) {
    matchLen = dataIdx;
    return true;
  }

  const auto& matcher = matchers[matcherIdx];
  
  // Try all possible counts from minCount to maxCount
  for (int count = matcher.minCount; count <= matcher.maxCount; ++count) {
    bool match = true;
    for (int i = 0; i < count; ++i) {
      if (!matcher.Matches(data[dataIdx + i])) {
        match = false;
        break;
      }
    }

    if (match) {
      // Recursively match the rest of the pattern
      if (MatchInternal(data, matchers, dataIdx + count, matcherIdx + 1, matchLen)) {
        return true;
      }
    }

    // Optimization: if minCount == maxCount, no need to try other counts
    if (matcher.minCount == matcher.maxCount) break;
  }

  return false;
}

/**
 * @brief Internal core scanning function that targets a specific module.
 */
/**
 * @brief Internal core scanning function that targets a specific module.
 */
uintptr_t PatternFinder::Find(const char* moduleName, const std::vector<ByteMatcher>& signature) {
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return 0;
  }

  uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;
  
  size_t minPatternLen = 0;
  for (const auto& m : signature) minPatternLen += m.minCount;

  for (uintptr_t i = 0; i < size - minPatternLen; ++i) {
    size_t dummyMatchLen = 0;
    if (MatchInternal(reinterpret_cast<uint8_t*>(base + i), signature, 0, 0, dummyMatchLen)) {
      return base + i;
    }
  }
  return 0;
}

/**
 * @brief Actual raw byte search for strings.
 */
static uintptr_t FindRaw(const char* moduleName, const uint8_t* data, size_t len) {
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return 0;
  }
  uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;
  for (uintptr_t i = 0; i < size - len; ++i) {
    if (memcmp((void*)(base + i), data, len) == 0) return base + i;
  }
  return 0;
}

uintptr_t PatternFinder::FindString(const char* str, const char* moduleName) {
  if (!str) return 0;
  return FindRaw(moduleName, (const uint8_t*)str, strlen(str));
}

std::vector<uintptr_t> PatternFinder::FindXrefs(uintptr_t targetAddr, const char* moduleName) {
  std::vector<uintptr_t> xrefs;
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return xrefs;
  }

  uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;

  for (uintptr_t i = 0; i < size - 7; ++i) {
    uint8_t* p = (uint8_t*)(base + i);
    // Check for LEA/MOV reg, [rip + offset]
    // REX prefix for 64-bit: 0x48 to 0x4F
    // Opcode: 0x8D (LEA) or 0x8B (MOV)
    // ModR/M: 0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D (any reg, mod=00, rm=101)
    if ((p[0] >= 0x48 && p[0] <= 0x4F) && (p[1] == 0x8D || p[1] == 0x8B) && (p[2] & 0x07) == 0x05) {
      uintptr_t resolved = GetRipAddress(base + i, 3, 7);
      if (resolved == targetAddr) {
        xrefs.push_back(base + i);
      }
    }
  }
  return xrefs;
}

uintptr_t PatternFinder::FindFunctionByString(const char* str, bool findStart, const char* contextSig, size_t contextRange) {
  if (!str) return 0;

  // Find ALL occurrences of the string
  std::vector<uintptr_t> strAddrs;
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(nullptr);
  if (hModule && GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
    uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;
    size_t len = strlen(str);
    for (uintptr_t i = 0; i < size - len; ++i) {
        if (memcmp((void*)(base + i), str, len) == 0) strAddrs.push_back(base + i);
    }
  }

  if (strAddrs.empty()) return 0;

  std::vector<uintptr_t> allXrefs;
  for (uintptr_t sAddr : strAddrs) {
    auto xrefs = FindXrefs(sAddr);
    allXrefs.insert(allXrefs.end(), xrefs.begin(), xrefs.end());
  }

  if (allXrefs.empty()) return 0;

  for (uintptr_t xref : allXrefs) {
    bool contextMatch = true;
    if (contextSig) {
        contextMatch = (Find(xref - contextRange, contextRange * 2, contextSig) != 0);
    }

    if (contextMatch) {
        if (!findStart) return xref;
        return GetFunctionStart(xref);
    }
  }
  return 0;
}

}  // namespace Utils
SPF_NS_END
