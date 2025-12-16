#include "SPF/UI/MainWindow.hpp"

#include <Windows.h>  // Pre-include for safety
#include <fmt/format.h>

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Modules/PerformanceMonitor.hpp"
#include "SPF/Telemetry/SCS/Common.hpp" //  Required for GameState definition

#include "SPF/Events/EventManager.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Hooks/HookManager.hpp"
#include "SPF/Hooks/IHook.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/System/PathManager.hpp"

#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIElements.hpp"
// #include <imgui.h>
#include "imgui_internal.h"

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
      m_licenseUrl(m_githubUrl + "/blob/main/LICENSE"),
      // Localisation Keys//
      m_locPatronsButtonTooltip("main_window.patrons_button_tooltip"),
      // Localisation Keys for Patrons Popup
      m_locPatronsTitle("patrons_popup.title"),
      m_locPatronsIntro("patrons_popup.intro_text"),
      m_locPatronsLinkIntro("patrons_popup.link_intro"),
      m_locPatronsLinkText("patrons_popup.link_text"),
      m_locPatronsLinkTooltip("patrons_popup.link_tooltip"),
      m_locPatronsHofTitle("patrons_popup.hof_title"),
      m_locPatronsHofEmpty("patrons_popup.hof_empty"),
      m_locPatronsHofTeaser("patrons_popup.hof_teaser"),
      m_locPatronsCloseButton("patrons_popup.close_button"),
      m_locTierMagnateHeader("patrons_popup.tiers.magnate"),
      m_locTierManagerHeader("patrons_popup.tiers.manager"),
      m_locTierMasterHeader("patrons_popup.tiers.master"),  // Road Master
      m_locTierHaulerHeader("patrons_popup.tiers.hauler"),
      m_locTierDriverHeader("patrons_popup.tiers.driver"),
      // Localization Keys for Update Popup
      m_locUpdateButtonTooltip("main_window.update_button_tooltip"),
      m_locVersionLabel("main_window.version_label"),
      m_locUpdateChecking("main_window.update_checking"),
      m_locUpdatePopupTitle("update_popup.title"),
      m_locUpdateNoUpdate("update_popup.no_update"),
      m_locUpdateAvailable("update_popup.update_available"),
      m_locUpdateSwitchToRelease("update_popup.switch_to_release"),
      m_locUpdateDownloadLink("update_popup.download_link"),
      m_locUpdateDownloadTooltip("update_popup.download_tooltip"),
      m_locUpdateDevNoteIntro("update_popup.developers_note_intro"),
      m_locUpdateDevNoteLink("update_popup.developers_note_link"),
      m_locUpdateGithubTooltip("update_popup.github_tooltip"),
      m_locUpdateErrorNoInternet("api.error.no_internet"),
      m_locUpdateErrorServerUnavailable("api.error.server_unavailable"),
      m_locUpdateErrorGeneric("api.error.generic"),
      m_locUpdateCloseButton("update_popup.close_button"),
      // Localization keys for common strings
      m_locForDevelopers("common.for_developers"),
      m_locForUsers("common.for_users"),
      // Localization keys for hamburger menu
      m_locMenuManual("main_window.menu.manual"),
      m_locMenuAbout("main_window.menu.about"),
      m_locMenuLegal("main_window.menu.legal"),
      m_locMenuReload("main_window.menu.reload"),
      m_locMenuReloadDisabledTooltip("main_window.menu.reload_disabled_tooltip"),
      m_locMenuShutdown("main_window.menu.shutdown"),
      m_locMenuOpenPluginsFolder("main_window.menu.open_plugins_folder"),
      // Localization keys for menu popups
      m_locManualPopupTitle("manual_popup.title"),
      m_locAboutFrameworkTitle("about_popup.framework_title"),
      // m_locManualPopupContent("manual_popup.content"),
      m_locAboutPopupTitle("about_popup.title"),
      m_locAboutUsTitle("about_popup.about_us_title"),
      m_locAboutUsText("about_popup.about_us_text"),
      m_locContactsTitle("about_popup.contacts_title"),
      m_locEmailText("about_popup.email_text"),
      m_locDiscordText("about_popup.discord_text"),
      m_locYoutubeText("about_popup.youtube_text"),
      m_locGithubText("about_popup.github_text"),
      m_locPatreonText("about_popup.patreon_text"),
      m_locScsForumText("about_popup.scs_forum_text"),
      m_locSteamProfileText("about_popup.steam_profile_text"),
      m_locShutdownPopupTitle("shutdown_popup.title"),
      m_locShutdownPopupContent("shutdown_popup.content"),
      m_locShutdownPopupConfirm("shutdown_popup.confirm_button"),
      m_locShutdownPopupCancel("shutdown_popup.cancel_button"),
      // Localization keys for Legal popup
      m_locLegalPopupTitle("legal_popup.title"),
      m_locLegalLicenseTitle("legal_popup.license_title"),
      m_locLegalLicenseText("legal_popup.license_text"),
      m_locLegalDisclaimerTitle("legal_popup.disclaimer_title"),
      m_locLegalDisclaimerText("legal_popup.disclaimer_text"),
      m_locLegalFairPlayTitle("legal_popup.fair_play_title"),
      m_locLegalFairPlayText("legal_popup.fair_play_text"),
      m_locLegalContactTitle("legal_popup.contact_title"),
      m_locLegalContactText("legal_popup.contact_text"),
      // Localization keys for FAQ popup
      m_locFaqQ1("manual_popup.q1_question"),
      m_locFaqA1("manual_popup.q1_answer"),
      m_locFaqQ2("manual_popup.q2_question"),
      m_locFaqA2("manual_popup.q2_answer"),
      m_locFaqQ3("manual_popup.q3_question"),
      m_locFaqA3("manual_popup.q3_answer"),
      m_locFaqQ4("manual_popup.q4_question"),
      m_locFaqA4("manual_popup.q4_answer"),
      m_locFaqQ5("manual_popup.q5_question"),
      m_locFaqA5("manual_popup.q5_answer"),
      m_locFaqQ6("manual_popup.q6_question"),
      m_locFaqA6("manual_popup.q6_answer"),
      // Game Status and Performance
      m_locGameStatusRunningGame("main_window.game_status.running_game_label"),
      m_locGameStatusCurrentVersion("main_window.game_status.current_version_label"),
      m_locPerfFpsAvg("main_window.performance.fps_avg_label"),
      m_locPerfFpsRollMinMax("main_window.performance.fps_roll_minmax_label"),
      m_locPerfFpsGblMinMax("main_window.performance.fps_gbl_minmax_label"),
      m_locPerfGraphicsApiLabel("main_window.performance.graphics_api_label"),
      m_locPluginsLoadedActivatedLabel("main_window.summary.plugins_loaded_activated_label"),
      m_locHooksLoadedActivatedLabel("main_window.summary.hooks_loaded_activated_label"),
      m_locTooltipFpsAvg("main_window.performance.tooltip_fps_avg"),
      m_locTooltipFpsRollMinMax("main_window.performance.tooltip_fps_roll_minmax"),
      m_locTooltipFpsGblMinMax("main_window.performance.tooltip_fps_gbl_minmax") {
  m_keyBindsManager.RegisterAction("framework.ui.main_window.toggle", [this]() { ToggleVisibility(); });
}

void MainWindow::ToggleVisibility() { SetVisibility(!IsVisible()); }

const char* MainWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get("main_window.title").c_str(); }

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
  auto& loc = LocalizationManager::GetInstance();

  // --- Initial API Calls on First Open ---
  if (IsVisible() && !m_wasVisibleLastFrame) {
    if (!m_updateCheckInitiated) {
      if (!m_frameworkVersion.empty()) {
        m_eventManager.System.OnRequestUpdateCheck.Call({});
      } else {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MainWindow");
        logger->Warn("Framework version is not specified in the manifest. Update check will be skipped.");
      }
      m_updateCheckInitiated = true;
    }
    if (!m_patronsFetchInitiated) {
      m_eventManager.System.OnRequestPatronsFetch.Call({});
      m_patronsFetchInitiated = true;
    }
  }

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

    // Patrons Button
    if (HyperlinkButton(ICON_FA_HAND_HOLDING_HEART, TextStyle::Regular().HoverColor(Colors::GOLD))) {
      m_isPatronsPopupOpen = true;
    }
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("%s", loc.Get(m_locPatronsButtonTooltip).c_str());
    }

    ImGui::SameLine();

    // Update Button
    {
      TextStyle updateButtonStyle = TextStyle::Regular().HoverColor(Colors::GOLD);
      switch (m_currentUpdateStatus) {
        case Modules::UpdateManager::UpdateStatus::PatchAvailable:
          updateButtonStyle.Color(Colors::YELLOW);
          break;
        case Modules::UpdateManager::UpdateStatus::MinorAvailable:
          updateButtonStyle.Color(Colors::ORANGE);
          break;
        case Modules::UpdateManager::UpdateStatus::MajorAvailable:
          updateButtonStyle.Color(Colors::RED);
          break;
        default:
          // Keep default color for Unknown or UpToDate
          break;
      }

      if (HyperlinkButton(ICON_FA_ARROWS_ROTATE, updateButtonStyle)) {
        m_isUpdatePopupOpen = true;
        if (!m_frameworkVersion.empty()) {
          m_eventManager.System.OnRequestUpdateCheck.Call({});
        } else {
          auto logger = Logging::LoggerFactory::GetInstance().GetLogger("MainWindow");
          logger->Warn("Cannot perform update check: Framework version is not specified in the manifest.");
        }
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locUpdateButtonTooltip).c_str());
      }
    }

    ImGui::SameLine();

    // Hamburger Menu Button
    if (HyperlinkButton(ICON_FA_BARS, TextStyle::Regular().HoverColor(Colors::GOLD))) {
      ImGui::OpenPopup("HamburgerMenu");
    }  
  }

    // --- Game Info ---
    ImGui::Spacing();
    ImGui::Spacing();

    const auto& gameState = m_telemetryService.GetGameState();
    if (!gameState.game_name.empty()) {
        const std::string& full_game_name = gameState.game_name;
        size_t last_space_pos = full_game_name.find_last_of(' ');

        if (last_space_pos != std::string::npos) {
            std::string game_name_only = full_game_name.substr(0, last_space_pos);
            std::string game_version = full_game_name.substr(last_space_pos + 1);

            Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameStatusRunningGame).c_str());
            ImGui::SameLine();
            Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", game_name_only.c_str());
            Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameStatusCurrentVersion).c_str());
            ImGui::SameLine();
            Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", game_version.c_str());
        }
    }

    // --- Performance Stats ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    auto& perf = PerformanceMonitor::GetInstance();
    auto* renderer = UIManager::GetInstance().GetRenderer();

    if (renderer) {
        const char* apiString = "Unknown";
        switch (renderer->GetDetectedAPI()) {
            case RenderAPI::D3D11:   apiString = "DirectX 11"; break;
            case RenderAPI::D3D12:   apiString = "DirectX 12"; break;
            case RenderAPI::OpenGL:  apiString = "OpenGL";     break;
            default:                 apiString = "Unknown";    break;
        }

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locPerfGraphicsApiLabel).c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%s", apiString);
        ImGui::SameLine();
        ImGui::Dummy(ImVec2(10.0f, 0.0f)); // Spacer
        ImGui::SameLine();
    }

    if (perf.GetDeltaTime() > 0.0f) {
        // FPSAvg
        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locPerfFpsAvg).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc.Get(m_locTooltipFpsAvg).c_str());
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

        // FPSRoll Min/Max
        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locPerfFpsRollMinMax).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc.Get(m_locTooltipFpsRollMinMax).c_str());
        }
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::WHITE), "%.0f/%.0f", perf.GetRollingMinFPS(), perf.GetRollingMaxFPS());
        ImGui::SameLine();

        ImGui::Dummy(ImVec2(10.0f, 0.0f));
        ImGui::SameLine();

        // FPSGbl Min/Max
        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locPerfFpsGblMinMax).c_str());
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", loc.Get(m_locTooltipFpsGblMinMax).c_str());
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

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locPluginsLoadedActivatedLabel).c_str());
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

        Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locHooksLoadedActivatedLabel).c_str());
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
  ImGuiDockNodeFlags dockspace_flags =
      ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_NoDockingSplit | ImGuiDockNodeFlags_NoDockingOverCentralNode | ImGuiDockNodeFlags_NoUndocking;
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

void MainWindow::OnUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("MainWindow");
  logger->Debug("Update check succeeded.");
  m_lastUpdateInfo = e.updateInfo;
  m_lastUpdateError.reset();

  m_updateApiStatus = m_lastUpdateInfo->status;  // Store the raw status string

  if (m_updateApiStatus == "switch_to_release") {
    m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::MinorAvailable;  // Force orange icon for "beta ended"
  } else if (m_lastUpdateInfo->updateAvailable) {
    if (m_lastUpdateInfo->severity == "major") {
      m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::MajorAvailable;
    } else if (m_lastUpdateInfo->severity == "minor") {
      m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::MinorAvailable;
    } else if (m_lastUpdateInfo->severity == "patch") {
      m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::PatchAvailable;
    } else {
      m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::Unknown;  // Should not happen
    }
  } else {
    m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::UpToDate;
  }
}

void MainWindow::OnUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("MainWindow");
  logger->Warn("Update check failed. Error: {}.", e.errorMessage.value_or("N/A"));
  m_lastUpdateInfo.reset();
  m_lastUpdateError = e.errorMessage;
  m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::Unknown;
}

void MainWindow::OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("MainWindow");
  logger->Debug("Patrons fetch completed. Success: {}. Error: {}.", e.result.success, e.result.errorMessage.value_or("N/A"));
  m_lastPatronsResult = e.result;
}

// --- Popup Modals: Patrons, Update, HamburgerMenu, Manual, About, Legal, Shutdown --- //

// --- Popup Modals: Patrons ---
void MainWindow::RenderPatronsPopup() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_isPatronsPopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locPatronsButtonTooltip).c_str());
    m_isPatronsPopupOpen = false;  // Reset flag
  }

  if (ImGui::BeginPopupModal(loc.Get(m_locPatronsButtonTooltip).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    // --- 1. Header ---
    {
      ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
      Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locPatronsTitle).c_str());
      ImGui::PopStyleVar();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // --- 2. Introduction Text ---
    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsIntro).c_str());
    ImGui::Spacing();

    // --- 3. Clickable Patreon Link ---
    {
      std::string patreonUrl = m_configService.GetValue("framework", "info.patreonUrl", "").get<std::string>();
      if (!patreonUrl.empty()) {  // Only draw if URL is present
        Typography::Text(TextStyle::Regular().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsLinkIntro).c_str());
        ImGui::SameLine();
        Typography::Text(TextStyle::Bold().Color(Colors::URL_LINK).Underline(), "%s", loc.Get(m_locPatronsLinkText).c_str());
        if (ImGui::IsItemHovered()) {
          ImGui::SetTooltip("%s", loc.Get(m_locPatronsLinkTooltip).c_str());
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          if (ImGui::IsMouseClicked(0)) {
            ShellExecute(NULL, "open", patreonUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
          }
        }
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsHofTeaser).c_str());
      } else {
        // If no Patreon URL, only draw the teaser text if it makes sense without the link
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsHofTeaser).c_str());
      }
    }

    // --- 4. Hall of Fame ---
    ImGui::Spacing();
    {
      ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
      Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locPatronsHofTitle).c_str());
      ImGui::PopStyleVar();
    }

    // --- Patrons List Rendering ---
    ImGui::BeginChild("patrons_list_scroll_region", ImVec2(750, 200), false, ImGuiWindowFlags_HorizontalScrollbar);

    if (!m_lastPatronsResult.has_value()) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "Loading patrons...");  // TODO: Localize
    } else if (!m_lastPatronsResult->success) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)).Color(Colors::RED),
                       "%s",
                       loc.Get(m_lastPatronsResult->errorMessage.value_or(m_locUpdateErrorGeneric)).c_str());
    } else if (m_lastPatronsResult->data->empty()) {
      Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsHofEmpty).c_str());
    } else {
      // Filter patrons by tiers that should be in the Hall of Fame
      std::map<int, std::vector<System::Patron>> patronsByTier;
      for (const auto& p : m_lastPatronsResult->data.value()) {
        if (p.tier >= 3) {
          patronsByTier[p.tier].push_back(p);
        }
      }

      if (patronsByTier.empty()) {
        Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)), "%s", loc.Get(m_locPatronsHofEmpty).c_str());
      } else {
        // TIER 5: Logistics Magnate
        if (patronsByTier.count(5)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", loc.Get(m_locTierMagnateHeader).c_str());
          ImGui::Spacing();
          if (ImGui::BeginTable("MagnatesTable", 5, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : patronsByTier[5]) {
              ImGui::TableNextColumn();
              Typography::Text(TextStyle::Bold().Padding(ImVec2(15.0f, 0.0f)), "%s %s", ICON_FA_CROWN, p.name.c_str());
            }
            ImGui::EndTable();
          }
        }
        // TIER 4: Fleet Manager
        if (patronsByTier.count(4)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", loc.Get(m_locTierManagerHeader).c_str());
          ImGui::Spacing();
          if (ImGui::BeginTable("ManagersTable", 5, ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_SizingStretchSame)) {
            for (const auto& p : patronsByTier[4]) {
              ImGui::TableNextColumn();
              Typography::Text(TextStyle::Bold().Padding(ImVec2(15.0f, 0.0f)), "%s %s", ICON_FA_GEM, p.name.c_str());
            }
            ImGui::EndTable();
          }
        }
        // TIER 3: Road Master
        if (patronsByTier.count(3)) {
          Typography::Text(TextStyle::H3().Align(TextAlign::Center).Color(Colors::GRAY), "%s", loc.Get(m_locTierMasterHeader).c_str());
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
    // --- 5. Close Button ---
    if (ImGui::Button(loc.Get(m_locPatronsCloseButton).c_str())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: Update ---
void MainWindow::RenderUpdatePopup() {
  auto& loc = LocalizationManager::GetInstance();

  if (m_isUpdatePopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locUpdatePopupTitle).c_str());
    m_isUpdatePopupOpen = false;  // Reset flag
  }

  if (ImGui::BeginPopupModal(loc.Get(m_locUpdatePopupTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    // Determine required popup size based on content
    bool hasChangelog = m_lastUpdateInfo.has_value() && !m_lastUpdateInfo->changelog.empty();
    ImVec2 childSize = hasChangelog ? ImVec2(575, 300) : ImVec2(450, 75);
    
    ImGui::BeginChild("description_modals_update", childSize, false);
    
    // --- Header ---
    ImGui::Spacing();
    ImGui::Separator();
    Typography::Text(
    TextStyle::H3().Align(TextAlign::Center).Color(Colors::GOLD), "%s%s v%s", loc.Get(m_locVersionLabel).c_str(), m_frameworkName.c_str(), m_frameworkVersion.c_str());
    ImGui::Separator();
    ImGui::Spacing();
    
    // --- Content ---
    if (!m_lastUpdateInfo.has_value() && !m_lastUpdateError.has_value()) {
      // 1. Loading state
      ImGui::Spacing();
      ImGui::Spacing();
      Typography::Text(TextStyle::Bold().Wrapped().Align(TextAlign::Center).Color(Colors::GRAY), "%s", loc.Get(m_locUpdateChecking).c_str());
    } else if (m_lastUpdateError.has_value()) {
      // 2. Error state
      Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::RED).Padding(ImVec2(15.0f, 0.0f)),
                       "%s",
                       loc.Get(m_lastUpdateError.value_or(m_locUpdateErrorGeneric)).c_str());
    } else if (m_lastUpdateInfo.has_value()) {
      // 3. Success states (guaranteed to have data)
      const auto& updateData = m_lastUpdateInfo.value();

      if (m_updateApiStatus == "up_to_date") {
        ImGui::Spacing();
        Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::WHITE).Align(TextAlign::Center), "%s", loc.Get(m_locUpdateNoUpdate).c_str());
      } else if (m_updateApiStatus == "switch_to_release") {
        Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::WHITE).Padding(ImVec2(15.0f, 0.0f)),
                       "%s %s",
                       loc.Get(m_locUpdateSwitchToRelease).c_str(),
                       updateData.formattedLatestVersion.c_str());

        if (hasChangelog) {
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          ImGui::BeginChild("ChangelogScroll", ImVec2(0, 175), false, ImGuiWindowFlags_HorizontalScrollbar);
          Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)).Color(Colors::GRAY), "%s", updateData.changelog.c_str());
          ImGui::EndChild();
          ImGui::Spacing();
          ImGui::Separator();
        }

        ImGui::Spacing();
        Typography::Text(TextStyle::Bold().Color(Colors::URL_LINK).Underline().Align(TextAlign::Center), "%s", loc.Get(m_locUpdateDownloadLink).c_str());
        if (ImGui::IsItemHovered()) {
          std::string tooltipText = loc.GetFormatted("framework", m_locUpdateDownloadTooltip, updateData.formattedLatestVersion);
          ImGui::SetTooltip("%s", tooltipText.c_str());
          ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
          if (ImGui::IsMouseClicked(0)) {
            ShellExecute(NULL, "open", updateData.downloadUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
          }
        }
      } else if (m_updateApiStatus == "update_available") {
        Typography::Text(TextStyle::Regular().Wrapped().Color(Colors::WHITE).Padding(ImVec2(15.0f, 0.0f)),
                       "%s",
                       loc.GetFormatted("framework", m_locUpdateAvailable, updateData.formattedLatestVersion).c_str());
        
        if (hasChangelog) {
          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();
          ImGui::BeginChild("ChangelogScroll", ImVec2(0, 175), false, ImGuiWindowFlags_HorizontalScrollbar);
          Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(10.0f, 0.0f)).Color(Colors::GRAY), "%s", updateData.changelog.c_str());
          ImGui::EndChild();
          ImGui::Spacing();
          ImGui::Separator();
        }

        ImGui::Spacing();
        Typography::Text(TextStyle::Bold().Color(Colors::URL_LINK).Underline().Align(TextAlign::Center), "%s", loc.Get(m_locUpdateDownloadLink).c_str());
        if (ImGui::IsItemHovered()) {
          std::string tooltipText = loc.GetFormatted("framework", m_locUpdateDownloadTooltip, updateData.formattedLatestVersion);
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

    // --- Footer with dev note and close button ---
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(15.0f, 0.0f)).Color(Colors::WHITE), "%s ", loc.Get(m_locForDevelopers).c_str());
    Typography::Text(TextStyle::Regular().Wrapped().Padding(ImVec2(15.0f, 0.0f)).Color(Colors::GRAY), "%s ", loc.Get(m_locUpdateDevNoteIntro).c_str());
    ImGui::SameLine();
    std::string githubUrl = m_configService.GetValue("framework", "info.githubUrl", "").get<std::string>();
    if (!githubUrl.empty()) {
      Typography::Text(TextStyle::Bold().Wrapped().Color(Colors::URL_LINK).Underline(), "%s", loc.Get(m_locUpdateDevNoteLink).c_str());
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locUpdateGithubTooltip).c_str());
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseClicked(0)) {
          ShellExecute(NULL, "open", githubUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
      }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button(loc.Get(m_locUpdateCloseButton).c_str())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: HamburgerMenu ---
void MainWindow::RenderHamburgerMenu() {
  auto& loc = LocalizationManager::GetInstance();
  if (ImGui::BeginPopup("HamburgerMenu")) {
    if (RenderStyledMenuItem(ICON_FA_CIRCLE_QUESTION, loc.Get(m_locMenuManual), loc.Get(m_locMenuManual))) {
      m_isManualPopupOpen = true;
    }
    if (RenderStyledMenuItem(ICON_FA_ENVELOPES_BULK, loc.Get(m_locMenuAbout), loc.Get(m_locMenuAbout))) {
      m_isAboutPopupOpen = true;
    }
    if (RenderStyledMenuItem(ICON_FA_SCALE_BALANCED, loc.Get(m_locMenuLegal), loc.Get(m_locMenuLegal))) {
      m_isLegalPopupOpen = true;
    }

    ImGui::Separator();
    ImGui::Spacing();
    // Action buttons at the bottom

    // Open Plugins Folder button (left-aligned)
    if (HyperlinkButton(ICON_FA_FOLDER_OPEN, TextStyle::Regular().HoverColor(Colors::GOLD))) {
        const std::string pluginsPath = PathManager::GetPluginsPath().string();
        ShellExecute(NULL, "open", pluginsPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
        ImGui::CloseCurrentPopup();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locMenuOpenPluginsFolder).c_str());
    }
    ImGui::SameLine();
    // Existing buttons (right-aligned)
    const float button_width = Typography::CalcTextSize(ICON_FA_ARROW_ROTATE_LEFT).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float buttons_total_width = (button_width * 2) + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttons_total_width);

    // Reload Button
    auto* hook = m_hookManager.GetHook("GameConsole");
    const bool isGameConsoleEnabled = hook && hook->IsEnabled();

    if (isGameConsoleEnabled) {
      if (HyperlinkButton(ICON_FA_ARROW_ROTATE_LEFT, TextStyle::Regular().HoverColor(Colors::GOLD))) {
        m_eventManager.System.OnRequestExecuteCommand.Call({"sdk reinit"});
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locMenuReload).c_str());
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
      ImGui::Text("%s", ICON_FA_ARROW_ROTATE_LEFT);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locMenuReloadDisabledTooltip).c_str());
      }
    }

    ImGui::SameLine();

    // Shutdown Button
    if (isGameConsoleEnabled) {
      if (HyperlinkButton(ICON_FA_POWER_OFF, TextStyle::Regular().HoverColor(Colors::GOLD))) {
        m_isShutdownPopupOpen = true;
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locMenuShutdown).c_str());
      }
    } else {
      ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
      ImGui::Text("%s", ICON_FA_POWER_OFF);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.Get(m_locMenuReloadDisabledTooltip).c_str());
      }
    }

    ImGui::EndPopup();
  }
}

// --- Popup Modals: Manual ---
void MainWindow::RenderManualPopup() {
  auto& loc = LocalizationManager::GetInstance();
  // --- Manual Popup Trigger ---
  if (m_isManualPopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locManualPopupTitle).c_str());
    m_isManualPopupOpen = false;
  }

  // Manual Popup
  if (ImGui::BeginPopupModal(loc.Get(m_locManualPopupTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    // Fetch URLs once at the beginning
    std::string githubUrl = m_configService.GetValue("framework", "info.githubUrl", "").get<std::string>();
    std::string patreonUrl = m_configService.GetValue("framework", "info.patreonUrl", "").get<std::string>();

    ImGui::BeginChild("ManualAnswer", ImVec2(575, 325), false, ImGuiWindowFlags_HorizontalScrollbar);
    //  Question 1
    if (ImGui::TreeNode(loc.Get(m_locFaqQ1).c_str())) {
      const std::string pluginsPath = PathManager::GetPluginsPath().string();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA1, pluginsPath), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();  // Add some spacing between questions

    // Question 2
    if (ImGui::TreeNode(loc.Get(m_locFaqQ2).c_str())) {
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA2, githubUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    // Question 3
    if (ImGui::TreeNode(loc.Get(m_locFaqQ3).c_str())) {
      // Dynamically build the example path
      std::filesystem::path examplePluginLocPath = PathManager::GetPluginsPath().filename() / "<PluginName>" / "localization";
      const std::string locDir = PathManager::GetLocalizationDir().string();
      const std::string examplePath = examplePluginLocPath.string();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA3, locDir, examplePath).c_str(), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    // Question 4
    if (ImGui::TreeNode(loc.Get(m_locFaqQ4).c_str())) {
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA4, githubUrl, patreonUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    // Question 5
    if (ImGui::TreeNode(loc.Get(m_locFaqQ5).c_str())) {
      // Dynamically build the example path
      std::filesystem::path examplePluginConfigPath = PathManager::GetPluginsPath().filename() / "<PluginName>" / "config" / "settings.json";
      const std::string configDir = PathManager::GetConfigDir().string();
      const std::string examplePath = examplePluginConfigPath.string();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA5, configDir, examplePath).c_str(), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::Spacing();

    // Question 6
    if (ImGui::TreeNode(loc.Get(m_locFaqQ6).c_str())) {
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locFaqA6, githubUrl), TextStyle::Regular().Wrapped().Color(Colors::LIGHT_GRAY));
      ImGui::TreePop();
    }
    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button(loc.Get(m_locUpdateCloseButton).c_str())) {  // Re-use close button text
      ImGui::Spacing();
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: About ---
void MainWindow::RenderAboutPopup() {
  auto& loc = LocalizationManager::GetInstance();
  // --- About Popup Trigger ---
  if (m_isAboutPopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locAboutPopupTitle).c_str());
    m_isAboutPopupOpen = false;
  }

  // About Popup
  if (ImGui::BeginPopupModal(loc.Get(m_locAboutPopupTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("AboutPopupContent", ImVec2(575, 325), false, ImGuiWindowFlags_HorizontalScrollbar);

    // --- About Us Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locAboutUsTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    std::string aboutText = loc.GetFormatted("framework", m_locAboutUsText, m_frameworkAuthor, m_frameworkName);
    Typography::Text(TextStyle::Regular().Wrapped(), "%s", aboutText.c_str());
    ImGui::Spacing();

    // --- Framework Description Section ---
    ImGui::Spacing();
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locAboutFrameworkTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    if (!m_descriptionKey.empty()) {
      Typography::RenderMarkdownText(loc.Get(m_descriptionKey), TextStyle::Regular().Wrapped());
      ImGui::Spacing();
    }

    // --- Contacts Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locContactsTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    // --- Contact Links ---
    if (!m_emailUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_ENVELOPE);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locEmailText, m_emailUrl), TextStyle::Regular());
    }
    if (!m_discordUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_DISCORD);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locDiscordText, m_discordUrl), TextStyle::Regular());
    }
    if (!m_youtubeUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_YOUTUBE);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locYoutubeText, m_youtubeUrl), TextStyle::Regular());
    }
    if (!m_githubUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_GITHUB);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locGithubText, m_githubUrl), TextStyle::Regular());
    }
    if (!m_patreonUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_PATREON);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locPatreonText, m_patreonUrl), TextStyle::Regular());
    }
    if (!m_scsForumUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_COMMENTS);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locScsForumText, m_scsForumUrl), TextStyle::Regular());
    }
    if (!m_steamProfileUrl.empty()) {
      Typography::Text(TextStyle::Regular(), "%s", ICON_FA_STEAM);
      ImGui::SameLine();
      Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locSteamProfileText, m_steamProfileUrl), TextStyle::Regular());
    }

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button(loc.Get(m_locUpdateCloseButton).c_str())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

// --- Popup Modals: Legal ---
void MainWindow::RenderLegalPopup() {
  auto& loc = LocalizationManager::GetInstance();
  // --- Legal Popup Trigger ---
  if (m_isLegalPopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locLegalPopupTitle).c_str());
    m_isLegalPopupOpen = false;
  }

  // Legal Popup
  if (ImGui::BeginPopupModal(loc.Get(m_locLegalPopupTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("LegalPopupContent", ImVec2(700, 400), false, ImGuiWindowFlags_HorizontalScrollbar);

    // --- License Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locLegalLicenseTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locLegalLicenseText, m_licenseUrl), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    // --- Disclaimer Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locLegalDisclaimerTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(loc.Get(m_locLegalDisclaimerText), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    // --- Fair Play Policy Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locLegalFairPlayTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(loc.Get(m_locLegalFairPlayText), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    // --- Contact Section ---
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD), "%s", loc.Get(m_locLegalContactTitle).c_str());
    ImGui::PopStyleVar();
    ImGui::Spacing();

    Typography::RenderMarkdownText(loc.GetFormatted("framework", m_locLegalContactText, m_emailUrl), TextStyle::Regular().Wrapped());
    ImGui::Spacing();

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();

    if (ImGui::Button(loc.Get(m_locUpdateCloseButton).c_str())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
}

// --- Popup Modals: Shutdown ---
void MainWindow::RenderShutdownPopup() {
  auto& loc = LocalizationManager::GetInstance();
  // --- Shutdown Popup Trigger ---
  if (m_isShutdownPopupOpen) {
    ImGui::OpenPopup(loc.Get(m_locShutdownPopupTitle).c_str());
    m_isShutdownPopupOpen = false;
  }

  // Shutdown Confirmation Popup
  if (ImGui::BeginPopupModal(loc.Get(m_locShutdownPopupTitle).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::BeginChild("ShutdownPopupContent", ImVec2(500, 150), false, ImGuiWindowFlags_HorizontalScrollbar);
    Typography::RenderMarkdownText(loc.Get(m_locShutdownPopupContent), TextStyle::Regular().Wrapped());
    ImGui::EndChild();
    ImGui::Separator();
    if (ImGui::Button(loc.Get(m_locShutdownPopupConfirm).c_str())) {
      m_eventManager.System.OnRequestExecuteCommand.Call({"sdk unload"});
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(loc.Get(m_locShutdownPopupCancel).c_str())) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

}  // namespace UI
SPF_NS_END
