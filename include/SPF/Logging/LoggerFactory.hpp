#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Utils/Signal.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <map>
#include <mutex>
#include <vector>
#include <filesystem>

SPF_NS_BEGIN

namespace Logging::Sinks {
class LoggerWindowSink;
class ErrorReportSink;
}  // namespace Logging::Sinks

namespace Logging {

class LoggerFactory : public Config::IConfigurable {
 public:
  static LoggerFactory& GetInstance();

  Core::InitializationReport Initialize(const std::filesystem::path& log_dir, const nlohmann::ordered_json& framework_config);
  void Shutdown();

  std::shared_ptr<Logger> GetLogger(const std::string& name);
  std::shared_ptr<Sinks::LoggerWindowSink> GetUISink() const;
  std::shared_ptr<Sinks::ErrorReportSink> GetErrorReportSink() const;
  std::shared_ptr<ILogSink> GetFrameworkFileSink() const;

  void ApplyConfigurationFor(const std::string& componentName, const nlohmann::ordered_json& config);

  // --- Signals ---
  Utils::Signal<void(std::shared_ptr<Sinks::LoggerWindowSink>)> OnUISinkChanged;
  Utils::Signal<void(std::shared_ptr<Sinks::ErrorReportSink>)> OnErrorReportSinkChanged;
  Utils::Signal<void(std::shared_ptr<ILogSink>)> OnFrameworkFileSinkChanged;

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) override;

 private:
  std::shared_ptr<Logger> GetLogger_unlocked(const std::string& name);

  LoggerFactory();
  ~LoggerFactory();
  LoggerFactory(const LoggerFactory&) = delete;
  LoggerFactory& operator=(const LoggerFactory&) = delete;

  void CreateGlobalSinks(const nlohmann::ordered_json& framework_sinks_config, Core::InitializationReport& report);
  void AddGlobalSink(const std::shared_ptr<ILogSink>& sink);
  void RemoveGlobalSink(const std::shared_ptr<ILogSink>& sink);
  void ManagePrivateFileSink(const std::string& componentName, bool wantsFileSink);

  LogLevel m_frameworkLogLevel = LogLevel::Info;
  bool m_isInitialized = false;
  std::filesystem::path m_logDirectory;
  mutable std::mutex m_mutex;

  // Logger and Sink Management
  std::shared_ptr<Logger> m_logger; // Internal logger for the factory itself
  std::shared_ptr<Logger> m_defaultLogger; // No-op logger for pre-init phase
  std::map<std::string, std::shared_ptr<Logger>> m_loggers;
  
  // Global Sinks that apply to all loggers
  std::vector<std::shared_ptr<ILogSink>> m_globalSinks;
  std::shared_ptr<Sinks::LoggerWindowSink> m_uiSink;
  std::shared_ptr<Sinks::ErrorReportSink> m_errorReportSink;
  std::shared_ptr<ILogSink> m_frameworkFileSink;
};

}  // namespace Logging

SPF_NS_END
