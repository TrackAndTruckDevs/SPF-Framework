#include "SPF/UI/InfoWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "imgui.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <stringapiset.h>
#include <winnls.h>

SPF_NS_BEGIN
namespace UI {
using namespace System;
using namespace Localization;

InfoWindow::InfoWindow(const std::string& componentName, const std::string& windowId) : BaseWindow(componentName, windowId) {
  // Main Window & Tabs
  m_locTitle = "info_window.title";
  m_locFrameworkTab = "info_window.tabs.framework";
  m_locGameTab = "info_window.tabs.game";
  m_locPathsTab = "info_window.tabs.paths";
  m_locSystemTab = "info_window.tabs.system";
  m_locStatusTab = "info_window.tabs.status";

  // Framework Labels
  m_locFrameworkVersion = "info_window.framework.version";
  m_locFrameworkBuildType = "info_window.framework.build_type";
  m_locFrameworkConfig = "info_window.framework.configuration";
  m_locFrameworkLoaderPath = "info_window.framework.loader_path";

  // Game Labels
  m_locGameFullName = "info_window.game.full_name";
  m_locGameCode = "info_window.game.code";
  m_locGameVersion = "info_window.game.version";
  m_locGameSteamId = "info_window.game.steam_id";
  m_locGameIsSteam = "info_window.game.is_steam";
  m_locGameActiveProfile = "info_window.game.active_profile";
  m_locGameProfileType = "info_window.game.profile_type";
  m_locGameExePath = "info_window.game.exe_path";
  m_locGameRootPath = "info_window.game.root_path";
  m_locGameCommandLine = "info_window.game.command_line";

  // Paths Labels
  m_locPathScsUser = "info_window.paths.scs_user";
  m_locPathScsMods = "info_window.paths.scs_mods";
  m_locPathScsMusic = "info_window.paths.scs_music";
  m_locPathScsScreenshots = "info_window.paths.scs_screenshots";
  m_locPathCurrentProfile = "info_window.paths.current_profile";
  m_locPathGlobalAssets = "info_window.paths.global_assets";
  m_locPathPluginsRoot = "info_window.paths.plugins_root";
  m_locPathGlobalConfig = "info_window.paths.global_config";
  m_locPathNotFound = "info_window.paths.not_found";

  // System Labels
  m_locSystemOs = "info_window.system.os";
  m_locSystemLocale = "info_window.system.locale";
  m_locSystemArch = "info_window.system.architecture";

  // Status Labels
  m_locStatusRenderer = "info_window.status.renderer";
  m_locStatusVr = "info_window.status.vr_active";
  m_locStatusTobii = "info_window.status.tobii_active";
  m_locStatusMultiplayer = "info_window.status.multiplayer";
  m_locStatusSteamOverlay = "info_window.status.steam_overlay";
  m_locStatusActive = "info_window.status.active";
  m_locStatusInactive = "info_window.status.inactive";
  m_locStatusDllLoaded = "info_window.status.dll_loaded";
  m_locStatusDllNotLoaded = "info_window.status.dll_not_loaded";
}

const char* InfoWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_locTitle).c_str(); }

void InfoWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();

  if (ImGui::BeginTabBar("InfoWindowTabs")) {
    if (ImGui::BeginTabItem((std::string(ICON_FA_GEAR) + " " + loc.Get(m_locFrameworkTab)).c_str())) {
      RenderFrameworkTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_TRUCK) + " " + loc.Get(m_locGameTab)).c_str())) {
      RenderGameTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_FOLDER_OPEN) + " " + loc.Get(m_locPathsTab)).c_str())) {
      RenderPathsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_GEAR) + " " + loc.Get(m_locSystemTab)).c_str())) {
      RenderSystemTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_CHART_LINE) + " " + loc.Get(m_locStatusTab)).c_str())) {
      RenderStatusTab();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void InfoWindow::RenderFrameworkTab() {
  auto& loc = LocalizationManager::GetInstance();
  const auto& info = EnvironmentManager::GetInstance().GetFrameworkInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locFrameworkVersion).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.version.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locFrameworkBuildType).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.buildType.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locFrameworkConfig).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.configuration.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locFrameworkLoaderPath).c_str());
  ImGui::TextWrapped("%s", info.loaderPath.string().c_str());
}

void InfoWindow::RenderGameTab() {
  auto& loc = LocalizationManager::GetInstance();
  const auto& info = EnvironmentManager::GetInstance().GetGameInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameFullName).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.name.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameCode).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.code.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameVersion).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.version.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameSteamId).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%u", info.steamAppId);

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameIsSteam).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(info.isSteamVersion ? Colors::GREEN : Colors::RED), "%s", info.isSteamVersion ? "Yes" : "No");

  const auto& status = EnvironmentManager::GetInstance().GetStatus();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameActiveProfile).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GOLD), "%s", status.profileName.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameProfileType).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.profileType.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameExePath).c_str());
  ImGui::TextWrapped("%s", info.exePath.string().c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locGameRootPath).c_str());
  ImGui::TextWrapped("%s", info.rootPath.string().c_str());

  if (ImGui::TreeNode(loc.Get(m_locGameCommandLine).c_str())) {
    std::string cmd;
    const std::wstring& wcmd = info.commandLine;
    if (!wcmd.empty()) {
      int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wcmd[0], (int)wcmd.size(), NULL, 0, NULL, NULL);
      cmd.resize(size_needed);
      WideCharToMultiByte(CP_UTF8, 0, &wcmd[0], (int)wcmd.size(), &cmd[0], size_needed, NULL, NULL);
    }
    ImGui::TextWrapped("%s", cmd.c_str());
    ImGui::TreePop();
  }
}

void InfoWindow::RenderPathsTab() {
  auto& loc = LocalizationManager::GetInstance();
  ImGui::Spacing();

  auto drawPath = [&](const std::string& key, const std::filesystem::path& path) {
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s:", loc.Get(key).c_str());
    if (path.empty()) {
      Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", loc.Get(m_locPathNotFound).c_str());
    } else {
      ImGui::TextWrapped("%s", (const char*)path.u8string().c_str());
    }
    ImGui::Spacing();
  };

  const auto& info = EnvironmentManager::GetInstance().GetGameInfo();
  drawPath(m_locPathScsUser, PathManager::GetSCSUserDir());
  drawPath(m_locPathScsMods, PathManager::GetSCSModsDir());
  drawPath(m_locPathScsMusic, info.musicPath);
  drawPath(m_locPathScsScreenshots, info.screenshotPath);
  drawPath(m_locPathCurrentProfile, PathManager::GetCurrentProfilePath());

  ImGui::Separator();
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "Framework Paths");
  drawPath(m_locPathGlobalAssets, PathManager::GetBasePath());
  drawPath(m_locPathPluginsRoot, PathManager::GetPluginsPath());
  drawPath(m_locPathGlobalConfig, PathManager::GetConfigDir());
}

void InfoWindow::RenderSystemTab() {
  auto& loc = LocalizationManager::GetInstance();
  const auto& info = EnvironmentManager::GetInstance().GetSystemInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locSystemOs).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.osName.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locSystemLocale).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.locale.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locSystemArch).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.architecture.c_str());
}

void InfoWindow::RenderStatusTab() {
  auto& loc = LocalizationManager::GetInstance();
  const auto& status = EnvironmentManager::GetInstance().GetStatus();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locStatusRenderer).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.renderer.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locStatusVr).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isVR ? Colors::GREEN : Colors::GRAY), "%s", status.isVR ? loc.Get(m_locStatusActive).c_str() : loc.Get(m_locStatusInactive).c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locStatusTobii).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isTobiiActive ? Colors::GREEN : Colors::GRAY), "%s", status.isTobiiActive ? loc.Get(m_locStatusDllLoaded).c_str() : loc.Get(m_locStatusDllNotLoaded).c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locStatusMultiplayer).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.multiplayer.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", loc.Get(m_locStatusSteamOverlay).c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isSteamOverlayActive ? Colors::GREEN : Colors::GRAY), "%s", status.isSteamOverlayActive ? loc.Get(m_locStatusDllLoaded).c_str() : loc.Get(m_locStatusDllNotLoaded).c_str());
}

}  // namespace UI
SPF_NS_END
