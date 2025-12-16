#pragma once

#include "SPF/Hooks/IHook.hpp"
#include "SPF/GameCamera/IGameCamera.hpp"
#include "SPF/GameCamera/GameCameraDebug.hpp"
#include "SPF/GameCamera/GameCameraDebugState.hpp"
#include "SPF/GameCamera/GameCameraDebugAnimation.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include <string>
#include <cstdint>
#include <memory>
#include <map>

SPF_NS_BEGIN
namespace GameCamera {
/**
 * @class GameCameraManager
 * @brief A high-level service for managing and controlling all game cameras.
 *
 * This class holds instances of all specific camera implementations (as IGameCamera),
 * handles switching between them, and calls the active camera's Update() method
 * every frame.
 */
class GameCameraManager : public Hooks::IHook {
 public:
  static GameCameraManager& GetInstance();

  GameCameraManager(const GameCameraManager&) = delete;
  void operator=(const GameCameraManager&) = delete;

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool IsInstalled() const override { return m_isReady; }
  const std::string& GetSignature() const override {
    static const std::string empty;
    return empty;
  }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  // --- Public Camera API for Framework (C++) ---
  void SwitchTo(GameCameraType cameraType);
  GameCameraType GetCurrentCameraType();
  void Update(float dt);

  IGameCamera* GetCamera(GameCameraType cameraType);
  GameCameraDebug* GetDebugCamera();
  GameCameraDebugState* GetDebugStateCamera();
  GameCameraDebugAnimation* GetDebugAnimationController();

 private:
  GameCameraManager();
  ~GameCameraManager() = default;

  void RegisterCameras();

  // --- Service Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "GameCameraManagerService";
  std::string m_displayName = "Game Camera Manager Service";
  bool m_isEnabled = true;

  // --- Runtime State ---
  bool m_isReady = false;
  Hooks::CameraHooks::InitializeCameraFunc m_initializeCameraFunc = nullptr;

  std::map<GameCameraType, std::unique_ptr<IGameCamera>> m_cameras;
  IGameCamera* m_activeCamera = nullptr;
  std::unique_ptr<GameCameraDebug> m_debugCamera;
  std::unique_ptr<GameCameraDebugState> m_debugStateCamera;
  std::unique_ptr<GameCameraDebugAnimation> m_debugAnimationController;
};
}  // namespace GameCamera
SPF_NS_END