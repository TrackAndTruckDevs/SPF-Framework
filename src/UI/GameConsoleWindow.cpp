#include "SPF/UI/GameConsoleWindow.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "imgui.h"

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Hooks;
using namespace SPF::Localization;

namespace {
constexpr int MAX_HISTORY_SIZE = 20;
} // namespace

GameConsoleWindow::GameConsoleWindow(const std::string& owner, const std::string& id, Events::EventManager& eventManager)
    : BaseWindow(owner, id), m_eventManager(eventManager), m_hookManager(HookManager::GetInstance()) {
  m_titleLocalizationKey = "game_console_window.title";
}

int GameConsoleWindow::HistoryCallback(ImGuiInputTextCallbackData* data) {
  GameConsoleWindow* self = (GameConsoleWindow*)data->UserData;
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

void GameConsoleWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();
  auto* hook = m_hookManager.GetHook("GameConsole");
  if (hook && hook->IsEnabled()) {
    ImGui::TextWrapped("%s", loc.Get("game_console_window.info_text").c_str());

    auto execute = [this]() {
      if (strlen(m_commandBuffer) > 0) {
        m_eventManager.System.OnRequestExecuteCommand.Call({m_commandBuffer});
        m_history.push_back(m_commandBuffer);
        if (m_history.size() > MAX_HISTORY_SIZE) {
            m_history.erase(m_history.begin());
        }
        m_commandBuffer[0] = '\0';
        m_historyPos = -1; // Reset history position after new command
      }
    };

    ImGui::PushItemWidth(-80.0f);  // Leave space for the button
    if (ImGui::InputText("##CommandInput", m_commandBuffer, sizeof(m_commandBuffer),
                         ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory,
                         &GameConsoleWindow::HistoryCallback, (void*)this)) {
      execute();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (ImGui::Button(loc.Get("game_console_window.send_button").c_str(), ImVec2(-1.0f, 0))) {
      execute();
    }
  } else {
    ImGui::Text(loc.Get("game_console_window.enable_hook_text").c_str(), loc.Get("hooks_window.title").c_str());
  }
}
}  // namespace UI
SPF_NS_END