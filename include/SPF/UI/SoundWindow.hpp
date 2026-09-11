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
  std::string m_locDuration;
  std::string m_locIs3D;
  std::string m_locIsLooping;
  std::string m_locIsOneshot;
  std::string m_locIsStream;
  std::string m_locHasSustainPoint;
  std::string m_locYes;
  std::string m_locNo;
  std::string m_locNA;
  std::string m_locDumpToLog;
  std::string m_locParameters;
  std::string m_locParamName;
  std::string m_locParamUnits;
  std::string m_locParamMin;
  std::string m_locParamMax;
  std::string m_locParamDefault;
  std::string m_locBuses;
  std::string m_locBusPath;
  std::string m_locGlobalParameters;
  std::string m_locGlobalParamPath;
  std::string m_locPlay;
  std::string m_locStop;
  std::string m_locPlayback;
  std::string m_locNoParameters;
  std::string m_locVolume;
  std::string m_locMute;
  std::string m_locUnmute;
  std::string m_locCurrentValue;
  std::string m_locPause;
  std::string m_locResume;
  std::string m_locAutoLoop;
  std::string m_locValueChanged;
  std::string m_locSelectParam;
  std::string m_locVCAs;
  std::string m_locVCAPath;
  std::string m_locSelectVCA;
  std::string m_locListener;
  std::string m_locNumListeners;
  std::string m_locPosition;
  std::string m_locVelocity;
  std::string m_locForward;
  std::string m_locUp;
  std::string m_locSearchFilter;
  std::string m_locPitch;
  std::string m_locTimelinePosition;
  std::string m_locInstances;
  std::string m_locInstanceCount;
  std::string m_locIsSnapshot;
  std::string m_locIsDoppler;
  std::string m_locMinMaxDistance;
  std::string m_locSoundSize;
  std::string m_locSampleState;
  std::string m_locListenerLabel;
  std::string m_locColBus;
  std::string m_locColParameter;
  std::string m_locColType;
  std::string m_locColRange;
  std::string m_locColDefault;
  std::string m_locColValue;
  std::string m_locTypeUser;
  std::string m_locTypeAuto;
  std::string m_locColFader;
  std::string m_locColBypass;
  std::string m_locUserProperties;
  std::string m_locActiveInstances;
  std::string m_locAttach;
  std::string m_locAttached;
  std::string m_locDetach;
  std::string m_locResetToGame;
  std::string m_locCreateNewInstance;
  std::string m_locLoadingState;
  std::string m_locSampleLoadingState;
  std::string m_locBankBusCount;
  std::string m_locBankVcaCount;
  std::string m_locPropBoolean;
  std::string m_locPropInteger;
  std::string m_locPropFloat;
  std::string m_locPropString;

  enum class Tab : int { Events = 0, Buses, VCAs, GlobalParams, Listener, COUNT };
  Tab m_activeTab = Tab::Events;

  std::vector<Data::GameData::SoundBankGroup> m_banks;
  std::vector<const char*> m_bankComboItems;
  std::vector<const char*> m_eventComboItems;
  std::vector<Data::GameData::SoundBusEntry> m_buses;
  std::vector<Data::GameData::SoundGlobalParameter> m_globalParams;
  std::vector<Data::GameData::SoundVCAEntry> m_vcas;
  std::vector<const char*> m_paramComboItems;
  std::vector<const char*> m_vcaComboItems;
  std::vector<int> m_busSortOrder;
  std::vector<float> m_globalParamValues;
  std::vector<Data::GameData::SoundVCAInfo> m_vcaInfos;
  int m_selectedBank = -1;
  int m_selectedEvent = -1;
  int m_selectedBus = -1;
  int m_selectedParam = -1;
  int m_selectedVCA = -1;
  bool m_listInitialized = false;
  bool m_enriched = false;
  bool m_parametersEnriched = false;
  std::vector<Data::GameData::SoundBankLoadInfo> m_bankLoadInfos;
  void* m_activeInstance = nullptr;
  int m_playbackState = -1;
  bool m_autoLoop = false;
  char m_searchFilter[256] = {};
  char m_lastSearchFilter[256] = {};
  float m_cached3DPos[3] = {};
  float m_cached3DVel[3] = {};
  float m_cached3DFwd[3] = {};
  float m_cached3DUp[3] = {};
  bool m_has3DCache = false;
  bool m_ownsInstance = false;

  void RefreshSoundList();
  void RefreshBusAndParamLists();
  void RefreshVCAList();
  void RenderTabEvents();
  void RenderTabBuses();
  void RenderTabVCAs();
  void RenderTabGlobalParams();
  void RenderTabListener();
  void RenderInstanceControls(const std::string& eventPath, bool is3D);
  void RenderEventDetail(const Data::GameData::SoundEvent& ev);
};

}  // namespace SPF::UI
