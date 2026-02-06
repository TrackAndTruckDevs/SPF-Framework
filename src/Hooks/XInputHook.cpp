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

// Forward declaration
static DWORD InternalProcessXInputState(DWORD dwUserIndex, XINPUT_STATE* pState, XInputGetState_t originalFunc);

// Our hooked functions
DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
  // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
  // logger->TraceThrottled(std::chrono::seconds(5), "XInput: HookedXInputGetState is active for user={}", dwUserIndex);
  return InternalProcessXInputState(dwUserIndex, pState, oXInputGetState);
}

DWORD WINAPI HookedXInputGetStateEx(DWORD dwUserIndex, XINPUT_STATE* pState) {
  return InternalProcessXInputState(dwUserIndex, pState, oXInputGetStateEx);
}

static DWORD InternalProcessXInputState(DWORD dwUserIndex, XINPUT_STATE* pState, XInputGetState_t originalFunc) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();

  // --- One-time SubType detection and registration with InputManager ---
  static bool registered[XUSER_MAX_COUNT] = {false};
  if (dwUserIndex < XUSER_MAX_COUNT && !registered[dwUserIndex]) {
    XINPUT_CAPABILITIES caps;
    // XInputGetCapabilities will return ERROR_SUCCESS even if no device is connected,
    // but caps.Type will be XINPUT_DEVTYPE_GAMEPAD and caps.SubType will be 0.
    // For now, we will just register whatever subtype is returned.
    if (XInputGetCapabilities(dwUserIndex, 0, &caps) == ERROR_SUCCESS) {
      inputManager.RegisterXInputDevice(dwUserIndex, caps.SubType);
    }
    registered[dwUserIndex] = true;
  }

  // Call the original function first to get the true, unmodified state
  DWORD result = originalFunc(dwUserIndex, pState);

  if (result == ERROR_SUCCESS && pState) {
    // Capture RAW state for logic, compare with TRUE previous state.
    const XINPUT_GAMEPAD hardwareState = pState->Gamepad;

    // Get the classified device type from the manager, which now knows about this device's SubType
    SPF::System::DeviceType classifiedType = inputManager.GetXInputDeviceType(dwUserIndex);

    // Frame-level block persistence logic, now per-controller
    if (pState->dwPacketNumber != s_lastPacketNumber[dwUserIndex]) {
      s_blockedButtonsMask[dwUserIndex] = 0;
      s_lastPacketNumber[dwUserIndex] = pState->dwPacketNumber;
    }

    // Now, apply the block mask from any previous polls within this same input packet.
    pState->Gamepad.wButtons &= ~s_blockedButtonsMask[dwUserIndex];

    const XINPUT_GAMEPAD& previousState = g_previousGamepads[dwUserIndex];
    auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
    bool anyMasked = false;

    if (classifiedType == SPF::System::DeviceType::Xbox) {
      // --- Standard Gamepad Processing (All Inputs) ---
      inputManager.SetXInputDeviceActive(true);

      // --- Process Buttons ---
      WORD allButtons[] = {XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
                           XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
                           XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
                           XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB};

      for (WORD xbtn : allButtons) {
        SPF::System::GamepadButton btn = mapping.FromXInput(xbtn);
        if (btn == SPF::System::GamepadButton::Unknown) continue;

        bool isPressed = (hardwareState.wButtons & xbtn) != 0;
        bool wasPressed = (previousState.wButtons & xbtn) != 0;

        bool block = false;
        if (isPressed && !wasPressed) {
          // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          // logger->Trace("XInput: Physical PRESS for user={}, button={}", dwUserIndex, (int)btn);
          block = inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, true, 1.0f});
        } else if (!isPressed && wasPressed) {
          // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          // logger->Trace("XInput: Physical RELEASE for user={}, button={}", dwUserIndex, (int)btn);
          inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, false, 0.0f});
        }

        // Always check with the manager if this button should be blocked right now.
        if (block || inputManager.IsGamepadButtonBlocked(btn)) {
          // if (!block) {
          //     auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          //     logger->TraceThrottled(std::chrono::milliseconds(500), "XInput: Masking button {} for user={}", (int)btn, dwUserIndex);
          // }
          pState->Gamepad.wButtons &= ~xbtn;
          s_blockedButtonsMask[dwUserIndex] |= xbtn;
          anyMasked = true;
        }
      }

      // --- Process Triggers ---
      auto processTrigger = [&](SPF::System::GamepadButton btn, BYTE& gameVal, BYTE rawCurVal, BYTE rawPrevVal) {
        const float deadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
        float normValue = static_cast<float>(rawCurVal) / 255.0f;
        bool isPressed = normValue >= deadzone;
        bool wasPressed = (static_cast<float>(rawPrevVal) / 255.0f) >= deadzone;
        
        bool block = false;
        if (isPressed != wasPressed) {
          block = inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, isPressed, normValue});
        }
        if (isPressed || wasPressed) {
          block |= inputManager.ProcessAndDecide({(int)dwUserIndex, btn, false, normValue});
        }

        if (block || inputManager.IsGamepadButtonBlocked(btn)) {
            // if (rawCurVal > 0) {
            //     auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
            //     logger->TraceThrottled(std::chrono::milliseconds(500), "XInput: Masking trigger {} for user={}", (int)btn, dwUserIndex);
            // }
            gameVal = 0;
        }
      };
      processTrigger(SPF::System::GamepadButton::LeftTrigger, pState->Gamepad.bLeftTrigger, hardwareState.bLeftTrigger, previousState.bLeftTrigger);
      processTrigger(SPF::System::GamepadButton::RightTrigger, pState->Gamepad.bRightTrigger, hardwareState.bRightTrigger, previousState.bRightTrigger);

      // --- Process Analog Sticks ---
      auto processStick = [&](SPF::System::GamepadButton btnX, SPF::System::GamepadButton btnY, SHORT rawX, SHORT rawY, SHORT rawPX, SHORT rawPY) {
        const float deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        float normX = std::abs(static_cast<float>(rawX) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(rawX) / 32767.0f;
        float normY = std::abs(static_cast<float>(rawY) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(rawY) / 32767.0f;
        float pNormX = std::abs(static_cast<float>(rawPX) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(rawPX) / 32767.0f;
        float pNormY = std::abs(static_cast<float>(rawPY) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(rawPY) / 32767.0f;
        if (std::abs(normX - pNormX) > 0.01f) {
          inputManager.ProcessAndDecide({(int)dwUserIndex, btnX, false, normX});
        }
        if (std::abs(normY - pNormY) > 0.01f) {
          inputManager.ProcessAndDecide({(int)dwUserIndex, btnY, false, normY});
        }
      };
      processStick(SPF::System::GamepadButton::LeftStickX, SPF::System::GamepadButton::LeftStickY, hardwareState.sThumbLX, hardwareState.sThumbLY, previousState.sThumbLX, previousState.sThumbLY);
      processStick(SPF::System::GamepadButton::RightStickX, SPF::System::GamepadButton::RightStickY, hardwareState.sThumbRX, hardwareState.sThumbRY, previousState.sThumbRX, previousState.sThumbRY);

    } else if (classifiedType == SPF::System::DeviceType::Joystick) {
      // --- Generic Joystick Processing (Buttons Only) ---
      inputManager.SetXInputDeviceActive(true); // Still an XInput device
      WORD pressedButtons = hardwareState.wButtons & ~previousState.wButtons;
      WORD releasedButtons = previousState.wButtons & ~hardwareState.wButtons;

      auto processJoystickButton = [&](WORD xbtn, int buttonIndex) {
        bool block = false;
        if (pressedButtons & xbtn) {
          // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          // logger->Trace("XInput (Joystick Mode): Physical PRESS for user={}, button={}", dwUserIndex, buttonIndex);
          block = inputManager.PublishJoystickEvent({buttonIndex, true});
        } else if (releasedButtons & xbtn) {
          // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          // logger->Trace("XInput (Joystick Mode): Physical RELEASE for user={}, button={}", dwUserIndex, buttonIndex);
          inputManager.PublishJoystickEvent({buttonIndex, false});
        }
        
        if (block || inputManager.IsJoystickButtonBlocked(buttonIndex)) {
          // if (!block) {
          //     auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
          //     logger->Trace("XInput (Joystick Mode): Blocking (Retroactive) for user={}, button={}", dwUserIndex, buttonIndex);
          // }
          pState->Gamepad.wButtons &= ~xbtn;
          s_blockedButtonsMask[dwUserIndex] |= xbtn;
          anyMasked = true;
        }
      };

      // Define a consistent mapping from XInput flags to generic joystick button indices
      processJoystickButton(XINPUT_GAMEPAD_A, 0);
      processJoystickButton(XINPUT_GAMEPAD_B, 1);
      processJoystickButton(XINPUT_GAMEPAD_X, 2);
      processJoystickButton(XINPUT_GAMEPAD_Y, 3);
      processJoystickButton(XINPUT_GAMEPAD_LEFT_SHOULDER, 4);
      processJoystickButton(XINPUT_GAMEPAD_RIGHT_SHOULDER, 5);
      processJoystickButton(XINPUT_GAMEPAD_BACK, 6);
      processJoystickButton(XINPUT_GAMEPAD_START, 7);
      processJoystickButton(XINPUT_GAMEPAD_LEFT_THUMB, 8);
      processJoystickButton(XINPUT_GAMEPAD_RIGHT_THUMB, 9);
      processJoystickButton(XINPUT_GAMEPAD_DPAD_UP, 10);
      processJoystickButton(XINPUT_GAMEPAD_DPAD_DOWN, 11);
      processJoystickButton(XINPUT_GAMEPAD_DPAD_LEFT, 12);
      processJoystickButton(XINPUT_GAMEPAD_DPAD_RIGHT, 13);
    }

    // Force packet change if masking is active to ensure the game processes the "release"
    if (anyMasked) {
        pState->dwPacketNumber++;
    }

    // This is crucial and must happen regardless of device type to prevent repeat-press events.
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