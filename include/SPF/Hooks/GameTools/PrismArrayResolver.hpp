#pragma once

#include "SPF/Hooks/IHook.hpp"

#include <cstdint>
#include <string>

namespace SPF::Hooks::GameTools {
/**
 * @class PrismArrayResolver
 * @brief Discovers the prism array building blocks used by the game when it
 * constructs request payloads (empty shared control block, push, destructor).
 *
 * All addresses are discovered via pattern scan from the DeliverOvsNewRoutesMail
 * etalon (FUN_14084d030) — no hardcoded function addresses.
 */
class PrismArrayResolver : public IHook {
 public:
  static PrismArrayResolver& GetInstance();

  PrismArrayResolver(const PrismArrayResolver&) = delete;
  void operator=(const PrismArrayResolver&) = delete;

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool IsInstalled() const override { return m_emptyCtrl != 0 && m_push != 0 && m_dtor != 0; }
  const std::string& GetSignature() const override { return m_signature; }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  // --- Public API for Framework ---
  /** @brief Empty shared control block factory (etched call 14084d12c). */
  uintptr_t GetEmptyCtrlAddr() const { return m_emptyCtrl; }

  /** @brief Push element into array (etched call 14084d2ae). */
  uintptr_t GetPushAddr() const { return m_push; }

  /** @brief Array destructor (etched call 14084d498). */
  uintptr_t GetDtorAddr() const { return m_dtor; }

  /** @brief prism array vtable (array+0x00; grow method at vtable+0x18). */
  uintptr_t GetArrayVtableAddr() const { return m_arrayVtable; }

 private:
  PrismArrayResolver() = default;
  ~PrismArrayResolver() = default;

  // --- Hook Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "PrismArrayResolver";
  std::string m_displayName = "Prism Array Resolver";
  bool m_isEnabled = true;
  std::string m_signature = "chained: prism_string etalon anchor → empty ctrl CALL / vtable LEA / push / dtor";

  // --- Runtime State ---
  uintptr_t m_emptyCtrl = 0;
  uintptr_t m_arrayVtable = 0;
  uintptr_t m_push = 0;
  uintptr_t m_dtor = 0;
};
}  // namespace SPF::Hooks::GameTools
