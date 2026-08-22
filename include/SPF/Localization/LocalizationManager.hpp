#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Utils/Signal.hpp"

#include "fmt/format.h"
#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

SPF_NS_BEGIN

namespace Localization {
/**
 * @class LocalizationManager
 * @brief A service for retrieving localized strings on demand.
 */
class LocalizationManager : public Config::IConfigurable {
 public:
  static LocalizationManager& GetInstance();

  /* @param allConfigs A map of component names to their localization configurations.
   * @return An InitializationReport detailing the results of the operation.
   */
  Core::InitializationReport Initialize(const std::map<std::string, nlohmann::ordered_json>* allConfigs);

  /**
   * @brief Signal emitted when the framework's own language changes.
   * @details Only fires for the "framework" component. Plugin language changes do NOT trigger this.
   *          The parameter is the component name (always "framework").
   */
  Utils::Signal<void(const std::string&)> OnFrameworkLanguageChanged;

  bool SetComponentLanguage(const std::string& componentName, const std::string& langCode);
  std::string GetComponentLanguage(const std::string& componentName) const;
  bool LanguageFileExists(const std::string& componentName, const std::string& langCode) const;
  const std::vector<std::string>& GetAvailableLanguagesFor(const std::string& componentName);
  const std::string& Get(const std::string& componentName, const std::string& key);
  const std::string& Get(const std::string& key);
  const std::string& GetWithFallback(const std::string& primaryComponentName, const std::string& key);

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) override;

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

  // --- Constants ---
  static constexpr const char* FRAMEWORK_COMPONENT_NAME = "framework";
  static constexpr const char* DEFAULT_LANGUAGE = "en";

  // --- Member Variables ---
  mutable std::mutex m_mutex;
  std::map<std::string, std::vector<std::string>> m_availableLanguages;
  std::map<std::string, std::string> m_currentLanguages;
  std::map<std::string, std::map<std::string, std::string>> m_translations;
  std::map<std::string, std::unordered_set<std::string>> m_reportedMissingKeys;
};
}  // namespace Localization
SPF_NS_END
