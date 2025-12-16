#pragma once

#include "SPF/Hooks/IHook.hpp"
#include <string>
#include <cstdint>

SPF_NS_BEGIN
namespace Hooks {
/**
 * @class CameraHooks
 * @brief A manageable hook service for essential game camera functions.
 *
 * This class acts as a feature hook that, when installed, finds the addresses
 * of the core camera functions (`InitializeCamera` and `GetCameraObject`)
 * without installing a traditional detour. It simply makes these functions
 * available to the rest of the framework.
 */
class CameraHooks : public IHook {
 public:
  // Function pointer types for the game's camera functions.
  using InitializeCameraFunc = void (*)(uintptr_t, uint32_t);
  using GetCameraObjectFunc = void* (*)(void* manager, int index);
  using UpdateCameraProjectionFunc = void (*)(void* pCameraObject, float width, float height);

 public:
  static CameraHooks& GetInstance();

  CameraHooks(const CameraHooks&) = delete;
  void operator=(const CameraHooks&) = delete;

  // --- IHook Implementation ---
  const std::string& GetName() const override { return m_name; }
  const std::string& GetDisplayName() const override { return m_displayName; }
  const std::string& GetOwnerName() const override { return m_ownerName; }
  bool IsEnabled() const override { return m_isEnabled; }
  void SetEnabled(bool enabled) override { m_isEnabled = enabled; }
  bool IsInstalled() const override {
    return m_initializeCameraFunc != nullptr && m_getCameraObjectFunc != nullptr && m_updateCameraProjectionFunc != nullptr && m_debugCameraHandleInputFunc != 0;
  }
  const std::string& GetSignature() const override { return m_signature; }

  bool Install() override;
  void Uninstall() override;
  void Remove() override;

  // --- Public API for Framework ---
  InitializeCameraFunc GetInitializeCameraFunc() const { return m_initializeCameraFunc; }
  GetCameraObjectFunc GetGetCameraObjectFunc() const { return m_getCameraObjectFunc; }
  UpdateCameraProjectionFunc GetUpdateCameraProjectionFunc() const { return m_updateCameraProjectionFunc; }
  uintptr_t GetDebugCameraHandleInputFunc() const { return m_debugCameraHandleInputFunc; }

 private:
  CameraHooks();
  ~CameraHooks() = default;

  // --- Hook Configuration ---
  std::string m_ownerName = "framework";
  std::string m_name = "CameraHooks";
  std::string m_displayName = "Core Camera Functions";
  bool m_isEnabled = true;  // This should always be enabled if the camera system is used.
  std::string m_signature;

  // --- Runtime State ---
  InitializeCameraFunc m_initializeCameraFunc = nullptr;
  GetCameraObjectFunc m_getCameraObjectFunc = nullptr;
  UpdateCameraProjectionFunc m_updateCameraProjectionFunc = nullptr;
  uintptr_t m_debugCameraHandleInputFunc = 0;
};
}  // namespace Hooks
SPF_NS_END
