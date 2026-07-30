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
  m_titleLocalizationKey = "info_window.title";
  RefreshLocalization();
}

void InfoWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locFrameworkTab = loc.Get("info_window.tabs.framework");
  m_locGameTab = loc.Get("info_window.tabs.game");
  m_locPathsTab = loc.Get("info_window.tabs.paths");
  m_locSystemTab = loc.Get("info_window.tabs.system");
  m_locStatusTab = loc.Get("info_window.tabs.status");
  m_locFrameworkVersion = loc.Get("info_window.framework.version");
  m_locFrameworkBuildType = loc.Get("info_window.framework.build_type");
  m_locFrameworkConfig = loc.Get("info_window.framework.configuration");
  m_locFrameworkLoaderPath = loc.Get("info_window.framework.loader_path");
  m_locGameFullName = loc.Get("info_window.game.full_name");
  m_locGameCode = loc.Get("info_window.game.code");
  m_locGameVersion = loc.Get("info_window.game.version");
  m_locGameSteamId = loc.Get("info_window.game.steam_id");
  m_locGameIsSteam = loc.Get("info_window.game.is_steam");
  m_locGameActiveProfile = loc.Get("info_window.game.active_profile");
  m_locGameProfileType = loc.Get("info_window.game.profile_type");
  m_locGameExePath = loc.Get("info_window.game.exe_path");
  m_locGameRootPath = loc.Get("info_window.game.root_path");
  m_locGameCommandLine = loc.Get("info_window.game.command_line");
  m_locPathScsUser = loc.Get("info_window.paths.scs_user");
  m_locPathScsMods = loc.Get("info_window.paths.scs_mods");
  m_locPathScsMusic = loc.Get("info_window.paths.scs_music");
  m_locPathScsScreenshots = loc.Get("info_window.paths.scs_screenshots");
  m_locPathCurrentProfile = loc.Get("info_window.paths.current_profile");
  m_locPathGlobalAssets = loc.Get("info_window.paths.global_assets");
  m_locPathPluginsRoot = loc.Get("info_window.paths.plugins_root");
  m_locPathGlobalConfig = loc.Get("info_window.paths.global_config");
  m_locPathNotFound = loc.Get("info_window.paths.not_found");
  m_locSystemOs = loc.Get("info_window.system.os");
  m_locSystemLocale = loc.Get("info_window.system.locale");
  m_locSystemArch = loc.Get("info_window.system.architecture");
  m_locStatusRenderer = loc.Get("info_window.status.renderer");
  m_locStatusVr = loc.Get("info_window.status.vr_active");
  m_locStatusTobii = loc.Get("info_window.status.tobii_active");
  m_locStatusMultiplayer = loc.Get("info_window.status.multiplayer");
  m_locStatusSteamOverlay = loc.Get("info_window.status.steam_overlay");
  m_locStatusActive = loc.Get("info_window.status.active");
  m_locStatusInactive = loc.Get("info_window.status.inactive");
  m_locStatusDllLoaded = loc.Get("info_window.status.dll_loaded");
  m_locStatusDllNotLoaded = loc.Get("info_window.status.dll_not_loaded");
}

void InfoWindow::RenderContent() {
  if (ImGui::BeginTabBar("InfoWindowTabs")) {
    if (ImGui::BeginTabItem((std::string(ICON_FA_GEAR) + " " + m_locFrameworkTab).c_str())) {
      RenderFrameworkTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_TRUCK) + " " + m_locGameTab).c_str())) {
      RenderGameTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_FOLDER_OPEN) + " " + m_locPathsTab).c_str())) {
      RenderPathsTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_GEAR) + " " + m_locSystemTab).c_str())) {
      RenderSystemTab();
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem((std::string(ICON_FA_CHART_LINE) + " " + m_locStatusTab).c_str())) {
      RenderStatusTab();
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
}

void InfoWindow::RenderFrameworkTab() {
  const auto& info = EnvironmentManager::GetInstance().GetFrameworkInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locFrameworkVersion.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.version.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locFrameworkBuildType.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.buildType.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locFrameworkConfig.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.configuration.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locFrameworkLoaderPath.c_str());
  ImGui::TextWrapped("%s", info.loaderPath.string().c_str());
}

void InfoWindow::RenderGameTab() {
  const auto& info = EnvironmentManager::GetInstance().GetGameInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameFullName.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.name.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameCode.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.code.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameVersion.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.version.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameSteamId.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%u", info.steamAppId);

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameIsSteam.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(info.isSteamVersion ? Colors::GREEN : Colors::RED), "%s", info.isSteamVersion ? "Yes" : "No");

  const auto& status = EnvironmentManager::GetInstance().GetStatus();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameActiveProfile.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::GOLD), "%s", status.profileName.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameProfileType.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.profileType.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameExePath.c_str());
  ImGui::TextWrapped("%s", info.exePath.string().c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locGameRootPath.c_str());
  ImGui::TextWrapped("%s", info.rootPath.string().c_str());

  if (ImGui::TreeNode(m_locGameCommandLine.c_str())) {
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
  ImGui::Spacing();

  auto drawPath = [&](const std::string& label, const std::filesystem::path& path) {
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s:", label.c_str());
    if (path.empty()) {
      Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locPathNotFound.c_str());
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
  const auto& info = EnvironmentManager::GetInstance().GetSystemInfo();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locSystemOs.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.osName.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locSystemLocale.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.locale.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locSystemArch.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", info.architecture.c_str());
}

void InfoWindow::RenderStatusTab() {
  const auto& status = EnvironmentManager::GetInstance().GetStatus();

  ImGui::Spacing();
  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locStatusRenderer.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.renderer.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locStatusVr.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isVR ? Colors::GREEN : Colors::GRAY), "%s", status.isVR ? m_locStatusActive.c_str() : m_locStatusInactive.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locStatusTobii.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isTobiiActive ? Colors::GREEN : Colors::GRAY), "%s", status.isTobiiActive ? m_locStatusDllLoaded.c_str() : m_locStatusDllNotLoaded.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locStatusMultiplayer.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(Colors::WHITE), "%s", status.multiplayer.c_str());

  Typography::Text(TextStyle::Bold().Color(Colors::GRAY), "%s", m_locStatusSteamOverlay.c_str());
  ImGui::SameLine();
  Typography::Text(TextStyle::Regular().Color(status.isSteamOverlayActive ? Colors::GREEN : Colors::GRAY), "%s", status.isSteamOverlayActive ? m_locStatusDllLoaded.c_str() : m_locStatusDllNotLoaded.c_str());
}

}  // namespace UI
SPF_NS_END
