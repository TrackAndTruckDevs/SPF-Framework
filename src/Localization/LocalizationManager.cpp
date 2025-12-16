#include "SPF/Localization/LocalizationManager.hpp"

#include <fstream>
#include <filesystem>
#include <unordered_set>

#include "SPF/System/PathManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Core/InitializationReport.hpp"

SPF_NS_BEGIN
namespace Localization {
using namespace SPF::Logging;
using namespace SPF::System;

namespace {
// Helper to recursively flatten a JSON object into a map of key-path strings.
void FlattenJson(const nlohmann::json& node, const std::string& prefix, std::map<std::string, std::string>& targetMap) {
  for (auto& [key, value] : node.items()) {
    std::string newPrefix = prefix.empty() ? key : prefix + "." + key;
    if (value.is_object()) {
      FlattenJson(value, newPrefix, targetMap);
    } else if (value.is_string()) {
      targetMap[newPrefix] = value.get<std::string>();
    }
  }
}
}  // namespace

LocalizationManager& LocalizationManager::GetInstance() {
  static LocalizationManager instance;
  return instance;
}

Core::InitializationReport LocalizationManager::Initialize(const std::map<std::string, nlohmann::json>* allConfigs) {
  Shutdown();  // Ensure idempotency

  Core::InitializationReport report;
  report.ServiceName = "localization";
  auto logger = LoggerFactory::GetInstance().GetLogger("Localization");

  if (!allConfigs) {
    report.InfoMessages.push_back("No localization settings found for any component.");
    return report;
  }

  for (const auto& [componentName, config] : *allConfigs) {
    std::string lang = DEFAULT_LANGUAGE;
    try {
        if (config.contains("language")) {
            const auto& langNode = config["language"];
            if (langNode.is_object() && langNode.contains("_value")) {
                lang = langNode["_value"].get<std::string>();
            } else if (langNode.is_string()) {
                lang = langNode.get<std::string>();
            } else {
                // The "language" key exists but is not a string or a value object.
                logger->Warn("Component '{}' has a 'language' setting with an invalid type '{}'. Falling back to default.", componentName, langNode.type_name());
                // lang is already DEFAULT_LANGUAGE
            }
        }
    } catch (const std::exception& e) {
        logger->Error("Error reading 'language' setting for component '{}': {}. Falling back to default language.", componentName, e.what());
        // lang is already DEFAULT_LANGUAGE
    }

    if (LoadLanguageFile(componentName, lang)) {
      report.InfoMessages.push_back(fmt::format("Successfully loaded configured language '{}' for component '{}'.", lang, componentName));
    } else {
      report.Errors.push_back(
          {fmt::format("Failed to load language '{}' for component '{}'. It might be missing or corrupted.", lang, componentName), fmt::format("{}.language", componentName)});
      // Attempt to fall back to default language
      if (lang != DEFAULT_LANGUAGE && LoadLanguageFile(componentName, DEFAULT_LANGUAGE)) {
        report.Warnings.push_back(
            Core::InitializationReport::Issue{fmt::format("Successfully loaded fallback language '{}' for component '{}'.", DEFAULT_LANGUAGE, componentName), ""});
      }
    }
  }

  return report;
}

void LocalizationManager::Shutdown() {
  std::lock_guard lock(m_mutex);
  m_availableLanguages.clear();
  m_translations.clear();
  m_reportedMissingKeys.clear();
}

void LocalizationManager::ScanAvailableLanguages(const std::string& componentName, const std::filesystem::path& directory) {
  // This is an internal method, mutex should be locked by the caller.
  if (m_availableLanguages.count(componentName)) return;  // Already scanned

  auto logger = LoggerFactory::GetInstance().GetLogger("Localization");
  logger->Info("Scanning for languages in: {}", directory.string());

  std::vector<std::string> languages;
  if (std::filesystem::exists(directory) && std::filesystem::is_directory(directory)) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.is_regular_file() && entry.path().extension() == ".json") {
        languages.push_back(entry.path().stem().string());
      }
    }
  }
  m_availableLanguages[componentName] = languages;
}

const std::vector<std::string>& LocalizationManager::GetAvailableLanguagesFor(const std::string& componentName) {
  std::lock_guard lock(m_mutex);
  // Ensure languages are scanned before returning.
  if (m_availableLanguages.find(componentName) == m_availableLanguages.end()) {
    std::filesystem::path dir = (componentName == FRAMEWORK_COMPONENT_NAME) ? PathManager::GetLocalizationDir() : PathManager::GetPluginLocalizationDir(componentName);
    ScanAvailableLanguages(componentName, dir);
  }
  return m_availableLanguages.at(componentName);
}

bool LocalizationManager::SetComponentLanguage(const std::string& componentName, const std::string& langCode) {
  std::lock_guard lock(m_mutex);
  if (LoadLanguageFile(componentName, langCode)) {
    return true;
  }

  auto logger = LoggerFactory::GetInstance().GetLogger("Localization");
  logger->Warn("Translation file for '{}' not found for component '{}'. Attempting to load default '{}'.", langCode, componentName, DEFAULT_LANGUAGE);

  if (langCode != DEFAULT_LANGUAGE) {
    if (LoadLanguageFile(componentName, DEFAULT_LANGUAGE)) {
      return true;  // Fallback succeeded
    }
  }

  logger->Error("Default translation file '{}.json' also not found for component '{}'. Localization will be disabled.", DEFAULT_LANGUAGE, componentName);
  m_translations.erase(componentName);
  return false;
}

bool LocalizationManager::LanguageFileExists(const std::string& componentName, const std::string& langCode) const {
  if (langCode.empty()) {
    return false;
  }

  std::filesystem::path langFilePath;
  if (componentName == FRAMEWORK_COMPONENT_NAME) {
    langFilePath = PathManager::GetLocalizationDir() / (langCode + ".json");
  } else {
    langFilePath = PathManager::GetPluginLocalizationDir(componentName) / (langCode + ".json");
  }

  return std::filesystem::exists(langFilePath);
}

bool LocalizationManager::LoadLanguageFile(const std::string& componentName, const std::string& langCode) {
  // This internal method assumes the mutex is already locked.
  auto logger = LoggerFactory::GetInstance().GetLogger("Localization");

  if (langCode.empty()) {
    logger->Warn("Attempted to set an empty language code for component '{}'. Ignoring.", componentName);
    return false;
  }

  std::filesystem::path langFilePath;
  if (componentName == FRAMEWORK_COMPONENT_NAME) {
    langFilePath = PathManager::GetLocalizationDir() / (langCode + ".json");
  } else {
    langFilePath = PathManager::GetPluginLocalizationDir(componentName) / (langCode + ".json");
  }

  if (!std::filesystem::exists(langFilePath)) {
    return false;  // File not found, let the caller handle logging and fallback.
  }

  logger->Info("Loading language file for component '{}' from '{}'", componentName, langFilePath.string());

  try {
    std::ifstream file(langFilePath);
    nlohmann::json newLanguageData = nlohmann::json::parse(file);

    // Use the new flattening mechanism
    m_translations[componentName].clear();
    FlattenJson(newLanguageData, "", m_translations[componentName]);
    m_reportedMissingKeys[componentName].clear();

    logger->Info("Successfully loaded and flattened language '{}' for component '{}'", langCode, componentName);
    return true;
  } catch (const std::exception& e) {
    logger->Error("Failed to parse language file '{}': {}", langFilePath.string(), e.what());
    return false;
  }
}

const std::string* LocalizationManager::FindKey(const std::string& componentName, const std::string& key) {
    auto componentIt = m_translations.find(componentName);
    if (componentIt == m_translations.end()) {
        return nullptr;
    }

    auto& translationMap = componentIt->second;
    auto keyIt = translationMap.find(key);

    if (keyIt != translationMap.end()) {
        return &keyIt->second;
    }

    return nullptr;
}

const std::string& LocalizationManager::Get(const std::string& componentName, const std::string& key) {
    std::lock_guard lock(m_mutex);

    if (const auto* result = FindKey(componentName, key)) {
        return *result;
    }

    auto& reported = m_reportedMissingKeys[componentName];
    auto [it, inserted] = reported.insert(key);
    if (inserted) {
        auto logger = LoggerFactory::GetInstance().GetLogger("Localization");
        logger->Warn("Localization key '{}' not found for component '{}'.", key, componentName);
    }
    return *it;
}

const std::string& LocalizationManager::Get(const std::string& key) { return Get(FRAMEWORK_COMPONENT_NAME, key); }

const std::string& LocalizationManager::GetWithFallback(const std::string& primaryComponentName, const std::string& key) {
    std::lock_guard lock(m_mutex);

    // 1. Try primary component
    if (const auto* result = FindKey(primaryComponentName, key)) {
        return *result;
    }

    // 2. Try framework fallback
    if (primaryComponentName != FRAMEWORK_COMPONENT_NAME) {
        if (const auto* result = FindKey(FRAMEWORK_COMPONENT_NAME, key)) {
            return *result;
        }
    }

    // 3. If not found anywhere, log ONCE and return the key.
    auto& reported = m_reportedMissingKeys[primaryComponentName];
    auto [it, inserted] = reported.insert(key);
    if (inserted) {
        auto logger = LoggerFactory::GetInstance().GetLogger("Localization");
        logger->Warn("Localization key '{}' not found for component '{}' and no framework fallback available.", key, primaryComponentName);
    }
    return *it;
}

bool LocalizationManager::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) {
  if (systemName != "localization" || keyPath != "language") {
    return false;
  }

  std::string newLangCode;
  if (newValue.is_object() && newValue.contains("_value")) {
    newLangCode = newValue["_value"].get<std::string>();
  } else if (newValue.is_string()) {
    newLangCode = newValue.get<std::string>();
  }

  if (!newLangCode.empty()) {
    SetComponentLanguage(componentName, newLangCode);
  }

  return true;
}
}  // namespace Localization
SPF_NS_END
