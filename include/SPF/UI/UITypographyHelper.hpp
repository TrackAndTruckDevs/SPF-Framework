#pragma once

#include "SPF/Namespace.hpp"
#include <imgui.h>
#include <string>
#include <optional>

SPF_NS_BEGIN

namespace UI {

    enum class TextAlign {
        Left,
        Center,
        Right
    };

    /// <summary>
    /// Structure to define a text's style attributes.
    /// Use chaining methods for a fluent interface, e.g., TextStyle::Bold().Color(C_RED).
    /// </summary>
    struct TextStyle {
        std::optional<std::string> fontKey;
        std::optional<ImVec4> color;
        std::optional<ImVec4> hoverColor;
        TextAlign align = TextAlign::Left;
        ImVec2 padding = {0.0f, 0.0f};
        bool wrap = false;
        bool disabled = false;
        bool strikethrough = false;
        bool underline = false;
        bool isSeparator = false;

        // --- Factory methods for common styles ---
        static TextStyle Regular();
        static TextStyle Bold();
        static TextStyle Italic();
        static TextStyle BoldItalic();
        static TextStyle Medium();
        static TextStyle MediumItalic();
        static TextStyle Monospace();
        static TextStyle H1();
        static TextStyle H2();
        static TextStyle H3();
        
        // --- Chaining methods ---

        /**
         * @brief Sets the font for the text.
         * @param key The key of the font to use (e.g., "bold", "h1").
         */
        TextStyle& Font(const std::string& key) { fontKey = key; return *this; }

        /**
         * @brief Sets the color of the text.
         * @param col The color to apply.
         */
        TextStyle& Color(const ImVec4& col) { color = col; return *this; }

        /**
         * @brief Sets the horizontal alignment of the text.
         * @param textAlign The alignment type (Left, Center, Right).
         */
        TextStyle& Align(TextAlign textAlign) { align = textAlign; return *this; }

        /**
         * @brief Applies padding before rendering the text.
         * @param pad An ImVec2 vector representing horizontal (x) and vertical (y) padding.
         */
        TextStyle& Padding(const ImVec2& pad) { padding = pad; return *this; }

        /**
         * @brief Enables or disables automatic text wrapping.
         * @param enable True to enable wrapping, false to disable.
         */
        TextStyle& Wrapped(bool enable = true) { wrap = enable; return *this; }

        /**
         * @brief Renders the text in a disabled (grayed-out) state.
         * @param enable True to render as disabled, false otherwise.
         */
        TextStyle& Disabled(bool enable = true) { disabled = enable; return *this; }

        /**
         * @brief Renders the text with a line through it.
         * @param enable True to enable strikethrough, false to disable.
         */
        TextStyle& Strikethrough(bool enable = true) { strikethrough = enable; return *this; }

        /**
         * @brief Renders the text with an underline.
         * @param enable True to enable underline, false to disable.
         */
        TextStyle& Underline(bool enable = true) { underline = enable; return *this; }

        /**
         * @brief Renders the text as a separator with lines on the sides.
         * @param enable True to render as a separator, false otherwise.
         */
        TextStyle& Separator(bool enable = true) { isSeparator = enable; return *this; }
        
        /**
         * @brief Sets the color of the text when hovered (for interactive elements).
         * @param col The color to apply on hover.
         */
        TextStyle& HoverColor(const ImVec4& col) { hoverColor = col; return *this; }
    };

    /// <summary>
    /// RAII helper to safely push and pop ImGui state (font, color).
    /// </summary>
    class ScopedStyle {
    public:
        ScopedStyle(const TextStyle& style);
        ~ScopedStyle();
        ScopedStyle(const ScopedStyle&) = delete;
        ScopedStyle& operator=(const ScopedStyle&) = delete;

    private:
        int m_colorCount = 0;
        bool m_fontPushed = false;
    };


    /// <summary>
    /// A static helper class for rendering text with various styles and fonts.
    /// </summary>
    class Typography {
    public:
        // --- Core Rendering Methods ---
        static void Text(const char* fmt, ...);
        static void Text(const TextStyle& style, const char* fmt, ...);
        static void RenderMarkdownText(const std::string& markdownText, const TextStyle& style = {});

        // --- Core Size Calculation Method ---
        static ImVec2 CalcTextSize(const char* text, const TextStyle& style = {});

    private:
        static void TextV(const TextStyle& style, const char* fmt, va_list args); // New private core method
    };

} // namespace UI
SPF_NS_END
