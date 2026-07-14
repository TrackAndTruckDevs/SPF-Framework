#include "SPF/Utils/HashUtils.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Utils/Windows.hpp"  // IWYU pragma: keep

#include <bcrypt.h>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <minwindef.h>
#include <sstream>
#include <string>
#include <vector>


#pragma comment(lib, "bcrypt.lib")

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

SPF_NS_BEGIN

namespace Utils {

std::string HashUtils::CalculateFileMD5(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) return "";

  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) return "";

  BCRYPT_ALG_HANDLE hAlg = NULL;
  BCRYPT_HASH_HANDLE hHash = NULL;
  NTSTATUS status = 0;
  DWORD cbData = 0, cbHash = 0, cbHashObject = 0;
  std::vector<BYTE> hashObject;
  std::vector<BYTE> hash;

  // Open an algorithm handle
  status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, NULL, 0);
  if (!NT_SUCCESS(status)) return "";

  // Calculate the size of the buffer to hold the hash object
  status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0);
  if (!NT_SUCCESS(status)) {
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return "";
  }

  hashObject.resize(cbHashObject);

  // Calculate the length of the hash
  status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0);
  if (!NT_SUCCESS(status)) {
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return "";
  }

  hash.resize(cbHash);

  // Create a hash
  status = BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, NULL, 0, 0);
  if (!NT_SUCCESS(status)) {
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return "";
  }

  // Read file in chunks and update hash
  std::vector<char> buffer(65536);  // 64KB chunks
  while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
    status = BCryptHashData(hHash, (PBYTE)buffer.data(), (ULONG)file.gcount(), 0);
    if (!NT_SUCCESS(status)) break;
  }

  if (NT_SUCCESS(status)) {
    status = BCryptFinishHash(hHash, hash.data(), cbHash, 0);
  }

  // Cleanup
  if (hHash) BCryptDestroyHash(hHash);
  if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);

  if (!NT_SUCCESS(status)) return "";

  // Convert to hex string
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (BYTE b : hash) {
    ss << std::setw(2) << (int)b;
  }

  return ss.str();
}

}  // namespace Utils

SPF_NS_END
