#pragma once

#include "SPF/Data/GameData/SoundService.hpp"
#include "SPF/UI/BaseWindow.hpp"

#include <string>
#include <vector>

namespace SPF::UI {

class SoundWindow : public BaseWindow {
 public:
  SoundWindow(const std::string& componentName, const std::string& windowId, Data::GameData::SoundService& soundService);
  virtual ~SoundWindow() = default;

 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  Data::GameData::SoundService& m_soundService;

  // Localization strings
  std::string m_locNotReady;
  std::string m_locBankComboLabel;
  std::string m_locEventComboLabel;
  std::string m_locNone;
  std::string m_locBanksFound;
  std::string m_locEventsFound;
  std::string m_locBankInfoTitle;
  std::string m_locEventInfoTitle;
  std::string m_locBankPath;
  std::string m_locEventCount;
  std::string m_locEventPath;
  std::string m_locGUID;
  std::string m_locRefresh;
  std::string m_locNoSelection;

  // Grouped sound data
  std::vector<Data::GameData::SoundBankGroup> m_banks;
  std::vector<const char*> m_bankComboItems;
  std::vector<const char*> m_eventComboItems;
  int m_selectedBank = -1;
  int m_selectedEvent = -1;
  bool m_listInitialized = false;

  void RefreshSoundList();
};

}  // namespace SPF::UI
