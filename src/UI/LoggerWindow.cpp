#include "SPF/UI/LoggerWindow.hpp"
#include "SPF/Logging/Sinks/LoggerWindowSink.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/Logger.hpp"  // For LogLevelToString and GetAllLogLevels

#include <imgui.h>
#include <fmt/format.h>
#include <set>

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Logging;
using namespace SPF::Localization;

// Helper function to get color for a log level
static ImVec4 GetColorForLogLevel(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
    case LogLevel::Debug:
      return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light Blue
    case LogLevel::Info:
      return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);  // Light Gray
    case LogLevel::Warn:
      return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
    case LogLevel::Error:
      return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);  // Red
    case LogLevel::Critical:
      return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);  // Bright Red
    default:
      return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  }
}

LoggerWindow::LoggerWindow(const std::string& componentName, const std::string& windowId, Sinks::LoggerWindowSink& sink,
                           Config::IConfigService& configService)
    : BaseWindow(componentName, windowId), m_sink(sink), m_configService(configService) {
  m_defaultTitle = "Logger";
  m_titleLocalizationKey = "logger_window.title";
}

const char* LoggerWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str(); }

bool LoggerWindow::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) {
  // We only care about changes to the global logging level for our own component
  if (systemName == "logging" && componentName == m_componentName && keyPath == "level") {
    if (newValue.is_string()) {
      m_filterLevel = LogLevelFromString(newValue.get<std::string>());
    }
    return true;  // This was handled.
  }
  return false;  // This was not handled.
}

void LoggerWindow::BuildComponentFilterList() {
  std::set<std::string> uniqueLoggerNames;
  auto messages = m_sink.GetMessages();
  for (const auto& item : messages) {
    uniqueLoggerNames.insert(std::string(item.logger_name));
  }

  std::vector<std::string> frameworkComponents;
  std::vector<std::string> pluginComponents;

  for (const auto& name : uniqueLoggerNames) {
    if (m_configService.GetAllComponentInfo().count(name)) {
      pluginComponents.push_back(name);
    } else {
      frameworkComponents.push_back(name);
    }
  }

  std::sort(frameworkComponents.begin(), frameworkComponents.end());
  std::sort(pluginComponents.begin(), pluginComponents.end());

  m_componentList.clear();
  m_componentList.push_back("All");

  if (!pluginComponents.empty()) {
    m_componentList.push_back("###SEPARATOR###");
  }

  // Add plugins first, as requested
  if (!pluginComponents.empty()) {
    m_componentList.insert(m_componentList.end(), pluginComponents.begin(), pluginComponents.end());
  }

  // Add separator if there are both framework and plugin components
  if (!frameworkComponents.empty() && !pluginComponents.empty()) {
    m_componentList.push_back("###SEPARATOR###");  // Special marker for the separator
  }

  // Add framework components
  if (!frameworkComponents.empty()) {
    m_componentList.insert(m_componentList.end(), frameworkComponents.begin(), frameworkComponents.end());
  }
}

void LoggerWindow::RenderContent() {
  auto& l10n = LocalizationManager::GetInstance();

  // --- Toolbar ---
  if (ImGui::Button(l10n.Get("logger_window.button_clear").c_str())) {
    m_sink.Clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox(l10n.Get("logger_window.checkbox_autoscroll").c_str(), &m_autoScroll);

  ImGui::Separator();

  // --- Filters ---
  ImGui::PushItemWidth(120);
  // Log Level Filter
  if (ImGui::BeginCombo("##level_filter", LogLevelToString(m_filterLevel))) {
    const auto& allLevels = GetAllLogLevels();
    for (const auto& level : allLevels) {
      const char* levelName = LogLevelToString(level);
      bool is_selected = (m_filterLevel == level);
      if (ImGui::Selectable(levelName, is_selected)) {
        m_filterLevel = level;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  ImGui::SameLine();

  // Component Filter
  BuildComponentFilterList();
  if (ImGui::BeginCombo("##component_filter", m_selectedComponent.c_str())) {
    for (const auto& componentName : m_componentList) {
      if (componentName == "###SEPARATOR###") {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        continue;
      }

      bool is_selected = (m_selectedComponent == componentName);
      if (ImGui::Selectable(componentName.c_str(), is_selected)) {
        m_selectedComponent = componentName;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();

  ImGui::Separator();

  // --- Log output area ---
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

  // Pre-filter messages before passing to the clipper
  std::vector<const Sinks::LoggerWindowSink::DisplayMessage*> filteredMessages;
  auto allMessages = m_sink.GetMessages();
  filteredMessages.reserve(allMessages.size());
  for (const auto& item : allMessages) {
    // Apply filters - show all if TRACE is selected, otherwise exact match
    if (!(m_filterLevel == LogLevel::Trace || item.level == m_filterLevel)) continue;
    if (m_selectedComponent != "All" && item.logger_name != m_selectedComponent) continue;
    filteredMessages.push_back(&item);
  }

  ImGuiListClipper clipper;
  clipper.Begin(filteredMessages.size());
  while (clipper.Step()) {
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
      const auto* item = filteredMessages[row];

      ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLogLevel(item->level));
      ImGui::TextUnformatted(fmt::format("[{}] [{}] {}", LogLevelToString(item->level), item->logger_name, item->message).c_str());
      ImGui::PopStyleColor();
    }
  }
  clipper.End();

  if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    ImGui::SetScrollHereY(1.0f);
  }

  ImGui::EndChild();
}
}  // namespace UI
SPF_NS_END