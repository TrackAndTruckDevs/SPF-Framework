#include "SPF/UI/NotificationWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/MarkdownRenderer.hpp"
#include <imgui.h>
#include <imgui_internal.h>

SPF_NS_BEGIN

namespace UI {

NotificationWindow::NotificationWindow(const std::string& componentName, const std::string& windowId)
    : BaseWindow(componentName, windowId) {
    m_isVisible = false;
    m_isInteractive = false;
}

void NotificationWindow::Show(const std::string& message, int type, float duration) {
    // 1. Process newlines
    m_message = message;
    size_t pos = 0;
    while ((pos = m_message.find("\\n", pos)) != std::string::npos) {
        m_message.replace(pos, 2, "\n");
        pos += 1;
    }

    m_type = type;
    m_duration = duration;
    m_startTime = std::chrono::steady_clock::now();
    m_active = true;
    m_isVisible = true;

    // Set initial size for BaseWindow to help with first frame
    m_sizeW = 500.0f;
    m_stateIsDirty = true;
}

void NotificationWindow::RenderContent() {
    if (!m_active) return;

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();

    if (elapsed >= m_duration) {
        m_active = false;
        m_isVisible = false;
        return;
    }

    // --- Sizing Logic ---
    const float viewport_w = ImGui::GetMainViewport()->Size.x;
    const float window_w = 500.0f;
    const float top_offset = 50.0f;
    
    // We set position manually because it's not managed by config anymore
    ImGui::SetWindowPos(ImVec2((viewport_w - window_w) * 0.5f, top_offset));
    
    // Sync internal state to prevent saving incorrect values if something tries to read them
    m_sizeW = window_w;
    m_lastSize.x = window_w;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    // Force established width from the inside
    ImGui::Dummy(ImVec2(window_w - 20.0f, 1.0f));

    const char* icon = nullptr;
    ImVec4 color;
    GetTypeStyle(&icon, color);

    // 1. Icon (Centered at the top)
    TextStyle iconStyle = TextStyle::H1().Color(color).Align(TextAlign::Center);
    Typography::Text(iconStyle, "%s", icon);

    ImGui::Spacing();

    // 2. Message (Markdown)
    // Fix: We push an absolute wrap position based on the window's 500px width
    // This solves the "1-3 character wrap" bug.
    ImGui::PushTextWrapPos(ImGui::GetWindowPos().x + (window_w - 15.0f)); 
    
    TextStyle msgStyle = TextStyle::Regular().Color(Colors::SILVER);
    Typography::RenderMarkdownText(m_message, msgStyle);

    ImGui::PopTextWrapPos();

    ImGui::Spacing();
    ImGui::Spacing();

    // 3. Progress Bar (At the very bottom)
    float progress = 1.0f - (elapsed / m_duration);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Position bar at the very bottom of the window
    ImVec2 p_min = ImGui::GetCursorScreenPos();
    float avail_w = ImGui::GetContentRegionAvail().x;
    float bar_h = 3.0f;

    drawList->AddRectFilled(p_min, ImVec2(p_min.x + avail_w, p_min.y + bar_h), 
                            ImGui::GetColorU32(ImGuiCol_FrameBg), 1.0f);
    drawList->AddRectFilled(p_min, ImVec2(p_min.x + (avail_w * progress), p_min.y + bar_h), 
                            ImGui::ColorConvertFloat4ToU32(color), 1.0f);
    
    ImGui::Dummy(ImVec2(0, bar_h + 5.0f)); // Reserve space + bottom padding
    
    ImGui::PopStyleVar(); // Pop WindowPadding
}

ImGuiWindowFlags NotificationWindow::GetExtraWindowFlags() const {
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
           ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
           ImGuiWindowFlags_NoInputs; // Clicks pass through
}

void NotificationWindow::GetTypeStyle(const char** out_icon, ImVec4& out_color) {
    switch (m_type) {
        case 1: // SUCCESS
            *out_icon = ICON_FA_CIRCLE_CHECK;
            out_color = Colors::LIME;
            break;
        case 2: // WARNING
            *out_icon = ICON_FA_TRIANGLE_EXCLAMATION;
            out_color = Colors::GOLD;
            break;
        case 3: // ERROR
            *out_icon = ICON_FA_CIRCLE_XMARK;
            out_color = Colors::BRIGHT_RED;
            break;
        case 4: // CRITICAL
            *out_icon = ICON_FA_RADIATION;
            out_color = Colors::DARK_RED;
            break;
        case 5: // HINT
            *out_icon = ICON_FA_LIGHTBULB;
            out_color = Colors::PURPLE;
            break;
        default: // INFO (0)
            *out_icon = ICON_FA_CIRCLE_INFO;
            out_color = Colors::LIGHT_BLUE;
            break;
    }
}

} // namespace UI

SPF_NS_END
