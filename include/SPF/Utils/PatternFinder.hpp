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

  /**
   * @brief Reads a 32-bit integer from the specified address.
   *
   * @param address The memory address to read from.
   * @return The 32-bit integer value.
   */
  static int32_t ReadInt32(uintptr_t address);

  /**
   * @brief Reads an 8-bit integer from the specified address.
   *
   * @param address The memory address to read from.
   * @return The 8-bit integer value.
   */
  static int8_t ReadInt8(uintptr_t address);

  /**
   * @brief Reads a 64-bit integer from the specified address.
   *
   * @param address The memory address to read from.
   * @return The 64-bit integer value.
   */
  static int64_t ReadInt64(uintptr_t address);

  /**
   * @brief Reads a floating-point value from the specified address.
   *
   * @param address The memory address to read from.
   * @return The float value.
   */
  static float ReadFloat(uintptr_t address);

  /**
   * @brief Reads a double-precision floating-point value from the specified address.
   *
   * @param address The memory address to read from.
   * @return The double value.
   */
  static double ReadDouble(uintptr_t address);

  /**
   * @brief Calculates an absolute address from a RIP-relative instruction.
   *
   * @param instructionAddr The starting address of the instruction.
   * @param offsetPos The position of the 32-bit displacement relative to instructionAddr.
   * @param instructionSize The total size of the instruction in bytes.
   * @return The calculated absolute address.
   */
  static uintptr_t GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize);

  /**
   * @brief Checks if an offset value is within a reasonable range for game objects.
   * Prevents using "garbage" values extracted from incorrect pattern matches.
   */
  static bool IsSaneOffset(int32_t offset);

 private:
  static std::vector<int> SignatureToVector(const std::string& signature);
  static uintptr_t Find(const char* moduleName, const std::vector<int>& signature);
};
}  // namespace Utils
SPF_NS_END
