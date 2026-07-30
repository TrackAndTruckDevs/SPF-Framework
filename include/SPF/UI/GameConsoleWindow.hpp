#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Hooks/HookManager.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include "imgui.h"

#include <string>
#include <vector>


SPF_NS_BEGIN

// Forward-declaration
namespace Events {
class EventManager;
}

namespace UI {
class GameConsoleWindow : public BaseWindow {
 public:
  GameConsoleWindow(const std::string& owner, const std::string& id, Events::EventManager& eventManager);

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  static int HistoryCallback(ImGuiInputTextCallbackData* data);

  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;
  char m_commandBuffer[256] = {0};
  std::vector<std::string> m_history;
  int m_historyPos = -1;  // -1: new command, 0..history.size()-1: history index

  std::string m_cachedInfoText;
  std::string m_cachedSendButton;
  std::string m_cachedEnableHookText;
  std::string m_cachedHooksWindowTitle;
};
}  // namespace UI

SPF_NS_END