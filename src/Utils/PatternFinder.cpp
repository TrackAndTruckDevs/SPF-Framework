#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>
#include <Psapi.h>
#include <vector>
#include <string>
#include <sstream>

SPF_NS_BEGIN
namespace Utils {
uintptr_t PatternFinder::Find(const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty()) {
    return 0;
  }
  return Find(nullptr, signatureVec);
}

uintptr_t PatternFinder::Find(uintptr_t base, size_t size, const char* signature) {
  auto signatureVec = SignatureToVector(signature);
  if (signatureVec.empty() || base == 0 || size == 0) {
    return 0;
  }

  size_t sigSize = signatureVec.size();
  const int* sigBytes = signatureVec.data();

  // The core scanning loop, modeled after the working module scanner
  for (uintptr_t i = 0; i < size - sigSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < sigSize; ++j) {
      if (sigBytes[j] != -1 && *(unsigned char*)(base + i + j) != sigBytes[j]) {
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

std::vector<int> PatternFinder::SignatureToVector(const std::string& signature) {
  std::vector<int> bytes;
  std::stringstream ss(signature);
  std::string byteStr;
  while (ss >> byteStr) {
    if (byteStr == "?" || byteStr == "??") {
      bytes.push_back(-1);  // Wildcard
    } else {
      bytes.push_back(std::stoi(byteStr, nullptr, 16));
    }
  }
  return bytes;
}

uintptr_t PatternFinder::Find(const char* moduleName, const std::vector<int>& signature) {
  MODULEINFO moduleInfo = {0};
  HMODULE hModule = GetModuleHandleA(moduleName);
  if (!hModule || !GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
    return 0;
  }
  uintptr_t base = (uintptr_t)moduleInfo.lpBaseOfDll;
  uintptr_t size = (uintptr_t)moduleInfo.SizeOfImage;
  size_t sigSize = signature.size();
  const int* sigBytes = signature.data();
  for (uintptr_t i = 0; i < size - sigSize; ++i) {
    bool found = true;
    for (size_t j = 0; j < sigSize; ++j) {
      if (sigBytes[j] != -1 && *(unsigned char*)(base + i + j) != sigBytes[j]) {
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
