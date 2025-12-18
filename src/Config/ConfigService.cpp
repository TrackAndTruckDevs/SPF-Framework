#include "SPF/Config/ConfigService.hpp"

#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/ConfigEvents.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Config/FrameworkManifest.hpp" // Provides the in-code framework manifest

#include <fstream>
#include <filesystem>
#include <regex>
#include <set>
#include <algorithm>
#include <objbase.h> // For CoCreateGuid
#include <cstdio>    // For snprintf

#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Hooks/IHook.hpp"


#include "SPF/Config/ManifestData.hpp" // Include the new C++ structure definitions

SPF_NS_BEGIN

namespace Config {
using namespace SPF::Logging;
using namespace SPF::System;
using namespace SPF::Core;

namespace {
// Helper to inject _meta block into a JSON object
void InjectMetadata(nlohmann::json& target, const std::string& titleKey, const std::string& descriptionKey) {
    if (!titleKey.empty() || !descriptionKey.empty()) {
        target["_meta"] = nlohmann::json::object();
        if (!titleKey.empty()) {
            target["_meta"]["titleKey"] = titleKey;
        }
        if (!descriptionKey.empty()) {
            target["_meta"]["descriptionKey"] = descriptionKey;
        }
    }
}

const nlohmann::json* GetSettings(const nlohmann::json& source, const std::string& systemName) {
  if (source.contains(systemName) && source[systemName].is_object()) {
    return &source[systemName];
  }
  return nullptr;
}

// Helper to serialize general settings from ManifestData to nlohmann::json.
nlohmann::json SerializeSettings(const ManifestData& manifest, const ManifestData& frameworkManifest) {
    nlohmann::json j = manifest.settings;
    // Inject custom settings metadata (no fallback for custom settings)
    for (const auto& meta : manifest.customSettingsMetadata) {
        if (meta.keyPath.empty()) continue;
        try {
            auto ptr = nlohmann::json::json_pointer("/" + std::regex_replace(meta.keyPath, std::regex("\\."), "/"));
            if (!j.contains(ptr)) continue;
            nlohmann::json& node = j[ptr];
            if (node.is_object() && node.contains("_value")) {
                InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
            } else if (node.is_primitive() || node.is_string() || node.is_array()) { // Added is_array() here
                auto value = node;
                node = nlohmann::json::object();
                node["_value"] = value;
                InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
            } else if (node.is_object()) {
                InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
            }

            // NEW: Inject UI rendering hints
            if (meta.widget.has_value() || !meta.widget_params.empty()) {
                if (!node.contains("_meta")) {
                     node["_meta"] = nlohmann::json::object();
                }
                node["_meta"]["ui"] = nlohmann::json::object();
                auto& ui_meta = node["_meta"]["ui"];
                if (meta.widget.has_value()) {
                    ui_meta["widget"] = meta.widget.value();
                }
                if (!meta.widget_params.empty()) {
                    ui_meta["params"] = meta.widget_params;
                }
            }
        } catch (const std::exception& e) {
            LoggerFactory::GetInstance().GetLogger("ConfigService")->Error("Error injecting custom setting metadata for key '{}': {}", meta.keyPath, e.what());
        }
    }
    return j;
}

// Helper to serialize logging settings from ManifestData to nlohmann::json.
nlohmann::json SerializeLogging(const ManifestData& manifest, const ManifestData& frameworkManifest) {
    nlohmann::json j;
    auto findLoggingMeta = [&](const std::string& key) -> const StandardSettingMetadata* {
        for (const auto& meta : manifest.loggingMetadata) { if (meta.key == key) return &meta; }
        if (&manifest != &frameworkManifest) {
            for (const auto& meta : frameworkManifest.loggingMetadata) { if (meta.key == key) return &meta; }
        }
        return nullptr;
    };

    if (manifest.logging.level.has_value()) {
        nlohmann::json node;
        node["_value"] = manifest.logging.level.value();
        if (const auto* meta = findLoggingMeta("level")) {
            InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
        }
        j["level"] = node;
    }

    nlohmann::json sinksNode;
    if (const auto* meta = findLoggingMeta("sinks")) {
        InjectMetadata(sinksNode, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    if (manifest.logging.sinks.file.has_value()) {
        nlohmann::json node;
        node["_value"] = manifest.logging.sinks.file.value();
        if (const auto* meta = findLoggingMeta("sinks.file")) {
            InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
        }
        sinksNode["file"] = node;
    }
    if (manifest.logging.sinks.ui.has_value()) {
        nlohmann::json node;
        node["_value"] = manifest.logging.sinks.ui.value();
        if (const auto* meta = findLoggingMeta("sinks.ui")) {
            InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
        }
        sinksNode["ui"] = node;
    }
    if (!sinksNode.empty()) {
        j["sinks"] = sinksNode;
    }
    return j;
}

// Helper to serialize localization settings from ManifestData to nlohmann::json.
nlohmann::json SerializeLocalization(const ManifestData& manifest, const ManifestData& frameworkManifest) {
    nlohmann::json j;
    auto findLocMeta = [&](const std::string& key) -> const StandardSettingMetadata* {
        for (const auto& meta : manifest.localizationMetadata) { if (meta.key == key) return &meta; }
        if (&manifest != &frameworkManifest) {
            for (const auto& meta : frameworkManifest.localizationMetadata) { if (meta.key == key) return &meta; }
        }
        return nullptr;
    };

    if (manifest.localization.language.has_value()) {
        nlohmann::json node;
        node["_value"] = manifest.localization.language.value();
        if (const auto* meta = findLocMeta("language")) {
            InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
        }
        j["language"] = node;
    }
    return j;
}

// Helper to serialize UI settings from ManifestData to nlohmann::json.
nlohmann::json SerializeUI(const ManifestData& manifest, const ManifestData& frameworkManifest) {
    nlohmann::json j;
    nlohmann::json windowsNode;

    auto findUIMeta = [&](const std::string& key) -> const WindowMetadata* {
        for (const auto& meta : manifest.uiMetadata) { if (meta.windowName == key) return &meta; }
        if (&manifest != &frameworkManifest) {
            for (const auto& meta : frameworkManifest.uiMetadata) { if (meta.windowName == key) return &meta; }
        }
        return nullptr;
    };
    
    if (const auto* meta = findUIMeta("windows")) {
        InjectMetadata(windowsNode, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }

    for (const auto& [name, data] : manifest.ui.windows) {
        nlohmann::json window_j;
        if (const auto* meta = findUIMeta(name)) {
            InjectMetadata(window_j, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
        }

        auto addValueWithMeta = [&](const std::string& propName, auto propValue) {
            nlohmann::json node;
            node["_value"] = propValue;
            if (const auto* meta = findUIMeta(propName)) {
                InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
            }
            window_j[propName] = node;
        };

        if (data.isVisible.has_value()) addValueWithMeta("is_visible", data.isVisible.value());
        if (data.isInteractive.has_value()) addValueWithMeta("is_interactive", data.isInteractive.value());
        if (data.posX.has_value()) addValueWithMeta("pos_x", data.posX.value());
        if (data.posY.has_value()) addValueWithMeta("pos_y", data.posY.value());
        if (data.sizeW.has_value()) addValueWithMeta("size_w", data.sizeW.value());
        if (data.sizeH.has_value()) addValueWithMeta("size_h", data.sizeH.value());
        if (data.isCollapsed.has_value()) addValueWithMeta("is_collapsed", data.isCollapsed.value());
        if (data.isDocked.has_value()) addValueWithMeta("is_docked", data.isDocked.value());
        if (data.dockPriority.has_value()) addValueWithMeta("dock_priority", data.dockPriority.value());
        if (data.allowUndocking.has_value()) addValueWithMeta("allow_undocking", data.allowUndocking.value());
        if (data.autoScroll.has_value()) addValueWithMeta("auto_scroll", data.autoScroll.value());

        if (!window_j.empty()) windowsNode[name] = window_j;
    }
    if (!windowsNode.empty()) j["windows"] = windowsNode;
    return j;
}

// Helper to serialize keybinds settings from ManifestData to nlohmann::json.
nlohmann::json SerializeKeybinds(const ManifestData& manifest) {
    nlohmann::json keybinds;
    for (const auto& [group, actions] : manifest.keybinds.actions) {
        for (const auto& [name, defs] : actions) {
            nlohmann::json bindingsArray = nlohmann::json::array();
            for (const auto& def : defs) {
                nlohmann::json def_j;
                if (def.type.has_value()) def_j["type"] = def.type.value();
                if (def.key.has_value()) def_j["key"] = def.key.value();
                if (def.pressType.has_value()) def_j["press_type"] = def.pressType.value();
                if (def.pressThresholdMs.has_value()) def_j["press_threshold_ms"] = def.pressThresholdMs.value();
                if (def.consume.has_value()) def_j["consume"] = def.consume.value();
                if (def.behavior.has_value()) def_j["behavior"] = def.behavior.value();
                if (!def_j.empty()) bindingsArray.push_back(def_j);
            }
            
            if (!bindingsArray.empty()) {
                nlohmann::json actionObject;
                actionObject["bindings"] = bindingsArray;
                
                // Keybinds do not use fallback, they are unique to the plugin.
                for (const auto& meta : manifest.keybindsMetadata) {
                    if (meta.groupName == group && meta.actionName == name) {
                        InjectMetadata(actionObject, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
                        break;
                    }
                }
                keybinds[group][name] = actionObject;
            }
        }
    }
    return keybinds;
}




bool IsUserConfigAllowed(const SPF::Config::ManifestData& manifest) { return manifest.configPolicy.allowUserConfig.value_or(true); }

nlohmann::json GetSystemSettingsAsJson(const ManifestData& manifest, const std::string& systemName, const ManifestData& frameworkManifest) {
    if (systemName == "settings") {
        return SerializeSettings(manifest, frameworkManifest);
    } else if (systemName == "logging") {
        return SerializeLogging(manifest, frameworkManifest);
    } else if (systemName == "localization") {
        return SerializeLocalization(manifest, frameworkManifest);
    } else if (systemName == "ui") {
        return SerializeUI(manifest, frameworkManifest);
    } else if (systemName == "keybinds") {
        return SerializeKeybinds(manifest);
    }
    return nlohmann::json();
}

/**
 * @brief Recursively merges user JSON settings into default JSON settings, handling metadata.
 *
 * This function performs a deep merge of two JSON objects (`user` into `defaults`),
 * storing the result in `target`. It includes specific logic to handle conflicts,
 * type mismatches, and a custom `_value` metadata structure.
 *
 * @param target The JSON object where the merged result will be stored.
 * @param defaults The base JSON object containing default values and structure.
 * @param user The JSON object containing user-defined values that override defaults.
 * @param report An InitializationReport to log warnings (e.g., type mismatches).
 * @param componentName The name of the component whose settings are being merged.
 * @param currentPath The current path within the JSON hierarchy for logging purposes.
 */
void MergeJsonObjects(nlohmann::json& target, const nlohmann::json& defaults, const nlohmann::json& user, InitializationReport& report, const std::string& componentName,
                      const std::string& currentPath = "") {
    // Pass 1: Iterate through defaults to merge existing keys and apply defaults
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        const std::string& key = it.key();
        const auto& defaultValue = it.value();
        std::string newPath = currentPath.empty() ? key : currentPath + "." + key;

        if (user.contains(key)) {
            const auto& userValue = user[key];

            const bool defaultIsObj = defaultValue.is_object();
            const bool userIsObj = userValue.is_object();
            const bool defaultIsValueObj = defaultIsObj && defaultValue.contains("_value");
            const bool userIsValueObj = userIsObj && userValue.contains("_value");

            // Case 1: Both are "compatible" objects (both regular objects, or both value objects). Recurse.
            if (defaultIsObj && userIsObj && (defaultIsValueObj == userIsValueObj)) {
                target[key] = nlohmann::json::object();
                MergeJsonObjects(target[key], defaultValue, userValue, report, componentName, newPath);
            }
            // Case 2: Default is a value object, but user provided a simple, compatible value.
            else if (defaultIsValueObj && !userIsObj) {
                const auto& defaultInnerValue = defaultValue["_value"];
                if (defaultInnerValue.type() == userValue.type() || (defaultInnerValue.is_number() && userValue.is_number())) {
                    target[key] = defaultValue; // Copy default (with _meta)
                    target[key]["_value"] = userValue; // Overwrite with user's value
                } else {
                    // Type mismatch between user's simple value and the inner default value.
                    target[key] = defaultValue;
                    report.Warnings.push_back(InitializationReport::Issue{
                        fmt::format("Type mismatch for key '{}' in component '{}'. User value type '{}' is incompatible with internal default type '{}'. Using default.",
                                    newPath, componentName, userValue.type_name(), defaultInnerValue.type_name()),
                        componentName + "." + newPath});
                }
            }
            // Case 3: Both are simple, compatible types.
            else if (!defaultIsObj && !userIsObj && (defaultValue.type() == userValue.type() || (defaultValue.is_number() && userValue.is_number()))) {
                target[key] = userValue;
            }
            // Case 4: All other combinations are type mismatches.
            else {
                target[key] = defaultValue;
                report.Warnings.push_back(InitializationReport::Issue{
                    fmt::format("Type mismatch for key '{}' in component '{}'. Expected '{}' but got '{}'. Using default value.",
                                newPath, componentName, defaultValue.type_name(), userValue.type_name()),
                    componentName + "." + newPath});
            }
        } else {
            // If user config doesn't have the key, use the default.
            target[key] = defaultValue;
        }
    }

    // Pass 2: Iterate through user keys to add keys that don't exist in defaults
    for (auto it = user.begin(); it != user.end(); ++it) {
        const std::string& key = it.key();
        if (!defaults.contains(key)) {
            target[key] = it.value();
        }
    }
}
void StripMetadata(nlohmann::json& node) {
    if (node.is_object()) {
        // Handle _value first: if it's a _value object, replace it entirely
        if (node.contains("_value")) {
            node = node["_value"];
            // After replacing, the new node might itself be an object/array that needs stripping
            StripMetadata(node);
            return; // Done with this node after replacement
        }

        // If it's not a _value object, but contains _meta, remove _meta
        if (node.contains("_meta")) {
            node.erase("_meta");
        }

        // Recurse into remaining object items
        for (auto it = node.begin(); it != node.end(); ++it) {
            StripMetadata(it.value());
        }
    } else if (node.is_array()) {
        // Recurse into arrays
        for (auto& item : node) {
            StripMetadata(item);
        }
    }
}

}  // namespace

ConfigService::ConfigService(Events::EventManager& eventManager) : m_eventManager(eventManager) {
  m_systemStrategies["keybinds"] = MergeStrategy::PriorityMerge;
  m_systemStrategies["logging"] = MergeStrategy::Isolate;
  m_systemStrategies["localization"] = MergeStrategy::Isolate;
  m_systemStrategies["ui"] = MergeStrategy::Isolate;
  m_systemStrategies["settings"] = MergeStrategy::Isolate;
  m_systemStrategies["hooks"] = MergeStrategy::Isolate;
}

void ConfigService::RegisterPluginManifest(const std::string& pluginName, const ManifestData& manifest) { m_manifests[pluginName] = manifest; }

void ConfigService::ProcessAllSystemConfigurations(Core::InitializationReport& report) {
  report.InfoMessages.push_back("Processing all system configurations.");

  // --- Step 1: Collect all system names ---
  report.InfoMessages.push_back("-> Step 1/2: Collecting all system names from registered manifests...");
  std::set<std::string> allSystems;
  for (const auto& [componentName, manifest] : m_manifests) {
    allSystems.insert("info");
    allSystems.insert("config_policy");
    allSystems.insert("settings");
    allSystems.insert("logging");
    allSystems.insert("localization");
    allSystems.insert("keybinds");
    allSystems.insert("ui");
  }

  // --- Step 2: Process systems, ensuring \"settings\" comes first ---
  report.InfoMessages.push_back("-> Step 2/2: Processing configuration systems (settings first)...");
  if (allSystems.count("settings")) {
    AggregateIsolatedSystem("settings", report);
  }

  for (const auto& systemName : allSystems) {
    if (systemName == "settings") continue;  // Already processed

    auto it = m_systemStrategies.find(systemName);
    if (it == m_systemStrategies.end()) continue;

    switch (it->second) {
      case MergeStrategy::PriorityMerge:
        MergePrioritySystem(systemName, report);
        break;
      case MergeStrategy::Isolate:
        AggregateIsolatedSystem(systemName, report);
        break;
    }
  }
  CheckDirtyKeybinds(report);
  report.InfoMessages.push_back("Finished processing all system configurations.");
}

void ConfigService::Finalize(InitializationReport* report) {
  if (!report) return;
  report->ServiceName = "ConfigService";
  report->InfoMessages.push_back("Finalization sequence started.");

  // --- Step 1: Load framework manifest ---
  report->InfoMessages.push_back("-> Step 1/2: Loading framework manifest...");
  try {
    m_manifests["framework"] = GetFrameworkManifestData();
    report->InfoMessages.push_back("-> Framework manifest loaded from C++ structure.");
  } catch (const std::exception& e) {
    report->Errors.push_back({fmt::format("Failed to process in-code framework manifest: {}", e.what()), ""});
    return;
  }

  // --- Step 2: Process all system configurations (framework only initially) ---
  report->InfoMessages.push_back("-> Step 2/2: Processing initial system configurations (framework only)...");
  ProcessAllSystemConfigurations(*report);

  report->InfoMessages.push_back("Finalization sequence finished.");
}

void ConfigService::ReconcilePluginStates(const std::vector<std::string>& physicalPluginNames, Core::InitializationReport* report) {
  if (!report) return;
  report->ServiceName = "ConfigServiceReconciliation";

  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (logger) logger->Info("--- Reconciling Component States ---");

  // Clear previous info
  m_allComponentInfo.clear();

  bool configWasModified = false;

  std::vector<std::string> allComponentNames = {"framework"};
  allComponentNames.insert(allComponentNames.end(), physicalPluginNames.begin(), physicalPluginNames.end());

  if (!m_isolatedConfigs["settings"].count("framework")) {
    m_isolatedConfigs["settings"]["framework"] = nlohmann::json::object();
  }
  auto& frameworkSettings = m_isolatedConfigs.at("settings").at("framework");
  if (!frameworkSettings.contains("plugin_states")) {
    frameworkSettings["plugin_states"] = nlohmann::json::object();
  }
  auto& pluginStates = frameworkSettings.at("plugin_states");

  for (const auto& componentName : allComponentNames) {
    ComponentInfo info;
    info.name = componentName;
    info.isFramework = (componentName == "framework");

    if (m_manifests.count(componentName)) {
      const auto& manifest = m_manifests.at(componentName);
      const auto& manifestInfo = manifest.info;

      // Populate all fields from the manifest info block
      if (manifestInfo.name.has_value() && !manifestInfo.name->empty()) info.name = manifestInfo.name;
      info.version = manifestInfo.version;
      info.author = manifestInfo.author;
      info.descriptionKey = manifestInfo.descriptionKey;
      info.descriptionLiteral = manifestInfo.descriptionLiteral;
      info.email = manifestInfo.email;
      info.discordUrl = manifestInfo.discordUrl;
      info.steamProfileUrl = manifestInfo.steamProfileUrl;
      info.githubUrl = manifestInfo.githubUrl;
      info.youtubeUrl = manifestInfo.youtubeUrl;
      info.scsForumUrl = manifestInfo.scsForumUrl;
      info.patreonUrl = manifestInfo.patreonUrl;
      info.websiteUrl = manifestInfo.websiteUrl;
      
      info.hasInfo = info.author.has_value() || info.version.has_value();
      info.hasDescription = info.descriptionKey.has_value() || info.descriptionLiteral.has_value();

      // Access config policy directly
      info.allowUserConfig = manifest.configPolicy.allowUserConfig.value_or(true);
      info.configurableSystems = manifest.configPolicy.userConfigurableSystems;
      info.required_hooks = manifest.configPolicy.requiredHooks;

      info.hasSettings = info.allowUserConfig && !info.configurableSystems.empty();
    }

    if (!info.isFramework) {
      if (pluginStates.contains(componentName)) {
        info.isEnabled = pluginStates.at(componentName).value("enabled", false);
      } else {
        if (logger) logger->Info("New plugin '{}' found. Adding to config as disabled.", componentName);
        info.isEnabled = false;
        pluginStates[componentName] = {{"enabled", false}};
        configWasModified = true;
      }
    } else {
      info.isEnabled = true;  // Framework is always enabled
    }

    m_allComponentInfo[componentName] = info;
  }

  std::vector<std::string> orphanedPlugins;
  for (auto& [configuredPlugin, state] : pluginStates.items()) {
    if (configuredPlugin == "_meta") continue;
    if (std::find(physicalPluginNames.begin(), physicalPluginNames.end(), configuredPlugin) == physicalPluginNames.end()) {
      orphanedPlugins.push_back(configuredPlugin);
    }
  }

  if (!orphanedPlugins.empty()) {
    configWasModified = true;
    for (const auto& orphanName : orphanedPlugins) {
      if (logger) logger->Info("Removing orphaned plugin configuration for '{}'.", orphanName);
      pluginStates.erase(orphanName);
    }
  }

  if (configWasModified) {
    m_dirtyComponents.insert("framework");
    report->InfoMessages.push_back("Plugin states were modified (new plugins found or orphans removed).");
  }
  if (logger) logger->Info("--- Finished Reconciling Component States ---");

  BuildAggregatedUserSettings();
}



void ConfigService::ReconcileHookStates(const std::vector<Hooks::IHook*>& featureHooks, Core::InitializationReport* report) {
  if (!report) return;
  report->ServiceName = "ConfigServiceReconciliation";

  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (logger) logger->Info("--- Reconciling Hook States ---");

  if (!m_isolatedConfigs["settings"].count("framework")) {
    m_isolatedConfigs["settings"]["framework"] = nlohmann::json::object();
  }
  auto& frameworkSettings = m_isolatedConfigs.at("settings").at("framework");
  if (!frameworkSettings.contains("hook_states")) {
    frameworkSettings["hook_states"] = nlohmann::json::object();
  }
  auto& hookStates = frameworkSettings.at("hook_states");

  bool configWasModified = false;

  // Pass 1: Add new hooks to the config
  for (const auto* hook : featureHooks) {
    if (!hookStates.contains(hook->GetName())) {
      if (logger) logger->Info("New hook '{}' found. Adding to config with default state (Enabled: {}).", hook->GetName(), hook->IsEnabled());
      hookStates[hook->GetName()] = {{"enabled", hook->IsEnabled()}};
      configWasModified = true;
    }
  }

  // Pass 2: Remove orphaned hooks from the config
  std::vector<std::string> orphanedHooks;
  for (auto& [hookName, state] : hookStates.items()) {
    if (hookName == "_meta") continue;
    bool found = false;
    for (const auto* hook : featureHooks) {
      if (hook->GetName() == hookName) {
        found = true;
        break;
      }
    }
    if (!found) {
      orphanedHooks.push_back(hookName);
    }
  }

  if (!orphanedHooks.empty()) {
    configWasModified = true;
    for (const auto& orphanName : orphanedHooks) {
      if (logger) logger->Info("Removing orphaned hook configuration for '{}'.", orphanName);
      hookStates.erase(orphanName);
    }
  }

  if (configWasModified) {
    m_dirtyComponents.insert("framework");
    report->InfoMessages.push_back("Hook states were modified (new hooks found or orphans removed).");
  }

  if (logger) logger->Info("--- Finished Reconciling Hook States ---");
}

const std::map<std::string, ComponentInfo>& ConfigService::GetAllComponentInfo() const { return m_allComponentInfo; }

void ConfigService::AggregateIsolatedSystem(const std::string& systemName, InitializationReport& report) {
  m_isolatedConfigs[systemName] = {};
  const auto& frameworkManifest = m_manifests.at("framework");
  for (const auto& [componentName, manifest] : m_manifests) {
    nlohmann::json defaultSettings = GetSystemSettingsAsJson(manifest, systemName, frameworkManifest);
    if (defaultSettings.is_null()) continue;

    nlohmann::json finalConfig = defaultSettings;

    if (IsUserConfigAllowed(manifest)) {
      std::filesystem::path userConfigPath =
          (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

      if (std::filesystem::exists(userConfigPath)) {
        try {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            nlohmann::json userJson = nlohmann::json::parse(file);
            const auto* userSettings = GetSettings(userJson, systemName);
            if (userSettings) {
              size_t warningsBefore = report.Warnings.size();
              MergeJsonObjects(finalConfig, defaultSettings, *userSettings, report, componentName);
              size_t warningsAfter = report.Warnings.size();

              if (warningsAfter > warningsBefore) {
                m_dirtyComponents.insert(componentName);
              }
            }
          }
        } catch (const std::exception& e) {
          report.Warnings.push_back({fmt::format("Failed to read/parse user config for component '{}'. Using defaults. Error: {}", componentName, e.what()), ""});
          m_dirtyComponents.insert(componentName);
          m_corruptedFilePaths.insert(userConfigPath.string());
          finalConfig = defaultSettings;
        }
      } else {
        m_dirtyComponents.insert(componentName);
      }
    }
    m_isolatedConfigs[systemName][componentName] = finalConfig;
  }
}

/**
 * @brief Merges configurations for a system using the PriorityMerge strategy (e.g., keybinds).
 *
 * This method aggregates keybind definitions from manifests and user settings across
 * all components (framework and plugins) based on a defined priority order.
 * It handles conflicts by prioritizing active components and prevents duplicate key assignments.
 * The process involves multiple passes to ensure correct merging and metadata injection.
 *
 * @param systemName The name of the system to merge (e.g., "keybinds").
 * @param report An InitializationReport to log information, warnings, and errors.
 */
void ConfigService::MergePrioritySystem(const std::string& systemName, InitializationReport& report) {
  nlohmann::json finalConfig = nlohmann::json::object();
  std::vector<nlohmann::json> usedKeyValues;
  m_keybindOwnership.clear();

  report.InfoMessages.push_back(fmt::format("Starting priority merge for system: '{}'", systemName));

  // Helper to check if a plugin is enabled, based on the already-loaded settings config.
  auto isPluginActive = [&](const std::string& pluginName) -> bool {
    if (m_isolatedConfigs.count("settings") && m_isolatedConfigs.at("settings").count("framework")) {
      const auto& frameworkSettings = m_isolatedConfigs.at("settings").at("framework");
      if (frameworkSettings.contains("plugin_states") && frameworkSettings.at("plugin_states").contains(pluginName)) {
        return frameworkSettings.at("plugin_states").at(pluginName).value("enabled", false);
      }
    }
    // Default to false if not found in config
    return false;
  };

  std::vector<std::string> pluginComponents;
  for (const auto& [name, manifest] : m_manifests) {
    if (name != "framework") {
      pluginComponents.push_back(name);
    }
  }
  std::sort(pluginComponents.begin(), pluginComponents.end());

  auto process_source = [&](const nlohmann::json& settings, const std::string& componentName) {
    if (settings.is_null()) return;

    for (const auto& group : settings.items()) {
      if (!group.value().is_object()) continue;

      for (const auto& action : group.value().items()) {
        std::string fullActionKey = group.key() + "." + action.key();
        m_keybindOwnership.try_emplace(fullActionKey, componentName);

        if (finalConfig.contains(group.key()) && finalConfig[group.key()].contains(action.key())) {
          continue;
        }

        const auto& actionNode = action.value();
        const nlohmann::json* keys_to_assign = nullptr;

        if (actionNode.is_object() && actionNode.contains("bindings")) {
            keys_to_assign = &actionNode["bindings"];
        } else if (actionNode.is_array()) {
            keys_to_assign = &actionNode;
        }

        if (!keys_to_assign || !keys_to_assign->is_array()) continue;

        nlohmann::json successful_keys = nlohmann::json::array();

        for (const auto& key_value : *keys_to_assign) {
          bool conflict = false;
          try {
              auto new_input = Modules::InputFactory::CreateFromJson(key_value);
              if (new_input) {
                  for (const auto& used_binding : usedKeyValues) {
                      auto existing_input = Modules::InputFactory::CreateFromJson(used_binding);
                      if (existing_input && new_input->IsSameAs(*existing_input)) {
                          conflict = true;
                          break;
                      }
                  }
              }
          } catch (const std::exception& e) {
              report.Warnings.push_back({fmt::format("Could not parse binding '{}' for action '{}' in component '{}'. Error: {}",
                                                     key_value.dump(), fullActionKey, componentName, e.what()),
                                           fullActionKey});
              continue; // Skip this invalid binding
          }

          if (conflict) {
            m_dirtyComponents.insert(componentName);
            report.Warnings.push_back({fmt::format("Keybind conflict for action '{}' in component '{}'. The key '{}' is already taken. This binding will be ignored.",
                                                   fullActionKey,
                                                   componentName,
                                                   key_value.dump()),
                                       fullActionKey});
          } else {
            successful_keys.push_back(key_value);
            usedKeyValues.push_back(key_value);
          }
        }

        if (!successful_keys.empty()) {
          if (!finalConfig.contains(group.key())) {
            finalConfig[group.key()] = nlohmann::json::object();
          }
          // We need to reconstruct the full action object here, not just the bindings array
          nlohmann::json finalActionObject;
          if(actionNode.is_object() && actionNode.contains("_meta")) {
            finalActionObject["_meta"] = actionNode["_meta"];
          }
          finalActionObject["bindings"] = successful_keys;
          finalConfig[group.key()][action.key()] = finalActionObject;
        }
      }
    }
  };

  // --- PROCESSING USER SETTINGS ---
  report.InfoMessages.push_back("Pass 1: Processing user settings for framework...");
  if (m_manifests.count("framework") && IsUserConfigAllowed(m_manifests.at("framework"))) {
    auto frameworkUserConfigPath = PathManager::GetConfigFilePath("framework_settings.json");
    if (std::filesystem::exists(frameworkUserConfigPath)) {
      try {
        std::ifstream file(frameworkUserConfigPath);
        if (file.peek() != std::ifstream::traits_type::eof()) {
          nlohmann::json userJson = nlohmann::json::parse(file);
          const auto* userSettings = GetSettings(userJson, systemName);
          if (userSettings) {
            process_source(*userSettings, "framework");
          }
        }
      } catch (const std::exception& e) {
        report.Warnings.push_back({fmt::format("Failed to parse user config for framework. Error: {}", e.what()), ""});
      }
    }
  }

  report.InfoMessages.push_back("Pass 2: Processing user settings for ACTIVE plugins...");
  for (const auto& componentName : pluginComponents) {
    if (isPluginActive(componentName) && m_manifests.count(componentName) && IsUserConfigAllowed(m_manifests.at(componentName))) {
      auto userConfigPath = PathManager::GetPluginConfigDir(componentName) / "settings.json";
      if (std::filesystem::exists(userConfigPath)) {
        try {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            nlohmann::json userJson = nlohmann::json::parse(file);
            const auto* userSettings = GetSettings(userJson, systemName);
            if (userSettings) {
                process_source(*userSettings, componentName);
            }
          }
        } catch (const std::exception& e) {
          report.Warnings.push_back({fmt::format("Failed to parse user config for plugin '{}'. Error: {}", componentName, e.what()), ""});
        }
      }
    }
  }

  report.InfoMessages.push_back("Pass 3: Processing manifest for framework...");
  if (m_manifests.count("framework")) {
    process_source(GetSystemSettingsAsJson(m_manifests.at("framework"), systemName, m_manifests.at("framework")), "framework");
  }

  report.InfoMessages.push_back("Pass 4: Processing manifests for ACTIVE plugins...");
  for (const auto& componentName : pluginComponents) {
    if (isPluginActive(componentName) && m_manifests.count(componentName)) {
      process_source(GetSystemSettingsAsJson(m_manifests.at(componentName), systemName, m_manifests.at("framework")), componentName);
    }
  }

  // --- PASS 2: DISABLED PLUGINS (SOFT RESERVATION) ---
  report.InfoMessages.push_back("Pass 5: Processing user settings for DISABLED plugins...");
  for (const auto& componentName : pluginComponents) {
    if (!isPluginActive(componentName) && m_manifests.count(componentName) && IsUserConfigAllowed(m_manifests.at(componentName))) {
      auto userConfigPath = PathManager::GetPluginConfigDir(componentName) / "settings.json";
      if (std::filesystem::exists(userConfigPath)) {
        try {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            nlohmann::json userJson = nlohmann::json::parse(file);
            const auto* userSettings = GetSettings(userJson, systemName);
            if (userSettings) {
                process_source(*userSettings, componentName);
            }
          }
        } catch (const std::exception& e) {
          report.Warnings.push_back({fmt::format("Failed to parse user config for plugin '{}'. Error: {}", componentName, e.what()), ""});
        }
      }
    }
  }

  report.InfoMessages.push_back("Pass 6: Processing manifests for DISABLED plugins...");
  for (const auto& componentName : pluginComponents) {
    if (!isPluginActive(componentName) && m_manifests.count(componentName)) {
      process_source(GetSystemSettingsAsJson(m_manifests.at(componentName), systemName, m_manifests.at("framework")), componentName);
    }
  }

  report.InfoMessages.push_back("Finalizing: ensuring all owned actions exist in the final config...");
  for (const auto& [actionKey, owner] : m_keybindOwnership) {
    size_t lastDot = actionKey.rfind('.');
    if (lastDot == std::string::npos) continue;
    std::string groupName = actionKey.substr(0, lastDot);
    std::string actionName = actionKey.substr(lastDot + 1);

    if (!finalConfig.contains(groupName) || !finalConfig[groupName].contains(actionName)) {
      if (!finalConfig.contains(groupName)) {
        finalConfig[groupName] = nlohmann::json::object();
      }
      finalConfig[groupName][actionName] = {{"bindings", nlohmann::json::array()}};
    }
  }

  // Final pass to inject metadata into the merged keybinds
  for (const auto& [componentName, manifest] : m_manifests) {
    for (const auto& meta : manifest.keybindsMetadata) {
        if (finalConfig.contains(meta.groupName) && finalConfig[meta.groupName].contains(meta.actionName)) {
            auto& actionNode = finalConfig[meta.groupName][meta.actionName];
            
            // If the node is an object and doesn't already have metadata, inject it.
            // This ensures that actions defined in user files (which don't have meta) get it from the manifest.
            if (actionNode.is_object() && !actionNode.contains("_meta")) {
                InjectMetadata(actionNode, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
            }
        }
    }
  }


  m_mergedConfigs[systemName] = finalConfig;
}

const nlohmann::json* ConfigService::GetMergedConfig(const std::string& systemName) const {
  auto it = m_mergedConfigs.find(systemName);
  return (it != m_mergedConfigs.end()) ? &it->second : nullptr;
}

const std::map<std::string, nlohmann::json>* ConfigService::GetAllComponentSettings(const std::string& systemName) const {
  auto it = m_isolatedConfigs.find(systemName);
  return (it != m_isolatedConfigs.end()) ? &it->second : nullptr;
}

void ConfigService::SetValue(const std::string& componentName, const std::string& jsonPath, const nlohmann::json& value) {
  size_t firstDot = jsonPath.find('.');
  if (firstDot == std::string::npos) return;

  std::string systemName = jsonPath.substr(0, firstDot);
  std::string keyPath = jsonPath.substr(firstDot + 1);

  auto strategyIt = m_systemStrategies.find(systemName);
  if (strategyIt == m_systemStrategies.end()) return;

  try {
    if (strategyIt->second == MergeStrategy::Isolate) {
      if (!m_isolatedConfigs.contains(systemName) || !m_isolatedConfigs[systemName].contains(componentName)) return;

      // Update the raw config data
      std::string pointerPathStr = "/" + std::regex_replace(keyPath, std::regex("\\."), "/");
      nlohmann::json::json_pointer ptr(pointerPathStr);

      // Check if the target node is a _value object and update it correctly
      auto& targetNode = m_isolatedConfigs[systemName][componentName][ptr];
      if (targetNode.is_object() && targetNode.contains("_value")) {
          if (value.is_object() && value.contains("_value")) {
            targetNode["_value"] = value["_value"];
          } else {
            targetNode["_value"] = value;
          }
      } else {
          targetNode = value;
      }

      // Perform a targeted update on the aggregated map as well
      auto& aggregatedTargetNode = m_aggregatedUserSettings[componentName][systemName][ptr];
      if (aggregatedTargetNode.is_object() && aggregatedTargetNode.contains("_value")) {
          aggregatedTargetNode["_value"] = value;
      } else {
          aggregatedTargetNode = value;
      }

      if (IsUserConfigAllowed(m_manifests.at(componentName))) {
        m_dirtyComponents.insert(componentName);
      }
    } else  // PriorityMerge
    {
      if (!m_mergedConfigs.contains(systemName)) return;

      std::string groupName = keyPath;
      std::string actionName;
      size_t index = 0;
      bool hasIndex = false;

      size_t bracketPos = groupName.find('[');
      if (bracketPos != std::string::npos) {
        hasIndex = true;
        try {
          index = std::stoul(groupName.substr(bracketPos + 1));
          groupName = groupName.substr(0, bracketPos);
        } catch (const std::exception&) {
          return;
        }
      }

      size_t lastDotPos = groupName.rfind('.');
      if (lastDotPos != std::string::npos) {
        actionName = groupName.substr(lastDotPos + 1);
        groupName = groupName.substr(0, lastDotPos);
      } else {
        return;
      }

      if (m_mergedConfigs[systemName].contains(groupName) && m_mergedConfigs[systemName][groupName].contains(actionName)) {
        if (hasIndex) {
          if (m_mergedConfigs[systemName][groupName][actionName].is_array()) {
            m_mergedConfigs[systemName][groupName][actionName][index] = value;
          }
        } else {
          m_mergedConfigs[systemName][groupName][actionName] = value;
        }
        if (IsUserConfigAllowed(m_manifests.at(componentName))) {
          m_dirtyComponents.insert(componentName);
        }
      }

    }

    // After any successful change, fire an event so other systems can react.
    m_eventManager.System.OnSettingWasChanged.Call({systemName, componentName, keyPath, value});
  } catch (const std::exception& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
    if (logger) logger->Error("Failed to set value for path '{}': {}", jsonPath, e.what());
  }
}

void ConfigService::ResetToDefault(const std::string& systemName, const std::string& keyPathWithComponent, InitializationReport* report) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (logger) logger->Debug("Attempting to reset key. System: '{}', KeyPath: '{}'", systemName, keyPathWithComponent);

  std::string originalComponent;
  nlohmann::json defaultValue;
  bool found = false;

  auto strategyIt = m_systemStrategies.find(systemName);
  if (strategyIt == m_systemStrategies.end()) return;

  std::string componentNameForSet;
  std::string jsonPathForSet;

  if (strategyIt->second == MergeStrategy::Isolate) {
    size_t firstDot = keyPathWithComponent.find('.');
    if (firstDot == std::string::npos) {
      if (report) {
        report->Errors.push_back({fmt::format("Invalid key path '{}' for isolated system '{}': missing component name.", keyPathWithComponent, systemName), ""});
      }
      return;
    }
    std::string componentName = keyPathWithComponent.substr(0, firstDot);
    std::string keyPath = keyPathWithComponent.substr(firstDot + 1);

    if (m_manifests.count(componentName)) {
      nlohmann::json defaultSettings = GetSystemSettingsAsJson(m_manifests.at(componentName), systemName, m_manifests.at("framework"));
      if (!defaultSettings.is_null()) {
        std::string pointerPath = "/" + std::regex_replace(keyPath, std::regex("\\."), "/");
        try {
          defaultValue = defaultSettings.at(nlohmann::json::json_pointer(pointerPath));
          originalComponent = componentName;
          found = true;
        } catch (const std::exception&) {
          found = false;
        }
      }
    }
    if (found) {
      componentNameForSet = originalComponent;
      jsonPathForSet = systemName + "." + keyPath;
    }
  } else  // PriorityMerge
  {
    std::string actionKey = keyPathWithComponent;
    size_t bracketPos = actionKey.find('[');
    if (bracketPos != std::string::npos) {
      actionKey = actionKey.substr(0, bracketPos);
    }

    auto ownerIt = m_keybindOwnership.find(actionKey);
    if (ownerIt != m_keybindOwnership.end()) {
      componentNameForSet = ownerIt->second;
      if (logger) logger->Debug("  -> Found owner '{}' for action '{}' in ownership map.", componentNameForSet, actionKey);
    } else {
      if (report) {
        report->Errors.push_back({fmt::format("Could not reset key '{}': owner not found in ownership map.", actionKey), ""});
      }
      return;
    }

    std::string groupName = actionKey;
    std::string actionName;
    size_t dotPos = groupName.rfind('.');
    if (dotPos != std::string::npos) {
      actionName = groupName.substr(dotPos + 1);
      groupName = groupName.substr(0, dotPos);
    } else {
      actionName = groupName;
      groupName = "";
    }

    for (const auto& [compName, manifest] : m_manifests) {
      nlohmann::json defaultSettings = GetSystemSettingsAsJson(manifest, systemName, m_manifests.at("framework"));
      if (!defaultSettings.is_null() && defaultSettings.contains(groupName) && defaultSettings[groupName].contains(actionName)) {
        const auto& settingValue = defaultSettings[groupName][actionName];

        defaultValue = settingValue;
        found = true;

        if (found) {
          originalComponent = compName;
          if (logger) logger->Debug("  -> Found default value in manifest of '{}': {}", compName, defaultValue.dump());
          break;
        }
      }
    }

    if (found) {
      // Use the actionKey which has the [index] stripped, to reset the whole action array.
      jsonPathForSet = systemName + "." + actionKey;
    }
  }

  if (found) {
    if (logger) logger->Debug("  -> Resetting. Component: '{}', Path: '{}', Value: {}", componentNameForSet, jsonPathForSet, defaultValue.dump());
    SetValue(componentNameForSet, jsonPathForSet, defaultValue);
    if (report) {
      report->InfoMessages.push_back(fmt::format("Successfully reset key '{}' for component '{}' to its default value.", keyPathWithComponent, componentNameForSet));
    }
  } else {
    if (report) {
      report->Errors.push_back({fmt::format("Could not reset key '{}': Not found in any manifest.", keyPathWithComponent), ""});
    }
  }
}

void ConfigService::UpdateBinding(const std::string& actionFullName, const nlohmann::json& originalBinding, const nlohmann::json& newBinding,
                                  const std::optional<std::pair<std::string, nlohmann::json>>& bindingToClear) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

    // 1. Find owner of the action being changed. This is for marking dirty files.
    auto ownerIt = m_keybindOwnership.find(actionFullName);
    if (ownerIt == m_keybindOwnership.end()) {
        if (logger) logger->Error("UpdateBinding failed: Could not find owner for action '{}'.", actionFullName);
        return;
    }
    const std::string& componentName = ownerIt->second;

    // 2. Parse action name
    size_t lastDot = actionFullName.rfind('.');
    if (lastDot == std::string::npos) {
        if (logger) logger->Error("UpdateBinding failed: Invalid action name format for '{}'.", actionFullName);
        return;
    }
    std::string groupName = actionFullName.substr(0, lastDot);
    std::string actionName = actionFullName.substr(lastDot + 1);

    // 3. Find the target bindings array in the merged config
    if (!m_mergedConfigs.count("keybinds") || !m_mergedConfigs["keybinds"].contains(groupName) || !m_mergedConfigs["keybinds"][groupName].contains(actionName)) {
        if (logger) logger->Warn("UpdateBinding: Action '{}' not found in merged config.", actionFullName);
        return;
    }
    auto& actionObject = m_mergedConfigs["keybinds"][groupName][actionName];
    if (!actionObject.is_object() || !actionObject.contains("bindings") || !actionObject["bindings"].is_array()) {
      if (logger) logger->Error("UpdateBinding failed: 'bindings' array not found or is not an array for action '{}'.", actionFullName);
      return;
    }
    auto& bindingsArray = actionObject["bindings"];

    // 4. Create a complete binding object by merging UI changes with manifest defaults
    nlohmann::json finalNewBinding = newBinding;
    const auto& ownerManifest = m_manifests.at(componentName);
    if (ownerManifest.keybinds.actions.count(groupName) && ownerManifest.keybinds.actions.at(groupName).count(actionName)) {
        const auto& defaultBindings = ownerManifest.keybinds.actions.at(groupName).at(actionName);
        if (!defaultBindings.empty()) {
            // Assume the first default binding is the template for defaults.
            const auto& defaultBindingDef = defaultBindings[0];
            nlohmann::json defaultJson;
            if (defaultBindingDef.behavior.has_value()) defaultJson["behavior"] = defaultBindingDef.behavior.value();
            if (defaultBindingDef.consume.has_value()) defaultJson["consume"] = defaultBindingDef.consume.value();
            if (defaultBindingDef.pressThresholdMs.has_value()) defaultJson["press_threshold_ms"] = defaultBindingDef.pressThresholdMs.value();
            // etc. for other defaultable fields...

            // Start with the defaults, then overwrite them with the values from the UI.
            nlohmann::json completeBinding = defaultJson;
            completeBinding.merge_patch(newBinding);
            finalNewBinding = completeBinding;
        }
    }

    // 5. Add or Update the binding in the target array
    if (originalBinding.empty()) { // Add new binding
        bindingsArray.push_back(finalNewBinding);
    } else { // Update existing binding
        bool found = false;
        for (auto& binding : bindingsArray) {
            // This simple comparison is OK here because the UI passes the exact original JSON object.
            if (binding == originalBinding) {
                binding = finalNewBinding;
                found = true;
                break;
            }
        }
        if (!found) {
            if (logger) logger->Warn("UpdateBinding: Could not find original binding for action '{}' to update. Adding as new.", actionFullName);
            bindingsArray.push_back(finalNewBinding);
        }
    }

    // Mark the owner component as dirty
    m_dirtyComponents.insert(componentName);

    // 5. Handle clearing the binding from the conflicting action
    if (bindingToClear.has_value()) {
        const auto& [conflictingAction, bindingJsonToClear] = bindingToClear.value();
        _DeleteBindingInternal(conflictingAction, bindingJsonToClear);
    }

    m_eventManager.System.OnKeybindsModified.Call({});
}

bool ConfigService::_DeleteBindingInternal(const std::string& actionFullName, const nlohmann::json& bindingToDelete) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

    auto ownerIt = m_keybindOwnership.find(actionFullName);
    if (ownerIt == m_keybindOwnership.end()) {
        if (logger) logger->Error("_DeleteBindingInternal failed: Could not find owner for action '{}'.", actionFullName);
        return false;
    }
    const std::string& componentName = ownerIt->second;

    size_t lastDot = actionFullName.rfind('.');
    if (lastDot == std::string::npos) {
        if (logger) logger->Error("_DeleteBindingInternal failed: Invalid action name format '{}'.", actionFullName);
        return false;
    }
    std::string groupName = actionFullName.substr(0, lastDot);
    std::string actionName = actionFullName.substr(lastDot + 1);

    if (m_mergedConfigs.count("keybinds") && m_mergedConfigs["keybinds"].contains(groupName) && m_mergedConfigs["keybinds"][groupName].contains(actionName)) {
        auto& actionObject = m_mergedConfigs["keybinds"][groupName][actionName];
        if (actionObject.is_object() && actionObject.contains("bindings") && actionObject["bindings"].is_array()) {
            auto& bindingsArray = actionObject["bindings"];
            
            std::unique_ptr<Modules::IBindableInput> inputToDelete;
            try { inputToDelete = Modules::InputFactory::CreateFromJson(bindingToDelete); } catch(...) {}

            if (inputToDelete) {
                for (auto it = bindingsArray.begin(); it != bindingsArray.end(); ++it) {
                    try {
                        auto storedInput = Modules::InputFactory::CreateFromJson(*it);
                        if (storedInput && storedInput->IsSameAs(*inputToDelete)) {
                            bindingsArray.erase(it);
                            m_dirtyComponents.insert(componentName);
                            if (logger) logger->Info("_DeleteBindingInternal: Removed binding '{}' from action '{}'. Component '{}' marked as dirty.", bindingToDelete.dump(), actionFullName, componentName);
                            return true; // Success
                        }
                    } catch(...) {}
                }
            }
        }
    }

    if (logger) logger->Warn("_DeleteBindingInternal: Could not find binding '{}' in action '{}' to delete.", bindingToDelete.dump(), actionFullName);
    return false; // Failure
}

void ConfigService::DeleteBinding(const std::string& actionFullName, const nlohmann::json& bindingToDelete) {
    if (_DeleteBindingInternal(actionFullName, bindingToDelete)) {
        m_eventManager.System.OnKeybindsModified.Call({});
    }
}

void ConfigService::UpdateBindingProperty(const std::string& actionFullName, const nlohmann::json& originalBinding, const std::string& propertyName,
                                          const nlohmann::json& newValue) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

    auto ownerIt = m_keybindOwnership.find(actionFullName);
    if (ownerIt == m_keybindOwnership.end()) {
        if (logger) logger->Error("UpdateBindingProperty failed: Could not find owner for action '{}'.", actionFullName);
        return;
    }
    const std::string& componentName = ownerIt->second;

    size_t lastDot = actionFullName.rfind('.');
    if (lastDot == std::string::npos) return;
    std::string groupName = actionFullName.substr(0, lastDot);
    std::string actionName = actionFullName.substr(lastDot + 1);

    if (m_mergedConfigs.count("keybinds") && m_mergedConfigs["keybinds"].contains(groupName) && m_mergedConfigs["keybinds"][groupName].contains(actionName)) {
        auto& actionObject = m_mergedConfigs["keybinds"][groupName][actionName];
        if (actionObject.is_object() && actionObject.contains("bindings") && actionObject["bindings"].is_array()) {
            auto& bindingsArray = actionObject["bindings"];

            std::unique_ptr<Modules::IBindableInput> inputToFind;
            try { inputToFind = Modules::InputFactory::CreateFromJson(originalBinding); } catch(...) {}

            if (inputToFind) {
                for (auto& binding : bindingsArray) {
                    try {
                        auto storedInput = Modules::InputFactory::CreateFromJson(binding);
                        if (storedInput && storedInput->IsSameAs(*inputToFind)) {
                            binding[propertyName] = newValue;
                            m_dirtyComponents.insert(componentName);
                            m_eventManager.System.OnKeybindsModified.Call({});
                            if (logger) logger->Info("UpdateBindingProperty: Updated property '{}' for binding in action '{}'.", propertyName, actionFullName);
                            return;
                        }
                    } catch(...) {}
                }
            }
        }
    }

    if (logger) logger->Warn("UpdateBindingProperty: Could not find binding to update property '{}' for in action '{}'.", propertyName, actionFullName);
}

/**
 * @brief Saves all modified ("dirty") configurations to their respective user setting files.
 *
 * This function iterates through all components marked as dirty. For each one, it
 * reconstructs the complete user settings JSON object by combining data from
 * _ISOLATED_ and _MERGED_ in-memory configurations, strips all `_meta` and `_value` structures,
 * and writes the result to the appropriate `settings.json` file.
 */
void ConfigService::SaveAllDirty() {
  if (m_dirtyComponents.empty()) return;

  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  logger->Info("--- Saving all dirty configurations to disk ---");

  for (const auto& componentName : m_dirtyComponents) {
    std::filesystem::path userConfigPath =
        (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

    nlohmann::json fullConfigToSave;
    try {
      // If the file was corrupted, we start fresh. Otherwise, load existing content to preserve other settings.
      if (m_corruptedFilePaths.find(userConfigPath.string()) == m_corruptedFilePaths.end()) {
        if (std::filesystem::exists(userConfigPath)) {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            fullConfigToSave = nlohmann::json::parse(file);
          }
        }
      }

      // --- Save ISOLATED systems for this component ---
      for (const auto& [systemName, components] : m_isolatedConfigs) {
        if (components.count(componentName)) {
          fullConfigToSave[systemName] = components.at(componentName);
        }
      }

      // --- Save MERGED systems (keybinds) for this component ---
      nlohmann::json keybindsToSave = nlohmann::json::object();
      const auto* keybindsConfig = GetMergedConfig("keybinds");

      if (keybindsConfig) {
        // Iterate over all known keybinds, not the merged config itself
        for (const auto& [fullActionKey, owner] : m_keybindOwnership) {
          if (owner == componentName) {
            size_t lastDot = fullActionKey.rfind('.');
            if (lastDot == std::string::npos) continue;
            std::string groupName = fullActionKey.substr(0, lastDot);
            std::string actionName = fullActionKey.substr(lastDot + 1);

            // Find the final value of this keybind in the merged config
            if (keybindsConfig->contains(groupName) && (*keybindsConfig)[groupName].contains(actionName)) {
              keybindsToSave[groupName][actionName] = (*keybindsConfig)[groupName][actionName];
            }
          }
        }
      }

      if (!keybindsToSave.empty()) {
        fullConfigToSave["keybinds"] = keybindsToSave;
      }

      if (fullConfigToSave.empty()) continue;

      // Strip all metadata before saving to file
      StripMetadata(fullConfigToSave);

      std::filesystem::create_directories(userConfigPath.parent_path());
      std::ofstream outFile(userConfigPath);
      outFile << fullConfigToSave.dump(4);
      if (logger) logger->Info("Saved configuration for '{}' to {}", componentName, userConfigPath.string());
    } catch (const std::exception& e) {
      if (logger) logger->Error("Failed to save config file for '{}' to {}. Error: {}", componentName, userConfigPath.string(), e.what());
    }
  }

  m_dirtyComponents.clear();
  logger->Info("--- Finished saving dirty configurations ---");
}

nlohmann::json ConfigService::GetValue(const std::string& componentName, const std::string& keyPath, const nlohmann::json& defaultValue) const {
  if (keyPath.rfind("info.", 0) == 0) {
    auto manifestIt = m_manifests.find(componentName);
    if (manifestIt != m_manifests.end()) {
      const auto& info = manifestIt->second.info;
      std::string subKey = keyPath.substr(5); // Length of "info."
      if (subKey == "name" && info.name.has_value()) return info.name.value();
      if (subKey == "version" && info.version.has_value()) return info.version.value();
      if (subKey == "author" && info.author.has_value()) return info.author.value();
      if (subKey == "description_key" && info.descriptionKey.has_value()) return info.descriptionKey.value();
      if (subKey == "description_literal" && info.descriptionLiteral.has_value()) return info.descriptionLiteral.value();
      if (subKey == "email" && info.email.has_value()) return info.email.value();
      if (subKey == "discordUrl" && info.discordUrl.has_value()) return info.discordUrl.value();
      if (subKey == "steamProfileUrl" && info.steamProfileUrl.has_value()) return info.steamProfileUrl.value();
      if (subKey == "githubUrl" && info.githubUrl.has_value()) return info.githubUrl.value();
      if (subKey == "youtubeUrl" && info.youtubeUrl.has_value()) return info.youtubeUrl.value();
      if (subKey == "scsForumUrl" && info.scsForumUrl.has_value()) return info.scsForumUrl.value();
      if (subKey == "patreonUrl" && info.patreonUrl.has_value()) return info.patreonUrl.value();
      if (subKey == "websiteUrl" && info.websiteUrl.has_value()) return info.websiteUrl.value();

    }
    return defaultValue;
  }

  size_t firstDot = keyPath.find('.');
  if (firstDot == std::string::npos) {
    return defaultValue;
  }

  std::string systemName = keyPath.substr(0, firstDot);
  std::string restOfPath = keyPath.substr(firstDot + 1);

  auto strategyIt = m_systemStrategies.find(systemName);
  if (strategyIt == m_systemStrategies.end()) {
    return defaultValue;
  }

  const nlohmann::json* configRoot = nullptr;
  if (strategyIt->second == MergeStrategy::Isolate) {
    auto isolatedSystemIt = m_isolatedConfigs.find(systemName);
    if (isolatedSystemIt != m_isolatedConfigs.end()) {
      auto componentIt = isolatedSystemIt->second.find(componentName);
      if (componentIt != isolatedSystemIt->second.end()) {
        configRoot = &componentIt->second;
      }
    }
  } else {
    auto mergedSystemIt = m_mergedConfigs.find(systemName);
    if (mergedSystemIt != m_mergedConfigs.end()) {
      configRoot = &mergedSystemIt->second;
    }
  }

  if (!configRoot) {
    return defaultValue;
  }

  try {
    std::string pointerPath = "/" + std::regex_replace(restOfPath, std::regex("\\."), "/");
    nlohmann::json::json_pointer ptr(pointerPath);
    return configRoot->at(ptr);
  } catch (const nlohmann::json::out_of_range&) {
    return defaultValue;
  } catch (const nlohmann::json::parse_error&) {
    return defaultValue;
  }
}

void ConfigService::CheckDirtyKeybinds(InitializationReport& report) {
  const auto* keybindsConfig = GetMergedConfig("keybinds");
  if (!keybindsConfig) return;

  report.InfoMessages.push_back("Checking for outdated keybind configurations...");

  for (const auto& [componentName, manifest] : m_manifests) {
    if (!IsUserConfigAllowed(manifest)) continue;

    std::filesystem::path userConfigPath =
        (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

    if (!std::filesystem::exists(userConfigPath)) continue;

    nlohmann::json userJson;
    try {
      std::ifstream file(userConfigPath);
      if (file.peek() != std::ifstream::traits_type::eof()) {
        userJson = nlohmann::json::parse(file);
      }
    } catch (...) {
      continue;  // File is likely corrupted, already marked as dirty
    }

    const nlohmann::json* originalUserBinds = GetSettings(userJson, "keybinds");

    nlohmann::json newBinds = nlohmann::json::object();
    for (const auto& [fullActionKey, owner] : m_keybindOwnership) {
      if (owner == componentName) {
        size_t lastDot = fullActionKey.rfind('.');
        if (lastDot == std::string::npos) continue;
        std::string groupName = fullActionKey.substr(0, lastDot);
        std::string actionName = fullActionKey.substr(lastDot + 1);

        if (keybindsConfig->contains(groupName) && (*keybindsConfig)[groupName].contains(actionName)) {
          if (!newBinds.contains(groupName)) newBinds[groupName] = nlohmann::json::object();
          newBinds[groupName][actionName] = (*keybindsConfig)[groupName][actionName];
        }
      }
    }

    if (!originalUserBinds && !newBinds.empty()) {
      report.InfoMessages.push_back(fmt::format("Keybind config for '{}' is missing from user file but should exist. Marking as dirty.", componentName));
      m_dirtyComponents.insert(componentName);
    } else if (originalUserBinds) {
      if (*originalUserBinds != newBinds) {
        report.InfoMessages.push_back(fmt::format("Keybind config for '{}' is outdated. Marking as dirty.", componentName));
        m_dirtyComponents.insert(componentName);
      }
    }
  }
}

void ConfigService::BuildAggregatedUserSettings() {
  m_aggregatedUserSettings.clear();

  // Iterate through all components that have been processed
  for (const auto& [componentName, info] : m_allComponentInfo) {
    // We only care about components that can be configured by the user and have specified systems
    if (!info.allowUserConfig || info.configurableSystems.empty()) {
      continue;
    }

    for (const auto& systemName : info.configurableSystems) {
      auto strategyIt = m_systemStrategies.find(systemName);
      if (strategyIt == m_systemStrategies.end() || strategyIt->second != MergeStrategy::Isolate) {
        continue;  // Only handle isolated systems here for now
      }

      const auto* allSystemSettings = GetAllComponentSettings(systemName);
      if (!allSystemSettings) continue;

      auto componentSettingsIt = allSystemSettings->find(componentName);
      if (componentSettingsIt != allSystemSettings->end()) {
        m_aggregatedUserSettings[componentName][systemName] = componentSettingsIt->second;
      }
    }
  }
}

const std::map<std::string, nlohmann::json>& ConfigService::GetAggregatedUserSettings() const { return m_aggregatedUserSettings; }

std::string ConfigService::GetOrCreateFrameworkInstanceId() {
    const std::string keyPath = "settings.framework.framework_instance_id";

    // 1. Try to get the existing ID
    nlohmann::json existingIdJson = GetValue("framework", keyPath, "");
    if (existingIdJson.is_string()) {
        std::string existingId = existingIdJson.get<std::string>();
        if (!existingId.empty()) {
            return existingId;
        }
    }

    // 2. If not found, generate a new one
    GUID guid;
    if (SUCCEEDED(CoCreateGuid(&guid))) {
        char guid_cstr[39];
        snprintf(guid_cstr, sizeof(guid_cstr),
                 "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
                 guid.Data1, guid.Data2, guid.Data3,
                 guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                 guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
        
        std::string newId(guid_cstr);

        // 3. Save the new ID back to the config
        // SetValue expects a dot-separated path starting with the system name.
        SetValue("framework", keyPath, newId);

        return newId;
    }

    // Fallback in case CoCreateGuid fails
    return "generation_failed";
}

}  // namespace Config

SPF_NS_END