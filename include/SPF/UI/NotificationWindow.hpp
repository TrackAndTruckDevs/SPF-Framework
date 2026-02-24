#pragma once

#include "SPF/UI/BaseWindow.hpp"
#include "SPF/Namespace.hpp"
#include <string>
#include <chrono>

SPF_NS_BEGIN

namespace UI {

/**
 * @brief Framework-owned window for displaying temporary, non-interactive notifications.
 * @details This window is automatically positioned at the top-center of the viewport.
 *          It supports Markdown rendering and displays a progress bar indicating
 *          the remaining time before it disappears.
 */
class NotificationWindow : public BaseWindow {
public:
    NotificationWindow(const std::string& componentName, const std::string& windowId);
    virtual ~NotificationWindow() = default;

    /**
     * @brief Triggers the notification to show.
     * @param message The text to display (Markdown supported).
     * @param type The category of the notification.
     * @param duration The time in seconds to stay visible.
     */
    void Show(const std::string& message, int type, float duration);

    /**
     * @brief Internal render loop for the content. Called by BaseWindow::Render().
     */
    void RenderContent() override;

    /**
     * @brief Custom flags to make the window transparent and non-interactive.
     */
    ImGuiWindowFlags GetExtraWindowFlags() const override;

    /**
     * @brief Notifications are always non-interactive (mouse clicks pass through).
     */
    bool IsInteractive() const override { return false; }

    // Disable configuration persistence
    void ApplySettings(const nlohmann::ordered_json& settings) override {}
    nlohmann::ordered_json GetCurrentSettings() const override { return nlohmann::ordered_json::object(); }

private:
    std::string m_message;
    int m_type = 0; // Maps to SPF_NotificationType
    
    std::chrono::steady_clock::time_point m_startTime;
    float m_duration = 3.0f;
    bool m_active = false;

    // Internal styling helpers
    void GetTypeStyle(const char** out_icon, ImVec4& out_color);
};

} // namespace UI

SPF_NS_END
