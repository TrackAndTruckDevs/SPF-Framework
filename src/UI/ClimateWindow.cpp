#include "SPF/UI/ClimateWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/UI/BaseWindow.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"

#include "imgui.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

SPF_NS_BEGIN
namespace UI {
using namespace Localization;

ClimateWindow::ClimateWindow(const std::string& componentName, const std::string& windowId, Data::GameData::ClimateService& climateService) : BaseWindow(componentName, windowId), m_climateService(climateService) {
  // Localization Keys
  m_locTitle = "climate_window.title";
  m_locNotReady = "climate_window.not_ready";
}

const char* ClimateWindow::GetWindowTitle() const { return LocalizationManager::GetInstance().Get(m_locTitle).c_str(); }

void ClimateWindow::RenderContent() {
  auto& loc = LocalizationManager::GetInstance();

  if (!m_climateService.IsReady()) {
    Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", loc.Get(m_locNotReady).c_str());
    return;
  }

  // Weather & Climate UI logic will go here
  Typography::Text(TextStyle::H3().Color(Colors::GOLD), "Weather & Climate");
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Weather & Environment", ImGuiTreeNodeFlags_DefaultOpen)) {
    // --- Profile Information ---
    std::string currentClimate = m_climateService.GetCurrentClimateName();
    ImGui::Text("Climate: %s", currentClimate.c_str());
    int32_t aIdx = m_climateService.GetActiveSunProfileIndex();
    int32_t bIdx = m_climateService.GetNextSunProfileIndex();
    ImGui::Text("Active Sun Profile: %s (idx: %d)", m_climateService.GetSunProfileName(aIdx).c_str(), aIdx);
    ImGui::Text("Next Sun Profile: %s (idx: %d)", m_climateService.GetSunProfileName(bIdx).c_str(), bIdx);
    float progress = m_climateService.GetTransitionProgress();
    ImGui::Text("Transition: %.1f%%", progress * 100.0f);
    ImGui::Text("Active Elev: %.4f rad", m_climateService.GetSunProfileElevation(aIdx));
    ImGui::Text("Next Elev:    %.4f rad", m_climateService.GetSunProfileElevation(bIdx));

    float sunAngle = m_climateService.GetSunAngle();
    ImGui::Text("Sun Angle: %.4f rad (%.1f°)", sunAngle, sunAngle * 180.0f / 3.14159265f);
    static float setSunAngle = 0.0f;

    //--- Climate Selection Dropdown ---
    static std::vector<Data::GameData::ClimateService::ClimateInfo> climateCache;
    if (climateCache.empty() && m_climateService.IsReady()) {
      climateCache = m_climateService.GetAvailableClimates();
    }

    if (!climateCache.empty()) {
      static int selectedIdx = -1;
      std::vector<const char*> items;
      for (size_t i = 0; i < climateCache.size(); ++i) {
        items.push_back(climateCache[i].name.c_str());
        if (selectedIdx == -1 && climateCache[i].name == currentClimate) {
          selectedIdx = (int)i;
        }
      }

      if (ImGui::Combo("Select Climate", &selectedIdx, items.data(), (int)items.size())) {
        if (selectedIdx >= 0 && selectedIdx < (int)climateCache.size()) {
          m_climateService.SetClimate(climateCache[selectedIdx].token, true);
        }
      }
      if (ImGui::Button("Refresh List")) {
        climateCache = m_climateService.GetAvailableClimates();
      }
    }

    ImGui::Separator();

    // --- Weather Mode Selection ---
    int32_t currentWeather = m_climateService.GetWeatherMode();
    static int32_t lastWeatherMode = -1;

    static bool interpolatedTransition = false;

    const char* weatherNames[] = {"Nice", "Bad"};
    if (ImGui::Combo("Weather Mode", &currentWeather, weatherNames, IM_ARRAYSIZE(weatherNames))) {
      m_climateService.SetWeatherMode(currentWeather, !interpolatedTransition);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Smooth", &interpolatedTransition)) {
      // No action needed here, state is static
    }
    if (interpolatedTransition) {
      ImGui::SameLine();
      ImGui::TextDisabled("(?)");
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Smooth transition takes ~30-60 game minutes to complete.\nYou won't see immediate changes.");
      }
    }

    // --- Next Weather Mode (for testing) ---
    int32_t nextWeather = m_climateService.GetNextWeatherMode();
    static int32_t lastNextWeather = -1;
    ImGui::Text("Next Weather: %s (%d)", weatherNames[nextWeather], nextWeather);
    ImGui::SameLine();
    if (ImGui::SmallButton("Set Next##weather")) {
      m_climateService.SetNextWeatherMode(currentWeather == 0 ? 1 : 0);
    }
    if (lastNextWeather != -1 && nextWeather != lastNextWeather) {
      auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ClimateWindow");
      logger->Info("NextWeatherMode changed: {} -> {}", lastNextWeather, nextWeather);
    }
    lastNextWeather = nextWeather;

    // --- Sun Profile Info ---
    int32_t profileCount = m_climateService.GetSunProfileCount();
    ImGui::Text("Sun Profiles: %d", profileCount);

    // --- Profile Log Button ---
    static bool logging = false;

    ImGui::SameLine();
    if (ImGui::Button(logging ? "Stop Time" : "time UP")) {
        logging = !logging;
    }

    if (logging) {
        auto& world = Data::GameData::GameWorldService::GetInstance();
        uint32_t simTime = world.GetSimulationTime();


        world.SetSimulationTime(simTime + 1);
    }

    // --- Change Detection & Logging ---
    if (currentWeather != lastWeatherMode) {
      lastWeatherMode = currentWeather;
    }

    // --- Weather Blend Progress ---
    float blendProgress = m_climateService.GetWeatherBlendProgress();
    ImGui::Text("Weather Blend: %.1f%%", blendProgress * 100.0f);
    static int durationMinutes = 20;
    if (ImGui::SliderInt("Duration (min)", &durationMinutes, 1, 120)) {
      m_climateService.SetTransitionDuration(durationMinutes);
    }

    ImGui::Separator();

    // --- Final Stats ---
    ImGui::Separator();
  }
}

}  // namespace UI
SPF_NS_END
