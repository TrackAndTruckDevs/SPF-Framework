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
    SPF_Notification_Params params = {};
    params.type = (SPF_NotificationType)type;
    params.message = message.c_str();
    params.mode = mode;
    params.duration = duration;
    // Defaults
    params.r = 0.0f; params.g = 0.0f; params.b = 0.0f; params.a = 0.0f;
    params.custom_icon = nullptr;
    
    ShowEx(params);
}

SPF_Notification_Handle NotificationWindow::ShowEx(const SPF_Notification_Params& params) {
    std::string processedMessage = params.message ? params.message : "";
    size_t pos = 0;
    while ((pos = processedMessage.find("\\n", pos)) != std::string::npos) {
        processedMessage.replace(pos, 2, "\n");
        pos += 1;
    }

    // Toggle logic for STICKY popups - REMOVE IMMEDIATELY if exists
    if (params.mode == SPF_NOTIF_MODE_STICKY) {
        auto it = std::find_if(m_notifications.begin(), m_notifications.end(), 
                               [&](const NotificationData& n) { return n.mode == SPF_NOTIF_MODE_STICKY && n.message == processedMessage; });
        if (it != m_notifications.end()) {
            m_notifications.erase(it);
            return nullptr;
        }
    }



    NotificationData notif;
    notif.message = processedMessage;
    notif.type = (int)params.type;
    notif.mode = params.mode;
    
    // Duration logic
    if (params.duration == 0.0f) {
        notif.duration = 0.0f;
        notif.isProgrammatic = true;
    } else {
        notif.duration = params.duration;
        notif.isProgrammatic = false;
    }
    
    // Sticky defaults to infinite if not explicitly managed, but usually treated as interactive
    if (params.mode == SPF_NOTIF_MODE_STICKY && params.duration < 0.0f) {
         notif.duration = 999999.0f; 
    }

    notif.startTime = std::chrono::steady_clock::now();
    notif.popupPos = ImGui::GetMousePos();
    
    // Handle generation
    if (m_nextHandle == 0) m_nextHandle = 1;
    notif.handle = m_nextHandle++;
    
    notif.customColor = ImVec4(params.r, params.g, params.b, params.a);
    if (params.custom_icon) notif.customIcon = params.custom_icon;
    
    m_notifications.push_back(notif);
    return reinterpret_cast<SPF_Notification_Handle>(notif.handle);
}

void NotificationWindow::Hide(SPF_Notification_Handle handle) {
    if (!handle) return;
    uint64_t h = reinterpret_cast<uint64_t>(handle);
    auto it = std::find_if(m_notifications.begin(), m_notifications.end(), 
                           [&](const NotificationData& n) { return n.handle == h; });
    if (it != m_notifications.end()) {
        it->isClosing = true;
    }
}

void NotificationWindow::RenderContent() {
    auto now = std::chrono::steady_clock::now();

    // Keep non-active TOP notifications from expiring while waiting in queue
    bool foundActiveTop = false;
    for (auto& notif : m_notifications) {
        if (notif.mode == SPF_NOTIF_MODE_TOP) {
            if (!foundActiveTop) {
                foundActiveTop = true;
            } else {
                notif.startTime = now;
            }
        }
    }
    
    // 1. Handle "Click outside" to close STICKY notifications
    bool clickedOutside = ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1);
    
    // 2. Remove expired or dismissed notifications
    m_notifications.erase(
        std::remove_if(m_notifications.begin(), m_notifications.end(),
            [&](const NotificationData& n) {
                if (n.isClosing) return true; // Explicitly closed via Hide()
                
                // Programmatic notifications (duration == 0) live until explicitly closed
                if (n.isProgrammatic) return false;

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
        std::string winName = "##notif_" + std::to_string(notif->handle); 
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
            std::string winName = "##notif_" + std::to_string(notif.handle);
            
            // Default: Centered horizontally, Above the cursor
            ImVec2 pivot = ImVec2(0.5f, 0.975f);
            float winW = 300.0f; 
            float winH = 60.0f;  
            
            ImGuiWindow* window = ImGui::FindWindowByName(winName.c_str());
            if (window) {
                winW = window->Size.x;
                winH = window->Size.y;
            }

            // Flip to below if not enough space above
            if (notif.popupPos.y - (winH * 1.5f) < viewPos.y) {
                pivot.y = 0.025f; 
            }

            // Shift horizontal pivot if too close to sides
            if (notif.popupPos.x - (winW * 1.5f) < viewPos.x) {
                pivot.x = 0.0f; // Window goes Right
            } else if (notif.popupPos.x + (winW * 1.5f) > viewPos.x + viewSize.x) {
                pivot.x = 1.0f; // Window goes Left
            }

            ImGui::SetNextWindowPos(notif.popupPos, ImGuiCond_Always, pivot);
            ImGui::SetNextWindowSize(ImVec2(300.0f, 0), ImGuiCond_Always);
            
            RenderSingleNotification(notif, 0);
            
            if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByPopup)) {
                 if (window && ImGui::IsMouseHoveringRect(window->Pos, ImVec2(window->Pos.x + window->Size.x, window->Pos.y + window->Size.y))) {
                     anyStickyHovered = true;
                 }
            }
            // Mark as initialized after first render to prevent immediate closure
            notif.initialized = true;
        }
    }
    
    // Delayed removal for sticky click-outside (only if not programmatic and already initialized)
    if (clickedOutside && !anyStickyHovered) {
         m_notifications.erase(
            std::remove_if(m_notifications.begin(), m_notifications.end(),
                [&](const NotificationData& n) { 
                    return n.mode == SPF_NOTIF_MODE_STICKY && !n.isProgrammatic && n.initialized; 
                }),
            m_notifications.end());
    }
}

void NotificationWindow::RenderSingleNotification(NotificationData& notif, int index) {
    // Unique ID based on handle (stable)
    std::string windowName = "##notif_" + std::to_string(notif.handle);
    
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
    GetTypeStyle(notif, &icon, color);
    ImGui::PushStyleColor(ImGuiCol_Border, color);

    if (ImGui::Begin(windowName.c_str(), nullptr, flags)) {
        if (ImGui::BeginTable("##notif_layout", 2, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("icon", ImGuiTableColumnFlags_WidthFixed, 12.0f);
            ImGui::TableSetupColumn("text", ImGuiTableColumnFlags_WidthStretch);
            
            ImGui::TableNextRow();
            
            // Icon Column
            ImGui::TableNextColumn();
            if (icon && *icon) {
                TextStyle iconStyle = TextStyle::H2().Color(color);
                Typography::Text(iconStyle, "%s", icon);
            }

            // Text Column
            ImGui::TableNextColumn();
            float textWidth = ImGui::GetContentRegionAvail().x;
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + textWidth);
            
            TextStyle msgStyle = TextStyle::Regular().Color(Colors::SILVER);
            Typography::RenderMarkdownText(notif.message, msgStyle);
            
            ImGui::PopTextWrapPos();
            
            ImGui::EndTable();
        }

        // Progress bar
        if (notif.mode != SPF_NOTIF_MODE_STICKY) {
            ImGui::Spacing();
            float progress = 1.0f;
            
            if (!notif.isProgrammatic && notif.duration > 0.0f) {
                float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - notif.startTime).count();
                progress = 1.0f - (elapsed / notif.duration);
                if (progress < 0.0f) progress = 0.0f;
            }
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            float avail_w = ImGui::GetContentRegionAvail().x;
            float bar_h = 2.0f;

            drawList->AddRectFilled(p_min, ImVec2(p_min.x + avail_w, p_min.y + bar_h), 
                                    ImGui::GetColorU32(ImGuiCol_FrameBg), 1.0f);
            
            // Draw progress bar if visible
            if (progress > 0.0f) {
                drawList->AddRectFilled(p_min, ImVec2(p_min.x + (avail_w * progress), p_min.y + bar_h), 
                                        ImGui::ColorConvertFloat4ToU32(color), 1.0f);
            }
            
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

void NotificationWindow::GetTypeStyle(const NotificationData& notif, const char** out_icon, ImVec4& out_color) {
    // 1. Check for custom overrides first
    bool hasCustomColor = (notif.customColor.x > 0.0f || notif.customColor.y > 0.0f || notif.customColor.z > 0.0f || notif.customColor.w > 0.0f);

    if (notif.type == 6) { // SPF_NOTIFICATION_CUSTOM
        *out_icon = notif.customIcon.empty() ? nullptr : notif.customIcon.c_str();
        if (hasCustomColor) {
            out_color = notif.customColor;
        } else {
            out_color = Colors::WHITE;
        }
        return;
    }

    // 2. Standard types
    switch (notif.type) {
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
    
    // 3. Allow partial overrides even for standard types
    if (hasCustomColor) {
         out_color = notif.customColor;
    }
    if (!notif.customIcon.empty()) {
        *out_icon = notif.customIcon.c_str();
    }
}

} // namespace UI

SPF_NS_END
