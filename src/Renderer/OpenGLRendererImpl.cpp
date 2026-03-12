#include <SPF/Renderer/OpenGLRendererImpl.hpp>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <gl/GL.h>
#include <stb_image.h>

#include <SPF/Hooks/OpenGLHook.hpp>
#include <SPF/Logging/LoggerFactory.hpp>
#include <SPF/Renderer/Renderer.hpp>
#include <SPF/UI/UIManager.hpp>


SPF_NS_BEGIN
namespace Rendering {

using namespace SPF::Logging;
using namespace SPF::Hooks;

namespace {

class OpenGLTexture : public ITexture {
public:
    OpenGLTexture(GLuint id, uint32_t width, uint32_t height)
        : m_id(id), m_width(width), m_height(height) {}

    ~OpenGLTexture() override {
        glDeleteTextures(1, &m_id);
    }

    void* GetHandle() const override { return reinterpret_cast<void*>(static_cast<uintptr_t>(m_id)); }
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }

private:
    GLuint m_id;
    uint32_t m_width;
    uint32_t m_height;
};

} // namespace

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

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

std::unique_ptr<ITexture> OpenGLRendererImpl::CreateTextureFromMemory(const unsigned char* data, size_t size) {
    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 4);
    if (!pixels) {
        m_logger->Error("OpenGL CreateTexture: stbi_load failed: {}", stbi_failure_reason());
        return nullptr;
    }

    // Ensure we have a valid context active for texture creation
    if (m_hdc && m_originalContext) {
        wglMakeCurrent(m_hdc, m_originalContext);
    }

    // Save current states
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    
    // Save unpack states to restore them later (prevent breaking the game)
    GLint last_unpack_alignment, last_unpack_row_length, last_unpack_skip_pixels, last_unpack_skip_rows;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &last_unpack_alignment);
    glGetIntegerv(0x0CF2, &last_unpack_row_length);   // GL_UNPACK_ROW_LENGTH
    glGetIntegerv(0x0CF4, &last_unpack_skip_pixels); // GL_UNPACK_SKIP_PIXELS
    glGetIntegerv(0x0CF3, &last_unpack_skip_rows);   // GL_UNPACK_SKIP_ROWS

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters for ImGui compatibility
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F); // GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F); // GL_CLAMP_TO_EDGE

    // CRITICAL: Reset all unpack states to defaults before glTexImage2D
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(0x0CF2, 0); // GL_UNPACK_ROW_LENGTH = 0
    glPixelStorei(0x0CF4, 0); // GL_UNPACK_SKIP_PIXELS = 0
    glPixelStorei(0x0CF3, 0); // GL_UNPACK_SKIP_ROWS = 0

    // Upload pixels. Using GL_RGBA (0x1908) for both internal and external format
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Restore all states
    glPixelStorei(GL_UNPACK_ALIGNMENT, last_unpack_alignment);
    glPixelStorei(0x0CF2, last_unpack_row_length);
    glPixelStorei(0x0CF4, last_unpack_skip_pixels);
    glPixelStorei(0x0CF3, last_unpack_skip_rows);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    
    stbi_image_free(pixels);

    return std::make_unique<OpenGLTexture>(textureID, width, height);
}

void OpenGLRendererImpl::RefreshFontAtlas() {
    if (m_isImGuiInitialized) {
        ImGui_ImplOpenGL3_DestroyDeviceObjects();
        ImGui_ImplOpenGL3_CreateDeviceObjects();
    }
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
    if (ImGui_ImplWin32_Init(OpenGLHook::MainWindow) && ImGui_ImplOpenGL3_Init("#version 130")) {
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

    // Ensure the correct context is active
    wglMakeCurrent(m_hdc, m_originalContext);

    // FIX: Disable sRGB conversion during ImGui rendering to prevent "over-bright" colors
    GLboolean last_enable_srgb = glIsEnabled(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_FRAMEBUFFER_SRGB);

    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Ask the UIManager to render all windows
    m_renderer.OnRendererRenderImGui();

    // Render the ImGui draw data
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Restore sRGB state
    if (last_enable_srgb) {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
}

}  // namespace Rendering
SPF_NS_END
