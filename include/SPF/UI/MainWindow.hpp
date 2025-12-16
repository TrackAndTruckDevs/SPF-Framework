#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Events/SystemEvents.hpp"    //  For update and patrons completion events
#include "SPF/Modules/UpdateManager.hpp"  //  For UpdateStatus enum
#include "SPF/System/ApiService.hpp"      //  For ApiResult, UpdateInfo, Patron
#include "SPF/Hooks/HookManager.hpp"

SPF_NS_BEGIN

namespace Events {
class EventManager;
}
namespace Modules {
class KeyBindsManager;
class UpdateManager;
class ITelemetryService;
}  // namespace Modules
namespace Config {
struct IConfigService;
}
namespace Input {
class InputManager;
}

namespace UI {
class MainWindow : public BaseWindow {
 public:
  MainWindow(Events::EventManager& eventManager, Input::InputManager& inputManager, Modules::KeyBindsManager& keyBindsManager,
             Config::IConfigService& configService, Modules::ITelemetryService& telemetryService);

  ImGuiID GetMainDockspaceID() const;

 protected:
  void RenderContent() override;
  const char* GetWindowTitle() const override;
  ImGuiWindowFlags GetExtraWindowFlags() const override;

  //  Override base window event handlers
  void OnUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e) override;
  void OnUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e) override;
  void OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) override;

 private:
  void ToggleVisibility();
  void RenderPatronsPopup();
  void RenderUpdatePopup();
  void RenderManualPopup();
  void RenderAboutPopup();
  void RenderShutdownPopup();
  void RenderLegalPopup();
  void RenderHamburgerMenu();

  // --- Injected Services ---
  Events::EventManager& m_eventManager;
  Input::InputManager& m_inputManager;
  Modules::KeyBindsManager& m_keyBindsManager;
  Config::IConfigService& m_configService;
  Hooks::HookManager& m_hookManager;
  Modules::ITelemetryService& m_telemetryService;

  ImGuiID m_dockspaceId = 0;

  // --- Framework Info ---
  std::string m_frameworkName;
  std::string m_frameworkVersion;
  std::string m_frameworkAuthor;
  std::string m_descriptionKey;

  // --- Contact URLs ---
  std::string m_emailUrl;
  std::string m_discordUrl;
  std::string m_youtubeUrl;
  std::string m_githubUrl;
  std::string m_patreonUrl;
  std::string m_scsForumUrl;
  std::string m_steamProfileUrl;
  std::string m_licenseUrl;

  std::string m_locPatronsButtonTooltip;

  // Localisation Keys for Patrons Popup
  std::string m_locPatronsTitle;
  std::string m_locPatronsIntro;
  std::string m_locPatronsLinkIntro;
  std::string m_locPatronsLinkText;
  std::string m_locPatronsLinkTooltip;
  std::string m_locPatronsHofTitle;
  std::string m_locPatronsHofEmpty;
  std::string m_locPatronsHofTeaser;
  std::string m_locPatronsCloseButton;
  std::string m_locTierMagnateHeader;
  std::string m_locTierManagerHeader;
  std::string m_locTierMasterHeader;  // Road Master
  std::string m_locTierHaulerHeader;
  std::string m_locTierDriverHeader;

  //  State for Update Check
  bool m_updateCheckInitiated = false;
  Modules::UpdateManager::UpdateStatus m_currentUpdateStatus = Modules::UpdateManager::UpdateStatus::Unknown;
  std::string m_updateApiStatus; // To store the status string from the API, e.g., "switch_to_release"
  std::optional<System::UpdateInfo> m_lastUpdateInfo;
  std::optional<std::string> m_lastUpdateError;
  bool m_isUpdatePopupOpen = false;
  bool m_isPatronsPopupOpen = false;

  //  State for Patrons Fetch
  bool m_patronsFetchInitiated = false;
  std::optional<System::ApiResult<std::vector<System::Patron>>> m_lastPatronsResult;

  //  Localization keys for Update Popup
  std::string m_locUpdateButtonTooltip;
  std::string m_locVersionLabel;
  std::string m_locUpdateChecking;
  std::string m_locUpdatePopupTitle;
  std::string m_locUpdateNoUpdate;
  std::string m_locUpdateAvailable;
  std::string m_locUpdateSwitchToRelease; // For the "beta has ended" message
  std::string m_locUpdateDownloadLink;
  std::string m_locUpdateDownloadTooltip;
  std::string m_locUpdateDevNoteIntro;
  std::string m_locUpdateDevNoteLink;
  std::string m_locUpdateGithubTooltip;
  std::string m_locUpdateErrorNoInternet;
  std::string m_locUpdateErrorServerUnavailable;
  std::string m_locUpdateErrorGeneric;
  std::string m_locUpdateCloseButton;

  // Localization keys for common strings
  std::string m_locForDevelopers;
  std::string m_locForUsers;

  // Localization keys for hamburger menu
  std::string m_locMenuManual;
  std::string m_locMenuAbout;
  std::string m_locMenuLegal;
  std::string m_locMenuReload;
  std::string m_locMenuReloadDisabledTooltip;
  std::string m_locMenuShutdown;
  std::string m_locMenuOpenPluginsFolder;

  // Localization keys for Game Status and Performance
  std::string m_locGameStatusRunningGame;
  std::string m_locGameStatusCurrentVersion;
  std::string m_locPerfFpsAvg;
  std::string m_locPerfFpsRollMinMax;
  std::string m_locPerfFpsGblMinMax;
  std::string m_locPerfGraphicsApiLabel;
  std::string m_locPluginsLoadedActivatedLabel;
  std::string m_locHooksLoadedActivatedLabel;
  std::string m_locTooltipFpsAvg;
  std::string m_locTooltipFpsRollMinMax;
  std::string m_locTooltipFpsGblMinMax;

  // Localization keys for menu popups
  std::string m_locManualPopupTitle;
  std::string m_locAboutFrameworkTitle;
  //std::string m_locManualPopupContent;
  std::string m_locAboutPopupTitle;
  std::string m_locAboutUsTitle;
  std::string m_locAboutUsText;
  std::string m_locContactsTitle;
  std::string m_locEmailText;
  std::string m_locDiscordText;
  std::string m_locYoutubeText;
  std::string m_locGithubText;
  std::string m_locPatreonText;
  std::string m_locScsForumText;
  std::string m_locSteamProfileText;
  std::string m_locShutdownPopupTitle;
  std::string m_locShutdownPopupContent;
  std::string m_locShutdownPopupConfirm;
  std::string m_locShutdownPopupCancel;

  // Localization keys for Legal popup
  std::string m_locLegalPopupTitle;
  std::string m_locLegalLicenseTitle;
  std::string m_locLegalLicenseText;
  std::string m_locLegalDisclaimerTitle;
  std::string m_locLegalDisclaimerText;
  std::string m_locLegalFairPlayTitle;
  std::string m_locLegalFairPlayText;
  std::string m_locLegalContactTitle;
  std::string m_locLegalContactText;

  // Localization keys for FAQ popup
  std::string m_locFaqQ1, m_locFaqA1;
  std::string m_locFaqQ2, m_locFaqA2;
  std::string m_locFaqQ3, m_locFaqA3;
  std::string m_locFaqQ4, m_locFaqA4;
  std::string m_locFaqQ5, m_locFaqA5;
  std::string m_locFaqQ6, m_locFaqA6;

  bool m_isManualPopupOpen = false;
  bool m_isAboutPopupOpen = false;
  bool m_isLegalPopupOpen = false;
  bool m_isShutdownPopupOpen = false;
};
}  // namespace UI

SPF_NS_END
