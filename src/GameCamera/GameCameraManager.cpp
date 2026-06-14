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
#include "SPF/GameCamera/GameCameraPhoto.hpp"

#include <Windows.h>
#include <memory>
#include <map>

using namespace SPF::Data::GameData;

SPF_NS_BEGIN
namespace GameCamera {
GameCameraManager::GameCameraManager() {
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
  if (!m_isReady) return;

  // Deactivate the current camera if it exists
  if (m_activeCamera) {
    m_activeCamera->OnDeactivate();
    m_activeCamera = nullptr;
  }

  // Find and activate the new C++ camera object
  auto it = m_cameras.find(cameraType);
  if (it != m_cameras.end()) {
    m_activeCamera = it->second.get();
    if (m_activeCamera) {
      m_activeCamera->OnActivate();
    }
  } else {
    // If the camera is not in our map, it's a simple camera managed by the game itself.
    // We don't have a C++ object for it, so m_activeCamera will be nullptr.
    logger->Info("[CameraSystem] Switching to a game-managed camera: {}", static_cast<int>(cameraType));
  }

  // --- Native Engine Call ---
  // We tell the game to switch the active camera.
  // Both gameplay cameras and the Developer Free Camera (ID 0) 
  // are initialized using the main Camera Manager as the context.
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  uintptr_t cameraManagerAddr = gameData.GetCameraManager();

  if (!cameraManagerAddr) {
    logger->Error("[CameraSystem] SwitchTo failed: Camera Manager is null.");
    return;
  }

  uint32_t cameraID = static_cast<uint32_t>(cameraType);

  if (cameraType != GameCameraType::DeveloperFreeCamera) {
    // Standard gameplay cameras are part of the game's main camera array and
    // are initialized using the main Camera Manager pointer as the context.
    m_initializeCameraFunc(cameraManagerAddr, cameraID);
  } else {
    // --- Special case for Developer Free Camera (ID 0) ---
    // Why this is necessary:
    // Unlike standard cameras, the ID 0 camera is a specialized developer tool 
    // in the Prism engine. It does not belong to the standard manager's array 
    // in the same way. Passing the standard Camera Manager pointer here would 
    // cause a crash because the engine expects a specific "Freecam Context" 
    // structure. We resolve this context by reading a specific offset from 
    // the Freecam Global Object.
    uintptr_t base_obj = gameData.GetFreecamGlobalObject();
    if (!base_obj) {
      logger->Error("[CameraSystem] SwitchTo(0) failed: Freecam global object is null.");
      return;
    }

    uintptr_t context_offset = gameData.GetFreecamContextOffset();
    if (context_offset == 0) {
      logger->Error("[CameraSystem] SwitchTo(0) failed: Freecam context offset is missing.");
      return;
    }

    uintptr_t freeCamInitContext = *reinterpret_cast<uintptr_t*>(base_obj + context_offset);
    if (!freeCamInitContext) {
      logger->Error("[CameraSystem] SwitchTo(0) failed: Resolved init context is null.");
      return;
    }

    m_initializeCameraFunc(freeCamInitContext, 0);
  }
  
  logger->Info("[CameraSystem] Switched to camera ID: {}", cameraID);
}

GameCameraType GameCameraManager::GetCurrentCameraType() {
  if (!m_isReady || !Data::GameData::GameDataCameraService::GetInstance().IsReady()) {
    return static_cast<GameCameraType>(-1);
  }

  // Get data fresh from the source services to ensure it's valid.
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  
  // GetCameraManager() handles the pointer dereferencing and version-specific adjustments.
  uintptr_t standardManagerPtr = gameData.GetCameraManager();
  intptr_t camera_id_offset = gameData.GetActiveCameraIdOffset();

  // Explicit safety checks for the manager pointer and the ID offset.
  if (!standardManagerPtr) {
    return static_cast<GameCameraType>(-1);
  }

  if (camera_id_offset == 0) {
    return static_cast<GameCameraType>(-1);
  }

  uintptr_t addressOfCameraId = standardManagerPtr + camera_id_offset;

  // Reading from game memory, be safe.
  if (IsBadReadPtr((void*)addressOfCameraId, sizeof(uint32_t))) {
    return static_cast<GameCameraType>(-1);
  }

  return static_cast<GameCameraType>(*(uint32_t*)addressOfCameraId);
}

uintptr_t GameCameraManager::GetVerifiedCameraObject(GameCameraType cameraType) {
  auto& gameData = Data::GameData::GameDataCameraService::GetInstance();
  
  // 1. Check if we already verified and cached this address
  uintptr_t verifiedAddr = gameData.GetVerifiedCamera(cameraType);
  if (verifiedAddr != 0) return verifiedAddr;

  // 2. Not cached yet. Perform lazy verification (Function call vs Array discovery)
  auto& hooks = Hooks::CameraHooks::GetInstance();
  auto getCamObjFunc = hooks.GetGetCameraObjectFunc();
  if (!getCamObjFunc) return 0;

  uintptr_t managerAddr = gameData.GetCameraManager();
  if (!managerAddr) return 0;

  uint32_t id = static_cast<uint32_t>(cameraType);
  uintptr_t addrFromFunc = reinterpret_cast<uintptr_t>(getCamObjFunc((void*)managerAddr, id));
  uintptr_t addrFromArray = gameData.GetDiscoveredAddress(static_cast<int>(id));

  // 3. Compare results and log any discrepancies
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
  
  if (addrFromFunc != 0) {
    if (addrFromArray != 0 && addrFromFunc != addrFromArray) {
      logger->Warn("[CameraSystem] Verification MISMATCH for Camera ID {}. Function: 0x{:X}, Array: 0x{:X}. Trusting Function result.", 
                   id, addrFromFunc, addrFromArray);
    }
    
    // Register the final verified address in the service cache
    gameData.RegisterVerifiedCamera(cameraType, addrFromFunc);
    
    // Logic check passed (either matched or function provided valid non-null address)
    return addrFromFunc;
  }

  return 0;
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

  // Re-create standalone debug services
  m_debugCamera = std::make_unique<GameCameraDebug>();
  m_debugStateCamera = std::make_unique<GameCameraDebugState>();
  m_debugAnimationController = std::make_unique<GameCameraDebugAnimation>();

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

  auto photoCam = std::make_unique<GameCameraPhoto>();
  m_cameras[photoCam->GetType()] = std::move(photoCam);
  logger->Info("  -> Registered {}", typeid(GameCameraPhoto).name());

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
