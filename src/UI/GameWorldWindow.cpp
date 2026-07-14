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
  // Localization Keys
  m_locTitle = "gameworld_window.title";
  m_locNotReady = "gameworld_window.not_ready";

  // Visual Time
  m_locVisualTimeTitle = "gameworld_window.visual_time.title";
  m_locVisualTimeLock = "gameworld_window.visual_time.lock_checkbox";
  m_locVisualTimeLockTooltip = "gameworld_window.visual_time.lock_tooltip";
  m_locVisualTimeSlider = "gameworld_window.visual_time.slider_label";
  m_locVisualTimeDisabledHint = "gameworld_window.visual_time.slider_disabled_hint";

  // Simulation Time
  m_locSimTimeTitle = "gameworld_window.simulation_time.title";
  m_locSimTimeState = "gameworld_window.simulation_time.state_label";
  m_locSimTimeClock = "gameworld_window.simulation_time.world_clock";
  m_locSimTimeSlider = "gameworld_window.simulation_time.slider_label";
  m_locSimTimeMinusDay = "gameworld_window.simulation_time.minus_day";
  m_locSimTimePlusDay = "gameworld_window.simulation_time.plus_day";
  m_locSimTimeReset = "gameworld_window.simulation_time.reset_midnight";

  // Engine Info
  m_locEngineInfoTitle = "gameworld_window.engine_info.title";
  m_locEngineMapScale = "gameworld_window.engine_info.map_scale";
  m_locEngineMapScaleTooltip = "gameworld_window.engine_info.map_scale_tooltip";
  m_locEnginePlaytime = "gameworld_window.engine_info.playtime";

  // Engine Controls
  m_locEngineControlsTitle = "gameworld_window.engine_controls.title";
  m_locEngineWarp = "gameworld_window.engine_controls.global_warp";
  m_locEngineWarpTooltip = "gameworld_window.engine_controls.warp_tooltip";
  m_locEnginePauseStatus = "gameworld_window.engine_controls.pause_status";
  m_locEnginePauseCheckbox = "gameworld_window.engine_controls.pause_checkbox";
  m_locEnginePauseTooltip = "gameworld_window.engine_controls.pause_tooltip";
  m_locEngineFrameCounter = "gameworld_window.engine_controls.frame_counter";
  m_locEngineDeltaTime = "gameworld_window.engine_controls.delta_time";

  // Days of Week
  m_locDays[0] = "gameworld_window.days.monday";
  m_locDays[1] = "gameworld_window.days.tuesday";
  m_locDays[2] = "gameworld_window.days.wednesday";
  m_locDays[3] = "gameworld_window.days.thursday";
  m_locDays[4] = "gameworld_window.days.friday";
  m_locDays[5] = "gameworld_window.days.saturday";
  m_locDays[6] = "gameworld_window.days.sunday";
}

const char* GameWorldWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_locTitle).c_str(); }

void GameWorldWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();

  if (!m_worldService.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", loc.Get(m_locNotReady).c_str());
    return;
  }

  // --- Visual Preview Time Section ---
  ImGui::Spacing();
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", loc.Get(m_locVisualTimeTitle).c_str());
  ImGui::Separator();
  ImGui::Spacing();

  static bool skyboxLock = false;
  if (ImGui::Checkbox(loc.Get(m_locVisualTimeLock).c_str(), &skyboxLock)) {
    m_worldService.SetSkyboxAutoUpdate(!skyboxLock);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", loc.Get(m_locVisualTimeLockTooltip).c_str());
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
  if (ImGui::SliderInt(loc.Get(m_locVisualTimeSlider).c_str(), &sliderVisualMins, 0, 1439, vTimeBuffer)) {
    // Keep the same day, just change the time
    uint32_t currentDayStart = (currentVisualMins / 1440) * 1440;
    m_worldService.SetPreviewTime(currentDayStart + (uint32_t)sliderVisualMins);
  }
  if (!skyboxLock) {
    ImGui::EndDisabled();
    ImGui::TextDisabled("%s", loc.Get(m_locVisualTimeDisabledHint).c_str());
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Real Game Simulation Time ---
  Typography::Text(TextStyle::H3().Color(Colors::CYAN), "%s", loc.Get(m_locSimTimeTitle).c_str());
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

  Typography::Text(TextStyle::Regular(), loc.Get(m_locSimTimeState).c_str(), simWeek + 1, loc.Get(m_locDays[dayOfWeek]).c_str(), simDays + 1);

  Typography::Text(TextStyle::Regular(), loc.Get(m_locSimTimeClock).c_str(), sTimeBuffer);

  if (ImGui::SliderInt(loc.Get(m_locSimTimeSlider).c_str(), &sliderSimMins, 0, 1439, sTimeBuffer)) {
    uint32_t dayStart = (uint32_t)(simDays * 1440);
    m_worldService.SetSimulationTime(dayStart + (uint32_t)sliderSimMins);
  }

  if (ImGui::Button(loc.Get(m_locSimTimeMinusDay).c_str())) {
    if (simTotalMinutes >= 1440) m_worldService.SetSimulationTime(simTotalMinutes - 1440);
  }
  ImGui::SameLine();
  if (ImGui::Button(loc.Get(m_locSimTimePlusDay).c_str())) {
    m_worldService.SetSimulationTime(simTotalMinutes + 1440);
  }
  ImGui::SameLine();
  if (ImGui::Button(loc.Get(m_locSimTimeReset).c_str())) {
    m_worldService.SetSimulationTime((uint32_t)(simDays * 1440));
  }

  ImGui::Spacing();
  ImGui::Spacing();

  // --- Statistics and Scale ---
  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", loc.Get(m_locEngineInfoTitle).c_str());
  ImGui::Separator();
  ImGui::Spacing();

  float mapScale = m_worldService.GetMapScale();
  Typography::Text(TextStyle::Regular(), loc.Get(m_locEngineMapScale).c_str(), mapScale);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", loc.Get(m_locEngineMapScaleTooltip).c_str());
  }

  uint32_t realPlayMins = m_worldService.GetRealPlayTime();
  int playHours = realPlayMins / 60;
  int playMins = realPlayMins % 60;
  Typography::Text(TextStyle::Regular(), loc.Get(m_locEnginePlaytime).c_str(), playHours, playMins);

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // --- Core Engine Control Section ---
  Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "%s", loc.Get(m_locEngineControlsTitle).c_str());

  float globalWarp = m_worldService.GetGlobalWarp();
  if (ImGui::SliderFloat(loc.Get(m_locEngineWarp).c_str(), &globalWarp, 0.0f, 10.0f, "%.2f")) {
    m_worldService.SetGlobalWarp(globalWarp);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", loc.Get(m_locEngineWarpTooltip).c_str());
  }

  // --- Engine Pause / Halt Section ---
  bool isPaused = m_worldService.IsGamePaused();
  Typography::Text(TextStyle::Regular().Color(isPaused ? Colors::RED : Colors::GREEN), loc.Get(m_locEnginePauseStatus).c_str(), isPaused ? "TRUE" : "FALSE");

  if (ImGui::Checkbox(loc.Get(m_locEnginePauseCheckbox).c_str(), &isPaused)) {
    m_worldService.SetGamePaused(isPaused);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("%s", loc.Get(m_locEnginePauseTooltip).c_str());
  }

  ImGui::Spacing();

  double deltaTime = m_worldService.GetRealDeltaTime();
  Typography::Text(TextStyle::Regular(), loc.Get(m_locEngineDeltaTime).c_str(), (float)deltaTime);

  ImGui::Separator();
}

}  // namespace UI
SPF_NS_END
