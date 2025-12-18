#include "SPF/UI/PluginsWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"

#include "imgui.h"
#include <string>

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;

PluginsWindow::PluginsWindow(Config::IConfigService& configService, Events::EventManager& eventManager, const std::string& componentName, const std::string& windowId)
    : BaseWindow(componentName, windowId), m_configService(configService), m_eventManager(eventManager) {
  m_locTitle = "plugins_window.title";
  m_locTableStatus = "plugins_window.table.status";
  m_locTableName = "plugins_window.table.name";
  m_locTableActions = "plugins_window.table.actions";
  m_locBtnEnable = "plugins_window.buttons.enable";
  m_locBtnDisable = "plugins_window.buttons.disable";
  m_locTooltipInfo = "plugins_window.tooltips.info";
  m_locTooltipDesc = "plugins_window.tooltips.description";
  m_locTooltipSettings = "plugins_window.tooltips.settings";
  m_locInfoPopupAuthor = "plugins_window.info_popup.author";
  m_locInfoPopupVersion = "plugins_window.info_popup.version";
  m_locStatusIncompatible = "plugins_window.status.incompatible";
}

const char* PluginsWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_locTitle).c_str(); }

void PluginsWindow::RenderContent() {
  auto& pluginManager = Modules::PluginManager::GetInstance();
  auto& loc = LocalizationManager::GetInstance();

  if (ImGui::BeginTable("plugins_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
    ImGui::TableSetupColumn(loc.Get(m_locTableStatus).c_str(), ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn(loc.Get(m_locTableName).c_str(), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(loc.Get(m_locTableActions).c_str(), ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    for (const auto& [componentId, componentInfo] : m_configService.GetAllComponentInfo()) {
      // This window should only display plugins, not the framework itself.
      if (componentInfo.isFramework) continue;

      ImGui::TableNextRow();

      bool isLoaded = pluginManager.IsPluginLoaded(componentId);

      // TODO: This is a placeholder for future game version compatibility check.
      // This logic will be replaced when the framework gains the ability to
      // query the current game version and compare it with the version
      // specified in the plugin's manifest.
      // The check should look something like this:
      // bool isCompatible = pluginManager.IsPluginCompatible(componentId);
      const bool isCompatible = true;  // Set to 'false' to demonstrate UI for incompatible plugin.

      // Make disabled plugins semi-transparent
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, isLoaded ? 1.0f : 0.6f);

      // --- Status Column ---
      ImGui::TableSetColumnIndex(0);
      const char* statusIcon = isLoaded ? ICON_FA_CHECK : ICON_FA_TIMES;
      float iconWidth = ImGui::CalcTextSize(statusIcon).x;
      float columnWidth = ImGui::GetColumnWidth(0);

      // Vertical centering logic
      const float rowHeight = ImGui::GetFrameHeight();
      const float iconHeight = ImGui::CalcTextSize(statusIcon).y;
      float yOffset = (rowHeight - iconHeight) * 0.5f;
      if (yOffset > 0) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
      }

      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (columnWidth - iconWidth) / 2.0f);

      if (isLoaded) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        ImGui::TextUnformatted(statusIcon);
        ImGui::PopStyleColor();
      } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        ImGui::TextUnformatted(statusIcon);
        ImGui::PopStyleColor();
      }

      // --- Name Column ---
      ImGui::TableSetColumnIndex(1);

      // Vertical centering logic
      const float textHeight = ImGui::GetTextLineHeight();
      yOffset = (rowHeight - textHeight) * 0.5f;
      if (yOffset > 0) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
      }

      const std::string displayName = componentInfo.name.value_or(componentId);
      ImGui::TextUnformatted(displayName.c_str());
      if (!isCompatible) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        // TODO: Here is a version with componentInfo, for now we are using a stub.
        ImGui::Text(loc.Get(m_locStatusIncompatible).c_str(), "1.2.3");
        ImGui::PopStyleColor();
      }

      // --- Actions Column ---
      ImGui::TableSetColumnIndex(2);
      ImGui::PushID(componentId.c_str());

      // Toggle button
      ImGui::BeginDisabled(!isCompatible);
      const char* toggleIcon = isLoaded ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF;
      const char* tooltipText = isLoaded ? loc.Get(m_locBtnDisable).c_str() : loc.Get(m_locBtnEnable).c_str();

      if (ImGui::Button(toggleIcon)) {
        m_eventManager.System.OnRequestPluginStateChange.Call({componentId, !isLoaded});
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltipText);
      }
      ImGui::EndDisabled();

      // Other buttons
      ImGui::SameLine();

      // --- "Info" button and modal window ---
      std::string infoTitle = loc.Get(m_locTooltipInfo) + ": " + displayName;
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasInfo);
      if (ImGui::Button(ICON_FA_INFO_CIRCLE)) {
        ImGui::OpenPopup(infoTitle.c_str());
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", loc.Get(m_locTooltipInfo).c_str());
      }

      if (ImGui::BeginPopupModal(infoTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
        ImGui::BeginChild("info_scroll_region", ImVec2(425, 75), false, ImGuiWindowFlags_HorizontalScrollbar);

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locInfoPopupAuthor).c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), " %s", componentInfo.author.value_or("").c_str());

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locInfoPopupVersion).c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), " %s", componentInfo.version.value_or("").c_str());

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Spacing();
        auto renderSocialLink = [](const std::optional<std::string>& url, const char* icon, const char* name) {
          if (url && !url->empty()) {
            ImGui::SameLine();
            std::string markdown = std::string("[") + icon + "](" + *url + ")";
            Typography::RenderMarkdownText(markdown, TextStyle::Regular());
            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", name);
            }
          }
        };
        renderSocialLink(componentInfo.websiteUrl, ICON_FA_GLOBE, "Website");
        renderSocialLink(componentInfo.email, ICON_FA_ENVELOPE, "Email");
        renderSocialLink(componentInfo.discordUrl, ICON_FA_DISCORD, "Discord");
        renderSocialLink(componentInfo.steamProfileUrl, ICON_FA_STEAM, "Steam");
        renderSocialLink(componentInfo.githubUrl, ICON_FA_GITHUB, "GitHub");
        renderSocialLink(componentInfo.youtubeUrl, ICON_FA_YOUTUBE, "YouTube");
        renderSocialLink(componentInfo.scsForumUrl, ICON_FA_COMMENTS, "SCS Forum");
        renderSocialLink(componentInfo.patreonUrl, ICON_FA_PATREON, "Patreon");

        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("OK")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      // --- "Description" button and modal window ---
      ImGui::SameLine();
      std::string descriptionTitle = loc.Get(m_locTooltipDesc) + ": " + displayName;
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasDescription);
      if (ImGui::Button(ICON_FA_FILE_ALT)) {
        ImGui::OpenPopup(descriptionTitle.c_str());
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", loc.Get(m_locTooltipDesc).c_str());
      }

      if (ImGui::BeginPopupModal(descriptionTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
        ImGui::BeginChild("description_scroll_region", ImVec2(400, 200), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (componentInfo.descriptionLiteral.has_value()) {
          ImGui::TextWrapped("%s", componentInfo.descriptionLiteral.value().c_str());
        } else if (componentInfo.descriptionKey.has_value()) {
          ImGui::TextWrapped("%s", loc.Get(componentId, componentInfo.descriptionKey.value()).c_str());
        }
        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("OK")) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      // --- "Settings" button ---
      ImGui::SameLine();
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasSettings);
      if (ImGui::Button(ICON_FA_COG)) {
        m_eventManager.System.OnFocusComponentInSettingsWindow.Call(Events::UI::FocusComponentInSettingsWindow{.componentName = componentId});
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", loc.Get(m_locTooltipSettings).c_str());
      }

      ImGui::PopID();
      ImGui::PopStyleVar();  // End semi-transparency
    }

    ImGui::EndTable();
  }
}
}  // namespace UI

SPF_NS_END
