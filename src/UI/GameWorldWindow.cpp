#include "SPF/UI/GameWorldWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/GameConsole/GameConsole.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "imgui.h"

#include <cstdint>
#include <cstdio>
#include <string>


SPF_NS_BEGIN
namespace UI {
using namespace Localization;

GameWorldWindow::GameWorldWindow(const std::string& componentName, const std::string& windowId, Data::GameData::GameWorldService& worldService) : BaseWindow(componentName, windowId), m_worldService(worldService) {
  m_titleLocalizationKey = "gameworld_window.title";
  RefreshLocalization();
}

void GameWorldWindow::RefreshLocalization() {
  BaseWindow::RefreshLocalization();
  auto& loc = LocalizationManager::GetInstance();
  m_locNotReady = loc.Get("gameworld_window.not_ready");
  m_locVisualTimeTitle = loc.Get("gameworld_window.visual_time.title");
  m_locVisualTimeLock = loc.Get("gameworld_window.visual_time.lock_checkbox");
  m_locVisualTimeLockTooltip = loc.Get("gameworld_window.visual_time.lock_tooltip");
  m_locVisualTimeSlider = loc.Get("gameworld_window.visual_time.slider_label");
  m_locVisualTimeDisabledHint = loc.Get("gameworld_window.visual_time.slider_disabled_hint");
  m_locSimTimeTitle = loc.Get("gameworld_window.simulation_time.title");
  m_locSimTimeState = loc.Get("gameworld_window.simulation_time.state_label");
  m_locSimTimeClock = loc.Get("gameworld_window.simulation_time.world_clock");
  m_locSimTimeSlider = loc.Get("gameworld_window.simulation_time.slider_label");
  m_locSimTimeMinusDay = loc.Get("gameworld_window.simulation_time.minus_day");
  m_locSimTimePlusDay = loc.Get("gameworld_window.simulation_time.plus_day");
  m_locSimTimeReset = loc.Get("gameworld_window.simulation_time.reset_midnight");
  m_locEngineInfoTitle = loc.Get("gameworld_window.engine_info.title");
  m_locEngineMapScale = loc.Get("gameworld_window.engine_info.map_scale");
  m_locEngineMapScaleTooltip = loc.Get("gameworld_window.engine_info.map_scale_tooltip");
  m_locEnginePlaytime = loc.Get("gameworld_window.engine_info.playtime");
  m_locEngineControlsTitle = loc.Get("gameworld_window.engine_controls.title");
  m_locEngineWarp = loc.Get("gameworld_window.engine_controls.global_warp");
  m_locEngineWarpTooltip = loc.Get("gameworld_window.engine_controls.warp_tooltip");
  m_locEnginePauseStatus = loc.Get("gameworld_window.engine_controls.pause_status");
  m_locEnginePauseCheckbox = loc.Get("gameworld_window.engine_controls.pause_checkbox");
  m_locEnginePauseTooltip = loc.Get("gameworld_window.engine_controls.pause_tooltip");
  m_locEngineFrameCounter = loc.Get("gameworld_window.engine_controls.frame_counter");
  m_locEngineDeltaTime = loc.Get("gameworld_window.engine_controls.delta_time");
  m_locCitiesTitle = loc.Get("gameworld_window.cities.title");
  m_locCitiesLabel = loc.Get("gameworld_window.cities.city_label");
  m_locCitiesNone = loc.Get("gameworld_window.cities.none");
  m_locCitiesNotSelected = loc.Get("gameworld_window.cities.not_selected");
  m_locCitiesCoordinates = loc.Get("gameworld_window.cities.coordinates");
  m_locCitiesName = loc.Get("gameworld_window.cities.name");
  m_locCitiesCoordX = loc.Get("gameworld_window.cities.coord_x");
  m_locCitiesCoordY = loc.Get("gameworld_window.cities.coord_y");
  m_locCitiesCoordZ = loc.Get("gameworld_window.cities.coord_z");
  m_locCitiesUid = loc.Get("gameworld_window.cities.uid");
  m_locCitiesPointCount = loc.Get("gameworld_window.cities.point_count");
  m_locCitiesItemScale = loc.Get("gameworld_window.cities.item_scale");
  m_locCitiesItemRadius = loc.Get("gameworld_window.cities.item_radius");
  m_locCitiesGroup = loc.Get("gameworld_window.cities.group");
  m_locCitiesPinScaleFactor = loc.Get("gameworld_window.cities.pin_scale_factor");
  m_locCitiesMapOffset = loc.Get("gameworld_window.cities.map_offset");
  m_locCitiesMapXOffset = loc.Get("gameworld_window.cities.map_x_offset");
  m_locCitiesMapYOffset = loc.Get("gameworld_window.cities.map_y_offset");
  m_locCitiesPriceCoef = loc.Get("gameworld_window.cities.price_coef");
  m_locCitiesCountry = loc.Get("gameworld_window.cities.country");
  m_locCitiesPopulation = loc.Get("gameworld_window.cities.population");
  m_locCitiesKeyCity = loc.Get("gameworld_window.cities.key_city");
  m_locCitiesTimeZone = loc.Get("gameworld_window.cities.time_zone");
  m_locCitiesNameLocalized = loc.Get("gameworld_window.cities.name_localized");
  m_locCitiesShortName = loc.Get("gameworld_window.cities.short_name");
  m_locCitiesShortNameLocalized = loc.Get("gameworld_window.cities.short_name_localized");
  m_locCitiesGotoButton = loc.Get("gameworld_window.cities.goto_button");
  m_locCitiesGotoTooltip = loc.Get("gameworld_window.cities.goto_tooltip");
  m_locDays[0] = loc.Get("gameworld_window.days.monday");
  m_locDays[1] = loc.Get("gameworld_window.days.tuesday");
  m_locDays[2] = loc.Get("gameworld_window.days.wednesday");
  m_locDays[3] = loc.Get("gameworld_window.days.thursday");
  m_locDays[4] = loc.Get("gameworld_window.days.friday");
  m_locDays[5] = loc.Get("gameworld_window.days.saturday");
  m_locDays[6] = loc.Get("gameworld_window.days.sunday");
}

void GameWorldWindow::RefreshCityList() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("GameWorldWindow");

  m_cityNames.clear();
  m_cityItems.clear();

  uint32_t count = m_worldService.GetCityCount();
  logger->Debug("RefreshCityList: GetCityCount() = {}.", count);
  m_cityNames.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    char nameBuffer[128];
    if (m_worldService.GetCityName(i, nameBuffer, sizeof(nameBuffer)) < 0) continue;
    m_cityNames.emplace_back(nameBuffer);
  }
  logger->Debug("RefreshCityList: resolved {} of {} city names.", m_cityNames.size(), count);

  // First combo entry is always the "no selection" placeholder.
  m_cityItems.reserve(m_cityNames.size() + 1);
  m_cityItems.push_back(m_locCitiesNone.c_str());
  for (const auto& name : m_cityNames) {
    m_cityItems.push_back(name.c_str());
  }

  m_cityCount = count;
  if (m_selectedCity >= (int)m_cityNames.size()) m_selectedCity = -1;
  m_cityEditLoaded = false;
}

void GameWorldWindow::LoadCityEditState() {
  m_cityEdit = CityEditState();
  if (m_selectedCity < 0 || m_selectedCity >= (int)m_cityNames.size()) return;

  uint32_t idx = (uint32_t)m_selectedCity;
  m_cityEdit.uid = m_worldService.GetCityUid(idx);

  float x = 0.0f, y = 0.0f, z = 0.0f;
  if (m_worldService.GetCityPosition(m_cityEdit.uid, &x, &y, &z)) {
    m_cityEdit.pos[0] = x;
    m_cityEdit.pos[1] = y;
    m_cityEdit.pos[2] = z;
  }
  m_cityEdit.itemScale = m_worldService.GetCityItemScale(idx);
  m_cityEdit.itemRadius = m_worldService.GetCityItemRadius(idx);
  m_cityEdit.group = m_worldService.GetCityGroup(idx);
  m_cityEdit.pinScale = m_worldService.GetCityPinScaleFactor(idx);
  m_worldService.GetCityMapXOffsets(idx, m_cityEdit.mapXOffsets, 8);
  m_worldService.GetCityMapYOffsets(idx, m_cityEdit.mapYOffsets, 8);
  m_cityEdit.priceCoef = m_worldService.GetCityPriceCoef(idx);
  m_cityEdit.country = m_worldService.GetCityCountry(idx);
  m_cityEdit.population = m_worldService.GetCityPopulation(idx);
  m_cityEdit.keyCity = m_worldService.GetCityKeyCity(idx);
  m_cityEdit.timeZone = m_worldService.GetCityTimeZone(idx);

  m_cityEditLoaded = true;
}

void GameWorldWindow::RenderContent() {
  if (!m_worldService.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", m_locNotReady.c_str());
    return;
  }

  // --- Visual Preview Time Section ---
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", m_locVisualTimeTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  static bool skyboxLock = false;
  if (ImGui::Checkbox(m_locVisualTimeLock.c_str(), &skyboxLock)) {
    m_worldService.SetSkyboxAutoUpdate(!skyboxLock);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locVisualTimeLockTooltip.c_str());
  }

  // Preview Slider logic
  int currentVisualMins = (int)m_worldService.GetPreviewTime();
  int sliderVisualMins = currentVisualMins % 1440;

  char vTimeBuffer[16];
  snprintf(vTimeBuffer, sizeof(vTimeBuffer), "%02d:%02d", sliderVisualMins / 60, sliderVisualMins % 60);

  // Disable slider if skybox is not locked
  if (!skyboxLock) {
    ImGui::BeginDisabled();
  }
  if (ImGui::SliderInt(m_locVisualTimeSlider.c_str(), &sliderVisualMins, 0, 1439, vTimeBuffer)) {
    // Keep the same day, just change the time
    uint32_t currentDayStart = (currentVisualMins / 1440) * 1440;
    m_worldService.SetPreviewTime(currentDayStart + (uint32_t)sliderVisualMins);
  }
  if (!skyboxLock) {
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", m_locVisualTimeDisabledHint.c_str());
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Real Game Simulation Time ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", m_locSimTimeTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  uint32_t simTotalMinutes = m_worldService.GetSimulationTime();
  uint32_t simDays = m_worldService.GetGameDay();
  uint32_t simWeek = m_worldService.GetGameWeek();
  uint32_t dayOfWeek = m_worldService.GetDayOfWeek();

  int simHours = (simTotalMinutes % 1440) / 60;
  int simMins = simTotalMinutes % 60;

  int sliderSimMins = simTotalMinutes % 1440;
  char sTimeBuffer[16];
  snprintf(sTimeBuffer, sizeof(sTimeBuffer), "%02d:%02d", simHours, simMins);

  Typography::Text(TextStyle::Regular(), m_locSimTimeState.c_str(), simWeek + 1, m_locDays[dayOfWeek].c_str(), simDays + 1);

  Typography::Text(TextStyle::Regular(), m_locSimTimeClock.c_str(), sTimeBuffer);

  if (ImGui::SliderInt(m_locSimTimeSlider.c_str(), &sliderSimMins, 0, 1439, sTimeBuffer)) {
    uint32_t dayStart = (uint32_t)(simDays * 1440);
    m_worldService.SetSimulationTime(dayStart + (uint32_t)sliderSimMins);
  }

  if (ImGui::Button(m_locSimTimeMinusDay.c_str())) {
    if (simTotalMinutes >= 1440) m_worldService.SetSimulationTime(simTotalMinutes - 1440);
  }
  ImGui::SameLine();
  if (ImGui::Button(m_locSimTimePlusDay.c_str())) {
    m_worldService.SetSimulationTime(simTotalMinutes + 1440);
  }
  ImGui::SameLine();
  if (ImGui::Button(m_locSimTimeReset.c_str())) {
    m_worldService.SetSimulationTime((uint32_t)(simDays * 1440));
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Statistics and Scale ---
  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", m_locEngineInfoTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  float mapScale = m_worldService.GetMapScale();
  Typography::Text(TextStyle::Regular(), m_locEngineMapScale.c_str(), mapScale);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locEngineMapScaleTooltip.c_str());
  }

  uint32_t realPlayMins = m_worldService.GetRealPlayTime();
  int playHours = realPlayMins / 60;
  int playMins = realPlayMins % 60;
  Typography::Text(TextStyle::Regular(), m_locEnginePlaytime.c_str(), playHours, playMins);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Core Engine Control Section ---
  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", m_locEngineControlsTitle.c_str());

  float globalWarp = m_worldService.GetGlobalWarp();
  if (ImGui::SliderFloat(m_locEngineWarp.c_str(), &globalWarp, 0.0f, 10.0f, "%.2f")) {
    m_worldService.SetGlobalWarp(globalWarp);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locEngineWarpTooltip.c_str());
  }

  // --- Engine Pause / Halt Section ---
  bool isPaused = m_worldService.IsGamePaused();
  Typography::Text(TextStyle::Regular().Color(isPaused ? Colors::RED : Colors::GREEN), m_locEnginePauseStatus.c_str(), isPaused ? "TRUE" : "FALSE");

  if (ImGui::Checkbox(m_locEnginePauseCheckbox.c_str(), &isPaused)) {
    m_worldService.SetGamePaused(isPaused);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", m_locEnginePauseTooltip.c_str());
  }

  ImGui::Spacing();

  double deltaTime = m_worldService.GetRealDeltaTime();
  Typography::Text(TextStyle::Regular(), m_locEngineDeltaTime.c_str(), (float)deltaTime);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Cities Section ---
  Typography::Text(TextStyle::H3().Color(Colors::GREEN), "%s", m_locCitiesTitle.c_str());
  ImGui::Separator();
  ImGui::Spacing();

  if (m_cityCount != m_worldService.GetCityCount()) {
    RefreshCityList();
  }

  if (m_cityNames.empty()) {
    Typography::Text(TextStyle::Regular(), "%s", m_locCitiesNone.c_str());
  } else {
    // First combo entry is the "no selection" placeholder.
    int comboIndex = m_selectedCity + 1;
    if (ImGui::Combo(m_locCitiesLabel.c_str(), &comboIndex, m_cityItems.data(), (int)m_cityItems.size())) {
      m_selectedCity = comboIndex - 1;
      m_cityEditLoaded = false;
    }
    const float cityComboWidth = ImGui::GetItemRectSize().x;

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (m_selectedCity < 0 || m_selectedCity >= (int)m_cityNames.size()) {
      Typography::Text(TextStyle::Regular().Color(Colors::GRAY), "%s", m_locCitiesNotSelected.c_str());
    } else {
      if (!m_cityEditLoaded) LoadCityEditState();
      const uint32_t cityIdx = (uint32_t)m_selectedCity;

      if (ImGui::BeginTable("cities_table", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX)) {
        ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, cityComboWidth * 0.5f);
        ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthFixed, cityComboWidth * 0.5f);

        auto rowLabel = [&](const std::string& label) {
          ImGui::TableNextRow();
          ImGui::TableSetColumnIndex(0);
          Typography::Text(TextStyle::Regular(), "%s", label.c_str());
          ImGui::TableSetColumnIndex(1);
        };

        // Centers the next widget at 75% width of the current column.
        auto setSliderWidth = [&]() {
          float avail = ImGui::GetContentRegionAvail().x;
          float width = avail * 0.75f;
          ImGui::SetNextItemWidth(width);
          if (avail > width) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - width) * 0.5f);
          }
        };

        // Centers read-only text values in the current (value) column.
        auto centerValue = [&](const std::string& value) {
          float avail = ImGui::GetContentRegionAvail().x;
          float textWidth = ImGui::CalcTextSize(value.c_str()).x;
          if (avail > textWidth) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail - textWidth) * 0.5f);
          }
          Typography::Text(TextStyle::Regular(), "%s", value.c_str());
        };

        // --- Name (read-only) ---
        m_scratchLen = m_worldService.GetCityName(cityIdx, m_scratchBuf, sizeof(m_scratchBuf));
        rowLabel(m_locCitiesName);
        centerValue(m_scratchLen >= 0 ? m_scratchBuf : "-");

        // "goto" test button: executes the game console command `goto <city name>`.
        ImGui::SameLine();
        if (ImGui::Button(m_locCitiesGotoButton.c_str())) {
          if (m_scratchLen >= 0) {
            std::string cityName(m_scratchBuf, (size_t)m_scratchLen);
            GameConsole::GetInstance().Execute("goto " + cityName);
          }
        }
        if (ImGui::IsItemHovered() && m_scratchLen >= 0) {
          std::string cityName(m_scratchBuf, (size_t)m_scratchLen);
          char tooltipBuf[512];
          snprintf(tooltipBuf, sizeof(tooltipBuf), m_locCitiesGotoTooltip.c_str(), cityName.c_str());
          ImGui::SetTooltip("%s", tooltipBuf);
        }

        // --- UID (read-only) ---
        char uidStr[32];
        snprintf(uidStr, sizeof(uidStr), "%u", m_cityEdit.uid);
        rowLabel(m_locCitiesUid);
        centerValue(uidStr);

        // --- Coordinates (label shows live values; three stacked sliders on the right) ---
        char coordLabel[128];
        snprintf(coordLabel, sizeof(coordLabel), "%s X: %.1f Y: %.1f Z: %.1f", m_locCitiesCoordinates.c_str(), m_cityEdit.pos[0], m_cityEdit.pos[1], m_cityEdit.pos[2]);
        rowLabel(coordLabel);
        for (int axis = 0; axis < 3; ++axis) {
          const char* axisLabel = axis == 0 ? m_locCitiesCoordX.c_str() : (axis == 1 ? m_locCitiesCoordY.c_str() : m_locCitiesCoordZ.c_str());
          setSliderWidth();
          if (ImGui::SliderFloat(axisLabel, &m_cityEdit.pos[axis], -500000.0f, 500000.0f, "%.2f")) {
            m_worldService.SetCityPosition(m_cityEdit.uid, m_cityEdit.pos[0], m_cityEdit.pos[1], m_cityEdit.pos[2]);
          }
        }

        // --- Name (Localized) (read-only) ---
        m_scratchLen = m_worldService.GetCityNameLocalized(cityIdx, m_scratchBuf, sizeof(m_scratchBuf));
        rowLabel(m_locCitiesNameLocalized);
        centerValue(m_scratchLen >= 0 ? m_scratchBuf : "-");

        // --- Short Name (read-only) ---
        m_scratchLen = m_worldService.GetCityShortName(cityIdx, m_scratchBuf, sizeof(m_scratchBuf));
        rowLabel(m_locCitiesShortName);
        centerValue(m_scratchLen >= 0 ? m_scratchBuf : "-");

        // --- Short Name (Localized) (read-only) ---
        m_scratchLen = m_worldService.GetCityShortNameLocalized(cityIdx, m_scratchBuf, sizeof(m_scratchBuf));
        rowLabel(m_locCitiesShortNameLocalized);
        centerValue(m_scratchLen >= 0 ? m_scratchBuf : "-");

        // --- Population ---
        rowLabel(m_locCitiesPopulation);
        int populationVal = (int)m_cityEdit.population;
        setSliderWidth();
        if (ImGui::InputInt("##population", &populationVal, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivated()) {
          if (m_worldService.SetCityPopulation(cityIdx, (uint32_t)populationVal)) m_cityEdit.population = m_worldService.GetCityPopulation(cityIdx);
        }

        // --- Point Count (read-only) ---
        rowLabel(m_locCitiesPointCount);
        Typography::Text(TextStyle::Regular(), "%u", m_worldService.GetCityPointCount(cityIdx));

        // --- Item Scale ---
        rowLabel(m_locCitiesItemScale);
        setSliderWidth();
        if (ImGui::SliderFloat("##item_scale", &m_cityEdit.itemScale, -1000.0f, 1000.0f, "%.4f")) {
          if (m_worldService.SetCityItemScale(cityIdx, m_cityEdit.itemScale)) m_cityEdit.itemScale = m_worldService.GetCityItemScale(cityIdx);
        }

        // --- Item Radius ---
        rowLabel(m_locCitiesItemRadius);
        setSliderWidth();
        if (ImGui::SliderFloat("##item_radius", &m_cityEdit.itemRadius, -1000.0f, 1000.0f, "%.4f")) {
          if (m_worldService.SetCityItemRadius(cityIdx, m_cityEdit.itemRadius)) m_cityEdit.itemRadius = m_worldService.GetCityItemRadius(cityIdx);
        }

        // --- City Group ---
        rowLabel(m_locCitiesGroup);
        int groupVal = (int)m_cityEdit.group;
        setSliderWidth();
        if (ImGui::SliderInt("##group", &groupVal, 0, 100000, "%d")) {
          if (m_worldService.SetCityGroup(cityIdx, (uint32_t)groupVal)) m_cityEdit.group = (uint32_t)groupVal;
        }

        // --- Pin Scale Factor ---
        rowLabel(m_locCitiesPinScaleFactor);
        setSliderWidth();
        if (ImGui::SliderFloat("##pin_scale", &m_cityEdit.pinScale, -1000.0f, 1000.0f, "%.4f")) {
          if (m_worldService.SetCityPinScaleFactor(cityIdx, m_cityEdit.pinScale)) m_cityEdit.pinScale = m_worldService.GetCityPinScaleFactor(cityIdx);
        }

        // --- Map Offset: zoom-index dropdown, then X/Y sliders for the selected position ---
        rowLabel(m_locCitiesMapOffset);
        {
          char idxBuf[64];
          snprintf(idxBuf, sizeof(idxBuf), "%s[%d]", m_locCitiesMapOffset.c_str(), m_cityEdit.mapOffsetIndex);
          setSliderWidth();
          if (ImGui::BeginCombo("##map_index", idxBuf)) {
            for (int i = 0; i < 8; ++i) {
              char itemBuf[64];
              snprintf(itemBuf, sizeof(itemBuf), "%s[%d]", m_locCitiesMapOffset.c_str(), i);
              const bool isSelected = (m_cityEdit.mapOffsetIndex == i);
              if (ImGui::Selectable(itemBuf, isSelected)) m_cityEdit.mapOffsetIndex = i;
              if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
          }

          setSliderWidth();
          if (ImGui::SliderFloat(m_locCitiesMapXOffset.c_str(), &m_cityEdit.mapXOffsets[m_cityEdit.mapOffsetIndex], -1000.0f, 1000.0f, "%.3f")) {
            if (m_worldService.SetCityMapXOffsets(cityIdx, m_cityEdit.mapXOffsets, 8)) {
              m_worldService.GetCityMapXOffsets(cityIdx, m_cityEdit.mapXOffsets, 8);
            }
          }

          setSliderWidth();
          if (ImGui::SliderFloat(m_locCitiesMapYOffset.c_str(), &m_cityEdit.mapYOffsets[m_cityEdit.mapOffsetIndex], -1000.0f, 1000.0f, "%.3f")) {
            if (m_worldService.SetCityMapYOffsets(cityIdx, m_cityEdit.mapYOffsets, 8)) {
              m_worldService.GetCityMapYOffsets(cityIdx, m_cityEdit.mapYOffsets, 8);
            }
          }
        }

        // --- Price Coef ---
        rowLabel(m_locCitiesPriceCoef);
        setSliderWidth();
        if (ImGui::SliderFloat("##price_coef", &m_cityEdit.priceCoef, -100.0f, 100.0f, "%.4f")) {
          if (m_worldService.SetCityPriceCoef(cityIdx, m_cityEdit.priceCoef)) m_cityEdit.priceCoef = m_worldService.GetCityPriceCoef(cityIdx);
        }

        // --- Country ---
        rowLabel(m_locCitiesCountry);
        int countryVal = (int)m_cityEdit.country;
        setSliderWidth();
        if (ImGui::InputInt("##country", &countryVal, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivated()) {
          if (m_worldService.SetCityCountry(cityIdx, (uint32_t)countryVal)) m_cityEdit.country = m_worldService.GetCityCountry(cityIdx);
        }

        // --- Key City (checkbox) ---
        rowLabel(m_locCitiesKeyCity);
        if (ImGui::Checkbox("##key_city", &m_cityEdit.keyCity)) {
          if (m_worldService.SetCityKeyCity(cityIdx, m_cityEdit.keyCity)) m_cityEdit.keyCity = m_worldService.GetCityKeyCity(cityIdx);
        }

        // --- Time Zone ---
        rowLabel(m_locCitiesTimeZone);
        int timeZoneVal = (int)m_cityEdit.timeZone;
        setSliderWidth();
        if (ImGui::InputInt("##time_zone", &timeZoneVal, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::IsItemDeactivated()) {
          if (m_worldService.SetCityTimeZone(cityIdx, (uint32_t)timeZoneVal)) m_cityEdit.timeZone = m_worldService.GetCityTimeZone(cityIdx);
        }

        ImGui::EndTable();
      }

      ImGui::Spacing();
    }
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();
}

}  // namespace UI
SPF_NS_END
