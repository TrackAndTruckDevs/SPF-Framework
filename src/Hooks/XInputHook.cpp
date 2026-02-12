#include "SPF/Hooks/XInputHook.hpp"

#include <Windows.h>
#include <Xinput.h>
#include <MinHook.h>

#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/System/GamepadButton.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"

// --- State Management ---
// Define the function pointer type for the original XInputGetState
typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);

// Pointer to the original functions
static XInputGetState_t oXInputGetState = nullptr;
static XInputGetState_t oXInputGetStateEx = nullptr;

// Pointer to the target function addresses
static void* pXInputGetStateTarget = nullptr;
static void* pXInputGetStateExTarget = nullptr;

// Storage for previous gamepad states to detect changes
static XINPUT_GAMEPAD g_previousGamepads[XUSER_MAX_COUNT] = {};

// State for the correct blocking mechanism, now per-controller
static DWORD s_lastPacketNumber[XUSER_MAX_COUNT] = {0};
static WORD s_blockedButtonsMask[XUSER_MAX_COUNT] = {0};

// --- Internal Helper Functions ---

static void AnalyzeXboxInput(DWORD dwUserIndex, const XINPUT_GAMEPAD& cur, const XINPUT_GAMEPAD& prev) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();
  auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();

  inputManager.SetXInputDeviceActive(true);

  // 1. Process Digital Buttons
  static const WORD allButtons[] = {
      XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
      XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
      XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
      XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB
  };

  for (WORD xbtn : allButtons) {
    SPF::System::GamepadButton btn = mapping.FromXInput(xbtn);
    if (btn == SPF::System::GamepadButton::Unknown) continue;

    bool isPressed = (cur.wButtons & xbtn) != 0;
    bool wasPressed = (prev.wButtons & xbtn) != 0;

    if (isPressed != wasPressed) {
      inputManager.PublishGamepadEvent(SPF::Input::GamepadEvent{(int)dwUserIndex, btn, isPressed, isPressed ? 1.0f : 0.0f}, 1);
    } else if (isPressed) {
      // Still pressed, allow state machine to evaluate long press/chords
      inputManager.ProcessAndDecide(SPF::Input::GamepadEvent{(int)dwUserIndex, btn, true, 1.0f}, 1);
    }
  }

    // 2. Process Triggers
  auto processTrigger = [&](SPF::System::GamepadButton axisBtn, SPF::System::GamepadButton digitalBtn, int axisIdx, BYTE curVal, BYTE prevVal) {
    float normValue = static_cast<float>(curVal) / 255.0f;
    float prevNorm = static_cast<float>(prevVal) / 255.0f;
    
    // --- RAW DIAGNOSTIC LOG ---
    // if (curVal > 5 || prevVal > 5) {
    //     auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
    //     logger->Info("[RawXInput] Axis: {} (Trigger), Value: {:.4f}", axisIdx, normValue);
    // }

    // Publish as Analog Axis (Raw) only if changed or active
    if (std::abs(normValue - prevNorm) > 0.001f || normValue > 0.001f) {
        inputManager.PublishAxisMove(0x02, axisIdx, normValue, 1);
    }

    // Legacy Button Support (using default XInput threshold for digital actions)
    const float triggerThreshold = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
    bool isPressed = normValue >= triggerThreshold;
    bool wasPressed = (static_cast<float>(prevVal) / 255.0f) >= triggerThreshold;

    if (isPressed != wasPressed) {
      inputManager.PublishGamepadEvent(SPF::Input::GamepadEvent{(int)dwUserIndex, digitalBtn, isPressed, normValue}, 1);
    } else if (isPressed) {
      inputManager.ProcessAndDecide(SPF::Input::GamepadEvent{(int)dwUserIndex, digitalBtn, true, normValue}, 1);
    }
  };
  processTrigger(SPF::System::GamepadButton::LeftTriggerAxis, SPF::System::GamepadButton::LeftTrigger, 4, cur.bLeftTrigger, prev.bLeftTrigger);
  processTrigger(SPF::System::GamepadButton::RightTriggerAxis, SPF::System::GamepadButton::RightTrigger, 5, cur.bRightTrigger, prev.bRightTrigger);

  // 3. Process Analog Sticks
  auto processStick = [&](SPF::System::GamepadButton btnX, SPF::System::GamepadButton btnY, int axisIdxX, int axisIdxY, SHORT curX, SHORT curY, SHORT prevX, SHORT prevY) {
    auto normalize = [&](SHORT val) {
        return static_cast<float>(val) / 32767.0f;
    };
    float nX = normalize(curX), nY = normalize(curY);
    float pX = normalize(prevX), pY = normalize(prevY);

    // Publish only if moved or not at zero
    if (std::abs(nX - pX) > 0.001f || std::abs(nX) > 0.001f) {
        inputManager.PublishAxisMove(0x02, axisIdxX, nX, 1);
    }
    if (std::abs(nY - pY) > 0.001f || std::abs(nY) > 0.001f) {
        inputManager.PublishAxisMove(0x02, axisIdxY, nY, 1);
    }

    // Call ProcessAndDecide ONLY during capture or if the axis is used as a digital button
    // To prevent massive overhead, we only pass it through if capture is active.
    if (inputManager.GetCaptureState() == SPF::Input::InputManager::InputCaptureState::Capturing) {
        if (std::abs(nX - pX) > 0.01f) inputManager.ProcessAndDecide(SPF::Input::GamepadEvent{(int)dwUserIndex, btnX, false, nX}, 1);
        if (std::abs(nY - pY) > 0.01f) inputManager.ProcessAndDecide(SPF::Input::GamepadEvent{(int)dwUserIndex, btnY, false, nY}, 1);
    }
  };
  processStick(SPF::System::GamepadButton::LeftStickX, SPF::System::GamepadButton::LeftStickY, 0, 1, cur.sThumbLX, cur.sThumbLY, prev.sThumbLX, prev.sThumbLY);
  processStick(SPF::System::GamepadButton::RightStickX, SPF::System::GamepadButton::RightStickY, 2, 3, cur.sThumbRX, cur.sThumbRY, prev.sThumbRX, prev.sThumbRY);
}

static void AnalyzeJoystickInput(DWORD dwUserIndex, const XINPUT_GAMEPAD& cur, const XINPUT_GAMEPAD& prev) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();
  inputManager.SetXInputDeviceActive(true);

  WORD pressed = cur.wButtons & ~prev.wButtons;
  WORD released = prev.wButtons & ~cur.wButtons;
  WORD held = cur.wButtons & prev.wButtons;

  auto processBtn = [&](WORD xbtn, int idx) {
    if (pressed & xbtn) inputManager.PublishJoystickEvent(SPF::Input::JoystickEvent{idx, true});
    else if (released & xbtn) inputManager.PublishJoystickEvent(SPF::Input::JoystickEvent{idx, false});
    else if (held & xbtn) inputManager.ProcessAndDecide(SPF::Input::JoystickEvent{idx, true});
  };

  processBtn(XINPUT_GAMEPAD_A, 0);
  processBtn(XINPUT_GAMEPAD_B, 1);
  processBtn(XINPUT_GAMEPAD_X, 2);
  processBtn(XINPUT_GAMEPAD_Y, 3);
  processBtn(XINPUT_GAMEPAD_LEFT_SHOULDER, 4);
  processBtn(XINPUT_GAMEPAD_RIGHT_SHOULDER, 5);
  processBtn(XINPUT_GAMEPAD_BACK, 6);
  processBtn(XINPUT_GAMEPAD_START, 7);
  processBtn(XINPUT_GAMEPAD_LEFT_THUMB, 8);
  processBtn(XINPUT_GAMEPAD_RIGHT_THUMB, 9);
  processBtn(XINPUT_GAMEPAD_DPAD_UP, 10);
  processBtn(XINPUT_GAMEPAD_DPAD_DOWN, 11);
  processBtn(XINPUT_GAMEPAD_DPAD_LEFT, 12);
  processBtn(XINPUT_GAMEPAD_DPAD_RIGHT, 13);
}

static void MaskXInputState(DWORD dwUserIndex, XINPUT_STATE* pState, const XINPUT_GAMEPAD& hardwareState) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
    SPF::System::DeviceType type = inputManager.GetXInputDeviceType(dwUserIndex);

    bool anyMasked = false;

    if (type == SPF::System::DeviceType::Xbox) {
        // Mask Buttons
        static const WORD allButtons[] = {
            XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
            XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
            XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
            XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB
        };

        for (WORD xbtn : allButtons) {
            SPF::System::GamepadButton btn = mapping.FromXInput(xbtn);
            if (btn == SPF::System::GamepadButton::Unknown) continue;

            bool virtualRelease = inputManager.ConsumeGamepadReleaseRequest(btn);
            if (virtualRelease || inputManager.IsGamepadButtonBlocked(btn)) {
                pState->Gamepad.wButtons &= ~xbtn;
                s_blockedButtonsMask[dwUserIndex] |= xbtn;
                anyMasked = true;
            }
        }

        // Mask Triggers
        if (inputManager.IsAxisConsumed(0x02, 4) || inputManager.IsGamepadButtonBlocked(SPF::System::GamepadButton::LeftTrigger)) {
            pState->Gamepad.bLeftTrigger = 0;
            anyMasked = true;
        }
        if (inputManager.IsAxisConsumed(0x02, 5) || inputManager.IsGamepadButtonBlocked(SPF::System::GamepadButton::RightTrigger)) {
            pState->Gamepad.bRightTrigger = 0;
            anyMasked = true;
        }
        
        // Mask Analog Sticks
        if (inputManager.IsAxisConsumed(0x02, 0)) { pState->Gamepad.sThumbLX = 0; anyMasked = true; }
        if (inputManager.IsAxisConsumed(0x02, 1)) { pState->Gamepad.sThumbLY = 0; anyMasked = true; }
        if (inputManager.IsAxisConsumed(0x02, 2)) { pState->Gamepad.sThumbRX = 0; anyMasked = true; }
        if (inputManager.IsAxisConsumed(0x02, 3)) { pState->Gamepad.sThumbRY = 0; anyMasked = true; }
    } 
    else if (type == SPF::System::DeviceType::Joystick) {
        static const std::pair<WORD, int> joyButtons[] = {
            {XINPUT_GAMEPAD_A, 0}, {XINPUT_GAMEPAD_B, 1}, {XINPUT_GAMEPAD_X, 2}, {XINPUT_GAMEPAD_Y, 3},
            {XINPUT_GAMEPAD_LEFT_SHOULDER, 4}, {XINPUT_GAMEPAD_RIGHT_SHOULDER, 5}, {XINPUT_GAMEPAD_BACK, 6},
            {XINPUT_GAMEPAD_START, 7}, {XINPUT_GAMEPAD_LEFT_THUMB, 8}, {XINPUT_GAMEPAD_RIGHT_THUMB, 9},
            {XINPUT_GAMEPAD_DPAD_UP, 10}, {XINPUT_GAMEPAD_DPAD_DOWN, 11}, {XINPUT_GAMEPAD_DPAD_LEFT, 12}, {XINPUT_GAMEPAD_DPAD_RIGHT, 13}
        };

        for (const auto& pair : joyButtons) {
            if (inputManager.IsJoystickButtonBlocked(pair.second)) {
                pState->Gamepad.wButtons &= ~pair.first;
                s_blockedButtonsMask[dwUserIndex] |= pair.first;
                anyMasked = true;
            }
        }
    }

    if (anyMasked) {
        pState->dwPacketNumber++;
    }
}

// Forward declaration
static DWORD InternalProcessXInputState(DWORD dwUserIndex, XINPUT_STATE* pState, XInputGetState_t originalFunc);

// Our hooked functions
DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
  return InternalProcessXInputState(dwUserIndex, pState, oXInputGetState);
}

DWORD WINAPI HookedXInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState) {
  return InternalProcessXInputState(dwUserIndex, pState, oXInputGetStateEx);
}

static DWORD InternalProcessXInputState(DWORD dwUserIndex, XINPUT_STATE* pState, XInputGetState_t originalFunc) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();

  // --- One-time SubType detection ---
  static bool registered[XUSER_MAX_COUNT] = {false};
  if (dwUserIndex < XUSER_MAX_COUNT && !registered[dwUserIndex]) {
    XINPUT_CAPABILITIES caps;
    if (XInputGetCapabilities(dwUserIndex, 0, &caps) == ERROR_SUCCESS) {
      inputManager.RegisterXInputDevice(dwUserIndex, caps.SubType);
    }
    registered[dwUserIndex] = true;
  }

  DWORD result = originalFunc(dwUserIndex, pState);

  if (result == ERROR_SUCCESS && pState) {
    const XINPUT_GAMEPAD hardwareState = pState->Gamepad;
    const XINPUT_GAMEPAD& previousState = g_previousGamepads[dwUserIndex];
    SPF::System::DeviceType classifiedType = inputManager.GetXInputDeviceType(dwUserIndex);

    // 1. Frame-level block persistence
    if (pState->dwPacketNumber != s_lastPacketNumber[dwUserIndex]) {
      s_blockedButtonsMask[dwUserIndex] = 0;
      s_lastPacketNumber[dwUserIndex] = pState->dwPacketNumber;
    }
    pState->Gamepad.wButtons &= ~s_blockedButtonsMask[dwUserIndex];

    // 2. Analyze Input (Publish events)
    if (classifiedType == SPF::System::DeviceType::Xbox) {
      AnalyzeXboxInput(dwUserIndex, hardwareState, previousState);
    } else {
      AnalyzeJoystickInput(dwUserIndex, hardwareState, previousState);
    }

    // 3. Mask State (Block input from game)
    MaskXInputState(dwUserIndex, pState, hardwareState);

    g_previousGamepads[dwUserIndex] = hardwareState;
  }

  return result;
}

SPF_NS_BEGIN

namespace Hooks {
bool XInputHook::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");

  if (pXInputGetStateTarget != nullptr) {
    logger->Info("XInput hook already created, enabling it...");
    if (auto err = MH_EnableHook(pXInputGetStateTarget); err != MH_OK && err != MH_ERROR_ENABLED) {
      logger->Error("Failed to re-enable hook for XInputGetState: {}", MH_StatusToString(err));
      return false;
    }
    logger->Info("XInput hook enabled.");
    return true;
  }

  logger->Info("Installing XInput hook for the first time...");

  // XInput is usually available in a few different DLL names.
  // xinput1_4.dll is the most modern one, included with Windows 8+.
  // xinput1_3.dll is older but very common.
  // We target them in order of preference.
  const char* dllName = "xinput1_4.dll";
  HMODULE hMod = GetModuleHandleA(dllName);
  if (!hMod) {
    dllName = "xinput1_3.dll";
    hMod = GetModuleHandleA(dllName);
  }
  if (!hMod) {
    logger->Warn("XInput library (1_4 or 1_3) not found in game process. Hook will not be installed.");
    return true;  // Return true because this is not a fatal error; the game might not use XInput.
  }

  pXInputGetStateTarget = reinterpret_cast<void*>(GetProcAddress(hMod, "XInputGetState"));
  if (pXInputGetStateTarget) {
    if (auto err = MH_CreateHook(pXInputGetStateTarget, reinterpret_cast<LPVOID>(&HookedXInputGetState), reinterpret_cast<LPVOID*>(&oXInputGetState)); err != MH_OK) {
      logger->Error("Failed to create hook for XInputGetState: {}", MH_StatusToString(err));
    } else if (auto err = MH_EnableHook(pXInputGetStateTarget); err != MH_OK) {
      logger->Error("Failed to enable hook for XInputGetState: {}", MH_StatusToString(err));
    }
  }

  // Also hook Ordinal 100 (XInputGetStateEx), commonly used by modern games
  pXInputGetStateExTarget = reinterpret_cast<void*>(GetProcAddress(hMod, (LPCSTR)100));
  if (pXInputGetStateExTarget && pXInputGetStateExTarget != pXInputGetStateTarget) {
    if (auto err = MH_CreateHook(pXInputGetStateExTarget, reinterpret_cast<LPVOID>(&HookedXInputGetState), reinterpret_cast<LPVOID*>(&oXInputGetStateEx)); err != MH_OK) {
      logger->Warn("Note: Could not hook XInput ordinal 100 (not critical).");
    } else if (auto err = MH_EnableHook(pXInputGetStateExTarget); err != MH_OK) {
      logger->Error("Failed to enable hook for XInput ordinal 100.");
    } else {
      logger->Info("Successfully hooked XInputGetStateEx (Ordinal 100).");
    }
  }

  if (!pXInputGetStateTarget && !pXInputGetStateExTarget) {
    logger->Error("Cannot find XInputGetState or ordinal 100 in {}.", dllName);
    return false;
  }

  logger->Info("XInput hook installation complete on {}.", dllName);
  return true;
}

void XInputHook::Uninstall() {
  if (pXInputGetStateTarget) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
    auto status = MH_DisableHook(pXInputGetStateTarget);
    if (status == MH_OK) {
      logger->Info("XInput hook disabled successfully.");
    } else {
      logger->Warn("Failed to disable XInput hook, status: {}", MH_StatusToString(status));
    }
  }
}

void XInputHook::Remove() {
  if (pXInputGetStateTarget) {
    MH_DisableHook(pXInputGetStateTarget);
    MH_RemoveHook(pXInputGetStateTarget);
  }
  if (pXInputGetStateExTarget) {
    MH_DisableHook(pXInputGetStateExTarget);
    MH_RemoveHook(pXInputGetStateExTarget);
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
  logger->Info("XInput hooks removed.");

  pXInputGetStateTarget = nullptr;
  pXInputGetStateExTarget = nullptr;
  oXInputGetState = nullptr;
  oXInputGetStateEx = nullptr;
}
}  // namespace Hooks

SPF_NS_END