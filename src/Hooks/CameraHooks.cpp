#include "SPF/Hooks/CameraHooks.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <Windows.h>

SPF_NS_BEGIN
namespace Hooks {
CameraHooks::CameraHooks() : m_signature("48 89 5C 24 08 48 89 74 24 10 57 48 81 EC ? ? ? ? 48 8B 1D ? ? ? ? 8B FA 48 8B F1") {}

// Internal signature for the secondary camera function.
namespace {
const char* GET_CAMERA_OBJECT_SIG = "48 83 EC 48 4C 63 C2 4C 3B 41 40";
const char* UPDATE_CAMERA_PROJECTION_SIG = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 70 0F B6 41 2C 48 8D 51 3C F3";
const char* DEBUG_CAMERA_HANDLE_INPUT_SIG = "55 56 48 8D ? ? ? FF FF 48 81 EC ? ? ? ? 80 B9 3C 01 00 00 00";
}  // namespace

CameraHooks& CameraHooks::GetInstance() {
  static CameraHooks instance;
  return instance;
}

bool CameraHooks::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);

  if (IsInstalled()) {
    logger->Info("Core camera functions already found. Skipping installation.");
    return true;
  }

  logger->Info("Searching for core camera functions...");

  // --- 1. Find InitializeCamera ---
  /*
   * HOW TO FIND THIS SIGNATURE (InitializeCamera):
   * 1. In Cheat Engine or x64dbg, search for the string \"camera.bank.chase\".
   * 2. Find the code that references this string. You will land in a large function
   *    that handles camera selection based on a string name.
   * 3. Set a breakpoint at the top of this function and switch cameras in the game.
   * 4. Look at the call stack. The function that calls this string-based selection
   *    is the main `InitializeCamera` function we need to hook. Its prologue is
   *    very distinct and has remained stable across many game versions.
   * 5. The signature targets this stable function prologue.
   */
  uintptr_t initCamAddr = Utils::PatternFinder::Find(m_signature.c_str());
  if (initCamAddr) {
    m_initializeCameraFunc = reinterpret_cast<InitializeCameraFunc>(initCamAddr);
    logger->Info("Found 'InitializeCamera' function at address: {:#x}", initCamAddr);
  } else {
    logger->Critical("'InitializeCamera' function not found! The camera system will be unavailable.");
    return false;
  }

  // --- 2. Find GetCameraObject ---
  /*
   * HOW TO FIND THIS SIGNATURE (GetCameraObject):
   * 1. In the `InitializeCamera` function found above, look for a call that takes the camera ID
   *    as a parameter and returns a pointer. This function is often called multiple times
   *    within `InitializeCamera`.
   * 2. Alternatively, set a breakpoint on the code that writes to the camera's FOV or position
   *    (which you can find with Cheat Engine). Look at the call stack to see where the pointer
   *    to the camera object comes from. You will likely see `GetCameraObject` in the call stack.
   * 3. This function is relatively small and has a distinct prologue, which is what this
   *    signature targets.
   */
  uintptr_t getCamObjAddr = Utils::PatternFinder::Find(GET_CAMERA_OBJECT_SIG);
  if (getCamObjAddr) {
    m_getCameraObjectFunc = reinterpret_cast<GetCameraObjectFunc>(getCamObjAddr);
    logger->Info("Found 'GetCameraObject' function at address: {:#x}", getCamObjAddr);
  } else {
    logger->Critical("'GetCameraObject' function not found! The camera system will be unavailable.");
    Uninstall();  // Clean up partially installed hooks
    return false;
  }

  // --- 3. Find UpdateCameraProjection ---
  uintptr_t updateProjAddr = Utils::PatternFinder::Find(UPDATE_CAMERA_PROJECTION_SIG);
  if (updateProjAddr) {
    m_updateCameraProjectionFunc = reinterpret_cast<UpdateCameraProjectionFunc>(updateProjAddr);
    logger->Info("Found 'UpdateCameraProjection' function at address: {:#x}", updateProjAddr);
  } else {
    logger->Critical("'UpdateCameraProjection' function not found! The camera system will be unavailable.");
    Uninstall();  // Clean up partially installed hooks
    return false;
  }

  // --- 4. Find DebugCamera_HandleInput ---
  uintptr_t handleInputAddr = Utils::PatternFinder::Find(DEBUG_CAMERA_HANDLE_INPUT_SIG);
  if (handleInputAddr) {
    // The signature matches at PUSH RBP. The actual function starts 7 bytes before that.
    const intptr_t offset_to_start = -7;
    m_debugCameraHandleInputFunc = handleInputAddr + offset_to_start;
    logger->Info("Found 'DebugCamera_HandleInput' function at address: {:#x}", (uintptr_t)m_debugCameraHandleInputFunc);
  } else {
    logger->Warn("'DebugCamera_HandleInput' function not found. Some free camera features (like mouse look) may be unavailable.");
    // This is not critical enough to fail the entire installation.
  }

  logger->Info("Core camera functions installed successfully.");
  return true;
}

void CameraHooks::Uninstall() {
  if (m_initializeCameraFunc || m_getCameraObjectFunc || m_updateCameraProjectionFunc || m_debugCameraHandleInputFunc) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Disabling core camera functions, clearing pointers.");
    m_initializeCameraFunc = nullptr;
    m_getCameraObjectFunc = nullptr;
    m_updateCameraProjectionFunc = nullptr;
    m_debugCameraHandleInputFunc = 0;
  }
}

void CameraHooks::Remove() {
  // For this class, Remove is the same as Uninstall as it's non-destructive.
  Uninstall();
}
}  // namespace Hooks
SPF_NS_END
