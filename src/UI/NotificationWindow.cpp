#include "SPF/UI/NotificationWindow.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/MarkdownRenderer.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>

SPF_NS_BEGIN

namespace UI {

NotificationWindow::NotificationWindow(const std::string& componentName, const std::string& windowId)
    : BaseWindow(componentName, windowId) {
    m_isVisible = true; // Always visible manager
    m_isInteractive = false;
}

void NotificationWindow::Show(const std::string& message, int type, float duration, SPF_Notification_DisplayMode mode) {
    std::string processedMessage = message;
    size_t pos = 0;
    while ((pos = processedMessage.find("\\n", pos)) != std::string::npos) {
        processedMessage.replace(pos, 2, "\n");
        pos += 1;
    }

    // Toggle logic for STICKY popups
    if (mode == SPF_NOTIF_MODE_STICKY) {
        auto it = std::find_if(m_notifications.begin(), m_notifications.end(), 
                               [&](const NotificationData& n) { return n.mode == SPF_NOTIF_MODE_STICKY && n.message == processedMessage; });
        if (it != m_notifications.end()) {
            m_notifications.erase(it);
            return;
        }
    }

    // Replace logic for TOP notifications
    if (mode == SPF_NOTIF_MODE_TOP) {
        auto it = std::find_if(m_notifications.begin(), m_notifications.end(), 
                               [](const NotificationData& n) { return n.mode == SPF_NOTIF_MODE_TOP; });
        if (it != m_notifications.end()) {
            it->message = processedMessage;
            it->type = type;
            it->duration = duration;
            it->startTime = std::chrono::steady_clock::now();
            return;
        }
    }

    NotificationData notif;
    notif.message = processedMessage;
    notif.type = type;
    notif.mode = mode;
    notif.duration = (mode == SPF_NOTIF_MODE_STICKY) ? 999999.0f : duration; // No timeout for sticky
    notif.startTime = std::chrono::steady_clock::now();
    notif.popupPos = ImGui::GetMousePos();
    
    m_notifications.push_back(notif);
}

void NotificationWindow::RenderContent() {
    auto now = std::chrono::steady_clock::now();
    
    // 1. Handle "Click outside" to close STICKY notifications
    bool clickedOutside = ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1);
    
    // 2. Remove expired or dismissed notifications
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
            [&](const NotificationData& n) {
                float elapsed = std::chrono::duration<float>(now - n.startTime).count();
                return elapsed >= n.duration;
            }),
        m_notifications.end());

    if (m_notifications.empty()) return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 viewSize = viewport->Size;
    const ImVec2 viewPos = viewport->Pos;

    // --- RENDERING PASSES ---

    // 1. Render TOP notification
    for (auto& notif : m_notifications) {
        if (notif.mode == SPF_NOTIF_MODE_TOP) {
            ImGui::SetNextWindowPos(ImVec2(viewPos.x + (viewSize.x - 500.0f) * 0.5f, viewPos.y + 50.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(500.0f, 0), ImGuiCond_Always);
            RenderSingleNotification(notif, 0);
            break; // Only one TOP supported
        }
    }

    // 2. Render BOTTOM_RIGHT_STACK
    std::vector<NotificationData*> stack;
    for (auto& notif : m_notifications) {
        if (notif.mode == SPF_NOTIF_MODE_STACK) stack.push_back(&notif);
    }

    float currentY = viewPos.y + viewSize.y - 20.0f;
    for (int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
        auto* notif = stack[i];
        ImGui::SetNextWindowPos(ImVec2(viewPos.x + viewSize.x - 20.0f, currentY), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowSize(ImVec2(350.0f, 0), ImGuiCond_Always);
        
        RenderSingleNotification(*notif, i);
        
        // Offset for next item in stack based on height of what we just rendered
        std::string winName = "##notif_" + std::to_string(static_cast<uint64_t>(notif->startTime.time_since_epoch().count())); 
        ImGuiWindow* window = ImGui::FindWindowByName(winName.c_str());
        if (window) {
            currentY -= (window->Size.y + 10.0f);
        } else {
            currentY -= 100.0f; // Guess height for first frame
        }
    }

    // 3. Render STICKY popups
    bool anyStickyHovered = false;
    for (auto& notif : m_notifications) {
        if (notif.mode == SPF_NOTIF_MODE_STICKY) {
            ImGui::SetNextWindowPos(notif.popupPos, ImGuiCond_Appearing);
            ImGui::SetNextWindowSize(ImVec2(300.0f, 0), ImGuiCond_Always);
            
            RenderSingleNotification(notif, 0);
            
            std::string winName = "##notif_" + std::to_string(static_cast<uint64_t>(notif.startTime.time_since_epoch().count()));
            ImGuiWindow* window = ImGui::FindWindowByName(winName.c_str());
            if (window) {
                 ImVec2 mousePos = ImGui::GetMousePos();
                 if (mousePos.x >= window->Pos.x && mousePos.x <= window->Pos.x + window->Size.x &&
                     mousePos.y >= window->Pos.y && mousePos.y <= window->Pos.y + window->Size.y) {
                     anyStickyHovered = true;
                 }
            }
        }
    }
    
    // Delayed removal for sticky click-outside
    if (clickedOutside && !anyStickyHovered) {
         m_notifications.erase(
            std::remove_if(m_notifications.begin(), m_notifications.end(),
                [&](const NotificationData& n) { return n.mode == SPF_NOTIF_MODE_STICKY; }),
            m_notifications.end());
    }
}

void NotificationWindow::RenderSingleNotification(NotificationData& notif, int index) {
    // Unique ID based on timestamp
    std::string windowName = "##notif_" + std::to_string(static_cast<uint64_t>(notif.startTime.time_since_epoch().count()));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoSavedSettings;

    if (notif.mode != SPF_NOTIF_MODE_STICKY) {
        flags |= ImGuiWindowFlags_NoInputs;
    }

    // Styling
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.1f, 0.94f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

    const char* icon = nullptr;
    ImVec4 color;
    GetTypeStyle(notif.type, &icon, color);
    ImGui::PushStyleColor(ImGuiCol_Border, color);

    if (ImGui::Begin(windowName.c_str(), nullptr, flags)) {
        ImGui::BeginGroup();
        {
            TextStyle iconStyle = TextStyle::H2().Color(color);
            Typography::Text(iconStyle, "%s", icon);
        }
        ImGui::EndGroup();
        
        ImGui::SameLine(40.0f);
        
        ImGui::BeginGroup();
        {
            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
            TextStyle msgStyle = TextStyle::Regular().Color(Colors::SILVER);
            Typography::RenderMarkdownText(notif.message, msgStyle);
            ImGui::PopTextWrapPos();
        }
        ImGui::EndGroup();

        // Progress bar (only for auto-fading notifications)
        if (notif.mode != SPF_NOTIF_MODE_STICKY) {
            ImGui::Spacing();
            float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - notif.startTime).count();
            float progress = 1.0f - (elapsed / notif.duration);
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            float avail_w = ImGui::GetContentRegionAvail().x;
            float bar_h = 2.0f;

            drawList->AddRectFilled(p_min, ImVec2(p_min.x + avail_w, p_min.y + bar_h), 
                                    ImGui::GetColorU32(ImGuiCol_FrameBg), 1.0f);
            drawList->AddRectFilled(p_min, ImVec2(p_min.x + (avail_w * progress), p_min.y + bar_h), 
                                    ImGui::ColorConvertFloat4ToU32(color), 1.0f);
            
            ImGui::Dummy(ImVec2(0, bar_h));
        }
    }
    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

ImGuiWindowFlags NotificationWindow::GetExtraWindowFlags() const {
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
           ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration |
           ImGuiWindowFlags_NoNav;
}

bool NotificationWindow::IsInteractive() const {
    for (const auto& n : m_notifications) {
        if (n.mode == SPF_NOTIF_MODE_STICKY) return true;
    }
    return false;
}

void NotificationWindow::GetTypeStyle(int type, const char** out_icon, ImVec4& out_color) {
    switch (type) {
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
