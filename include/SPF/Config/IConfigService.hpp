#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/ComponentInfo.hpp"
#include "SPF/Config/ManifestData.hpp"  // Required for ManifestData type
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/System/EnvironmentManager.hpp"

#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>


SPF_NS_BEGIN

namespace Hooks {
class IHook;
}  // namespace Hooks

namespace Config {
/**
 * @brief Interface for the central configuration service.
 *
 * This service is responsible for loading, managing, and providing
 * access to all framework and plugin configurations according to the
 * architecture defined in plan v9.0.
 */
struct IConfigService {
  virtual ~IConfigService() = default;

  // --- Lifecycle ---

  /**
   * @brief Registers a plugin's manifest for later processing.
   * This method only stores the manifest and does not process it immediately.
   * @param pluginName The unique name of the plugin.
   * @param manifest The JSON content of the plugin's manifest.
   */
  virtual void RegisterPluginManifest(const std::string& pluginName, const ManifestData& manifest) = 0;

  /**
   * @brief Finalizes the configuration.
   * This is the main processing step where all registered manifests are read,
   * user settings files are loaded, and all configurations are merged or
   * aggregated according to their defined strategies.
   * No configuration data is available before this method is called.
   * @param report A report to log the outcome of the operation.
   */
  virtual void Finalize(Core::InitializationReport* report) = 0;

  /**
   * @brief Reconciles the loaded configuration with the list of physically existing plugins.
   * This method handles plugins without manifests and cleans up orphaned config entries.
   * It must be called after Finalize() and after plugins have been discovered.
   * @param physicalPluginNames A list of names of plugins that physically exist.
   * @param report A report to log the outcome of the operation.
   */
  virtual void ReconcilePluginStates(const std::vector<std::string>& physicalPluginNames, Core::InitializationReport* report) = 0;

  /**
   * @brief Reconciles the loaded configuration with the list of registered feature hooks.
   * This method handles hooks without config entries and cleans up orphaned ones.
   * @param featureHooks A list of pointers to all registered feature hooks.
   * @param report A report to log the outcome of the operation.
   */
  virtual void ReconcileHookStates(const std::vector<Hooks::IHook*>& featureHooks, Core::InitializationReport* report) = 0;

  /**
   * @brief Processes all registered system configurations (isolated and priority-merged).
   * This method iterates through all known systems and applies the appropriate merge strategy
   * to build the m_isolatedConfigs and m_mergedConfigs maps.
   * @param report A report to log the outcome of the operation.
   */
  virtual void ProcessAllSystemConfigurations(Core::InitializationReport& report) = 0;

  /**
   * @brief Saves any configurations that have been modified to their respective files.
   * This is typically called during the framework's shutdown sequence.
   */
  virtual void SaveAllDirty() = 0;

  // --- Data Access ---

  /**
   * @brief Gets the structured information for all components (framework + plugins).
   * @return A constant reference to a map of component IDs to their ComponentInfo structures.
   */
  virtual const std::map<std::string, ComponentInfo>& GetAllComponentInfo() const = 0;

  /**
   * @brief Gets the unique, anonymous identifier for this framework installation.
   * @details Stored in the registry under HKCU\Software\SPF_Framework and generated
   * there on first access. Every call also refreshes the stored Version and the
   * current game's DLL path.
   * @return A string containing the framework instance UUID.
   */
  virtual std::string GetFrameworkInstanceId() = 0;

  /**
   * @brief Gets the installation status based on the version stored in the configuration.
   * This is determined during the configuration loading phase.
   */
  virtual System::InstallationStatus GetInstallationStatus() const = 0;

  /**
   * @brief Checks if network connection is allowed by the user.
   * Creates the 'connect' key if it doesn't exist.
   */
  virtual bool IsConnectionAllowed() = 0;
  /**
   * @brief Gets all aggregated user settings for display in the UI.
   * This map contains all user-configurable settings, structured for easy consumption by the UI.
   * @return A constant reference to a map where the key is the full setting path
   *         and the value is its current nlohmann::ordered_json representation.
   */
  virtual const std::map<std::string, nlohmann::ordered_json>& GetAggregatedUserSettings() const = 0;

  /**
   * @brief Gets the final, merged configuration for a "merged" system.
   * @param systemName The name of the system (e.g., "keybinds").
   * @return A const pointer to the merged JSON object, or nullptr if not found or not a merged system.
   */
  virtual const nlohmann::ordered_json* GetMergedConfig(const std::string& systemName) const = 0;

  /**
   * @brief Gets all aggregated settings for an "isolated" system.
   * @param systemName The name of the system (e.g., "logging", "ui").
   * @return A const pointer to a map where the key is the component name (e.g., "framework", "TestPlugin")
   *         and the value is its specific configuration JSON. Returns nullptr if not found.
   */
  virtual const std::map<std::string, nlohmann::ordered_json>* GetAllComponentSettings(const std::string& systemName) const = 0;

  /**
   * @brief Gets a single value from a component's configuration.
   * This is a convenience method for simple value retrieval, primarily for the C-API.
   * @param componentName The ID of the component (e.g., "framework", "TestPlugin").
   * @param keyPath A dot-separated path to the value (e.g., "ui.windows.main_window.is_visible").
   * @param defaultValue The value to return if the key is not found.
   * @return The found JSON value, or the default value.
   */
  virtual nlohmann::ordered_json GetValue(const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& defaultValue) const = 0;

  /**
   * @brief Gets a stable pointer to a single value from a component's configuration.
   * This is for advanced C-API usage where a handle to the raw JSON is needed.
   * @param componentName The ID of the component (e.g., "framework", "TestPlugin").
   * @param keyPath A dot-separated path to the value (e.g., "ui.windows.main_window.is_visible").
   * @return A constant pointer to the found JSON value, or nullptr if not found.
   *         The lifetime of the pointed-to object is managed by the service.
   */
  virtual const nlohmann::ordered_json* GetValuePtr(const std::string& componentName, const std::string& keyPath) const = 0;

  // --- Data Modification & Reset ---

  /**
   * @brief Sets a specific value in a component's configuration.
   * This will mark the component's config as "dirty" to be saved on shutdown.
   * @param componentName The ID of the component (e.g., "framework", "TestPlugin").
   * @param jsonPath A dot-separated path to the value (e.g., "window.size.width").
   * @param value The JSON value to set.
   */
  virtual void SetValue(const std::string& componentName, const std::string& jsonPath, const nlohmann::ordered_json& value) = 0;

  /**
   * @brief Updates a specific binding.
   * @param actionFullName The full name of the action to update.
   * @param originalBinding The original JSON object of the binding to find and replace. If empty, a new binding is added.
   * @param newBinding The new JSON object for the binding.
   * @param bindingToClear Optional: if the input was taken from another action, this holds the action name and binding JSON to clear.
   */
  virtual void UpdateBinding(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding, const nlohmann::ordered_json& newBinding,
                             const std::optional<std::pair<std::string, nlohmann::ordered_json>>& bindingToClear) = 0;

  /**
   * @brief Deletes a specific binding from an action.
   * @param actionFullName The full name of the action to modify.
   * @param bindingToDelete The specific JSON object of the binding to remove from the array.
   */
  virtual void DeleteBinding(const std::string& actionFullName, const nlohmann::ordered_json& bindingToDelete) = 0;

  /**
   * @brief Updates a single property of a specific binding.
   * @param actionFullName The full name of the action to modify.
   * @param originalBinding The binding object to find and modify.
   * @param propertyName The name of the JSON property to change (e.g., "press_type").
   * @param newValue The new value for the property.
   */
  virtual void UpdateBindingProperty(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding, const std::string& propertyName,
                                     const nlohmann::ordered_json& newValue) = 0;

  /**
   * @brief Resets a specific key in a component's config to its default value from the manifest.
   * @param systemName The name of the system the key belongs to (e.g., "keybinds").
   * @param keyPath The full path to the key that needs resetting (e.g., "keybinds.actions.TestPlugin.toggle_window").
   * @param report A report to log the outcome of the operation.
   */
  virtual void ResetToDefault(const std::string& systemName, const std::string& keyPath, Core::InitializationReport* report) = 0;

  /**
   * @brief Dynamically registers metadata for a keybind action at runtime.
   * @param componentName The ID of the component (plugin) owning the action.
   * @param actionFullName The full name of the action (e.g., "MyPlugin.Dynamic.Cmd1").
   * @param titleKey Localization key or literal for the title.
   * @param descKey Localization key or literal for the description.
   */
  virtual void RegisterActionMetadata(const std::string& componentName, const std::string& actionFullName, const std::string& titleKey, const std::string& descKey) = 0;

  /**
   * @brief Dynamically unregisters metadata for a keybind action at runtime.
   * @param componentName The ID of the component (plugin) owning the action.
   * @param actionFullName The full name of the action to remove.
   */
  virtual void UnregisterActionMetadata(const std::string& componentName, const std::string& actionFullName) = 0;

  /**
   * @brief Retrieves a list of all action full names owned by a specific component.
   * @param componentName The ID of the component.
   * @return A vector of full action keys (e.g. {"Plugin.Action1", "Plugin.Action2"}).
   */
  virtual std::vector<std::string> GetOwnedActions(const std::string& componentName) const = 0;

  /**
   * @brief Checks if a specific key exists in a component's configuration.
   * @param componentName The ID of the component.
   * @param keyPath The dot-separated path to the key.
   */
  virtual bool HasKey(const std::string& componentName, const std::string& keyPath) const = 0;

  /**
   * @brief Force-saves the configuration for a specific component.
   * @param componentName The ID of the component.
   */
  virtual void SaveComponentConfig(const std::string& componentName) = 0;

  /**
   * @brief Removes a key or an entire path from a component's configuration.
   * @param componentName The ID of the component.
   * @param keyPath The dot-separated path to the key or object to remove.
   */
  virtual void RemoveKey(const std::string& componentName, const std::string& keyPath) = 0;

  /**
   * @brief Reloads the configuration for a specific component from disk.
   * @param componentName The ID of the component.
   */
  virtual void ReloadComponentConfig(const std::string& componentName) = 0;

  /**
   * @brief Checks if a specific setting is marked as hidden in the manifest metadata.
   * @param componentName The ID of the component.
   * @param keyPath The dot-separated path to the key (without system name).
   */
  virtual bool IsSettingHidden(const std::string& componentName, const std::string& keyPath) const = 0;

  /**
   * @brief Creates or retrieves a configuration context for an arbitrary JSON file.
   * @param filePath The full physical path to the JSON file.
   * @return A unique identifier for this custom context (e.g. the filePath itself).
   */
  virtual std::string CreateCustomContext(const std::string& filePath) = 0;

  /**
   * @brief Enables or disables automatic saving for a specific configuration context.
   * @param contextId The ID of the context (pluginName or custom context ID).
   * @param enabled If true, changes are automatically synced to disk.
   */
  virtual void SetAutoSave(const std::string& contextId, bool enabled) = 0;
};
}  // namespace Config

SPF_NS_END
