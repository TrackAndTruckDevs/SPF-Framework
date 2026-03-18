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
    std::string m_locTimeSlider;
    std::string m_locTimeLabel;
    std::string m_locNotReady;
};

} // namespace UI
SPF_NS_END
