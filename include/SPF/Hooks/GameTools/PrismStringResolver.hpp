#pragma once

#include "SPF/Hooks/IHook.hpp"

#include <cstdint>
#include <string>

namespace SPF::Hooks::GameTools {
/**
 * @class PrismStringResolver
 * @brief A manageable hook service for reading and writing game prism_string objects.
 *
 * Finds prism_string_set via pattern scan, then extracts buffer/length field
 * offsets from the function body — no hardcoded struct layout. Provides Set()
 * (game function) and GetBuffer/GetLength (direct field reads via discovered
 * offsets).
 */
class PrismStringResolver : public IHook {
 public:
  // Game function signature (Ghidra prism_string_set @1400f41b0: MOV R8,[RDX]):
  // second argument is char** — callee dereferences it once to get the C-string.
  using PrismStringSetFunc = void (*)(void* prismString, const char** value);

 public:
  static PrismStringResolver& GetInstance();

  PrismStringResolver(const PrismStringResolver&) = delete;
  void operator=(const PrismStringResolver&) = delete;

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool IsInstalled() const override { return m_prismStringSet != 0 && m_bufferOffset != 0 && m_lengthOffset != 0; }
  const std::string& GetSignature() const override { return m_signature; }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  // --- Public API for Framework ---
  /**
   * @brief Writes a C-string into a game-owned prism_string (buffer + length).
   * @param prismString Destination prism_string object (e.g. event+0x38).
   * @param value Null-terminated source string.
   * @return true if the game function was invoked.
   */
  bool Set(void* prismString, const char* value);

  /**
   * @brief Reads the char* buffer pointer from a prism_string via discovered offset.
   * @param prismString Source prism_string object.
   * @return Pointer to the internal buffer, or nullptr on failure.
   */
  const char* GetBuffer(const void* prismString) const;

  int32_t GetBufferOffset() const { return m_bufferOffset; }

  /**
   * @brief Reads the length field from a prism_string via discovered offset.
   * @param prismString Source prism_string object.
   * @return String length, or 0 on failure.
   */
  uint32_t GetLength(const void* prismString) const;

  /**
   * @brief Safely materializes a game prism_string into std::string.
   *
   * Distrusts an implausible length field (0 = uninitialized / wrong offset,
   * > 64 KB = garbage) and falls back to a bounded NUL scan; buffer
   * readability is validated page-granular (VirtualQuery granularity).
   * @param prismString Source prism_string object.
   * @return String content, or empty on failure.
   */
  std::string ReadString(const void* prismString) const;

  /**
   * @brief Initializes an empty prism_string: vtable + empty-string buffer + zero length.
   * @param prismString Destination prism_string object (e.g. MailRequest+0x00).
   * @return true if both anchors were found and the fields were written.
   */
  bool InitEmpty(void* prismString);

  /**
   * @brief Calls the game prism_string destructor (releases owned buffer).
   * @param prismString Prism_string object to destroy (e.g. MailRequest+0x00 temp).
   * @return true if the game function was invoked.
   */
  bool Destroy(void* prismString);

  /** @brief prism_string destructor address (0 if not found). */
  uintptr_t GetDtorAddr() const { return m_prismStringDtor; }

  /**
   * @brief LEA address of the "mailtext.ovs.new_routes" etalon anchor (cached
   * during Install, shared with other finders to avoid a duplicate string scan).
   */
  uintptr_t GetEtalonAnchorAddr() const { return m_etalonAnchor; }

  /**
   * @brief Checks that the prism_string's buffer pointer is readable memory.
   * @param prismString Source prism_string object.
   * @return true if a readable buffer pointer is present.
   */
  bool IsBufferReadable(const void* prismString) const;

 private:
  PrismStringResolver() = default;
  ~PrismStringResolver() = default;

  // --- Hook Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "PrismStringResolver";
  std::string m_displayName = "Prism String Resolver";
  bool m_isEnabled = true;
  std::string m_signature = "str:[win] Window creation failed. (phased: ctor / set / dtor)";

  // --- Runtime State ---
  uintptr_t m_prismStringSet = 0;
  uintptr_t m_prismStringDtor = 0;   // prism_string destructor
  uintptr_t m_etalonAnchor = 0;      // "mailtext.ovs.new_routes" LEA site
  uintptr_t m_vtableAddr = 0;        // prism_string vtable
  uintptr_t m_emptyStringAddr = 0;   // static empty C-string
  int32_t m_bufferOffset = 0;   // +0x08 relative to prism_string base
  int32_t m_lengthOffset = 0;   // +0x10 relative to prism_string base
};
}  // namespace SPF::Hooks::GameTools
