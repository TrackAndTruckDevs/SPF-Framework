#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include <imgui.h>
#include <stdarg.h>
#include "SPF/UI/MarkdownRenderer.hpp"
#include <algorithm> // For std::min, std::max

SPF_NS_BEGIN

namespace UI {

// --- TextStyle Factory Methods ---

TextStyle TextStyle::Regular() { return {}; }
TextStyle TextStyle::Bold() { return { .fontKey = "bold" }; }
TextStyle TextStyle::Italic() { return { .fontKey = "italic" }; }
TextStyle TextStyle::BoldItalic() { return { .fontKey = "bold_italic" }; }
TextStyle TextStyle::Medium() { return { .fontKey = "medium" }; }
TextStyle TextStyle::MediumItalic() { return { .fontKey = "medium_italic" }; }
TextStyle TextStyle::Monospace() { return { .fontKey = "monospace" }; }
TextStyle TextStyle::H1() { return { .fontKey = "h1" }; }
TextStyle TextStyle::H2() { return { .fontKey = "h2" }; }
TextStyle TextStyle::H3() { return { .fontKey = "h3" }; }

// --- ScopedStyle RAII Helper Implementation ---

ScopedStyle::ScopedStyle(const TextStyle& style) {
    if (style.fontKey) {
        ImFont* font = UIManager::GetInstance().GetFont(*style.fontKey);
        if (font) {
            ImGui::PushFont(font);
            m_fontPushed = true;
        }
    }

    if (style.disabled) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        m_colorCount++;
    } else if (style.color) {
        ImGui::PushStyleColor(ImGuiCol_Text, *style.color);
        m_colorCount++;
    }
}

ScopedStyle::~ScopedStyle() {
    if (m_fontPushed) {
        ImGui::PopFont();
    }
    if (m_colorCount > 0) {
        ImGui::PopStyleColor(m_colorCount);
    }
}

// --- Typography Core Methods Implementation ---

void Typography::TextV(const TextStyle& style, const char* fmt, va_list args) {
    ScopedStyle scopedStyle(style);

    // If it's a separator, we use a different ImGui call and logic.
    if (style.isSeparator) {
        char temp_buffer[2048];
        vsnprintf(temp_buffer, IM_ARRAYSIZE(temp_buffer), fmt, args);
        ImGui::SeparatorText(temp_buffer);
        return; // Separator handles its own rendering.
    }

    // 1. Apply padding by adjusting cursor position
    if (style.padding.x != 0.0f || style.padding.y != 0.0f) {
        const ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursorPos.x + style.padding.x, cursorPos.y + style.padding.y));
    }

    // 2. Handle Text Wrapping
    if (style.wrap) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + ImGui::GetContentRegionAvail().x);
    }

    // 3. Handle Alignment
    if (style.align == TextAlign::Center || style.align == TextAlign::Right) {
        // We need the formatted text to calculate its size for alignment.
        // So we format it once here.
        char temp_buffer[2048]; // A bit risky, but vsnprintf requires a buffer.
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(temp_buffer, IM_ARRAYSIZE(temp_buffer), fmt, args_copy);
        va_end(args_copy);

        const float textWidth = ImGui::CalcTextSize(temp_buffer).x;
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        
        float offsetX = 0.0f;
        if (style.align == TextAlign::Center) {
            offsetX = (availableWidth - textWidth) * 0.5f;
        } else { // Right
            offsetX = availableWidth - textWidth;
        }

        if (offsetX > 0.0f) {
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
        }
    }

    // 4. Render the text
    ImGui::TextV(fmt, args);

    // 5. Handle Underline and Strikethrough
    if (style.underline || style.strikethrough) {
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        const float thickness = 1.0f;
        auto* drawList = ImGui::GetWindowDrawList();
        const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);

        if (style.underline) {
            const float y = max.y - thickness;
            drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), col, thickness);
        }
        if (style.strikethrough) {
            const float y = (min.y + max.y) * 0.5f;
            drawList->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), col, thickness);
        }
    }

    // Pop wrapping state
    if (style.wrap) {
        ImGui::PopTextWrapPos();
    }
}

void Typography::Text(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(TextStyle::Regular(), fmt, args);
    va_end(args);
}

void Typography::Text(const TextStyle& style, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    TextV(style, fmt, args);
    va_end(args);
}

ImVec2 Typography::CalcTextSize(const char* text, const TextStyle& style) {
    ScopedStyle scopedStyle(style);
    return ImGui::CalcTextSize(text);
}

void Typography::RenderMarkdownText(const std::string& markdownText, const TextStyle& style) {
    std::string processedText = markdownText;
    // Smart replacement to ensure ``` is on its own line for the parser
    size_t start_pos = 0;
    std::string to_find = "```";
    std::string replacement = "\n```\n";
    while((start_pos = processedText.find(to_find, start_pos)) != std::string::npos) {
        processedText.replace(start_pos, to_find.length(), replacement);
        start_pos += replacement.length(); // Move past the replacement
    }


    // Apply padding for the entire markdown block
    if (style.padding.x != 0.0f || style.padding.y != 0.0f) {
        const ImVec2 cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(cursorPos.x + style.padding.x, cursorPos.y + style.padding.y));
    }

    // Apply a base text color if specified, but allow the renderer to override it
    if (style.color) {
        ImGui::PushStyleColor(ImGuiCol_Text, *style.color);
    }

    // Use our powerful MarkdownRenderer for the entire text
    static UI::MarkdownRenderer renderer;
    renderer.Render(processedText);

    if (style.color) {
        ImGui::PopStyleColor();
    }
}


} // namespace UI
SPF_NS_END
