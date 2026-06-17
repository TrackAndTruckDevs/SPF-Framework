#define NOMINMAX
#include "SPF/Utils/PatternFinder.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <regex>
#include <algorithm>

SPF_NS_BEGIN
namespace Utils {

// ===========================================================================
// PRIVATE HELPERS
// ===========================================================================

namespace {

/**
 * @brief High-performance scanner using SIMD-accelerated memchr for initial matches.
 * @param skipCode If true, skips executable sections.
 */
static std::vector<uintptr_t> FindAllRawInternal(const char* moduleName, const uint8_t* data, size_t len, bool skipCode) {
  std::vector<uintptr_t> results;
  if (len == 0) return results;

  auto sections = PatternFinder::GetModuleSections(moduleName);
  uint8_t firstByte = data[0];

  for (const auto& sec : sections) {
    if (skipCode && (sec.name == ".text" || sec.name == "INIT")) continue;
    if (sec.name == ".pdata") continue;
    if (sec.size < len) continue;

    const uint8_t* start = reinterpret_cast<const uint8_t*>(sec.base);
    const uint8_t* end = start + sec.size;
    const uint8_t* curr = start;

    // Fast SIMD-accelerated search loop
    while (curr <= end - len) {
      // Find the next occurrence of the first byte
      const uint8_t* found = static_cast<const uint8_t*>(memchr(curr, firstByte, (end - len) - curr + 1));
      if (!found) break;

      // Verify the full sequence
      if (len == 1 || memcmp(found, data, len) == 0) {
        results.push_back(reinterpret_cast<uintptr_t>(found));
      }
      curr = found + 1;
    }
  }
  return results;
}

/**
 * @brief Resolves a pointer chain with depth protection.
 */
static bool PointerLeadsToString(uintptr_t addr, const char* substring, int maxDepth = 3) {
  if (addr < 0x10000 || maxDepth <= 0) return false;
  try {
    const char* str = reinterpret_cast<const char*>(addr);
    if (str[0] >= 0x20 && str[0] <= 0x7E && strstr(str, substring)) return true;
    return PointerLeadsToString(*reinterpret_cast<uintptr_t*>(addr), substring, maxDepth - 1);
  } catch (...) { return false; }
}

} // anonymous namespace

// ===========================================================================
// SINGLETON ACCESS
// ===========================================================================

PatternFinder& PatternFinder::GetInstance() {
  static PatternFinder instance;
  return instance;
}

// ===========================================================================
// SECTION 1: Pattern Matching Core
// ===========================================================================

uintptr_t PatternFinder::Find(const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty()) return 0;
  uintptr_t result = Find(nullptr, signatureVec);
  if (!result) {
    Logging::LoggerFactory::GetInstance().GetLogger("PatternFinder")->Error("Find: Signature not found: '{}'", signature);
  }
  return result;
}

uintptr_t PatternFinder::Find(uintptr_t base, size_t size, const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty() || base == 0 || size == 0) return 0;

  // Follow JMP thunk if present at the start address
  uint8_t* pBase = reinterpret_cast<uint8_t*>(base);
  if (pBase[0] == 0xE9) { // JMP rel32
    uintptr_t target = GetRipAddress(base, 1, 5);
    if (target) {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PatternFinder");
        logger->Info("Find: Following JMP thunk from 0x{:X} to 0x{:X}", base, target);
        base = target;
    }
  }

  size_t minLen = 0;
  for (const auto& m : signatureVec) minLen += m.minCount;
  if (size < minLen) return 0;

  const uint8_t* data = reinterpret_cast<const uint8_t*>(base);
  const auto& first = signatureVec[0];

  for (uintptr_t i = 0; i <= size - minLen; ++i) {
    // Optimization: Skip bytes that don't match the first matcher
    if (first.type == ByteMatcher::EXACT) {
        const uint8_t* found = static_cast<const uint8_t*>(memchr(data + i, first.min, size - minLen - i + 1));
        if (!found) break;
        i = reinterpret_cast<uintptr_t>(found) - reinterpret_cast<uintptr_t>(data);
    } else if (!first.Matches(data[i])) {
        continue;
    }

    size_t matchLen = 0;
    if (MatchInternal(data + i, size - i, signatureVec, 0, 0, matchLen)) return base + i;
  }
  return 0;
}

uintptr_t PatternFinder::FindBackward(uintptr_t startAddress, size_t searchRange, const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty() || startAddress == 0 || searchRange == 0) return 0;

  size_t patternLen = 0;
  for (const auto& m : signatureVec) patternLen += m.minCount;

  /*
   * BACKWARD SEARCH LOGIC:
   * We iterate through memory by moving the START point of our comparison window backwards.
   * However, for each start point, we perform a standard FORWARD match of the signature.
   * 
   * Example: Signature "48 8D" (LEA)
   * Memory: [48] [8D] [0D] [AA] [BB] <--- startAddress
   * 1. Start at [BB]: Does not match "48 8D"
   * 2. Move to [AA]: Does not match
   * 3. ...
   * 4. Move to [48]: [48][8D] matches! Result = address of [48].
   */
  for (uintptr_t i = 0; i < searchRange; ++i) {
    uintptr_t currentStart = startAddress - i;
    size_t matchLen = 0;
    if (MatchInternal(reinterpret_cast<const uint8_t*>(currentStart), searchRange - i, signatureVec, 0, 0, matchLen)) {
      return currentStart;
    }
  }

  return 0;
}

uintptr_t PatternFinder::Find(uintptr_t base, size_t size, const unsigned char* signature, size_t signatureSize) {
  if (!base || !size || !signature || !signatureSize || size < signatureSize) return 0;
  
  const uint8_t* data = reinterpret_cast<const uint8_t*>(base);
  uint8_t firstByte = signature[0];

  for (uintptr_t i = 0; i <= size - signatureSize; ) {
    const uint8_t* found = static_cast<const uint8_t*>(memchr(data + i, firstByte, size - signatureSize - i + 1));
    if (!found) break;

    uintptr_t foundIdx = reinterpret_cast<uintptr_t>(found) - base;
    bool match = true;
    for (size_t j = 1; j < signatureSize; ++j) {
      if (signature[j] != '?' && found[j] != signature[j]) { match = false; break; }
    }
    if (match) return reinterpret_cast<uintptr_t>(found);
    i = foundIdx + 1;
  }
  return 0;
}

uintptr_t PatternFinder::FindChain(const std::vector<std::string>& signatures, size_t maxGap, uintptr_t startAddress, size_t searchRange) {
  if (signatures.empty()) return 0;
  std::vector<std::vector<ByteMatcher>> compiledSigs;
  for (const auto& sig : signatures) compiledSigs.push_back(SignatureToVector(sig));

  auto sections = GetModuleSections(nullptr);
  if (sections.empty()) return 0;

  uintptr_t moduleBase = sections.front().base;
  uintptr_t moduleEnd = sections.back().base + sections.back().size;
  uintptr_t searchBase = (startAddress == 0) ? moduleBase : startAddress;
  uintptr_t searchLimit = (startAddress != 0) ? (startAddress + (searchRange > 0 ? searchRange : 4096)) : moduleEnd;
  if (searchLimit > moduleEnd) searchLimit = moduleEnd;

  size_t firstMinLen = 0;
  for (const auto& m : compiledSigs[0]) firstMinLen += m.minCount;
  if (searchLimit < searchBase + firstMinLen) return 0;

  for (uintptr_t i = searchBase; i <= searchLimit - firstMinLen; ++i) {
    size_t matchLen = 0;
    if (MatchInternal(reinterpret_cast<const uint8_t*>(i), searchLimit - i, compiledSigs[0], 0, 0, matchLen)) {
        if (compiledSigs.size() == 1) return i;
        if (FindChainRecursive(compiledSigs, 1, i + matchLen, maxGap, searchLimit)) return i;
    }
  }
  return 0;
}

// ===========================================================================
// SECTION 2: SCS Reflection Support
// ===========================================================================

uintptr_t PatternFinder::FindAttributeOffset(const char* className, const char* attributeName) {
  auto& instance = GetInstance();
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PatternFinder");
  
  if (instance.m_reflectionCache.count(className)) {
    auto& classMap = instance.m_reflectionCache[className];
    if (classMap.count(attributeName)) return classMap[attributeName];
    // Optimization: If the class descriptor was already harvested but attribute is missing,
    // it's likely not in the table. Skip Module scan.
  }

  // Find attribute name string (with null terminator for higher accuracy and harvesting speed)
  std::vector<uintptr_t> attrStrAddrs;
  if (instance.m_stringCache.count(attributeName)) {
    attrStrAddrs = instance.m_stringCache[attributeName];
  } else {
    // For reflection table strings, we search WITH null terminator (+1) to avoid false partial matches
    attrStrAddrs = FindAllRawInternal(nullptr, (const uint8_t*)attributeName, strlen(attributeName) + 1, true);
    instance.m_stringCache[attributeName] = attrStrAddrs;
  }

  if (attrStrAddrs.empty()) return 0;

  for (uintptr_t attrStrAddr : attrStrAddrs) {
    std::vector<uintptr_t> allAttrXrefs = FindDataPointers(attrStrAddr);
    for (uintptr_t xref : allAttrXrefs) {
      /* [xref-24]:Offset, [xref-16]:TypeID, [xref]:NamePtr, [xref+8]:OwnerPtr */
      uintptr_t entryOwner = *reinterpret_cast<uintptr_t*>(xref + 8);
      if (PointerLeadsToString(entryOwner, className)) {
        logger->Info("FindAttributeOffset: Harvesting class '{}'...", className);
        std::vector<uintptr_t> allClassAttrs = FindDataPointers(entryOwner);
        auto& classMap = instance.m_reflectionCache[className];
        
        // Get data sections once for faster validation
        auto sections = GetModuleSections(nullptr);
        auto isInDataSection = [&](uintptr_t addr) {
            for (const auto& s : sections) {
                if (s.name == ".data" || s.name == ".rdata" || s.name == "DATA") {
                    if (addr >= s.base && addr < s.base + s.size) return true;
                }
            }
            return false;
        };

        for (uintptr_t pOwnerPtr : allClassAttrs) {
            // Hard Validation:
            // 1. Must be 8-byte aligned (pointers in reflection table always are)
            if (pOwnerPtr % 8 != 0) continue;
            // 2. Must be in a valid data section to avoid hitting code or stack
            if (!isInDataSection(pOwnerPtr)) continue;

            uintptr_t pNamePtr = pOwnerPtr - 8;
            
            // 3. Safety check for the pointer address itself
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<void*>(pNamePtr), &mbi, sizeof(mbi)) == 0) continue;
            if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) continue;

            uintptr_t nameAddr = *reinterpret_cast<uintptr_t*>(pNamePtr);
            if (!nameAddr || nameAddr < 0x10000 || nameAddr > 0x00007FFFFFFFFFFF) continue;
            
            try {
              const char* fName = reinterpret_cast<const char*>(nameAddr);
              
              // 4. Double check if nameAddr is readable
              if (VirtualQuery(fName, &mbi, sizeof(mbi)) == 0) continue;
              if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))) continue;

              // Safe check for printable string
              if (fName[0] >= 0x20 && fName[0] <= 0x7E) {
                int32_t off = ReadInt32(pNamePtr - 24);
                if (off == 0) off = static_cast<int32_t>(GetSizeFromTypeId(*reinterpret_cast<uint64_t*>(pNamePtr - 16)));
                
                if (IsSaneOffset(off)) {
                   std::string attrName;
                   for(int k=0; k<128; ++k) {
                       if (fName[k] == 0) break;
                       if (fName[k] < 0x20 || fName[k] > 0x7E) { attrName.clear(); break; }
                       attrName += fName[k];
                   }
                   if (!attrName.empty()) classMap[attrName] = static_cast<uintptr_t>(off);
                }
              }
            } catch (...) {}
        }
        if (classMap.count(attributeName)) return classMap[attributeName];
        int32_t finalOff = ReadInt32(xref - 24);
        if (IsSaneOffset(finalOff)) return static_cast<uintptr_t>(finalOff);
      }
    }
  }
  return 0;
}

size_t PatternFinder::GetSizeFromTypeId(uint64_t typeId) {
    static const std::unordered_map<uint64_t, size_t> kTypeSizeMap = {{0x05,4},{0x09,12},{0x0A,8},{0x39,4},{0x3E,8},{0x3B,8}};
    auto it = kTypeSizeMap.find(typeId);
    return (it != kTypeSizeMap.end()) ? it->second : 0;
}

// ===========================================================================
// SECTION 3: Reverse Engineering Tools
// ===========================================================================

uintptr_t PatternFinder::GetFunctionStart(uintptr_t address) {
  if (address == 0) return 0;
  DWORD64 imageBase = 0;
  PRUNTIME_FUNCTION funcEntry = RtlLookupFunctionEntry(static_cast<DWORD64>(address), &imageBase, nullptr);
  uintptr_t candidate = (funcEntry && imageBase) ? static_cast<uintptr_t>(imageBase + funcEntry->BeginAddress) : address;
  for (uintptr_t addr = candidate; addr > candidate - 0x2000; --addr) {
    if (addr <= 1) break;
    uint8_t* p = reinterpret_cast<uint8_t*>(addr);
    if (((p[-1] == 0xCC || p[-1] == 0x90) && (p[-2] == 0xCC || p[-2] == 0x90)) ||
        ((p[0] == 0x48 && p[1] == 0x8B && p[2] == 0xC4) || (p[0] == 0x40 && p[1] == 0x53))) {
      if (addr % 8 == 0) return addr;
    }
  }
  return candidate;
}

uintptr_t PatternFinder::GetFunctionEnd(uintptr_t address) {
  if (address == 0) return 0;
  DWORD64 imageBase = 0;
  PRUNTIME_FUNCTION funcEntry = RtlLookupFunctionEntry(static_cast<DWORD64>(address), &imageBase, nullptr);
  if (funcEntry && imageBase) {
    return static_cast<uintptr_t>(imageBase + funcEntry->EndAddress);
  }
  return 0;
}
std::vector<uintptr_t> PatternFinder::FindXrefs(uintptr_t targetAddr, const char* moduleName) {
  std::vector<uintptr_t> xrefs;
  auto sections = GetModuleSections(moduleName);
  for (const auto& sec : sections) {
    if (sec.size < 7 || sec.name == ".pdata") continue;
    for (uintptr_t i = 0; i <= sec.size - 7; ++i) {
      uint8_t* p = (uint8_t*)(sec.base + i);
      if ((p[0] >= 0x48 && p[0] <= 0x4F) && (p[1] == 0x8D || p[1] == 0x8B) && (p[2] & 0x07) == 0x05) {
        if (GetRipAddress(sec.base + i, 3, 7) == targetAddr) xrefs.push_back(sec.base + i);
      }
    }
  }
  return xrefs;
}

std::vector<uintptr_t> PatternFinder::FindDataPointers(uintptr_t targetAddr, const char* moduleName) {
  auto& instance = GetInstance();
  if (instance.m_pointerCache.count(targetAddr)) return instance.m_pointerCache[targetAddr];
  std::vector<uintptr_t> pointers;
  auto sections = GetModuleSections(moduleName);
  for (const auto& sec : sections) {
    if (sec.name == ".text" || sec.name == "INIT" || sec.name == ".pdata") continue;
    if (sec.size < 8) continue;
    for (uintptr_t i = 0; i <= sec.size - 8; i += 8) {
      if (*reinterpret_cast<uintptr_t*>(sec.base + i) == targetAddr) pointers.push_back(sec.base + i);
    }
  }
  instance.m_pointerCache[targetAddr] = pointers;
  return pointers;
}

uintptr_t PatternFinder::FindVTable(const char* signature, int offsetPos, int instructionSize) {
  uintptr_t instrAddr = Find(signature);
  return instrAddr ? GetRipAddress(instrAddr, offsetPos, instructionSize) : 0;
}

uintptr_t PatternFinder::GetVTableFunction(uintptr_t vtableAddr, int index) {
  return vtableAddr ? *reinterpret_cast<uintptr_t*>(vtableAddr + (index * 8)) : 0;
}

uintptr_t PatternFinder::FindFunctionByConstant(uint32_t constant, bool findStart) {
  auto sections = GetModuleSections(nullptr);
  for (const auto& sec : sections) {
    if (sec.size < 4 || sec.name == ".pdata") continue;
    for (uintptr_t i = 0; i <= sec.size - 4; ++i) {
      if (*reinterpret_cast<uint32_t*>(sec.base + i) == constant) {
          auto xrefs = FindXrefs(sec.base + i);
          if (!xrefs.empty()) return findStart ? GetFunctionStart(xrefs[0]) : xrefs[0];
      }
    }
  }
  return 0;
}

uintptr_t PatternFinder::FindFunctionByString(const char* str, bool findStart, const char* contextSig, size_t contextRange) {
  if (!str) return 0;
  
  // Find ALL occurrences of the string in memory
  auto stringAddrs = FindAllRawInternal(nullptr, (const uint8_t*)str, strlen(str), false);
  if (stringAddrs.empty()) return 0;

  std::vector<uintptr_t> allXrefs;
  for (uintptr_t sAddr : stringAddrs) {
    // Get direct XREFs (LEA/MOV [REG], [RIP+offset])
    std::vector<uintptr_t> xrefs = FindXrefs(sAddr);
    allXrefs.insert(allXrefs.end(), xrefs.begin(), xrefs.end());
    
    // Get indirect pointers (data pointers to this string)
    std::vector<uintptr_t> ptrs = FindDataPointers(sAddr);
    for (uintptr_t ptrAddr : ptrs) {
        auto indirect = FindXrefs(ptrAddr);
        allXrefs.insert(allXrefs.end(), indirect.begin(), indirect.end());
    }
  }

  if (allXrefs.empty()) return 0;

  for (uintptr_t xref : allXrefs) {
    if (!contextSig || (Find(xref - contextRange, contextRange * 2, contextSig) != 0)) {
        return findStart ? GetFunctionStart(xref) : xref;
    }
  }
  return 0;
}

// ===========================================================================
// SECTION 4: Safe Memory Access & Utilities
// ===========================================================================

int32_t PatternFinder::ReadInt32(uintptr_t address) { return address ? *reinterpret_cast<int32_t*>(address) : 0; }
int8_t  PatternFinder::ReadInt8(uintptr_t address)  { return address ? *reinterpret_cast<int8_t*>(address) : 0; }
int64_t PatternFinder::ReadInt64(uintptr_t address) { return address ? *reinterpret_cast<int64_t*>(address) : 0; }
float   PatternFinder::ReadFloat(uintptr_t address)  { return address ? *reinterpret_cast<float*>(address) : 0.0f; }
double  PatternFinder::ReadDouble(uintptr_t address) { return address ? *reinterpret_cast<double*>(address) : 0.0; }

uintptr_t PatternFinder::GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize) {
  if (!instructionAddr) return 0;
  return instructionAddr + instructionSize + ReadInt32(instructionAddr + offsetPos);
}

bool PatternFinder::IsSaneOffset(int32_t offset) { return offset > 0 && offset < 0x6000; }

uintptr_t PatternFinder::FindString(const char* str, const char* moduleName) {
  if (!str) return 0;
  size_t len = strlen(str);
  // Scan ALL sections for generic strings for reliability, but use high-performance memchr
  auto results = FindAllRawInternal(moduleName, (const uint8_t*)str, len, false);
  if (results.empty()) return 0;

  // Prioritize exact (null-terminated) matches to avoid substring false positives (e.g. "abc" inside "abc.sii")
  auto sections = GetModuleSections(moduleName);
  for (uintptr_t addr : results) {
    for (const auto& sec : sections) {
      if (addr >= sec.base && addr + len < sec.base + sec.size) {
        if (reinterpret_cast<const char*>(addr)[len] == '\0') return addr;
        break; // Found the section, move to next result
      }
    }
  }
  
  // Fallback to the first occurrence if no null-terminated match is found
  return results[0];
}

std::vector<PatternFinder::MemorySection> PatternFinder::GetModuleSections(const char* moduleName) {
  auto& instance = GetInstance();
  std::string key = moduleName ? moduleName : "";
  if (instance.m_sectionCache.count(key)) return instance.m_sectionCache[key];
  std::vector<MemorySection> sections;
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule) return sections;
  auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
  auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<uintptr_t>(hModule) + dos->e_lfanew);
  auto section = IMAGE_FIRST_SECTION(nt);
  for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section) {
    MemorySection s;
    s.base = reinterpret_cast<uintptr_t>(hModule) + section->VirtualAddress;
    s.size = section->Misc.VirtualSize;
    s.name = std::string(reinterpret_cast<const char*>(section->Name), 8);
    size_t nullPos = s.name.find('\0');
    if (nullPos != std::string::npos) s.name.erase(nullPos);
    sections.push_back(s);
  }
  instance.m_sectionCache[key] = sections;
  return sections;
}

// ===========================================================================
// INTERNAL MATCHING ENGINE
// ===========================================================================

uintptr_t PatternFinder::FindChainRecursive(const std::vector<std::vector<ByteMatcher>>& compiledSigs, size_t sigIdx, uintptr_t currentBase, size_t maxGap, uintptr_t searchLimit) {
    if (sigIdx == compiledSigs.size()) return 1;
    uintptr_t end = std::min(currentBase + maxGap, searchLimit);
    size_t minLen = 0;
    for (const auto& m : compiledSigs[sigIdx]) minLen += m.minCount;
    if (end < currentBase + minLen) return 0;
    for (uintptr_t i = currentBase; i <= end - minLen; ++i) {
        size_t matchLen = 0;
        if (MatchInternal(reinterpret_cast<const uint8_t*>(i), searchLimit - i, compiledSigs[sigIdx], 0, 0, matchLen)) {
            if (sigIdx == compiledSigs.size() - 1 || FindChainRecursive(compiledSigs, sigIdx + 1, i + matchLen, maxGap, searchLimit)) return i;
        }
    }
    return 0;
}

bool PatternFinder::MatchInternal(const uint8_t* data, size_t dataSize, const std::vector<ByteMatcher>& matchers, size_t dataIdx, size_t matcherIdx, size_t& matchLen) {
  // Base case: all matchers satisfied
  if (matcherIdx == matchers.size()) { 
    matchLen = dataIdx; 
    return true; 
  }

  const auto& m = matchers[matcherIdx];
  
  // Quick bound check for current matcher
  if (dataIdx + m.minCount > dataSize) return false;

  // Handle standard matchers (EXACT, RANGE, fixed WILDCARD)
  if (m.minCount == m.maxCount) {
    for (int i = 0; i < m.minCount; ++i) {
      if (!m.Matches(data[dataIdx + i])) return false;
    }
    return MatchInternal(data, dataSize, matchers, dataIdx + m.minCount, matcherIdx + 1, matchLen);
  }

  // Handle variable wildcards with backtracking (e.g. [0-4?])
  for (int count = m.minCount; count <= m.maxCount; ++count) {
    if (dataIdx + count > dataSize) break;
    
    // We already know wildcard matches any byte, so we just recurse
    if (MatchInternal(data, dataSize, matchers, dataIdx + count, matcherIdx + 1, matchLen)) {
      return true;
    }
  }

  return false;
}

std::vector<PatternFinder::ByteMatcher> PatternFinder::SignatureToVector(const std::string& signature) {
  std::vector<ByteMatcher> matchers;
  std::stringstream ss(signature);
  std::string part;
  static const std::regex rangeRegex(R"(\[([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})\])");
  static const std::regex countRegex(R"(\[([0-9]+)-([0-9]+)\?\])");
  
  while (ss >> part) {
    if (part == "?" || part == "??") {
      ByteMatcher bm;
      bm.type = ByteMatcher::WILDCARD;
      bm.min = 0;
      bm.max = 0;
      bm.minCount = 1;
      bm.maxCount = 1;
      matchers.push_back(bm);
    } else {
      std::smatch m;
      if (std::regex_match(part, m, countRegex)) {
        ByteMatcher bm;
        bm.type = ByteMatcher::WILDCARD;
        bm.min = 0;
        bm.max = 0;
        bm.minCount = std::stoi(m[1].str());
        bm.maxCount = std::stoi(m[2].str());
        matchers.push_back(bm);
      } else if (std::regex_match(part, m, rangeRegex)) {
        ByteMatcher bm;
        bm.type = ByteMatcher::RANGE;
        bm.min = (uint8_t)std::stoi(m[1].str(), nullptr, 16);
        bm.max = (uint8_t)std::stoi(m[2].str(), nullptr, 16);
        bm.minCount = 1;
        bm.maxCount = 1;
        matchers.push_back(bm);
      } else {
        ByteMatcher bm;
        bm.type = ByteMatcher::EXACT;
        uint8_t val = (uint8_t)std::stoi(part, nullptr, 16);
        bm.min = val;
        bm.max = val;
        bm.minCount = 1;
        bm.maxCount = 1;
        matchers.push_back(bm);
      }
    }
  }
  return matchers;
}

uintptr_t PatternFinder::Find(const char* moduleName, const std::vector<ByteMatcher>& signature) {
  auto sections = GetModuleSections(moduleName);
  size_t minLen = 0;
  for (const auto& m : signature) minLen += m.minCount;
  
  const auto& first = signature[0];

  for (const auto& sec : sections) {
    if (sec.size < minLen) continue;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(sec.base);

    for (uintptr_t i = 0; i <= sec.size - minLen; ++i) {
      if (first.type == ByteMatcher::EXACT) {
          const uint8_t* found = static_cast<const uint8_t*>(memchr(data + i, first.min, sec.size - minLen - i + 1));
          if (!found) break;
          i = reinterpret_cast<uintptr_t>(found) - reinterpret_cast<uintptr_t>(data);
      } else if (!first.Matches(data[i])) {
          continue;
      }

      size_t matchLen = 0;
      if (MatchInternal(data + i, sec.size - i, signature, 0, 0, matchLen)) return sec.base + i;
    }
  }
  return 0;
}

}  // namespace Utils
SPF_NS_END
