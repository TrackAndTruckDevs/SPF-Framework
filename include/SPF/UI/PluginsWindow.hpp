#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include <string>


SPF_NS_BEGIN

namespace Events {
class EventManager;
}

namespace UI {
class PluginsWindow : public BaseWindow {
 public:
  PluginsWindow(const std::string& componentName, const std::string& windowId, Config::IConfigService& configService, Events::EventManager& eventManager);

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  Config::IConfigService& m_configService;
  Events::EventManager& m_eventManager;

  // Localization Keys
  std::string m_locTableStatus;
  std::string m_locTableName;
  std::string m_locTableActions;
  std::string m_locBtnEnable;
  std::string m_locBtnDisable;
  std::string m_locTooltipInfo;
  std::string m_locTooltipDesc;
  std::string m_locTooltipSettings;
  std::string m_locInfoPopupAuthor;
  std::string m_locInfoPopupVersion;
  std::string m_locStatusIncompatible;
  std::string m_locVirtInputRestartRequired;
  std::string m_locTooltipRestartSDK;
  std::string m_locStatusUpdateAvailable;
  std::string m_locTooltipUpdateAvailable;
};
}  // namespace UI

SPF_NS_END
