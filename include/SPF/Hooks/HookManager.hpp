#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Renderer/RenderAPI.hpp" // Added for the new method
#include <vector>
#include <string>
#include <set>
#include <map>

SPF_NS_BEGIN
namespace Hooks {
/**
 * @class HookManager
 * @brief A singleton service to manage the lifecycle of all hooks.
 *        It handles both critical system hooks and configurable feature hooks.
 */
class HookManager {
 public:
  static HookManager& GetInstance();

  HookManager(const HookManager&) = delete;
  void operator=(const HookManager&) = delete;

  /**
   * @brief Registers a configurable feature hook with the manager.
   * @param hook A pointer to the hook instance that implements IHook.
   */
  void RegisterFeatureHook(IHook* hook);

  /**
   * @brief Unregisters a feature hook from the manager.
   * @param hook A pointer to the hook instance to remove.
   */
  void UnregisterFeatureHook(IHook* hook);

  /**
   * @brief Installs the correct graphics hook based on the detected API.
   * @param api The render API detected by the Renderer.
   * @return True if the graphics hook was installed successfully, false otherwise.
   */
  bool InstallGraphicsHooks(Rendering::RenderAPI api);

  /**
   * @brief Installs all non-graphics system hooks and all enabled feature hooks.
   * @return True if all installed hooks were successful, false otherwise.
   */
  bool InstallSystemAndFeatureHooks();

  /**
   * @brief Installs a single feature hook.
   *        Used for hooks that are registered dynamically after initial startup.
   * @param hook A pointer to the hook to install.
   */
  void InstallFeatureHook(IHook* hook);

  /**
   * @brief Disables all hooks for a framework reload.
   */
  void UninstallAllHooks();

  /**
   * @brief Completely removes all hooks on shutdown.
   */
  void RemoveAllHooks();

  /**
   * @brief Gets a list of all registered feature hooks for the UI.
   * @return A constant reference to the vector of IHook pointers.
   */
  const std::vector<IHook*>& GetFeatureHooks() const;

  /**
   * @brief Gets a pointer to a registered feature hook by its programmatic name.
   * @param name The name of the hook to find.
   * @return A pointer to the IHook object, or nullptr if not found.
   */
  IHook* GetHook(const std::string& name);

  /**
   * @brief Re-evaluates the state of a single hook and installs or uninstalls it
   *        based on its enabled status and whether it is required by any plugin.
   * @param hook A pointer to the hook to reconcile.
   */
  void ReconcileHookState(IHook* hook, bool configuredEnabledState);

  /**
   * @brief Called by a plugin to request that a feature hook be enabled.
   * @param hookName The programmatic name of the hook.
   * @param pluginName The name of the plugin making the request.
   */
  void RequestEnableHook(const std::string& hookName, const std::string& pluginName);

  /**
   * @brief Called by a plugin when it's being unloaded to release its request.
   * @param hookName The programmatic name of the hook.
   * @param pluginName The name of the plugin releasing the request.
   */
  void ReleaseEnableRequest(const std::string& hookName, const std::string& pluginName);

  /**
   * @brief Checks if a feature hook is required by any active plugin.
   * @param hookName The programmatic name of the hook.
   * @return True if at least one plugin requires this hook.
   */
  bool IsHookRequired(const std::string& hookName) const;

 private:
  HookManager() = default;
  ~HookManager() = default;

  std::vector<IHook*> m_featureHooks;
  // Maps a hook name to a set of plugin names that require it.
  std::map<std::string, std::set<std::string>> m_hookRequests;
};
}  // namespace Hooks
SPF_NS_END
