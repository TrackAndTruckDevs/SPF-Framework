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

    // --- Time Manipulation Section ---
    ImGui::Spacing();
    Typography::Text(TextStyle::H3().Color(Colors::GOLD), "%s", loc.Get(m_locTimeLabel).c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Get current time from game to sync the slider
    int currentMinutes = (int)m_worldService.GetPreviewTime();
    
    // Time Slider (0 - 1439 minutes)
    // We use a temporary int for the slider and then push the update to the service
    int sliderMinutes = currentMinutes;
    
    // Formatting the time for the slider label (HH:MM)
    char timeBuffer[16];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", sliderMinutes / 60, sliderMinutes % 60);

    if (ImGui::SliderInt(loc.Get(m_locTimeSlider).c_str(), &sliderMinutes, 0, 1439, timeBuffer)) {
        m_worldService.SetPreviewTime((uint32_t)sliderMinutes);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("This changes the visual skybox only. Real game time remains unchanged.");
}

} // namespace UI
SPF_NS_END
