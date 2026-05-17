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

  size_t sigSize = signatureVec.size();
  const ByteMatcher* matchers = signatureVec.data();

  for (uintptr_t i = 0; i < size - sigSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < sigSize; ++j) {
      if (!matchers[j].Matches(*(unsigned char*)(base + i + j))) {
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
 * Handles exact bytes (XX), wildcards (??, ?), and ranges ([XX-YY]).
 */
std::vector<PatternFinder::ByteMatcher> PatternFinder::SignatureToVector(const std::string& signature) {
  std::vector<ByteMatcher> matchers;
  std::stringstream ss(signature);
  std::string part;

  // Regex to match range: [XX-YY] where XX and YY are hex bytes
  static const std::regex rangeRegex(R"(\[([0-9A-Fa-f]{1,2})-([0-9A-Fa-f]{1,2})\])");

  while (ss >> part) {
    if (part == "?" || part == "??") {
      matchers.push_back({ByteMatcher::WILDCARD, 0, 0});
    } else {
      std::smatch match;
      if (std::regex_match(part, match, rangeRegex)) {
        uint8_t min = static_cast<uint8_t>(std::stoi(match[1].str(), nullptr, 16));
        uint8_t max = static_cast<uint8_t>(std::stoi(match[2].str(), nullptr, 16));
        matchers.push_back({ByteMatcher::RANGE, min, max});
      } else {
        // Exact byte
        try {
          uint8_t val = static_cast<uint8_t>(std::stoi(part, nullptr, 16));
          matchers.push_back({ByteMatcher::EXACT, val, val});
        } catch (...) {
          // Handle invalid hex parts if necessary (skip or log)
        }
      }
    }
  }
  return matchers;
}

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
  size_t sigSize = signature.size();
  const ByteMatcher* matchers = signature.data();

  for (uintptr_t i = 0; i < size - sigSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < sigSize; ++j) {
      if (!matchers[j].Matches(*(unsigned char*)(base + i + j))) {
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

}  // namespace Utils
SPF_NS_END
