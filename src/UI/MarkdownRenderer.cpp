#include "SPF/UI/MarkdownRenderer.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/Icons.hpp"
#include <regex>
#include <windows.h>
#include <imgui_internal.h>

SPF_NS_BEGIN

namespace UI {

MarkdownRenderer::MarkdownRenderer() {
}

void MarkdownRenderer::Render(const std::string& markdownText) {
    m_codeBlockCounter = 0;
    m_listStack.clear();
    m_style = Style(); 
    m_href.clear();
    m_isAtStartOfLine = true;

    MD_PARSER parser = {
        0, 
        MD_DIALECT_GITHUB | MD_FLAG_UNDERLINE | MD_FLAG_TABLES | MD_FLAG_TASKLISTS, 
        OnEnterBlock,
        OnLeaveBlock,
        OnEnterSpan,
        OnLeaveSpan,
        OnText,
        nullptr, 
        nullptr  
    };

    md_parse(markdownText.c_str(), (MD_SIZE)markdownText.length(), &parser, this);
}

int MarkdownRenderer::OnEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
    static_cast<MarkdownRenderer*>(userdata)->HandleBlock(type, detail, true);
    return 0;
}

int MarkdownRenderer::OnLeaveBlock(MD_BLOCKTYPE type, void* detail, void* userdata) {
    static_cast<MarkdownRenderer*>(userdata)->HandleBlock(type, detail, false);
    return 0;
}

int MarkdownRenderer::OnEnterSpan(MD_SPANTYPE type, void* detail, void* userdata) {
    static_cast<MarkdownRenderer*>(userdata)->HandleSpan(type, detail, true);
    return 0;
}

int MarkdownRenderer::OnLeaveSpan(MD_SPANTYPE type, void* detail, void* userdata) {
    static_cast<MarkdownRenderer*>(userdata)->HandleSpan(type, detail, false);
    return 0;
}

int MarkdownRenderer::OnText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    static_cast<MarkdownRenderer*>(userdata)->HandleText(type, text, size);
    return 0;
}

ImFont* MarkdownRenderer::GetFontForCurrentStyle() const {
    auto& ui = UIManager::GetInstance();
    if (m_style.hLevel == 1) return ui.GetFont("h1");
    if (m_style.hLevel == 2) return ui.GetFont("h2");
    if (m_style.hLevel == 3) return ui.GetFont("h3");
    if (m_style.isCode) return ui.GetFont("monospace");

    bool bold = m_style.isBold || m_style.hLevel >= 4;
    bool italic = m_style.isItalic;

    if (bold && italic) return ui.GetFont("bold_italic");
    if (bold) return ui.GetFont("bold");
    if (italic) return ui.GetFont("italic");

    return ui.GetFont("regular");
}

void MarkdownRenderer::EnsureNewLine() {
    if (!m_isAtStartOfLine) {
        ImGui::NewLine();
        m_isAtStartOfLine = true;
    }
}

void MarkdownRenderer::SetHref(const MD_ATTRIBUTE& attr) {
    m_href.assign(attr.text, attr.size);
    if (m_href.compare(0, 7, "color:#") == 0 && m_href.length() == 14) {
        unsigned int r, g, b;
        if (sscanf(m_href.c_str() + 7, "%02x%02x%02x", &r, &g, &b) == 3) {
            m_style.customColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        }
    }
}

void MarkdownRenderer::HandleBlock(MD_BLOCKTYPE type, void* detail, bool enter) {
    switch (type) {
        case MD_BLOCK_DOC: break;

        case MD_BLOCK_P:
            if (enter) {
                // Ensure we start a new line for a paragraph, especially inside quotes
                EnsureNewLine();
            } else {
                if (!m_isAtStartOfLine) ImGui::NewLine();
                ImGui::Spacing(); 
                m_isAtStartOfLine = true;
            }
            break;

        case MD_BLOCK_H: {
            auto hDetail = (MD_BLOCK_H_DETAIL*)detail;
            if (enter) {
                m_style.hLevel = hDetail->level;
                EnsureNewLine();
                if (hDetail->level == 1) ImGui::Spacing();
            } else {
                if (m_style.hLevel <= 2) {
                    ImGui::Separator();
                }
                m_style.hLevel = 0;
                ImGui::Spacing();
                m_isAtStartOfLine = true;
            }
            break;
        }

        case MD_BLOCK_HR:
            if (enter) {
                EnsureNewLine();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                m_isAtStartOfLine = true;
            }
            break;

        case MD_BLOCK_QUOTE:
            if (enter) {
                EnsureNewLine();
                m_style.isBlockQuote = true;
                m_quoteStartY = ImGui::GetCursorScreenPos().y;
                ImGui::Indent(16.0f);
            } else {
                float endY = ImGui::GetCursorScreenPos().y;
                ImGui::Unindent(16.0f);
                
                // Draw a continuous vertical line
                ImVec2 p = ImGui::GetCursorScreenPos(); // Current position after unindent
                ImGui::GetWindowDrawList()->AddRectFilled(
                    ImVec2(p.x + 4.0f, m_quoteStartY), 
                    ImVec2(p.x + 7.0f, endY), 
                    ImGui::GetColorU32(ImGuiCol_Separator)
                );

                m_style.isBlockQuote = false;
                ImGui::Spacing();
                m_isAtStartOfLine = true;
            }
            break;

        case MD_BLOCK_UL:
        case MD_BLOCK_OL: {
            if (enter) {
                ImGui::Spacing();
                bool isOrdered = (type == MD_BLOCK_OL);
                int start = isOrdered ? (int)((MD_BLOCK_OL_DETAIL*)detail)->start : 0;
                bool isTight = isOrdered ? ((MD_BLOCK_OL_DETAIL*)detail)->is_tight : ((MD_BLOCK_UL_DETAIL*)detail)->is_tight;
                m_listStack.push_back({isOrdered, start, isTight});
                ImGui::Indent(16.0f);
            } else {
                m_listStack.pop_back();
                ImGui::Unindent(16.0f);
                if (!m_listStack.empty() && !m_listStack.back().isTight) {
                    ImGui::Spacing();
                }
                m_isAtStartOfLine = true;
            }
            break;
        }

        case MD_BLOCK_LI: {
            auto liDetail = (MD_BLOCK_LI_DETAIL*)detail;
            if (enter) {
                EnsureNewLine();
                if (liDetail->is_task) {
                    Typography::Text(TextStyle::Medium().Color(Colors::GRAY), "%s", liDetail->task_mark == 'x' || liDetail->task_mark == 'X' ? ICON_FA_SQUARE_CHECK : ICON_FA_SQUARE);
                } else if (!m_listStack.empty() && m_listStack.back().isOrdered) {
                    Typography::Text(TextStyle::Medium().Color(Colors::GRAY), "%d.", m_listStack.back().counter++);
                } else {
                    Typography::Text(TextStyle::Medium().Color(Colors::GRAY), "%s", ICON_FA_CARET_RIGHT);
                }
                ImGui::SameLine(0, 5.0f);
                ImGui::BeginGroup();
                m_isAtStartOfLine = true;
            } else {
                ImGui::EndGroup();
                m_isAtStartOfLine = true;
            }
            break;
        }

        case MD_BLOCK_CODE:
            if (enter) {
                EnsureNewLine();
                m_style.isCode = true;
                m_isInsideCodeBlock = true;
                m_currentCodeBlockText.clear();

                ImFont* mono = UIManager::GetInstance().GetFont("monospace");
                if (mono) ImGui::PushFont(mono);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, UI::Colors::CODE_BG);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
                
                std::string child_id = "##CodeBlock" + std::to_string(m_codeBlockCounter++);
                ImGui::BeginChild(child_id.c_str(), ImVec2(-FLT_MIN, 0), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
                
                // Reserve space for the compact copy button (font height + small padding)
                float headerHeight = ImGui::GetFontSize() + 2.0f; 
                ImGui::Dummy(ImVec2(0, headerHeight)); 
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushTextWrapPos(-1.0f); // Force disable wrapping for code
                m_isAtStartOfLine = true;
            } else {
                // Render the copy button in the reserved top area
                ImVec2 currentCursor = ImGui::GetCursorPos();
                
                // Position button at the top-left of the child window padding
                ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().WindowPadding.x, ImGui::GetStyle().WindowPadding.y));
                
                // Button font is already monospace here because we pop it only at the end of this block
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 1)); // Very small padding to make it compact
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
                
                if (ImGui::Button(ICON_FA_COPY " Copy")) {
                    ImGui::SetClipboardText(m_currentCodeBlockText.c_str());
                }
                
                ImGui::PopStyleColor();
                ImGui::PopStyleVar(2);

                // Restore cursor and add dummy
                ImGui::SetCursorPos(currentCursor);
                ImGui::Dummy(ImVec2(0.0f, 0.0f)); 

                ImGui::PopTextWrapPos();
                ImGui::EndChild();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
                if (UIManager::GetInstance().GetFont("monospace")) ImGui::PopFont();
                m_style.isCode = false;
                m_isInsideCodeBlock = false;
                ImGui::Spacing();
                m_isAtStartOfLine = true;
            }
            break;

        case MD_BLOCK_TABLE:
            if (enter) {
                auto tblDetail = (MD_BLOCK_TABLE_DETAIL*)detail;
                EnsureNewLine();
                ImGui::BeginTable("##md_table", (int)tblDetail->col_count, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp);
                for (int i = 0; i < (int)tblDetail->col_count; i++) ImGui::TableSetupColumn(""); 
            } else {
                ImGui::EndTable();
                ImGui::Spacing();
                m_isAtStartOfLine = true;
            }
            break;

        case MD_BLOCK_TR:
            if (enter) ImGui::TableNextRow();
            break;

        case MD_BLOCK_TH:
        case MD_BLOCK_TD:
            if (enter) {
                ImGui::TableNextColumn();
                if (type == MD_BLOCK_TH) m_style.isBold = true;
                m_isAtStartOfLine = true;
            } else {
                if (type == MD_BLOCK_TH) m_style.isBold = false;
            }
            break;

        default: break;
    }
}

void MarkdownRenderer::HandleSpan(MD_SPANTYPE type, void* detail, bool enter) {
    switch (type) {
        case MD_SPAN_EM:     m_style.isItalic = enter; break;
        case MD_SPAN_STRONG: m_style.isBold = enter; break;
        case MD_SPAN_U:      m_style.isUnderline = enter; break;
        case MD_SPAN_DEL:    m_style.isStrikethrough = enter; break;
        case MD_SPAN_CODE:   m_style.isCode = enter; break;
        case MD_SPAN_A: {
            auto aDetail = (MD_SPAN_A_DETAIL*)detail;
            if (enter) {
                SetHref(aDetail->href);
                m_style.isLink = true;
            } else {
                m_href.clear();
                m_style.isLink = false;
                m_style.customColor = {0,0,0,0};
            }
            break;
        }
        default: break;
    }
}

void MarkdownRenderer::HandleText(MD_TEXTTYPE type, const char* text, MD_SIZE size) {
    if (size == 0) return;

    std::string textStr(text, size);
    
    // Skip the language identifier text often sent at start of code blocks in MD4C
    if (m_style.isCode && m_isAtStartOfLine && (textStr == "cpp" || textStr == "c" || textStr == "javascript" || textStr == "json")) {
        return;
    }

    // --- Custom Color Parsing ---
    static const std::regex colorRegex(R"(\{#([0-9a-fA-F]{6})\}([\s\S]*?)\{/\})");
    std::smatch match;
    if (type == MD_TEXT_NORMAL && !m_style.isCode && std::regex_search(textStr, match, colorRegex)) {
        HandleText(type, textStr.substr(0, match.position()).c_str(), (MD_SIZE)match.position());
        
        ImVec4 oldColor = m_style.customColor;
        unsigned int r, g, b;
        sscanf(match[1].str().c_str(), "%02x%02x%02x", &r, &g, &b);
        m_style.customColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
        HandleText(type, match[2].str().c_str(), (MD_SIZE)match[2].length());
        m_style.customColor = oldColor;
        
        std::string suffix = textStr.substr(match.position() + match.length());
        HandleText(type, suffix.c_str(), (MD_SIZE)suffix.length());
        return;
    }

    ImGui::PushFont(GetFontForCurrentStyle());

    bool colorPushed = false;
    if (m_style.customColor.w > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, m_style.customColor);
        colorPushed = true;
    } else if (m_style.isLink) {
        ImGui::PushStyleColor(ImGuiCol_Text, UI::Colors::URL_LINK);
        colorPushed = true;
    }
if (type == MD_TEXT_SOFTBR) {
    // Treat soft breaks as hard breaks for intuitive \n behavior in game UI
    ImGui::NewLine();
    m_isAtStartOfLine = true;
    if (m_isInsideCodeBlock) m_currentCodeBlockText += "\n";
} else if (type == MD_TEXT_BR) {
    // Hard break
    ImGui::NewLine();
    m_isAtStartOfLine = true;
    if (m_isInsideCodeBlock) m_currentCodeBlockText += "\n";
} else {
        const char* p = text;
        const char* end = text + size;

        while (p < end) {
            const char* atom_start = p;
            bool isNewline = false;

            // Identify atom (Newline, Whitespace, or Word)
            if (*p == '\n' || *p == '\r') {
                isNewline = true;
                if (*p == '\r' && (p + 1) < end && *(p + 1) == '\n') p += 2;
                else p++;
            } else if (*p == ' ' || *p == '\t') {
                while (p < end && (*p == ' ' || *p == '\t')) p++;
            } else {
                while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') p++;
            }

            if (isNewline) {
                ImGui::NewLine();
                m_isAtStartOfLine = true;
                if (m_isInsideCodeBlock) m_currentCodeBlockText += "\n";
            } else {
                float atom_width = ImGui::CalcTextSize(atom_start, p).x;
                float available = ImGui::GetContentRegionAvail().x;

                // Manual wrapping check: ONLY for non-code text or inline code.
                // For Code Blocks (which have horizontal scroll), we skip this.
                // Code blocks in our implementation are identified by m_style.isCode AND being inside a child window (checked via TextWrapPos)
                bool isCodeBlock = m_style.isCode && ImGui::GetCurrentWindow()->DC.TextWrapPos < 0.0f;

                if (!m_isAtStartOfLine && atom_width > available && !isCodeBlock) {
                    ImGui::NewLine();
                    m_isAtStartOfLine = true;
                }

                // Background for inline code
                if (m_style.isCode && m_style.hLevel == 0 && !m_style.isBlockQuote) {
                    ImVec2 cur = ImGui::GetCursorScreenPos();
                    ImVec2 sz = ImGui::CalcTextSize(atom_start, p);
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(cur.x - 1.0f, cur.y), ImVec2(cur.x + sz.x + 1.0f, cur.y + sz.y), 
                        ImGui::GetColorU32(UI::Colors::CODE_BG), 2.0f
                    );
                }

                // Render the atom
                ImGui::TextUnformatted(atom_start, p);

                // Accumulate text for copying if in a code block
                if (m_isInsideCodeBlock) {
                    m_currentCodeBlockText.append(atom_start, p - atom_start);
                }
                
                // Decorations (Underline / Strikethrough)
                if (m_style.isUnderline || m_style.isStrikethrough) {
                    ImVec2 p_min = ImGui::GetItemRectMin();
                    ImVec2 p_max = ImGui::GetItemRectMax();
                    if (m_style.isUnderline) ImGui::GetWindowDrawList()->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), ImGui::GetColorU32(ImGuiCol_Text));
                    if (m_style.isStrikethrough) ImGui::GetWindowDrawList()->AddLine(ImVec2(p_min.x, (p_min.y + p_max.y) * 0.5f), ImVec2(p_max.x, (p_min.y + p_max.y) * 0.5f), ImGui::GetColorU32(ImGuiCol_Text));
                }

                // Interactive Links
                if (m_style.isLink && !m_href.empty() && m_href.find("color:#") == std::string::npos) {
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        if (ImGui::IsItemClicked()) ShellExecute(NULL, "open", m_href.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    }
                }

                // Connect to next atom
                ImGui::SameLine(0, 0);
                m_isAtStartOfLine = false;
            }
        }
    }

    if (colorPushed) ImGui::PopStyleColor();
    ImGui::PopFont();
}

} // namespace UI
SPF_NS_END
