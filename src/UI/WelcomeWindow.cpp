#include "SPF/UI/WelcomeWindow.hpp"

#include "SPF/Config/FrameworkManifest.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/Resources/spf_logo.hpp"
#include "SPF/UI/UIElements.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/Utils/Windows.hpp"

#include "imgui.h"
#include "imgui_internal.h"
#include "nlohmann/json_fwd.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <regex>
#include <shellapi.h>
#include <string>
#include <system_error>

namespace SPF::UI {

// ============================================================================
// UI LAYOUT CONSTANTS - Adjust these to tune the look
// ============================================================================
namespace {
constexpr float kContentWidth = 980.0f;   // Width of the intro + cards column
constexpr float kMinMargin = 40.0f;       // Minimum gap to the screen edges
constexpr float kLogoWidth = 400.0f;      // Logo is drawn at its native width (pixel perfect)
constexpr float kTitleTopOffset = 28.0f;  // Gap between logo and title
constexpr float kIntroTopOffset = 12.0f;  // Gap between title and intro
constexpr float kCardsTopOffset = 36.0f;  // Gap between intro and cards
constexpr float kCardGap = 16.0f;         // Gap between cards
constexpr float kCardRounding = 10.0f;
constexpr ImVec2 kCardPadding = ImVec2(24.0f, 20.0f);
constexpr float kIconTileSize = 48.0f;  // Red square behind the keyboard / release notes icon
constexpr float kIconTileGap = 20.0f;   // Gap between the icon tile and its text
constexpr float kKeycapHeight = 52.0f;
constexpr float kKeycapMinWidth = 96.0f;
constexpr float kKeycapDepth = 4.0f;  // Visible "side" of the key
constexpr float kButtonTopOffset = 36.0f;
constexpr float kButtonWidth = 280.0f;
constexpr float kButtonHeight = 56.0f;
constexpr float kFooterTopOffset = 20.0f;
constexpr float kPathRowTopOffset = 6.0f;
constexpr float kPathRowHeight = 42.0f;

// --- Two-column layout (logo/title/button on the left, cards on the right) ---
constexpr float kLeftColumnWidth = 560.0f;
constexpr float kRightColumnWidth = 680.0f;
constexpr float kColumnGap = 72.0f;           // Gap on each side of the divider
constexpr float kDividerHeightRatio = 0.85f;  // Divider height relative to the tallest column
constexpr float kTwoColumnTotalWidth = kLeftColumnWidth + kColumnGap * 2.0f + 1.0f + kRightColumnWidth;
constexpr float kLeftTitleTopOffset = 32.0f;
constexpr float kLeftIntroTopOffset = 18.0f;
constexpr float kLeftButtonTopOffset = 36.0f;
constexpr float kLeftFooterTopOffset = 28.0f;
constexpr float kRightCardGap = 14.0f;

// --- Update mode (version badge + scrollable release notes card) ---
constexpr float kBadgeHeight = 32.0f;
constexpr float kBadgePaddingX = 12.0f;
constexpr float kBadgeRounding = 6.0f;
constexpr float kBadgeTextSize = 15.0f;
constexpr float kNotesMaxHeight = 420.0f;  // Notes taller than this scroll inside the card
constexpr float kNotesMinHeight = 160.0f;
constexpr float kNotesScrollbarSize = 6.0f;
constexpr float kNotesScrollbarInset = 8.0f;  // Gap between the scrollbar and the card border
constexpr float kNotesScrollInsetY = 10.0f;   // Gap between the scrollbar and the header line / card bottom

// --- Typography (pixel sizes; fonts are scalable since ImGui 1.92) ---
constexpr float kBodySize = 16.0f;       // Card text, intro, footer
constexpr float kCardTitleSize = 19.0f;  // Card titles and their icons
constexpr float kMenuIconSize = 22.0f;   // Keyboard icon inside the red tile
constexpr float kPathTextSize = 14.0f;
constexpr float kOpenTextSize = 14.0f;

constexpr float kOverlayAlpha = 0.96f;

// --- Palette (accent is the red of the SPF logo) ---
constexpr ImVec4 kColOverlay = ImVec4(0.043f, 0.043f, 0.051f, kOverlayAlpha);
constexpr ImVec4 kColTitle = ImVec4(0.949f, 0.953f, 0.961f, 1.0f);
constexpr ImVec4 kColBody = ImVec4(0.663f, 0.686f, 0.722f, 1.0f);
constexpr ImVec4 kColMuted = ImVec4(0.541f, 0.565f, 0.600f, 1.0f);
constexpr ImVec4 kColAccent = ImVec4(1.0f, 0.302f, 0.302f, 1.0f);
constexpr ImVec4 kColAccentSoft = ImVec4(1.0f, 0.176f, 0.176f, 0.12f);
constexpr ImVec4 kColCardBg = ImVec4(0.075f, 0.078f, 0.094f, 1.0f);
constexpr ImVec4 kColCardBorder = ImVec4(0.141f, 0.149f, 0.169f, 1.0f);
constexpr ImVec4 kColHighlightBg = ImVec4(0.086f, 0.090f, 0.106f, 1.0f);
constexpr ImVec4 kColHighlightBorder = ImVec4(0.165f, 0.173f, 0.196f, 1.0f);
constexpr ImVec4 kColKeyFace = ImVec4(0.141f, 0.149f, 0.173f, 1.0f);
constexpr ImVec4 kColKeyEdge = ImVec4(0.227f, 0.239f, 0.271f, 1.0f);
constexpr ImVec4 kColButton = ImVec4(0.851f, 0.102f, 0.102f, 1.0f);
constexpr ImVec4 kColButtonHovered = ImVec4(0.910f, 0.200f, 0.200f, 1.0f);
constexpr ImVec4 kColPathBg = ImVec4(0.043f, 0.043f, 0.051f, 1.0f);
constexpr ImVec4 kColPathText = ImVec4(0.835f, 0.847f, 0.867f, 1.0f);
constexpr ImVec4 kColOpenBg = ImVec4(0.110f, 0.114f, 0.133f, 1.0f);
constexpr ImVec4 kColOpenBgHovered = ImVec4(0.153f, 0.161f, 0.184f, 1.0f);
constexpr ImVec4 kColOpenBgActive = ImVec4(0.086f, 0.090f, 0.106f, 1.0f);
constexpr ImVec4 kColOpenBorder = ImVec4(0.180f, 0.188f, 0.212f, 1.0f);
constexpr ImVec4 kColOpenText = ImVec4(0.788f, 0.804f, 0.827f, 1.0f);
constexpr ImVec4 kColBadgeBorder = ImVec4(1.0f, 0.302f, 0.302f, 0.40f);
constexpr ImVec4 kColScrollTrack = kColOpenBg;  // Same greys as the "Open" button and the keycap edge
constexpr ImVec4 kColScrollGrab = kColKeyEdge;
constexpr ImVec4 kColScrollGrabHovered = ImVec4(0.290f, 0.306f, 0.341f, 1.0f);

/** @brief Text centered in a rect, drawn with the given style (font, size, colour). */
void DrawCenteredText(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, const char* text, const TextStyle& style) {
  ScopedStyle scoped(style);
  const ImVec2 textSize = ImGui::CalcTextSize(text);
  const ImVec2 textPos(min.x + (max.x - min.x - textSize.x) * 0.5f, min.y + (max.y - min.y - textSize.y) * 0.5f);
  drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(ImGuiCol_Text), text);
}

/**
 * @brief Renders a rounded card as a child window and returns its measured content height.
 * @param height Fixed height, or <= 0 to let the card size itself to its content.
 * @param body Callable receiving the inner (padded) width of the card.
 * @return Height the content needs (including padding), or 0 if the card was clipped.
 */
template <typename Body>
float RenderCard(const char* id, float width, float height, bool highlight, Body&& body) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, highlight ? kColHighlightBg : kColCardBg);
  ImGui::PushStyleColor(ImGuiCol_Border, highlight ? kColHighlightBorder : kColCardBorder);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Scaled(kCardRounding));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Scaled(kCardPadding));

  ImGuiChildFlags childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding;
  if (height <= 0.0f) childFlags |= ImGuiChildFlags_AutoResizeY;

  float contentHeight = 0.0f;
  if (ImGui::BeginChild(id, ImVec2(width, height > 0.0f ? height : 0.0f), childFlags, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    body(ImGui::GetContentRegionAvail().x);
    contentHeight = ImGui::GetCursorPosY() - ImGui::GetStyle().ItemSpacing.y + Scaled(kCardPadding).y;
  }
  ImGui::EndChild();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
  return contentHeight;
}

/** @brief Card header: red icon followed by a bold title. */
void RenderCardHeader(const char* icon, const std::string& title) {
  Typography::Text(TextStyle::Bold().Size(kCardTitleSize).Color(kColAccent), "%s", icon);
  ImGui::SameLine(0.0f, Scaled(10.0f));
  Typography::Text(TextStyle::Bold().Size(kCardTitleSize).Color(kColTitle).Wrapped(), "%s", title.c_str());
}

/** @brief A standard grid card: header + wrapped markdown body. */
void RenderSectionBody(const char* icon, const std::string& title, const std::string& markdown) {
  RenderCardHeader(icon, title);
  ImGui::Dummy(ImVec2(0.0f, Scaled(4.0f)));
  Typography::RenderMarkdownText(markdown, TextStyle::Regular().Size(kBodySize).Wrapped().Color(kColBody));
}

/**
 * @brief Renders wrapped markdown inside an invisible child window of exactly `width`.
 * @details The markdown renderer wraps on the window's content region, not on PushTextWrapPos(), so a block narrower
 *          than its window must live in its own child window, otherwise words overflow and get split letter by letter.
 * @return Width actually used by the rendered text.
 */
float RenderMarkdownBlock(const char* id, const std::string& markdown, const TextStyle& style, float width) {
  float usedWidth = 0.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginChild(id, ImVec2(width, 0.0f), ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    Typography::RenderMarkdownText(markdown, TextStyle(style).Wrapped());
    usedWidth = ImGui::GetItemRectSize().x;
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  return usedWidth;
}

/**
 * @brief Renders markdown centered on centerX when it fits on one line, otherwise left-aligned in a maxWidth column.
 * @param lastWidth Width measured on the previous frame (updated in place). Markdown width is only known after rendering.
 */
void RenderCenteredMarkdown(const char* id, const std::string& markdown, const TextStyle& style, float centerX, float maxWidth, float& lastWidth) {
  const bool center = lastWidth > 0.0f && lastWidth < maxWidth * 0.9f;
  const float width = center ? lastWidth + 2.0f : maxWidth;  // +2: rounding margin so the single line never re-wraps
  ImGui::SetCursorPosX(std::floor(centerX - width * 0.5f));

  const float used = RenderMarkdownBlock(id, markdown, style, width);
  if (used > 0.0f) lastWidth = used;
}

/** @brief Size of a keycap for the given label (font "h1" must be used when drawing). */
ImVec2 CalcKeycapSize(const char* label) {
  const ImVec2 textSize = Typography::CalcTextSize(label, TextStyle::Bold().Font("h1"));
  return ImVec2(std::max(Scaled(kKeycapMinWidth), std::ceil(textSize.x) + Scaled(32.0f)), Scaled(kKeycapHeight));
}

/** @brief Draws a keyboard key: a lighter bottom edge with the key face on top. */
void DrawKeycap(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size, const char* label) {
  const ImVec2 max(pos.x + size.x, pos.y + size.y);
  const ImVec2 faceMax(max.x, max.y - Scaled(kKeycapDepth));
  drawList->AddRectFilled(pos, max, ImGui::GetColorU32(kColKeyEdge), Scaled(8.0f));
  drawList->AddRectFilled(pos, faceMax, ImGui::GetColorU32(kColKeyFace), Scaled(8.0f));
  drawList->AddRect(pos, max, ImGui::GetColorU32(kColKeyEdge), Scaled(8.0f));

  ScopedStyle font(TextStyle::Bold().Font("h1"));
  const ImVec2 textSize = ImGui::CalcTextSize(label);
  const ImVec2 textPos(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - Scaled(kKeycapDepth) - textSize.y) * 0.5f);
  drawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), textPos, ImGui::GetColorU32(Colors::WHITE), label);
}

/**
 * @brief Restyles the server changelog for the update card.
 * @details The shared markdown renderer draws headings and bold text in the body colour. Here section titles are shown
 *          as bold white lines (same size as the text) and bold words in white, as on the welcome cards.
 */
std::string StyleChangelog(const std::string& markdown) {
  static const std::regex kBold(R"(\*\*([^*\n]+)\*\*)");
  static const std::regex kHeading(R"((^|\n)#{1,6}[ \t]+([^\n]*?)[ \t]*#*[ \t]*(?=\n|$))");
  std::string out = std::regex_replace(markdown, kHeading, "$1<#F2F3F5>@@HEADING@@$2@@HEADING@@</>\n");
  out = std::regex_replace(out, kBold, "<#FFFFFF>**$1**</>");
  // Headings become bold lines; done after the bold pass so they are not wrapped in a second colour tag.
  for (size_t pos = 0; (pos = out.find("@@HEADING@@", pos)) != std::string::npos;) out.replace(pos, 11, "**");
  return out;
}

/** @brief Size of the version badge for the given label. */
ImVec2 CalcBadgeSize(const char* label) {
  const ImVec2 textSize = Typography::CalcTextSize(label, TextStyle::Bold().Size(kBadgeTextSize));
  return ImVec2(std::ceil(textSize.x) + Scaled(kBadgePaddingX) * 2.0f, Scaled(kBadgeHeight));
}

/** @brief Draws the red-tinted version badge at the cursor (window-local x) and advances the cursor. */
void RenderVersionBadge(float x, const char* label) {
  const ImVec2 size = CalcBadgeSize(label);
  ImGui::SetCursorPosX(std::floor(x));
  const ImVec2 cursor = ImGui::GetCursorScreenPos();
  const ImVec2 min(std::floor(cursor.x), std::floor(cursor.y));  // Whole pixels: a 1px border on a half pixel looks blurry
  ImGui::SetCursorScreenPos(min);
  const ImVec2 max(min.x + size.x, min.y + size.y);
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  drawList->AddRectFilled(min, max, ImGui::GetColorU32(kColAccentSoft), Scaled(kBadgeRounding));
  drawList->AddRect(min, max, ImGui::GetColorU32(kColBadgeBorder), Scaled(kBadgeRounding));
  DrawCenteredText(drawList, min, max, label, TextStyle::Bold().Size(kBadgeTextSize).Color(Colors::WHITE));
  ImGui::Dummy(size);
}
}  // namespace
// ============================================================================

WelcomeWindow::WelcomeWindow(const std::string& componentName, const std::string& windowId) : m_componentName(componentName), m_windowId(windowId), m_langSink(Localization::LocalizationManager::GetInstance().OnFrameworkLanguageChanged) {
  m_isVisible = false;
  m_langSink.Connect<&WelcomeWindow::OnFrameworkLanguageChanged>(this);
  UpdateLocalization();
}

void WelcomeWindow::OnFrameworkLanguageChanged(const std::string& /*componentName*/) {
  UpdateLocalization();
  // Text widths change with the language: re-measure instead of centering on stale values.
  m_introWidth = 0.0f;
  m_footerWidth = 0.0f;
}

void WelcomeWindow::UpdateLocalization() {
  auto& loc = Localization::LocalizationManager::GetInstance();
  const std::string comp = "framework";

  m_locTitle = loc.Get(comp, "welcome_window.title");
  m_locIntroTitle = loc.Get(comp, "welcome_window.intro_title");
  m_locIntroText = loc.Get(comp, "welcome_window.intro_text");

  m_locSection1Title = loc.Get(comp, "welcome_window.section1.title");
  m_locSection2Title = loc.Get(comp, "welcome_window.section2.title");
  m_locSection2Text = loc.Get(comp, "welcome_window.section2.text");
  m_locSection3Title = loc.Get(comp, "welcome_window.section3.title");
  m_locSection3Text = loc.Get(comp, "welcome_window.section3.text");
  m_locSection4Title = loc.Get(comp, "welcome_window.section4.title");
  m_locSection5Title = loc.Get(comp, "welcome_window.section5.title");
  m_locSection5Text = loc.Get(comp, "welcome_window.section5.text");

  m_locFooter = loc.Get(comp, "welcome_window.footer");
  m_locBtnGo = loc.Get(comp, "welcome_window.button_go");
  m_locMenuKey = loc.Get(comp, "welcome_window.menu_key");
  m_locOpenFolder = loc.Get(comp, "welcome_window.open_folder");

  m_locUpdateTitle = loc.Get(comp, "welcome_window.update.title");
  m_locUpdateText = loc.Get(comp, "welcome_window.update.text");
  m_locUpdateNotesTitle = loc.Get(comp, "welcome_window.update.notes_title");
  m_locUpdateButton = loc.Get(comp, "welcome_window.update.button");
}

void WelcomeWindow::InitializeResources() {
  if (m_resourcesInitialized) return;

  auto renderer = UIManager::GetInstance().GetRenderer();
  if (renderer) {
    m_logoTexture = renderer->CreateTextureFromMemory(Resources::spf_logo_data, Resources::spf_logo_size);
    m_resourcesInitialized = true;
  }
}

void WelcomeWindow::Render() {
  if (!m_isVisible) {
    m_wasVisibleLastFrame = false;
    return;
  }

  // Play transition on the first frame of appearance
  if (!m_wasVisibleLastFrame) {
    UIManager::GetInstance().PlayTransition(2, 0.4f, false, 0);  // CROSS (Dip to black), 0.4s, BLACK
    m_wasVisibleLastFrame = true;
  }

  InitializeResources();

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);

  // Background style
  ImGui::PushStyleColor(ImGuiCol_WindowBg, kColOverlay);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  // No NoScrollbar: on small screens the content may not fit and the overlay must scroll.
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  if (ImGui::Begin("WelcomeOverlay", nullptr, flags)) {
    m_isFocused = ImGui::IsWindowFocused();
    RenderContent();
  }
  ImGui::End();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor();
}

void WelcomeWindow::RenderContent() {
  // Two columns whenever they fit, otherwise everything stacked in one centered column (small screens).
  if (ImGui::GetIO().DisplaySize.x >= Scaled(kTwoColumnTotalWidth) + Scaled(kMinMargin) * 2.0f) {
    RenderTwoColumnLayout();
  } else {
    RenderStackedLayout();
  }
}

void WelcomeWindow::RenderStackedLayout() {
  const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
  // Every position is floored: rects, lines and images drawn on half pixels look blurry.
  const float centerX = std::floor(screenSize.x * 0.5f);
  const float contentWidth = std::floor(std::min(Scaled(kContentWidth), screenSize.x - Scaled(kMinMargin) * 2.0f));
  const float contentX = std::floor(centerX - contentWidth * 0.5f);

  // Center the whole block vertically, using the height measured on the previous frame.
  const float topY = std::floor(std::max(Scaled(kMinMargin), (screenSize.y - m_lastLayoutHeight) * 0.5f));
  ImGui::SetCursorPosY(topY);

  // 1. Logo
  if (m_logoTexture) {
    const float aspect = (float)m_logoTexture->GetHeight() / (float)m_logoTexture->GetWidth();
    ImGui::SetCursorPosX(std::floor(centerX - Scaled(kLogoWidth) * 0.5f));
    ImGui::Image(m_logoTexture->GetHandle(), ImVec2(Scaled(kLogoWidth), Scaled(kLogoWidth) * aspect));
  }

  // 2. Title
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kTitleTopOffset));
  const bool isUpdate = m_mode == WelcomeMode::FrameworkUpdate;
  Typography::Text(TextStyle::H1().Font("h1_large_bold").Color(kColTitle).Align(TextAlign::Center), "%s", (isUpdate ? m_locUpdateTitle : m_locTitle).c_str());

  // 3. Intro (first install) or version badge + thanks (update)
  if (!isUpdate) {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kIntroTopOffset));
    Typography::Text(TextStyle::Medium().Color(kColTitle).Align(TextAlign::Center), "%s", m_locIntroTitle.c_str());
    ImGui::Dummy(ImVec2(0.0f, Scaled(2.0f)));
    RenderCenteredMarkdown("##WelcomeIntro", m_locIntroText, TextStyle::Regular().Size(kBodySize).Color(kColBody), centerX, contentWidth, m_introWidth);
  } else {
    const std::string badge = GetVersionLabel();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kIntroTopOffset));
    RenderVersionBadge(centerX - CalcBadgeSize(badge.c_str()).x * 0.5f, badge.c_str());
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kIntroTopOffset));
    RenderCenteredMarkdown("##WelcomeUpdateText", m_locUpdateText, TextStyle::Regular().Size(kBodySize).Color(kColBody), centerX, contentWidth, m_introWidth);
  }

  // 4. Cards
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kCardsTopOffset));
  if (m_mode == WelcomeMode::FirstInstall) {
    RenderFirstInstallCards(contentX, contentWidth, false);
  } else {
    const float maxNotesHeight = std::min(Scaled(kNotesMaxHeight), std::max(Scaled(kNotesMinHeight), std::floor(screenSize.y * 0.4f)));
    RenderUpdateCard(contentX, contentWidth, maxNotesHeight);
  }

  // 5. Button + footer
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kButtonTopOffset));
  RenderGoButton(std::floor(centerX - Scaled(kButtonWidth) * 0.5f));
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kFooterTopOffset));
  RenderFooter(Scaled(kMinMargin), screenSize.x - Scaled(kMinMargin) * 2.0f, true);

  m_lastLayoutHeight = ImGui::GetCursorPosY() - topY;
}

void WelcomeWindow::RenderTwoColumnLayout() {
  const ImVec2 screenSize = ImGui::GetIO().DisplaySize;
  // Every position is floored: rects, lines and images drawn on half pixels look blurry.
  const float leftX = std::floor((screenSize.x - Scaled(kTwoColumnTotalWidth)) * 0.5f);
  const float dividerX = leftX + Scaled(kLeftColumnWidth) + Scaled(kColumnGap);
  const float rightX = dividerX + Scaled(1.0f) + Scaled(kColumnGap);

  // Both columns are centered on the same horizontal axis, using the heights measured on the previous frame.
  const float rowHeight = std::max(m_leftColumnHeight, m_rightColumnHeight);
  const float topY = std::floor(std::max(Scaled(kMinMargin), (screenSize.y - rowHeight) * 0.5f));

  // --- Left column: logo, title, intro, button, footer ---
  ImGui::SetCursorPos(ImVec2(leftX, topY + std::floor(std::max(0.0f, rowHeight - m_leftColumnHeight) * 0.5f)));
  ImGui::BeginGroup();
  {
    if (m_logoTexture) {
      const float aspect = (float)m_logoTexture->GetHeight() / (float)m_logoTexture->GetWidth();
      ImGui::Image(m_logoTexture->GetHandle(), ImVec2(Scaled(kLogoWidth), Scaled(kLogoWidth) * aspect));
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftTitleTopOffset));
    ImGui::PushTextWrapPos(leftX + Scaled(kLeftColumnWidth));
    const bool isUpdate = m_mode == WelcomeMode::FrameworkUpdate;
    Typography::Text(TextStyle::H1().Font("h1_large_bold").Color(kColTitle), "%s", (isUpdate ? m_locUpdateTitle : m_locTitle).c_str());

    if (!isUpdate) {
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftIntroTopOffset));
      Typography::Text(TextStyle::Medium().Color(kColTitle), "%s", m_locIntroTitle.c_str());
      ImGui::PopTextWrapPos();
      ImGui::Dummy(ImVec2(0.0f, Scaled(4.0f)));
      RenderMarkdownBlock("##WelcomeIntro", m_locIntroText, TextStyle::Regular().Size(kBodySize).Color(kColBody), Scaled(kLeftColumnWidth));
    } else {
      ImGui::PopTextWrapPos();
      const std::string badge = GetVersionLabel();
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftIntroTopOffset));
      RenderVersionBadge(leftX, badge.c_str());
      ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftIntroTopOffset));
      ImGui::SetCursorPosX(leftX);
      RenderMarkdownBlock("##WelcomeUpdateText", m_locUpdateText, TextStyle::Regular().Size(kBodySize).Color(kColBody), Scaled(kLeftColumnWidth));
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftButtonTopOffset));
    RenderGoButton(leftX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Scaled(kLeftFooterTopOffset));
    RenderFooter(leftX, Scaled(kLeftColumnWidth), false);
  }
  ImGui::EndGroup();
  const float leftHeight = ImGui::GetItemRectSize().y;

  // --- Right column: cards ---
  ImGui::SetCursorPos(ImVec2(rightX, topY + std::floor(std::max(0.0f, rowHeight - m_rightColumnHeight) * 0.5f)));
  ImGui::BeginGroup();
  if (m_mode == WelcomeMode::FirstInstall) {
    RenderFirstInstallCards(rightX, Scaled(kRightColumnWidth), true);
  } else {
    // The card always fits on screen: longer notes scroll inside it.
    const float maxNotesHeight = std::min(Scaled(kNotesMaxHeight), std::max(Scaled(kNotesMinHeight), screenSize.y - Scaled(kMinMargin) * 2.0f - Scaled(80.0f)));
    RenderUpdateCard(rightX, Scaled(kRightColumnWidth), maxNotesHeight);
  }
  ImGui::EndGroup();
  const float rightHeight = ImGui::GetItemRectSize().y;

  // --- Divider between the columns ---
  if (rowHeight > 0.0f) {
    const ImVec2 windowPos = ImGui::GetWindowPos();
    const float dividerHeight = std::floor(rowHeight * kDividerHeightRatio);
    const float dividerTop = std::floor(windowPos.y - ImGui::GetScrollY() + topY + (rowHeight - dividerHeight) * 0.5f);
    const float lineX = std::floor(windowPos.x + dividerX) + 0.5f;  // +0.5: a 1px line exactly on one pixel column
    ImGui::GetWindowDrawList()->AddLine(ImVec2(lineX, dividerTop), ImVec2(lineX, dividerTop + dividerHeight), ImGui::GetColorU32(kColCardBorder));
  }

  m_leftColumnHeight = leftHeight;
  m_rightColumnHeight = rightHeight;

  // Keep a bottom margin when the overlay has to scroll.
  ImGui::SetCursorPos(ImVec2(0.0f, topY + std::max(leftHeight, rightHeight)));
  ImGui::Dummy(ImVec2(0.0f, Scaled(kMinMargin)));
}

void WelcomeWindow::RenderFirstInstallCards(float contentX, float contentWidth, bool twoColumnLayout) {
  auto& loc = Localization::LocalizationManager::GetInstance();
  const std::string comp = "framework";
  const auto& manifest = Config::GetFrameworkManifestData();
  const float spacingY = ImGui::GetStyle().ItemSpacing.y;
  const float cardGap = twoColumnLayout ? Scaled(kRightCardGap) : Scaled(kCardGap);

  // --- Highlight card: how to open the menu, with the key drawn as a keycap ---
  const std::string section1Text = loc.Get(comp, "welcome_window.section1.text");
  {
    ImGui::SetCursorPosX(contentX);
    RenderCard("##WelcomeMenuKey", contentWidth, 0.0f, false, [&](float innerWidth) {
      const ImVec2 origin = ImGui::GetCursorScreenPos();
      const ImVec2 startPos = ImGui::GetCursorPos();
      const float gap = Scaled(kIconTileGap);
      const ImVec2 keySize = CalcKeycapSize(m_locMenuKey.c_str());
      const float textX = startPos.x + Scaled(kIconTileSize) + gap;
      const float textWidth = innerWidth - Scaled(kIconTileSize) - gap * 2.0f - keySize.x;

      // The text is centered on the keycap row using its height measured on the previous frame.
      const float rowHeightEstimate = std::max({m_menuKeyTextHeight, Scaled(kIconTileSize), keySize.y});
      const float textY = startPos.y + std::floor((rowHeightEstimate - m_menuKeyTextHeight) * 0.5f);
      ImGui::SetCursorPos(ImVec2(textX, textY));
      ImGui::BeginGroup();
      ImGui::PushTextWrapPos(textX + textWidth);
      Typography::Text(TextStyle::Bold().Size(kCardTitleSize).Color(kColTitle), "%s", m_locSection1Title.c_str());
      ImGui::PopTextWrapPos();
      ImGui::Dummy(ImVec2(0.0f, Scaled(2.0f)));
      RenderMarkdownBlock("##WelcomeMenuKeyText", section1Text, TextStyle::Regular().Size(kBodySize).Color(kColBody), textWidth);
      ImGui::EndGroup();

      const float textHeight = ImGui::GetItemRectSize().y;
      if (textHeight > 0.0f) m_menuKeyTextHeight = textHeight;
      const float rowHeight = std::max({textHeight + (textY - startPos.y), Scaled(kIconTileSize), keySize.y});
      ImDrawList* drawList = ImGui::GetWindowDrawList();

      // Icon tile (left, vertically centered)
      const ImVec2 tileMin(origin.x, origin.y + std::floor((rowHeight - Scaled(kIconTileSize)) * 0.5f));
      const ImVec2 tileMax(tileMin.x + Scaled(kIconTileSize), tileMin.y + Scaled(kIconTileSize));
      drawList->AddRectFilled(tileMin, tileMax, ImGui::GetColorU32(kColAccentSoft), Scaled(kCardRounding));
      DrawCenteredText(drawList, tileMin, tileMax, ICON_FA_KEYBOARD, TextStyle::Bold().Size(kMenuIconSize).Color(kColAccent));

      // Keycap (right, vertically centered)
      DrawKeycap(drawList, ImVec2(std::floor(origin.x + innerWidth - keySize.x), origin.y + std::floor((rowHeight - keySize.y) * 0.5f)), keySize, m_locMenuKey.c_str());

      // Register the row's full extent so the card sizes itself correctly.
      ImGui::SetCursorPos(startPos);
      ImGui::Dummy(ImVec2(innerWidth, rowHeight));
    });
  }

  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cardGap - spacingY);

  // --- Grid cards ---
  const std::string pluginsPath = System::PathManager::GetPluginsPath().string();
  std::string iconGithub = ICON_FA_GITHUB;
  std::string githubUrl = manifest.info.githubUrl.value_or("");
  const std::string section4Text = loc.GetFormatted(comp, "welcome_window.section4.text", iconGithub, githubUrl);

  struct GridCard {
    const char* id;
    const char* icon;
    const std::string& title;
    const std::string& text;
  };
  const GridCard cards[4] = {
    {"##WelcomePlugins", ICON_FA_FOLDER_OPEN, m_locSection2Title, m_locSection2Text},
    {"##WelcomeSettings", ICON_FA_SLIDERS, m_locSection3Title, m_locSection3Text},
    {"##WelcomeDevelopers", ICON_FA_CODE, m_locSection4Title, section4Text},
    {"##WelcomeCommunity", ICON_FA_USERS, m_locSection5Title, m_locSection5Text},
  };

  // Stacked layout: a 2x2 grid. Two-column layout (narrower column): plugins and settings on their own rows, then
  // developers + community side by side. Cards sharing a row get the height of the taller one (measured last frame).
  struct Row {
    int first;
    int count;
  };
  static constexpr Row kGridRows[] = {{0, 2}, {2, 2}};
  static constexpr Row kColumnRows[] = {{0, 1}, {1, 1}, {2, 2}};
  const Row* rows = twoColumnLayout ? kColumnRows : kGridRows;
  const int rowCount = twoColumnLayout ? 3 : 2;

  for (int row = 0; row < rowCount; ++row) {
    const Row& r = rows[row];
    const float cardWidth = std::floor((contentWidth - cardGap * static_cast<float>(r.count - 1)) / static_cast<float>(r.count));
    float rowHeight = 0.0f;
    if (r.count > 1) {
      for (int i = 0; i < r.count; ++i) rowHeight = std::max(rowHeight, m_gridCardHeights[r.first + i]);
    }

    ImGui::SetCursorPosX(contentX);
    for (int i = 0; i < r.count; ++i) {
      const int index = r.first + i;
      const GridCard& card = cards[index];
      if (i > 0) ImGui::SameLine(0.0f, cardGap);
      // The last card takes the remaining pixel(s) so the row ends exactly where the full-width cards end.
      const float width = (i == r.count - 1) ? contentWidth - (cardWidth + cardGap) * static_cast<float>(r.count - 1) : cardWidth;
      const float measured = RenderCard(card.id, width, rowHeight, false, [&](float) {
        RenderSectionBody(card.icon, card.title, card.text);
        if (index == 0) RenderPluginsPathRow(pluginsPath);
      });
      if (measured > 0.0f) m_gridCardHeights[index] = measured;
    }
    if (row < rowCount - 1) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cardGap - spacingY);
  }
}

void WelcomeWindow::RenderPluginsPathRow(const std::string& path) {
  ImGui::Dummy(ImVec2(0.0f, Scaled(kPathRowTopOffset)));

  const float width = ImGui::GetContentRegionAvail().x;
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const ImVec2 rowMax(origin.x + width, origin.y + Scaled(kPathRowHeight));
  ImDrawList* drawList = ImGui::GetWindowDrawList();

  drawList->AddRectFilled(origin, rowMax, ImGui::GetColorU32(kColPathBg), Scaled(6.0f));
  drawList->AddRect(origin, rowMax, ImGui::GetColorU32(kColCardBorder), Scaled(6.0f));

  // --- "Open" button (right): opens the plugins folder in the Explorer ---
  const std::string label = std::string(ICON_FA_FOLDER_OPEN) + "  " + m_locOpenFolder;
  const TextStyle labelStyle = TextStyle::Medium().Size(kOpenTextSize).Color(kColOpenText);
  const ImVec2 labelSize = Typography::CalcTextSize(label.c_str(), labelStyle);
  const ImVec2 buttonSize(std::ceil(labelSize.x) + Scaled(24.0f), Scaled(kPathRowHeight) - Scaled(12.0f));
  const ImVec2 buttonMin(std::floor(rowMax.x - Scaled(6.0f) - buttonSize.x), std::floor(origin.y + Scaled(6.0f)));
  const ImVec2 buttonMax(buttonMin.x + buttonSize.x, buttonMin.y + buttonSize.y);

  ImGui::SetCursorScreenPos(buttonMin);
  if (ImGui::InvisibleButton("##WelcomeOpenPlugins", buttonSize)) {
    std::error_code ec;
    std::filesystem::create_directories(path, ec);  // The folder may not exist yet on a fresh install
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  }
  const bool hovered = ImGui::IsItemHovered();
  const bool held = ImGui::IsItemActive();
  if (hovered) ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

  drawList->AddRectFilled(buttonMin, buttonMax, ImGui::GetColorU32(held ? kColOpenBgActive : hovered ? kColOpenBgHovered : kColOpenBg), Scaled(5.0f));
  drawList->AddRect(buttonMin, buttonMax, ImGui::GetColorU32(kColOpenBorder), Scaled(5.0f));
  DrawCenteredText(drawList, buttonMin, buttonMax, label.c_str(), labelStyle);

  // --- Path (left), cut with "..." when it does not fit; the full path is shown in a tooltip ---
  const TextStyle pathStyle = TextStyle::Monospace().Size(kPathTextSize).Color(kColPathText);
  const ImVec2 textMin(origin.x + Scaled(12.0f), origin.y);
  const ImVec2 textMax(buttonMin.x - Scaled(10.0f), rowMax.y);
  {
    ScopedStyle scoped(pathStyle);
    const ImVec2 pathSize = ImGui::CalcTextSize(path.c_str());
    const ImVec2 textPos(textMin.x, origin.y + (Scaled(kPathRowHeight) - pathSize.y) * 0.5f);
    ImGui::RenderTextEllipsis(drawList, textPos, ImVec2(textMax.x, textPos.y + pathSize.y), textMax.x, path.c_str(), nullptr, &pathSize);

    if (pathSize.x > textMax.x - textMin.x) {
      ImGui::SetCursorScreenPos(textMin);
      ImGui::InvisibleButton("##WelcomePathTooltip", ImVec2(std::max(1.0f, textMax.x - textMin.x), Scaled(kPathRowHeight)));
      ImGui::SetItemTooltip("%s", path.c_str());
    }
  }

  // Register the row's full extent.
  ImGui::SetCursorScreenPos(origin);
  ImGui::Dummy(ImVec2(width, Scaled(kPathRowHeight)));
}

void WelcomeWindow::RenderUpdateCard(float contentX, float contentWidth, float maxNotesHeight) {
  ImGui::SetCursorPosX(contentX);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, kColCardBg);
  ImGui::PushStyleColor(ImGuiCol_Border, kColCardBorder);
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Scaled(kCardRounding));
  ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  if (ImGui::BeginChild("##WelcomeUpdate", ImVec2(contentWidth, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
    // --- Header: same tile, gap and padding as the menu-key card, centered, with a full-width line below ---
    const TextStyle titleStyle = TextStyle::Bold().Size(kCardTitleSize).Color(kColTitle);
    const ImVec2 titleSize = Typography::CalcTextSize(m_locUpdateNotesTitle.c_str(), titleStyle);
    const float tileSize = Scaled(kIconTileSize);
    const float gap = Scaled(kIconTileGap);
    const float headerHeight = std::floor(Scaled(kCardPadding).y * 2.0f + std::max(tileSize, titleSize.y));
    const float startX = std::floor(std::max(0.0f, contentWidth - (tileSize + gap + titleSize.x)) * 0.5f);

    const ImVec2 cardPos = ImGui::GetWindowPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 tileMin(std::floor(cardPos.x + startX), std::floor(cardPos.y + (headerHeight - tileSize) * 0.5f));
    const ImVec2 tileMax(tileMin.x + tileSize, tileMin.y + tileSize);
    drawList->AddRectFilled(tileMin, tileMax, ImGui::GetColorU32(kColAccentSoft), Scaled(kCardRounding));
    DrawCenteredText(drawList, tileMin, tileMax, ICON_FA_FILE_LINES, TextStyle::Bold().Size(kMenuIconSize).Color(kColAccent));

    ImGui::SetCursorPos(ImVec2(startX + tileSize + gap, std::floor((headerHeight - titleSize.y) * 0.5f)));
    Typography::Text(titleStyle, "%s", m_locUpdateNotesTitle.c_str());

    const float lineY = std::floor(cardPos.y + headerHeight) + 0.5f;  // +0.5: a 1px line exactly on one pixel row
    drawList->AddLine(ImVec2(cardPos.x, lineY), ImVec2(cardPos.x + contentWidth, lineY), ImGui::GetColorU32(kColCardBorder));

    // --- Notes: sized to their content up to maxNotesHeight, then scroll inside the card ---
    // The notes child starts kNotesScrollInsetY below the line so the scrollbar never touches it or the card bottom.
    const float insetY = Scaled(kNotesScrollInsetY);
    ImGui::SetCursorPos(ImVec2(0.0f, headerHeight + 1.0f + insetY));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, kColScrollTrack);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, kColScrollGrab);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, kColScrollGrabHovered);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, kColScrollGrabHovered);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Scaled(kCardPadding).x, Scaled(kCardPadding).y - insetY));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Scaled(kNotesScrollbarSize));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, Scaled(kNotesScrollbarSize) * 0.5f);

    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(FLT_MAX, maxNotesHeight));
    if (ImGui::BeginChild("##WelcomeUpdateNotes", ImVec2(contentWidth - Scaled(kNotesScrollbarInset), 0.0f), ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysUseWindowPadding)) {
      Typography::RenderMarkdownText(m_updateChangelog, TextStyle::Regular().Size(kBodySize).Wrapped().Color(kColBody));
    }
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0.0f, insetY - ImGui::GetStyle().ItemSpacing.y));  // Bottom inset (ItemSpacing already added)

    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor(5);
  }
  ImGui::EndChild();

  ImGui::PopStyleVar(3);
  ImGui::PopStyleColor(2);
}

void WelcomeWindow::RenderGoButton(float buttonX) {
  // Update mode: "CONTINUE ->" instead of the truck + "LET'S GO!".
  const std::string label = m_mode == WelcomeMode::FrameworkUpdate ? m_locUpdateButton + " " + ICON_FA_ARROW_RIGHT : std::string(ICON_FA_TRUCK_FAST) + " " + m_locBtnGo;

  ImGui::PushStyleColor(ImGuiCol_Button, kColButton);
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, kColButtonHovered);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Scaled(kCardRounding));

  ImGui::SetCursorPosX(buttonX);
  if (Button(label.c_str(), TextStyle::Bold().Font("h1").Color(Colors::WHITE).HoverColor(Colors::WHITE), ImVec2(Scaled(kButtonWidth), Scaled(kButtonHeight)))) {
    SetVisibility(false);
  }
  if (ImGui::IsItemHovered()) {
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
  }

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(2);
}

void WelcomeWindow::RenderFooter(float x, float width, bool centered) {
  auto& loc = Localization::LocalizationManager::GetInstance();
  const auto& manifest = Config::GetFrameworkManifestData();
  std::string iconHeart = ICON_FA_HEART;
  std::string iconPatreon = ICON_FA_PATREON;
  std::string patreonUrl = manifest.info.patreonUrl.value_or("");
  const std::string footer = loc.GetFormatted("framework", "welcome_window.footer", iconHeart, iconPatreon, patreonUrl);

  const TextStyle style = TextStyle::Regular().Size(kBodySize).Color(kColMuted);
  if (centered) {
    RenderCenteredMarkdown("##WelcomeFooter", footer, style, x + width * 0.5f, width, m_footerWidth);
  } else {
    ImGui::SetCursorPosX(x);
    RenderMarkdownBlock("##WelcomeFooter", footer, style, width);
  }
}

void WelcomeWindow::SetUpdateContent(const std::string& changelogMarkdown) {
  m_updateChangelog = StyleChangelog(changelogMarkdown);
  m_mode = WelcomeMode::FrameworkUpdate;
}

std::string WelcomeWindow::GetVersionLabel() const {
  return "v" + Config::GetFrameworkManifestData().info.version.value_or("");
}

void WelcomeWindow::ApplySettings(const nlohmann::ordered_json& settings) {
  // We don't need to load anything from config for this window
}

nlohmann::ordered_json WelcomeWindow::GetCurrentSettings() const {
  // Return empty object to avoid saving state to config
  return nlohmann::ordered_json::object();
}

}  // namespace SPF::UI
