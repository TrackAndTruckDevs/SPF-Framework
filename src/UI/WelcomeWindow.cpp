#include "SPF/UI/WelcomeWindow.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/UI/UITypographyHelper.hpp"
#include "SPF/UI/Icons.hpp"
#include "SPF/UI/UIStyle.hpp"
#include "SPF/UI/Resources/spf_logo.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/Localization/LocalizationManager.hpp"
#include "SPF/Config/FrameworkManifest.hpp"

#include "SPF/UI/UIElements.hpp"

#include <imgui.h>
#include <imgui_internal.h>

SPF_NS_BEGIN

namespace UI {

// ============================================================================
// UI LAYOUT CONSTANTS - Adjust these to tune the look
// ============================================================================
namespace {
    constexpr float kContentWidth       = 700.0f;
    constexpr float kLogoWidth          = 400.0f;
    constexpr float kLogoTopOffset      = 150.0f;  // From screen top
    constexpr float kTitleTopOffset     = 20.0f;  // Gap between logo and title
    constexpr float kChildTopOffset     = 20.0f;  // Gap between title and text area
    constexpr float kButtonBottomOffset = 125.0f;
    constexpr float kButtonWidth        = 250.0f;
    constexpr float kButtonHeight       = 50.0f;
    
    constexpr float kOverlayAlpha       = 0.90f;
    constexpr float kChildBgAlpha       = 0.0f;
}
// ============================================================================

WelcomeWindow::WelcomeWindow(const std::string& componentName, const std::string& windowId)
    : m_componentName(componentName), m_windowId(windowId) {
    m_isVisible = false;
    UpdateLocalization();
}

void WelcomeWindow::UpdateLocalization() {
    auto& loc = Localization::LocalizationManager::GetInstance();
    const std::string comp = "framework";

    m_locTitle = loc.Get(comp, "welcome_window.title");
    m_locIntroTitle = loc.Get(comp, "welcome_window.intro_title");
    m_locIntroText = loc.Get(comp, "welcome_window.intro_text");
    
    m_locSection1Title = loc.Get(comp, "welcome_window.section1.title");
    m_locSection2Title = loc.Get(comp, "welcome_window.section2.title");
    m_locSection3Title = loc.Get(comp, "welcome_window.section3.title");
    m_locSection3Text = loc.Get(comp, "welcome_window.section3.text");
    m_locSection4Title = loc.Get(comp, "welcome_window.section4.title");
    m_locSection5Title = loc.Get(comp, "welcome_window.section5.title");
    m_locSection5Text = loc.Get(comp, "welcome_window.section5.text");
    
    m_locFooter = loc.Get(comp, "welcome_window.footer");
    m_locBtnGo = loc.Get(comp, "welcome_window.button_go");
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
        UIManager::GetInstance().PlayTransition(2, 0.4f, false, 0); // CROSS (Dip to black), 0.4s, BLACK
        m_wasVisibleLastFrame = true;
    }

    InitializeResources();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    
    // Background style
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, kOverlayAlpha));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("WelcomeOverlay", nullptr, flags)) {
        m_isFocused = ImGui::IsWindowFocused();
        RenderContent();
    }
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

void WelcomeWindow::RenderContent() {
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    float centerX = screenSize.x * 0.5f;

    // 1. Logo Calculation and Rendering
    float logoHeight = 0.0f;
    ImGui::SetCursorPosY(kLogoTopOffset);
    if (m_logoTexture) {
        float aspect = (float)m_logoTexture->GetHeight() / (float)m_logoTexture->GetWidth();
        logoHeight = kLogoWidth * aspect;
        
        ImGui::SetCursorPosX(centerX - (kLogoWidth * 0.5f));
        ImGui::Image(m_logoTexture->GetHandle(), ImVec2(kLogoWidth, logoHeight));
    }

    // 2. Title Calculation and Rendering
    ImVec2 titleSize = Typography::CalcTextSize(m_locTitle.c_str(), TextStyle::H1().Font("h1_large_bold"));
    
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + kTitleTopOffset);
    float titlePosY = ImGui::GetCursorPosY();
    Typography::Text(TextStyle::H1().Font("h1_large_bold").Color(Colors::LIGHT_GRAY).Align(TextAlign::Center), "%s", m_locTitle.c_str());

    // 3. Dynamic Child Height Calculation
    // Total used height excluding child = LogoTop + LogoHeight + TitleGap + TitleHeight + (ChildGap * 2) + ButtonHeight + ButtonBottom
    float staticHeight = kLogoTopOffset + logoHeight + kTitleTopOffset + titleSize.y + (kChildTopOffset * 2.0f) + kButtonHeight + kButtonBottomOffset;
    float childHeight = screenSize.y - staticHeight;
    
    // Safety clamp
    if (childHeight < 100.0f) childHeight = 100.0f;

    // 4. Child Window Rendering
    ImGui::SetCursorPosX(centerX - (kContentWidth * 0.5f));
    ImGui::SetCursorPosY(titlePosY + titleSize.y + kChildTopOffset);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, kChildBgAlpha));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));

    if (ImGui::BeginChild("WelcomeContent", ImVec2(kContentWidth, childHeight), false, ImGuiWindowFlags_NoScrollbar)) {
        RenderMarkdownContent();
    }
    ImGui::EndChild();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    // 5. "GO" Button (Static at bottom)
    ImGui::SetCursorPosX(centerX - (kButtonWidth * 0.5f));
    ImGui::SetCursorPosY(screenSize.y - kButtonHeight - kButtonBottomOffset);

    if (Button((std::string(ICON_FA_TRUCK_FAST) + " " + m_locBtnGo).c_str(), TextStyle::Bold().Font("h1"), ImVec2(kButtonWidth, kButtonHeight))) {
        SetVisibility(false);
    }
}

void WelcomeWindow::RenderMarkdownContent() {
    auto& loc = Localization::LocalizationManager::GetInstance();
    const std::string comp = "framework";
    const auto& manifest = Config::GetFrameworkManifestData();
    std::string pluginsPath = System::PathManager::GetPluginsPath().string();

    std::string iconKeyboard = ICON_FA_KEYBOARD;
    std::string iconGithub   = ICON_FA_GITHUB;
    std::string iconHeart    = ICON_FA_HEART;
    std::string iconPatreon  = ICON_FA_PATREON;
    std::string githubUrl    = manifest.info.githubUrl.value_or("");
    std::string patreonUrl   = manifest.info.patreonUrl.value_or("");

    // Header 1: Intro (Inside Child)
    ImGui::PushStyleVar(ImGuiStyleVar_SeparatorTextAlign, ImVec2(0.5f, 0.5f));
    Typography::Text(TextStyle::H2().Separator().Color(Colors::GOLD).Align(TextAlign::Center).Wrapped(), 
                     "%s", m_locIntroTitle.c_str());
    ImGui::PopStyleVar();
    
    auto bodyStyle = TextStyle::Bold().Wrapped().Color(Colors::SILVER).Padding(ImVec2(7, 0));
    ImGui::Spacing();
    Typography::RenderMarkdownText(m_locIntroText, bodyStyle);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section 1
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY).Separator(), "%s %s", ICON_FA_BARS_STAGGERED, m_locSection1Title.c_str());
    Typography::RenderMarkdownText(loc.GetFormatted(comp, "welcome_window.section1.text", iconKeyboard), bodyStyle);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section 2
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY).Separator(), "%s %s", ICON_FA_FOLDER_OPEN, m_locSection2Title.c_str());
    Typography::RenderMarkdownText(loc.GetFormatted(comp, "welcome_window.section2.text", pluginsPath), bodyStyle);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section 3
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY).Separator(), "%s %s", ICON_FA_GEAR, m_locSection3Title.c_str());
    Typography::RenderMarkdownText(m_locSection3Text, bodyStyle);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section 4
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY).Separator(), "%s %s", ICON_FA_CODE, m_locSection4Title.c_str());
    Typography::RenderMarkdownText(loc.GetFormatted(comp, "welcome_window.section4.text", iconGithub, githubUrl), bodyStyle);
    ImGui::Spacing();
    ImGui::Spacing();

    // Section 5
    Typography::Text(TextStyle::Bold().Color(Colors::GRAY).Separator(), "%s %s", ICON_FA_HANDSHAKE, m_locSection5Title.c_str());
    Typography::RenderMarkdownText(m_locSection5Text, bodyStyle);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    Typography::RenderMarkdownText(loc.GetFormatted(comp, "welcome_window.footer", iconHeart, iconPatreon, patreonUrl), bodyStyle);
}

void WelcomeWindow::ApplySettings(const nlohmann::ordered_json& settings) {
    // We don't need to load anything from config for this window
}

nlohmann::ordered_json WelcomeWindow::GetCurrentSettings() const {
    // Return empty object to avoid saving state to config
    return nlohmann::ordered_json::object();
}

}  // namespace UI

SPF_NS_END
