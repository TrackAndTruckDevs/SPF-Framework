#include "SPF/UI/ClimateWindow.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Data/GameData/ClimateService.hpp"
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
    ImGui::Text("Profile A: %s (idx: %u)", m_climateService.GetActiveProfileName(0).c_str(), m_climateService.GetActiveProfileIndex(0));

    uint32_t idxB = m_climateService.GetActiveProfileIndex(1);
    if (idxB != 0xFFFFFFFF && idxB != m_climateService.GetActiveProfileIndex(0)) {
      ImGui::Text("Profile B: %s (idx: %u)", m_climateService.GetActiveProfileName(1).c_str(), idxB);
    }

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
    static uint32_t lastSkyboxIndex = 0xFFFFFFFF;

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

    // --- Skybox Selection ---
    uint64_t skyboxCount = m_climateService.GetSkyboxCount(currentWeather);
    uint32_t currentSkybox = 0;
    if (skyboxCount > 0) {
      currentSkybox = m_climateService.GetSkyboxIndex(currentWeather);
      int currentSkyboxInt = (int)currentSkybox;
      if (ImGui::SliderInt("Skybox Index", &currentSkyboxInt, 0, (int)(skyboxCount - 1))) {
        m_climateService.SetSkyboxIndex(currentWeather, (uint32_t)currentSkyboxInt);
        currentSkybox = (uint32_t)currentSkyboxInt;
      }
      ImGui::Text("Available Skyboxes: %llu", skyboxCount);
    } else {
      ImGui::TextDisabled("Skybox data not available.");
    }

    // --- Change Detection & Logging ---
    if (currentWeather != lastWeatherMode || currentSkybox != lastSkyboxIndex) {
      lastWeatherMode = currentWeather;
      lastSkyboxIndex = currentSkybox;
    }

    ImGui::Separator();

    // // --- SECTION: SUN PROFILE EDITOR (70 PARAMS) ---
    // if (ImGui::TreeNodeEx("Sun Profile Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     // if (ImGui::Button("Dump Current Profile to Log")) {
    //     //     m_climateService.LogSunProfileData(0);
    //     // }
    //     ImGui::SameLine();
    //     ImGui::TextDisabled("(?) HDR Mode: Use decimal input (0.000)");

    //     int32_t mode = m_climateService.GetWeatherMode();
    //     uint32_t varIdx = m_climateService.GetSkyboxIndex(mode);

    //     auto renderColorRow = [&](const char* label, auto getter, auto setter) {
    //         Utils::Vector3 col = (m_climateService.*getter)(0, varIdx);
    //         float colorArr[3] = { col.x, col.y, col.z };
    //         ImGui::PushID(label);
    //         if (ImGui::ColorEdit3("##picker", colorArr, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoInputs)) {
    //             (m_climateService.*setter)(0, varIdx, { colorArr[0], colorArr[1], colorArr[2] });
    //         }
    //         ImGui::SameLine();
    //         ImGui::Text("%-18s", label);
    //         ImGui::SameLine();
    //         ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.95f);
    //         if (ImGui::DragFloat3("##raw", colorArr, 0.01f, 0.0f, 10000.0f, "%.3f")) {
    //             (m_climateService.*setter)(0, varIdx, { colorArr[0], colorArr[1], colorArr[2] });
    //         }
    //         ImGui::PopID();
    //     };

    //     auto renderFloatRow = [&](const char* label, auto getter, auto setter, float min, float max) {
    //         float val = (m_climateService.*getter)(0, varIdx);
    //         ImGui::PushID(label);
    //         ImGui::Text("%-21s", label);
    //         ImGui::SameLine();
    //         ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.9f);
    //         if (ImGui::DragFloat("##val", &val, 0.005f, min, max, "%.3f")) {
    //             (m_climateService.*setter)(0, varIdx, val);
    //         }
    //         ImGui::PopID();
    //     };

    //     if (ImGui::TreeNode("1. Lighting & Shadows")) {
    //         renderColorRow("Ambient", &Data::GameData::ClimateService::GetAmbientColor, &Data::GameData::ClimateService::SetAmbientColor);
    //         renderColorRow("Diffuse (HDR)", &Data::GameData::ClimateService::GetSunDiffuseColor, &Data::GameData::ClimateService::SetSunDiffuseColor);
    //         renderColorRow("Specular", &Data::GameData::ClimateService::GetSunSpecularColor, &Data::GameData::ClimateService::SetSunSpecularColor);
    //         renderFloatRow("Env Intensity", &Data::GameData::ClimateService::GetEnvIntensity, &Data::GameData::ClimateService::SetEnvIntensity, 0.0f, 10.0f);
    //         renderFloatRow("Sun Shadow Str", &Data::GameData::ClimateService::GetSunShadowStrength, &Data::GameData::ClimateService::SetSunShadowStrength, 0.0f, 1.0f);
    //         ImGui::TreePop();
    //     }

    //     if (ImGui::TreeNode("2. Sky & Sun")) {
    //         renderColorRow("Sky Color", &Data::GameData::ClimateService::GetSkyColor, &Data::GameData::ClimateService::SetSkyColor);
    //         renderColorRow("Sky Bottom", &Data::GameData::ClimateService::GetSkyBottomColor, &Data::GameData::ClimateService::SetSkyBottomColor);
    //         renderColorRow("Sun Color", &Data::GameData::ClimateService::GetSunColor, &Data::GameData::ClimateService::SetSunColor);
    //         renderColorRow("Sun Halo", &Data::GameData::ClimateService::GetSunHaloColor, &Data::GameData::ClimateService::SetSunHaloColor);
    //         renderColorRow("Sunshaft Color", &Data::GameData::ClimateService::GetSunshaftColor, &Data::GameData::ClimateService::SetSunshaftColor);
    //         renderFloatRow("Sun Opacity", &Data::GameData::ClimateService::GetSunOpacity, &Data::GameData::ClimateService::SetSunOpacity, 0.0f, 1.0f);
    //         renderFloatRow("Sunshaft Size", &Data::GameData::ClimateService::GetSunshaftSize, &Data::GameData::ClimateService::SetSunshaftSize, 0.0f, 10.0f);
    //         ImGui::TreePop();
    //     }

    //     if (ImGui::TreeNode("3. Night Sky & Stars")) {
    //         renderColorRow("Moon Color", &Data::GameData::ClimateService::GetMoonColor, &Data::GameData::ClimateService::SetMoonColor);
    //         renderColorRow("Moon Halo", &Data::GameData::ClimateService::GetMoonHaloColor, &Data::GameData::ClimateService::SetMoonHaloColor);
    //         renderColorRow("Starmap", &Data::GameData::ClimateService::GetStarmapColor, &Data::GameData::ClimateService::SetStarmapColor);
    //         renderColorRow("Stars", &Data::GameData::ClimateService::GetStarsColor, &Data::GameData::ClimateService::SetStarsColor);
    //         renderFloatRow("Moon Halo Scale", &Data::GameData::ClimateService::GetMoonHaloScale, &Data::GameData::ClimateService::SetMoonHaloScale, 0.0f, 100.0f);
    //         ImGui::TreePop();
    //     }

    //     if (ImGui::TreeNode("4. Fog & Atmosphere")) {
    //         renderColorRow("Fog Color 1", &Data::GameData::ClimateService::GetFogColor, &Data::GameData::ClimateService::SetFogColor);
    //         renderColorRow("Fog Color 2", &Data::GameData::ClimateService::GetFogColor2, &Data::GameData::ClimateService::SetFogColor2);
    //         renderFloatRow("Fog V-Gradient", &Data::GameData::ClimateService::GetFogVGradient, &Data::GameData::ClimateService::SetFogVGradient, 0.0f, 1.0f);
    //         renderFloatRow("Temperature (K)", &Data::GameData::ClimateService::GetTemperature, &Data::GameData::ClimateService::SetTemperature, 0.0f, 100.0f);
    //         ImGui::TreePop();
    //     }

    //     if (ImGui::TreeNode("5. Visual Quality (HDR)")) {
    //         renderColorRow("Color Balance", &Data::GameData::ClimateService::GetColorBalance, &Data::GameData::ClimateService::SetColorBalance);
    //         renderFloatRow("Contrast", &Data::GameData::ClimateService::GetContrast, &Data::GameData::ClimateService::SetContrast, 0.0f, 5.0f);
    //         renderFloatRow("Saturation", &Data::GameData::ClimateService::GetColorSaturation, &Data::GameData::ClimateService::SetColorSaturation, 0.0f, 5.0f);
    //         renderFloatRow("Bloom Intensity", &Data::GameData::ClimateService::GetBloomIntensity, &Data::GameData::ClimateService::SetBloomIntensity, 0.0f, 50.0f);
    //         renderFloatRow("Bloom Threshold", &Data::GameData::ClimateService::GetBloomThreshold, &Data::GameData::ClimateService::SetBloomThreshold, 0.0f, 1.0f);
    //         renderFloatRow("Bloom Limit", &Data::GameData::ClimateService::GetBloomLimit, &Data::GameData::ClimateService::SetBloomLimit, 0.0f, 200.0f);

    //         float minExp, maxExp;
    //         m_climateService.GetExposureLimits(0, varIdx, minExp, maxExp);
    //         float exp[2] = { minExp, maxExp };
    //         ImGui::Text("Exposure (Min/Max)"); ImGui::SameLine();
    //         ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.95f);
    //         if (ImGui::DragFloat2("##exp", exp, 0.01f, 0.0f, 100.0f, "%.3f")) {
    //             m_climateService.SetExposureLimits(0, varIdx, exp[0], exp[1]);
    //         }
    //         ImGui::TreePop();
    //     }

    //     ImGui::TreePop();
    // }

    // // --- SECTION: RAIN & ROAD ---
    // if (ImGui::TreeNodeEx("Rain & Road", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     // --- Rain ---
    //     float currentRain = m_climateService.GetRainIntensity();
    //     static float setRain = 0.0f;

    //     ImGui::Columns(2, "rain_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Rain Intensity (SET)", &setRain, 0.0f, 1.0f, "%.2f")) {
    //         m_climateService.SetRainIntensity(setRain);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentRain);
    //     ImGui::Columns(1);

    //     // --- Wetness ---
    //     float currentWet = m_climateService.GetRoadWetness();
    //     static float setWet = 0.35f;

    //     ImGui::Columns(2, "wet_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Road Wetness (SET)", &setWet, 0.0f, 1.0f, "%.2f")) {
    //         m_climateService.SetRoadWetness(setWet);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentWet);
    //     ImGui::Columns(1);

    //     ImGui::TreePop();
    // }

    // // --- SECTION: SNOW ---
    // if (ImGui::TreeNodeEx("Snow", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     // --- Snow Intensity ---
    //     float currentSnow = m_climateService.GetSnowIntensity();
    //     static float setSnow = 0.0f;

    //     ImGui::Columns(2, "snow_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Snow Intensity (SET)", &setSnow, 0.0f, 1.0f, "%.2f")) {
    //         m_climateService.SetSnowIntensity(setSnow);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentSnow);
    //     ImGui::Columns(1);

    //     // --- Snowflake Size ---
    //     float minSize, maxSize;
    //     m_climateService.GetSnowflakeSize(minSize, maxSize);
    //     static float setMinSize = 0.01f, setMaxSize = 0.02f;

    //     ImGui::Columns(2, "snow_size_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Snowflake Min Size (SET)", &setMinSize, 0.001f, 0.1f, "%.3f")) {
    //         m_climateService.SetSnowflakeSize(setMinSize, setMaxSize);
    //     }
    //     if (ImGui::SliderFloat("Snowflake Max Size (SET)", &setMaxSize, 0.001f, 0.1f, "%.3f")) {
    //         m_climateService.SetSnowflakeSize(setMinSize, setMaxSize);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("MIN GET: %.4f", minSize);
    //     ImGui::Text("MAX GET: %.4f", maxSize);
    //     ImGui::Columns(1);

    //     // --- Snow Chaos ---
    //     float chaosRate, chaosWeight;
    //     m_climateService.GetSnowChaos(chaosRate, chaosWeight);
    //     static float setChaosRate = 0.01f, setChaosWeight = 0.25f;

    //     ImGui::Columns(2, "snow_chaos_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Snow Chaos Rate (SET)", &setChaosRate, 0.0f, 1.0f, "%.3f")) {
    //         m_climateService.SetSnowChaos(setChaosRate, setChaosWeight);
    //     }
    //     if (ImGui::SliderFloat("Snow Chaos Weight (SET)", &setChaosWeight, 0.0f, 1.0f, "%.3f")) {
    //         m_climateService.SetSnowChaos(setChaosRate, setChaosWeight);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("RATE GET: %.4f", chaosRate);
    //     ImGui::Text("WGHT GET: %.4f", chaosWeight);
    //     ImGui::Columns(1);

    //     ImGui::TreePop();
    // }

    // // --- SECTION: SUN ---
    // if (ImGui::TreeNodeEx("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     int32_t mode = m_climateService.GetWeatherMode();
    //     uint32_t varIdx = m_climateService.GetSkyboxIndex(mode);

    //     // --- Sun Appearance ---
    //     float currentSunApp = m_climateService.GetSunAppearance(0, varIdx);
    //     static float setSunApp = 1.0f;

    //     ImGui::Columns(2, "sun_app_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Sun Appearance (SET)", &setSunApp, -10.0f, 10.0f, "%.1f")) {
    //         m_climateService.SetSunAppearance(0, varIdx, setSunApp);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentSunApp);
    //     ImGui::Columns(1);

    //     // --- Sun Glow ---
    //     float currentSunGlow = m_climateService.GetSunGlowSize(0, varIdx);
    //     static float setSunGlow = 3.0f;

    //     ImGui::Columns(2, "sun_glow_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Sun Glow Size (SET)", &setSunGlow, 0.0f, 30.0f, "%.2f")) {
    //         m_climateService.SetSunGlowSize(0, varIdx, setSunGlow);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentSunGlow);
    //     ImGui::Columns(1);

    //     ImGui::TreePop();
    // }

    // // --- SECTION: FOG ---
    // if (ImGui::TreeNodeEx("Fog", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     // --- Fog Density ---
    //     float currentFog = m_climateService.GetFogDensity();
    //     static float setFog = 0.01f;

    //     ImGui::Columns(2, "fog_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Fog Density (SET)", &setFog, 0.0f, 0.5f, "%.4f")) {
    //         m_climateService.SetFogDensity(setFog);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentFog);
    //     ImGui::Columns(1);

    //     // --- Fog Offset ---
    //     float currentFogOffset = m_climateService.GetFogOffset();
    //     static float setFogOffset = 0.0f;

    //     ImGui::Columns(2, "fog_offset_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Fog Offset (SET)", &setFogOffset, 0.0f, 100.0f, "%.2f")) {
    //         m_climateService.SetFogOffset(setFogOffset);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.2f", currentFogOffset);
    //     ImGui::Columns(1);

    //     ImGui::TreePop();
    // }

    // // --- SECTION: ATMOSPHERE & EFFECTS ---
    // if (ImGui::TreeNodeEx("Atmosphere & Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
    //     int32_t mode = m_climateService.GetWeatherMode();
    //     uint32_t varIdx = m_climateService.GetSkyboxIndex(mode);

    //     // --- Temperature (Air Logic) ---
    //     float currentTemp = m_climateService.GetTemperature(0, varIdx);
    //     static float setTemp = 20.0f;

    //     ImGui::Columns(2, "air_temp_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Temperature (SET)", &setTemp, -30.0f, 50.0f, "%.1f C")) {
    //         m_climateService.SetTemperature(0, varIdx, setTemp);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentTemp);
    //     ImGui::Columns(1);

    //     // --- Cloud Speed ---
    //     float currentCloudSpeed = m_climateService.GetCloudSpeed(0, varIdx);
    //     static float setCloudSpeed = 1.0f;

    //     ImGui::Columns(2, "cloud_speed_cols", false);
    //     ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //     if (ImGui::SliderFloat("Cloud Speed (SET)", &setCloudSpeed, 0.0f, 100.0f, "%.2f")) {
    //         m_climateService.SetCloudSpeed(0, varIdx, setCloudSpeed);
    //     }
    //     ImGui::NextColumn();
    //     ImGui::Text("GET: %.4f", currentCloudSpeed);
    //     ImGui::Columns(1);

    //     // --- Stats ---
    //     ImGui::Text("Dashboard Temperature: %.1f C", m_climateService.GetDashboardTemperature());

    //     // --- Lightning ---
    //     bool lightningEnabled = m_climateService.IsLightningEnabled();
    //     if (ImGui::Checkbox("Lightning Enabled", &lightningEnabled)) {
    //         m_climateService.SetLightningEnabled(lightningEnabled);
    //     }

    //     if (lightningEnabled) {
    //         float currentLight = m_climateService.GetLightningIntensity();
    //         static float setLight = 1.0f;

    //         ImGui::Columns(2, "light_cols", false);
    //         ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.7f);
    //         if (ImGui::SliderFloat("Lightning Power (SET)", &setLight, 0.0f, 5.0f, "%.2f")) {
    //             m_climateService.SetLightningIntensity(setLight);
    //         }
    //         ImGui::NextColumn();
    //         ImGui::Text("GET: %.4f", currentLight);
    //         ImGui::Columns(1);
    //     }

    //     ImGui::TreePop();
    // }

    // --- Final Stats ---
    ImGui::Separator();
  }
}

}  // namespace UI
SPF_NS_END
