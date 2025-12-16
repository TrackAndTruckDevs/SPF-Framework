#include <SPF/Logging/LoggerFactory.hpp>

// --- Standard Library ---
#include <algorithm>
#include <memory>

// --- Framework ---
#include <SPF/Core/InitializationReport.hpp>
#include <SPF/Logging/Sinks/FileSink.hpp>
#include <SPF/Logging/Sinks/LoggerWindowSink.hpp>
#include <SPF/System/PathManager.hpp>

SPF_NS_BEGIN
namespace Logging {

using namespace SPF::Core;
using namespace SPF::System;
using namespace SPF::Logging::Sinks;

// --- Singleton ---
LoggerFactory& LoggerFactory::GetInstance() {
  static LoggerFactory instance;
  return instance;
}

// --- Lifecycle ---
Core::InitializationReport LoggerFactory::Initialize(const std::filesystem::path& log_dir, const nlohmann::json& framework_config) {
  std::lock_guard<std::mutex> lock(m_mutex);
  InitializationReport report;
  report.ServiceName = "LoggerFactory";

  if (m_isInitialized) {
    report.Warnings.push_back({"LoggerFactory already initialized. Ignoring call.", ""});
    return report;
  }

  m_logDirectory = log_dir;
  report.InfoMessages.push_back("Log directory set to: " + m_logDirectory.string());

  // Create a logger for the factory itself, but with no sinks yet.
  m_logger = std::make_shared<Logger>("LoggerFactory");

  // --- Early Framework Sink Creation ---
  if (!framework_config.is_object() || framework_config.empty()) {
      m_logger->Warn("Framework logging configuration is missing or empty. Using default: level=info, file_sink=true");
      CreateGlobalSinks({{"level", "info"}, {"sinks", {{"file", true}}}}, report);
  } else {
      CreateGlobalSinks(framework_config, report);
  }

  // Now that we have sinks, give them to the factory's own logger.
  m_logger->SetSinks(m_globalSinks);

  m_isInitialized = true;
  m_logger->Info("Logging system initialized with {} global sinks.", m_globalSinks.size());
  return report;
}

void LoggerFactory::Shutdown() {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_isInitialized) {
    return;
  }

  if (m_logger) m_logger->Info("Shutting down logging system...");

  m_loggers.clear();
  m_globalSinks.clear();
  m_uiSink.reset();
  m_frameworkFileSink.reset();
  m_logger.reset();
  m_defaultLogger.reset();

  m_isInitialized = false;
}

// --- Public API ---
std::shared_ptr<Logger> LoggerFactory::GetLogger(const std::string& name) {
  std::lock_guard<std::mutex> lock(m_mutex);
  return GetLogger_unlocked(name);
}

std::shared_ptr<Sinks::LoggerWindowSink> LoggerFactory::GetUISink() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_uiSink;
}

void LoggerFactory::ApplyConfigurationFor(const std::string& componentName, const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isInitialized) return;

    //m_logger->Debug("Applying configuration for component: '{}'", componentName);

    auto logger = GetLogger_unlocked(componentName);

    // 1. Set Log Level
    if (config.contains("level")) {
        const auto& levelNode = config["level"];
        std::string levelStr;
        if (levelNode.is_object() && levelNode.contains("_value")) {
            levelStr = levelNode["_value"].get<std::string>();
        } else if (levelNode.is_string()) {
            levelStr = levelNode.get<std::string>();
        }
        
        LogLevel level = LogLevel::Info; // Default
        if (!levelStr.empty() && TryParseLogLevel(levelStr, level)) {
            logger->SetLevel(level);
        }
    }

    // 2. Manage Sinks
    if (!config.contains("sinks")) return;
    const auto& sinksConfig = config["sinks"];

    if (sinksConfig.contains("file")) {
        const auto& fileNode = sinksConfig["file"];
        bool wantsFileSink = false;
        if (fileNode.is_object() && fileNode.contains("_value")) {
            wantsFileSink = fileNode["_value"].get<bool>();
        } else if (fileNode.is_boolean()) {
            wantsFileSink = fileNode.get<bool>();
        }
        ManagePrivateFileSink(componentName, wantsFileSink);
    }
}

// --- IConfigurable ---
bool LoggerFactory::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) {
  if (systemName != "logging") {
    return false;
  }
  
  std::lock_guard<std::mutex> lock(m_mutex);
  m_logger->Info("Handling logging setting change for '{}': Path='{}', Value={}", componentName, keyPath, newValue.dump());

  if (keyPath == "level") {
    auto logger = GetLogger_unlocked(componentName);
    if (!logger) return false;
    LogLevel newLevel;
    if (newValue.is_string() && TryParseLogLevel(newValue.get<std::string>(), newLevel)) {
      logger->SetLevel(newLevel);
      //m_logger->Debug("Updated log level for '{}' to {}", componentName, LogLevelToString(newLevel));
    }
  } else if (keyPath == "sinks.file") {
      if (newValue.is_boolean()) {
        ManagePrivateFileSink(componentName, newValue.get<bool>());
      }
  } else if (keyPath == "sinks.ui") {
    if (newValue.is_boolean()) {
      if (newValue.get<bool>() && !m_uiSink) {
          // Create and add UI sink if it doesn't exist
          m_logger->Info("Creating and adding global UI sink via settings change.");
          m_uiSink = std::make_shared<LoggerWindowSink>();
          AddGlobalSink(m_uiSink);
      } else if (!newValue.get<bool>() && m_uiSink) {
          // Remove UI sink if it exists
          m_logger->Info("Removing global UI sink via settings change.");
          RemoveGlobalSink(m_uiSink);
          m_uiSink.reset();
      }
    }
  }

  return true; // We handled this event.
}

// --- Private Implementations ---
std::shared_ptr<Logger> LoggerFactory::GetLogger_unlocked(const std::string& name) {
  if (!m_isInitialized) {
    return m_defaultLogger;
  }

  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }

  auto newLogger = std::make_shared<Logger>(name);
  newLogger->SetSinks(m_globalSinks);
  newLogger->SetLevel(m_frameworkLogLevel); // Immediately apply the cached framework level

  m_loggers[name] = newLogger;
  //m_logger->Debug("Created new logger: '{}' with {} global sinks.", name, m_globalSinks.size());
  return newLogger;
}

void LoggerFactory::CreateGlobalSinks(const nlohmann::json& framework_config, Core::InitializationReport& report) {
    // Set factory logger level first
    LogLevel level = LogLevel::Info;
    if (framework_config.contains("level")) {
        const auto& levelNode = framework_config["level"];
        std::string levelStr;
        if (levelNode.is_object() && levelNode.contains("_value")) {
            levelStr = levelNode["_value"].get<std::string>();
        } else if (levelNode.is_string()) {
            levelStr = levelNode.get<std::string>();
        }
        if (!levelStr.empty() && TryParseLogLevel(levelStr, level)) {
            m_logger->SetLevel(level);
        }
    }
    m_frameworkLogLevel = level; // Cache the framework's log level
    report.InfoMessages.push_back(fmt::format("Framework log level set to '{}'", LogLevelToString(level)));

    // Create and add global sinks
    if (framework_config.contains("sinks")) {
        const auto& sinksConfig = framework_config["sinks"];
        
        bool fileSinkEnabled = false;
        if (sinksConfig.contains("file")) {
            const auto& fileNode = sinksConfig["file"];
            if (fileNode.is_object() && fileNode.contains("_value")) {
                fileSinkEnabled = fileNode["_value"].get<bool>();
            } else if (fileNode.is_boolean()) {
                fileSinkEnabled = fileNode.get<bool>();
            }
        }

        if (fileSinkEnabled) {
            try {
                auto logFilePath = m_logDirectory / "framework.log";
                m_logger->Info("Creating GLOBAL file sink at path '{}'", logFilePath.string());
                m_frameworkFileSink = std::make_shared<FileSink>(logFilePath, "file_framework");
                AddGlobalSink(m_frameworkFileSink);
            } catch (const std::exception& e) {
                m_logger->Error("Failed to create framework file sink. Error: {}", e.what());
                report.Errors.push_back({fmt::format("Failed to create framework file sink: {}", e.what()), "sinks.file"});
            }
        }

        bool uiSinkEnabled = false;
        if (sinksConfig.contains("ui")) {
            const auto& uiNode = sinksConfig["ui"];
            if (uiNode.is_object() && uiNode.contains("_value")) {
                uiSinkEnabled = uiNode["_value"].get<bool>();
            } else if (uiNode.is_boolean()) {
                uiSinkEnabled = uiNode.get<bool>();
            }
        }

        if (uiSinkEnabled) {
            m_logger->Info("Creating GLOBAL UI sink.");
            m_uiSink = std::make_shared<LoggerWindowSink>();
            AddGlobalSink(m_uiSink);
        }
    }
}

void LoggerFactory::AddGlobalSink(const std::shared_ptr<ILogSink>& sink) {
    m_globalSinks.push_back(sink);
    // Propagate to all existing loggers
    for (auto& [name, logger] : m_loggers) {
        logger->AddSink(sink);
    }
    // Also add to the factory's own logger
    if (m_logger) {
        m_logger->AddSink(sink);
    }
}

void LoggerFactory::RemoveGlobalSink(const std::shared_ptr<ILogSink>& sink) {
    m_globalSinks.erase(std::remove(m_globalSinks.begin(), m_globalSinks.end(), sink), m_globalSinks.end());
    // Remove from all existing loggers
    for (auto& [name, logger] : m_loggers) {
        logger->RemoveSink(sink);
    }
    // Also remove from the factory's own logger
    if (m_logger) {
        m_logger->RemoveSink(sink);
    }
}

void LoggerFactory::ManagePrivateFileSink(const std::string& componentName, bool wantsFileSink) {
    if (componentName == "framework") return; // The framework uses the global file sink, not a private one.

    auto logger = GetLogger_unlocked(componentName);
    if (!logger) return;

    std::string sinkName = "file_" + componentName;

    // Find if this logger already has a private file sink
    std::shared_ptr<FileSink> existingFileSink;
    for(const auto& sink : logger->GetSinks()) {
        auto fileSink = std::dynamic_pointer_cast<FileSink>(sink);
        if (fileSink && fileSink->GetName() == sinkName) {
            existingFileSink = fileSink;
            break;
        }
    }

    if (wantsFileSink && !existingFileSink) {
        try {
            auto logFilePath = PathManager::GetPluginLogsDir(componentName) / (componentName + ".log");
            std::filesystem::create_directories(logFilePath.parent_path());

            m_logger->Info("Creating PRIVATE file sink for component: '{}' at path '{}'", componentName, logFilePath.string());
            auto privateFileSink = std::make_shared<FileSink>(logFilePath, sinkName);
            logger->AddSink(privateFileSink);
        } catch (const std::exception& e) {
            m_logger->Error("Failed to create private file sink for '{}'. Error: {}", componentName, e.what());
        }
    } else if (!wantsFileSink && existingFileSink) {
        m_logger->Info("Removing PRIVATE file sink for component: '{}'", componentName);
        logger->RemoveSink(existingFileSink);
    }
}

// --- Constructor / Destructor ---
LoggerFactory::LoggerFactory() : m_isInitialized(false) {
  m_defaultLogger = std::make_shared<Logger>("default");
  m_defaultLogger->SetLevel(LogLevel::Critical);
}

LoggerFactory::~LoggerFactory() {
  if (m_isInitialized) {
    Shutdown();
  }
}

}  // namespace Logging
SPF_NS_END
