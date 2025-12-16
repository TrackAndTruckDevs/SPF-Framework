#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Namespace.hpp"

#include <vector>
#include <string>

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

 private:
  static int HistoryCallback(ImGuiInputTextCallbackData* data);

  Events::EventManager& m_eventManager;
  Hooks::HookManager& m_hookManager;
  char m_commandBuffer[256] = {0};
  std::vector<std::string> m_history;
  int m_historyPos = -1; // -1: new command, 0..history.size()-1: history index
};
}  // namespace UI

SPF_NS_END