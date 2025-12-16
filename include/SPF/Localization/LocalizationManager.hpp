#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <unordered_set>

#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Core/InitializationReport.hpp"

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Localization {
/**
 * @class LocalizationManager
 * @brief A service for retrieving localized strings on demand.
 */
class LocalizationManager : public Config::IConfigurable {
 public:
  static LocalizationManager& GetInstance();

  Core::InitializationReport Initialize(const std::map<std::string, nlohmann::json>* allConfigs);

  bool SetComponentLanguage(const std::string& componentName, const std::string& langCode);
  const std::vector<std::string>& GetAvailableLanguagesFor(const std::string& componentName);
  const std::string& Get(const std::string& componentName, const std::string& key);
  const std::string& Get(const std::string& key);
  const std::string& GetWithFallback(const std::string& primaryComponentName, const std::string& key);

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override;

  template <typename... Args>
  std::string GetFormatted(const std::string& componentName, const std::string& key, Args&&... args) {
    const std::string& formatString = Get(componentName, key);
    if (formatString == key) {
      return formatString;
    }
    return fmt::vformat(formatString, fmt::make_format_args(std::forward<Args>(args)...));
  }

 private:
  const std::string* FindKey(const std::string& componentName, const std::string& key);
  LocalizationManager() = default;
  ~LocalizationManager() = default;

  LocalizationManager(const LocalizationManager&) = delete;
  LocalizationManager& operator=(const LocalizationManager&) = delete;
  LocalizationManager(LocalizationManager&&) = delete;
  LocalizationManager& operator=(LocalizationManager&&) = delete;

  void Shutdown();
  void ScanAvailableLanguages(const std::string& componentName, const std::filesystem::path& directory);
  bool LoadLanguageFile(const std::string& componentName, const std::string& langCode);

  bool LanguageFileExists(const std::string& componentName, const std::string& langCode) const;

  // --- Constants ---
  static constexpr const char* FRAMEWORK_COMPONENT_NAME = "framework";
  static constexpr const char* DEFAULT_LANGUAGE = "en";

  // --- Member Variables ---
  mutable std::mutex m_mutex;
  std::map<std::string, std::vector<std::string>> m_availableLanguages;
  std::map<std::string, std::map<std::string, std::string>> m_translations;
  std::map<std::string, std::unordered_set<std::string>> m_reportedMissingKeys;
};
}  // namespace Localization
SPF_NS_END
