#include "SPF/Hooks/DInput8Hook.hpp"

#include "SPF/Input/InputEvents.hpp"

#include <vector>
#include <unordered_set>
#include <mutex>

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

static std::mutex g_dinputStateMutex;

struct PerDeviceState {
    bool buttonStates[128] = {};
    bool virtualUpSent[128] = {};
    int previousPov[4] = {-1, -1, -1, -1};
    LONG axes[8] = {0}; // X, Y, Z, RX, RY, RZ, SL0, SL1
    LONG axisMin[8] = {0};
    LONG axisMax[8] = {0};
};
static std::map<IDirectInputDevice8W*, PerDeviceState> g_perDeviceStates;

static DWORD g_lastSeenSequence = 0; // Global sync for sequence numbers across all DInput devices

// --- Internal Helper Functions ---

static void InjectVirtualEvent(DIDEVICEOBJECTDATA* rgdod, DWORD& writeIdx, DWORD capacity, DWORD offset, DWORD data, DWORD& sequence) {
    if (writeIdx < capacity) {
        rgdod[writeIdx].dwOfs = offset;
        rgdod[writeIdx].dwData = data;
        rgdod[writeIdx].dwTimeStamp = GetTickCount();
        rgdod[writeIdx].dwSequence = ++sequence;
        writeIdx++;
        
        if (sequence > g_lastSeenSequence) {
            g_lastSeenSequence = sequence;
        }
    }
}

static void MaskMouseState(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    
    // Mask Axes (X, Y) if requested
    if (!inputManager.ShouldGameControlMouseAxes()) {
        LONG* pAxes = (LONG*)lpvData;
        pAxes[0] = 0; // X
        pAxes[1] = 0; // Y
    }

    // Offset to buttons in DIMOUSESTATE/DIMOUSESTATE2 is after 3 LONG axes (12 bytes)
    BYTE* pButtons = (BYTE*)lpvData + 12;
    int maxButtons = (cbData >= sizeof(DIMOUSESTATE2)) ? 8 : 4;

    for (int i = 0; i < maxButtons; ++i) {
        if (inputManager.IsMouseButtonBlocked(static_cast<SPF::System::MouseButton>(i)) || inputManager.IsMouseCaptured()) {
            pButtons[i] = 0;
        }
    }
}

static void MaskJoystickState(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    int maxButtons = (cbData >= sizeof(DIJOYSTATE2)) ? 128 : (cbData >= sizeof(DIJOYSTATE) ? 32 : 0);
    if (maxButtons <= 0) return;

    BYTE* pButtons = (BYTE*)lpvData + DIJOFS_BUTTON0;
    auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
    auto type = inputManager.GetDeviceType((UINT_PTR)self);
    bool isGamepad = (type == SPF::System::DeviceType::Xbox || type == SPF::System::DeviceType::PlayStation);
    uint8_t typeNum = isGamepad ? 0x02 : 0x04;

    for (int i = 0; i < maxButtons; ++i) {
        if (DIJOFS_BUTTON0 + i >= (int)cbData) break;

        bool blocked = inputManager.IsJoystickButtonBlocked(i);
        if (!blocked && isGamepad) {
            auto gamepadBtn = mapping.FromDInput(DIJOFS_BUTTON0 + i);
            if (gamepadBtn != SPF::System::GamepadButton::Unknown) {
                blocked = inputManager.IsGamepadButtonBlocked(gamepadBtn);
            }
        }

        if (blocked) pButtons[i] = 0;
    }

    // Mask Axes
    LONG* pAxes = (LONG*)lpvData; // DIJOYSTATE starts with axes
    auto deviceType = inputManager.GetDeviceType((UINT_PTR)self);

    for (int axisIdx = 0; axisIdx < 8; ++axisIdx) {
        int mappedIdx = axisIdx;
        bool consumed = false;

        if (isGamepad && deviceType == SPF::System::DeviceType::Xbox) {
            if (axisIdx == 0) mappedIdx = 0;
            else if (axisIdx == 1) mappedIdx = 1;
            else if (axisIdx == 2) mappedIdx = 4; // Z -> LT
            else if (axisIdx == 3) mappedIdx = 2; // Rx -> RS X
            else if (axisIdx == 4) mappedIdx = 3; // Ry -> RS Y
            else if (axisIdx == 5) mappedIdx = 5; // Rz -> RT
            else if (axisIdx == 6) mappedIdx = 6; // Slider 0
            else if (axisIdx == 7) mappedIdx = 7; // Slider 1
            else mappedIdx = -1;
            
            if (mappedIdx != -1) consumed = inputManager.IsAxisConsumed(0x02, mappedIdx);
        }
        else if (isGamepad) {
            if (axisIdx == 2) mappedIdx = 2;
            else if (axisIdx == 3) mappedIdx = 3;
            else if (axisIdx == 4) mappedIdx = 4;
            else if (axisIdx == 5) mappedIdx = 5;
            else if (axisIdx == 6) mappedIdx = 6;
            else if (axisIdx == 7) mappedIdx = 7;
            consumed = inputManager.IsAxisConsumed(typeNum, mappedIdx);
        }
        else {
            consumed = inputManager.IsAxisConsumed(typeNum, axisIdx);
        }

        if (consumed) {
            pAxes[axisIdx] = 0;
        }
    }
}

static void HandleMouseData(IDirectInputDevice8W* self, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD capacity, DWORD& sequence) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    DWORD originalCount = *pdwInOut;
    DWORD writeIdx = 0;

    for (DWORD i = 0; i < originalCount; ++i) {
        const auto& data = rgdod[i];
        if (data.dwSequence > g_lastSeenSequence) g_lastSeenSequence = data.dwSequence;

        bool block = false;
        if (data.dwOfs == DIMOFS_X || data.dwOfs == DIMOFS_Y) {
            if (!inputManager.ShouldGameControlMouseAxes()) block = true;
        } else if (data.dwOfs >= DIMOFS_BUTTON0 && data.dwOfs <= DIMOFS_BUTTON7) {
            int btnIdx = data.dwOfs - DIMOFS_BUTTON0;
            uint32_t code = 0x03000000 | static_cast<uint32_t>(btnIdx);
            
            if (inputManager.IsPendingVirtualRelease(code)) {
                block = false; // Internal framework release, let it through to game
            } else {
                block = inputManager.PublishMouseButton({btnIdx, (data.dwData & 0x80) != 0});
            }
        } else if (data.dwOfs == DIMOFS_Z) {
            if (inputManager.PublishMouseWheel({(float)((int)data.dwData) / (float)WHEEL_DELTA})) block = true;
        }

        if (!block) {
            if (data.dwOfs == DIMOFS_X) inputManager.PublishMouseMove({(long)data.dwData, 0});
            else if (data.dwOfs == DIMOFS_Y) inputManager.PublishMouseMove({0, (long)data.dwData});

            if (writeIdx != i) rgdod[writeIdx] = data;
            writeIdx++;
        }
    }

    // Virtual Injections
    sequence = (writeIdx > 0) ? rgdod[writeIdx - 1].dwSequence : sequence;
    for (int b = 0; b <= 7; ++b) {
        if (inputManager.ConsumeMouseReleaseRequest(static_cast<SPF::System::MouseButton>(b))) {
            InjectVirtualEvent(rgdod, writeIdx, capacity, DIMOFS_BUTTON0 + b, 0, sequence);
        }
    }
    *pdwInOut = writeIdx;
}

static void ProcessPovData(IDirectInputDevice8W* self, PerDeviceState& state, DIDEVICEOBJECTDATA& data, SPF::System::DeviceType type) {
    int povIdx = -1;
    if (data.dwOfs == DIJOFS_POV(0)) povIdx = 0;
    else if (data.dwOfs == DIJOFS_POV(1)) povIdx = 1;
    else if (data.dwOfs == DIJOFS_POV(2)) povIdx = 2;
    else if (data.dwOfs == DIJOFS_POV(3)) povIdx = 3;

    if (povIdx == -1) return;

    int currentPov = (int)data.dwData;
    int prevPov = state.previousPov[povIdx];
    if (currentPov == prevPov) return;

    auto& inputManager = SPF::Input::InputManager::GetInstance();
    auto isDir = [](int pov, int dir) {
        if (pov == -1 || pov == 0xFFFFFFFF) return false;
        switch (dir) {
            case 0: return pov == 31500 || pov == 0 || pov == 4500;
            case 9000: return pov == 4500 || pov == 9000 || pov == 13500;
            case 18000: return pov == 13500 || pov == 18000 || pov == 22500;
            case 27000: return pov == 22500 || pov == 27000 || pov == 31500;
            default: return false;
        }
    };

    // Mapping based on POV index
    SPF::System::GamepadButton dirs[4];
    if (povIdx == 0 && (type == SPF::System::DeviceType::Xbox || type == SPF::System::DeviceType::PlayStation)) {
        dirs[0] = SPF::System::GamepadButton::DPadUp;
        dirs[1] = SPF::System::GamepadButton::DPadRight;
        dirs[2] = SPF::System::GamepadButton::DPadDown;
        dirs[3] = SPF::System::GamepadButton::DPadLeft;
    } else {
        // Generic POV to extended button mapping
        static const SPF::System::GamepadButton genericMap[4][4] = {
            {SPF::System::GamepadButton::DPadUp, SPF::System::GamepadButton::DPadRight, SPF::System::GamepadButton::DPadDown, SPF::System::GamepadButton::DPadLeft}, // POV0
            {SPF::System::GamepadButton::POV1Up, SPF::System::GamepadButton::POV1Right, SPF::System::GamepadButton::POV1Down, SPF::System::GamepadButton::POV1Left}, // POV1
            {SPF::System::GamepadButton::POV2Up, SPF::System::GamepadButton::POV2Right, SPF::System::GamepadButton::POV2Down, SPF::System::GamepadButton::POV2Left}, // POV2
            {SPF::System::GamepadButton::POV3Up, SPF::System::GamepadButton::POV3Right, SPF::System::GamepadButton::POV3Down, SPF::System::GamepadButton::POV3Left}  // POV3
        };
        for (int k = 0; k < 4; ++k) dirs[k] = genericMap[povIdx][k];
    }

    // Process each direction
    const int vals[] = {0, 9000, 18000, 27000};
    bool activeToGame[4] = {false, false, false, false};

    for (int j = 0; j < 4; ++j) {
        bool curActive = isDir(currentPov, vals[j]);
        bool prevActive = isDir(prevPov, vals[j]);
        bool blocked = false;

        if (curActive != prevActive) blocked = inputManager.PublishGamepadEvent(SPF::Input::GamepadEvent{0, dirs[j], curActive, curActive ? 1.0f : 0.0f}, 2);
        else if (curActive) blocked = inputManager.ProcessAndDecide(SPF::Input::GamepadEvent{0, dirs[j], true, 1.0f}, 2);

        if (curActive && !blocked) activeToGame[j] = true;
    }

    // Reconstruct POV value for the game
    int reconstructed = -1;
    if (activeToGame[0] && activeToGame[1]) reconstructed = 4500;
    else if (activeToGame[2] && activeToGame[1]) reconstructed = 13500;
    else if (activeToGame[2] && activeToGame[3]) reconstructed = 22500;
    else if (activeToGame[0] && activeToGame[3]) reconstructed = 31500;
    else if (activeToGame[0]) reconstructed = 0;
    else if (activeToGame[1]) reconstructed = 9000;
    else if (activeToGame[2]) reconstructed = 18000;
    else if (activeToGame[3]) reconstructed = 27000;
    data.dwData = (DWORD)reconstructed;

    state.previousPov[povIdx] = currentPov;
}

static void HandleJoystickData(IDirectInputDevice8W* self, const DIDEVICEINSTANCEW& instance, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD capacity, DWORD& sequence) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    auto& mapping = SPF::System::GamepadButtonMapping::GetInstance();
    
    DWORD vid = 0, pid = 0;
    DIPROPDWORD dipdw;
    dipdw.diph.dwSize = sizeof(DIPROPDWORD); dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0; dipdw.diph.dwHow = DIPH_DEVICE;
    if (SUCCEEDED(IDirectInputDevice8_GetProperty(self, DIPROP_VIDPID, &dipdw.diph))) {
        vid = LOWORD(dipdw.dwData); pid = HIWORD(dipdw.dwData);
    }
    inputManager.UpdateDeviceType((UINT_PTR)self, instance.tszProductName, vid, pid);

    auto type = inputManager.GetDeviceType((UINT_PTR)self);
    auto& state = g_perDeviceStates[self];
    DWORD writeIdx = 0;

    for (DWORD i = 0; i < *pdwInOut; ++i) {
        if (rgdod[i].dwSequence > sequence) sequence = rgdod[i].dwSequence;
        if (sequence > g_lastSeenSequence) g_lastSeenSequence = sequence;

        bool block = false;
        int axisIdx = -1;

        // 1. Buttons
        if (rgdod[i].dwOfs >= DIJOFS_BUTTON0 && rgdod[i].dwOfs < DIJOFS_BUTTON0 + 128) {
            int btnIdx = rgdod[i].dwOfs - DIJOFS_BUTTON0;
            bool pressed = (rgdod[i].dwData & 0x80) != 0;
            SPF::System::GamepadButton gamepadBtn = (type == SPF::System::DeviceType::Xbox || type == SPF::System::DeviceType::PlayStation) ? mapping.FromDInput(rgdod[i].dwOfs) : SPF::System::GamepadButton::Unknown;

            if (pressed) {
                if (gamepadBtn != SPF::System::GamepadButton::Unknown) block = inputManager.IsGamepadButtonBlocked(gamepadBtn);
                else block = inputManager.IsJoystickButtonBlocked(btnIdx);
            }

            if (pressed != state.buttonStates[btnIdx]) {
                state.buttonStates[btnIdx] = pressed;
                state.virtualUpSent[btnIdx] = false;
                bool managerBlock = false;
                if (gamepadBtn != SPF::System::GamepadButton::Unknown) managerBlock = inputManager.PublishGamepadEvent(SPF::Input::GamepadEvent{0, gamepadBtn, pressed, pressed ? 1.0f : 0.0f}, 2);
                else managerBlock = inputManager.PublishJoystickEvent(SPF::Input::JoystickEvent{btnIdx, pressed}, 2);
                if (managerBlock && pressed) block = true;
            }
        }
        // 2. POV
        else if (rgdod[i].dwOfs >= DIJOFS_POV(0) && rgdod[i].dwOfs <= DIJOFS_POV(3)) {
            ProcessPovData(self, state, rgdod[i], type);
        }
        // 3. Axes
        else if (rgdod[i].dwOfs >= DIJOFS_X && rgdod[i].dwOfs <= DIJOFS_SLIDER(1)) {
            if (rgdod[i].dwOfs == DIJOFS_X) axisIdx = 0;
            else if (rgdod[i].dwOfs == DIJOFS_Y) axisIdx = 1;
            else if (rgdod[i].dwOfs == DIJOFS_Z) axisIdx = 2;
            else if (rgdod[i].dwOfs == DIJOFS_RX) axisIdx = 3;
            else if (rgdod[i].dwOfs == DIJOFS_RY) axisIdx = 4;
            else if (rgdod[i].dwOfs == DIJOFS_RZ) axisIdx = 5;
            else if (rgdod[i].dwOfs == DIJOFS_SLIDER(0)) axisIdx = 6;
            else if (rgdod[i].dwOfs == DIJOFS_SLIDER(1)) axisIdx = 7;

            if (axisIdx != -1) {
                std::lock_guard<std::mutex> lock(g_dinputStateMutex);
                
                // Fetch and cache range if not already present
                if (state.axisMax[axisIdx] == 0 && state.axisMin[axisIdx] == 0) {
                    DIPROPRANGE diprg;
                    diprg.diph.dwSize = sizeof(DIPROPRANGE);
                    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                    diprg.diph.dwHow = DIPH_BYOFFSET;
                    diprg.diph.dwObj = rgdod[i].dwOfs;
                    if (SUCCEEDED(IDirectInputDevice8_GetProperty(self, DIPROP_RANGE, &diprg.diph))) {
                        state.axisMin[axisIdx] = diprg.lMin;
                        state.axisMax[axisIdx] = diprg.lMax;
                    } else {
                        state.axisMin[axisIdx] = -32768;
                        state.axisMax[axisIdx] = 32767;
                    }
                }

                LONG rawData = (LONG)rgdod[i].dwData;
                LONG rangeMin = state.axisMin[axisIdx];
                LONG rangeMax = state.axisMax[axisIdx];
                LONG rangeLen = rangeMax - rangeMin;

                float normalized01 = 0.0f;
                if (rangeLen != 0) {
                    normalized01 = (float)(rawData - rangeMin) / (float)rangeLen;
                }

                float rawVal = 0.0f;
                bool isCentered = (rangeMin < -100);

                // Architectural fix: Triggers on Xbox controllers in DInput.
                // Axis 2 (Z) is combined LT/RT and MUST be centered to split them.
                // Axis 5 (Rz) is sometimes an independent RT and should be 0..1.
                if (type == SPF::System::DeviceType::Xbox && axisIdx == 5) {
                    isCentered = false;
                }

                if (isCentered) {
                    rawVal = normalized01 * 2.0f - 1.0f;
                } else {
                    rawVal = normalized01;
                }

                bool isGamepad = (type == SPF::System::DeviceType::Xbox || type == SPF::System::DeviceType::PlayStation);
                uint8_t finalType = isGamepad ? 0x02 : 0x04;
                
                int mappedIdx = axisIdx;
                float finalNormValue = rawVal;

                if (isGamepad && type == SPF::System::DeviceType::Xbox) {
                    if (axisIdx == 0) mappedIdx = 0;      // LS X
                    else if (axisIdx == 1) mappedIdx = 1; // LS Y
                    else if (axisIdx == 2) { 
                        // Split combined Z axis: rawVal is -1..1 (0 at rest)
                        float lt = (rawVal > 0.01f) ? rawVal : 0.0f;
                        float rt = (rawVal < -0.01f) ? -rawVal : 0.0f;
                        inputManager.PublishAxisMove(0x02, 4, lt, 2);
                        inputManager.PublishAxisMove(0x02, 5, rt, 2);
                        block = inputManager.IsAxisConsumed(0x02, 4) || inputManager.IsAxisConsumed(0x02, 5);
                        goto next_object;
                    }
                    else if (axisIdx == 3) mappedIdx = 2; // Rx -> RS X
                    else if (axisIdx == 4) mappedIdx = 3; // Ry -> RS Y
                    else if (axisIdx == 5) mappedIdx = 5; // Independent Rz -> RT
                    else if (axisIdx == 6) mappedIdx = 6;
                    else if (axisIdx == 7) mappedIdx = 7;
                    else mappedIdx = -1;
                } 
                else if (isGamepad) {
                    if (axisIdx == 2) mappedIdx = 2;      
                    else if (axisIdx == 3) mappedIdx = 3; 
                    else if (axisIdx == 4) mappedIdx = 4; 
                    else if (axisIdx == 5) mappedIdx = 5; 
                    else if (axisIdx == 6) mappedIdx = 6;
                    else if (axisIdx == 7) mappedIdx = 7;
                }

                if (mappedIdx != -1 && inputManager.PublishAxisMove(finalType, mappedIdx, finalNormValue, 2)) {
                    block = true;
                }
            }

            next_object:
            // Sync previous values for capture logic delta detection
            if (axisIdx >= 0 && axisIdx < 8) state.axes[axisIdx] = (LONG)rgdod[i].dwData;
        }

        if (!block) {
            if (writeIdx != i) rgdod[writeIdx] = rgdod[i];
            writeIdx++;
        }
    }

    // Virtual Injections
    sequence = (writeIdx > 0) ? rgdod[writeIdx - 1].dwSequence : sequence;
    for (int b = 0; b < 128; ++b) {
        if (inputManager.HasPendingJoystickRelease(b) && !state.virtualUpSent[b]) {
            state.virtualUpSent[b] = true;
            InjectVirtualEvent(rgdod, writeIdx, capacity, DIJOFS_BUTTON0 + b, 0, sequence);
        }
    }
    if (type == SPF::System::DeviceType::Xbox || type == SPF::System::DeviceType::PlayStation) {
        for (int b = static_cast<int>(SPF::System::GamepadButton::FaceDown); b <= static_cast<int>(SPF::System::GamepadButton::RightStick); ++b) {
            auto btn = static_cast<SPF::System::GamepadButton>(b);
            if (inputManager.ConsumeGamepadReleaseRequest(btn)) {
                for (DWORD off = DIJOFS_BUTTON0; off < DIJOFS_BUTTON0 + 32; ++off) {
                    if (mapping.FromDInput(off) == btn) {
                        InjectVirtualEvent(rgdod, writeIdx, capacity, off, 0, sequence); break;
                    }
                }
            }
        }
    }
    *pdwInOut = writeIdx;
}

static void ProcessDeviceState(IDirectInputDevice8W* self, DWORD cbData, LPVOID lpvData) {
  DIDEVICEINSTANCEW instance;
  instance.dwSize = sizeof(DIDEVICEINSTANCEW);
  if (SUCCEEDED(IDirectInputDevice8_GetDeviceInfo(self, &instance))) {
    const auto type = GET_DIDEVICE_TYPE(instance.dwDevType);
    if (type == DI8DEVTYPE_MOUSE) MaskMouseState(self, cbData, lpvData);
    else if (type >= DI8DEVTYPE_JOYSTICK) MaskJoystickState(self, cbData, lpvData);
  }
}

static void ProcessDeviceData(IDirectInputDevice8W* self, DWORD cbObjectData, DIDEVICEOBJECTDATA* rgdod, DWORD* pdwInOut, DWORD capacity) {
  DIDEVICEINSTANCEW instance;
  instance.dwSize = sizeof(DIDEVICEINSTANCEW);
  if (SUCCEEDED(IDirectInputDevice8_GetDeviceInfo(self, &instance))) {
    DWORD& sequence = g_deviceSequenceNumbers[self];
    if (g_lastSeenSequence > sequence) sequence = g_lastSeenSequence;

    const auto type = GET_DIDEVICE_TYPE(instance.dwDevType);
    if (type == DI8DEVTYPE_MOUSE) HandleMouseData(self, rgdod, pdwInOut, capacity, sequence);
    else if (type >= DI8DEVTYPE_JOYSTICK) HandleJoystickData(self, instance, rgdod, pdwInOut, capacity, sequence);
    
    sequence = g_deviceSequenceNumbers[self];
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