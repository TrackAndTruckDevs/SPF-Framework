#include "SPF/UI/MainWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Modules/PerformanceMonitor.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/System/ApiService.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Windows.hpp"

#include "fmt/base.h"
#include "imgui.h"
#include "imgui_internal.h"

#include <cstddef>
#include <filesystem>
#include <fmt/format.h>
#include <map>
#include <shellapi.h>
#include <string>
#include <vector>

// Anonymous namespace for file-local definitions
namespace {
// Patrons will be fetched from API
}  // end anonymous namespace

SPF_NS_BEGIN
namespace UI {
using namespace SPF::Input;
using namespace SPF::Localization;
using namespace SPF::Modules;
using namespace SPF::UI;
using namespace SPF::Logging;
using namespace SPF::System;
using namespace SPF::Rendering;

MainWindow::MainWindow(Events::EventManager& eventManager, Input::InputManager& inputManager, Modules::KeyBindsManager& keyBindsManager, Config::IConfigService& configService, ITelemetryService& telemetryService)
    : BaseWindow("framework", "main_window"),
      m_eventManager(eventManager),
      m_inputManager(inputManager),
      m_keyBindsManager(keyBindsManager),
      m_configService(configService),
      m_hookManager(Hooks::HookManager::GetInstance()),
      m_telemetryService(telemetryService),
      // --- Framework Info ---
      m_frameworkName(m_configService.GetValue("framework", "info.name", "").get<std::string>()),
      m_frameworkVersion(m_configService.GetValue("framework", "info.version", "").get<std::string>()),
      m_frameworkAuthor(m_configService.GetValue("framework", "info.author", "").get<std::string>()),
      m_descriptionKey(m_configService.GetValue("framework", "info.description_key", "").get<std::string>()),
      // --- Contact URLs ---
      m_emailUrl(m_configService.GetValue("framework", "info.email", "").get<std::string>()),
      m_discordUrl(m_configService.GetValue("framework", "info.discordUrl", "").get<std::string>()),
      m_youtubeUrl(m_configService.GetValue("framework", "info.youtubeUrl", "").get<std::string>()),
      m_githubUrl(m_configService.GetValue("framework", "info.githubUrl", "").get<std::string>()),
      m_patreonUrl(m_configService.GetValue("framework", "info.patreonUrl", "").get<std::string>()),
      m_scsForumUrl(m_configService.GetValue("framework", "info.scsForumUrl", "").get<std::string>()),
      m_steamProfileUrl(m_configService.GetValue("framework", "info.steamProfileUrl", "").get<std::string>()),
      m_licenseUrl(m_githubUrl + "/blob/main/LICENSE") {
  m_isDeveloperMode = m_configService.GetValue("framework", "settings.framework.developer_mode", false).get<bool>();
  m_keyBindsManager.RegisterAction("framework.ui.main_window.toggle", [this]() { ToggleVisibility(); });
  m_titleLocalizationKey = "main_window.title";
  RefreshLocalization();
}

void MainWindow::ToggleVisibility() { SetVisibility(!IsVisible()); }

void MainWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locPatronsButtonTooltip = loc.Get("main_window.patrons_button_tooltip");
  m_locPatronsTitle = loc.Get("patrons_popup.title");
  m_locPatronsIntro = loc.Get("patrons_popup.intro_text");
  m_locPatronsLinkIntro = loc.Get("patrons_popup.link_intro");
  m_locPatronsLinkText = loc.Get("patrons_popup.link_text");
  m_locPatronsLinkTooltip = loc.Get("patrons_popup.link_tooltip");
  m_locPatronsHofTitle = loc.Get("patrons_popup.hof_title");
  m_locPatronsHofEmpty = loc.Get("patrons_popup.hof_empty");
  m_locPatronsHofTeaser = loc.Get("patrons_popup.hof_teaser");
  m_locPatronsCloseButton = loc.Get("patrons_popup.close_button");
  m_locTierMagnateHeader = loc.Get("patrons_popup.tiers.magnate");
  m_locTierManagerHeader = loc.Get("patrons_popup.tiers.manager");
  m_locTierMasterHeader = loc.Get("patrons_popup.tiers.master");
  m_locTierHaulerHeader = loc.Get("patrons_popup.tiers.hauler");
  m_locTierDriverHeader = loc.Get("patrons_popup.tiers.driver");
  m_locUpdateButtonTooltip = loc.Get("main_window.update_button_tooltip");
  m_locVersionLabel = loc.Get("main_window.version_label");
  m_locUpdateChecking = loc.Get("main_window.update_checking");
  m_locConnectDisabled = loc.Get("main_window.connect_disabled_tooltip");
  m_locUpdatePopupTitle = loc.Get("update_popup.title");
  m_locUpdateNoUpdate = loc.Get("update_popup.no_update");
  m_locUpdateAvailable = loc.Get("update_popup.update_available");
  m_locUpdateSwitchToRelease = loc.Get("update_popup.switch_to_release");
  m_locUpdateDownloadLink = loc.Get("update_popup.download_link");
  m_locUpdateDownloadTooltip = loc.Get("update_popup.download_tooltip");
  m_locUpdateDevNoteIntro = loc.Get("update_popup.developers_note_intro");
  m_locUpdateDevNoteLink = loc.Get("update_popup.developers_note_link");
  m_locUpdateGithubTooltip = loc.Get("update_popup.github_tooltip");
  m_locUpdateErrorNoInternet = loc.Get("api.error.no_internet");
  m_locUpdateErrorServerUnavailable = loc.Get("api.error.server_unavailable");
  m_locUpdateErrorGeneric = loc.Get("api.error.generic");
  m_locUpdateCloseButton = loc.Get("update_popup.close_button");
  m_locForDevelopers = loc.Get("common.for_developers");
  m_locForUsers = loc.Get("common.for_users");
  m_locMenuManual = loc.Get("main_window.menu.manual");
  m_locMenuAbout = loc.Get("main_window.menu.about");
  m_locMenuLegal = loc.Get("main_window.menu.legal");
  m_locMenuReload = loc.Get("main_window.menu.reload");
  m_locMenuReloadDisabledTooltip = loc.Get("main_window.menu.reload_disabled_tooltip");
  m_locMenuShutdown = loc.Get("main_window.menu.shutdown");
  m_locMenuOpenPluginsFolder = loc.Get("main_window.menu.open_plugins_folder");
  m_locManualPopupTitle = loc.Get("manual_popup.title");
  m_locAboutFrameworkTitle = loc.Get("about_popup.framework_title");
  m_locAboutPopupTitle = loc.Get("about_popup.title");
  m_locAboutUsTitle = loc.Get("about_popup.about_us_title");
  m_locAboutUsText = loc.Get("about_popup.about_us_text");
  m_locContactsTitle = loc.Get("about_popup.contacts_title");
  m_locEmailText = loc.Get("about_popup.email_text");
  m_locDiscordText = loc.Get("about_popup.discord_text");
  m_locYoutubeText = loc.Get("about_popup.youtube_text");
  m_locGithubText = loc.Get("about_popup.github_text");
  m_locPatreonText = loc.Get("about_popup.patreon_text");
  m_locScsForumText = loc.Get("about_popup.scs_forum_text");
  m_locSteamProfileText = loc.Get("about_popup.steam_profile_text");
  m_locShutdownPopupTitle = loc.Get("shutdown_popup.title");
  m_locShutdownPopupContent = loc.Get("shutdown_popup.content");
  m_locShutdownPopupConfirm = loc.Get("shutdown_popup.confirm_button");
  m_locShutdownPopupCancel = loc.Get("shutdown_popup.cancel_button");
  m_locLegalPopupTitle = loc.Get("legal_popup.title");
  m_locLegalLicenseTitle = loc.Get("legal_popup.license_title");
  m_locLegalLicenseText = loc.Get("legal_popup.license_text");
  m_locLegalDisclaimerTitle = loc.Get("legal_popup.disclaimer_title");
  m_locLegalDisclaimerText = loc.Get("legal_popup.disclaimer_text");
  m_locLegalFairPlayTitle = loc.Get("legal_popup.fair_play_title");
  m_locLegalFairPlayText = loc.Get("legal_popup.fair_play_text");
  m_locLegalContactTitle = loc.Get("legal_popup.contact_title");
  m_locLegalContactText = loc.Get("legal_popup.contact_text");
  m_locFaqQ1 = loc.Get("manual_popup.q1_question");
  m_locFaqA1 = loc.Get("manual_popup.q1_answer");
  m_locFaqQ2 = loc.Get("manual_popup.q2_question");
  m_locFaqA2 = loc.Get("manual_popup.q2_answer");
  m_locFaqQ3 = loc.Get("manual_popup.q3_question");
  m_locFaqA3 = loc.Get("manual_popup.q3_answer");
  m_locFaqQ4 = loc.Get("manual_popup.q4_question");
  m_locFaqA4 = loc.Get("manual_popup.q4_answer");
  m_locFaqQ5 = loc.Get("manual_popup.q5_question");
  m_locFaqA5 = loc.Get("manual_popup.q5_answer");
  m_locFaqQ6 = loc.Get("manual_popup.q6_question");
  m_locFaqA6 = loc.Get("manual_popup.q6_answer");
  m_locDeveloperMode = loc.Get("main_window.common.developer_mode");
  m_locUserMode = loc.Get("main_window.common.user_mode");
  m_locGameStatusRunningGame = loc.Get("main_window.game_status.running_game_label");
  m_locGameStatusCurrentVersion = loc.Get("main_window.game_status.current_version_label");
  m_locPerfFpsAvg = loc.Get("main_window.performance.fps_avg_label");
  m_locPerfFpsRollMinMax = loc.Get("main_window.performance.fps_roll_minmax_label");
  m_locPerfFpsGblMinMax = loc.Get("main_window.performance.fps_gbl_minmax_label");
  m_locPerfGraphicsApiLabel = loc.Get("main_window.performance.graphics_api_label");
  m_locPluginsLoadedActivatedLabel = loc.Get("main_window.summary.plugins_loaded_activated_label");
  m_locHooksLoadedActivatedLabel = loc.Get("main_window.summary.hooks_loaded_activated_label");
  m_locTooltipFpsAvg = loc.Get("main_window.performance.tooltip_fps_avg");
  m_locTooltipFpsRollMinMax = loc.Get("main_window.performance.tooltip_fps_roll_minmax");
  m_locTooltipFpsGblMinMax = loc.Get("main_window.performance.tooltip_fps_gbl_minmax");
}

namespace {
// Helper function for rendering a styled menu item.
// Returns true if the item is clicked.
bool RenderStyledMenuItem(const char* icon, const std::string& label, const std::string& tooltip) {
  ImGuiStyle& style = ImGui::GetStyle();
  // Add some vertical spacing to make it look less cramped.
  const float vertical_padding = style.ItemSpacing.y * 0.15f;       // Adjusted padding
  const std::string content = fmt::format(" {}  {}", icon, label);  // This is the text content

  ImGui::Dummy(ImVec2(0.0f, vertical_padding));

  // Use Selectable for full control over rendering and interaction.
  // We make it span the full width of the popup. It's invisible but captures clicks.
  // We also make the default hover background transparent to only show our custom text highlight.
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  bool clicked = ImGui::Selectable(fmt::format("##{}", label).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
  ImGui::PopStyleColor(2);

  // IMPORTANT: Check for hover state *immediately after* the item is drawn.
  const bool isHovered = ImGui::IsItemHovered();

  // Go back to the start of the line to draw the text on top of the selectable.
  // We need to adjust cursor position to correctly overlay the text on the selectable's area.
  ImGui::SameLine();
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.FramePadding.y);  // Adjust for default text padding that Typography::Text will use

  // Now, draw our custom content using Typography::Text. This ensures the layout engine registers the width.
  ImVec4 textColor = isHovered ? Colors::GOLD : Colors::WHITE;  // Direct Colors usage
  Typography::Text(TextStyle::Regular().Color(textColor), content.c_str());

  // Add tooltip on hover.
  if (isHovered) {
    ImGui::SetTooltip("%s", tooltip.c_str());
  }

  ImGui::Dummy(ImVec2(0.0f, vertical_padding));  // Final dummy for spacing

  return clicked;
}
}  // anonymous namespace

void MainWindow::RenderContent() {
  // --- Framework Header ---
  if (!m_frameworkName.empty()) {
    // --- 1. Title (Name and Version) ---
    std::string title = m_frameworkName + " v" + m_frameworkVersion;
    Typography::Text(TextStyle::H1().Align(TextAlign::Center).Color(Colors::GOLD), "%s", title.c_str());
    ImGui::SameLine();

    // --- Top-right corner buttons ---
    {
      // Calculate widths of each button individually for accuracy
      const float button1_w = Typography::CalcTextSize(ICON_FA_HAND_HOLDING_HEART).x + ImGui::GetStyle().FramePadding.x * 2.0f;
      const float button2_w = Typography::CalcTextSize(ICON_FA_ARROWS_ROTATE).x + ImGui::GetStyle().FramePadding.x * 2.0f;
      const float button3_w = Typography::CalcTextSize(ICON_FA_SCALE_BALANCED).x + ImGui::GetStyle().FramePadding.x * 2.0f;

      // Total width of all buttons plus the spacing between them
      const float total_buttons_width = button1_w + button2_w + button3_w + (ImGui::GetStyle().ItemSpacing.x * 2);

      // Desired padding from the right edge, matching the top padding for consistency
      const float right_padding = ImGui::GetStyle().WindowPadding.y;

      // Use SameLine to position the button block from the right edge of the window
      ImGui::SameLine(ImGui::GetWindowWidth() - total_buttons_width - right_padding);

      // Fetch the current connectivity setting
      bool isConnectEnabled = m_configService.GetValue("framework", "settings.framework.connect", true).get<bool>();

      // Patrons Button
      {
        TextStyle patronsButtonStyle = TextStyle::DefaultButton();
        if (!isConnectEnabled) patronsButtonStyle.Color(Colors::GRAY).HoverColor(Colors::GRAY).ActiveColor(Colors::GRAY);

        if (Button(ICON_FA_HAND_HOLDING_HEART, patronsButtonStyle)) {
          if (isConnectEnabled) {
            m_isPatronsPopupOpen = true;
            m_eventManager.System.OnRequestPatronsFetch.Call({});
          }
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", (isConnectEnabled ? m_locPatronsButtonTooltip : m_locConnectDisabled).c_str());
        }
      }

      ImGui::SameLine();

      // Update Button
      {
        TextStyle updateButtonStyle = TextStyle::DefaultButton();
        if (isConnectEnabled) {
          switch (m_currentUpdateStatus) {
            case Modules::CommunicationManager::UpdateStatus::PatchAvailable:
              updateButtonStyle.Color(Colors::YELLOW);
              break;
            case Modules::CommunicationManager::UpdateStatus::MinorAvailable:
              updateButtonStyle.Color(Colors::ORANGE);
              break;
            case Modules::CommunicationManager::UpdateStatus::MajorAvailable:
              updateButtonStyle.Color(Colors::RED);
              break;
            default:
              updateButtonStyle.Color(Colors::WHITE);
              break;
          }
        } else {
          updateButtonStyle.Color(Colors::GRAY).HoverColor(Colors::GRAY).ActiveColor(Colors::GRAY);
        }

        if (Button(ICON_FA_ARROWS_ROTATE, updateButtonStyle)) {
          if (isConnectEnabled) {
            m_isUpdatePopupOpen = true;
            if (!m_frameworkVersion.empty()) {
              m_eventManager.System.OnRequestUpdateCheck.Call({});
            } else {
              auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MainWindow");
              logger->Warn("Cannot perform update check: Framework version is not specified in the manifest.");
            }
          }
        }
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", (isConnectEnabled ? m_locUpdateButtonTooltip : m_locConnectDisabled).c_str());
        }
      }

      ImGui::SameLine();

      // Hamburger Menu Button
      if (Button(ICON_FA_BARS, TextStyle::DefaultButton())) {
        ImGui::OpenPopup("HamburgerMenu");
      }
    }

    // --- Mode Toggle Row (Centered on new line) ---
    {
      const float toggle_switch_width = 46.0f;
      const float toggle_switch_height = 20.0f;
      const float toggle_radius = toggle_switch_height * 0.5f;

      ImVec2 userLabelSize = Typography::CalcTextSize(m_locUserMode.c_str(), TextStyle::Bold());
      ImVec2 devLabelSize = Typography::CalcTextSize(m_locDeveloperMode.c_str(), TextStyle::Bold());
      const float toggle_group_width = userLabelSize.x + toggle_switch_width + devLabelSize.x + ImGui::GetStyle().ItemSpacing.x * 4.0f;

      ImGui::Spacing();
      ImGui::SetCursorPosX((ImGui::GetWindowWidth() - toggle_group_width) * 0.5f);

      ImGui::BeginGroup();
      {
        // User Mode Label
        Typography::Text(TextStyle::Bold().Color(m_isDeveloperMode ? Colors::WHITE : Colors::GOLD), "%s", m_locUserMode.c_str());
        ImGui::SameLine();

        // Switch
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImGui::InvisibleButton("##mode_toggle", ImVec2(toggle_switch_width, toggle_switch_height));
        const bool is_hovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
          m_isDeveloperMode = !m_isDeveloperMode;
          m_configService.SetValue("framework", "settings.framework.developer_mode", m_isDeveloperMode);
          m_configService.SaveAllDirty();
          UIManager::GetInstance().ApplyDeveloperMode(m_isDeveloperMode);
        }

        // Background
        ImVec4 bg_col = is_hovered ? ImVec4(0.25f, 0.25f, 0.25f, 1.0f) : ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        draw_list->AddRectFilled(p, ImVec2(p.x + toggle_switch_width, p.y + toggle_switch_height), ImGui::GetColorU32(bg_col), toggle_radius);

        // Outline
        ImVec4 outline_col = Colors::WHITE;
        outline_col.w = is_hovered ? 0.3f : 0.15f;
        draw_list->AddRect(p, ImVec2(p.x + toggle_switch_width, p.y + toggle_switch_height), ImGui::GetColorU32(outline_col), toggle_radius);

        // Oval Knob
        float knob_w = toggle_switch_width * 0.45f;
        float knob_h = toggle_switch_height - 4.0f;
        float knob_x = m_isDeveloperMode ? (p.x + toggle_switch_width - knob_w - 2.0f) : (p.x + 2.0f);
        float knob_y = p.y + 2.0f;

        ImVec4 knob_color = m_isDeveloperMode ? Colors::ORANGE : Colors::BLUE;
        draw_list->AddRectFilled(ImVec2(knob_x, knob_y), ImVec2(knob_x + knob_w, knob_y + knob_h), ImGui::GetColorU32(knob_color), toggle_radius);

        ImGui::SameLine();
        // Developer Mode Label
        Typography::Text(TextStyle::Bold().Color(m_isDeveloperMode ? Colors::GOLD : Colors::WHITE), "%s", m_locDeveloperMode.c_str());
      }
      ImGui::EndGroup();
    }

    // --- Game Info ---
    ImGui::Spacing();
    ImGui::Spacing();

    const auto& game = System::EnvironmentManager::GetInstance().GetGameInfo();
    if (!game.name.empty()) {
      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameStatusRunningGame.c_str());
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", game.name.c_str());
      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameStatusCurrentVersion.c_str());
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", game.version.c_str());
    }

    // --- Performance Stats ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto& perf = PerformanceMonitor::GetInstance();
    const auto& status = System::EnvironmentManager::GetInstance().GetStatus();

    if (!status.renderer.empty()) {
      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locPerfGraphicsApiLabel.c_str());
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", status.renderer.c_str());
      ImGui::SameLine();
      ImGui::Dummy(ImVec2(10.0f, 0.0f));
      ImGui::SameLine();
    }

    if (perf.GetDeltaTime() > 0.0f) {
      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locPerfFpsAvg.c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locTooltipFpsAvg.c_str());
      }
      ImGui::SameLine();

      const float fpsAvg = perf.GetRollingAvgFPS();
      ImVec4 fpsColor = Colors::WHITE;
      if (fpsAvg < 30.0f) {
        fpsColor = Colors::RED;
      } else if (fpsAvg < 60.0f) {
        fpsColor = Colors::YELLOW;
      }
      Typography::Text(TextStyle::Bold().Color(fpsColor), "%.0f", fpsAvg);
      ImGui::SameLine();

      ImGui::Dummy(ImVec2(10.0f, 0.0f));
      ImGui::SameLine();

      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locPerfFpsRollMinMax.c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locTooltipFpsRollMinMax.c_str());
      }
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%.0f/%.0f", perf.GetRollingMinFPS(), perf.GetRollingMaxFPS());
      ImGui::SameLine();

      ImGui::Dummy(ImVec2(10.0f, 0.0f));
      ImGui::SameLine();

      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locPerfFpsGblMinMax.c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locTooltipFpsGblMinMax.c_str());
      }
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%.0f/%.0f", perf.GetGlobalMinFPS(), perf.GetGlobalMaxFPS());
    }
    ImGui::Spacing();

    // --- Plugin Stats ---
    ImGui::Spacing();
    {
      auto& pluginManager = Modules::PluginManager::GetInstance();
      const auto& allComponents = m_configService.GetAllComponentInfo();

      int totalPlugins = 0;
      int enabledPlugins = 0;

      for (const auto& [id, info] : allComponents) {
        if (info.isFramework) continue;
        totalPlugins++;
        if (pluginManager.IsPluginLoaded(id)) {
          enabledPlugins++;
        }
      }

      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locPluginsLoadedActivatedLabel.c_str());
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%d/%d", totalPlugins, enabledPlugins);

      // --- Hook Stats ---
      ImGui::SameLine();

      ImGui::SameLine();
      auto& hookManager = Hooks::HookManager::GetInstance();
      const auto& hooks = hookManager.GetFeatureHooks();

      int totalHooks = hooks.size();
      int enabledHooks = 0;
      for (const auto* hook : hooks) {
        if (hook->IsEnabled()) {
          enabledHooks++;
        }
      }

      Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locHooksLoadedActivatedLabel.c_str());
      ImGui::SameLine();
      Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%d/%d", totalHooks, enabledHooks);
    }

    // --- 4. Separator ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  // --- Render all popups --- //
  // Each function manages the state and rendering of a specific popup.
  RenderPatronsPopup();
  RenderUpdatePopup();
  RenderHamburgerMenu();
  RenderManualPopup();
  RenderAboutPopup();
  RenderLegalPopup();
  RenderShutdownPopup();

  // --- Dockspace ---
  // Make the separator and overline below the tab bar transparent
  ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

  m_dockspaceId = ImGui::GetID("MainDockSpace");
  ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingSplit | ImGuiDockNodeFlags_NoDockingOverCentralNode | ImGuiDockNodeFlags_NoUndocking;
  ImGui::DockSpace(m_dockspaceId, ImVec2(0.0f, 0.0f), dockspace_flags);

  // Get the dock node by its ID to change its behavior
  if (ImGuiDockNode* node = ImGui::DockBuilderGetNode(m_dockspaceId)) {
    // Add a flag that removes the dropdown button from the tab bar
    node->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
  }

  ImGui::PopStyleColor();
}

ImGuiID MainWindow::GetMainDockspaceID() const { return m_dockspaceId; }

ImGuiWindowFlags MainWindow::GetExtraWindowFlags() const { return ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse; }

void MainWindow::OnUpdateCheckCompleted(const Events::System::OnUpdateCheckCompleted& e) {
  if (e.result.success && e.result.data.has_value()) {
    auto logger = LoggerFactory::GetInstance().GetLogger("MainWindow");
    logger->Debug("Update check succeeded.");

    m_lastUpdateInfo = e.result.data.value();
    m_lastUpdateError.reset();

    if (m_lastUpdateInfo->updateAvailable) {
      // For now, determining status based on version presence
      m_currentUpdateStatus = Modules::CommunicationManager::UpdateStatus::PatchAvailable;  // Default

      auto currentVerOpt = System::Version::FromString(m_frameworkVersion);
      if (currentVerOpt) {
        if (m_lastUpdateInfo->latestVersion.ver.major > currentVerOpt->major)
          m_currentUpdateStatus = Modules::CommunicationManager::UpdateStatus::MajorAvailable;
        else if (m_lastUpdateInfo->latestVersion.ver.minor > currentVerOpt->minor)
          m_currentUpdateStatus = Modules::CommunicationManager::UpdateStatus::MinorAvailable;
      }
    } else {
      m_currentUpdateStatus = Modules::CommunicationManager::UpdateStatus::UpToDate;
    }
  } else {
    m_lastUpdateInfo.reset();
    m_lastUpdateError = e.result.errorMessage.has_value() ? e.result.errorMessage : std::string("api.error.generic");
    m_currentUpdateStatus = Modules::CommunicationManager::UpdateStatus::Unknown;
  }
}

void MainWindow::OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) {
  if (e.result.success && e.result.data.has_value()) {
    auto logger = LoggerFactory::GetInstance().GetLogger("MainWindow");
    logger->Debug("Patrons fetch completed successfully.");
  }
  m_lastPatronsResult = e.result;
}

// --- Popup Modals: Patrons, Update, HamburgerMenu, Manual, About, Legal, Shutdown --- //

// --- Popup Modals: Patrons ---
void MainWindow::RenderPatronsPopup() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_isPatronsPopupOpen) {
    ImGui::OpenPopup(m_locPatronsButtonTooltip.c_str());
    m_isPatronsPopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locPatronsButtonTooltip.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    {
      ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
      Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locPatronsTitle.c_str());
      ImGui::PopStyleVar();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsIntro.c_str());
    ImGui::Spacing();

    {
      std::string patreonUrl = m_configService.GetValue("framework", "info.patreonUrl", "").get<std::string>();
      if (!patreonUrl.empty()) {
        Typography::Text(TextStyle::Regular().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsLinkIntro.c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::URL_LINK).Underline(), "%s", m_locPatronsLinkText.c_str());
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", m_locPatronsLinkTooltip.c_str());
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          if (ImGui::IsMouseClicked(0)) {
            ShellExecute(NULL, "open", patreonUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
          }
        }
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsHofTeaser.c_str());
      } else {
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsHofTeaser.c_str());
      }
    }

    ImGui::Spacing();
    {
      ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
      Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locPatronsHofTitle.c_str());
      ImGui::PopStyleVar();
    }

    ImGui::BeginChild("patrons_list_scroll_region", ImVec2(750, 200), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (!m_lastPatronsResult.has_value()) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "Loading patrons...");
    } else if (!m_lastPatronsResult->success || !m_lastPatronsResult->data.has_value()) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)).Color(Colors::RED), "%s", loc.Get(m_lastPatronsResult->errorMessage.value_or(m_locUpdateErrorGeneric)).c_str());
    } else if (m_lastPatronsResult->data->empty()) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsHofEmpty.c_str());
    } else {
      std::map<int, std::vector<System::Patron>> patronsByTier;
      for (const auto& p : m_lastPatronsResult->data.value()) {
        if (p.tier >= 3) {
          patronsByTier[p.tier].push_back(p);
        }
      }

      if (patronsByTier.empty()) {
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", m_locPatronsHofEmpty.c_str());
      } else {
        if (patronsByTier.count(5)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", m_locTierMagnateHeader.c_str());
          ImGui::Spacing();
          if (ImGui::BeginTable("MagnatesTable", 5, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : patronsByTier[5]) {
              ImGui::TableNextColumn();
              Typography::Text(TextStyle::Bold().Padding(ImVec2(15.0f, 0.0f)), "%s %s", ICON_FA_CROWN, p.name.c_str());
            }
            ImGui::EndTable();
          }
        }
        if (patronsByTier.count(4)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", m_locTierManagerHeader.c_str());
          ImGui::Spacing();
          if (ImGui::BeginTable("ManagersTable", 5, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : patronsByTier[4]) {
              ImGui::TableNextColumn();
              Typography::Text(TextStyle::Bold().Padding(ImVec2(15.0f, 0.0f)), "%s %s", ICON_FA_GEM, p.name.c_str());
            }
            ImGui::EndTable();
          }
        }
        if (patronsByTier.count(3)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", m_locTierMasterHeader.c_str());
          ImGui::Spacing();
          if (ImGui::BeginTable("MastersTable", 5, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : patronsByTier[3]) {
              ImGui::TableNextColumn();
              Typography::Text(TextStyle::Bold().Padding(ImVec2(15.0f, 0.0f)), "%s %s", ICON_FA_STAR, p.name.c_str());
            }
            ImGui::EndTable();
          }
        }
      }
    }

    ImGui::EndChild();
    ImGui::Spacing();
    if (Button(m_locPatronsCloseButton.c_str(), TextStyle::DefaultButton())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: Update ---
void MainWindow::RenderUpdatePopup() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_isUpdatePopupOpen) {
    ImGui::OpenPopup(m_locUpdatePopupTitle.c_str());
    m_isUpdatePopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locUpdatePopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    bool hasChangelog = m_lastUpdateInfo.has_value() && !m_lastUpdateInfo->content.markdown.empty();
    ImVec2 childSize = hasChangelog ? ImVec2(575, 300) : ImVec2(450, 100);

    ImGui::BeginChild("description_modals_update", childSize, false);

    ImGui::Spacing();
    ImGui::Separator();
    Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GOLD), "%s%s v%s", m_locVersionLabel.c_str(), m_frameworkName.c_str(), m_frameworkVersion.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_lastUpdateInfo.has_value() && !m_lastUpdateError.has_value()) {
      ImGui::Spacing();
      ImGui::Spacing();
      Typography::Text(TextStyle::Bold().Wrapped().Align(TextAlign::Center).Color(Colors::GRAY), "%s", m_locUpdateChecking.c_str());
    } else if (m_lastUpdateError.has_value()) {
      Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::RED).Padding(ImVec2(15.0f, 0.0f)), "%s", loc.Get(m_lastUpdateError.value_or(m_locUpdateErrorGeneric)).c_str());
    } else if (m_lastUpdateInfo.has_value()) {
      const auto& updateData = m_lastUpdateInfo.value();
      auto bodyStyle = TextStyle::Bold().Wrapped().Color(Colors::SILVER).Padding(ImVec2(10, 0));

      if (m_currentUpdateStatus == Modules::CommunicationManager::UpdateStatus::UpToDate) {
        ImGui::Spacing();
        Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::WHITE).Align(TextAlign::Center), "%s", m_locUpdateNoUpdate.c_str());
      } else if (updateData.updateAvailable) {
        Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::WHITE).Padding(ImVec2(15.0f, 0.0f)), "%s", fmt::format(fmt::runtime(m_locUpdateAvailable), updateData.latestVersion.full).c_str());

        if (hasChangelog) {
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          ImGui::BeginChild("ChangelogScroll", ImVec2(0, 175), false, ImGuiWindowFlags_HorizontalScrollbar);
          Typography::RenderMarkdownText(updateData.content.markdown, bodyStyle);
          ImGui::EndChild();
          ImGui::Spacing();
          ImGui::Separator();
        }

        ImGui::Spacing();
        Typography::Text(TextStyle::Bold().Color(Colors::URL_LINK).Underline().Align(TextAlign::Center), "%s", m_locUpdateDownloadLink.c_str());
        if (ImGui::IsItemHovered()) {
          std::string tooltipText = fmt::format(fmt::runtime(m_locUpdateDownloadTooltip), updateData.latestVersion.full);
          ImGui::SetTooltip("%s", tooltipText.c_str());
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          if (ImGui::IsMouseClicked(0)) {
            ShellExecute(NULL, "open", updateData.downloadUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
          }
        }
      }
    }

    ImGui::Spacing();
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(15.0f, 0.0f)).Color(Colors::WHITE), "%s ", m_locForDevelopers.c_str());
    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(15.0f, 0.0f)).Color(Colors::GRAY), "%s ", m_locUpdateDevNoteIntro.c_str());
    ImGui::SameLine();
    std::string githubUrl = m_configService.GetValue("framework", "info.githubUrl", "").get<std::string>();
    if (!githubUrl.empty()) {
      Typography::Text(TextStyle::Bold().Wrapped().Color(Colors::URL_LINK).Underline(), "%s", m_locUpdateDevNoteLink.c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locUpdateGithubTooltip.c_str());
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseClicked(0)) {
          ShellExecute(NULL, "open", githubUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
      }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (Button(m_locUpdateCloseButton.c_str(), TextStyle::DefaultButton())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: HamburgerMenu ---
void MainWindow::RenderHamburgerMenu() {
  if (ImGui::BeginPopup("HamburgerMenu")) {
    if (RenderStyledMenuItem(ICON_FA_CIRCLE_QUESTION, m_locMenuManual, m_locMenuManual)) {
      m_isManualPopupOpen = true;
    }
    if (RenderStyledMenuItem(ICON_FA_ENVELOPES_BULK, m_locMenuAbout, m_locMenuAbout)) {
      m_isAboutPopupOpen = true;
    }
    if (RenderStyledMenuItem(ICON_FA_SCALE_BALANCED, m_locMenuLegal, m_locMenuLegal)) {
      m_isLegalPopupOpen = true;
    }

    ImGui::Separator();
    ImGui::Spacing();

    if (Button(ICON_FA_FOLDER_OPEN, TextStyle::DefaultButton())) {
      const std::string pluginsPath = PathManager::GetPluginsPath().string();
      ShellExecute(NULL, "open", pluginsPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
      ImGui::CloseCurrentPopup();
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", m_locMenuOpenPluginsFolder.c_str());
    }
    ImGui::SameLine();
    const float button_width = Typography::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttons_total_width = (button_width * 2) + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttons_total_width);

    auto* hook = m_hookManager.GetHook("GameConsole");
    const bool isGameConsoleEnabled = hook && hook->IsEnabled();

    if (isGameConsoleEnabled) {
      if (Button(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::DefaultButton())) {
        m_eventManager.System.OnRequestExecuteCommand.Call({"sdk reinit"});
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locMenuReload.c_str());
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
      ImGui::Text("%s", ICON_FA_ARROW_ROTATE_LEFT);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locMenuReloadDisabledTooltip.c_str());
      }
    }

    ImGui::SameLine();

    if (isGameConsoleEnabled) {
      if (Button(ICON_FA_POWER_OFF, TextStyle::DefaultButton())) {
        m_isShutdownPopupOpen = true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locMenuShutdown.c_str());
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
      ImGui::Text("%s", ICON_FA_POWER_OFF);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", m_locMenuReloadDisabledTooltip.c_str());
      }
    }

    ImGui::EndPopup();
  }
}

// --- Popup Modals: Manual ---
void MainWindow::RenderManualPopup() {
  if (m_isManualPopupOpen) {
    ImGui::OpenPopup(m_locManualPopupTitle.c_str());
    m_isManualPopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locManualPopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    std::string githubUrl = m_configService.GetValue("framework", "info.githubUrl", "").get<std::string>();
    std::string patreonUrl = m_configService.GetValue("framework", "info.patreonUrl", "").get<std::string>();

    ImGui::BeginChild("ManualAnswer", ImVec2(575, 325), false);
    if (ImGui::TreeNode(m_locFaqQ1.c_str())) {
      const std::string pluginsPath = PathManager::GetPluginsPath().string();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA1), pluginsPath), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    if (ImGui::TreeNode(m_locFaqQ2.c_str())) {
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA2), githubUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    if (ImGui::TreeNode(m_locFaqQ3.c_str())) {
      std::filesystem::path examplePluginLocPath = PathManager::GetPluginsPath().filename() / "<PluginName>" / "localization";
      const std::string locDir = PathManager::GetLocalizationDir().string();
      const std::string examplePath = examplePluginLocPath.string();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA3), locDir, examplePath).c_str(), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    if (ImGui::TreeNode(m_locFaqQ4.c_str())) {
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA4), githubUrl, patreonUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    if (ImGui::TreeNode(m_locFaqQ5.c_str())) {
      std::filesystem::path examplePluginConfigPath = PathManager::GetPluginsPath().filename() / "<PluginName>" / "config" / "settings.json";
      const std::string configDir = PathManager::GetConfigDir().string();
      const std::string examplePath = examplePluginConfigPath.string();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA5), configDir, examplePath).c_str(), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    if (ImGui::TreeNode(m_locFaqQ6.c_str())) {
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locFaqA6), githubUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (Button(m_locUpdateCloseButton.c_str(), TextStyle::DefaultButton())) {
      ImGui::Spacing();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: About ---
void MainWindow::RenderAboutPopup() {
  auto& loc = LocalizationManager::GetInstance();
  if (m_isAboutPopupOpen) {
    ImGui::OpenPopup(m_locAboutPopupTitle.c_str());
    m_isAboutPopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locAboutPopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("AboutPopupContent", ImVec2(575, 325), false);

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locAboutUsTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    std::string aboutText = fmt::format(fmt::runtime(m_locAboutUsText), m_frameworkAuthor, m_frameworkName);
    Typography::Text(TextStyle::Regular().Wrapped(), "%s", aboutText.c_str());
    ImGui::Spacing();

    ImGui::Spacing();
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locAboutFrameworkTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    if (!m_descriptionKey.empty()) {
      Typography::RenderMarkdownText(loc.Get(m_descriptionKey), TextStyle::Regular().Wrapped());
      ImGui::Spacing();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locContactsTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    if (!m_emailUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_ENVELOPE);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locEmailText), m_emailUrl), TextStyle::Regular());
    }
    if (!m_discordUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_DISCORD);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locDiscordText), m_discordUrl), TextStyle::Regular());
    }
    if (!m_youtubeUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_YOUTUBE);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locYoutubeText), m_youtubeUrl), TextStyle::Regular());
    }
    if (!m_githubUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_GITHUB);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locGithubText), m_githubUrl), TextStyle::Regular());
    }
    if (!m_patreonUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_PATREON);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locPatreonText), m_patreonUrl), TextStyle::Regular());
    }
    if (!m_scsForumUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_COMMENTS);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locScsForumText), m_scsForumUrl), TextStyle::Regular());
    }
    if (!m_steamProfileUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_STEAM);
      ImGui::SameLine();
      Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locSteamProfileText), m_steamProfileUrl), TextStyle::Regular());
    }

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (Button(m_locUpdateCloseButton.c_str(), TextStyle::DefaultButton())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: Legal ---
void MainWindow::RenderLegalPopup() {
  if (m_isLegalPopupOpen) {
    ImGui::OpenPopup(m_locLegalPopupTitle.c_str());
    m_isLegalPopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locLegalPopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("LegalPopupContent", ImVec2(700, 400), false, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locLegalLicenseTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locLegalLicenseText), m_licenseUrl), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locLegalDisclaimerTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(m_locLegalDisclaimerText, TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locLegalFairPlayTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(m_locLegalFairPlayText, TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", m_locLegalContactTitle.c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(fmt::format(fmt::runtime(m_locLegalContactText), m_emailUrl), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (Button(m_locUpdateCloseButton.c_str(), TextStyle::DefaultButton())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void MainWindow::RenderShutdownPopup() {
  if (m_isShutdownPopupOpen) {
    ImGui::OpenPopup(m_locShutdownPopupTitle.c_str());
    m_isShutdownPopupOpen = false;
  }

  if (ImGui::BeginPopupModal(m_locShutdownPopupTitle.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("ShutdownPopupContent", ImVec2(500, 200), false, ImGuiWindowFlags_HorizontalScrollbar);
    Typography::RenderMarkdownText(m_locShutdownPopupContent, TextStyle::Regular().Wrapped());
    ImGui::EndChild();
    ImGui::Separator();
    if (Button(m_locShutdownPopupConfirm.c_str(), TextStyle::DefaultButton())) {
      m_eventManager.System.OnRequestExecuteCommand.Call({"sdk unload"});
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (Button(m_locShutdownPopupCancel.c_str(), TextStyle::DefaultButton())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

}  // namespace UI
SPF_NS_END
