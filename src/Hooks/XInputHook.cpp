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

// Pointer to the original function
static XInputGetState_t oXInputGetState = nullptr;

// Pointer to the target function address, which will persist across reloads
static void* pXInputGetStateTarget = nullptr;

// Storage for previous gamepad states to detect changes
static XINPUT_GAMEPAD g_previousGamepads[XUSER_MAX_COUNT] = {};

// State for the correct blocking mechanism
static DWORD s_lastPacketNumber = 0;
static WORD s_blockedButtonsMask = 0;

// Our hooked function
DWORD WINAPI HookedXInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState) {
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
  DWORD result = oXInputGetState(dwUserIndex, pState);

  if (result == ERROR_SUCCESS && pState) {
    // Get the classified device type from the manager, which now knows about this device's SubType
    SPF::System::DeviceType classifiedType = inputManager.GetXInputDeviceType(dwUserIndex);

    // Frame-level block persistence logic
    if (pState->dwPacketNumber != s_lastPacketNumber) {
      s_blockedButtonsMask = 0;
      s_lastPacketNumber = pState->dwPacketNumber;
    }

    // Now, apply the block mask from any previous polls within this same input frame.
    pState->Gamepad.wButtons &= ~s_blockedButtonsMask;

    // For change detection, compare the TRUE current state with the TRUE previous state.
    const XINPUT_GAMEPAD& currentState = pState->Gamepad;
    const XINPUT_GAMEPAD& previousState = g_previousGamepads[dwUserIndex];

    if (classifiedType == SPF::System::DeviceType::Xbox) {
      // --- Standard Gamepad Processing (All Inputs) ---
      inputManager.SetXInputDeviceActive(true);

      // --- Process Buttons ---
      WORD pressedButtons = currentState.wButtons & ~previousState.wButtons;
      WORD releasedButtons = previousState.wButtons & ~currentState.wButtons;

      auto processButton = [&](SPF::System::GamepadButton btn, WORD xbtn) {
        bool block = false;
        if (pressedButtons & xbtn) {
          block = inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, true, 1.0f});
        } else if (releasedButtons & xbtn) {
          inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, false, 0.0f});
        }
        if (block) {
          pState->Gamepad.wButtons &= ~xbtn;
          s_blockedButtonsMask |= xbtn;
        }
      };

      processButton(SPF::System::GamepadButton::FaceDown, XINPUT_GAMEPAD_A);
      processButton(SPF::System::GamepadButton::FaceRight, XINPUT_GAMEPAD_B);
      processButton(SPF::System::GamepadButton::FaceLeft, XINPUT_GAMEPAD_X);
      processButton(SPF::System::GamepadButton::FaceUp, XINPUT_GAMEPAD_Y);
      processButton(SPF::System::GamepadButton::DPadUp, XINPUT_GAMEPAD_DPAD_UP);
      processButton(SPF::System::GamepadButton::DPadDown, XINPUT_GAMEPAD_DPAD_DOWN);
      processButton(SPF::System::GamepadButton::DPadLeft, XINPUT_GAMEPAD_DPAD_LEFT);
      processButton(SPF::System::GamepadButton::DPadRight, XINPUT_GAMEPAD_DPAD_RIGHT);
      processButton(SPF::System::GamepadButton::LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
      processButton(SPF::System::GamepadButton::RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);
      processButton(SPF::System::GamepadButton::SpecialLeft, XINPUT_GAMEPAD_BACK);
      processButton(SPF::System::GamepadButton::SpecialRight, XINPUT_GAMEPAD_START);
      processButton(SPF::System::GamepadButton::LeftStick, XINPUT_GAMEPAD_LEFT_THUMB);
      processButton(SPF::System::GamepadButton::RightStick, XINPUT_GAMEPAD_RIGHT_THUMB);

      // --- Process Triggers ---
      auto processTrigger = [&](SPF::System::GamepadButton btn, BYTE currentVal, const BYTE& prevVal) {
        const float deadzone = XINPUT_GAMEPAD_TRIGGER_THRESHOLD / 255.0f;
        float normValue = static_cast<float>(currentVal) / 255.0f;
        bool isPressed = normValue >= deadzone;
        bool wasPressed = (static_cast<float>(prevVal) / 255.0f) >= deadzone;
        if (isPressed != wasPressed) {
          inputManager.PublishGamepadEvent({(int)dwUserIndex, btn, isPressed, normValue});
        }
        if (isPressed || wasPressed) {
          inputManager.ProcessAndDecide({(int)dwUserIndex, btn, false, normValue});
        }
      };
      processTrigger(SPF::System::GamepadButton::LeftTrigger, currentState.bLeftTrigger, previousState.bLeftTrigger);
      processTrigger(SPF::System::GamepadButton::RightTrigger, currentState.bRightTrigger, previousState.bRightTrigger);

      // --- Process Analog Sticks ---
      auto processStick = [&](SPF::System::GamepadButton btnX, SPF::System::GamepadButton btnY, SHORT cX, SHORT cY, const SHORT& pX, const SHORT& pY) {
        const float deadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
        float normX = std::abs(static_cast<float>(cX) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(cX) / 32767.0f;
        float normY = std::abs(static_cast<float>(cY) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(cY) / 32767.0f;
        float pNormX = std::abs(static_cast<float>(pX) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(pX) / 32767.0f;
        float pNormY = std::abs(static_cast<float>(pY) / 32767.0f) < deadzone ? 0.0f : static_cast<float>(pY) / 32767.0f;
        if (std::abs(normX - pNormX) > 0.01f) {
          inputManager.ProcessAndDecide({(int)dwUserIndex, btnX, false, normX});
        }
        if (std::abs(normY - pNormY) > 0.01f) {
          inputManager.ProcessAndDecide({(int)dwUserIndex, btnY, false, normY});
        }
      };
      processStick(SPF::System::GamepadButton::LeftStickX, SPF::System::GamepadButton::LeftStickY, currentState.sThumbLX, currentState.sThumbLY, previousState.sThumbLX, previousState.sThumbLY);
      processStick(SPF::System::GamepadButton::RightStickX, SPF::System::GamepadButton::RightStickY, currentState.sThumbRX, currentState.sThumbRY, previousState.sThumbRX, previousState.sThumbRY);

    } else if (classifiedType == SPF::System::DeviceType::Joystick) {
      // --- Generic Joystick Processing (Buttons Only) ---
      inputManager.SetXInputDeviceActive(true); // Still an XInput device
      WORD pressedButtons = currentState.wButtons & ~previousState.wButtons;
      WORD releasedButtons = previousState.wButtons & ~currentState.wButtons;

      auto processJoystickButton = [&](WORD xbtn, int buttonIndex) {
        bool block = false;
        if (pressedButtons & xbtn) {
          block = inputManager.PublishJoystickEvent({buttonIndex, true});
        } else if (releasedButtons & xbtn) {
          inputManager.PublishJoystickEvent({buttonIndex, false});
        }
        if (block) {
          pState->Gamepad.wButtons &= ~xbtn;
          s_blockedButtonsMask |= xbtn;
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
      // Triggers and sticks are intentionally not processed for the Joystick type
    }

    // This is crucial and must happen regardless of device type to prevent repeat-press events.
    g_previousGamepads[dwUserIndex] = currentState;
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
  if (!pXInputGetStateTarget) {
    logger->Error("Cannot find XInputGetState in {}.", dllName);
    return false;
  }

  if (auto err = MH_CreateHook(pXInputGetStateTarget, reinterpret_cast<LPVOID>(&HookedXInputGetState), reinterpret_cast<LPVOID*>(&oXInputGetState)); err != MH_OK) {
    logger->Error("Failed to create hook for XInputGetState: {}", MH_StatusToString(err));
    pXInputGetStateTarget = nullptr;
    return false;
  }

  if (auto err = MH_EnableHook(pXInputGetStateTarget); err != MH_OK) {
    logger->Error("Failed to enable hook for XInputGetState: {}", MH_StatusToString(err));
    return false;
  }

  logger->Info("XInput hook installed successfully on {}.", dllName);
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
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("XInputHook");
    logger->Info("XInput hook removed.");

    pXInputGetStateTarget = nullptr;
    oXInputGetState = nullptr;
  }
}
}  // namespace Hooks

SPF_NS_END