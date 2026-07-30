#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/UI/BaseWindow.hpp"

#include <string>


SPF_NS_BEGIN
namespace UI {

/**
 * @class InfoWindow
 * @brief Window for displaying detailed environment, game, and system information.
 */
class InfoWindow : public BaseWindow {
 public:
  InfoWindow(const std::string& componentName, const std::string& windowId);
  virtual ~InfoWindow() = default;


 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  void RenderFrameworkTab();
  void RenderGameTab();
  void RenderPathsTab();
  void RenderSystemTab();
  void RenderStatusTab();

  // --- Localization Keys ---
  std::string m_locFrameworkTab;
  std::string m_locGameTab;
  std::string m_locPathsTab;
  std::string m_locSystemTab;
  std::string m_locStatusTab;

  // Framework Labels
  std::string m_locFrameworkVersion;
  std::string m_locFrameworkBuildType;
  std::string m_locFrameworkConfig;
  std::string m_locFrameworkLoaderPath;

  // Game Labels
  std::string m_locGameFullName;
  std::string m_locGameCode;
  std::string m_locGameVersion;
  std::string m_locGameSteamId;
  std::string m_locGameIsSteam;
  std::string m_locGameActiveProfile;
  std::string m_locGameProfileType;
  std::string m_locGameExePath;
  std::string m_locGameRootPath;
  std::string m_locGameCommandLine;

  // Paths Labels
  std::string m_locPathScsUser;
  std::string m_locPathScsMods;
  std::string m_locPathScsMusic;
  std::string m_locPathScsScreenshots;
  std::string m_locPathCurrentProfile;
  std::string m_locPathGlobalAssets;
  std::string m_locPathPluginsRoot;
  std::string m_locPathGlobalConfig;
  std::string m_locPathNotFound;

  // System Labels
  std::string m_locSystemOs;
  std::string m_locSystemLocale;
  std::string m_locSystemArch;

  // Status Labels
  std::string m_locStatusRenderer;
  std::string m_locStatusVr;
  std::string m_locStatusTobii;
  std::string m_locStatusMultiplayer;
  std::string m_locStatusSteamOverlay;
  std::string m_locStatusActive;
  std::string m_locStatusInactive;
  std::string m_locStatusDllLoaded;
  std::string m_locStatusDllNotLoaded;
};

}  // namespace UI
SPF_NS_END
