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
   * @brief Finds the starting address of a function containing the given address.
   * Uses Windows Runtime Function Tables (.pdata) for 100% accuracy on x64.
   * 
   * @param address Any address within the function.
   * @return The address of the function's first instruction, or 0 if not found.
   */
  static uintptr_t GetFunctionStart(uintptr_t address);

  /**
   * @brief Extracts a VTable address from an instruction that references it.
   * Usually looks for 'LEA RAX, [RIP + offset]' or similar.
   * 
   * @param signature A signature that matches the instruction referencing the VTable.
   * @param offsetPos The position of the 32-bit displacement within the instruction.
   * @param instructionSize The total size of the instruction.
   * @return The absolute address of the VTable, or 0 if not found.
   */
  static uintptr_t FindVTable(const char* signature, int offsetPos = 3, int instructionSize = 7);

  /**
   * @brief Gets a function address from a Virtual Function Table (VTable) by its index.
   * 
   * @param vtableAddr The absolute address of the VTable.
   * @param index The 0-based index of the function in the VTable.
   * @return The address of the function, or 0 if invalid.
   */
  static uintptr_t GetVTableFunction(uintptr_t vtableAddr, int index);

  /**
   * @brief Finds a function that references a specific 32-bit constant value.
   * 
   * @param constant The 32-bit value to search for (e.g., 0x3C888889).
   * @param findStart If true, returns the function start, otherwise returns the reference address.
   * @return The function or reference address, or 0 if not found.
   */
  static uintptr_t FindFunctionByConstant(uint32_t constant, bool findStart = true);

  /**
   * @brief Finds a sequence of patterns that appear close to each other.
   * Useful when compiler inserts padding or minor logic between key instructions.
   * 
   * @param signatures A list of signatures to find in order.
   * @param maxGap The maximum number of bytes allowed between each pattern.
   * @param startAddress Optional address to start searching from (scans module if 0).
   * @param searchRange Optional range limit. If 0 and startAddress is set, tries to detect function end.
   * @return The address where the FIRST pattern in the chain starts, or 0.
   */
  static uintptr_t FindChain(const std::vector<std::string>& signatures, size_t maxGap = 256, uintptr_t startAddress = 0, size_t searchRange = 0);

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

  /**
   * @struct ByteMatcher
   * @brief Represents a single byte matching rule in a signature.
   * 
   * Supports:
   * - EXACT: Matches a specific byte value (e.g., "48").
   * - WILDCARD: Matches any byte value (e.g., "??", "?").
   * - RANGE: Matches any byte within a [min, max] range (e.g., "[40-7F]").
   */
  struct ByteMatcher {
    enum Type { EXACT, WILDCARD, RANGE };
    Type type;
    uint8_t min;
    uint8_t max;
    int minCount = 1;
    int maxCount = 1;

    /**
     * @brief Checks if a given byte matches this rule's criteria.
     * @param b The byte to check.
     * @return true if it matches, false otherwise.
     */
    bool Matches(uint8_t b) const {
      if (type == WILDCARD) return true;
      if (type == EXACT) return b == min;
      return b >= min && b <= max;
    }
  };

  /**
   * @brief Finds the address of a null-terminated string in the module's data sections.
   */
  static uintptr_t FindString(const char* str, const char* moduleName = nullptr);

  /**
   * @brief Finds all RIP-relative references (Xrefs) to a specific target address.
   */
  static std::vector<uintptr_t> FindXrefs(uintptr_t targetAddr, const char* moduleName = nullptr);

  /**
   * @brief Finds a function address based on a string it contains.
   * @param str The string to look for.
   * @param findStart If true, will backtrack to the beginning of the function (prologue).
   * @param contextSig Optional additional signature to match near the string reference to disambiguate.
   * @param contextRange The search window size (in bytes) for the context signature.
   */
  static uintptr_t FindFunctionByString(const char* str, bool findStart = true, const char* contextSig = nullptr, size_t contextRange = 512);

 private:
  /**
   * @brief Helper for recursive chain validation with backtracking.
   */
  static uintptr_t FindChainRecursive(
      const std::vector<std::vector<ByteMatcher>>& compiledSigs,
      size_t sigIdx,
      uintptr_t currentBase,
      size_t maxGap,
      uintptr_t searchLimit);

  /**
   * @brief Internal helper to match a pattern starting at a specific address using backtracking.
   */
  static bool MatchInternal(const uint8_t* data, const std::vector<ByteMatcher>& matchers, size_t dataIdx, size_t matcherIdx, size_t& matchLen);
  /**
   * @brief Internal helper to parse a signature string into a vector of matchers.
   * Supports "??", "?", "XX", and "[XX-YY]" formats.
   */
  static std::vector<ByteMatcher> SignatureToVector(const std::string& signature);

  /**
   * @brief Internal helper to perform a scan using pre-parsed ByteMatchers.
   */
  static uintptr_t Find(const char* moduleName, const std::vector<ByteMatcher>& signature);

  /**
   * @brief Finds all 8-byte aligned memory locations that contain the specified address.
   * Useful for finding indirect pointers in data sections.
   */
  static std::vector<uintptr_t> FindDataPointers(uintptr_t targetAddr, const char* moduleName = nullptr);
};
}  // namespace Utils
SPF_NS_END
