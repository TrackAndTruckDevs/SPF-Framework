#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Hooks/IHook.hpp"

#include <cstdint>
#include <string>


SPF_NS_BEGIN
namespace Hooks::GameTools {
/**
 * @class ScsNameResolver
 * @brief A manageable hook service for resolving SCS internal names (unit IDs, tokens).
 *
 * This class acts as a feature hook that finds the addresses of native SCS
 * string-resolution functions without installing detours. It wraps them behind
 * a clean C++ API so callers can resolve unit names and decode tokens without
 * duplicating pattern-scanning or stream-wrapper logic.
 */
class ScsNameResolver : public IHook {
 public:
  // Function pointer types for the game's resolver functions.
  using ResolveUnitNameFunc = void (*)(void* stream, uint32_t unitId, uint64_t hashType);
  using DecodeTokenFunc = bool (*)(void* stream, uint64_t* token);

 public:
  static ScsNameResolver& GetInstance();

  ScsNameResolver(const ScsNameResolver&) = delete;
  void operator=(const ScsNameResolver&) = delete;

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool IsInstalled() const override {
    return m_resolveAddr != 0 && m_decodeAddr != 0 && m_unitIdToTokenFn != 0;
  }
  const std::string& GetSignature() const override { return m_signature; }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  // --- Public API for Framework ---
  /**
   * @brief Resolves an SCS unit ID to its canonical name string.
   * @param unitId The numeric unit identifier.
   * @return The resolved name, or an empty string on failure.
   */
  std::string ResolveUnitName(uint32_t unitId);

  /**
   * @brief Decodes an SCS-hashed token back to a human-readable string.
   * @param token The 64-bit token to decode.
   * @return The decoded string, or an empty string on failure.
   */
  std::string DecodeToken(uint64_t token);

  uint64_t ResolveUnitToken(uint32_t unitId);

 private:
  ScsNameResolver();
  ~ScsNameResolver() = default;

  // --- Internal Stream Wrapper ---
  /**
   * @brief Stack-allocated stream that mimics the SCS internal writer.
   *
   * Layout must match the game's ScsStringBuilder (or similar) used by the
   * native resolution functions.
   */
  struct ScsStringWriter {
    static void* g_vtable[3];
    static size_t EnsureCapacity(ScsStringWriter* self, size_t required);

    void** vtable;          // +0x00
    char*  buffer;          // +0x08
    int32_t length;         // +0x10
    int32_t capacity;       // +0x14
    char m_data[256];       // +0x18
  };

  // --- Hook Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "ScsNameResolver";
  std::string m_displayName = "SCS Name Resolver";
  bool m_isEnabled = true;
  std::string m_signature;

  // --- Runtime State ---
  uintptr_t m_resolveAddr = 0;
  uintptr_t m_decodeAddr = 0;
  uintptr_t m_unitIdToTokenFn = 0;
};
}  // namespace Hooks::GameTools
SPF_NS_END
