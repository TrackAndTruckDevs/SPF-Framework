#include "SPF/UI/ToolsWindow.hpp"

#include "SPF/Events/EventManager.hpp"
#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/Hooks/GameLogHook.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Logging/Sinks/GameLogWindowSink.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "fmt/format.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace SPF::UI {
using namespace SPF::Hooks;
using namespace SPF::Localization;
using namespace SPF::Logging;

namespace {
constexpr int MAX_HISTORY_SIZE = 20;

ImVec4 GetColorForLogLevel(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return Colors::GRAY;
    case LogLevel::Debug:
      return Colors::VERY_LIGHT_BLUE;
    case LogLevel::Info:
      return Colors::VERY_LIGHT_GREEN;
    case LogLevel::Warn:
      return Colors::ORANGE;
    case LogLevel::Error:
      return Colors::RED;
    case LogLevel::Critical:
      return Colors::BRIGHT_RED;
    default:
      return Colors::WHITE;
  }
}

const char* GetIconForLogLevel(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return ICON_FA_BUG;
    case LogLevel::Debug:
      return ICON_FA_CODE;
    case LogLevel::Info:
      return ICON_FA_CIRCLE_INFO;
    case LogLevel::Warn:
      return ICON_FA_TRIANGLE_EXCLAMATION;
    case LogLevel::Error:
      return ICON_FA_CIRCLE_XMARK;
    case LogLevel::Critical:
      return ICON_FA_RADIATION;
    default:
      return ICON_FA_QUESTION;
  }
}
}  // namespace

ToolsWindow::ToolsWindow(const std::string& owner, const std::string& id, Events::EventManager& eventManager) : BaseWindow(owner, id), m_eventManager(eventManager), m_hookManager(HookManager::GetInstance()) {
  m_titleLocalizationKey = "tools_window.title";
  RefreshLocalization();
}

int ToolsWindow::HistoryCallback(ImGuiInputTextCallbackData* data) {
  ToolsWindow* self = (ToolsWindow*)data->UserData;
  if (!self) {
    return 0;
  }

  switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackHistory: {
      const int history_size = self->m_history.size();
      if (history_size == 0) {
        break;
      }

      if (data->EventKey == ImGuiKey_UpArrow) {
        self->m_historyPos--;
        if (self->m_historyPos < 0) {
          self->m_historyPos = 0;
        }
      } else if (data->EventKey == ImGuiKey_DownArrow) {
        self->m_historyPos++;
        if (self->m_historyPos >= history_size) {
          self->m_historyPos = history_size - 1;
        }
      }

      // Retrieve command from history
      if (self->m_historyPos >= 0 && self->m_historyPos < history_size) {
        const std::string& command = self->m_history[self->m_historyPos];
        data->DeleteChars(0, data->BufTextLen);
        data->InsertChars(0, command.c_str());
      }
      break;
    }
  }
  return 0;
}

void ToolsWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_cachedInfoText = loc.Get("tools_window.info_text");
  m_cachedSendButton = loc.Get("tools_window.send_button");
  m_cachedDumpCommands = loc.Get("tools_window.dump_commands");
  m_cachedGameLogLabel = loc.Get("tools_window.game_log_label");
  m_cachedCheckboxGameLogToFile = loc.Get("tools_window.checkbox_game_log_to_file");
  m_cachedEnableHookText = loc.Get("tools_window.enable_hook_text");
  m_cachedHooksWindowTitle = loc.Get("hooks_window.title");
  m_cachedTabConsoleLog = loc.Get("tools_window.tab_console_log");

  m_cachedButtonClear = loc.Get("logger_window.button_clear");
  m_cachedCheckboxAutoscroll = loc.Get("logger_window.checkbox_autoscroll");
  m_cachedLabelLevel = loc.Get("logger_window.label_level");
  m_cachedLabelModule = loc.Get("logger_window.label_module");
  m_cachedContextCopyLine = loc.Get("logger_window.context_copy_line");
  m_cachedContextCopyMessage = loc.Get("logger_window.context_copy_message");
  m_cachedContextCopySelected = loc.Get("logger_window.context_copy_selected");
  m_cachedContextCopyAll = loc.Get("logger_window.context_copy_all");
  m_cachedMsgCleanSession = loc.Get("logger_window.msg_clean_session");
}

void ToolsWindow::RenderContent() {
  if (ImGui::BeginTabBar("##tools_tabs")) {
    if (ImGui::BeginTabItem(m_cachedTabConsoleLog.c_str())) {
      RenderConsoleSection();
      ImGui::Separator();
      RenderGameLogSection();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void ToolsWindow::RenderConsoleSection() {
  auto* hook = m_hookManager.GetHook("GameConsole");
  if (hook && hook->IsEnabled()) {
    ImGui::TextWrapped("%s", m_cachedInfoText.c_str());

    auto execute = [this]() {
      if (strlen(m_commandBuffer) > 0) {
        m_eventManager.System.OnRequestExecuteCommand.Call({m_commandBuffer});
        m_history.push_back(m_commandBuffer);
        if (m_history.size() > MAX_HISTORY_SIZE) {
          m_history.erase(m_history.begin());
        }
        m_commandBuffer[0] = '\0';
        m_historyPos = -1;  // Reset history position after new command
      }
    };

    ImGui::PushItemWidth(-80.0f);  // Leave space for the button
    if (ImGui::InputText("##CommandInput", m_commandBuffer, sizeof(m_commandBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, &ToolsWindow::HistoryCallback, (void*)this)) {
      execute();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (Button(m_cachedSendButton.c_str(), TextStyle::DefaultButton(), ImVec2(-1.0f, 0))) {
      execute();
    }

    RenderSuggestions();

    if (Button(m_cachedDumpCommands.c_str(), TextStyle::DefaultButton(), ImVec2(-1.0f, 0))) {
      const auto& commands = GameConsole::GetInstance().GetCommandList();
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameConsole");
      if (logger) {
        logger->Info("Console command dump ({} commands):", commands.size());
        for (const auto& cmd : commands) {
          logger->Info("  {}", cmd);
        }
      }
    }
  } else {
    ImGui::Text(m_cachedEnableHookText.c_str(), m_cachedHooksWindowTitle.c_str());
  }
}

void ToolsWindow::RenderSuggestions() {
  const std::vector<std::string>& commands = GameConsole::GetInstance().GetCommandList();
  if (commands.empty() || m_commandBuffer[0] == '\0') {
    return;
  }

  std::string prefix(m_commandBuffer);
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

  std::vector<std::string> matches;
  for (const auto& cmd : commands) {
    std::string lowerCmd = cmd;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);
    if (lowerCmd.substr(0, prefix.length()) == prefix && lowerCmd != prefix) {
      matches.push_back(cmd);
    }
  }

  if (matches.empty()) {
    return;
  }

  float rowHeight = ImGui::GetFrameHeight();
  float fullHeight = matches.size() * rowHeight + ImGui::GetStyle().WindowPadding.y * 2;
  float maxHeight = 10 * rowHeight + ImGui::GetStyle().WindowPadding.y * 2;
  ImGui::BeginChild("##suggestions", ImVec2(0, ImMin(fullHeight, maxHeight)), false);
  for (const auto& cmd : matches) {
    if (ImGui::Selectable(cmd.c_str())) {
      strncpy(m_commandBuffer, cmd.c_str(), sizeof(m_commandBuffer) - 1);
      m_commandBuffer[sizeof(m_commandBuffer) - 1] = '\0';
      ImGui::SetKeyboardFocusHere(-1);
    }
  }
  ImGui::EndChild();
}

void ToolsWindow::RenderGameLogSection() {
  auto& sink = Sinks::GameLogWindowSink::GetInstance();
  auto& loc = LocalizationManager::GetInstance();

  // Title on the left, Clear + Autoscroll aligned to the right
  float clearWidth = 0.0f;
  {
    std::string clearLabel = std::string(ICON_FA_TRASH_CAN) + " " + m_cachedButtonClear;
    clearWidth = ImGui::CalcTextSize(clearLabel.c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
  }
  float cbTextWidth = ImGui::CalcTextSize(m_cachedCheckboxAutoscroll.c_str()).x;
  float checkboxWidth = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + cbTextWidth;
  float rightWidth = clearWidth + ImGui::GetStyle().ItemSpacing.x + checkboxWidth;

  Typography::Text(TextStyle::H3().Color(UI::Colors::WHITE), m_cachedGameLogLabel.c_str());
  ImGui::SameLine();
  float leftPosX = ImGui::GetCursorPosX();
  ImGui::SetCursorPosX(leftPosX + ImGui::GetContentRegionAvail().x - rightWidth);
  std::string clearLabel = std::string(ICON_FA_TRASH_CAN) + " " + m_cachedButtonClear;
  if (Button(clearLabel.c_str())) {
    sink.Clear();
  }
  ImGui::SameLine();
  ImGui::Checkbox(m_cachedCheckboxAutoscroll.c_str(), &m_autoScroll);

  ImGui::SameLine();
  bool logToFile = Hooks::GameLogHook::IsLogToFrameworkEnabled();
  if (ImGui::Checkbox(m_cachedCheckboxGameLogToFile.c_str(), &logToFile)) {
    m_eventManager.System.OnRequestSettingChange.Call({"framework", "logging.sinks.game_log", logToFile});
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::SetTooltip("%s", loc.Get("tools_window.tooltip_game_log_to_file").c_str());
  }

  // Level filter buttons on the left
  ImGui::SameLine();
  ImGui::SetCursorPosX(leftPosX);

  for (const auto& level : GetAllLogLevels()) {
    if (level == LogLevel::Unknown) continue;

    bool isSelected = (m_filterLevel == level);
    ImVec4 color = GetColorForLogLevel(level);
    std::string label = fmt::format("{} {}", GetIconForLogLevel(level), LogLevelToString(level));

    if (isSelected) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(color.x * 0.25f, color.y * 0.25f, color.z * 0.25f, color.w));
    }
    auto btnStyle = TextStyle::DefaultButton().Color(color).HoverColor(color).ActiveColor(ImVec4(0.15f, 0.19f, 0.24f, 1.00f));
    if (Button(label.c_str(), btnStyle, ImVec2(0, 0), m_cachedLabelLevel.c_str())) {
      m_filterLevel = level;
    }
    if (isSelected) {
      ImGui::PopStyleColor();
    }

    if (level != LogLevel::Critical) {
      ImGui::SameLine();
    }
  }

  ImGui::Separator();

  // Module filter combo (built from collected game log messages)
  auto allMessages = sink.GetMessages();
  {
    std::set<std::string> uniqueLoggerNames;
    for (const auto& item : allMessages) {
      uniqueLoggerNames.insert(item.logger_name);
    }
    m_componentList.clear();
    m_componentList.push_back("All");
    for (const auto& name : uniqueLoggerNames) {
      m_componentList.push_back(name);
    }
  }

  float maxNameWidth = 0.0f;
  for (const auto& name : m_componentList) {
    float w = ImGui::CalcTextSize(name.c_str()).x;
    if (w > maxNameWidth) maxNameWidth = w;
  }
  float comboWidth = ImMax(maxNameWidth + ImGui::GetStyle().FramePadding.x * 2.0f, ImGui::GetContentRegionAvail().x * 0.35f);
  ImGui::PushItemWidth(comboWidth);

  if (ImGui::BeginCombo("##component_filter", m_selectedComponent.c_str())) {
    for (const auto& componentName : m_componentList) {
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

  if (allMessages.empty()) {
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() * 0.5f - ImGui::CalcTextSize(m_cachedMsgCleanSession.c_str()).x * 0.5f, ImGui::GetWindowHeight() * 0.5f));
    ImGui::TextDisabled("%s", m_cachedMsgCleanSession.c_str());
    ImGui::EndChild();
    return;
  }

  std::vector<const Sinks::GameLogWindowSink::DisplayMessage*> filteredMessages;
  filteredMessages.reserve(allMessages.size());
  for (const auto& item : allMessages) {
    if (!(m_filterLevel == LogLevel::Trace || item.level == m_filterLevel)) continue;
    if (m_selectedComponent != "All" && item.logger_name != m_selectedComponent) continue;
    filteredMessages.push_back(&item);
  }

  // Handle Ctrl+C for selection
  if (ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
    if (m_selectionStart != -1 && m_selectionEnd != -1) {
      int start = (std::min)(m_selectionStart, m_selectionEnd);
      int end = (std::max)(m_selectionStart, m_selectionEnd);
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
        int start = (std::min)(m_selectionStart, m_selectionEnd);
        int end = (std::max)(m_selectionStart, m_selectionEnd);
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
      ImGui::PushStyleColor(ImGuiCol_Text, Colors::WHITE);  // Force white text for menu
      if (ImGui::BeginPopupContextItem()) {
        // If we right-click outside current selection, select only this line
        if (!isSelected) {
          m_selectionStart = row;
          m_selectionEnd = row;
        }

        if (m_selectionStart != m_selectionEnd) {
          if (ImGui::MenuItem(m_cachedContextCopySelected.c_str())) {
            int start = (std::min)(m_selectionStart, m_selectionEnd);
            int end = (std::max)(m_selectionStart, m_selectionEnd);
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
      ImGui::PopStyleColor();  // Pop WHITE text

      ImGui::PopStyleColor();  // Pop log level color
      ImGui::PopID();
    }
  }
  clipper.End();

  if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
}
}  // namespace SPF::UI
