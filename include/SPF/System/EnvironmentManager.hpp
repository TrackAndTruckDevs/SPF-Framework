/**                                                                                               
* @file EnvironmentManager.hpp                                                                          
* @brief Centralized registry for framework, game, and system environment information.
*                                                                                                 
* @details Static data is resolved once during initialization. Dynamic status 
*          (VR, Overlay, Multiplayer) is cached with a 1-second throttle.
*/ 

#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <filesystem>
#include <chrono>
#include <Windows.h>

SPF_NS_BEGIN
enum class Game;
namespace System {

struct FrameworkInfo {
    std::string version;
    std::string buildType;
    std::string configuration;
    std::filesystem::path loaderPath;
};

struct GameInfo {
    std::string name;
    std::string code;
    std::string version;
    std::filesystem::path exePath;
    std::filesystem::path rootPath;
    std::filesystem::path musicPath;
    std::filesystem::path screenshotPath;
    uint32_t steamAppId;
    bool isSteamVersion;
    std::wstring commandLine;
};

struct SystemInfo {
    std::string osName;
    std::string locale;
    std::string architecture;
};

struct EnvironmentStatus {
    std::string profileName;
    bool isVR;
    bool isTobiiActive;
    std::string renderer;
    std::string multiplayer;
    bool isSteamOverlayActive;
};

class EnvironmentManager {
public:
    static EnvironmentManager& GetInstance();

    EnvironmentManager(const EnvironmentManager&) = delete;
    void operator=(const EnvironmentManager&) = delete;

    /** @brief Detects all static info. Called in Core::Preload. */
    void Initialize(HMODULE hModule);

    /** @brief Called from Telemetry SDK to set precise game name and ID. */
    void SetGameData(const std::string& fullName, SPF::Game gameId);

    // --- Accessors ---
    const FrameworkInfo& GetFrameworkInfo() const { return m_framework; }
    const GameInfo& GetGameInfo() const { return m_game; }
    const SystemInfo& GetSystemInfo() const { return m_system; }
    
    /** @brief Returns runtime status, refreshing cache if older than 1s. */
    const EnvironmentStatus& GetStatus();

private:
    EnvironmentManager() = default;
    ~EnvironmentManager() = default;

    void DetectStaticFramework(HMODULE hModule);
    void DetectStaticGame();
    void DetectStaticSystem();
    void RefreshDynamicStatus();

    FrameworkInfo m_framework;
    GameInfo m_game;
    SystemInfo m_system;
    EnvironmentStatus m_status;

    // Caching for dynamic status
    std::chrono::steady_clock::time_point m_lastStatusUpdate;
    const std::chrono::milliseconds CACHE_TTL{5000};
};

} // namespace System
SPF_NS_END
