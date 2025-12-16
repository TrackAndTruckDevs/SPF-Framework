#pragma once

#include "SPF/Namespace.hpp"
#include <stdint.h>  // For uintptr_t
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace Utils {
/**
 * @brief A utility class for finding byte patterns in memory.
 */
class PatternFinder {
 public:
  /**
   * @brief Finds a byte pattern in the game's memory.
   *
   * @param signature A string representing the byte pattern (e.g., "48 89 5C 24 ? 57 48 83").
   *                  Use '-' or '-' for wildcards.
   * @return The memory address where the pattern was found, or 0 if not found.
   */
  static uintptr_t Find(const char* signature);

  /**
   * @brief Finds a byte pattern within a specific memory range.
   *
   * @param base The base address to start searching from.
   * @param size The size of the memory block to search.
   * @param signature A string representing the byte pattern.
   * @return The memory address where the pattern was found, or 0 if not found.
   */
  static uintptr_t Find(uintptr_t base, size_t size, const char* signature);

  /**
   * @brief Finds a byte pattern within a specific memory range using a byte array.
   *
   * @param base The base address to start searching from.
   * @param size The size of the memory block to search.
   * @param signature A pointer to the byte array representing the pattern.
   * @param signatureSize The size of the byte array pattern.
   * @return The memory address where the pattern was found, or 0 if not found.
   */
  static uintptr_t Find(uintptr_t base, size_t size, const unsigned char* signature, size_t signatureSize);

 private:
  static std::vector<int> SignatureToVector(const std::string& signature);
  static uintptr_t Find(const char* moduleName, const std::vector<int>& signature);
};
}  // namespace Utils
SPF_NS_END
