#pragma once

#include "SPF/Hooks/IHook.hpp"
#include "SPF/Logging/Logger.hpp"
#include <string>
#include "MinHook.h"

SPF_NS_BEGIN
namespace Hooks {

/**
 * @class BaseHook
 * @brief An abstract base class for hooks that provides a reusable and robust
 *        implementation of the IHook interface using MinHook.
 * @details This class centralizes the logic for installing, uninstalling, and
 *          removing hooks, as well as managing their enabled/installed state.
 *          Derived classes only need to provide the specific details for their
 *          target function, such as the signature and detour function pointers.
 */
class BaseHook : public IHook {
 public:
  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override;
  const std::string& GetSignature() const override { return m_signature; }
  bool IsInstalled() const override { return m_hookedAddress != 0; }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

 protected:
  /**
   * @brief Constructs the BaseHook.
   * @param name The programmatic name of the hook (e.g., "GameLogHook").
   * @param displayName The user-facing name for the UI (e.g., "Game Log").
   * @param signature The byte signature of the target function.
   * @param ownerName The name of the owner (e.g., "framework" or plugin name).
   */
  BaseHook(std::string name, std::string displayName, std::string signature, std::string ownerName, bool isEnabled = false);
  virtual ~BaseHook() = default;

  // --- Pure Virtual Functions for Derived Classes ---

  /**
   * @brief Gets a pointer to the detour function.
   * @return A void pointer to the static detour function implementation.
   */
  virtual void* GetDetourFunc() = 0;

  /**
   * @brief Gets a pointer to the trampoline function pointer.
   * @return A pointer to the static function pointer that will hold the address
   *         of the original function (the trampoline).
   */
  virtual void** GetOriginalFuncPtr() = 0;

  // --- Member variable for derived classes ---
  std::string m_ownerName = "framework";

 private:

  // --- Hook Configuration (from constructor) ---
  std::string m_name;
  std::string m_displayName;
  std::string m_signature;

  // --- Runtime State ---
  bool m_isEnabled = false;
  uintptr_t m_hookedAddress = 0;
};
}  // namespace Hooks
SPF_NS_END
