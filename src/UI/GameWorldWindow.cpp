#include "SPF/UI/GameWorldWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
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
  m_locDays[0] = loc.Get("gameworld_window.days.monday");
  m_locDays[1] = loc.Get("gameworld_window.days.tuesday");
  m_locDays[2] = loc.Get("gameworld_window.days.wednesday");
  m_locDays[3] = loc.Get("gameworld_window.days.thursday");
  m_locDays[4] = loc.Get("gameworld_window.days.friday");
  m_locDays[5] = loc.Get("gameworld_window.days.saturday");
  m_locDays[6] = loc.Get("gameworld_window.days.sunday");
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

  ImGui::Separator();
}

}  // namespace UI
SPF_NS_END
