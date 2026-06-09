/**                                                                                               
 * @file GameWorldWindow.hpp                                                                          
 * @brief UI window for Game World manipulation (time, weather, skybox).
 */ 

#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include <string>

SPF_NS_BEGIN
namespace Data::GameData { class GameWorldService; }

namespace UI {

/**
 * @class GameWorldWindow
 * @brief Implements the GameWorld tab for the framework overlay.
 */
class GameWorldWindow : public BaseWindow {
public:
    GameWorldWindow(const std::string& componentName, const std::string& windowId, Data::GameData::GameWorldService& worldService);
    virtual ~GameWorldWindow() = default;

    const char* GetWindowTitle() const override;

protected:
    void RenderContent() override;

private:
    Data::GameData::GameWorldService& m_worldService;

    // Localization keys
    std::string m_locTitle;
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

    // Days of week keys
    std::string m_locDays[7];
};

} // namespace UI
SPF_NS_END
