#include <SPF/UI/UIStyle.hpp>
#include <imgui.h>

SPF_NS_BEGIN
namespace UI
{
    void Style::ApplyGameStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // Colors
        style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.90f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.96f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.08f, 0.08f, 0.08f, 0.75f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.90f, 0.90f, 0.90f, 0.80f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.90f); // Base background for table rows
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.07f, 0.07f, 0.07f, 0.90f); // Slightly lighter for alternating rows 
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.18f, 0.22f, 0.30f, 0.20f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.27f, 0.36f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.25f, 0.30f, 0.40f, 0.95f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.30f, 0.40f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.19f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.30f, 0.40f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.22f, 0.27f, 0.36f, 0.50f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.22f, 0.27f, 0.36f, 1.00f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

        // Rounding and Borders
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.TabRounding = 4.0f;
        
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 0.0f;

        // Spacing
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(6.0f, 4.0f);
        style.CellPadding = ImVec2(6.0f, 6.0f); //table w/h
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 21.0f;

        // Alignment
        style.WindowMenuButtonPosition = ImGuiDir_Right;
    }
}
SPF_NS_END