#include "SPF/Config/ConfigService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/ComponentInfo.hpp"
#include "SPF/Config/FrameworkManifest.hpp"
#include "SPF/Config/ManifestData.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/System/ApiService.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Utils/SystemUtils.hpp"

#include "fmt/format.h"
#include "nlohmann/json_fwd.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <libloaderapi.h>
#include <map>
#include <minwindef.h>
#include <objbase.h>
#include <optional>
#include <set>
#include <string>
#include <stringapiset.h>
#include <utility>
#include <vector>
#include <winerror.h>
#include <winnls.h>
#include <winnt.h>
#include <winreg.h>

SPF_NS_BEGIN

namespace Config {
using namespace SPF::Logging;
using namespace SPF::System;
using namespace SPF::Core;
using namespace SPF::Utils;
using namespace SPF::Localization;

namespace {
// Helper to inject _meta block into a JSON object
void InjectMetadata(nlohmann::ordered_json& target, const std::string& titleKey, const std::string& descriptionKey) {
  if (!titleKey.empty() || !descriptionKey.empty()) {
    target["_meta"] = nlohmann::ordered_json::object();
    if (!titleKey.empty()) {
      target["_meta"]["titleKey"] = titleKey;
    }
    if (!descriptionKey.empty()) {
      target["_meta"]["descriptionKey"] = descriptionKey;
    }
  }
}

constexpr wchar_t kSpfRegSubKey[] = L"Software\\SPF_Framework";

std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return {};
  int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
  std::string out(size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr);
  return out;
}

std::wstring Utf8ToWide(const std::string& text) {
  if (text.empty()) return {};
  int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
  std::wstring out(size, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), size);
  return out;
}

std::string ReadRegString(const wchar_t* valueName) {
  DWORD size = 0;
  if (RegGetValueW(HKEY_CURRENT_USER, kSpfRegSubKey, valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS || size < sizeof(wchar_t)) return {};
  std::wstring buf(size / sizeof(wchar_t), L'\0');
  if (RegGetValueW(HKEY_CURRENT_USER, kSpfRegSubKey, valueName, RRF_RT_REG_SZ, nullptr, buf.data(), &size) != ERROR_SUCCESS) return {};
  while (!buf.empty() && buf.back() == L'\0') buf.pop_back();
  return WideToUtf8(buf);
}

void WriteRegString(const char* valueName, const std::string& data) {
  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, kSpfRegSubKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
  std::wstring wide = Utf8ToWide(data);
  RegSetValueExW(key, Utf8ToWide(valueName).c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(wide.c_str()), static_cast<DWORD>((wide.size() + 1) * sizeof(wchar_t)));
  RegCloseKey(key);
}

// Helper to convert a dot-separated path to a JSON pointer, automatically descending into _value wrappers if they exist.
nlohmann::ordered_json::json_pointer GetMetaAwarePointer(const nlohmann::ordered_json& root, const std::string& dotPath) {
  if (dotPath.empty()) return nlohmann::ordered_json::json_pointer("/");

  std::string pointerStr = "";
  const nlohmann::ordered_json* current = &root;

  std::string remaining = dotPath;
  while (!remaining.empty()) {
    size_t dotPos = remaining.find('.');
    std::string part = (dotPos == std::string::npos) ? remaining : remaining.substr(0, dotPos);
    remaining = (dotPos == std::string::npos) ? "" : remaining.substr(dotPos + 1);

    // If current node is a Value Object (has _value), we MUST descend into _value to find the 'part'
    if (current && current->is_object() && current->contains("_value")) {
      pointerStr += "/_value";
      current = &((*current)["_value"]);
    }

    pointerStr += "/";
    // JSON pointer requires escaping ~ to ~0 and / to ~1
    for (char c : part) {
      if (c == '~')
        pointerStr += "~0";
      else if (c == '/')
        pointerStr += "~1";
      else
        pointerStr += c;
    }

    // Move 'current' for the next iteration to detect intermediate _value wrappers
    if (current) {
      if (current->is_object() && current->contains(part)) {
        current = &((*current)[part]);
      } else if (current->is_array()) {
        try {
          size_t idx = std::stoul(part);
          if (idx < current->size()) {
            current = &((*current)[idx]);
          } else {
            current = nullptr;
          }
        } catch (...) {
          current = nullptr;
        }
      } else {
        current = nullptr;
      }
    }
  }

  return nlohmann::ordered_json::json_pointer(pointerStr);
}

// // Helper to convert a dot-separated path to a JSON pointer path string (Loop version)
std::string ToJSONPointerPath(const std::string& dotPath) {
  if (dotPath.empty()) return "/";
  std::string result = "/" + dotPath;
  for (size_t i = 1; i < result.length(); ++i) {
    if (result[i] == '.') result[i] = '/';
  }
  return result;
}

const nlohmann::ordered_json* GetSettings(const nlohmann::ordered_json& source, const std::string& systemName) {
  if (source.contains(systemName) && source[systemName].is_object()) {
    return &source[systemName];
  }
  return nullptr;
}

// Helper to serialize general settings from ManifestData to  nlohmann::ordered_json.
nlohmann::ordered_json SerializeSettings(const ManifestData& manifest, const ManifestData& frameworkManifest) {
  nlohmann::ordered_json j = manifest.settings;
  // Inject custom settings metadata (no fallback for custom settings)
  for (const auto& meta : manifest.customSettingsMetadata) {
    if (meta.keyPath.empty()) continue;
    try {
      auto ptr = nlohmann::ordered_json::json_pointer(ToJSONPointerPath(meta.keyPath));
      if (!j.contains(ptr)) continue;
      nlohmann::ordered_json& node = j[ptr];
      if (node.is_object() && node.contains("_value")) {
        InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
      } else if (node.is_primitive() || node.is_string() || node.is_array()) {
        auto value = node;
        node = nlohmann::ordered_json::object();
        node["_value"] = value;
        InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
      } else if (node.is_object()) {
        InjectMetadata(node, meta.titleKey.value_or(""), meta.descriptionKey.value_or(""));
      }

      // NEW: Inject UI rendering hints
      if (meta.widget.has_value() || !meta.widget_params.empty() || meta.hide_in_ui) {
        if (!node.contains("_meta")) {
          node["_meta"] = nlohmann::ordered_json::object();
        }

        if (meta.hide_in_ui) {
          node["_meta"]["hide_in_ui"] = true;
        }

        if (meta.widget.has_value() || !meta.widget_params.empty()) {
          node["_meta"]["ui"] = nlohmann::ordered_json::object();
          auto& ui_meta = node["_meta"]["ui"];
          if (meta.widget.has_value()) {
            ui_meta["widget"] = meta.widget.value();
          }
          if (!meta.widget_params.empty()) {
            ui_meta["params"] = meta.widget_params;
          }
        }
      }
    } catch (const std::exception& e) {
      LoggerFactory::GetInstance().GetLogger("ConfigService")->Error("Error injecting custom setting metadata for key '{}': {}", meta.keyPath, e.what());
    }
  }
  return j;
}

// Helper to serialize logging settings from ManifestData to  nlohmann::ordered_json.
nlohmann::ordered_json SerializeLogging(const ManifestData& manifest, const ManifestData& frameworkManifest) {
  nlohmann::ordered_json j;
  auto findLoggingMeta = [&](const std::string& key) -> const StandardSettingMetadata* {
    for (const auto& meta : manifest.loggingMetadata) {
      if (meta.key == key) return &meta;
    }
    if (&manifest != &frameworkManifest) {
      for (const auto& meta : frameworkManifest.loggingMetadata) {
        if (meta.key == key) return &meta;
      }
    }
    return nullptr;
  };

  if (manifest.logging.level.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.logging.level.value();
    if (const auto* meta = findLoggingMeta("level")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    j["level"] = node;
  }

  nlohmann::ordered_json sinksNode;
  if (const auto* meta = findLoggingMeta("sinks")) {
    InjectMetadata(sinksNode, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
  }
  if (manifest.logging.sinks.file.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.logging.sinks.file.value();
    if (const auto* meta = findLoggingMeta("sinks.file")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    sinksNode["file"] = node;
  }
  if (manifest.logging.sinks.ui.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.logging.sinks.ui.value();
    if (const auto* meta = findLoggingMeta("sinks.ui")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    sinksNode["ui"] = node;
  }
  if (manifest.logging.sinks.report.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.logging.sinks.report.value();
    if (const auto* meta = findLoggingMeta("sinks.report")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    sinksNode["report"] = node;
  }
  if (!sinksNode.empty()) {
    j["sinks"] = sinksNode;
  }
  return j;
}

// Helper to serialize localization settings from ManifestData to  nlohmann::ordered_json.
nlohmann::ordered_json SerializeLocalization(const ManifestData& manifest, const ManifestData& frameworkManifest) {
  nlohmann::ordered_json j;
  auto findLocMeta = [&](const std::string& key) -> const StandardSettingMetadata* {
    for (const auto& meta : manifest.localizationMetadata) {
      if (meta.key == key) return &meta;
    }
    if (&manifest != &frameworkManifest) {
      for (const auto& meta : frameworkManifest.localizationMetadata) {
        if (meta.key == key) return &meta;
      }
    }
    return nullptr;
  };

  if (manifest.localization.language.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.localization.language.value();
    if (const auto* meta = findLocMeta("language")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    j["language"] = node;
  }

  if (manifest.localization.sync_plugin_languages.has_value()) {
    nlohmann::ordered_json node;
    node["_value"] = manifest.localization.sync_plugin_languages.value();
    if (const auto* meta = findLocMeta("sync_plugin_languages")) {
      InjectMetadata(node, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }
    j["sync_plugin_languages"] = node;
  }
  return j;
}

// Helper to serialize UI settings from ManifestData to  nlohmann::ordered_json.
nlohmann::ordered_json SerializeUI(const ManifestData& manifest, const ManifestData& frameworkManifest) {
  nlohmann::ordered_json j;
  nlohmann::ordered_json windowsNode;

  auto findUIMeta = [&](const std::string& key) -> const WindowMetadata* {
    for (const auto& meta : manifest.uiMetadata) {
      if (meta.windowName == key) return &meta;
    }
    if (&manifest != &frameworkManifest) {
      for (const auto& meta : frameworkManifest.uiMetadata) {
        if (meta.windowName == key) return &meta;
      }
    }
    return nullptr;
  };

  if (const auto* meta = findUIMeta("windows")) {
    InjectMetadata(windowsNode, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
  }

  for (const auto& [name, data] : manifest.ui.windows) {
    nlohmann::ordered_json window_j;
    if (const auto* meta = findUIMeta(name)) {
      InjectMetadata(window_j, meta->titleKey.value_or(""), meta->descriptionKey.value_or(""));
    }

    auto addValueWithMeta = [&](const std::string& propName, auto propValue) {
      nlohmann::ordered_json node;
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
    if (data.isDeveloperOnly.has_value()) addValueWithMeta("is_developer_only", data.isDeveloperOnly.value());

    if (!window_j.empty()) windowsNode[name] = window_j;
  }
  if (!windowsNode.empty()) j["windows"] = windowsNode;
  return j;
}

// Helper to serialize keybinds settings from ManifestData to  nlohmann::ordered_json.
nlohmann::ordered_json SerializeKeybinds(const ManifestData& manifest) {
  nlohmann::ordered_json keybinds;
  for (const auto& [group, actions] : manifest.keybinds.actions) {
    for (const auto& [name, defs] : actions) {
      nlohmann::ordered_json bindingsArray = nlohmann::ordered_json::array();
      for (const auto& def : defs) {
        nlohmann::ordered_json temp;
        std::string typeStr = def.type.value_or("");
        if (!typeStr.empty()) temp["type"] = typeStr;

        if (typeStr == "chord" && !def.bindings.empty()) {
          nlohmann::ordered_json subArray = nlohmann::ordered_json::array();
          for (const auto& subDef : def.bindings) {
            nlohmann::ordered_json subJ;
            if (subDef.type.has_value()) subJ["type"] = *subDef.type;
            if (subDef.key.has_value()) subJ["key"] = *subDef.key;
            subArray.push_back(subJ);
          }
          temp["bindings"] = subArray;
        } else {
          if (def.key.has_value()) temp["key"] = def.key.value();
        }

        auto input = Modules::InputFactory::CreateFromJson(temp);
        nlohmann::ordered_json input_j = input ? input->ToJson() : temp;

        bool isAxis = (typeStr.find("_axis") != std::string::npos);
        std::string mode = input_j.value("mode", isAxis ? "analog" : "digital");

        nlohmann::ordered_json final_j;
        final_j["type"] = input_j.value("type", typeStr);

        if (final_j["type"] == "chord") {
          final_j["bindings"] = input_j["bindings"];
        } else {
          final_j["key"] = input_j["key"];
        }

        final_j["consume"] = def.consume.value_or("never");

        if (isAxis) {
          final_j["mode"] = mode;
          if (mode == "analog") {
            final_j["curve"] = input_j.value("curve", "linear");
            final_j["invert"] = input_j.value("invert", false);
            final_j["deadzone"] = input_j.value("deadzone", 0.0);
            final_j["saturation"] = input_j.value("saturation", 1.0);
            final_j["sensitivity"] = input_j.value("sensitivity", 1.0);
            final_j["smoothing"] = input_j.value("smoothing", 0.0);

            bool isMouse = (final_j["type"] == "mouse_axis");
            bool accumulator = input_j.value("accumulator", isMouse);
            final_j["accumulator"] = accumulator;

            bool isTrigger = false;
            if (final_j["key"].is_string()) {
              std::string k = final_j["key"].get<std::string>();
              isTrigger = (k.find("TRIGGER") != std::string::npos);
            }

            final_j["range_min"] = input_j.value("range_min", isMouse ? -100.0 : (isTrigger ? 0.0 : -1.0));
            final_j["range_max"] = input_j.value("range_max", isMouse ? 100.0 : 1.0);
          } else {
            final_j["threshold"] = input_j.value("threshold", 0.5);
            final_j["behavior"] = def.behavior.value_or("toggle");
            final_j["press_type"] = def.pressType.value_or("short");
            final_j["press_threshold_ms"] = def.pressThresholdMs.value_or(500);
          }
        } else {
          final_j["behavior"] = def.behavior.value_or("toggle");
          final_j["press_type"] = def.pressType.value_or("short");
          final_j["press_threshold_ms"] = def.pressThresholdMs.value_or(500);
        }

        if (!final_j.empty()) bindingsArray.push_back(final_j);
      }

      if (!bindingsArray.empty()) {
        nlohmann::ordered_json actionObject;
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

nlohmann::ordered_json GetSystemSettingsAsJson(const ManifestData& manifest, const std::string& systemName, const ManifestData& frameworkManifest) {
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
  return nlohmann::ordered_json();
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
void MergeJsonObjects(nlohmann::ordered_json& target, const nlohmann::ordered_json& defaults, const nlohmann::ordered_json& user, InitializationReport& report, const std::string& componentName, const std::string& currentPath = "") {
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
        target[key] = nlohmann::ordered_json::object();
        MergeJsonObjects(target[key], defaultValue, userValue, report, componentName, newPath);
      }
      // Case 2: Default is a value object, but user provided a simple, compatible value.
      else if (defaultIsValueObj && !userIsObj) {
        const auto& defaultInnerValue = defaultValue["_value"];
        if (defaultInnerValue.type() == userValue.type() || (defaultInnerValue.is_number() && userValue.is_number())) {
          target[key] = defaultValue;         // Copy default (with _meta)
          target[key]["_value"] = userValue;  // Overwrite with user's value
        } else {
          // Type mismatch between user's simple value and the inner default value.
          target[key] = defaultValue;
          report.Warnings.push_back(InitializationReport::Issue{
            fmt::format("Type mismatch for key '{}' in component '{}'. User value type '{}' is incompatible with internal default type '{}'. Using default.", newPath, componentName, userValue.type_name(), defaultInnerValue.type_name()),
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
        report.Warnings.push_back(InitializationReport::Issue{fmt::format("Type mismatch for key '{}' in component '{}'. Expected '{}' but got '{}'. Using default value.", newPath, componentName, defaultValue.type_name(), userValue.type_name()),
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
void StripMetadata(nlohmann::ordered_json& node) {
  if (node.is_object()) {
    // Handle _value first: if it's a _value object, replace it entirely
    if (node.contains("_value")) {
      node = node["_value"];
      // After replacing, the new node might itself be an object/array that needs stripping
      StripMetadata(node);
      return;  // Done with this node after replacement
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

  // --- FINAL STEP: Inject current framework version into the configuration ---
  // This ensures the version is set correctly after all re-aggregations and merges.
  std::string currentVersionStr = GetFrameworkManifestData().info.version.value_or("0.0.0");
  SetValue("framework", "settings.framework.version", currentVersionStr);

  report.InfoMessages.push_back("Finished processing all system configurations.");
}

void ConfigService::Finalize(InitializationReport* report) {
  if (!report) return;
  report->ServiceName = "ConfigService";
  report->InfoMessages.push_back("Finalization sequence started.");

  // --- Step 0: Detect Installation/Update Status ---
  try {
    auto frameworkUserConfigPath = PathManager::GetConfigFilePath("framework_settings.json");
    std::string currentVersion = GetFrameworkManifestData().info.version.value_or("0.0.0");

    if (!std::filesystem::exists(frameworkUserConfigPath)) {
      m_installationStatus = InstallationStatus::NewInstall;
      report->InfoMessages.push_back("No configuration file found. Marked as New Installation.");
    } else {
      std::ifstream file(frameworkUserConfigPath);
      if (file.is_open() && file.peek() != std::ifstream::traits_type::eof()) {
        nlohmann::ordered_json userJson = nlohmann::ordered_json::parse(file);

        // Path: settings -> framework -> version
        std::string storedVersion = "0.0.0";
        if (userJson.contains("settings") && userJson["settings"].contains("framework") && userJson["settings"]["framework"].contains("version")) {
          storedVersion = userJson["settings"]["framework"]["version"].get<std::string>();
        }

        if (storedVersion == "0.0.0" || storedVersion.empty()) {
          m_installationStatus = InstallationStatus::Updated;  // File exists but no version key
          report->InfoMessages.push_back("Configuration file found but version key is missing. Marked as Updated.");
        } else if (storedVersion == currentVersion) {
          m_installationStatus = InstallationStatus::SameVersion;
          report->InfoMessages.push_back(fmt::format("Framework version {} is up to date.", storedVersion));
        } else {
          auto storedVerOpt = System::Version::FromString(storedVersion);
          auto currentVerOpt = System::Version::FromString(currentVersion);

          if (storedVerOpt && currentVerOpt) {
            if (*currentVerOpt < *storedVerOpt) {
              m_installationStatus = InstallationStatus::Downgraded;
              report->InfoMessages.push_back(fmt::format("Downgrade detected: Stored version {} is newer than current {}.", storedVersion, currentVersion));
            } else {
              m_installationStatus = InstallationStatus::Updated;
              report->InfoMessages.push_back(fmt::format("Update detected: Stored version {} is older than current {}.", storedVersion, currentVersion));
            }
          } else {
            // Fallback to string comparison if parsing fails
            m_installationStatus = (storedVersion < currentVersion) ? InstallationStatus::Updated : InstallationStatus::Downgraded;
          }
        }
      }
    }

    // NEW: Propagate status to EnvironmentManager
    System::EnvironmentManager::GetInstance().SetInstallationStatus(m_installationStatus);

  } catch (...) {
    m_installationStatus = InstallationStatus::NewInstall;
    System::EnvironmentManager::GetInstance().SetInstallationStatus(m_installationStatus);
  }

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
    m_isolatedConfigs["settings"]["framework"] = nlohmann::ordered_json::object();
  }
  auto& frameworkSettings = m_isolatedConfigs.at("settings").at("framework");
  if (!frameworkSettings.contains("plugin_states")) {
    frameworkSettings["plugin_states"] = nlohmann::ordered_json::object();
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

      // Version compatibility check
      if (manifest.info.minFrameworkVersion.has_value() && !manifest.info.minFrameworkVersion->empty()) {
        const auto& requiredVersionStr = manifest.info.minFrameworkVersion.value();
        const auto& frameworkVersionStr = GetFrameworkManifestData().info.version.value_or("0.0.0");

        auto requiredVersionOpt = System::Version::FromString(requiredVersionStr);
        auto frameworkVersionOpt = System::Version::FromString(frameworkVersionStr);

        if (requiredVersionOpt.has_value() && frameworkVersionOpt.has_value()) {
          if (frameworkVersionOpt.value() < requiredVersionOpt.value()) {
            info.incompatibilityReason = requiredVersionStr;
            logger->Warn("Plugin '{}' is incompatible. Requires framework version >= {}. Current framework version is {}.", componentName, requiredVersionStr, frameworkVersionStr);
          }
        }
      }
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
      // Override isEnabled if incompatible
      if (info.incompatibilityReason.has_value()) {
        if (info.isEnabled) {
          // If the user had it enabled, we force it off and mark config for saving.
          info.isEnabled = false;
          pluginStates[componentName]["enabled"] = false;
          configWasModified = true;
        }
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
    m_isolatedConfigs["settings"]["framework"] = nlohmann::ordered_json::object();
  }
  auto& frameworkSettings = m_isolatedConfigs.at("settings").at("framework");
  if (!frameworkSettings.contains("hook_states")) {
    frameworkSettings["hook_states"] = nlohmann::ordered_json::object();
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
    nlohmann::ordered_json defaultSettings = GetSystemSettingsAsJson(manifest, systemName, frameworkManifest);
    if (defaultSettings.is_null()) continue;

    nlohmann::ordered_json finalConfig = defaultSettings;

    if (IsUserConfigAllowed(manifest)) {
      std::filesystem::path userConfigPath = (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

      if (std::filesystem::exists(userConfigPath)) {
        try {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            nlohmann::ordered_json userJson = nlohmann::ordered_json::parse(file);
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

        // Auto-detect system language for new installations
        if (systemName == "localization" && m_installationStatus == System::InstallationStatus::NewInstall) {
          std::string sysLocale = SystemUtils::GetSystemLocaleName();
          if (sysLocale.length() >= 2) {
            std::string langCode = sysLocale.substr(0, 2);
            std::transform(langCode.begin(), langCode.end(), langCode.begin(), ::tolower);

            if (LocalizationManager::GetInstance().LanguageFileExists(componentName, langCode)) {
              if (finalConfig.contains("language")) {
                if (finalConfig["language"].is_object() && finalConfig["language"].contains("_value")) {
                  finalConfig["language"]["_value"] = langCode;
                } else {
                  finalConfig["language"] = langCode;
                }
                report.InfoMessages.push_back(fmt::format("Auto-detected system language '{}' for component '{}' during new installation.", langCode, componentName));
              }
            }
          }
        }
      }
    }
    // TODO: remove this migration prune after a couple of releases
    if (componentName == "framework" && finalConfig.contains("framework")) {
      nlohmann::ordered_json& frameworkSettings = finalConfig["framework"];
      if (frameworkSettings.is_object() && frameworkSettings.contains("framework_instance_id")) {
        frameworkSettings.erase("framework_instance_id");
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
  nlohmann::ordered_json finalConfig = nlohmann::ordered_json::object();
  std::vector<nlohmann::ordered_json> usedKeyValues;
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

  auto process_source = [&](const nlohmann::ordered_json& settings, const std::string& componentName) {
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
        const nlohmann::ordered_json* keys_to_assign = nullptr;

        if (actionNode.is_object() && actionNode.contains("bindings")) {
          keys_to_assign = &actionNode["bindings"];
        } else if (actionNode.is_array()) {
          keys_to_assign = &actionNode;
        }

        if (!keys_to_assign || !keys_to_assign->is_array()) continue;

        nlohmann::ordered_json successful_keys = nlohmann::ordered_json::array();

        for (const auto& key_value : *keys_to_assign) {
          bool conflict = false;
          try {
            auto new_input_obj = Modules::InputFactory::CreateFromJson(key_value);
            if (new_input_obj) {
              std::string new_press_type = key_value.value("press_type", "short");

              for (const auto& used_binding_json : usedKeyValues) {
                auto existing_input_obj = Modules::InputFactory::CreateFromJson(used_binding_json);
                if (existing_input_obj && new_input_obj->IsSameAs(*existing_input_obj)) {
                  std::string existing_press_type = used_binding_json.value("press_type", "short");
                  if (new_press_type == existing_press_type) {
                    conflict = true;
                    break;
                  }
                }
              }
            }
          } catch (const std::exception& e) {
            report.Warnings.push_back({fmt::format("Could not parse binding '{}' for action '{}' in component '{}'. Error: {}", key_value.dump(), fullActionKey, componentName, e.what()), fullActionKey});
            continue;  // Skip this invalid binding
          }

          if (conflict) {
            m_dirtyComponents.insert(componentName);
            report.Warnings.push_back({fmt::format("Keybind conflict for action '{}' in component '{}'. The key '{}' is already taken. This binding will be ignored.", fullActionKey, componentName, key_value.dump()), fullActionKey});
          } else {
            successful_keys.push_back(key_value);
            usedKeyValues.push_back(key_value);
          }
        }

        if (!successful_keys.empty()) {
          if (!finalConfig.contains(group.key())) {
            finalConfig[group.key()] = nlohmann::ordered_json::object();
          }
          // We need to reconstruct the full action object here, not just the bindings array
          nlohmann::ordered_json finalActionObject;
          if (actionNode.is_object() && actionNode.contains("_meta")) {
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
          nlohmann::ordered_json userJson = nlohmann::ordered_json::parse(file);
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
            nlohmann::ordered_json userJson = nlohmann::ordered_json::parse(file);
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
            nlohmann::ordered_json userJson = nlohmann::ordered_json::parse(file);
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
        finalConfig[groupName] = nlohmann::ordered_json::object();
      }
      finalConfig[groupName][actionName] = {{"bindings", nlohmann::ordered_json::array()}};
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

const nlohmann::ordered_json* ConfigService::GetMergedConfig(const std::string& systemName) const {
  auto it = m_mergedConfigs.find(systemName);
  return (it != m_mergedConfigs.end()) ? &it->second : nullptr;
}

const std::map<std::string, nlohmann::ordered_json>* ConfigService::GetAllComponentSettings(const std::string& systemName) const {
  auto it = m_isolatedConfigs.find(systemName);
  return (it != m_isolatedConfigs.end()) ? &it->second : nullptr;
}

void ConfigService::SetValue(const std::string& componentName, const std::string& jsonPath, const nlohmann::ordered_json& value) {
  // --- Check custom configs first ---
  auto customIt = m_customConfigs.find(componentName);
  if (customIt != m_customConfigs.end()) {
    try {
      nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(customIt->second, jsonPath);
      customIt->second[ptr] = value;

      // Auto-save logic for custom configs
      if (m_disabledAutoSave.find(componentName) == m_disabledAutoSave.end()) {
        auto pathIt = m_customContextPaths.find(componentName);
        if (pathIt != m_customContextPaths.end()) {
          std::ofstream file(pathIt->second);
          file << customIt->second.dump(4);
        }
      }

      m_eventManager.System.OnSettingWasChanged.Call({"", componentName, jsonPath, value});
      return;
    } catch (...) {
    }
  }

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
      nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(m_isolatedConfigs[systemName][componentName], keyPath);

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
      nlohmann::ordered_json::json_pointer aggregatedPtr = GetMetaAwarePointer(m_aggregatedUserSettings[componentName][systemName], keyPath);
      auto& aggregatedTargetNode = m_aggregatedUserSettings[componentName][systemName][aggregatedPtr];
      if (aggregatedTargetNode.is_object() && aggregatedTargetNode.contains("_value")) {
        if (value.is_object() && value.contains("_value")) {
          aggregatedTargetNode["_value"] = value["_value"];
        } else {
          aggregatedTargetNode["_value"] = value;
        }
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

    // Special case: if we are changing plugin states in framework settings, update the ComponentInfo cache
    if (systemName == "settings" && componentName == "framework" && keyPath.find("plugin_states.") == 0) {
      // Path format: plugin_states.PLUGIN_ID.enabled
      std::string rest = keyPath.substr(14);  // skip "plugin_states."
      size_t dotPos = rest.find('.');
      if (dotPos != std::string::npos) {
        std::string pluginId = rest.substr(0, dotPos);
        std::string prop = rest.substr(dotPos + 1);
        if (prop == "enabled" && m_allComponentInfo.count(pluginId)) {
          bool newState = false;
          if (value.is_boolean())
            newState = value.get<bool>();
          else if (value.is_object() && value.contains("_value"))
            newState = value["_value"].get<bool>();

          m_allComponentInfo[pluginId].isEnabled = newState;
        }
      }
    }
  } catch (const std::exception& e) {
    auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
    if (logger) logger->Error("Failed to set value for path '{}': {}", jsonPath, e.what());
  }
}

void ConfigService::ResetToDefault(const std::string& systemName, const std::string& keyPathWithComponent, InitializationReport* report) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (logger) logger->Debug("Attempting to reset key. System: '{}', KeyPath: '{}'", systemName, keyPathWithComponent);

  std::string originalComponent;
  nlohmann::ordered_json defaultValue;
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
      nlohmann::ordered_json defaultSettings = GetSystemSettingsAsJson(m_manifests.at(componentName), systemName, m_manifests.at("framework"));
      if (!defaultSettings.is_null()) {
        try {
          defaultValue = defaultSettings.at(nlohmann::ordered_json::json_pointer(ToJSONPointerPath(keyPath)));
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
      nlohmann::ordered_json defaultSettings = GetSystemSettingsAsJson(manifest, systemName, m_manifests.at("framework"));
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

void ConfigService::UpdateBinding(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding, const nlohmann::ordered_json& newBinding, const std::optional<std::pair<std::string, nlohmann::ordered_json>>& bindingToClear) {
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
  nlohmann::ordered_json mergedData = newBinding;
  const auto& ownerManifest = m_manifests.at(componentName);
  if (ownerManifest.keybinds.actions.count(groupName) && ownerManifest.keybinds.actions.at(groupName).count(actionName)) {
    const auto& defaultBindings = ownerManifest.keybinds.actions.at(groupName).at(actionName);
    if (!defaultBindings.empty()) {
      const auto& def = defaultBindings[0];
      nlohmann::ordered_json manifestDefaults;
      manifestDefaults["consume"] = def.consume.value_or("never");
      if (newBinding.value("type", "").find("_axis") == std::string::npos) {
        manifestDefaults["press_type"] = def.pressType.value_or("short");
        manifestDefaults["behavior"] = def.behavior.value_or("toggle");
        manifestDefaults["press_threshold_ms"] = def.pressThresholdMs.value_or(500);
      }
      manifestDefaults.merge_patch(newBinding);
      mergedData = manifestDefaults;
    }
  }

  // --- RECONSTRUCT WITH FIXED ORDER ---
  std::string type = mergedData.value("type", "");
  bool isAxis = (type.find("_axis") != std::string::npos);
  bool isMouse = (type == "mouse_axis");
  std::string mode = mergedData.value("mode", isAxis ? "analog" : "digital");

  nlohmann::ordered_json finalNewBinding;
  finalNewBinding["type"] = type;

  if (type == "chord") {
    finalNewBinding["bindings"] = mergedData["bindings"];
  } else {
    finalNewBinding["key"] = mergedData["key"];
  }

  finalNewBinding["consume"] = mergedData.value("consume", "never");

  if (isAxis) {
    finalNewBinding["mode"] = mode;
    if (mode == "analog") {
      finalNewBinding["curve"] = mergedData.value("curve", "linear");
      finalNewBinding["side"] = mergedData.value("side", "both");
      finalNewBinding["invert"] = mergedData.value("invert", false);
      finalNewBinding["deadzone"] = mergedData.value("deadzone", 0.0);
      finalNewBinding["saturation"] = mergedData.value("saturation", 1.0);
      finalNewBinding["sensitivity"] = mergedData.value("sensitivity", 1.0);
      finalNewBinding["smoothing"] = mergedData.value("smoothing", 0.0);
      bool accumulator = mergedData.value("accumulator", isMouse);
      finalNewBinding["accumulator"] = accumulator;

      bool isTrigger = false;
      if (finalNewBinding["key"].is_string()) {
        std::string k = finalNewBinding["key"].get<std::string>();
        isTrigger = (k.find("TRIGGER") != std::string::npos);
      }

      finalNewBinding["range_min"] = mergedData.value("range_min", isMouse ? -100.0 : (isTrigger ? 0.0 : -1.0));
      finalNewBinding["range_max"] = mergedData.value("range_max", isMouse ? 100.0 : 1.0);
    } else {
      finalNewBinding["threshold"] = mergedData.value("threshold", 0.5);
      finalNewBinding["behavior"] = mergedData.value("behavior", "toggle");
      finalNewBinding["press_type"] = mergedData.value("press_type", "short");
      finalNewBinding["press_threshold_ms"] = mergedData.value("press_threshold_ms", 500);
    }
  } else {
    finalNewBinding["behavior"] = mergedData.value("behavior", "toggle");
    finalNewBinding["press_type"] = mergedData.value("press_type", "short");
    finalNewBinding["press_threshold_ms"] = mergedData.value("press_threshold_ms", 500);
  }

  // 5. Add or Update the binding in the target array
  if (originalBinding.empty()) {  // Add new binding
    bindingsArray.push_back(finalNewBinding);
  } else {  // Update existing binding
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

bool ConfigService::_DeleteBindingInternal(const std::string& actionFullName, const nlohmann::ordered_json& bindingToDelete) {
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

      for (auto it = bindingsArray.begin(); it != bindingsArray.end();) {
        if (*it == bindingToDelete) {
          it = bindingsArray.erase(it);
          m_dirtyComponents.insert(componentName);
          if (logger) logger->Info("_DeleteBindingInternal: Removed binding from action '{}'. Component '{}' marked as dirty.", actionFullName, componentName);
          return true;
        } else {
          ++it;
        }
      }
    }
  }

  if (logger) logger->Warn("_DeleteBindingInternal: Could not find binding '{}' in action '{}' to delete.", bindingToDelete.dump(), actionFullName);
  return false;  // Failure
}

void ConfigService::DeleteBinding(const std::string& actionFullName, const nlohmann::ordered_json& bindingToDelete) {
  if (_DeleteBindingInternal(actionFullName, bindingToDelete)) {
    m_eventManager.System.OnKeybindsModified.Call({});
  }
}

void ConfigService::UpdateBindingProperty(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding, const std::string& propertyName, const nlohmann::ordered_json& newValue) {
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

      if (originalBinding.is_object()) {
        std::string origType = originalBinding.value("type", "");
        std::string origKey = originalBinding.value("key", "");
        std::string origPressType = originalBinding.value("press_type", "short");
        std::string origSide = originalBinding.value("side", "both");
        auto const& origBindings = originalBinding.contains("bindings") ? originalBinding["bindings"] : nlohmann::ordered_json();

        for (auto& binding : bindingsArray) {
          bool match = false;
          if (binding.is_object()) {
            // 1. Compare basic identity fields
            bool typeMatch = (binding.value("type", "") == origType);
            bool pressTypeMatch = (binding.value("press_type", "short") == origPressType);

            // 2. Compare key or chord constituents
            bool keyOrChordMatch = false;
            if (origType == "chord") {
              keyOrChordMatch = (binding.contains("bindings") && binding["bindings"] == origBindings);
            } else {
              keyOrChordMatch = (binding.value("key", "") == origKey);
            }

            // 3. Compare side (only relevant for axes)
            bool sideMatch = true;
            if (origType.find("_axis") != std::string::npos) {
              sideMatch = (binding.value("side", "both") == origSide);
            }

            if (typeMatch && pressTypeMatch && keyOrChordMatch && sideMatch) {
              match = true;
            }
          }

          if (match) {
            binding[propertyName] = newValue;

            // --- RECONSTRUCT TO CLEAN UP WHILE PRESERVING USER VALUES ---
            nlohmann::ordered_json current = binding;
            std::string type = current.value("type", "");
            bool isAxis = (type.find("_axis") != std::string::npos);
            bool isMouse = (type == "mouse_axis");
            std::string mode = current.value("mode", isAxis ? "analog" : "digital");

            bool isTrigger = false;
            if (current.contains("key") && current["key"].is_string()) {
              std::string k = current["key"].get<std::string>();
              isTrigger = (k.find("TRIGGER") != std::string::npos);
            }

            nlohmann::ordered_json clean;
            clean["type"] = type;
            if (type == "chord") {
              clean["bindings"] = current["bindings"];
            } else {
              clean["key"] = current["key"];
            }
            clean["consume"] = current.value("consume", "never");

            if (isAxis) {
              clean["mode"] = mode;
              if (mode == "analog") {
                clean["curve"] = current.value("curve", "linear");
                clean["side"] = current.value("side", "both");
                clean["invert"] = current.value("invert", false);
                clean["deadzone"] = current.value("deadzone", 0.0);
                clean["saturation"] = current.value("saturation", 1.0);
                clean["sensitivity"] = current.value("sensitivity", 1.0);
                clean["smoothing"] = current.value("smoothing", 0.0);
                bool accumulator = current.value("accumulator", isMouse);
                clean["accumulator"] = accumulator;
                clean["range_min"] = current.value("range_min", isMouse ? -100.0 : (isTrigger ? 0.0 : -1.0));
                clean["range_max"] = current.value("range_max", isMouse ? 100.0 : 1.0);
              } else {
                clean["threshold"] = current.value("threshold", 0.5);
                clean["behavior"] = current.value("behavior", "toggle");
                clean["press_type"] = current.value("press_type", "short");
                clean["press_threshold_ms"] = current.value("press_threshold_ms", 500);
              }
            } else {
              clean["behavior"] = current.value("behavior", "toggle");
              clean["press_type"] = current.value("press_type", "short");
              clean["press_threshold_ms"] = current.value("press_threshold_ms", 500);
            }

            binding = clean;  // Swap dirty with clean

            m_dirtyComponents.insert(componentName);
            m_eventManager.System.OnKeybindsModified.Call({});
            if (logger) logger->Info("UpdateBindingProperty: Updated property '{}' and filtered stale fields.", propertyName);
            return;
          }
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
    std::filesystem::path userConfigPath = (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

    nlohmann::ordered_json fullConfigToSave;
    try {
      // If the file was corrupted, we start fresh. Otherwise, load existing content to preserve other settings.
      if (m_corruptedFilePaths.find(userConfigPath.string()) == m_corruptedFilePaths.end()) {
        if (std::filesystem::exists(userConfigPath)) {
          std::ifstream file(userConfigPath);
          if (file.peek() != std::ifstream::traits_type::eof()) {
            fullConfigToSave = nlohmann::ordered_json::parse(file);
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
      nlohmann::ordered_json keybindsToSave = nlohmann::ordered_json::object();
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

nlohmann::ordered_json ConfigService::GetValue(const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& defaultValue) const {
  // --- Check custom configs first ---
  auto customIt = m_customConfigs.find(componentName);
  if (customIt != m_customConfigs.end()) {
    try {
      nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(customIt->second, keyPath);
      if (customIt->second.contains(ptr)) {
        return customIt->second[ptr];
      }
    } catch (...) {
    }
    return defaultValue;
  }

  if (keyPath.rfind("info.", 0) == 0) {
    auto manifestIt = m_manifests.find(componentName);
    if (manifestIt != m_manifests.end()) {
      const auto& info = manifestIt->second.info;
      std::string subKey = keyPath.substr(5);  // Length of "info."
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

  const nlohmann::ordered_json* configRoot = nullptr;
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
    auto ptr = GetMetaAwarePointer(*configRoot, restOfPath);

    // Use find() logic or similar to avoid double lookup and exceptions
    if (configRoot->contains(ptr)) {
      const auto& rawNode = configRoot->at(ptr);
      if (rawNode.is_object() && rawNode.contains("_value")) {
        return rawNode["_value"];
      }
      return rawNode;
    }
  } catch (...) {
    // json_pointer might throw if path is invalid, return default
  }

  return defaultValue;
}

const nlohmann::ordered_json* ConfigService::GetValuePtr(const std::string& componentName, const std::string& keyPath) const {
  // --- Check custom configs first ---
  auto customIt = m_customConfigs.find(componentName);
  if (customIt != m_customConfigs.end()) {
    try {
      nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(customIt->second, keyPath);
      if (customIt->second.contains(ptr)) {
        return &customIt->second.at(ptr);
      }
    } catch (...) {
    }
    return nullptr;
  }

  size_t firstDot = keyPath.find('.');
  if (firstDot == std::string::npos) {
    return nullptr;
  }

  std::string systemName = keyPath.substr(0, firstDot);
  std::string restOfPath = keyPath.substr(firstDot + 1);

  auto strategyIt = m_systemStrategies.find(systemName);
  if (strategyIt == m_systemStrategies.end()) {
    return nullptr;
  }

  const nlohmann::ordered_json* configRoot = nullptr;
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
    return nullptr;
  }

  try {
    nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(*configRoot, restOfPath);
    return &configRoot->at(ptr);
  } catch (const nlohmann::ordered_json::out_of_range&) {
    return nullptr;
  } catch (const nlohmann::ordered_json::parse_error&) {
    return nullptr;
  }
}

void ConfigService::CheckDirtyKeybinds(InitializationReport& report) {
  const auto* keybindsConfig = GetMergedConfig("keybinds");
  if (!keybindsConfig) return;

  report.InfoMessages.push_back("Checking for outdated keybind configurations...");

  for (const auto& [componentName, manifest] : m_manifests) {
    if (!IsUserConfigAllowed(manifest)) continue;

    std::filesystem::path userConfigPath = (componentName == "framework") ? PathManager::GetConfigFilePath("framework_settings.json") : PathManager::GetPluginConfigDir(componentName) / "settings.json";

    if (!std::filesystem::exists(userConfigPath)) continue;

    nlohmann::ordered_json userJson;
    try {
      std::ifstream file(userConfigPath);
      if (file.peek() != std::ifstream::traits_type::eof()) {
        userJson = nlohmann::ordered_json::parse(file);
      }
    } catch (...) {
      continue;  // File is likely corrupted, already marked as dirty
    }

    const nlohmann::ordered_json* originalUserBinds = GetSettings(userJson, "keybinds");

    nlohmann::ordered_json newBinds = nlohmann::ordered_json::object();
    for (const auto& [fullActionKey, owner] : m_keybindOwnership) {
      if (owner == componentName) {
        size_t lastDot = fullActionKey.rfind('.');
        if (lastDot == std::string::npos) continue;
        std::string groupName = fullActionKey.substr(0, lastDot);
        std::string actionName = fullActionKey.substr(lastDot + 1);

        if (keybindsConfig->contains(groupName) && (*keybindsConfig)[groupName].contains(actionName)) {
          if (!newBinds.contains(groupName)) newBinds[groupName] = nlohmann::ordered_json::object();
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

const std::map<std::string, nlohmann::ordered_json>& ConfigService::GetAggregatedUserSettings() const { return m_aggregatedUserSettings; }

System::InstallationStatus ConfigService::GetInstallationStatus() const { return m_installationStatus; }

std::string ConfigService::GetFrameworkInstanceId() {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

  std::string instanceId = ReadRegString(L"framework_instance_id");
  if (instanceId.empty()) {
    GUID guid;
    if (FAILED(CoCreateGuid(&guid))) {
      logger->Error("Failed to generate framework instance id");
      return "generation_failed";
    }
    char guid_cstr[39];
    snprintf(guid_cstr,
             sizeof(guid_cstr),
             "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX",
             guid.Data1,
             guid.Data2,
             guid.Data3,
             guid.Data4[0],
             guid.Data4[1],
             guid.Data4[2],
             guid.Data4[3],
             guid.Data4[4],
             guid.Data4[5],
             guid.Data4[6],
             guid.Data4[7]);

    instanceId = guid_cstr;
    WriteRegString("framework_instance_id", instanceId);
    logger->Info("Generated new framework instance id");
  }

  // Mirror live installation facts into the registry.
  const auto& components = GetAllComponentInfo();
  auto frameworkInfo = components.find("framework");
  if (frameworkInfo != components.end() && frameworkInfo->second.version && !frameworkInfo->second.version->empty()) {
    WriteRegString("Version", *frameworkInfo->second.version);
  }

  const std::string& gameCode = EnvironmentManager::GetInstance().GetGameInfo().code;
  const char* suffix = nullptr;
  if (gameCode == "ats")
    suffix = "ATS";
  else if (gameCode == "eut2")
    suffix = "ETS2";

  HMODULE thisModule = nullptr;
  if (suffix && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&kSpfRegSubKey), &thisModule)) {
    wchar_t dllPath[MAX_PATH]{};
    if (GetModuleFileNameW(thisModule, dllPath, MAX_PATH) > 0) {
      WriteRegString((std::string("DllPath") + suffix).c_str(), WideToUtf8(dllPath));
    }
  }

  return instanceId;
}

bool ConfigService::IsConnectionAllowed() {
  const std::string keyPath = "settings.framework.connect";

  // 1. Try to get the existing value
  nlohmann::ordered_json valueJson = GetValue("framework", keyPath, nullptr);

  if (valueJson.is_boolean()) {
    return valueJson.get<bool>();
  }

  // 2. If not found or not a boolean, create/fix it with default 'true'
  SetValue("framework", keyPath, true);
  return true;
}

void ConfigService::RegisterActionMetadata(const std::string& componentName, const std::string& actionFullName, const std::string& titleKey, const std::string& descKey) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

  // 1. NORMALIZE: Ensure actionFullName starts with componentName (e.g. "ExamplePlugin.myAction")
  // This is mandatory for UI visibility and proper JSON serialization.
  std::string sanitizedFullName = actionFullName;
  std::string prefix = componentName + ".";
  if (actionFullName.find(prefix) != 0) {
    sanitizedFullName = prefix + actionFullName;
  }

  // 2. Register ownership
  m_keybindOwnership[sanitizedFullName] = componentName;

  // 3. Split into group and action
  size_t lastDot = sanitizedFullName.rfind('.');
  std::string groupName = sanitizedFullName.substr(0, lastDot);
  std::string actionName = sanitizedFullName.substr(lastDot + 1);

  if (!m_mergedConfigs.count("keybinds")) {
    m_mergedConfigs["keybinds"] = nlohmann::ordered_json::object();
  }

  auto& keybinds = m_mergedConfigs["keybinds"];
  if (!keybinds.contains(groupName)) {
    keybinds[groupName] = nlohmann::ordered_json::object();
  }

  if (!keybinds[groupName].contains(actionName)) {
    keybinds[groupName][actionName] = nlohmann::ordered_json::object();
    keybinds[groupName][actionName]["bindings"] = nlohmann::ordered_json::array();
  }

  // 4. Inject metadata
  auto& actionNode = keybinds[groupName][actionName];
  InjectMetadata(actionNode, titleKey, descKey);

  // 5. Mark dirty to ensure it's saved in the plugin's settings.json
  m_dirtyComponents.insert(componentName);

  // 6. Fire event to refresh UI
  m_eventManager.System.OnKeybindsModified.Call({});

  if (logger) logger->Info("Dynamically registered action metadata for '{}' (Owner: {})", sanitizedFullName, componentName);

  // 7. Ensure the action exists in KeyBindsManager
  Modules::KeyBindsManager::GetInstance().EnsureActionExists(sanitizedFullName);
}

void ConfigService::UnregisterActionMetadata(const std::string& componentName, const std::string& actionFullName) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

  // NORMALIZE input name first to find the correct internal key
  std::string sanitizedFullName = actionFullName;
  std::string prefix = componentName + ".";
  if (actionFullName.find(prefix) != 0) {
    sanitizedFullName = prefix + actionFullName;
  }

  // 1. Remove from ownership map
  auto it = m_keybindOwnership.find(sanitizedFullName);
  if (it != m_keybindOwnership.end() && it->second == componentName) {
    m_keybindOwnership.erase(it);
  } else {
    return;
  }

  // 2. Remove from merged config
  size_t lastDot = sanitizedFullName.rfind('.');
  std::string groupName = sanitizedFullName.substr(0, lastDot);
  std::string actionName = sanitizedFullName.substr(lastDot + 1);

  if (m_mergedConfigs.count("keybinds") && m_mergedConfigs["keybinds"].contains(groupName)) {
    auto& group = m_mergedConfigs["keybinds"][groupName];
    if (group.contains(actionName)) {
      group.erase(actionName);
      if (group.empty()) {
        m_mergedConfigs["keybinds"].erase(groupName);
      }
      m_dirtyComponents.insert(componentName);
      m_eventManager.System.OnKeybindsModified.Call({});
      if (logger) logger->Info("Dynamically unregistered action '{}' for component '{}'.", sanitizedFullName, componentName);
    }
  }

  // 3. Completely remove from KeyBindsManager logic
  Modules::KeyBindsManager::GetInstance().RemoveAction(sanitizedFullName);
}

std::vector<std::string> ConfigService::GetOwnedActions(const std::string& componentName) const {
  std::vector<std::string> actions;
  for (const auto& [actionKey, owner] : m_keybindOwnership) {
    if (owner == componentName) {
      actions.push_back(actionKey);
    }
  }
  return actions;
}

bool ConfigService::HasKey(const std::string& componentName, const std::string& keyPath) const { return GetValuePtr(componentName, keyPath) != nullptr; }

void ConfigService::SaveComponentConfig(const std::string& componentName) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");

  // --- Support manual saving for custom configs ---
  auto customIt = m_customConfigs.find(componentName);
  if (customIt != m_customConfigs.end()) {
    auto pathIt = m_customContextPaths.find(componentName);
    if (pathIt != m_customContextPaths.end()) {
      try {
        std::ofstream file(pathIt->second);
        if (file.is_open()) {
          file << customIt->second.dump(4);
          if (logger) logger->Info("Manually saved custom config to {}", pathIt->second);
          return;
        }
      } catch (const std::exception& e) {
        if (logger) logger->Error("Failed to manually save custom config {}: {}", pathIt->second, e.what());
        return;
      }
    }
  }

  if (m_dirtyComponents.find(componentName) == m_dirtyComponents.end()) {
    if (logger) logger->Debug("SaveComponentConfig: Component '{}' is not dirty, skipping.", componentName);
    return;
  }

  // Reuse SaveAllDirty logic but only for this component
  std::set<std::string> originalDirty = m_dirtyComponents;
  m_dirtyComponents.clear();
  m_dirtyComponents.insert(componentName);

  SaveAllDirty();

  // Restore remaining dirty components
  for (const auto& comp : originalDirty) {
    if (comp != componentName) {
      m_dirtyComponents.insert(comp);
    }
  }
}

void ConfigService::RemoveKey(const std::string& componentName, const std::string& keyPath) {
  size_t firstDot = keyPath.find('.');
  if (firstDot == std::string::npos) return;

  std::string systemName = keyPath.substr(0, firstDot);
  std::string restOfPath = keyPath.substr(firstDot + 1);

  auto strategyIt = m_systemStrategies.find(systemName);
  if (strategyIt == m_systemStrategies.end()) return;

  try {
    auto unescape = [](std::string s) -> std::string {
      size_t pos = 0;
      while ((pos = s.find("~1", pos)) != std::string::npos) {
        s.replace(pos, 2, "/");
        pos += 1;
      }
      pos = 0;
      while ((pos = s.find("~0", pos)) != std::string::npos) {
        s.replace(pos, 2, "~");
        pos += 1;
      }
      return s;
    };

    if (strategyIt->second == MergeStrategy::Isolate) {
      if (!m_isolatedConfigs.contains(systemName) || !m_isolatedConfigs[systemName].contains(componentName)) return;

      auto& config = m_isolatedConfigs[systemName][componentName];
      nlohmann::ordered_json::json_pointer ptr = GetMetaAwarePointer(config, restOfPath);

      if (config.contains(ptr)) {
        std::string actualPath = ptr.to_string();
        size_t lastSlash = actualPath.rfind('/');
        if (lastSlash != std::string::npos) {
          std::string parentPathStr = actualPath.substr(0, lastSlash);
          std::string leafKey = unescape(actualPath.substr(lastSlash + 1));

          nlohmann::ordered_json::json_pointer parentPtr(parentPathStr);
          if (config.contains(parentPtr)) {
            auto& parentNode = config[parentPtr];
            if (parentNode.is_object()) {
              parentNode.erase(leafKey);
            } else if (parentNode.is_array()) {
              try {
                size_t idx = std::stoul(leafKey);
                if (idx < parentNode.size()) {
                  parentNode.erase(parentNode.begin() + idx);
                }
              } catch (...) {
              }
            }
          }
        } else {
          // Root level key
          std::string rootKey = unescape(actualPath.substr(1));
          if (config.is_object()) {
            config.erase(rootKey);
          }
        }

        m_dirtyComponents.insert(componentName);
        BuildAggregatedUserSettings();
      }
    } else {
      // PriorityMerge (keybinds)
      if (!m_mergedConfigs.contains(systemName)) return;

      size_t lastDot = restOfPath.rfind('.');
      if (lastDot == std::string::npos) return;
      std::string groupName = restOfPath.substr(0, lastDot);
      std::string actionName = restOfPath.substr(lastDot + 1);

      if (m_mergedConfigs[systemName].contains(groupName) && m_mergedConfigs[systemName][groupName].contains(actionName)) {
        m_mergedConfigs[systemName][groupName].erase(actionName);
        if (m_mergedConfigs[systemName][groupName].empty()) {
          m_mergedConfigs[systemName].erase(groupName);
        }
        m_dirtyComponents.insert(componentName);
      }
    }
  } catch (...) {
  }
}

void ConfigService::ReloadComponentConfig(const std::string& componentName) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (logger) logger->Info("Reloading configuration for component '{}' from disk.", componentName);

  // 1. Remove from dirty to prevent overwriting during process
  m_dirtyComponents.erase(componentName);

  // 2. Re-process configurations
  InitializationReport report;
  ProcessAllSystemConfigurations(report);
}

bool ConfigService::IsSettingHidden(const std::string& componentName, const std::string& keyPath) const {
  auto it = m_manifests.find(componentName);
  if (it == m_manifests.end()) return false;

  // customSettingsMetadata contains paths WITHOUT the system prefix (e.g. "commands", not "settings.commands")
  for (const auto& meta : it->second.customSettingsMetadata) {
    if (meta.keyPath == keyPath) {
      return meta.hide_in_ui;
    }
  }

  return false;
}

std::string ConfigService::CreateCustomContext(const std::string& filePath) {
  auto logger = LoggerFactory::GetInstance().GetLogger("ConfigService");
  if (m_customConfigs.count(filePath)) return filePath;

  try {
    if (std::filesystem::exists(filePath)) {
      std::ifstream file(filePath);
      if (file.is_open()) {
        nlohmann::ordered_json j;
        file >> j;
        m_customConfigs[filePath] = std::move(j);
        if (logger) logger->Info("Loaded custom configuration from {}", filePath);
      }
    } else {
      m_customConfigs[filePath] = nlohmann::ordered_json::object();
      if (logger) logger->Info("Created new custom configuration context for {}", filePath);
    }
    m_customContextPaths[filePath] = filePath;
    return filePath;
  } catch (const std::exception& e) {
    if (logger) logger->Error("Failed to create custom context for {}. Error: {}", filePath, e.what());
    return "";
  }
}

void ConfigService::SetAutoSave(const std::string& contextId, bool enabled) {
  if (enabled) {
    m_disabledAutoSave.erase(contextId);
  } else {
    m_disabledAutoSave.insert(contextId);
  }
}

}  // namespace Config

SPF_NS_END