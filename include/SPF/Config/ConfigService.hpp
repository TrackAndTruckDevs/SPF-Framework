#pragma once

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Config/ManifestData.hpp"
#include "SPF/Config/ComponentInfo.hpp"
#include "SPF/Namespace.hpp"

#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <vector>
#include <optional>
#include <utility>

SPF_NS_BEGIN

namespace Events {
class EventManager;
}

namespace Config {
// Enum to define the processing strategy for a configuration system
enum class MergeStrategy {
  PriorityMerge,  // For shared systems like keybinds
  Isolate         // For component-specific systems like logging, ui
};

class ConfigService : public IConfigService {
 public:
  ConfigService(Events::EventManager& eventManager);
  ~ConfigService() override = default;

  // --- IConfigService Implementation ---
  void RegisterPluginManifest(const std::string& pluginName, const ManifestData& manifest) override;
  void Finalize(Core::InitializationReport* report) override;
  void ReconcilePluginStates(const std::vector<std::string>& physicalPluginNames, Core::InitializationReport* report) override;
  void ReconcileHookStates(const std::vector<Hooks::IHook*>& featureHooks, Core::InitializationReport* report) override;
  void SaveAllDirty() override;

  const std::map<std::string, ComponentInfo>& GetAllComponentInfo() const override;
  const std::map<std::string, nlohmann::json>& GetAggregatedUserSettings() const override;
  const nlohmann::json* GetMergedConfig(const std::string& systemName) const override;
  const std::map<std::string, nlohmann::json>* GetAllComponentSettings(const std::string& systemName) const override;
  void SetValue(const std::string& componentName, const std::string& jsonPath, const nlohmann::json& value) override;
  void UpdateBinding(const std::string& actionFullName, const nlohmann::json& originalBinding, const nlohmann::json& newBinding,
                     const std::optional<std::pair<std::string, nlohmann::json>>& bindingToClear) override;
  void DeleteBinding(const std::string& actionFullName, const nlohmann::json& bindingToDelete) override;
  void UpdateBindingProperty(const std::string& actionFullName, const nlohmann::json& originalBinding, const std::string& propertyName, const nlohmann::json& newValue) override;
  nlohmann::json GetValue(const std::string& componentName, const std::string& keyPath, const nlohmann::json& defaultValue) const override;
  std::string GetOrCreateFrameworkInstanceId() override;
  void ResetToDefault(const std::string& systemName, const std::string& keyPath, Core::InitializationReport* report) override;
  /**
   * @brief Processes all registered system configurations (isolated and priority-merged).
   * This method iterates through all known systems and applies the appropriate merge strategy
   * to build the m_isolatedConfigs and m_mergedConfigs maps.
   * @param report A report to log the outcome of the operation.
   */
  void ProcessAllSystemConfigurations(Core::InitializationReport& report) override;

 private:
  /**
   * @brief Processes a system that uses the priority merge strategy.
   * @param systemName The name of the system (e.g., "keybinds").
   * @param report The report to log warnings/errors.
   */
  void MergePrioritySystem(const std::string& systemName, Core::InitializationReport& report);

  /**
   * @brief Processes a system that uses the isolation strategy.
   * @param systemName The name of the system (e.g., "logging").
   * @param report The report to log warnings/errors.
   */
  void AggregateIsolatedSystem(const std::string& systemName, Core::InitializationReport& report);

  /**
   * @brief Compares the final merged keybinds with user settings on disk
   *        and marks components as dirty if they don't match.
   * @param report The report to log warnings/errors.
   */
  void CheckDirtyKeybinds(Core::InitializationReport& report);

  /**
   * @brief Builds the aggregated user settings map for the UI.
   */
  void BuildAggregatedUserSettings();
  bool _DeleteBindingInternal(const std::string& actionFullName, const nlohmann::json& bindingToDelete);

  // --- Data Members ---
  Events::EventManager& m_eventManager;

  // Stores the manifest content for each component ("framework", "TestPlugin", etc.)
  std::map<std::string, ManifestData> m_manifests;

  // Hardcoded map defining the strategy for each known system
  std::map<std::string, MergeStrategy> m_systemStrategies;

  // Stores the final configuration for merged systems. Key: systemName.
  std::map<std::string, nlohmann::json> m_mergedConfigs;

  // Stores the final configurations for isolated systems.
  // Key1: systemName, Key2: componentName
  std::map<std::string, std::map<std::string, nlohmann::json>> m_isolatedConfigs;

  // Stores a map of all user-configurable settings, aggregated for the UI.
  std::map<std::string, nlohmann::json> m_aggregatedUserSettings;

  // Stores structured information about all components (framework + plugins) after reconciliation.
  std::map<std::string, ComponentInfo> m_allComponentInfo;

  // Stores which component owns which keybind action. Key: full action name, Value: component name.
  std::map<std::string, std::string> m_keybindOwnership;

  // Tracks which components have had their configs modified and need saving.
  std::set<std::string> m_dirtyComponents;
  // Paths to user config files that were found to be corrupted (invalid JSON) during loading.
  std::set<std::string> m_corruptedFilePaths;

  // List of system names that are considered user-configurable (e.g., "settings", "keybinds").
  std::vector<std::string> m_userConfigurableSystems;
};

}  // namespace Config

SPF_NS_END