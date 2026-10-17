#include "SPF/UI/ToolsWindow.hpp"

#include "SPF/Events/EventManager.hpp"
#include "SPF/Data/GameData/MailService.hpp"
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
#include <array>
#include <cctype>
#include <cstring>
#include <set>
#include <string>
#include <unordered_map>
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
      const int matchCount = (int)self->m_suggestionMatches.size();
      if (matchCount > 0) {
        if (self->m_suggestionIndex < -1 || self->m_suggestionIndex >= matchCount) {
          self->m_suggestionIndex = -1;
        }
        if (data->EventKey == ImGuiKey_DownArrow) {
          self->m_suggestionIndex = (self->m_suggestionIndex + 1) % matchCount;
        } else if (data->EventKey == ImGuiKey_UpArrow) {
          self->m_suggestionIndex = (self->m_suggestionIndex <= 0) ? matchCount - 1 : self->m_suggestionIndex - 1;
        } else {
          break;
        }
        return 1;
      }

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
  m_cachedEnableCheats = loc.Get("tools_window.enable_cheats");
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

  m_cachedTabMail = loc.Get("tools_window.tab_mail");
  m_cachedMailSendSection = loc.Get("tools_window.mail.send_section");
  m_cachedMailModeLabel = loc.Get("tools_window.mail.mode_label");
  m_cachedMailModeCustom = loc.Get("tools_window.mail.mode_custom");
  m_cachedMailGameSection = loc.Get("tools_window.mail.game_section");
  m_cachedMailGameHint = loc.Get("tools_window.mail.game_hint");
  m_cachedMailGameTemplate = loc.Get("tools_window.mail.game_template");
  m_cachedMailGameNoPlaceholders = loc.Get("tools_window.mail.game_no_placeholders");
  m_cachedMailGameEmptyList = loc.Get("tools_window.mail.game_empty_list");
  m_cachedMailGameSelectPrompt = loc.Get("tools_window.mail.game_select_prompt");
  m_cachedMailCustomInvalid = loc.Get("tools_window.mail.custom_invalid");
  m_cachedMailSendButton = loc.Get("tools_window.mail.send_button");
  m_cachedMailInbox = loc.Get("tools_window.mail.inbox");
  m_cachedMailPending = loc.Get("tools_window.mail.pending");
  m_cachedMailColId = loc.Get("tools_window.mail.col_id");
  m_cachedMailColName = loc.Get("tools_window.mail.col_name");
  m_cachedMailColRead = loc.Get("tools_window.mail.col_read");
  m_cachedMailColUnread = loc.Get("tools_window.mail.col_unread");
  m_cachedMailRead = loc.Get("tools_window.mail.read");
  m_cachedMailDelete = loc.Get("tools_window.mail.delete");
  m_cachedMailFrom = loc.Get("tools_window.mail.from");
  m_cachedMailSubject = loc.Get("tools_window.mail.subject");
  m_cachedMailBody = loc.Get("tools_window.mail.body");
  m_cachedMailNoSelection = loc.Get("tools_window.mail.no_selection");
  m_cachedMailEmptyList = loc.Get("tools_window.mail.empty_list");
  m_cachedMailServiceNotReady = loc.Get("tools_window.mail.service_not_ready");
  m_cachedMailCreateSection = loc.Get("tools_window.mail.create_section");
  m_cachedMailCreateHint = loc.Get("tools_window.mail.create_hint");
  m_cachedMailCreateButton = loc.Get("tools_window.mail.create_button");
  m_cachedMailOwner = loc.Get("tools_window.mail.owner");
  m_cachedMailTag = loc.Get("tools_window.mail.tag");
  m_cachedMailOwnerTooltip = loc.Get("tools_window.mail.owner_tooltip");
  m_cachedMailTagTooltip = loc.Get("tools_window.mail.tag_tooltip");
}

void ToolsWindow::RenderContent() {
  if (ImGui::BeginTabBar("##tools_tabs")) {
    if (ImGui::BeginTabItem(m_cachedTabConsoleLog.c_str())) {
      RenderConsoleSection();
      ImGui::Separator();
      RenderGameLogSection();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem(m_cachedTabMail.c_str())) {
      RenderMailSection();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void ToolsWindow::RenderConsoleSection() {
  auto* hook = m_hookManager.GetHook("GameConsole");
  if (hook && hook->IsEnabled()) {
    ImGui::TextWrapped("%s", m_cachedInfoText.c_str());

    bool cheatsEnabled = GameConsole::GetInstance().IsCheatsEnabled();
    if (ImGui::Checkbox(m_cachedEnableCheats.c_str(), &cheatsEnabled)) {
      GameConsole::GetInstance().Execute(cheatsEnabled ? "g_developer 1" : "g_developer 0");
    }

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

    UpdateSuggestionMatches();

    const bool focusQueued = m_focusCommandInput;
    if (focusQueued) {
      ImGui::SetKeyboardFocusHere(-1);
      m_focusCommandInput = false;
    }

    const bool keepActive = ImGui::GetIO().ConfigInputTextEnterKeepActive;
    ImGui::GetIO().ConfigInputTextEnterKeepActive = true;
    ImGui::PushItemWidth(-80.0f);  // Leave space for the button
    const bool entered = ImGui::InputText("##CommandInput", m_commandBuffer, sizeof(m_commandBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory, &ToolsWindow::HistoryCallback, (void*)this);
    ImGui::PopItemWidth();
    ImGui::GetIO().ConfigInputTextEnterKeepActive = keepActive;
    const ImGuiID inputId = ImGui::GetID("##CommandInput");

    if (entered) {
      const int matchCount = (int)m_suggestionMatches.size();
      if (m_suggestionIndex >= 0 && m_suggestionIndex < matchCount) {
        strncpy(m_commandBuffer, m_suggestionMatches[m_suggestionIndex].c_str(), sizeof(m_commandBuffer) - 1);
        m_commandBuffer[sizeof(m_commandBuffer) - 1] = '\0';
        m_suggestionIndex = -1;
      } else {
        execute();
      }
      SyncCommandInputCursor(inputId);
    } else if (focusQueued) {
      SyncCommandInputCursor(inputId);
    }

    ImGui::SameLine();

    if (Button(m_cachedSendButton.c_str(), TextStyle::DefaultButton(), ImVec2(-1.0f, 0))) {
      execute();
      SyncCommandInputCursor(inputId);
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

void ToolsWindow::UpdateSuggestionMatches() {
  m_suggestionMatches.clear();

  const std::vector<std::string>& commands = GameConsole::GetInstance().GetCommandList();
  if (commands.empty() || m_commandBuffer[0] == '\0') {
    m_suggestionIndex = -1;
    return;
  }

  std::string prefix(m_commandBuffer);
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);

  for (const auto& cmd : commands) {
    std::string lowerCmd = cmd;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);
    if (lowerCmd.substr(0, prefix.length()) == prefix && lowerCmd != prefix) {
      m_suggestionMatches.push_back(cmd);
    }
  }

  if (m_suggestionIndex >= (int)m_suggestionMatches.size()) {
    m_suggestionIndex = -1;
  }
}

void ToolsWindow::SyncCommandInputCursor(ImGuiID inputId) {
  if (ImGuiInputTextState* state = ImGui::GetInputTextState(inputId)) {
    state->ReloadUserBufAndMoveToEnd();
  }
}

void ToolsWindow::RenderSuggestions() {
  const int matchCount = (int)m_suggestionMatches.size();
  if (matchCount == 0) {
    return;
  }

  float rowHeight = ImGui::GetFrameHeight();
  float fullHeight = matchCount * rowHeight + ImGui::GetStyle().WindowPadding.y * 2;
  float maxHeight = 10 * rowHeight + ImGui::GetStyle().WindowPadding.y * 2;
  ImGui::BeginChild("##suggestions", ImVec2(0, ImMin(fullHeight, maxHeight)), false);
  for (int i = 0; i < matchCount; i++) {
    const bool isSelected = (i == m_suggestionIndex);
    if (ImGui::Selectable(m_suggestionMatches[i].c_str(), isSelected)) {
      strncpy(m_commandBuffer, m_suggestionMatches[i].c_str(), sizeof(m_commandBuffer) - 1);
      m_commandBuffer[sizeof(m_commandBuffer) - 1] = '\0';
      m_suggestionIndex = -1;
      m_focusCommandInput = true;
    }
    if (isSelected) {
      ImGui::SetItemDefaultFocus();
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

namespace {
// Collects unique $PLACEHOLDER$ tokens from a mail_text template in order of
// appearance. The game substitutes each key into From/Subject/Body at once, so
// duplicates across fields collapse into a single editor row.
std::vector<std::string> ExtractMailPlaceholders(const std::string& text) {
  std::vector<std::string> keys;
  size_t pos = 0;
  while ((pos = text.find('$', pos)) != std::string::npos) {
    size_t end = text.find('$', pos + 1);
    if (end == std::string::npos) break;
    if (end > pos + 1) {
      std::string key = text.substr(pos + 1, end - pos - 1);
      if (std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
    }
    pos = end + 1;
  }
  return keys;
}

// Game def name for a mail_text unit: "mailtext.x" -> "mail.x" (verified pair
// mailtext.ovs.new_routes / mail.ovs.new_routes), otherwise "mail.<name>".
std::string MailTextToDefName(const std::string& mailTextName) {
  const std::string kPrefix = "mailtext.";
  if (mailTextName.rfind(kPrefix, 0) == 0) return "mail." + mailTextName.substr(kPrefix.size());
  return "mail." + mailTextName;
}

// Fixed widget sizing for the mail tab: inputs are intentionally NOT stretched
// across the whole window.
constexpr float kMailFieldW = 460.0f;   // input/preview column width
constexpr float kMailLabelCol = 170.0f; // label column before SameLine inputs
constexpr float kMailParamW = 320.0f;   // placeholder value inputs
}  // namespace

void ToolsWindow::RebuildMailGameParams() {
  m_gameParamKeys.clear();
  m_gameParamBufs.clear();
  m_paramsBuiltFor = m_selectedMailText;
  for (const auto& entry : m_mailTextCache) {
    if (entry.name != m_selectedMailText) continue;
    for (const std::string* field : {&entry.from, &entry.subject, &entry.body}) {
      for (const std::string& key : ExtractMailPlaceholders(*field)) {
        if (m_gameParamBufs.find(key) != m_gameParamBufs.end()) continue;
        m_gameParamKeys.push_back(key);
        m_gameParamBufs.emplace(key, std::array<char, 128>{});
      }
    }
    break;
  }
}

void ToolsWindow::RenderMailSection() {
  auto& mail = Data::GameData::MailService::GetInstance();

  if (!mail.IsReady()) {
    ImGui::TextWrapped("%s", m_cachedMailServiceNotReady.c_str());
    return;
  }

  // --- Top: mail type selector (custom demo vs shipped game mailtext) ---
  Typography::Text(TextStyle::H3().Color(Colors::WHITE), m_cachedMailSendSection.c_str());
  ImGui::TextUnformatted(m_cachedMailModeLabel.c_str());

  const char* comboPreview = m_cachedMailModeCustom.c_str();
  if (m_mailMode == 1) {
    comboPreview = m_selectedMailText.empty() ? m_cachedMailGameSelectPrompt.c_str()
                                              : m_selectedMailText.c_str();
  }
  ImGui::SetNextItemWidth(kMailFieldW);
  const bool comboOpen = ImGui::BeginCombo("##mail_mode", comboPreview);
  if (comboOpen && !m_mailComboWasOpen) {
    // Shipped mail_text units are static per locale - rebuild the list only
    // when the dropdown is (re)opened instead of every frame.
    m_mailTextCache.clear();
    std::vector<Data::GameData::MailService::MailTextInfo> texts;
    if (mail.EnumerateMailTexts(texts)) {
      for (const auto& t : texts) {
        if (t.name.empty()) continue;
        m_mailTextCache.push_back({t.name, t.from, t.subject, t.body});
      }
    }
    if (!m_selectedMailText.empty()) {
      bool found = false;
      for (const auto& e : m_mailTextCache) {
        if (e.name == m_selectedMailText) {
          found = true;
          break;
        }
      }
      if (!found) {
        m_selectedMailText.clear();
        m_paramsBuiltFor.clear();
        m_mailMode = 0;
      }
    }
  }
  m_mailComboWasOpen = comboOpen;
  if (comboOpen) {
    if (ImGui::Selectable(m_cachedMailModeCustom.c_str(), m_mailMode == 0)) m_mailMode = 0;
    if (!m_mailTextCache.empty()) ImGui::Separator();
    for (size_t i = 0; i < m_mailTextCache.size(); ++i) {
      ImGui::PushID(static_cast<int>(i));
      const bool selected = (m_mailMode == 1 && m_selectedMailText == m_mailTextCache[i].name);
      if (ImGui::Selectable(m_mailTextCache[i].name.c_str(), selected)) {
        m_mailMode = 1;
        m_selectedMailText = m_mailTextCache[i].name;
      }
      ImGui::PopID();
    }
    if (m_mailTextCache.empty()) ImGui::TextDisabled("%s", m_cachedMailGameEmptyList.c_str());
    ImGui::EndCombo();
  }
  ImGui::Spacing();

  if (m_mailMode == 0) {
    // ---------- Custom mail (API demo) ----------
    Typography::Text(TextStyle::H3().Color(Colors::WHITE), m_cachedMailCreateSection.c_str());
    ImGui::TextDisabled("%s", m_cachedMailCreateHint.c_str());
    ImGui::Spacing();

    ImGui::TextUnformatted(m_cachedMailOwner.c_str());
    ImGui::SameLine(kMailLabelCol);
    ImGui::SetNextItemWidth(kMailFieldW);
    ImGui::InputText("##mail_custom_owner", m_customMailOwnerBuf, sizeof(m_customMailOwnerBuf));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
      ImGui::SetTooltip("%s", m_cachedMailOwnerTooltip.c_str());
    }

    ImGui::Spacing();
    ImGui::TextUnformatted(m_cachedMailTag.c_str());
    ImGui::SameLine(kMailLabelCol);
    ImGui::SetNextItemWidth(kMailFieldW);
    ImGui::InputText("##mail_custom_tag", m_customMailTagBuf, sizeof(m_customMailTagBuf));
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
      ImGui::SetTooltip("%s", m_cachedMailTagTooltip.c_str());
    }

    ImGui::Spacing();
    ImGui::TextUnformatted(m_cachedMailFrom.c_str());
    ImGui::SameLine(kMailLabelCol);
    ImGui::SetNextItemWidth(kMailFieldW);
    ImGui::InputText("##mail_custom_from", m_customMailFromBuf, sizeof(m_customMailFromBuf));

    ImGui::Spacing();
    ImGui::TextUnformatted(m_cachedMailSubject.c_str());
    ImGui::SameLine(kMailLabelCol);
    ImGui::SetNextItemWidth(kMailFieldW);
    ImGui::InputText("##mail_custom_subject", m_customMailSubjectBuf, sizeof(m_customMailSubjectBuf));

    ImGui::Spacing();
    ImGui::TextUnformatted(m_cachedMailBody.c_str());
    ImGui::SetNextItemWidth(kMailFieldW);
    ImGui::InputTextMultiline("##mail_custom_body", m_customMailBodyBuf, sizeof(m_customMailBodyBuf),
                              ImVec2(0.0f, ImGui::GetFrameHeight() * 5));

    ImGui::Spacing();
    const bool canSendCustom = m_customMailOwnerBuf[0] != '\0' && m_customMailTagBuf[0] != '\0';
    ImGui::PushID("framework_mail_custom_send");
    ImGui::BeginDisabled(!canSendCustom);
    if (Button(m_cachedMailCreateButton.c_str(), TextStyle::DefaultButton(), ImVec2(kMailFieldW, 0))) {
      mail.SendCustomMail(m_customMailFromBuf, m_customMailSubjectBuf, m_customMailBodyBuf,
                          m_customMailOwnerBuf, m_customMailTagBuf);
    }
    ImGui::EndDisabled();
    ImGui::PopID();
    if (!canSendCustom) ImGui::TextDisabled("%s", m_cachedMailCustomInvalid.c_str());
  } else {
    // ---------- Game mailtext (template + placeholder values) ----------
    Typography::Text(TextStyle::H3().Color(Colors::WHITE), m_cachedMailGameSection.c_str());
    ImGui::TextDisabled("%s", m_cachedMailGameHint.c_str());
    ImGui::Spacing();

    const MailTextEntry* entry = nullptr;
    for (const auto& e : m_mailTextCache) {
      if (e.name == m_selectedMailText) {
        entry = &e;
        break;
      }
    }
    if (entry == nullptr) {
      ImGui::TextDisabled("%s", m_cachedMailGameSelectPrompt.c_str());
    } else {
      if (m_paramsBuiltFor != m_selectedMailText) RebuildMailGameParams();

      // Raw template with $PLACEHOLDER$ tokens - exactly what the receiver's
      // Compose will substitute before showing the letter.
      ImGui::TextUnformatted(m_cachedMailGameTemplate.c_str());
      ImGui::BeginChild("##mail_template", ImVec2(kMailFieldW, ImGui::GetFrameHeight() * 4), true);
      ImGui::TextColored(Colors::VERY_LIGHT_BLUE, "%s: %s", m_cachedMailFrom.c_str(),
                         entry->from.c_str());
      ImGui::TextColored(Colors::VERY_LIGHT_BLUE, "%s: %s", m_cachedMailSubject.c_str(),
                         entry->subject.c_str());
      ImGui::Separator();
      ImGui::TextWrapped("%s", entry->body.c_str());
      ImGui::EndChild();

      ImGui::Spacing();
      if (m_gameParamKeys.empty()) {
        ImGui::TextDisabled("%s", m_cachedMailGameNoPlaceholders.c_str());
      } else {
        for (size_t i = 0; i < m_gameParamKeys.size(); ++i) {
          ImGui::PushID(static_cast<int>(i));
          ImGui::Text("$%s$", m_gameParamKeys[i].c_str());
          ImGui::SameLine(kMailLabelCol);
          ImGui::SetNextItemWidth(kMailParamW);
          auto it = m_gameParamBufs.find(m_gameParamKeys[i]);
          if (it != m_gameParamBufs.end()) {
            ImGui::InputText("##mail_param", it->second.data(), it->second.size());
          }
          ImGui::Spacing();
          ImGui::PopID();
        }
      }

      ImGui::PushID("framework_mail_game_send");
      if (Button(m_cachedMailSendButton.c_str(), TextStyle::DefaultButton(), ImVec2(kMailParamW, 0))) {
        Data::GameData::MailService::Params params;
        for (const std::string& key : m_gameParamKeys) {
          auto it = m_gameParamBufs.find(key);
          if (it == m_gameParamBufs.end() || it->second[0] == '\0') continue;  // empty value: placeholder stays
          params.emplace_back(key, std::string(it->second.data()));
        }
        mail.Send(MailTextToDefName(entry->name).c_str(), params);
      }
      ImGui::PopID();
    }
  }

  // --- Letters (no refresh buttons: new letters appear on their own) ---
  ImGui::Separator();
  RenderMailList(false);
  ImGui::Separator();
  RenderMailList(true);

  // Reader: always rendered at a fixed height so selecting a letter never
  // shifts the layout.
  ImGui::Separator();
  ImGui::BeginChild("##mail_reader", ImVec2(-1.0f, ImGui::GetFrameHeight() * 8), true);
  if (m_selectedMailId >= 0 && m_mailBodyLoaded) {
    ImGui::TextColored(Colors::VERY_LIGHT_BLUE, "%s: %s", m_cachedMailFrom.c_str(),
                       m_mailBodyFrom.c_str());
    ImGui::TextColored(Colors::VERY_LIGHT_BLUE, "%s: %s", m_cachedMailSubject.c_str(),
                       m_mailBodySubject.c_str());
    ImGui::Separator();
    ImGui::TextWrapped("%s", m_mailBodyText.c_str());
  } else {
    ImGui::TextDisabled("%s", m_cachedMailNoSelection.c_str());
  }
  ImGui::EndChild();
}

void ToolsWindow::RenderMailList(bool pending) {
  auto& mail = Data::GameData::MailService::GetInstance();

  ImGui::PushID(pending ? "mail_pending" : "mail_inbox");

  const char* sectionTitle = pending ? m_cachedMailPending.c_str() : m_cachedMailInbox.c_str();
  Typography::Text(TextStyle::H3().Color(Colors::WHITE), sectionTitle);

  const int count = pending ? mail.GetPendingCount() : mail.GetInboxCount();
  if (count <= 0) {
    ImGui::TextDisabled("%s", m_cachedMailEmptyList.c_str());
    ImGui::PopID();
    return;
  }

  if (ImGui::BeginTable("##mail_table", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                        ImVec2(0, ImGui::GetFrameHeight() * 6))) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(m_cachedMailColId.c_str(), ImGuiTableColumnFlags_WidthFixed, 40.0f);
    ImGui::TableSetupColumn(m_cachedMailColName.c_str(), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(m_cachedMailSubject.c_str(), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(m_cachedMailColRead.c_str(), ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn("##actions", ImGuiTableColumnFlags_WidthFixed, 175.0f);
    ImGui::TableHeadersRow();

    ImGuiListClipper clipper;
    clipper.Begin(count);
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
        Data::GameData::MailService::MailInfo info;
        if (!mail.GetMailInfo(static_cast<size_t>(row), pending, info)) continue;

        ImGui::TableNextRow();
        ImGui::PushID(row);

        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", info.id);

        // Name column: custom letters also show their owner/tag identity.
        std::string label = info.name;
        if (!info.owner.empty()) label += " - " + info.owner + "/" + info.tag;

        ImGui::TableSetColumnIndex(1);
        const bool isSelected = (m_selectedMailId == info.id && m_selectedMailPending == pending);
        // Plain text: no row-wide Selectable — opening a letter is the Read
        // button's job, row highlight would cover it.
        ImGui::TextWrapped("%s", label.c_str());

        ImGui::TableSetColumnIndex(2);
        ImGui::TextUnformatted(info.subject.c_str());

        ImGui::TableSetColumnIndex(3);
        const char* readLabel = info.read ? m_cachedMailColRead.c_str() : m_cachedMailColUnread.c_str();
        const ImVec4 readColor = info.read ? Colors::GRAY : Colors::VERY_LIGHT_GREEN;
        ImGui::TextColored(readColor, "%s", readLabel);

        ImGui::TableSetColumnIndex(4);
        const float avail = ImGui::GetContentRegionAvail().x;
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        if (pending) {
          if (Button(m_cachedMailDelete.c_str(), TextStyle::DefaultButton(), ImVec2(avail, 0))) {
            mail.DeleteMail(info.index, pending);
            if (isSelected) {
              m_selectedMailId = -1;
              m_mailBodyLoaded = false;
            }
          }
        } else {
          const float btnW = (avail - spacing) * 0.5f;
          if (Button(m_cachedMailRead.c_str(), TextStyle::DefaultButton(), ImVec2(btnW, 0))) {
            m_selectedMailId = info.id;
            m_selectedMailPending = pending;
            m_mailBodyLoaded =
                mail.GetMailBody(info.id, pending, m_mailBodyFrom, m_mailBodySubject, m_mailBodyText);
            if (!info.read) mail.MarkRead(info.id);
          }
          ImGui::SameLine();
          if (Button(m_cachedMailDelete.c_str(), TextStyle::DefaultButton(), ImVec2(btnW, 0))) {
            mail.DeleteMail(info.index, pending);
            if (isSelected) {
              m_selectedMailId = -1;
              m_mailBodyLoaded = false;
            }
          }
        }

        ImGui::PopID();
      }
    }
    clipper.End();
    ImGui::EndTable();
  }

  ImGui::PopID();
}
}  // namespace SPF::UI
