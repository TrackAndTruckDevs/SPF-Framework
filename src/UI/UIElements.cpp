#include "SPF/UI/UIElements.hpp"
#include <imgui_internal.h> // For ImGui::ButtonBehavior

SPF_NS_BEGIN
namespace UI
{
    bool HyperlinkButton(const char* label, const TextStyle& style)
    {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& imgui_style = g.Style;
        const ImGuiID id = window->GetID(label);
        
        // Use ScopedStyle to apply font from TextStyle
        ScopedStyle scopedStyle(style);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
        
        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size = ImGui::CalcItemSize(ImVec2(0, 0), label_size.x + imgui_style.FramePadding.x * 2.0f, label_size.y + imgui_style.FramePadding.y * 2.0f);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(size, imgui_style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);

        // Determine background color
        const ImU32 bg_col = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

        // Determine text color with new logic
        ImVec4 final_text_color;
        if (hovered && style.hoverColor) {
            final_text_color = *style.hoverColor;
        } else if (style.color) {
            final_text_color = *style.color;
        } else {
            final_text_color = imgui_style.Colors[ImGuiCol_Text];
        }
        const ImU32 text_col = ImGui::ColorConvertFloat4ToU32(final_text_color);
        
        // Render the button background
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, imgui_style.FrameRounding);

        // Render the label (icon or text)
        const ImVec2 text_pos = ImVec2(bb.Min.x + imgui_style.FramePadding.x, bb.Min.y + imgui_style.FramePadding.y);
        window->DrawList->AddText(text_pos, text_col, label, ImGui::FindRenderedTextEnd(label));

        return pressed;
    }

} // namespace UI
SPF_NS_END
