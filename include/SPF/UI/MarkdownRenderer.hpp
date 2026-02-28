#pragma once

#include "SPF/Namespace.hpp"
#include <md4c.h>
#include <imgui.h>
#include <string>
#include <vector>

SPF_NS_BEGIN

namespace UI {

/**
 * @brief Custom Markdown renderer using MD4C directly.
 * @details This implementation provides full control over spacing, fonts, and custom colors.
 */
class MarkdownRenderer {
public:
    MarkdownRenderer();
    ~MarkdownRenderer() = default;

    /**
     * @brief Renders the provided markdown text to the current ImGui window.
     */
    void Render(const std::string& markdownText);

private:
    // --- Rendering Context ---
    struct Style {
        bool isBold = false;
        bool isItalic = false;
        bool isUnderline = false;
        bool isStrikethrough = false;
        bool isCode = false;
        bool isLink = false;
        bool isBlockQuote = false;
        unsigned int hLevel = 0; // 0 = no heading, 1-6 = H1-H6
        ImVec4 customColor = {0,0,0,0}; // (0,0,0,0) means no custom color
    };

    struct ListInfo {
        bool isOrdered;
        int counter;
        bool isTight;
    };

    // --- MD4C Static Callbacks ---
    static int OnEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int OnLeaveBlock(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int OnEnterSpan(MD_SPANTYPE type, void* detail, void* userdata);
    static int OnLeaveSpan(MD_SPANTYPE type, void* detail, void* userdata);
    static int OnText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata);

    // --- Internal Processing ---
    void HandleBlock(MD_BLOCKTYPE type, void* detail, bool enter);
    void HandleSpan(MD_SPANTYPE type, void* detail, bool enter);
    void HandleText(MD_TEXTTYPE type, const char* text, MD_SIZE size);

    // --- Helpers ---
    ImFont* GetFontForCurrentStyle() const;
    void ApplyStyleColor(bool push);
    void EnsureNewLine();
    void SetHref(const MD_ATTRIBUTE& attr);

    // --- Members ---
    Style m_style;
    std::vector<ListInfo> m_listStack;
    std::string m_href;
    std::string m_currentCodeBlockText; // Buffer for copying
    int m_codeBlockCounter = 0;
    float m_quoteStartY = 0.0f;
    bool m_isAtStartOfLine = true;
    bool m_isInsideCodeBlock = false; // Flag to distinguish block vs inline code
};

} // namespace UI

SPF_NS_END
