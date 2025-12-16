#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/UI/UIStyle.hpp" // For Colors::CODE_BG
#include "imgui_md.h"
#include "md4c.h"
#include <string>
#include <map> // For font management

// Forward declarations if necessary, but imgui_md.h should include ImGui

SPF_NS_BEGIN

namespace UI {

class MarkdownRenderer : public imgui_md {
    public:
        MarkdownRenderer();
        ~MarkdownRenderer() override = default;

        // Public method to render markdown text
        void Render(const std::string& markdownText);

    protected:
        // Override virtual functions from imgui_md for custom rendering
        ImFont* get_font() const override;
        ImVec4 get_color() const override;
        void open_url() const override;
        void BLOCK_CODE(const MD_BLOCK_CODE_DETAIL* detail, bool is_enter) override;
        void SPAN_CODE(bool is_enter) override;
        void soft_break() override;
        void BLOCK_P(bool is_enter) override;

    private:
        int m_codeBlockCounter = 0;
        // A map to store fonts for markdown elements if different from main UI fonts
        // std::map<int, ImFont*> m_fonts; 
    };

} // namespace UI
SPF_NS_END
