#include "SPF/UI/PluginsWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/UIEvents.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Modules/IInputService.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Windows.hpp"

#include "imgui.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <shellapi.h>
#include <string>
#include <vector>

SPF_NS_BEGIN

namespace UI {
using namespace SPF::Localization;

PluginsWindow::PluginsWindow(const std::string& componentName, const std::string& windowId, Config::IConfigService& configService, Events::EventManager& eventManager)
    : BaseWindow(componentName, windowId), m_configService(configService), m_eventManager(eventManager) {
  m_titleLocalizationKey = "plugins_window.title";
  RefreshLocalization();
}

void PluginsWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locTableStatus = loc.Get("plugins_window.table.status");
  m_locTableName = loc.Get("plugins_window.table.name");
  m_locTableActions = loc.Get("plugins_window.table.actions");
  m_locBtnEnable = loc.Get("plugins_window.buttons.enable");
  m_locBtnDisable = loc.Get("plugins_window.buttons.disable");
  m_locTooltipInfo = loc.Get("plugins_window.tooltips.info");
  m_locTooltipDesc = loc.Get("plugins_window.tooltips.description");
  m_locTooltipSettings = loc.Get("plugins_window.tooltips.settings");
  m_locInfoPopupAuthor = loc.Get("plugins_window.info_popup.author");
  m_locInfoPopupVersion = loc.Get("plugins_window.info_popup.version");
  m_locStatusIncompatible = loc.Get("plugins_window.status.incompatible");
  m_locVirtInputRestartRequired = loc.Get("plugins_window.status.virt_input_restart_required");
  m_locTooltipRestartSDK = loc.Get("plugins_window.tooltips.restart_sdk");
  m_locStatusUpdateAvailable = loc.Get("plugins_window.status.update_available");
  m_locTooltipUpdateAvailable = loc.Get("plugins_window.tooltips.update_available");
}

void PluginsWindow::RenderContent() {
  auto& pluginManager = Modules::PluginManager::GetInstance();
  auto& loc = LocalizationManager::GetInstance();

  if (ImGui::BeginTable("plugins_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
    ImGui::TableSetupColumn(m_locTableStatus.c_str(), ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn(m_locTableName.c_str(), ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn(m_locTableActions.c_str(), ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    for (const auto& [componentId, componentInfo] : m_configService.GetAllComponentInfo()) {
      // This window should only display plugins, not the framework itself.
      if (componentInfo.isFramework) continue;

      ImGui::TableNextRow();

      bool isLoaded = pluginManager.IsPluginLoaded(componentId);
      const bool isCompatible = !componentInfo.incompatibilityReason.has_value();

      // Make disabled plugins semi-transparent
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, isLoaded ? 1.0f : 0.6f);

      // --- Status Column ---
      ImGui::TableSetColumnIndex(0);
      const char* statusIcon = isLoaded ? ICON_FA_CHECK : ICON_FA_XMARK;
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

      // Calculate right-aligned status warnings
      bool showRestart = pluginManager.GetInputService() && pluginManager.GetInputService()->IsRestartRequiredForComponent(componentId);
      bool showIncompatible = !isCompatible;
      const auto* updateInfo = UIManager::GetInstance().GetPluginUpdate(componentId);
      bool showUpdate = (updateInfo != nullptr);

      if (showRestart || showIncompatible || showUpdate) {
        float totalRightWidth = 0.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float framePaddingX = ImGui::GetStyle().FramePadding.x;

        // Accumulate widths for active blocks
        std::vector<float> blockWidths;
        int activeBlocksCount = 0;

        float restartWidth = 0.0f;
        std::string warningText;
        if (showRestart) {
          warningText = m_locVirtInputRestartRequired;
          float warningIconWidth = Typography::CalcTextSize(ICON_FA_TRIANGLE_EXCLAMATION).x;
          float textWidth = Typography::CalcTextSize(warningText.c_str()).x;
          float reloadBtnWidth = Typography::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x + framePaddingX * 2.0f;
          restartWidth = warningIconWidth + spacing + textWidth + spacing + reloadBtnWidth;
          blockWidths.push_back(restartWidth);
          activeBlocksCount++;
        }

        float incompatibleWidth = 0.0f;
        std::string incompatibleText;
        if (showIncompatible) {
          incompatibleText = m_locStatusIncompatible + " " + componentInfo.incompatibilityReason.value_or("");
          incompatibleWidth = Typography::CalcTextSize(incompatibleText.c_str(), TextStyle::Bold()).x;
          blockWidths.push_back(incompatibleWidth);
          activeBlocksCount++;
        }

        float updateWidth = 0.0f;
        std::string updateText;
        if (showUpdate) {
          updateText = loc.GetFormatted("framework", m_locStatusUpdateAvailable, updateInfo->latestVersion);
          float updateTextWidth = Typography::CalcTextSize(updateText.c_str(), TextStyle::Bold()).x;
          float githubBtnWidth = Typography::CalcTextSize(ICON_FA_GITHUB).x + framePaddingX * 2.0f;
          updateWidth = updateTextWidth + spacing + githubBtnWidth;
          blockWidths.push_back(updateWidth);
          activeBlocksCount++;
        }

        // Sum up block widths and inter-block spacing (we use 15.0f spacing between different status items)
        float blockSpacing = 15.0f;
        for (float w : blockWidths) {
          totalRightWidth += w;
        }
        if (activeBlocksCount > 1) {
          totalRightWidth += blockSpacing * (activeBlocksCount - 1);
        }

        // Align cursor to the right
        ImGui::SameLine();
        float currentPosX = ImGui::GetCursorPosX();
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(currentPosX + availWidth - totalRightWidth - 10.0f);

        // Render blocks in order
        bool firstBlock = true;
        auto drawBlockSpacing = [&]() {
          if (!firstBlock) {
            ImGui::SameLine(0, blockSpacing);
          }
          firstBlock = false;
        };

        // 1. Restart required
        if (showRestart) {
          drawBlockSpacing();
          // Yellow Warning Icon
          Typography::Text(TextStyle::Regular().Color(Colors::YELLOW), ICON_FA_TRIANGLE_EXCLAMATION);
          // Localized Text
          ImGui::SameLine();
          Typography::Text(TextStyle::Regular().Color(Colors::WHITE), warningText.c_str());
          // Interactive Reload Icon
          ImGui::SameLine();
          if (Button(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::DefaultButton())) {
            m_eventManager.System.OnRequestExecuteCommand.Call({"sdk reinit"});
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", m_locTooltipRestartSDK.c_str());
          }
        }

        // 2. Incompatible warning
        if (showIncompatible) {
          drawBlockSpacing();
          Typography::Text(TextStyle::Bold().Color(Colors::RED), "%s", incompatibleText.c_str());
        }

        // 3. Update Available warning
        if (showUpdate) {
          drawBlockSpacing();
          // Orange text
          Typography::Text(TextStyle::Bold().Color(Colors::ORANGE), "%s", updateText.c_str());
          // Github Button
          ImGui::SameLine();
          if (Button(ICON_FA_GITHUB, TextStyle::DefaultButton().Color(Colors::WHITE).HoverColor(Colors::GOLD))) {
            ShellExecute(NULL, "open", updateInfo->downloadUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
          }
          if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", m_locTooltipUpdateAvailable.c_str());
          }
        }
      }

      // --- Actions Column ---
      ImGui::TableSetColumnIndex(2);
      ImGui::PushID(componentId.c_str());

      // Toggle button
      ImGui::BeginDisabled(!isCompatible);
      const char* toggleIcon = isLoaded ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF;
      const char* tooltipText = isLoaded ? m_locBtnDisable.c_str() : m_locBtnEnable.c_str();

      if (Button(toggleIcon, TextStyle::DefaultButton())) {
        m_eventManager.System.OnRequestPluginStateChange.Call({componentId, !isLoaded});
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tooltipText);
      }
      ImGui::EndDisabled();

      // Other buttons
      ImGui::SameLine();

      // --- "Info" button and modal window ---
      std::string infoTitle = m_locTooltipInfo + ": " + displayName;
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasInfo);
      if (Button(ICON_FA_CIRCLE_INFO)) {
        ImGui::OpenPopup(infoTitle.c_str());
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", m_locTooltipInfo.c_str());
      }

      if (ImGui::BeginPopupModal(infoTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
        ImGui::BeginChild("info_scroll_region", ImVec2(425, 90), false, ImGuiWindowFlags_HorizontalScrollbar);

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locInfoPopupAuthor.c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), " %s", componentInfo.author.value_or("").c_str());

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locInfoPopupVersion.c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), " %s", componentInfo.version.value_or("").c_str());

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Spacing();
        struct SocialLink {
          const char* icon;
          std::string url;
          const char* name;
          ImVec4 color;
        };
        std::vector<SocialLink> links;
        auto addLink = [&](const std::optional<std::string>& url, const char* icon, const char* name, ImVec4 color) {
          if (url && !url->empty()) links.push_back({icon, *url, name, color});
        };

        addLink(componentInfo.websiteUrl, ICON_FA_GLOBE, "Website", Colors::GOLD);
        addLink(componentInfo.email, ICON_FA_ENVELOPE, "Email", Colors::GRAY);
        addLink(componentInfo.discordUrl, ICON_FA_DISCORD, "Discord", ImVec4(0.35f, 0.40f, 0.95f, 1.0f));
        addLink(componentInfo.steamProfileUrl, ICON_FA_STEAM, "Steam", ImVec4(0.40f, 0.60f, 0.80f, 1.0f));
        addLink(componentInfo.githubUrl, ICON_FA_GITHUB, "GitHub", Colors::WHITE);
        addLink(componentInfo.youtubeUrl, ICON_FA_YOUTUBE, "YouTube", ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        addLink(componentInfo.scsForumUrl, ICON_FA_COMMENTS, "SCS Forum", ImVec4(0.0f, 0.70f, 0.90f, 1.0f));
        addLink(componentInfo.patreonUrl, ICON_FA_PATREON, "Patreon", ImVec4(0.98f, 0.41f, 0.33f, 1.0f));

        if (!links.empty()) {
          // Total width of the child window minus horizontal padding
          float contentWidth = 425.0f - ImGui::GetStyle().WindowPadding.x * 2.0f;
          float totalIconsWidth = 0;
          float buttonPaddingX = ImGui::GetStyle().FramePadding.x * 2.0f;

          for (const auto& link : links) {
            totalIconsWidth += ImGui::CalcTextSize(link.icon).x + buttonPaddingX;
          }

          // Calculate spacing to distribute icons evenly
          float spacing = (contentWidth - totalIconsWidth) / (float)(links.size() + 1);

          // Ensure spacing is not negative and has a minimum
          spacing = (std::max)(spacing, ImGui::GetStyle().ItemSpacing.x);

          // Initial offset from the left
          ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x + spacing);

          for (size_t i = 0; i < links.size(); ++i) {
            if (i > 0) ImGui::SameLine(0, spacing);

            if (Button(links[i].icon, TextStyle::DefaultButton().Color(links[i].color))) {
              ShellExecute(NULL, "open", links[i].url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }

            if (ImGui::IsItemHovered()) {
              ImGui::SetTooltip("%s", links[i].name);
            }
          }
        }

        ImGui::EndChild();
        ImGui::Separator();
        if (Button("OK", TextStyle::DefaultButton())) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      // --- "Description" button and modal window ---
      ImGui::SameLine();
      std::string descriptionTitle = m_locTooltipDesc + ": " + displayName;
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasDescription);
      if (Button(ICON_FA_FILE_LINES, TextStyle::DefaultButton())) {
        ImGui::OpenPopup(descriptionTitle.c_str());
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", m_locTooltipDesc.c_str());
      }

      if (ImGui::BeginPopupModal(descriptionTitle.c_str(), NULL, ImGuiWindowFlags_NoResize)) {
        ImGui::BeginChild("description_scroll_region", ImVec2(500, 250), false, ImGuiWindowFlags_HorizontalScrollbar);

        std::string description;
        if (componentInfo.descriptionLiteral.has_value()) {
          description = componentInfo.descriptionLiteral.value();
        } else if (componentInfo.descriptionKey.has_value()) {
          description = loc.Get(componentId, componentInfo.descriptionKey.value());
        }

        if (!description.empty()) {
          Typography::RenderMarkdownText(description, TextStyle::Regular().Wrapped(true));
        }

        ImGui::EndChild();
        ImGui::Separator();
        if (Button("OK", TextStyle::DefaultButton())) {
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }

      // --- "Settings" button ---
      ImGui::SameLine();
      ImGui::BeginDisabled(!isLoaded || !componentInfo.hasSettings);
      if (Button(ICON_FA_GEAR, TextStyle::DefaultButton())) {
        m_eventManager.System.OnFocusComponentInSettingsWindow.Call(Events::UI::FocusComponentInSettingsWindow{.componentName = componentId});
      }
      ImGui::EndDisabled();
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", m_locTooltipSettings.c_str());
      }

      ImGui::PopID();
      ImGui::PopStyleVar();  // End semi-transparency
    }

    ImGui::EndTable();
  }
}
}  // namespace UI

SPF_NS_END
