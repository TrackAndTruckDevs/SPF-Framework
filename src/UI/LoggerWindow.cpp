#include "SPF/UI/LoggerWindow.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/Logging/Sinks/LoggerWindowSink.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/UI/UIStyle.hpp"

#include <imgui.h>
#include <fmt/format.h>
#include <set>

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Logging;
using namespace SPF::Localization;

static ImVec4 GetColorForLogLevel(LogLevel level) {
  switch (level) {
    case LogLevel::Trace: return Colors::GRAY;
    case LogLevel::Debug: return Colors::VERY_LIGHT_BLUE;
    case LogLevel::Info:  return Colors::VERY_LIGHT_GREEN;
    case LogLevel::Warn:  return Colors::ORANGE;
    case LogLevel::Error: return Colors::RED;
    case LogLevel::Critical: return Colors::BRIGHT_RED;
    default: return Colors::WHITE;
  }
}

LoggerWindow::LoggerWindow(const std::string& componentName, const std::string& windowId,
                           Config::IConfigService& configService)
    : BaseWindow(componentName, windowId), m_configService(configService) {
  m_defaultTitle = "Logger";
  m_titleLocalizationKey = "logger_window.title";

  auto& loc = LocalizationManager::GetInstance();
  m_cachedButtonClear = loc.Get("logger_window.button_clear");
  m_cachedCheckboxAutoscroll = loc.Get("logger_window.checkbox_autoscroll");
  m_cachedLabelLevel = loc.Get("logger_window.label_level");
  m_cachedLabelModule = loc.Get("logger_window.label_module");
  m_cachedContextCopyLine = loc.Get("logger_window.context_copy_line");
  m_cachedContextCopyMessage = loc.Get("logger_window.context_copy_message");
  m_cachedContextCopySelected = loc.Get("logger_window.context_copy_selected");
  m_cachedContextCopyAll = loc.Get("logger_window.context_copy_all");
  m_cachedMsgCleanSession = loc.Get("logger_window.msg_clean_session");

  m_sink = LoggerFactory::GetInstance().GetUISink();
  SetVisibility(m_sink != nullptr);

  m_onUISinkChangedSink = std::make_unique<Utils::Sink<void(std::shared_ptr<Logging::Sinks::LoggerWindowSink>)>>(LoggerFactory::GetInstance().OnUISinkChanged);
  m_onUISinkChangedSink->Connect<&LoggerWindow::OnUISinkChanged>(this);
}

void LoggerWindow::OnUISinkChanged(std::shared_ptr<Logging::Sinks::LoggerWindowSink> sink) {
    m_sink = sink;
    SetVisibility(sink != nullptr);
}

const char* LoggerWindow::GetWindowTitle() const { 
    return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str(); 
}

bool LoggerWindow::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) {
  if (systemName == "logging" && componentName == m_componentName && keyPath == "level") {
    if (newValue.is_string()) {
      m_filterLevel = LogLevelFromString(newValue.get<std::string>());
    }
    return true;
  }
  return false;
}

void LoggerWindow::BuildComponentFilterList() {
  if (!m_sink) return;
  std::set<std::string> uniqueLoggerNames;
  auto messages = m_sink->GetMessages();
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
  if (!pluginComponents.empty()) m_componentList.push_back("###SEPARATOR###");
  if (!pluginComponents.empty()) m_componentList.insert(m_componentList.end(), pluginComponents.begin(), pluginComponents.end());
  if (!frameworkComponents.empty() && !pluginComponents.empty()) m_componentList.push_back("###SEPARATOR###");
  if (!frameworkComponents.empty()) m_componentList.insert(m_componentList.end(), frameworkComponents.begin(), frameworkComponents.end());
}

void LoggerWindow::RenderContent() {
  if (!m_sink) return;

  if (Button(m_cachedButtonClear.c_str())) {
    m_sink->Clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox(m_cachedCheckboxAutoscroll.c_str(), &m_autoScroll);
  ImGui::Separator();

  ImGui::PushItemWidth(120);
  if (ImGui::BeginCombo("##level_filter", LogLevelToString(m_filterLevel))) {
    for (const auto& level : GetAllLogLevels()) {
      bool is_selected = (m_filterLevel == level);
      if (ImGui::Selectable(LogLevelToString(level), is_selected)) m_filterLevel = level;
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", m_cachedLabelLevel.c_str());

  ImGui::SameLine();

  BuildComponentFilterList();
  if (ImGui::BeginCombo("##component_filter", m_selectedComponent.c_str())) {
    for (const auto& componentName : m_componentList) {
      if (componentName == "###SEPARATOR###") {
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        continue;
      }
      bool is_selected = (m_selectedComponent == componentName);
      if (ImGui::Selectable(componentName.c_str(), is_selected)) m_selectedComponent = componentName;
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", m_cachedLabelModule.c_str());

  ImGui::PopItemWidth();
  ImGui::Separator();

  ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
  
  auto allMessages = m_sink->GetMessages();
  if (allMessages.empty()) {
      ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize(m_cachedMsgCleanSession.c_str()).x * 0.5f, ImGui::GetWindowHeight() * 0.5f));
      ImGui::TextDisabled("%s", m_cachedMsgCleanSession.c_str());
      ImGui::EndChild();
      return;
  }

  std::vector<const Sinks::LoggerWindowSink::DisplayMessage*> filteredMessages;
  filteredMessages.reserve(allMessages.size());
  for (const auto& item : allMessages) {
    if (!(m_filterLevel == LogLevel::Trace || item.level == m_filterLevel)) continue;
    if (m_selectedComponent != "All" && item.logger_name != m_selectedComponent) continue;
    filteredMessages.push_back(&item);
  }

  // Handle Ctrl+C for selection
  if (ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
      if (m_selectionStart != -1 && m_selectionEnd != -1) {
          int start = std::min(m_selectionStart, m_selectionEnd);
          int end = std::max(m_selectionStart, m_selectionEnd);
          std::string copiedText;
          for (int i = start; i <= end && i < filteredMessages.size(); ++i) {
              const auto* log = filteredMessages[i];
              copiedText += fmt::format("[{}] [{}] {}\n", LogLevelToString(log->level), log->logger_name, log->message);
          }
          if (!copiedText.empty()) ImGui::SetClipboardText(copiedText.c_str());
      }
  }

  ImGuiListClipper clipper;
  clipper.Begin(filteredMessages.size());
  while (clipper.Step()) {
    for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
      const auto* item = filteredMessages[row];
      std::string lineText = fmt::format("[{}] [{}] {}", LogLevelToString(item->level), item->logger_name, item->message);
      
      ImGui::PushID(row);
      
      // Determine if this row is within the selection range
      bool isSelected = false;
      if (m_selectionStart != -1 && m_selectionEnd != -1) {
          int start = std::min(m_selectionStart, m_selectionEnd);
          int end = std::max(m_selectionStart, m_selectionEnd);
          isSelected = (row >= start && row <= end);
      }

      ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLogLevel(item->level));
      
      // Use Selectable to allow highlighting and multi-selection
      if (ImGui::Selectable(lineText.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
          if (ImGui::GetIO().KeyShift && m_selectionStart != -1) {
              m_selectionEnd = row;
          } else {
              m_selectionStart = row;
              m_selectionEnd = row;
          }
      }

      // Handle drag selection
      if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
          m_selectionEnd = row;
      }

      // Right-click context menu
      ImGui::PushStyleColor(ImGuiCol_Text, Colors::WHITE); // Force white text for menu
      if (ImGui::BeginPopupContextItem()) {
          // If we right-click outside current selection, select only this line
          if (!isSelected) {
              m_selectionStart = row;
              m_selectionEnd = row;
          }

          if (m_selectionStart != m_selectionEnd) {
              if (ImGui::MenuItem(m_cachedContextCopySelected.c_str())) {
                  int start = std::min(m_selectionStart, m_selectionEnd);
                  int end = std::max(m_selectionStart, m_selectionEnd);
                  std::string batchText;
                  for (int i = start; i <= end && i < filteredMessages.size(); ++i) {
                      const auto* log = filteredMessages[i];
                      batchText += fmt::format("[{}] [{}] {}\n", LogLevelToString(log->level), log->logger_name, log->message);
                  }
                  ImGui::SetClipboardText(batchText.c_str());
              }
              ImGui::Separator();
          }

          if (ImGui::MenuItem(m_cachedContextCopyLine.c_str())) {
              ImGui::SetClipboardText(lineText.c_str());
          }
          if (ImGui::MenuItem(m_cachedContextCopyMessage.c_str())) {
              ImGui::SetClipboardText(item->message.c_str());
          }
          ImGui::Separator();
          if (ImGui::MenuItem(m_cachedContextCopyAll.c_str())) {
              std::string allVisible;
              for (const auto* log : filteredMessages) {
                  allVisible += fmt::format("[{}] [{}] {}\n", LogLevelToString(log->level), log->logger_name, log->message);
              }
              ImGui::SetClipboardText(allVisible.c_str());
          }
          ImGui::EndPopup();
      }
      ImGui::PopStyleColor(); // Pop WHITE text

      ImGui::PopStyleColor(); // Pop log level color
      ImGui::PopID();
    }
  }
  clipper.End();

  if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
}
}  // namespace UI
SPF_NS_END
