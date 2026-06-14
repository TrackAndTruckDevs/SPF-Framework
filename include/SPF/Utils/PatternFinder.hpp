#pragma once

#include "SPF/Namespace.hpp"
#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>

SPF_NS_BEGIN
namespace Utils {

/**
 * @class PatternFinder
 * @brief A high-performance memory scanning and reverse engineering toolkit for the SCS engine.
 * 
 * This class provides optimized tools for:
 * 1. Byte pattern matching (Signatures) with backtracking support.
 * 2. Automated harvesting of SCS Reflection Tables with caching.
 * 3. Advanced code analysis (Xrefs, function start detection, VTable resolution).
 * 4. Section-aware memory scanning to minimize CPU overhead.
 * 
 * It operates as a Singleton to manage internal performance caches (Strings, Pointers, Reflection).
 */
class PatternFinder {
 public:
  /**
   * @brief Accesses the global PatternFinder instance.
   * @return PatternFinder& Reference to the singleton instance.
   */
  static PatternFinder& GetInstance();

  PatternFinder(const PatternFinder&) = delete;
  PatternFinder& operator=(const PatternFinder&) = delete;

  // ===========================================================================
  // SECTION 1: Pattern Matching Core
  // ===========================================================================

  /**
   * @brief Scans the entire module for a specific hex signature.
   * 
   * @param signature A space-separated hex string (e.g., "48 89 5C 24 ? 57").
   *                  Supports '?' or '??' as wildcards.
   * @return uintptr_t The absolute memory address of the first match, or 0 if not found.
   */
  static uintptr_t Find(const char* signature);

  /**
   * @brief Scans a specific memory block for a text signature.
   * 
   * @param base Starting address of the memory range.
   * @param size Size of the memory block in bytes.
   * @param signature Hex string pattern to search for.
   * @return uintptr_t The address of the found pattern, or 0.
   */
  static uintptr_t Find(uintptr_t base, size_t size, const char* signature);

  /**
   * @brief Searches for a pattern backwards from a base address.
   * Moves the starting point step-by-step backwards, but performs forward matching.
   * 
   * @param startAddress The address where the backward search starts.
   * @param searchRange How many bytes to look back from the startAddress.
   * @param signature Hex string pattern to search for.
   * @return uintptr_t The address of the found pattern, or 0.
   */
  static uintptr_t FindBackward(uintptr_t startAddress, size_t searchRange, const char* signature);

  /**
   * @brief Performs an optimized search for a raw byte sequence in a memory block.
   * 
   * @param base Starting address of the memory range.
   * @param size Size of the memory block.
   * @param signature Pointer to the raw byte array pattern.
   * @param signatureSize Length of the byte array.
   * @return uintptr_t The address of the found pattern, or 0.
   * @note Fastest search method as it skips parsing overhead.
   */
  static uintptr_t Find(uintptr_t base, size_t size, const unsigned char* signature, size_t signatureSize);

  /**
   * @brief Searches for a stable chain of instructions (multiple patterns).
   * 
   * @param signatures Ordered vector of signatures to find.
   * @param maxGap Maximum allowed distance (bytes) between each signature in the chain.
   * @param startAddress Address to start searching from (scans full module if 0).
   * @param searchRange Limit of the search window (ignored if startAddress is 0).
   * @return uintptr_t Address where the FIRST signature starts, or 0.
   */
  static uintptr_t FindChain(const std::vector<std::string>& signatures, size_t maxGap = 256, uintptr_t startAddress = 0, size_t searchRange = 0);

  /**
   * @struct ByteMatcher
   * @brief Represents a rule for matching a single byte in a pattern.
   */
  struct ByteMatcher {
    enum Type { EXACT, WILDCARD, RANGE };
    Type type;        ///< Match type (exact byte, any byte, or value range)
    uint8_t min;      ///< Minimum value for EXACT or RANGE
    uint8_t max;      ///< Maximum value for RANGE
    int minCount = 1; ///< For future support of variable length wildcards
    int maxCount = 1;

    /**
     * @brief Checks if a byte satisfies the matching rule.
     * @param b The byte to check.
     * @return true if it matches, false otherwise.
     */
    bool Matches(uint8_t b) const {
      if (type == WILDCARD) return true;
      if (type == EXACT) return b == min;
      return b >= min && b <= max;
    }
  };

  // ===========================================================================
  // SECTION 2: SCS Reflection Support
  // ===========================================================================

  /**
   * @brief Finds the offset of a class attribute using the game's internal reflection.
   * 
   * On the first call for a class, it harvests ALL attributes of that class and 
   * stores them in the cache for instant future access.
   * 
   * @param className Name of the SCS class (e.g., "vehicle_interior_camera").
   * @param attributeName Name of the field to find (e.g., "head_offset").
   * @return uintptr_t The field offset relative to the class instance, or 0.
   */
  static uintptr_t FindAttributeOffset(const char* className, const char* attributeName);

  /**
   * @brief Retrieves the C++ data type size from an SCS TypeID.
   * 
   * @param typeId The internal SCS TypeID (e.g., 0x05 for float).
   * @return size_t Size in bytes, or 0 if unknown.
   */
  static size_t GetSizeFromTypeId(uint64_t typeId);

  // ===========================================================================
  // SECTION 3: Reverse Engineering Tools
  // ===========================================================================

  /**
   * @brief Locates the prologue (start) of the function containing the given address.
   * 
   * @param address Any address inside the function.
   * @return uintptr_t Address of the function's first instruction, or 0.
   * @note Uses Windows exception tables (.pdata) for maximum reliability on x64.
   */
  static uintptr_t GetFunctionStart(uintptr_t address);

  /**
   * @brief Finds all instructions that reference a target address (RIP-relative).
   * 
   * @param targetAddr The absolute address being referenced.
   * @param moduleName Name of the module to scan (nullptr for main EXE).
   * @return std::vector<uintptr_t> List of instruction addresses (LEA, MOV, etc.).
   */
  static std::vector<uintptr_t> FindXrefs(uintptr_t targetAddr, const char* moduleName = nullptr);

  /**
   * @brief Finds all 8-byte aligned data pointers pointing to a target address.
   * 
   * @param targetAddr The address to find pointers to.
   * @param moduleName Name of the module to scan.
   * @return std::vector<uintptr_t> Addresses of the found pointers in data sections.
   * @note Results are cached for subsequent lookups of the same target.
   */
  static std::vector<uintptr_t> FindDataPointers(uintptr_t targetAddr, const char* moduleName = nullptr);

  /**
   * @brief Extracts a VTable address from an instruction referencing it.
   * 
   * @param signature Signature that matches the referencing instruction.
   * @param offsetPos Position of the 32-bit displacement within the instruction.
   * @param instructionSize Total length of the instruction.
   * @return uintptr_t Absolute address of the VTable, or 0.
   */
  static uintptr_t FindVTable(const char* signature, int offsetPos = 3, int instructionSize = 7);

  /**
   * @brief Gets a function address from a Virtual Table by index.
   * 
   * @param vtableAddr Absolute address of the VTable.
   * @param index 0-based index of the function pointer.
   * @return uintptr_t Address of the target function, or 0.
   */
  static uintptr_t GetVTableFunction(uintptr_t vtableAddr, int index);

  /**
   * @brief Finds a function start based on a 32-bit constant it uses.
   * 
   * @param constant The 32-bit value to look for.
   * @param findStart If true, returns the function start; if false, returns the instruction address.
   * @return uintptr_t Address found, or 0.
   */
  static uintptr_t FindFunctionByConstant(uint32_t constant, bool findStart = true);

  /**
   * @brief Finds a function that references a specific unique string.
   * 
   * @param str The string to search for.
   * @param findStart If true, returns the function prologue.
   * @param contextSig Optional signature to find near the string reference to disambiguate.
   * @param contextRange Search window for the context signature.
   * @return uintptr_t Function address, or 0.
   */
  static uintptr_t FindFunctionByString(const char* str, bool findStart = true, const char* contextSig = nullptr, size_t contextRange = 512);

  // ===========================================================================
  // SECTION 4: Safe Memory Access & Utilities
  // ===========================================================================

  /**
   * @brief Safely reads a 32-bit integer.
   * @param address Target memory address.
   * @return int32_t The value read, or 0 if address is null.
   */
  static int32_t ReadInt32(uintptr_t address);
  
  /**
   * @brief Safely reads an 8-bit integer.
   */
  static int8_t  ReadInt8(uintptr_t address);
  
  /**
   * @brief Safely reads a 64-bit integer.
   */
  static int64_t ReadInt64(uintptr_t address);
  
  /**
   * @brief Safely reads a float.
   */
  static float   ReadFloat(uintptr_t address);
  
  /**
   * @brief Safely reads a double.
   */
  static double  ReadDouble(uintptr_t address);

  /**
   * @brief Resolves an absolute address from a RIP-relative displacement.
   * 
   * @param instructionAddr Starting address of the instruction.
   * @param offsetPos Displacement field offset.
   * @param instructionSize Instruction length.
   * @return uintptr_t The resolved absolute address.
   */
  static uintptr_t GetRipAddress(uintptr_t instructionAddr, int offsetPos, int instructionSize);

  /**
   * @brief Checks if a class offset is within typical game object boundaries.
   * @param offset The value to validate.
   * @return true if sane, false if suspicious.
   */
  static bool IsSaneOffset(int32_t offset);

  /**
   * @brief Finds the address of a null-terminated string in data sections.
   * @param str The string to find.
   * @param moduleName Module name.
   * @return uintptr_t Absolute address of the string, or 0.
   */
  static uintptr_t FindString(const char* str, const char* moduleName = nullptr);

  /**
   * @struct MemorySection
   * @brief Holds information about a PE module section.
   */
  struct MemorySection {
    uintptr_t base;   ///< Start address of the section
    size_t size;      ///< Virtual size of the section
    std::string name; ///< Section name (e.g., ".data")
  };

  /**
   * @brief Parses and caches all section headers for a module.
   * @param moduleName Name of the module (nullptr for main EXE).
   * @return std::vector<MemorySection> List of identified sections.
   */
  static std::vector<MemorySection> GetModuleSections(const char* moduleName = nullptr);

 private:
  PatternFinder() = default;

  /**
   * @brief Internal recursive chain validator with backtracking.
   */
  static uintptr_t FindChainRecursive(const std::vector<std::vector<ByteMatcher>>& compiledSigs, size_t sigIdx, uintptr_t currentBase, size_t maxGap, uintptr_t searchLimit);
  
  /**
   * @brief Core backtracking engine for signature matching.
   */
  static bool MatchInternal(const uint8_t* data, size_t dataSize, const std::vector<ByteMatcher>& matchers, size_t dataIdx, size_t matcherIdx, size_t& matchLen);
  
  /**
   * @brief Parses a hex signature string into internal matcher rules.
   */
  static std::vector<ByteMatcher> SignatureToVector(const std::string& signature);
  
  /**
   * @brief Core scanner that iterates over module memory.
   */
  static uintptr_t Find(const char* moduleName, const std::vector<ByteMatcher>& signature);

  // --- Performance Caches ---
  std::unordered_map<std::string, std::unordered_map<std::string, uintptr_t>> m_reflectionCache; ///< className -> {attrName -> offset}
  std::unordered_map<std::string, std::vector<uintptr_t>> m_stringCache;      ///< string -> [addresses]
  std::unordered_map<uintptr_t, std::vector<uintptr_t>> m_pointerCache;       ///< targetAddr -> [pointer_locations]
  std::unordered_map<std::string, std::vector<MemorySection>> m_sectionCache; ///< moduleName -> [sections]
};

}  // namespace Utils
SPF_NS_END
