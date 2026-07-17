#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Utils/Signal.hpp"

#include "nlohmann/json_fwd.hpp"

#include <memory>
#include <string>
#include <vector>


SPF_NS_BEGIN

namespace Logging::Sinks {
class LoggerWindowSink;
}

namespace UI {
/**
 * @class LoggerWindow
 * @brief An ImGui window responsible for displaying logs collected by a LoggerWindowSink.
 */
class LoggerWindow : public BaseWindow, public Config::IConfigurable {
 public:
  LoggerWindow(const std::string& componentName, const std::string& windowId, Config::IConfigService& configService);

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) override;

 protected:
  void RenderContent() override;
  const char* GetWindowTitle() const override;

 private:
  void BuildComponentFilterList();
  void OnUISinkChanged(std::shared_ptr<Logging::Sinks::LoggerWindowSink> sink);

  std::shared_ptr<Logging::Sinks::LoggerWindowSink> m_sink;
  Config::IConfigService& m_configService;
  std::unique_ptr<Utils::Sink<void(std::shared_ptr<Logging::Sinks::LoggerWindowSink>)>> m_onUISinkChangedSink;

  // --- Cached Localization ---
  std::string m_cachedButtonClear;
  std::string m_cachedCheckboxAutoscroll;
  std::string m_cachedLabelLevel;
  std::string m_cachedLabelModule;
  std::string m_cachedContextCopyLine;
  std::string m_cachedContextCopyMessage;
  std::string m_cachedContextCopySelected;
  std::string m_cachedContextCopyAll;
  std::string m_cachedMsgCleanSession;

  // --- Selection State ---
  int m_selectionStart = -1;
  int m_selectionEnd = -1;

  // --- Filter State ---
  Logging::LogLevel m_filterLevel = Logging::LogLevel::Trace;
  std::string m_selectedComponent = "All";
  std::vector<std::string> m_componentList;
};
}  // namespace UI

SPF_NS_END
