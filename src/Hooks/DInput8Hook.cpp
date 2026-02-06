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
typedef HRESULT(WINAPI* GetDeviceState_t)(IDirectInputDevice8W*, DWORD, LPVOID);

// Maximum number of unique VTables we can hook. Should be plenty.
constexpr int MAX_DINPUT_HOOKS = 8;

// Holds the trampoline for each hook
static GetDeviceData_t g_originalGetDeviceData[MAX_DINPUT_HOOKS] = {nullptr};
static GetDeviceState_t g_originalGetDeviceState[MAX_DINPUT_HOOKS] = {nullptr};
static int g_hookCount = 0;

// Forward declarations
static void ProcessDeviceData(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD capacity);
static void ProcessDeviceState(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData);

// Define the pool of hook handlers.
// Using a macro to avoid repetitive code.
#define DINPUT_HOOK_HANDLER(index)                                                                                                                           \
  HRESULT __stdcall HookedGetDeviceData_##index(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD dwFlags) { \
    DWORD capacity = (pdwInOut != nullptr) ? *pdwInOut : 0;                                                                                                 \
    HRESULT result = g_originalGetDeviceData[index](self, cbObjectData, rgdod, pdwInOut, dwFlags);                                                               \
    if (SUCCEEDED(result) && self != nullptr && rgdod != nullptr && pdwInOut != nullptr && *pdwInOut > 0) {                                                  \
      ProcessDeviceData(self, cbObjectData, rgdod, pdwInOut, capacity);                                                                                      \
    } else if (SUCCEEDED(result) && self != nullptr && rgdod != nullptr && pdwInOut != nullptr && capacity > 0) {                                            \
      /* Even if no physical events, we call it to allow virtual injection */                                                                                \
      ProcessDeviceData(self, cbObjectData, rgdod, pdwInOut, capacity);                                                                                      \
    }                                                                                                                                                        \
    return result;                                                                                                                                           \
  }                                                                                                                                                          \
  HRESULT __stdcall HookedGetDeviceState_##index(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData) {                                                  \
    HRESULT result = g_originalGetDeviceState[index](self, cbData, lpvData);                                                                                 \
    if (SUCCEEDED(result) && self != nullptr && lpvData != nullptr) {                                                                                        \
      ProcessDeviceState(self, cbData, lpvData);                                                                                                             \
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
static void* g_hookCallbacksData[MAX_DINPUT_HOOKS] = {
    (void*)&HookedGetDeviceData_0,
    (void*)&HookedGetDeviceData_1,
    (void*)&HookedGetDeviceData_2,
    (void*)&HookedGetDeviceData_3,
    (void*)&HookedGetDeviceData_4,
    (void*)&HookedGetDeviceData_5,
    (void*)&HookedGetDeviceData_6,
    (void*)&HookedGetDeviceData_7,
};

static void* g_hookCallbacksState[MAX_DINPUT_HOOKS] = {
    (void*)&HookedGetDeviceState_0,
    (void*)&HookedGetDeviceState_1,
    (void*)&HookedGetDeviceState_2,
    (void*)&HookedGetDeviceState_3,
    (void*)&HookedGetDeviceState_4,
    (void*)&HookedGetDeviceState_5,
    (void*)&HookedGetDeviceState_6,
    (void*)&HookedGetDeviceState_7,
};

// Set to keep track of VTable addresses we have already hooked.
static std::unordered_set<void*> g_hookedTargets;
static std::map<IDirectInputDevice8W*, DWORD> g_deviceSequenceNumbers; // Track sequence numbers per device

struct PerDeviceState {
    bool buttonStates[128] = {};
    bool virtualUpSent[128] = {};
    int previousPov = -1;
    LONG axes[6] = {0}; // X, Y, Z, RX, RY, RZ
};
static std::map<IDirectInputDevice8W*, PerDeviceState> g_perDeviceStates;

static DWORD g_lastSeenSequence = 0; // Global sync for sequence numbers across all DInput devices

static void ProcessDeviceState(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();

  DIDEVICEINSTANCEW instance;
  instance.dwSize = sizeof(DIDEVICEINSTANCEW);
  if (SUCCEEDED(IDirectInputDevice8_GetDeviceInfo(self, &instance))) {
    const auto deviceType = GET_DIDEVICE_TYPE(instance.dwDevType);
    if (deviceType == DI8DEVTYPE_MOUSE) {
      // DirectInput mouse state is either DIMOUSESTATE (4 buttons) or DIMOUSESTATE2 (8 buttons).
      // Both start with axes, then buttons. Buttons are stored as bytes where 0x80 means pressed.
      
      // Offset to buttons in DIMOUSESTATE/DIMOUSESTATE2 is after 3 LONG axes (12 bytes)
      BYTE* pButtons = (BYTE*)lpvData + 12;
      int maxButtons = (cbData >= sizeof(DIMOUSESTATE2)) ? 8 : 4;

      for (int i = 0; i < maxButtons; ++i) {
        auto button = static_cast<SPF::System::MouseButton>(i);
        if (inputManager.IsMouseButtonBlocked(button)) {
          // Force the button to be seen as released by the game
          pButtons[i] = 0;
        }
      }
    } else if (deviceType == DI8DEVTYPE_JOYSTICK || deviceType == DI8DEVTYPE_GAMEPAD || 
               deviceType == DI8DEVTYPE_1STPERSON || deviceType == DI8DEVTYPE_DRIVING || deviceType == DI8DEVTYPE_FLIGHT) {
        
        int maxButtons = 0;
        if (cbData >= sizeof(DIJOYSTATE2)) {
            maxButtons = 128;
        } else if (cbData >= sizeof(DIJOYSTATE)) {
            maxButtons = 32;
        }

        if (maxButtons > 0) {
            BYTE* pButtons = (BYTE*)lpvData + DIJOFS_BUTTON0;
            auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
            auto internalDeviceType = inputManager.GetDeviceType((UINT_PTR)self);

            for (int i = 0; i < maxButtons; ++i) {
                // Ensure we don't write past the buffer provided by the game
                if (DIJOFS_BUTTON0 + i < (int)cbData) {
                    bool shouldBlock = inputManager.IsJoystickButtonBlocked(i);

                    // If it's a recognized gamepad (Xbox/PS), also check gamepad-specific block state
                    if (!shouldBlock && (internalDeviceType == SPF::System::DeviceType::Xbox || internalDeviceType == SPF::System::DeviceType::PlayStation)) {
                        auto gamepadBtn = mapping.FromDInput(DIJOFS_BUTTON0 + i);
                        if (gamepadBtn != SPF::System::GamepadButton::Unknown) {
                            shouldBlock = inputManager.IsGamepadButtonBlocked(gamepadBtn);
                        }
                    }

                    if (shouldBlock) {
                        // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
                        // logger->Trace("DInput8: Masking button {} for device={:p} (State)", i, (void*)self);
                        pButtons[i] = 0;
                    }
                }
            }
        }
    }
  }
}

static void ProcessDeviceData(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD capacity) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();  // Declared once for all device types
  // auto logger_entry = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
  // logger_entry->Trace("DInput8: ProcessDeviceData called for self={:p}, count={}, capacity={}", (void*)self, *pdwInOut, capacity);
  DWORD write_idx = 0;                                           // Declared once for all device types

  // Retrieve or initialize the last sequence number for this device
  DWORD current_sequence = g_deviceSequenceNumbers[self];

  DIDEVICEINSTANCEW instance;
  instance.dwSize = sizeof(DIDEVICEINSTANCEW);
  if (SUCCEEDED(IDirectInputDevice8_GetDeviceInfo(self, &instance))) {
    const auto deviceType = GET_DIDEVICE_TYPE(instance.dwDevType);
    switch (deviceType) {
      case DI8DEVTYPE_MOUSE: {
        DWORD original_count = *pdwInOut;
        for (DWORD i = 0; i < original_count; ++i) {
          const auto& data = rgdod[i];

          // Update global sequence tracker from real mouse events
          if (data.dwSequence > g_lastSeenSequence) {
            g_lastSeenSequence = data.dwSequence;
          }

          bool block_this_event = false;

          // Using if-else if instead of switch to avoid C4644 warning
          if (data.dwOfs == DIMOFS_X || data.dwOfs == DIMOFS_Y) {
            if (!inputManager.ShouldGameControlMouseAxes()) {
              block_this_event = true;
            }
          } else if (data.dwOfs >= DIMOFS_BUTTON0 && data.dwOfs <= DIMOFS_BUTTON7) {
            uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(data.dwOfs - DIMOFS_BUTTON0);
            
            // Check if this is a virtual event sent by the framework itself
            if (inputManager.IsPendingVirtualRelease(hardwareCode)) {
                // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
                // logger->Trace("DInput8: Detected virtual release for framework (ignoring): hardwareCode={:#08x}", hardwareCode);
                // Pass to game, ignore for framework
                block_this_event = false; 
            } else if (inputManager.PublishMouseButton({(int)(data.dwOfs - DIMOFS_BUTTON0), (data.dwData & 0x80) != 0})) {
                // Framework requested to block this physical event
                block_this_event = true;
            }
          } else if (data.dwOfs == DIMOFS_Z) {
            if (inputManager.PublishMouseWheel({(float)((int)data.dwData) / (float)WHEEL_DELTA})) {
                block_this_event = true;
            }
          }

          if (!block_this_event) {
            if (data.dwOfs == DIMOFS_X) {
              inputManager.PublishMouseMove({(long)data.dwData, 0});
            } else if (data.dwOfs == DIMOFS_Y) {
              inputManager.PublishMouseMove({0, (long)data.dwData});
            }

            // Copy event to the game's buffer
            if (write_idx != i) {
              rgdod[write_idx] = data;
            }
            write_idx++;
          }
        }

        // --- VIRTUAL INJECTION LOGIC ---
        // Synchronize sequence numbers
        DWORD last_sequence = (write_idx > 0) ? rgdod[write_idx - 1].dwSequence : 0;

        for (int b = 0; b <= 7; ++b) {
            auto button = static_cast<SPF::System::MouseButton>(b);
            if (inputManager.ConsumeMouseReleaseRequest(button)) {
                // Safely add if there's space in the buffer provided by the game
                if (write_idx < capacity) { 
                    // auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
                    // logger->Trace("DInput8: Injecting virtual UP event for button={} at index={} (capacity={})", b, write_idx, capacity);
                    rgdod[write_idx].dwOfs = DIMOFS_BUTTON0 + b;
                    rgdod[write_idx].dwData = 0; // Up
                    rgdod[write_idx].dwTimeStamp = GetTickCount();
                    rgdod[write_idx].dwSequence = ++last_sequence;
                    write_idx++;
                }
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
        // Sync with global sequence before processing
        if (g_lastSeenSequence > current_sequence) {
          current_sequence = g_lastSeenSequence;
        }

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

        inputManager.UpdateDeviceType((UINT_PTR)self, instance.tszProductName, vid, pid);

        // Get the device type based on its device ID
        auto deviceType = inputManager.GetDeviceType((UINT_PTR)self);
        auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
        auto& deviceState = g_perDeviceStates[self];

        for (DWORD i = 0; i < *pdwInOut; ++i) {
          // Update sequence tracker from real events
          if (rgdod[i].dwSequence > current_sequence) {
            current_sequence = rgdod[i].dwSequence;
          }
          if (current_sequence > g_lastSeenSequence) {
            g_lastSeenSequence = current_sequence;
          }

          bool block_this_event = false;

          // --- Buttons ---
          if (rgdod[i].dwOfs >= DIJOFS_BUTTON0 && rgdod[i].dwOfs < DIJOFS_BUTTON0 + 128) {
            int buttonIndex = rgdod[i].dwOfs - DIJOFS_BUTTON0;
            bool isPressed = (rgdod[i].dwData & 0x80) != 0;
            bool wasPressed = deviceState.buttonStates[buttonIndex];

            // Mapping to framework types - ONLY for identified gamepads
            SPF::System::GamepadButton gamepadBtn = SPF::System::GamepadButton::Unknown;
            if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
                gamepadBtn = mapping.FromDInput(rgdod[i].dwOfs);
            }

            // Standard joystick/gamepad behavior: only block PRESS events.
            if (isPressed) {
                if (gamepadBtn != SPF::System::GamepadButton::Unknown) {
                    if (inputManager.IsGamepadButtonBlocked(gamepadBtn)) {
                        block_this_event = true;
                    }
                } else if (inputManager.IsJoystickButtonBlocked(buttonIndex)) {
                    block_this_event = true;
                }
                
                // if (block_this_event) {
                //     auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
                //     logger->Trace("DInput8: Blocking physical PRESS for device={:p}, button={}", (void*)self, buttonIndex);
                // }
            }

            if (isPressed != wasPressed) {
                deviceState.buttonStates[buttonIndex] = isPressed;
                deviceState.virtualUpSent[buttonIndex] = false; // Reset virtual tracking on any physical change
                bool manager_block = false;

                if (gamepadBtn != SPF::System::GamepadButton::Unknown) {
                    SPF::Input::GamepadEvent event{0, gamepadBtn, isPressed, isPressed ? 1.0f : 0.0f};
                    manager_block = inputManager.PublishGamepadEvent(event);
                } else {
                    SPF::Input::JoystickEvent event{buttonIndex, isPressed};
                    manager_block = inputManager.PublishJoystickEvent(event);
                }

                if (manager_block && isPressed) {
                    block_this_event = true;
                }
            }
          }
          // --- POV (D-Pad) ---
          else if (rgdod[i].dwOfs == DIJOFS_POV(0)) {
            int currentPov = (int)rgdod[i].dwData;
            if (currentPov != deviceState.previousPov) {
              // POV handling for standard gamepads (Xbox/PlayStation)
              if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
                // ... (POV logic remains same but uses deviceState.previousPov)
                auto isDirectionActive = [](int pov, int direction) {
                  if (pov == -1) return false;
                  switch (direction) {
                    case 0: return pov == 31500 || pov == 0 || pov == 4500;
                    case 9000: return pov == 4500 || pov == 9000 || pov == 13500;
                    case 18000: return pov == 13500 || pov == 18000 || pov == 22500;
                    case 27000: return pov == 22500 || pov == 27000 || pov == 31500;
                  }
                  return false;
                };
                const SPF::System::GamepadButton directions[] = {
                    SPF::System::GamepadButton::DPadUp, SPF::System::GamepadButton::DPadRight, SPF::System::GamepadButton::DPadDown, SPF::System::GamepadButton::DPadLeft};
                const int povValues[] = {0, 9000, 18000, 27000};

                bool shouldBeActiveToGame[4] = {false, false, false, false};

                for (int j = 0; j < 4; ++j) {
                  bool isCurrentlyActive = isDirectionActive(currentPov, povValues[j]);
                  bool wasPreviouslyActive = isDirectionActive(deviceState.previousPov, povValues[j]);
                  bool consumedThisFrame = false;

                  if (isCurrentlyActive && !wasPreviouslyActive) {
                    consumedThisFrame = inputManager.PublishGamepadEvent({0, directions[j], true, 1.0f});
                  } else if (!isCurrentlyActive && wasPreviouslyActive) {
                    consumedThisFrame = inputManager.PublishGamepadEvent({0, directions[j], false, 0.0f});
                  } else if (isCurrentlyActive && wasPreviouslyActive) {
                    consumedThisFrame = inputManager.ProcessAndDecide({0, directions[j], true, 1.0f});
                  }

                  if (isCurrentlyActive && !consumedThisFrame) {
                    shouldBeActiveToGame[j] = true;
                  }
                }

                int reconstructedPov = -1;
                if (shouldBeActiveToGame[0] && shouldBeActiveToGame[1]) reconstructedPov = 4500;
                else if (shouldBeActiveToGame[2] && shouldBeActiveToGame[1]) reconstructedPov = 13500;
                else if (shouldBeActiveToGame[2] && shouldBeActiveToGame[3]) reconstructedPov = 22500;
                else if (shouldBeActiveToGame[0] && shouldBeActiveToGame[3]) reconstructedPov = 31500;
                else if (shouldBeActiveToGame[0]) reconstructedPov = 0;
                else if (shouldBeActiveToGame[1]) reconstructedPov = 9000;
                else if (shouldBeActiveToGame[2]) reconstructedPov = 18000;
                else if (shouldBeActiveToGame[3]) reconstructedPov = 27000;

                rgdod[i].dwData = reconstructedPov;
              }
              deviceState.previousPov = currentPov;
            }
          }
          // --- Axes ---
          else if (rgdod[i].dwOfs >= DIJOFS_X && rgdod[i].dwOfs <= DIJOFS_RZ) {
            if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
              LONG* pPreviousValue = nullptr;
              SPF::System::GamepadButton axisId = SPF::System::GamepadButton::Unknown;
              if (rgdod[i].dwOfs == DIJOFS_X) { pPreviousValue = &deviceState.axes[0]; axisId = SPF::System::GamepadButton::LeftStickX; }
              else if (rgdod[i].dwOfs == DIJOFS_Y) { pPreviousValue = &deviceState.axes[1]; axisId = SPF::System::GamepadButton::LeftStickY; }
              else if (rgdod[i].dwOfs == DIJOFS_Z) { pPreviousValue = &deviceState.axes[2]; axisId = SPF::System::GamepadButton::RightTrigger; }
              else if (rgdod[i].dwOfs == DIJOFS_RX) { pPreviousValue = &deviceState.axes[3]; axisId = SPF::System::GamepadButton::RightStickX; }
              else if (rgdod[i].dwOfs == DIJOFS_RY) { pPreviousValue = &deviceState.axes[4]; axisId = SPF::System::GamepadButton::RightStickY; }
              else if (rgdod[i].dwOfs == DIJOFS_RZ) { pPreviousValue = &deviceState.axes[5]; axisId = SPF::System::GamepadButton::LeftTrigger; }

              if (pPreviousValue && axisId != SPF::System::GamepadButton::Unknown) {
                LONG currentValue = (LONG)rgdod[i].dwData;
                float norm = static_cast<float>(currentValue) / 32767.0f;
                float oldNorm = static_cast<float>(*pPreviousValue) / 32767.0f;
                if (std::abs(norm - oldNorm) > 0.01f) {
                  inputManager.ProcessAndDecide({0, axisId, false, norm});
                  *pPreviousValue = currentValue;
                }
              }
            }
          }

          if (!block_this_event) {
            if (write_idx != i) {
              rgdod[write_idx] = rgdod[i];
            }
            write_idx++;
          }
        }

        // --- VIRTUAL INJECTION LOGIC ---
        // Final sync before injection (especially if *pdwInOut was 0)
        if (g_lastSeenSequence > current_sequence) {
          current_sequence = g_lastSeenSequence;
        }

        for (int b = 0; b < 128; ++b) {
             if (inputManager.HasPendingJoystickRelease(b) && !deviceState.virtualUpSent[b]) {
                 if (write_idx < capacity) {
                    //  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("DInput8Hook");
                    //  logger->Trace("DInput8: Injecting virtual UP for Joystick button={} for device={:p}", b, (void*)self);

                     deviceState.virtualUpSent[b] = true;
                     rgdod[write_idx].dwOfs = DIJOFS_BUTTON0 + b;
                     rgdod[write_idx].dwData = 0; // Up
                     rgdod[write_idx].dwTimeStamp = GetTickCount();
                     rgdod[write_idx].dwSequence = ++current_sequence;
                     write_idx++;

                     // Update global sequence to stay ahead
                     if (current_sequence > g_lastSeenSequence) {
                         g_lastSeenSequence = current_sequence;
                     }
                 }
             }
        }

        // --- VIRTUAL GAMEPAD INJECTION ---
        // Only for identified gamepads (Xbox/PlayStation)
        if (deviceType == SPF::System::DeviceType::Xbox || deviceType == SPF::System::DeviceType::PlayStation) {
            for (int b = static_cast<int>(SPF::System::GamepadButton::FaceDown); b <= static_cast<int>(SPF::System::GamepadButton::RightStick); ++b) {
                auto btn = static_cast<SPF::System::GamepadButton>(b);
                if (inputManager.ConsumeGamepadReleaseRequest(btn)) {
                    // Find the DInput offset for this gamepad button
                    DWORD offset = 0;
                    for (DWORD off = DIJOFS_BUTTON0; off < DIJOFS_BUTTON0 + 32; ++off) {
                        if (mapping.FromDInput(off) == btn) {
                            offset = off;
                            break;
                        }
                    }

                    if (offset != 0 && write_idx < capacity) {
                        rgdod[write_idx].dwOfs = offset;
                        rgdod[write_idx].dwData = 0; // Up
                        rgdod[write_idx].dwTimeStamp = GetTickCount();
                        rgdod[write_idx].dwSequence = ++current_sequence;
                        write_idx++;

                        if (current_sequence > g_lastSeenSequence) {
                            g_lastSeenSequence = current_sequence;
                        }
                    }
                }
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
  
  // Save the updated sequence back to the map
  g_deviceSequenceNumbers[self] = current_sequence;
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

  void* pTargetData = reinterpret_cast<void*>(pDevice->lpVtbl->GetDeviceData);
  void* pTargetState = reinterpret_cast<void*>(pDevice->lpVtbl->GetDeviceState);
  IDirectInputDevice8_Release(pDevice);  // Release immediately after getting the pointer

  // Check if this VTable address has already been hooked
  if (g_hookedTargets.find(pTargetData) == g_hookedTargets.end()) {
    logger->Info("Found new VTable for device '{}' (Type={}) at [{:p}]. Hooking...", instanceName, devType, pTargetData);

    // Hook GetDeviceData
    if (auto err = MH_CreateHook(pTargetData, g_hookCallbacksData[g_hookCount], reinterpret_cast<LPVOID*>(&g_originalGetDeviceData[g_hookCount])); err != MH_OK) {
      logger->Error(" -> Failed to create hook for GetDeviceData at [{:p}]. Error: {}", pTargetData, (int)err);
    } 
    // Hook GetDeviceState
    else if (auto err = MH_CreateHook(pTargetState, g_hookCallbacksState[g_hookCount], reinterpret_cast<LPVOID*>(&g_originalGetDeviceState[g_hookCount])); err != MH_OK) {
      logger->Error(" -> Failed to create hook for GetDeviceState at [{:p}]. Error: {}", pTargetState, (int)err);
      MH_RemoveHook(pTargetData); // Cleanup first hook if second fails
    }
    else {
      g_hookedTargets.insert(pTargetData);
      g_hookedTargets.insert(pTargetState);
      g_hookCount++;
    }
  } else {
    logger->Info("Device '{}' (Type={}) is covered by existing hook for VTable at [{:p}].", instanceName, devType, pTargetData);
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
      g_originalGetDeviceData[i] = nullptr;
      g_originalGetDeviceState[i] = nullptr;
    }
    g_hookCount = 0;
  }
}
}  // namespace Hooks
SPF_NS_END