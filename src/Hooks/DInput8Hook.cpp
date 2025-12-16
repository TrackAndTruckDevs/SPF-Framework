#include "SPF/Hooks/DInput8Hook.hpp"

#include "SPF/Input/InputEvents.hpp"

#include <vector>
#include <unordered_set>

// Explicitly define DirectInput version before including the header.
#define DIRECTINPUT_VERSION 0x0800

// Enable C-style for COM. This must be BEFORE #include <dinput.h>
// and before any other headers that might include it (MinHook.h).
#define COBJMACROS
#define CINTERFACE
#include <dinput.h>
#include <Windows.h>
#include <MinHook.h>
#include "SPF/Input/InputManager.hpp"

#include "SPF/Logging/LoggerFactory.hpp"



// --- End of new hook pool implementation ---

SPF_NS_BEGIN
namespace Hooks {
namespace {
// Helper function to convert WCHAR strings to UTF-8 std::string using Windows API
std::string WstringToUtf8(const wchar_t* wstr) {
  if (!wstr || wstr[0] == L'\0') {
    return std::string();
  }
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  if (size_needed == 0) {
    return std::string();
  }
  std::string strTo(size_needed - 1, 0);  // size_needed includes the null terminator
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
  return strTo;
}

static const GUID IID_IDirectInput8W_spf = {0xbf798031, 0x483a, 0x4da2, {0xaa, 0x99, 0x5d, 0x64, 0xed, 0x36, 0x97, 0x00}};
static const GUID GUID_SysMouseEm_spf = {0x6F1D2B80, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

typedef HRESULT(WINAPI* GetDeviceData_t)(IDirectInputDevice8W*, DWORD, DIDEVICEOBJECTDATA*, DWORD*, DWORD);

// Maximum number of unique VTables we can hook. Should be plenty.
constexpr int MAX_DINPUT_HOOKS = 8;

// Holds the trampoline for each hook
static GetDeviceData_t g_originalFunctions[MAX_DINPUT_HOOKS] = {nullptr};
static int g_hookCount = 0;

// Forward declaration for the universal processing function.
// It must be declared here because the hook handlers below use it.
static void ProcessDeviceData(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut);

// Define the pool of hook handlers.
// Using a macro to avoid repetitive code.
#define DINPUT_HOOK_HANDLER(index)                                                                                                                           \
  HRESULT __stdcall HookedGetDeviceData_##index(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD dwFlags) { \
    HRESULT result = g_originalFunctions[index](self, cbObjectData, rgdod, pdwInOut, dwFlags);                                                               \
    if (SUCCEEDED(result) && self != nullptr && rgdod != nullptr && pdwInOut != nullptr && *pdwInOut > 0) {                                                  \
      ProcessDeviceData(self, cbObjectData, rgdod, pdwInOut);                                                                                                \
    }                                                                                                                                                        \
    return result;                                                                                                                                           \
  }

// Instantiate the handlers
DINPUT_HOOK_HANDLER(0)
DINPUT_HOOK_HANDLER(1)
DINPUT_HOOK_HANDLER(2)
DINPUT_HOOK_HANDLER(3)
DINPUT_HOOK_HANDLER(4)
DINPUT_HOOK_HANDLER(5)
DINPUT_HOOK_HANDLER(6)
DINPUT_HOOK_HANDLER(7)

// Array of pointers to our hook handlers
static void* g_hookCallbacks[MAX_DINPUT_HOOKS] = {
    (void*)&HookedGetDeviceData_0,
    (void*)&HookedGetDeviceData_1,
    (void*)&HookedGetDeviceData_2,
    (void*)&HookedGetDeviceData_3,
    (void*)&HookedGetDeviceData_4,
    (void*)&HookedGetDeviceData_5,
    (void*)&HookedGetDeviceData_6,
    (void*)&HookedGetDeviceData_7,
};

// Set to keep track of VTable addresses we have already hooked.
static std::unordered_set<void*> g_hookedTargets;
static int g_previousPov = -1;  // Used to track D-Pad state
static LONG g_previousAxisX = 0, g_previousAxisY = 0, g_previousAxisZ = 0;
static LONG g_previousAxisRX = 0, g_previousAxisRY = 0, g_previousAxisRZ = 0;
static bool g_previousButtonStates[128] = {};  // To track button press/release state

static void ProcessDeviceData(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();  // Declared once for all device types
  DWORD write_idx = 0;                                           // Declared once for all device types

  DIDEVICEINSTANCEW instance;
  instance.dwSize = sizeof(DIDEVICEINSTANCEW);
  if (SUCCEEDED(IDirectInputDevice8_GetDeviceInfo(self, &instance))) {
    const auto deviceType = GET_DIDEVICE_TYPE(instance.dwDevType);
    switch (deviceType) {
      case DI8DEVTYPE_MOUSE: {
        for (DWORD i = 0; i < *pdwInOut; ++i) {
          const auto& data = rgdod[i];
          bool block_this_event = false;

          // Using if-else if instead of switch to avoid C4644 warning
          if (data.dwOfs == DIMOFS_X || data.dwOfs == DIMOFS_Y) {
            if (!inputManager.ShouldGameControlMouseAxes()) {
              block_this_event = true;
            }
          } else if (data.dwOfs >= DIMOFS_BUTTON0 && data.dwOfs <= DIMOFS_BUTTON7)  // DIMOFS_BUTTON7 is the highest defined standard button
          {
            if (!inputManager.ShouldGameControlMouseButtons()) {
              block_this_event = true;
            }
          } else if (data.dwOfs == DIMOFS_Z)  // Mouse Wheel
          {
            if (!inputManager.ShouldGameControlMouseWheel()) {
              block_this_event = true;
            }
          }

          if (!block_this_event) {
            if (data.dwOfs == DIMOFS_X) {
              inputManager.PublishMouseMove({(long)data.dwData, 0});
            } else if (data.dwOfs == DIMOFS_Y) {
              inputManager.PublishMouseMove({0, (long)data.dwData});
            } else if (data.dwOfs >= DIMOFS_BUTTON0 && data.dwOfs <= DIMOFS_BUTTON3)  // Only first 4 buttons are standard
            {
              inputManager.PublishMouseButton({(int)(data.dwOfs - DIMOFS_BUTTON0), (data.dwData & 0x80) != 0});
            }

            // Copy event to the game's buffer
            if (write_idx != i) {
              rgdod[write_idx] = data;
            }
            write_idx++;
          }
        }

        *pdwInOut = write_idx;
        break;
      }
      case DI8DEVTYPE_JOYSTICK:
      case DI8DEVTYPE_GAMEPAD:
      case DI8DEVTYPE_DRIVING:
      case DI8DEVTYPE_FLIGHT:
      case DI8DEVTYPE_1STPERSON: {
        // --- Get VID/PID for robust device detection ---
        DWORD vid = 0;
        DWORD pid = 0;
        DIPROPDWORD dipdw;
        dipdw.diph.dwSize = sizeof(DIPROPDWORD);
        dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
        if (SUCCEEDED(IDirectInputDevice8_GetProperty(self, DIPROP_VIDPID, &dipdw.diph))) {
          vid = LOWORD(dipdw.dwData);
          pid = HIWORD(dipdw.dwData);
        }

        // Реєстрація пристрою в InputManager
        inputManager.UpdateDeviceType((UINT_PTR)self, instance.tszProductName, vid, pid);

        // Get the device type based on its device ID
        auto deviceType = inputManager.GetDeviceType((UINT_PTR)self);

        for (DWORD i = 0; i < *pdwInOut; ++i) {
          bool block_this_event = false;

          // --- Buttons ---
          if (rgdod[i].dwOfs >= DIJOFS_BUTTON0 && rgdod[i].dwOfs < DIJOFS_BUTTON0 + 128) {
            int buttonIndex = rgdod[i].dwOfs - DIJOFS_BUTTON0;
            bool isPressed = (rgdod[i].dwData & 0x80) != 0;
            bool wasPressed = g_previousButtonStates[buttonIndex];

            if (isPressed != wasPressed) {
              g_previousButtonStates[buttonIndex] = isPressed;

              if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
                // Process as a standard gamepad button
                SPF::System::GamepadButton buttonId = SPF::System::GamepadButton::Unknown;
                switch (rgdod[i].dwOfs) {
                  case DIJOFS_BUTTON0:
                    buttonId = SPF::System::GamepadButton::FaceDown;
                    break;
                  case DIJOFS_BUTTON1:
                    buttonId = SPF::System::GamepadButton::FaceRight;
                    break;
                  case DIJOFS_BUTTON2:
                    buttonId = SPF::System::GamepadButton::FaceLeft;
                    break;
                  case DIJOFS_BUTTON3:
                    buttonId = SPF::System::GamepadButton::FaceUp;
                    break;
                  case DIJOFS_BUTTON4:
                    buttonId = SPF::System::GamepadButton::LeftShoulder;
                    break;
                  case DIJOFS_BUTTON5:
                    buttonId = SPF::System::GamepadButton::RightShoulder;
                    break;
                  case DIJOFS_BUTTON6:
                    buttonId = SPF::System::GamepadButton::SpecialLeft;
                    break;
                  case DIJOFS_BUTTON7:
                    buttonId = SPF::System::GamepadButton::SpecialRight;
                    break;
                  case DIJOFS_BUTTON8:
                    buttonId = SPF::System::GamepadButton::LeftStick;
                    break;
                  case DIJOFS_BUTTON9:
                    buttonId = SPF::System::GamepadButton::RightStick;
                    break;
                    // Add more cases if needed for standard gamepads
                }

                if (buttonId != SPF::System::GamepadButton::Unknown) {
                  SPF::Input::GamepadEvent event{0, buttonId, isPressed, isPressed ? 1.0f : 0.0f};
                  block_this_event = inputManager.PublishGamepadEvent(event);
                }
              } else {
                // Process as a generic joystick button
                SPF::Input::JoystickEvent event{buttonIndex, isPressed};
                block_this_event = inputManager.PublishJoystickEvent(event);
              }
            }
          }
          // --- POV (D-Pad) ---
          else if (rgdod[i].dwOfs == DIJOFS_POV(0)) {
            int currentPov = (int)rgdod[i].dwData;
            if (currentPov != g_previousPov) {
              // POV handling for standard gamepads (Xbox/PlayStation)
              if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
                // POV handling for standard gamepads (Xbox/PlayStation)
                auto isDirectionActive = [](int pov, int direction) {
                  if (pov == -1) return false;
                  switch (direction) {
                    case 0:
                      return pov == 31500 || pov == 0 || pov == 4500;
                    case 9000:
                      return pov == 4500 || pov == 9000 || pov == 13500;
                    case 18000:
                      return pov == 13500 || pov == 18000 || pov == 22500;
                    case 27000:
                      return pov == 22500 || pov == 27000 || pov == 31500;
                  }
                  return false;
                };
                const SPF::System::GamepadButton directions[] = {
                    SPF::System::GamepadButton::DPadUp, SPF::System::GamepadButton::DPadRight, SPF::System::GamepadButton::DPadDown, SPF::System::GamepadButton::DPadLeft};
                const int povValues[] = {0, 9000, 18000, 27000};  // Up, Right, Down, Left angles

                bool shouldBeActiveToGame[4] = {false, false, false, false};  // Up, Right, Down, Left

                for (int j = 0; j < 4; ++j) {
                  bool isCurrentlyActive = isDirectionActive(currentPov, povValues[j]);
                  bool wasPreviouslyActive = isDirectionActive(g_previousPov, povValues[j]);
                  bool consumedThisFrame = false;

                  if (isCurrentlyActive && !wasPreviouslyActive) {  // Press event
                    consumedThisFrame = inputManager.ProcessAndDecide({0, directions[j], true, 1.0f});
                  } else if (!isCurrentlyActive && wasPreviouslyActive) {  // Release event
                    consumedThisFrame = inputManager.ProcessAndDecide({0, directions[j], false, 0.0f});
                  } else if (isCurrentlyActive && wasPreviouslyActive) {  // Hold event (still active)
                    // For a held direction, re-evaluate consumption for this frame.
                    consumedThisFrame = inputManager.ProcessAndDecide({0, directions[j], true, 1.0f});
                  }

                  // If it's currently active and not consumed, it should be active for the game
                  if (isCurrentlyActive && !consumedThisFrame) {
                    shouldBeActiveToGame[j] = true;
                  }
                }

                // Reconstruct finalPovValue based on shouldBeActiveToGame
                int reconstructedPov = -1;
                bool up = shouldBeActiveToGame[0];
                bool right = shouldBeActiveToGame[1];
                bool down = shouldBeActiveToGame[2];
                bool left = shouldBeActiveToGame[3];

                if (up && right) {
                  reconstructedPov = 4500;
                } else if (down && right) {
                  reconstructedPov = 13500;
                } else if (down && left) {
                  reconstructedPov = 22500;
                } else if (up && left) {
                  reconstructedPov = 31500;
                } else if (up) {
                  reconstructedPov = 0;
                } else if (right) {
                  reconstructedPov = 9000;
                } else if (down) {
                  reconstructedPov = 18000;
                } else if (left) {
                  reconstructedPov = 27000;
                } else {
                  reconstructedPov = -1;  // No active and unconsumed directions
                }

                rgdod[i].dwData = reconstructedPov;  // Assign the reconstructed value
              } else {
                // For generic devices, we ignore POV for now, let it pass to game.
              }
              g_previousPov = currentPov;  // Update g_previousPov with the original input
            }
          }
          // --- Axes ---
          else if (rgdod[i].dwOfs >= DIJOFS_X && rgdod[i].dwOfs <= DIJOFS_RZ) {
            // Axis handling for standard gamepads (Xbox/PlayStation)
            if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
              LONG* pPreviousValue = nullptr;
              SPF::System::GamepadButton axisId = SPF::System::GamepadButton::Unknown;
              if (rgdod[i].dwOfs == DIJOFS_X) {
                  pPreviousValue = &g_previousAxisX;
                  axisId = SPF::System::GamepadButton::LeftStickX;
              } else if (rgdod[i].dwOfs == DIJOFS_Y) {
                  pPreviousValue = &g_previousAxisY;
                  axisId = SPF::System::GamepadButton::LeftStickY;
              } else if (rgdod[i].dwOfs == DIJOFS_RX) {
                  pPreviousValue = &g_previousAxisRX;
                  axisId = SPF::System::GamepadButton::RightStickX;
              } else if (rgdod[i].dwOfs == DIJOFS_RY) {
                  pPreviousValue = &g_previousAxisRY;
                  axisId = SPF::System::GamepadButton::RightStickY;
              } else if (rgdod[i].dwOfs == DIJOFS_Z) {
                  pPreviousValue = &g_previousAxisZ;
                  axisId = SPF::System::GamepadButton::RightTrigger;
              } else if (rgdod[i].dwOfs == DIJOFS_RZ) {
                  pPreviousValue = &g_previousAxisRZ;
                  axisId = SPF::System::GamepadButton::LeftTrigger;
              }

              if (pPreviousValue && axisId != SPF::System::GamepadButton::Unknown) {
                LONG currentValue = (LONG)rgdod[i].dwData;
                float normalizedValue = static_cast<float>(currentValue) / 32767.0f;
                constexpr float deadzone = 0.2f;
                if (std::abs(normalizedValue) < deadzone) {
                  normalizedValue = 0.0f;
                }

                float oldNormalized = static_cast<float>(*pPreviousValue) / 32767.0f;
                if (std::abs(normalizedValue - oldNormalized) > 0.01f) {
                  inputManager.ProcessAndDecide({0, axisId, false, normalizedValue});
                  *pPreviousValue = currentValue;
                }
              }
            } else {
              // For generic devices, we ignore axes for now, let it pass to game.
              block_this_event = false;
            }
          }

          if (!block_this_event) {
            if (write_idx != i) {
              rgdod[write_idx] = rgdod[i];
            }
            write_idx++;
          }
        }
        *pdwInOut = write_idx;
        break;
      }
      default: {
        break;
      }
    }
  }
}


// This struct is passed to the enumeration callback to provide necessary context.
struct EnumContext {
  IDirectInput8W* pDI;
  SPF::Logging::Logger* logger;
};

// This callback is executed for each DirectInput device found.
// It creates a temporary device to find its GetDeviceData VTable address and hooks it if it's new.
BOOL CALLBACK EnumAndHookDeviceCallback(LPCDIDEVICEINSTANCEW lpddi, LPVOID pvRef) {
  auto* context = static_cast<EnumContext*>(pvRef);
  auto logger = context->logger;

  // Convert names for logging using the new helper
  std::string instanceName = WstringToUtf8(lpddi->tszInstanceName);
  DWORD devType = GET_DIDEVICE_TYPE(lpddi->dwDevType);

  // Stop if hook pool is full
  if (g_hookCount >= MAX_DINPUT_HOOKS) {
    if (g_hookCount == MAX_DINPUT_HOOKS) {
      logger->Warn("Reached max DInput hooks [{}]. Will not hook more device types. The following devices will NOT be hooked:", MAX_DINPUT_HOOKS);
      g_hookCount++;
    }
    logger->Warn(" -> Device not hooked: '{}' (Type={})", instanceName, devType);
    return DIENUM_CONTINUE;
  }

  // Create temp device to get VTable
  IDirectInputDevice8W* pDevice = nullptr;
  if (FAILED(IDirectInput8_CreateDevice(context->pDI, lpddi->guidInstance, &pDevice, nullptr))) {
    logger->Warn("Could not create temporary DInput device for '{}'. Skipping.", instanceName);
    return DIENUM_CONTINUE;
  }

  void* pTarget = reinterpret_cast<void*>(pDevice->lpVtbl->GetDeviceData);
  IDirectInputDevice8_Release(pDevice);  // Release immediately after getting the pointer

  // Check if this VTable address has already been hooked
  if (g_hookedTargets.find(pTarget) == g_hookedTargets.end()) {
    logger->Info("Found new VTable for device '{}' (Type={}) at [{:p}]. Hooking...", instanceName, devType, pTarget);

    if (auto err = MH_CreateHook(pTarget, g_hookCallbacks[g_hookCount], reinterpret_cast<LPVOID*>(&g_originalFunctions[g_hookCount])); err != MH_OK) {
      logger->Error(" -> Failed to create hook for VTable at [{:p}]. Error: {}", pTarget, (int)err);
    } else {
      g_hookedTargets.insert(pTarget);
      g_hookCount++;
    }
  } else {
    logger->Info("Device '{}' (Type={}) is covered by existing hook for VTable at [{:p}].", instanceName, devType, pTarget);
  }

  return DIENUM_CONTINUE;
}
}  // namespace

bool DInput8Hook::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");

  // If hooks are already created, just enable them.
  if (g_hookCount > 0) {
    logger->Info("DInput8 hooks already created, re-enabling them...");
    if (auto err = MH_EnableHook(MH_ALL_HOOKS); err != MH_OK && err != MH_ERROR_ENABLED) {
      logger->Error("Cannot re-enable DInput8 hooks. Error: [{}]", (int)err);
      return false;
    }
    logger->Info("DInput8 hooks enabled.");
    return true;
  }

  logger->Info("Installing DInput8 hooks for the first time...");

  HMODULE libDInput = ::GetModuleHandleA("dinput8.dll");
  if (libDInput == nullptr) {
    logger->Critical("Cannot find dinput8.dll");
    return false;
  }

  auto dInput8Create = (decltype(&DirectInput8Create))::GetProcAddress(libDInput, "DirectInput8Create");
  if (dInput8Create == nullptr) {
    logger->Critical("Cannot find DirectInput8Create inside dinput8.dll");
    return false;
  }

  IDirectInput8W* pDI = nullptr;
  if (FAILED(dInput8Create(::GetModuleHandle(nullptr), 0x0800, IID_IDirectInput8W_spf, reinterpret_cast<void**>(&pDI), nullptr))) {
    logger->Critical("Cannot create DInput instance");
    return false;
  }

  EnumContext context = {pDI, logger.get()};

  // Enumerate and hook all attached devices for diagnostics.
  logger->Info("Enumerating ALL DirectInput devices for diagnostics...");
  IDirectInput8_EnumDevices(pDI, 0, EnumAndHookDeviceCallback, &context, DIEDFL_ATTACHEDONLY);

  IDirectInput8_Release(pDI);

  if (g_hookCount == 0) {
    logger->Warn("No DirectInput devices were found or hooked. DInput input will not be captured.");
    return true;  // Not a fatal error, the game might not use DInput.
  }

  if (auto err = MH_EnableHook(MH_ALL_HOOKS); err != MH_OK) {
    logger->Error("Cannot enable one or more DInput8 hooks. Error: [{}]", (int)err);
    return false;
  }

  logger->Info("DInput8 hooks installed successfully for {} unique device type(s).", g_hookCount);
  return true;
}

void DInput8Hook::Uninstall() {
  if (g_hookCount > 0) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
    if (auto status = MH_DisableHook(MH_ALL_HOOKS); status == MH_OK) {
      logger->Info("DInput8 hooks disabled successfully.");
    } else {
      logger->Warn("Failed to disable one or more DInput8 hooks, status: {}", MH_StatusToString(status));
    }
  }
}

void DInput8Hook::Remove() {
  if (g_hookCount > 0) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
    for (void* target : g_hookedTargets) {
      MH_DisableHook(target);
      MH_RemoveHook(target);
    }
    logger->Info("DInput8 hooks removed.");
    g_hookedTargets.clear();
    for (int i = 0; i < MAX_DINPUT_HOOKS; ++i) {
      g_originalFunctions[i] = nullptr;
    }
    g_hookCount = 0;
  }
}
}  // namespace Hooks
SPF_NS_END