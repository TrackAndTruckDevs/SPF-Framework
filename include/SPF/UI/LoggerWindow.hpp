#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Logging/Logger.hpp"  // For LogLevel
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Logging::Sinks {
class LoggerWindowSink;
}  // namespace Logging::Sinks

namespace UI {
/**
 * @class LoggerWindow
 * @brief An ImGui window responsible for displaying logs collected by a LoggerWindowSink.
 */
class LoggerWindow : public BaseWindow, public Config::IConfigurable {
 public:
  LoggerWindow(const std::string& componentName, const std::string& windowId, Logging::Sinks::LoggerWindowSink& sink,
               Config::IConfigService& configService);

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override;

 protected:
  void RenderContent() override;
  const char* GetWindowTitle() const override;

 private:
  void BuildComponentFilterList();

  Logging::Sinks::LoggerWindowSink& m_sink;
  Config::IConfigService& m_configService;

  // --- Filter State ---
  Logging::LogLevel m_filterLevel = Logging::LogLevel::Trace;
  std::string m_selectedComponent = "All";
  std::vector<std::string> m_componentList;
};
}  // namespace UI

SPF_NS_END
