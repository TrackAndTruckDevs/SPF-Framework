#pragma once

#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "imgui.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace SPF::UI {
class ToolsWindow : public BaseWindow {
 public:
  ToolsWindow(const std::string& owner, const std::string& id, Events::EventManager& eventManager);

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  static int HistoryCallback(ImGuiInputTextCallbackData* data);

  void RenderConsoleSection();
  void RenderGameLogSection();
  void RenderSuggestions();
  void UpdateSuggestionMatches();
  void SyncCommandInputCursor(ImGuiID inputId);
  void RenderMailSection();
  void RenderMailList(bool pending);
  void RebuildMailGameParams();

  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;
  char m_commandBuffer[256] = {0};
  std::vector<std::string> m_history;
  int m_historyPos = -1;  // -1: new command, 0..history.size()-1: history index
  std::vector<std::string> m_suggestionMatches;
  int m_suggestionIndex = -1;
  bool m_focusCommandInput = false;

  std::string m_cachedInfoText;
  std::string m_cachedSendButton;
  std::string m_cachedDumpCommands;
  std::string m_cachedEnableCheats;
  std::string m_cachedGameLogLabel;
  std::string m_cachedCheckboxGameLogToFile;
  std::string m_cachedEnableHookText;
  std::string m_cachedHooksWindowTitle;
  std::string m_cachedTabConsoleLog;

  // Game log section state
  Logging::LogLevel m_filterLevel = Logging::LogLevel::Trace;
  std::string m_selectedComponent = "All";
  std::vector<std::string> m_componentList;
  bool m_autoScroll = true;
  int m_selectionStart = -1;
  int m_selectionEnd = -1;

  std::string m_cachedButtonClear;
  std::string m_cachedCheckboxAutoscroll;
  std::string m_cachedLabelLevel;
  std::string m_cachedLabelModule;
  std::string m_cachedContextCopyLine;
  std::string m_cachedContextCopyMessage;
  std::string m_cachedContextCopySelected;
  std::string m_cachedContextCopyAll;
  std::string m_cachedMsgCleanSession;

  // Mail section state
  char m_customMailOwnerBuf[128] = {0};
  char m_customMailTagBuf[128] = {0};
  char m_customMailFromBuf[128] = {0};
  char m_customMailSubjectBuf[128] = {0};
  char m_customMailBodyBuf[1024] = {0};
  // Top "mail type" selector: 0 = custom mail (API demo), 1 = game mailtext.
  int m_mailMode = 0;
  struct MailTextEntry {
    std::string name;  // mail_text unit name, e.g. "mailtext.td_visit"
    std::string from;    // raw template, $PLACEHOLDER$ tokens kept
    std::string subject;
    std::string body;
  };
  std::vector<MailTextEntry> m_mailTextCache;  // rebuilt when the combo opens
  bool m_mailComboWasOpen = false;
  std::string m_selectedMailText;  // selected mail_text name ("" = none)
  std::string m_paramsBuiltFor;    // mail_text name the param editors were built for
  std::vector<std::string> m_gameParamKeys;  // unique $KEY$ tokens of the template
  std::unordered_map<std::string, std::array<char, 128>> m_gameParamBufs;
  // Letter list / reader state
  int m_selectedMailId = -1;
  bool m_selectedMailPending = false;
  std::string m_mailBodyFrom;
  std::string m_mailBodySubject;
  std::string m_mailBodyText;
  bool m_mailBodyLoaded = false;
  std::string m_cachedTabMail;
  std::string m_cachedMailSendSection;
  std::string m_cachedMailModeLabel;
  std::string m_cachedMailModeCustom;
  std::string m_cachedMailGameSection;
  std::string m_cachedMailGameHint;
  std::string m_cachedMailGameTemplate;
  std::string m_cachedMailGameNoPlaceholders;
  std::string m_cachedMailGameEmptyList;
  std::string m_cachedMailGameSelectPrompt;
  std::string m_cachedMailCustomInvalid;
  std::string m_cachedMailSendButton;
  std::string m_cachedMailInbox;
  std::string m_cachedMailPending;
  std::string m_cachedMailColId;
  std::string m_cachedMailColName;
  std::string m_cachedMailColRead;
  std::string m_cachedMailColUnread;
  std::string m_cachedMailRead;
  std::string m_cachedMailDelete;
  std::string m_cachedMailFrom;
  std::string m_cachedMailSubject;
  std::string m_cachedMailBody;
  std::string m_cachedMailNoSelection;
  std::string m_cachedMailEmptyList;
  std::string m_cachedMailServiceNotReady;
  std::string m_cachedMailCreateSection;
  std::string m_cachedMailCreateHint;
  std::string m_cachedMailCreateButton;
  std::string m_cachedMailOwner;
  std::string m_cachedMailTag;
  std::string m_cachedMailOwnerTooltip;
  std::string m_cachedMailTagTooltip;
};
}  // namespace SPF::UI
