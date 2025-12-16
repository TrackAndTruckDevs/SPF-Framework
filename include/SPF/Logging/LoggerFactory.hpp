#pragma once

#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include <SPF/Logging/Logger.hpp>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Logging::Sinks {
class LoggerWindowSink;
}  // namespace Logging::Sinks

namespace Logging {
class LoggerFactory : public Config::IConfigurable {
 public:
  static LoggerFactory& GetInstance();

  Core::InitializationReport Initialize(const std::filesystem::path& log_dir, const nlohmann::json& framework_config);
  void Shutdown();

  std::shared_ptr<Logger> GetLogger(const std::string& name);
  std::shared_ptr<Sinks::LoggerWindowSink> GetUISink() const;

  void ApplyConfigurationFor(const std::string& componentName, const nlohmann::json& config);

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override;

 private:
  std::shared_ptr<Logger> GetLogger_unlocked(const std::string& name);

  LoggerFactory();
  ~LoggerFactory();
  LoggerFactory(const LoggerFactory&) = delete;
  LoggerFactory& operator=(const LoggerFactory&) = delete;

  void CreateGlobalSinks(const nlohmann::json& framework_sinks_config, Core::InitializationReport& report);
  void AddGlobalSink(const std::shared_ptr<ILogSink>& sink);
  void RemoveGlobalSink(const std::shared_ptr<ILogSink>& sink);
  void ManagePrivateFileSink(const std::string& componentName, bool wantsFileSink);

  LogLevel m_frameworkLogLevel = LogLevel::Info;
  bool m_isInitialized;
  std::filesystem::path m_logDirectory;
  mutable std::mutex m_mutex;

  // Logger and Sink Management
  std::shared_ptr<Logger> m_logger; // Internal logger for the factory itself
  std::shared_ptr<Logger> m_defaultLogger; // No-op logger for pre-init phase
  std::map<std::string, std::shared_ptr<Logger>> m_loggers;
  
  // Global Sinks that apply to all loggers
  std::vector<std::shared_ptr<ILogSink>> m_globalSinks;
  std::shared_ptr<Sinks::LoggerWindowSink> m_uiSink;
  std::shared_ptr<ILogSink> m_frameworkFileSink;
};

}  // namespace Logging

SPF_NS_END
