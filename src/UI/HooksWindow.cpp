#include "SPF/UI/HooksWindow.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Localization/LocalizationManager.hpp"

#include <imgui.h>
#include <cctype>  // for isxdigit

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Hooks;
using namespace SPF::Events;
using namespace SPF::Localization;

HooksWindow::HooksWindow(UIManager& uiManager, EventManager& eventManager, const std::string& componentName, const std::string& windowId)
    : BaseWindow(componentName, windowId), m_uiManager(uiManager), m_eventManager(eventManager), m_hookManager(HookManager::GetInstance()) {
  m_defaultTitle = "Hooks";
  m_titleLocalizationKey = "hooks_window.title";
}

const char* HooksWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_titleLocalizationKey).c_str(); }

void HooksWindow::RenderContent() {
  const auto& hooks = m_hookManager.GetFeatureHooks();

  if (hooks.empty()) {
    ImGui::TextUnformatted(LocalizationManager::GetInstance().Get("hooks_window.no_hooks_text").c_str());
    return;
  }

  ImFont* monoFont = m_uiManager.GetFont("monospace");

  for (IHook* hook : hooks) {
    if (ImGui::CollapsingHeader(hook->GetDisplayName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(LocalizationManager::GetInstance().Get("hooks_window.tooltip.internal_name").c_str());
        ImGui::Separator();
        ImGui::PushFont(monoFont);
        ImGui::TextUnformatted(hook->GetName().c_str());
        ImGui::PopFont();
        ImGui::EndTooltip();
      }
      ImGui::PushID(hook->GetName().c_str());

      // --- Enabled Checkbox ---
      bool isRequired = m_hookManager.IsHookRequired(hook->GetName());
      bool isEnabled = hook->IsEnabled();

      if (isRequired) {
        ImGui::BeginDisabled(true);
        bool forcedEnabled = true;
        ImGui::Checkbox(LocalizationManager::GetInstance().Get("hooks_window.enabled_checkbox").c_str(), &forcedEnabled);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", LocalizationManager::GetInstance().Get("hooks_window.required_by_plugin_text").c_str());
      } else {
        if (ImGui::Checkbox(LocalizationManager::GetInstance().Get("hooks_window.enabled_checkbox").c_str(), &isEnabled)) {
          m_eventManager.System.OnRequestSettingChange.Call({"framework", "settings.hook_states." + hook->GetName() + ".enabled", isEnabled});
        }
      }

      // --- Signature Info Tooltip ---
      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushFont(monoFont);
        ImGui::TextUnformatted(hook->GetSignature().c_str());
        ImGui::PopFont();
        ImGui::EndTooltip();
      }

      ImGui::PopID();
    }
  }
}
}  // namespace UI
SPF_NS_END