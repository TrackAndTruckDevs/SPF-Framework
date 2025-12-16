#include <SPF/Hooks/OpenGLHook.hpp>

#include <Windows.h>
#include <gl/GL.h>
#include <MinHook.h>
#include <SPF/Logging/LoggerFactory.hpp>

#pragma comment(lib, "opengl32.lib")

SPF_NS_BEGIN
namespace Hooks {
using namespace SPF::Logging;

namespace {
// --- Logger for this module ---
auto GetLogger() {
    static auto logger = LoggerFactory::GetInstance().GetLogger("OpenGLHook");
    return logger;
}

// --- Function Prototypes & Original Pointers ---
using FnSwapBuffers = BOOL(WINAPI*)(HDC);
inline FnSwapBuffers o_wglSwapBuffers = nullptr;

BOOL WINAPI new_wglSwapBuffers(HDC hDC);
LRESULT CALLBACK WndProcOpenGL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// --- State ---
WNDPROC g_originalWndProcOpenGL = nullptr;
bool g_isInitedOpenGL = false;
bool g_isCreatedOpenGL = false; // Is the hook created via MH_CreateHook?
LPVOID g_pSwapBuffersTarget = nullptr;

// Helper to restore WndProc safely
void RestoreWndProcOpenGL() {
    if (g_originalWndProcOpenGL != nullptr && OpenGLHook::MainWindow != nullptr) {
        SetWindowLongPtr(OpenGLHook::MainWindow, GWLP_WNDPROC, (LONG_PTR)g_originalWndProcOpenGL);
        GetLogger()->Info("Restored original OpenGL WndProc.");
        g_originalWndProcOpenGL = nullptr;
    }
}
} // namespace

bool OpenGLHook::Install() {
    auto logger = GetLogger();
    if (g_isCreatedOpenGL) {
        logger->Info("OpenGL hook already created, ensuring it is enabled...");
        if (g_pSwapBuffersTarget && MH_EnableHook(g_pSwapBuffersTarget) != MH_OK) {
            logger->Error("Failed to re-enable hook for wglSwapBuffers.");
            return false;
        }
        logger->Info("OpenGL hook successfully re-enabled.");
        return true;
    }

    logger->Info("Installing OpenGL hooks for the first time...");
    
    g_pSwapBuffersTarget = (LPVOID)GetProcAddress(GetModuleHandle(TEXT("opengl32.dll")), "wglSwapBuffers");
    if (!g_pSwapBuffersTarget) {
        logger->Error("Failed to get address of wglSwapBuffers.");
        return false;
    }

    if (MH_CreateHook(g_pSwapBuffersTarget, reinterpret_cast<LPVOID>(&new_wglSwapBuffers), reinterpret_cast<LPVOID*>(&o_wglSwapBuffers)) != MH_OK) {
        logger->Critical("Failed to create hook for wglSwapBuffers.");
        g_pSwapBuffersTarget = nullptr;
        return false;
    }

    if (MH_EnableHook(g_pSwapBuffersTarget) != MH_OK) {
        logger->Critical("Failed to enable hook for wglSwapBuffers.");
        MH_RemoveHook(g_pSwapBuffersTarget); // cleanup
        g_pSwapBuffersTarget = nullptr;
        return false;
    }

    logger->Info("Successfully hooked wglSwapBuffers.");
    g_isCreatedOpenGL = true;
    return true;
}

void OpenGLHook::Uninstall() {
    auto logger = GetLogger();
    if (!g_isCreatedOpenGL) {
        return;
    }
    logger->Info("Disabling OpenGL hooks for reload...");
    RestoreWndProcOpenGL();
    
    if (g_pSwapBuffersTarget) {
        MH_DisableHook(g_pSwapBuffersTarget);
    }

    g_isInitedOpenGL = false;
    logger->Info("OpenGL hooks disabled.");
}

void OpenGLHook::Remove() {
    auto logger = GetLogger();
    if (!g_isCreatedOpenGL) {
        return;
    }
    logger->Info("Removing OpenGL hooks for shutdown...");
    RestoreWndProcOpenGL();

    if (g_pSwapBuffersTarget) {
        MH_DisableHook(g_pSwapBuffersTarget);
        MH_RemoveHook(g_pSwapBuffersTarget);
    }

    g_pSwapBuffersTarget = nullptr;
    o_wglSwapBuffers = nullptr;
    g_isInitedOpenGL = false;
    g_isCreatedOpenGL = false;
    logger->Info("OpenGL hooks completely removed.");
}

bool OpenGLHook::IsInstalled() {
    return g_isCreatedOpenGL;
}

namespace {
// --- Hook Implementations ---

BOOL WINAPI new_wglSwapBuffers(HDC hDC) {
    if (!g_isInitedOpenGL) {
        auto logger = GetLogger();
        logger->Info("First call to wglSwapBuffers. Initializing...");

        g_isInitedOpenGL = true;
        OpenGLHook::MainWindow = WindowFromDC(hDC);
        logger->Info("Game HWND captured via HDC: {0:p}", static_cast<void*>(OpenGLHook::MainWindow));

        g_originalWndProcOpenGL = reinterpret_cast<WNDPROC>(SetWindowLongPtr(OpenGLHook::MainWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProcOpenGL)));
        logger->Info("Original OpenGL WndProc at {0:p}, hooked with ours.", reinterpret_cast<void*>(g_originalWndProcOpenGL));

        OpenGLHook::OnInit.Call(hDC);
    }

    if (g_isInitedOpenGL) {
        OpenGLHook::OnPresent.Call(hDC);
    }

    return o_wglSwapBuffers(hDC);
}

LRESULT CALLBACK WndProcOpenGL(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    OpenGLHook::block_wndproc_message = false;
    OpenGLHook::OnWndProc.Call(hWnd, uMsg, wParam, lParam);

    if (OpenGLHook::block_wndproc_message) {
        return 0;
    }

    return CallWindowProc(g_originalWndProcOpenGL, hWnd, uMsg, wParam, lParam);
}

} // namespace
}  // namespace Hooks
SPF_NS_END
