/**                                                                                               
 * @file ClimateWindow.hpp                                                                          
 * @brief Header for the UI window for Climate and Weather manipulation.
 */ 

#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Namespace.hpp"
#include <string>

SPF_NS_BEGIN

namespace Data::GameData {
class ClimateService;
}

namespace UI {

/**
 * @class ClimateWindow
 * @brief A window that allows players to control weather, climate, and sun profiles.
 */
class ClimateWindow : public BaseWindow {
 public:
  ClimateWindow(const std::string& componentName, const std::string& windowId, Data::GameData::ClimateService& climateService);
  virtual ~ClimateWindow() = default;

  void RenderContent() override;
  const char* GetWindowTitle() const override;

 private:
  Data::GameData::ClimateService& m_climateService;

  // Localization keys
  std::string m_locTitle;
  std::string m_locNotReady;
};

} // namespace UI
SPF_NS_END
