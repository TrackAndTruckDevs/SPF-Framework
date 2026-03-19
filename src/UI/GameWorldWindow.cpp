/**                                                                                               
 * @file GameWorldWindow.cpp                                                                          
 * @brief Implementation of the UI window for Game World manipulation.
 */ 

#include "SPF/UI/GameWorldWindow.hpp"
#include "SPF/Data/GameData/GameWorldService.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "imgui.h"

SPF_NS_BEGIN
namespace UI {
using namespace Localization;

GameWorldWindow::GameWorldWindow(const std::string& componentName, const std::string& windowId, Data::GameData::GameWorldService& worldService)
    : BaseWindow(componentName, windowId), m_worldService(worldService) {
    
    // Localization Keys
    m_locTitle = "gameworld_window.title";
    m_locTimeSlider = "gameworld_window.time_slider";
    m_locTimeLabel = "gameworld_window.time_label";
    m_locNotReady = "gameworld_window.not_ready";
}

const char* GameWorldWindow::GetWindowTitle() const {
    return LocalizationManager::GetInstance().Get(m_locTitle).c_str();
}

void GameWorldWindow::RenderContent() {
    auto& loc = LocalizationManager::GetInstance();

    if (!m_worldService.IsReady()) {
        Typography::Text(TextStyle::Regular().Color(Colors::RED), "%s", loc.Get(m_locNotReady).c_str());
        return;
    }

    // --- Visual Preview Time Section ---
    ImGui::Spacing();
    Typography::Text(TextStyle::H3().Color(Colors::GOLD), "Visual (Preview) Time");
    ImGui::Separator();
    ImGui::Spacing();

    static bool skyboxLock = false;
    if (ImGui::Checkbox("Lock Visual Time (Disable Skybox Auto-update)", &skyboxLock)) {
        m_worldService.SetSkyboxAutoUpdate(!skyboxLock);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("If locked, the game won't override your visual time when unpaused.");
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
    if (ImGui::SliderInt("Visual Time Slider", &sliderVisualMins, 0, 1439, vTimeBuffer)) {
        // Keep the same day, just change the time
        uint32_t currentDayStart = (currentVisualMins / 1440) * 1440;
        m_worldService.SetPreviewTime(currentDayStart + (uint32_t)sliderVisualMins);
    }
    if (!skyboxLock) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("Enable 'Lock Visual Time' to use this slider.");
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Real Game Simulation Time ---
    Typography::Text(TextStyle::H3().Color(Colors::CYAN), "Game Simulation Time (Permanent)");
    ImGui::Separator();
    ImGui::Spacing();

    uint32_t simTotalMinutes = m_worldService.GetSimulationTime();
    uint32_t simDays = m_worldService.GetGameDay();
    uint32_t simWeek = m_worldService.GetGameWeek();
    uint32_t dayOfWeek = m_worldService.GetDayOfWeek();
    
    int simHours = (simTotalMinutes % 1440) / 60;
    int simMins = simTotalMinutes % 60;

    const char* daysOfWeek[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };

    int sliderSimMins = simTotalMinutes % 1440;
    char sTimeBuffer[16];
    snprintf(sTimeBuffer, sizeof(sTimeBuffer), "%02d:%02d", simHours, simMins);

    ImGui::Text("Current State: Week %d, %s (Day %d)", simWeek + 1, daysOfWeek[dayOfWeek], simDays + 1);
    ImGui::Text("World Clock: %02d:%02d", simHours, simMins);

    if (ImGui::SliderInt("Time of Day Slider", &sliderSimMins, 0, 1439, sTimeBuffer)) {
        uint32_t dayStart = (uint32_t)(simDays * 1440);
        m_worldService.SetSimulationTime(dayStart + (uint32_t)sliderSimMins);
    }

    if (ImGui::Button("-1 Day")) {
        if (simTotalMinutes >= 1440) m_worldService.SetSimulationTime(simTotalMinutes - 1440);
    }
    ImGui::SameLine();
    if (ImGui::Button("+1 Day")) {
        m_worldService.SetSimulationTime(simTotalMinutes + 1440);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Midnight")) {
        m_worldService.SetSimulationTime((uint32_t)(simDays * 1440));
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // --- Statistics and Scale ---
    Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "Game Engine Info");
    ImGui::Separator();
    ImGui::Spacing();

    float mapScale = m_worldService.GetMapScale();
    ImGui::Text("Map Scale (local.scale): %.2f", mapScale);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("1:19 on highways, 1:3 in cities.");
    }

    uint32_t realPlayMins = m_worldService.GetRealPlayTime();
    int playHours = realPlayMins / 60;
    int playMins = realPlayMins % 60;
    ImGui::Text("Total Session Playtime: %d hours, %d mins", playHours, playMins);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Core Engine Control Section ---
    Typography::Text(TextStyle::H3().Color(Colors::MAGENTA), "Core Engine Controls");
    
    float globalWarp = m_worldService.GetGlobalWarp();
    if (ImGui::SliderFloat("Global Game Warp", &globalWarp, 0.0f, 10.0f, "%.2f")) {
        m_worldService.SetGlobalWarp(globalWarp);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Controls the speed of the entire game (physics, traffic, animations).");
    }

    // bool isPaused = m_worldService.IsGamePaused();
    // if (ImGui::Checkbox("Engine Pause Status", &isPaused)) {
    //     m_worldService.SetGamePaused(isPaused);
    // }

    ImGui::Spacing();
    
    uint32_t frameCount = m_worldService.GetFrameCounter();
    ImGui::Text("Engine Frame Counter: %u", frameCount);

    double deltaTime = m_worldService.GetRealDeltaTime();
    ImGui::Text("Real Delta Time: %.6f s", (float)deltaTime);
}

} // namespace UI
SPF_NS_END
