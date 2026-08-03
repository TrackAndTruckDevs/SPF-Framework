/**
 * @file GameWorldWindow.hpp
 * @brief UI window for Game World manipulation (time, weather, skybox).
 */

#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/UI/BaseWindow.hpp"

#include <cstdint>
#include <string>
#include <vector>


SPF_NS_BEGIN
namespace Data::GameData {
class GameWorldService;
}

namespace UI {

/**
 * @class GameWorldWindow
 * @brief Implements the GameWorld tab for the framework overlay.
 */
class GameWorldWindow : public BaseWindow {
 public:
  GameWorldWindow(const std::string& componentName, const std::string& windowId, Data::GameData::GameWorldService& worldService);
  virtual ~GameWorldWindow() = default;


 protected:
  void RenderContent() override;
  void RefreshLocalization() override;

 private:
  Data::GameData::GameWorldService& m_worldService;

  // Localization keys
  std::string m_locNotReady;

  // Visual Time
  std::string m_locVisualTimeTitle;
  std::string m_locVisualTimeLock;
  std::string m_locVisualTimeLockTooltip;
  std::string m_locVisualTimeSlider;
  std::string m_locVisualTimeDisabledHint;

  // Simulation Time
  std::string m_locSimTimeTitle;
  std::string m_locSimTimeState;
  std::string m_locSimTimeClock;
  std::string m_locSimTimeSlider;
  std::string m_locSimTimeMinusDay;
  std::string m_locSimTimePlusDay;
  std::string m_locSimTimeReset;

  // Engine Info
  std::string m_locEngineInfoTitle;
  std::string m_locEngineMapScale;
  std::string m_locEngineMapScaleTooltip;
  std::string m_locEnginePlaytime;

  // Engine Controls
  std::string m_locEngineControlsTitle;
  std::string m_locEngineWarp;
  std::string m_locEngineWarpTooltip;
  std::string m_locEnginePauseStatus;
  std::string m_locEnginePauseCheckbox;
  std::string m_locEnginePauseTooltip;
  std::string m_locEngineFrameCounter;
  std::string m_locEngineDeltaTime;

  // Cities
  std::string m_locCitiesTitle;
  std::string m_locCitiesLabel;
  std::string m_locCitiesNone;
  std::string m_locCitiesNotSelected;
  std::string m_locCitiesCoordinates;
  std::string m_locCitiesName;
  std::string m_locCitiesCoordX;
  std::string m_locCitiesCoordY;
  std::string m_locCitiesCoordZ;
  std::string m_locCitiesUid;
  std::string m_locCitiesPointCount;
  std::string m_locCitiesItemScale;
  std::string m_locCitiesItemRadius;
  std::string m_locCitiesGroup;
  std::string m_locCitiesPinScaleFactor;
  std::string m_locCitiesMapOffset;
  std::string m_locCitiesMapXOffset;
  std::string m_locCitiesMapYOffset;
  std::string m_locCitiesPriceCoef;
  std::string m_locCitiesCountry;
  std::string m_locCitiesPopulation;
  std::string m_locCitiesKeyCity;
  std::string m_locCitiesTimeZone;
  std::string m_locCitiesNameLocalized;
  std::string m_locCitiesShortName;
  std::string m_locCitiesShortNameLocalized;
  std::string m_locCitiesGotoButton;
  std::string m_locCitiesGotoTooltip;

  // Days of week keys
  std::string m_locDays[7];

  // City list runtime state
  std::vector<std::string> m_cityNames;
  std::vector<const char*> m_cityItems;
  int m_selectedCity = -1;
  uint32_t m_cityCount = 0;

  // Scratch buffer for read-only string getters
  char m_scratchBuf[256];
  int m_scratchLen = -1;

  /**
   * @brief Editable state backing the city getter/setter table rows.
   * @details Filled from the getters when the selection changes; the apply
   *          buttons push edited values back through the setters.
   */
  struct CityEditState {
    uint32_t uid = 0;
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float itemScale = 0.0f;
    float itemRadius = 0.0f;
    uint32_t group = 0;
    float pinScale = 0.0f;
    int mapOffsetIndex = 0;  ///< Selected zoom index for the map offsets arrays.
    float mapXOffsets[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float mapYOffsets[8] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float priceCoef = 0.0f;
    uint32_t country = 0;
    uint32_t population = 0;
    bool keyCity = false;
    uint32_t timeZone = 0;
  };
  CityEditState m_cityEdit;
  bool m_cityEditLoaded = false;

  void RefreshCityList();
  void LoadCityEditState();
};

}  // namespace UI
SPF_NS_END
