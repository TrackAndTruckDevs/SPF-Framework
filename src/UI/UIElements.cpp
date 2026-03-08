#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIStyle.hpp"
#include <imgui_internal.h> // For ImGui::ButtonBehavior

SPF_NS_BEGIN
namespace UI
{
    bool Button(const char* label, const TextStyle& style, const ImVec2& size, const char* tooltip)
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
        ImVec2 actual_size = ImGui::CalcItemSize(size, label_size.x + imgui_style.FramePadding.x * 2.0f, label_size.y + imgui_style.FramePadding.y * 2.0f);

        const ImRect bb(pos, ImVec2(pos.x + actual_size.x, pos.y + actual_size.y));
        ImGui::ItemSize(actual_size, imgui_style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, 0);
        
        const bool is_disabled = (g.CurrentItemFlags & ImGuiItemFlags_Disabled);

        // Determine background color
        ImU32 bg_col;
        if (is_disabled) {
            bg_col = ImGui::GetColorU32(ImGuiCol_Button);
        } else if (held) {
            bg_col = ImGui::GetColorU32(Colors::GOLD); // Always GOLD on click
        } else if (hovered) {
            bg_col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        } else {
            bg_col = ImGui::GetColorU32(ImGuiCol_Button);
        }

        // Determine text color with "Smart Fallbacks"
        ImVec4 final_text_color;
        if (is_disabled) {
            final_text_color = imgui_style.Colors[ImGuiCol_TextDisabled];
        } else if (held) {
            // When button is GOLD, text must be DARK for contrast
            final_text_color = style.activeColor.value_or(ImVec4(0.15f, 0.19f, 0.24f, 1.00f));
        } else if (hovered) {
            final_text_color = style.hoverColor.value_or(Colors::GOLD);
        } else {
            final_text_color = style.color.value_or(imgui_style.Colors[ImGuiCol_Text]);
        }
        
        // If disabled, we also need to respect the Global Alpha which ImGui sets for disabled items
        if (is_disabled) {
            final_text_color.w *= imgui_style.DisabledAlpha;
        }
        
        const ImU32 text_col = ImGui::ColorConvertFloat4ToU32(final_text_color);

        // Render the button background
        window->DrawList->AddRectFilled(bb.Min, bb.Max, bg_col, imgui_style.FrameRounding);
        // Render the label centered in the bounding box
        const ImVec2 text_pos = ImVec2(bb.Min.x + (actual_size.x - label_size.x) * 0.5f, bb.Min.y + (actual_size.y - label_size.y) * 0.5f);
        window->DrawList->AddText(text_pos, text_col, label, ImGui::FindRenderedTextEnd(label));

        // Tooltip support
        if (tooltip && hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        return pressed;
    }

} // namespace UI
SPF_NS_END
