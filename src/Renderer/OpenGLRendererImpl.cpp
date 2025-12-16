#include <SPF/Renderer/OpenGLRendererImpl.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <gl/GL.h>

#include <SPF/Hooks/OpenGLHook.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/Renderer/Renderer.hpp>
#include <SPF/UI/UIManager.hpp>


SPF_NS_BEGIN
namespace Rendering {

using namespace SPF::Logging;
using namespace SPF::Hooks;

OpenGLRendererImpl::OpenGLRendererImpl(Renderer& renderer, UI::UIManager& uiManager)
    : RendererBase(renderer), 
      m_uiManager(uiManager),
      m_onInitSink(OpenGLHook::OnInit),
      m_onPresentSink(OpenGLHook::OnPresent)
{
    m_logger = LoggerFactory::GetInstance().GetLogger("OpenGLImpl");
    m_logger->Info("OpenGL Renderer Implementation created.");
}

OpenGLRendererImpl::~OpenGLRendererImpl() {
    Shutdown();
}

void OpenGLRendererImpl::Init() {
    m_logger->Info("Connecting to OpenGLHook signals...");
    m_onInitSink.Connect<&OpenGLRendererImpl::OnInit>(this);
    m_onPresentSink.Connect<&OpenGLRendererImpl::OnPresent>(this);
}

void OpenGLRendererImpl::Shutdown() {
    if (!m_isImGuiInitialized) {
        return;
    }
    m_logger->Info("Shutting down ImGui OpenGL implementation...");
    
    // Ensure we are using the correct context when shutting down
    wglMakeCurrent(m_hdc, m_originalContext);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    m_isImGuiInitialized = false;
    m_logger->Info("ImGui OpenGL implementation shut down.");
}

void OpenGLRendererImpl::OnInit(HDC hdc) {
    if (m_isImGuiInitialized) {
        return;
    }
    m_logger->Info("OnInit signal received. Initializing ImGui for OpenGL...");
    m_hdc = hdc;
    
    // Store the original OpenGL context
    m_originalContext = wglGetCurrentContext();
    
    // Initialize ImGui for Win32 and OpenGL
    if (ImGui_ImplWin32_Init(OpenGLHook::MainWindow) && ImGui_ImplOpenGL3_Init()) {
        m_renderer.OnRendererInit();
        m_isImGuiInitialized = true;
        m_logger->Info("ImGui OpenGL implementation initialized successfully.");
    } else {
        m_logger->Critical("Failed to initialize ImGui OpenGL backends.");
    }
}

void OpenGLRendererImpl::OnPresent(HDC hdc) {
    if (!m_isImGuiInitialized) {
        return;
    }

    // It's crucial to ensure the correct OpenGL context is active before rendering.
    wglMakeCurrent(m_hdc, m_originalContext);

    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Ask the UIManager to render all windows
    m_renderer.OnRendererRenderImGui();

    // Render the ImGui draw data
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace Rendering
SPF_NS_END
