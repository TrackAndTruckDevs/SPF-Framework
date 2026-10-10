#pragma once

#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "imgui.h"

#include <string>
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

  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;
  char m_commandBuffer[256] = {0};
  std::vector<std::string> m_history;
  int m_historyPos = -1;  // -1: new command, 0..history.size()-1: history index

  std::string m_cachedInfoText;
  std::string m_cachedSendButton;
  std::string m_cachedDumpCommands;
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
};
}  // namespace SPF::UI
