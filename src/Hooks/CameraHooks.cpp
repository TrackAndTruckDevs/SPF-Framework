#include "SPF/Hooks/CameraHooks.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Utils/FinderLog.hpp"
#include "SPF/Utils/PatternFinder.hpp"

#include <cstdint>

SPF_NS_BEGIN
namespace Hooks {
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(InitializeCamera[1405c09e0]) ---/
* 1405c09f2  48 8B 1D AF 42 F9 02          MOV RBX,qword ptr [0x143554ca8]
* 1405c09f9  8B FA                         MOV EDI,EDX
* 1405c09fb  48 8B F1                      MOV RSI,RCX
* 1405c09fe  83 7B 10 0E                   CMP dword ptr [RBX + 0x10],0xe
* 1405c0a02  89 53 14                      MOV dword ptr [RBX + 0x14],EDX
* 1405c0a05  75 08                         JNZ 0x1405c0a0f
 */
CameraHooks::CameraHooks() : m_signature("[MOV r64, [rip+off32]] [MOV r32, r32] [MOV r64, r64] ? ? ? ? [MOV [r64+off8], r32] [JNE rel8]") {}

// Internal signature for the secondary camera function.
namespace {
/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(GetCameraObjectByID[1404f0190]) ---/
 * 1404f0190  48 83 EC 48                   SUB RSP,0x48
 * 1404f0194  4C 63 C2                      MOVSXD R8,EDX
 * 1404f0197  4C 3B 41 40                   CMP R8,qword ptr [RCX + 0x40]
 * 1404f019b  72 07                         JC 0x1404f01a4
 */
const char* GET_CAMERA_OBJECT_SIG = "[SUB r64, imm8] [MOVSXD r64, r32] [CMP r64, [r64+off8]] [JB rel8]";

/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(UpdateCameraProjection[140771010]) ---/
 * 140771010  48 89 5C 24 08                MOV qword ptr [RSP + 0x8],RBX
 * 140771015  48 89 74 24 10                MOV qword ptr [RSP + 0x10],RSI
 * 14077101a  57                            PUSH RDI
 * 14077101b  48 83 EC 70                   SUB RSP,0x70
 * 14077101f  0F B6 41 2C                   MOVZX EAX,byte ptr [RCX + 0x2c]
 * 140771023  48 8D 51 3C                   LEA RDX,[RCX + 0x3c]
 * 140771027  F3 0F 5E CA                   DIVSS XMM1,XMM2
 */
const char* UPDATE_CAMERA_PROJECTION_SIG = "[MOV [r64+off8], r64] [MOV [r64+off8], r64] [PUSH r64] [SUB r64, imm8] [MOVZX r32, [r64+off8]] [LEA r64, [r64+off8]] [DIVSS xmm, xmm]";

/** /--- Ghidra:(amtrucks_1_60.exe) Fun:(DebugCamera_HandleInput[14053e760]) ---/
 * 14053ef66  48 8D 0D 6B 0D BF 01          LEA RCX,[0x14212fcd8] = "Camera roll speed: %.2f"
 */
const char* DEBUG_CAMERA_HANDLE_INPUT_STR = "Camera roll speed: %.2f";

// const char* EXECUTE_COMMAND_SIG = "48 89 5c ? ? 48 89 ? ? ? ? 48 83 ? ? 48 69 ? ? ? ? ? ? ? ? ? 48 8b ? ? ? ? ? 48 8b f2"; //for Photo Camera
// const char* ACTIVATE_CAMERA_BY_ID_SIG = "40 53 48 83 ec 50 83 79 ? ? 48 8b d9 0f ? ? ? ? 0f 28 f1";  //for Photo Camera
}  // namespace

CameraHooks& CameraHooks::GetInstance() {
  static CameraHooks instance;
  return instance;
}

bool CameraHooks::Install() {
  Utils::FinderLog log(m_name);

  if (IsInstalled()) {
    log.Info("Core camera functions already found. Skipping installation.");
    return true;
  }

  log.Info("Searching for core camera functions...");

  // ── Phase 1: Core Camera Functions ──
  {
    auto phase = log.MakePhase("Core Camera Functions");

    // --- 1. Find InitializeCamera ---
    // Anchor on a stable mid-function sequence, then resolve the true prologue
    // via Windows exception tables (.pdata) — reliable across game versions.
    uintptr_t initCamAnchor = Utils::PatternFinder::Find(m_signature.c_str());
    if (!phase.Step(initCamAnchor, "InitializeCamera (anchor)", "RT")) {
      log.Error("The camera system will be unavailable.");
      return log.Finish(false);
    }
    uintptr_t initCamAddr = Utils::PatternFinder::GetFunctionStart(initCamAnchor);
    if (!phase.Step(initCamAddr, "InitializeCamera (.pdata prologue)", "FN")) {
      log.Error("The camera system will be unavailable.");
      return log.Finish(false);
    }
    m_initializeCameraFunc = reinterpret_cast<InitializeCameraFunc>(initCamAddr);

    // --- 2. Find GetCameraObject ---
    uintptr_t getCamObjAddr = Utils::PatternFinder::Find(GET_CAMERA_OBJECT_SIG);
    if (!phase.Step(getCamObjAddr, "GetCameraObject", "FN")) {
      log.Error("The camera system will be unavailable.");
      Uninstall();  // Clean up partially installed hooks
      return log.Finish(false);
    }
    m_getCameraObjectFunc = reinterpret_cast<GetCameraObjectFunc>(getCamObjAddr);

    // --- 3. Find UpdateCameraProjection ---
    uintptr_t updateProjAddr = Utils::PatternFinder::Find(UPDATE_CAMERA_PROJECTION_SIG);
    if (!phase.Step(updateProjAddr, "UpdateCameraProjection", "FN")) {
      log.Error("The camera system will be unavailable.");
      Uninstall();  // Clean up partially installed hooks
      return log.Finish(false);
    }
    m_updateCameraProjectionFunc = reinterpret_cast<UpdateCameraProjectionFunc>(updateProjAddr);
  }

  // ── Phase 2: Debug Camera Input (optional) ──
  {
    auto phase = log.MakePhase("Debug Camera Input");

    // --- 4. Find DebugCamera_HandleInput ---
    uintptr_t handleInputAddr = Utils::PatternFinder::FindFunctionByString(DEBUG_CAMERA_HANDLE_INPUT_STR, true);
    if (phase.StepOptional(handleInputAddr, "DebugCamera_HandleInput", "FN")) {
      m_debugCameraHandleInputFunc = handleInputAddr;
    } else {
      log.Warn("Some free camera features (like mouse look) may be unavailable.");
      // Not critical enough to fail the entire installation.
    }
  }

  //  //for Photo Camera
  //   // --- 5. Find ExecuteCommand ---
  //   uintptr_t execCmdAddr = Utils::PatternFinder::Find(EXECUTE_COMMAND_SIG);
  //   if (execCmdAddr) {
  //     m_executeCommandFunc = reinterpret_cast<ExecuteCommandFunc>(execCmdAddr);
  //     logger->Info("Found 'ExecuteCommand' function at address: {:#x}", execCmdAddr);
  //   } else {
  //     logger->Critical("'ExecuteCommand' function not found! UI commands will be unavailable.");
  //     Uninstall();
  //     return false;
  //   }
  //  //for Photo Camera
  //   // --- 6. Find ActivateCameraByID ---
  //   uintptr_t activateByIDAddr = Utils::PatternFinder::Find(ACTIVATE_CAMERA_BY_ID_SIG);
  //   if (activateByIDAddr) {
  //     m_activateCameraByIDFunc = reinterpret_cast<ActivateCameraByIDFunc>(activateByIDAddr);
  //     logger->Info("Found 'ActivateCameraByID' function at address: {:#x}", activateByIDAddr);
  //   } else {
  //     logger->Warn("'ActivateCameraByID' function not found. Cleanup logic for Photo Mode will be unavailable.");
  //   }

  //   logger->Info("Core camera functions installed successfully.");
  //   return true;

  log.Info("Core camera functions installed successfully.");
  return log.Finish(true);
}

void CameraHooks::Uninstall() {
  if (m_initializeCameraFunc || m_getCameraObjectFunc || m_updateCameraProjectionFunc || m_debugCameraHandleInputFunc) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger(m_name);
    logger->Info("Disabling core camera functions, clearing pointers.");
    m_initializeCameraFunc = nullptr;
    m_getCameraObjectFunc = nullptr;
    m_updateCameraProjectionFunc = nullptr;
    // m_executeCommandFunc = nullptr; //for Photo Camera
    // m_activateCameraByIDFunc = nullptr; //for Photo Camera
    m_debugCameraHandleInputFunc = 0;
  }
}

void CameraHooks::Remove() {
  // For this class, Remove is the same as Uninstall as it's non-destructive.
  Uninstall();
}
}  // namespace Hooks
SPF_NS_END
