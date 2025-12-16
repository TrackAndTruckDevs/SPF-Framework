#pragma once

#include "SPF/Hooks/BaseHook.hpp"
#include <string>

SPF_NS_BEGIN
namespace Modules {
/**
 * @class PluginHook
 * @brief An internal proxy class that represents a plugin-owned hook.
 *
 * This class implements the IHook interface and performs the actual hooking
 * using MinHook on behalf of the plugin.
 */
class PluginHook : public Hooks::BaseHook {
 public:
  PluginHook(const std::string& pluginName, const std::string& hookName, const std::string& displayName, void* pDetour, void** ppOriginal, const std::string& signature, bool isEnabled);
  ~PluginHook();

 private:
  // --- Pure Virtual Functions from BaseHook ---
  void* GetDetourFunc() override;
  void** GetOriginalFuncPtr() override;

  // --- Hooking Details provided by the plugin ---
  void* m_pDetour;
  void** m_ppOriginal;
};
}  // namespace Modules
SPF_NS_END