#include "SPF/GameCamera/GameCameraManager.hpp"
#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Data/GameData/GameDataCameraService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/GameCamera/GameCameraInterior.hpp"
#include "SPF/GameCamera/GameCameraBehind.hpp"
#include "SPF/GameCamera/GameCameraTop.hpp"
#include "SPF/GameCamera/GameCameraCabin.hpp"
#include "SPF/GameCamera/GameCameraWindow.hpp"
#include "SPF/GameCamera/GameCameraBumper.hpp"
#include "SPF/GameCamera/GameCameraWheel.hpp"
#include "SPF/GameCamera/GameCameraTV.hpp"
#include "SPF/GameCamera/GameCameraFree.hpp"

#include <Windows.h>
#include <memory>
#include <map>

using namespace SPF::Data::GameData;

SPF_NS_BEGIN
namespace GameCamera {
GameCameraManager::GameCameraManager() {
  m_debugCamera = std::make_unique<GameCameraDebug>();
  m_debugStateCamera = std::make_unique<GameCameraDebugState>();
  m_debugAnimationController = std::make_unique<GameCameraDebugAnimation>();
}

GameCameraManager& GameCameraManager::GetInstance() {
  static GameCameraManager instance;
  return instance;
}

bool GameCameraManager::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
  logger->Info("Installing Game Camera Service...");

  // This service depends on two other services being ready.
  auto& cameraHooks = Hooks::CameraHooks::GetInstance();
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();

  if (!cameraHooks.IsInstalled()) {  // Only check CameraHooks here
    logger->Warn("Deferring install: CameraHooks are not ready.");
    return false;
  }

  // Now that CameraHooks are installed, try to find all game data offsets.
  // This will be called repeatedly until critical offsets are found.
  if (!gameData.TryFindAllOffsets()) {
    logger->Warn("Deferring install: Critical GameData offsets not found yet. Will retry.");
    return false;
  }
  // Cache the function pointer for performance.
  m_initializeCameraFunc = cameraHooks.GetInitializeCameraFunc();

  RegisterCameras();

  // Set the initial active camera
  auto initialCameraType = GetCurrentCameraType();
  auto it = m_cameras.find(initialCameraType);
  if (it != m_cameras.end()) {
    m_activeCamera = it->second.get();
    if (m_activeCamera) {
      m_activeCamera->OnActivate();
    }
  }

  m_isReady = true;
  logger->Info("Game Camera Service installed.");
  return true;
}

void GameCameraManager::Uninstall() {
  if (m_isReady) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Uninstalling Game Camera Service...");
    m_isReady = false;
    m_initializeCameraFunc = nullptr;
    m_activeCamera = nullptr;
    m_cameras.clear();
    m_debugCamera.reset();
    m_debugStateCamera.reset();
    m_debugAnimationController.reset();
  }
}

void GameCameraManager::Remove() { Uninstall(); }

void GameCameraManager::SwitchTo(GameCameraType cameraType) {
  if (!GameDataCameraService::GetInstance().IsReady()) return;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
  if (!m_isReady) {
    return;
  }

  // Deactivate the current camera if it exists
  if (m_activeCamera) {
    m_activeCamera->OnDeactivate();
    m_activeCamera = nullptr;
  }

  // Find and activate the new camera if it's one of our managed C++ objects
  auto it = m_cameras.find(cameraType);
  if (it != m_cameras.end()) {
    m_activeCamera = it->second.get();
    if (m_activeCamera) {
      m_activeCamera->OnActivate();
    }
  } else {
    // If the camera is not in our map, it's a simple camera managed by the game itself.
    // We don't have a C++ object for it, so m_activeCamera will be nullptr.
    logger->Info("Switching to a game-managed camera: {}", static_cast<int>(cameraType));
  }

  // --- Low-level game call ---
  // This part is the same as before, telling the game engine to switch.
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManagerAddr = gameData.GetStandardManagerPtrAddr();

  if (!pStandardManagerAddr) {
    logger->Error("SwitchTo failed: StandardManagerPtrAddr is null.");
    return;
  }

  uintptr_t standardManagerPtr = *(uintptr_t*)pStandardManagerAddr;
  if (!standardManagerPtr) {
    logger->Error("SwitchTo failed: standardManagerPtr is null.");
    return;
  }

  uint32_t cameraID = static_cast<uint32_t>(cameraType);

  if (cameraType != GameCameraType::DeveloperFreeCamera) {
    m_initializeCameraFunc(standardManagerPtr, cameraID);
  } else  // Special case for Free Camera (ID 0)
  {
    // --- Free Camera Initialization Context ---
    // The free camera requires a special "initialization context" to be passed to the
    // native InitializeCamera function, unlike other cameras that just use the StandardManager pointer.
    // The following logic resolves this special context pointer.

    // 1. Get the pointer to the global object related to the free camera system.
    uintptr_t* pFreecamGlobalObjectPtr = gameData.GetFreecamGlobalObjectPtr();
    if (!pFreecamGlobalObjectPtr || !*pFreecamGlobalObjectPtr) {
      logger->Error("SwitchTo(0) failed: Freecam global object pointer is not available.");
      return;
    }

    // 2. Calculate the final initialization context pointer.
    // This involves dereferencing the global object pointer and adding a specific offset.
    uintptr_t freeCamInitContext = 0;
    uintptr_t base_obj = *pFreecamGlobalObjectPtr;
    uintptr_t context_offset = gameData.GetFreecamContextOffset();

    freeCamInitContext = *(uintptr_t*)(base_obj + context_offset);

    if (!freeCamInitContext) {
      logger->Error("SwitchTo(0) failed: Could not resolve freeCamInitContext.");
      return;
    }

    // 3. Call the native function with the special context.
    // IMPORTANT: `freeCamInitContext` is NOT the camera object pointer used for reading data (like position).
    // It is a temporary context used only for this initialization call.
    m_initializeCameraFunc(freeCamInitContext, 0);
  }
}

GameCameraType GameCameraManager::GetCurrentCameraType() {
  if (!m_isReady || !Data::GameData::GameDataCameraService::GetInstance().IsReady()) {
    return static_cast<GameCameraType>(-1);
  }

  // Get data fresh from the source services to ensure it's valid.
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t pStandardManagerAddr = gameData.GetStandardManagerPtrAddr();
  intptr_t camera_id_offset = gameData.GetActiveCameraIdOffset();

  if (!pStandardManagerAddr || camera_id_offset == 0) {
    return static_cast<GameCameraType>(-1);
  }

  uintptr_t standardManagerPtr = *(uintptr_t*)pStandardManagerAddr;
  if (!standardManagerPtr) {
    return static_cast<GameCameraType>(-1);
  }

  uintptr_t addressOfCameraId = standardManagerPtr + camera_id_offset;

  // Reading from game memory, be safe.
  if (IsBadReadPtr((void*)addressOfCameraId, sizeof(uint32_t))) {
    return static_cast<GameCameraType>(-1);
  }

  return static_cast<GameCameraType>(*(uint32_t*)addressOfCameraId);
}

void GameCameraManager::Update(float dt) {
  if (!GameDataCameraService::GetInstance().IsReady()) return;

  auto currentTypeInGame = GetCurrentCameraType();

  // If we think a camera is active, but the game has a different one, we need to re-sync.
  if (m_activeCamera && m_activeCamera->GetType() != currentTypeInGame) {
    m_activeCamera->OnDeactivate();
    m_activeCamera = nullptr;
  }

  // If no camera is active (either from the start or after re-sync), find the correct one.
  if (!m_activeCamera) {
    auto it = m_cameras.find(currentTypeInGame);
    if (it != m_cameras.end()) {
      m_activeCamera = it->second.get();
      if (m_activeCamera) {
        m_activeCamera->OnActivate();
      }
    }
  }

  // --- Automatic Default State Saving ---
  // If we have an active camera and its default state has not been saved yet, save it now.
  // This ensures that the initial state is captured on the very first frame the camera is active.
  if (m_activeCamera && !m_activeCamera->HasSavedDefaults()) {
    m_activeCamera->StoreDefaultState();
  }

  // Finally, if we have a valid, synced active camera, update it.
  if (m_activeCamera) {
    m_activeCamera->Update(dt);
  }

  if (m_debugAnimationController) {
    m_debugAnimationController->Update(dt);
  }
}

void GameCameraManager::RegisterCameras() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
  logger->Info("Registering camera implementations...");

  auto interiorCam = std::make_unique<GameCameraInterior>();
  m_cameras[interiorCam->GetType()] = std::move(interiorCam);
  logger->Info("  -> Registered {}", typeid(GameCameraInterior).name());

  auto behindCam = std::make_unique<GameCameraBehind>();
  m_cameras[behindCam->GetType()] = std::move(behindCam);
  logger->Info("  -> Registered {}", typeid(GameCameraBehind).name());

  auto topCam = std::make_unique<GameCameraTop>();
  m_cameras[topCam->GetType()] = std::move(topCam);
  logger->Info("  -> Registered {}", typeid(GameCameraTop).name());

  auto cabinCam = std::make_unique<GameCameraCabin>();
  m_cameras[cabinCam->GetType()] = std::move(cabinCam);
  logger->Info("  -> Registered {}", typeid(GameCameraCabin).name());

  auto windowCam = std::make_unique<GameCameraWindow>();
  m_cameras[windowCam->GetType()] = std::move(windowCam);
  logger->Info("  -> Registered {}", typeid(GameCameraWindow).name());

  auto bumperCam = std::make_unique<GameCameraBumper>();
  m_cameras[bumperCam->GetType()] = std::move(bumperCam);
  logger->Info("  -> Registered {}", typeid(GameCameraBumper).name());

  auto wheelCam = std::make_unique<GameCameraWheel>();
  m_cameras[wheelCam->GetType()] = std::move(wheelCam);
  logger->Info("  -> Registered {}", typeid(GameCameraWheel).name());

  auto tvCam = std::make_unique<GameCameraTV>();
  m_cameras[tvCam->GetType()] = std::move(tvCam);
  logger->Info("  -> Registered {}", typeid(GameCameraTV).name());

  auto freeCam = std::make_unique<GameCameraFree>();
  m_cameras[freeCam->GetType()] = std::move(freeCam);
  logger->Info("  -> Registered {}", typeid(GameCameraFree).name());

  // Future cameras will be registered here...
}

IGameCamera* GameCameraManager::GetCamera(GameCameraType cameraType) {
  auto it = m_cameras.find(cameraType);
  if (it != m_cameras.end()) {
    return it->second.get();
  }
  return nullptr;
}

GameCameraDebug* GameCameraManager::GetDebugCamera() { return m_debugCamera.get(); }

GameCameraDebugState* GameCameraManager::GetDebugStateCamera() { return m_debugStateCamera.get(); }

GameCameraDebugAnimation* GameCameraManager::GetDebugAnimationController() { return m_debugAnimationController.get(); }
}  // namespace GameCamera
SPF_NS_END
