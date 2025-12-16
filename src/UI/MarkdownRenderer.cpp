#include "SPF/UI/MarkdownRenderer.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include <regex>                          // For preprocessing
#include <windows.h>                      // Required for ShellExecute and its dependencies

SPF_NS_BEGIN

namespace UI {

MarkdownRenderer::MarkdownRenderer() {
  // Constructor
}

void MarkdownRenderer::Render(const std::string& markdownText) {
  m_codeBlockCounter = 0;  // Reset for each render pass
  // Call the base class's render method directly.
  print(markdownText.c_str(), markdownText.c_str() + markdownText.length());
}
void MarkdownRenderer::BLOCK_CODE(const MD_BLOCK_CODE_DETAIL* detail, bool is_enter) {

  const float padding = 3.0f;

  if (is_enter) {
    ImGui::Spacing();
    ImGui::PushFont(UIManager::GetInstance().GetFont("monospace"));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UI::Colors::CODE_BG);
    ImGui::PushStyleColor(ImGuiCol_Text, UI::Colors::WHITE);

    std::string child_id = "##CodeBlock" + std::to_string(m_codeBlockCounter++);

    ImGui::BeginChild(child_id.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY);

    // Manual padding

    ImGui::Dummy(ImVec2(0.0f, padding));  // Top padding
    ImGui::Indent(padding);               // Left padding
  } else {
    ImGui::Unindent(padding);             // Remove left padding
    ImGui::Dummy(ImVec2(0.0f, padding));  // Bottom padding

    ImGui::EndChild();
    ImGui::PopStyleColor(2);  // Pop text and background color
    ImGui::PopFont();
    ImGui::Spacing();
  }
}

void MarkdownRenderer::SPAN_CODE(bool is_enter) {
  if (is_enter) {
    ImGui::PushFont(UIManager::GetInstance().GetFont("monospace"));
    ImGui::PushStyleColor(ImGuiCol_Text, UI::Colors::WHITE);  // Light gray text
  } else {
    ImGui::PopStyleColor();
    ImGui::PopFont();
  }
}

void MarkdownRenderer::soft_break() {
  ImGui::NewLine();
}

void MarkdownRenderer::BLOCK_P(bool is_enter) {
  // The base implementation adds a NewLine, which is too much space. An empty
  // function removes all space, causing elements to merge.
  // The correct approach is to add a standard spacing *after* a block element finishes rendering.
  // We do this on 'leave_block' (is_enter == false).
  // This ensures consistent spacing between paragraphs, and between paragraphs and other block elements like code blocks.
  if (!is_enter) {
    ImGui::NewLine();
  }
}

ImVec4 MarkdownRenderer::get_color() const {
  if (!m_href.empty()) {
    return UI::Colors::URL_LINK;
  }
  return imgui_md::get_color();
}

ImFont* MarkdownRenderer::get_font() const {
    // Headers have the highest priority
    if (m_hlevel == 1) return UIManager::GetInstance().GetFont("h1");
    if (m_hlevel == 2) return UIManager::GetInstance().GetFont("h2");
    if (m_hlevel == 3) return UIManager::GetInstance().GetFont("h3");

    // Handle combinations of bold/italic
    if (m_is_strong && m_is_em) {
        return UIManager::GetInstance().GetFont("bold_italic");
    }
    if (m_is_strong) {
        return UIManager::GetInstance().GetFont("bold");
    }
    if (m_is_em) {
        return UIManager::GetInstance().GetFont("italic");
    }

    // Fallback to the base implementation (which returns nullptr for the default font)
    return imgui_md::get_font();
}

void MarkdownRenderer::open_url() const {
  // This function is called when a link is clicked.
  // m_href is a member of the base imgui_md class and holds the URL.
  ShellExecute(NULL, "open", m_href.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

}  // namespace UI
SPF_NS_END
