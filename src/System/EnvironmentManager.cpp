#include "SPF/System/EnvironmentManager.hpp"
#include "SPF/System/PathManager.hpp"
#include "SPF/Data/GameData/GameObjectSessionService.hpp"
#include "SPF/Config/FrameworkManifest.hpp"
#include "SPF/UI/UIManager.hpp"
#include "SPF/Renderer/Renderer.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Types.hpp"
#include <Windows.h>
#include <algorithm>

// Define RtlGetVersion prototype
typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

SPF_NS_BEGIN
namespace System {

EnvironmentManager& EnvironmentManager::GetInstance() {
    static EnvironmentManager instance;
    return instance;
}

void EnvironmentManager::Initialize(HMODULE hModule) {
    DetectStaticFramework(hModule);
    DetectStaticGame();
    DetectStaticSystem();
    
    // Initial status detection
    RefreshDynamicStatus();
}

void EnvironmentManager::SetGameData(const std::string& fullName, SPF::Game gameId) {
    if (gameId == SPF::Game::ETS2) {
        m_game.code = "eut2";
        m_game.steamAppId = 227300;
    } else if (gameId == SPF::Game::ATS) {
        m_game.code = "ats";
        m_game.steamAppId = 270880;
    } else {
        m_game.code = "unknown";
        m_game.steamAppId = 0;
    }
    
    // Logic from your example: split "Game Name 1.50.1.2s"
    size_t lastSpace = fullName.find_last_of(' ');
    if (lastSpace != std::string::npos) {
        m_game.name = fullName.substr(0, lastSpace);
        m_game.version = fullName.substr(lastSpace + 1);
    } else {
        m_game.name = fullName;
        m_game.version = "Unknown";
    }
}

const EnvironmentStatus& EnvironmentManager::GetStatus() {
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastStatusUpdate) > CACHE_TTL) {
        RefreshDynamicStatus();
        m_lastStatusUpdate = now;
    }
    return m_status;
}

void EnvironmentManager::DetectStaticFramework(HMODULE hModule) {
    m_framework.version = Config::GetFrameworkManifestData().info.version.value_or("Unknown");
    
    std::string lowerVersion = m_framework.version;
    std::transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), ::tolower);
    m_framework.buildType = (lowerVersion.find("beta") != std::string::npos) ? "Beta" : "Stable";

#ifdef _DEBUG
    m_framework.configuration = "Debug";
#else
    m_framework.configuration = "Release";
#endif

    wchar_t path[MAX_PATH];
    if (GetModuleFileNameW(hModule, path, MAX_PATH)) {
        m_framework.loaderPath = std::filesystem::path(path);
    }
}

void EnvironmentManager::DetectStaticGame() {
    m_game.commandLine = GetCommandLineW();
    
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
        m_game.exePath = std::filesystem::path(exePath).make_preferred();
    }

    // Try to get the root data path from UFS Manager 3 (base archives)
    // We already know ResolveVirtualPath is stable.
    std::string baseScsPath = PathManager::ResolveVirtualPath(""); 
    if (!baseScsPath.empty()) {
        m_game.rootPath = std::filesystem::path(baseScsPath).parent_path().make_preferred();
    } else {
        // Fallback to EXE location logic: bin/win_x64/amtrucks.exe -> parent is win_x64, parent is bin, parent is root
        m_game.rootPath = m_game.exePath.parent_path().parent_path().parent_path();
    }

    m_game.isSteamVersion = (GetModuleHandleA("steam_api64.dll") != NULL);
}

void EnvironmentManager::DetectStaticSystem() {
    m_system.architecture = "x64";

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        auto pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (pRtlGetVersion) {
            RTL_OSVERSIONINFOW osvi = { 0 };
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (pRtlGetVersion(&osvi) == 0) {
                m_system.osName = "Windows " + std::to_string(osvi.dwMajorVersion);
                if (osvi.dwMajorVersion == 10 && osvi.dwBuildNumber >= 22000) m_system.osName = "Windows 11";
                m_system.osName += " (Build " + std::to_string(osvi.dwBuildNumber) + ")";
            }
        }
    }

    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    if (GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
        char mbs[LOCALE_NAME_MAX_LENGTH];
        size_t converted;
        wcstombs_s(&converted, mbs, localeName, _TRUNCATE);
        m_system.locale = mbs;
    }
}

void EnvironmentManager::RefreshDynamicStatus() {
    auto& sessionService = Data::GameData::GameObjectSessionService::GetInstance();
    
    // 1. Profile Information
    // We get the human-readable name (e.g. 'SPF_Test') for the Game tab
    std::string activeName = PathManager::GetCurrentProfileName();
    m_status.profileName = activeName.empty() ? "None" : activeName;

    // 2. Additional Paths (resolved via UFS with fallback to /home subdirectory)
    std::string musicStr = PathManager::ResolveVirtualPath("/home/music");
    std::filesystem::path homeDir = PathManager::GetSCSUserDir();

    if (musicStr.empty()) {
        m_game.musicPath = homeDir.empty() ? "" : (homeDir / "music").make_preferred();
    } else {
        m_game.musicPath = std::filesystem::path(musicStr).make_preferred();
    }

    std::string screenshotStr = PathManager::ResolveVirtualPath("/home/screenshot");
    if (screenshotStr.empty()) {
        m_game.screenshotPath = homeDir.empty() ? "" : (homeDir / "screenshot").make_preferred();
    } else {
        m_game.screenshotPath = std::filesystem::path(screenshotStr).make_preferred();
    }

    // 3. VR & Hardware
    std::wstring cmd = m_game.commandLine;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    m_status.isVR = (cmd.find(L"-oculus") != std::wstring::npos || 
                     cmd.find(L"-openvr") != std::wstring::npos ||
                     GetModuleHandleA("openvr_api.dll") != NULL);

    m_status.isTobiiActive = (GetModuleHandleA("tobii_gameintegration_x64.dll") != NULL);

    // 4. Renderer
    auto renderer = UI::UIManager::GetInstance().GetRenderer();
    if (renderer) {
        switch (renderer->GetDetectedAPI()) {
            case Rendering::RenderAPI::D3D11:  m_status.renderer = "DirectX 11"; break;
            case Rendering::RenderAPI::D3D12:  m_status.renderer = "DirectX 12"; break;
            case Rendering::RenderAPI::OpenGL: m_status.renderer = "OpenGL";     break;
            default:                           m_status.renderer = "Unknown";    break;
        }
    }

    // 5. Multiplayer (TruckersMP or Convoy)
    if (GetModuleHandleA("core_atsmp.dll") || GetModuleHandleA("core_ets2mp.dll")) {
        m_status.multiplayer = "TruckersMP";
    } else if (sessionService.AreAllFindersReady()) {
        uintptr_t sessionMgrPtrAddr = sessionService.GetSessionMgrPtrAddr();
        uintptr_t sessionMgr = *reinterpret_cast<uintptr_t*>(sessionMgrPtrAddr);
        if (sessionMgr != 0) {
            uint8_t status = *reinterpret_cast<uint8_t*>(sessionMgr + sessionService.GetConvoyStatusOffset());
            // Based on Ghidra: status 6, 7, 8, 9 mean Convoy session is active
            if (status >= 6 && status <= 9) {
                m_status.multiplayer = "Convoy";
            } else {
                m_status.multiplayer = "None";
            }
        } else {
            m_status.multiplayer = "None";
        }
    } else {
        m_status.multiplayer = "None";
    }

    // 6. Steam Overlay
    m_status.isSteamOverlayActive = (GetModuleHandleA("GameOverlayRenderer64.dll") != NULL);
}

} // namespace System
SPF_NS_END
