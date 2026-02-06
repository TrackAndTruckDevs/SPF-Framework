#include "SPF/Hooks/User32Hook.hpp"

#include <MinHook.h>
#include "SPF/Input/InputManager.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

// --- State Management ---
// Flag to track if hooks have been created, to support reload.
static bool g_hooksCreated = false;

// State tracking for keyboard. We only care about the high-order bit (pressed or not).
static BYTE g_previousKeyboardState[256] = {};

// Define pointer types for the original functions
using SetCursorPos_t = BOOL(WINAPI*)(int, int);
using GetKeyboardState_t = BOOL(WINAPI*)(PBYTE);
using GetAsyncKeyState_t = SHORT(WINAPI*)(int);
using GetKeyState_t = SHORT(WINAPI*)(int);

// Pointers to the original functions
static SetCursorPos_t oSetCursorPos = nullptr;
static GetKeyboardState_t oGetKeyboardState = nullptr;
static GetAsyncKeyState_t oGetAsyncKeyState = nullptr;
static GetKeyState_t oGetKeyState = nullptr;

// Our hook functions
BOOL WINAPI hkSetCursorPos(int X, int Y) {
  if (!SPF::Input::InputManager::GetInstance().ShouldGameControlMouseAxes()) {
    return TRUE;  // Lie that we set the position
  }
  return oSetCursorPos(X, Y);
}

BOOL WINAPI hkGetKeyboardState(PBYTE lpKeyState) {
  BOOL result = oGetKeyboardState(lpKeyState);
  if (result && lpKeyState != nullptr) {
    auto& inputManager = SPF::Input::InputManager::GetInstance();
    auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
    // Create a copy of the real key state before potential modification for blocking.
    BYTE realKeyState[256];
    memcpy(realKeyState, lpKeyState, 256);

    for (int i = 0; i < 256; ++i) {
      // The high-order bit of each byte indicates if the key is down.
      bool isPressedWinAPI = (lpKeyState[i] & 0x80) != 0;
      bool isPressedPhysical = (GetAsyncKeyState(i) & 0x8000) != 0;
      
      // The framework should follow the physical state to maintain chords even if we fake-release keys to the game.
      bool isPressedForFramework = isPressedWinAPI || isPressedPhysical;
      
      bool wasPressed = (g_previousKeyboardState[i] & 0x80) != 0;

      if (isPressedForFramework != wasPressed) {
        inputManager.PublishKeyboardEvent({keyMapper.FromWinAPI(i), isPressedForFramework});
      }

      // After processing the event, check if the key should be blocked.
      if (inputManager.IsKeyBlocked(keyMapper.FromWinAPI(i))) {
        // If so, clear the bit to make the game think the key is up.
        lpKeyState[i] &= ~0x80;
      }
      
      // Update our state tracker with the PHYSICAL state (or mixed) so we don't lose the "Down" event
      if (isPressedForFramework)
        g_previousKeyboardState[i] |= 0x80;
      else
        g_previousKeyboardState[i] &= ~0x80;
    }
    // We update g_previousKeyboardState inside the loop now.
  }
  return result;
}

SHORT WINAPI hkGetAsyncKeyState(int vKey) {
  SHORT result = oGetAsyncKeyState(vKey);

  // For GetAsyncKeyState, the high-order bit indicates the down state.
  bool isPressed = (result & 0x8000) != 0;
  bool wasPressed = (g_previousKeyboardState[vKey] & 0x80) != 0;

  if (isPressed != wasPressed) {
    auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
    SPF::Input::InputManager::GetInstance().PublishKeyboardEvent({keyMapper.FromWinAPI(vKey), isPressed});
  }

  // Update our state tracker
  if (isPressed)
    g_previousKeyboardState[vKey] |= 0x80;
  else
    g_previousKeyboardState[vKey] &= ~0x80;

  if (SPF::Input::InputManager::GetInstance().IsKeyBlocked(SPF::System::VirtualKeyMapping::GetInstance().FromWinAPI(vKey))) {
    // To block, we must clear the "is pressed" bit AND the "was pressed since last call" bit.
    return 0;
  }

  // Check for mouse button blocking
  auto mouseButton = SPF::System::MouseButtonMapping::GetInstance().FromWinAPI(vKey);
  if (mouseButton != SPF::System::MouseButton::Unknown) {
    if (SPF::Input::InputManager::GetInstance().IsMouseButtonBlocked(mouseButton)) {
      return 0;
    }
  }

  return result;
}

SHORT WINAPI hkGetKeyState(int nVirtKey) {
  SHORT result = oGetKeyState(nVirtKey);

  // For GetKeyState, the high-order bit of the return value indicates the down state.
  bool isPressedWinAPI = (result & 0x8000) != 0;
  bool isPressedPhysical = (GetAsyncKeyState(nVirtKey) & 0x8000) != 0;
  
  bool isPressedForFramework = isPressedWinAPI || isPressedPhysical;
  bool wasPressed = (g_previousKeyboardState[nVirtKey] & 0x80) != 0;

  if (isPressedForFramework != wasPressed) {
    auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
    SPF::Input::InputManager::GetInstance().PublishKeyboardEvent({keyMapper.FromWinAPI(nVirtKey), isPressedForFramework});
  }

  // Update our state tracker
  if (isPressedForFramework)
    g_previousKeyboardState[nVirtKey] |= 0x80;
  else
    g_previousKeyboardState[nVirtKey] &= ~0x80;

  if (SPF::Input::InputManager::GetInstance().IsKeyBlocked(SPF::System::VirtualKeyMapping::GetInstance().FromWinAPI(nVirtKey))) {
    return 0;  // Return 0 to indicate the key is up.
  }

  // Check for mouse button blocking
  auto mouseButton = SPF::System::MouseButtonMapping::GetInstance().FromWinAPI(nVirtKey);
  if (mouseButton != SPF::System::MouseButton::Unknown) {
    if (SPF::Input::InputManager::GetInstance().IsMouseButtonBlocked(mouseButton)) {
      return 0;
    }
  }

  return result;
}

SPF_NS_BEGIN

namespace Hooks {
    
    void User32Hook::SendVirtualKeyRelease(uint32_t hardwareCode) {
      uint8_t type = (hardwareCode >> 24) & 0xFF;
      uint32_t rawCode = hardwareCode & 0x00FFFFFF;
      int vkCode = -1;

      if (type == 0x01) { // Keyboard
          auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
          System::Keyboard targetKey = static_cast<System::Keyboard>(rawCode);
          
          // Since we don't have a reverse map, we iterate. This is acceptable for this rare event.
          for (int i = 0; i < 256; ++i) {
              if (keyMapper.FromWinAPI(i) == targetKey) {
                  vkCode = i;
                  break;
              }
          }
      }

      if (vkCode == -1) return; // Not supported or not found

      // Send WM_KEYUP to the game window
      HWND hwnd = GetActiveWindow();
      if (!hwnd) {
          hwnd = GetForegroundWindow();
      }

      if (hwnd) {
          UINT msg = WM_KEYUP;
          UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
          LPARAM lParam = 1; // Repeat count
          lParam |= (scanCode << 16);
          lParam |= (1 << 30); // Previous state was down
          lParam |= (1 << 31); // Transition state is up
          
          PostMessageA(hwnd, msg, vkCode, lParam);
      }
    }
bool User32Hook::Install() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("User32Hook");

  if (g_hooksCreated) {
    logger->Info("User32 hooks already created, enabling them...");
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
      logger->Error("Failed to re-enable User32 hooks.");
      return false;
    }
    logger->Info("User32 hooks enabled.");
    return true;
  }

  logger->Info("Installing User32 hooks for the first time...");

  // Initialize the baseline keyboard state
  if (!GetKeyboardState(g_previousKeyboardState)) {
    logger->Warn("Could not get initial keyboard state.");
  }

  // --- Create all hooks ---
  MH_CreateHook(reinterpret_cast<LPVOID>(&SetCursorPos), reinterpret_cast<LPVOID>(&hkSetCursorPos), reinterpret_cast<void**>(&oSetCursorPos));
  MH_CreateHook(reinterpret_cast<LPVOID>(&GetKeyboardState), reinterpret_cast<LPVOID>(&hkGetKeyboardState), reinterpret_cast<void**>(&oGetKeyboardState));
  MH_CreateHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState), reinterpret_cast<LPVOID>(&hkGetAsyncKeyState), reinterpret_cast<void**>(&oGetAsyncKeyState));
  MH_CreateHook(reinterpret_cast<LPVOID>(&GetKeyState), reinterpret_cast<LPVOID>(&hkGetKeyState), reinterpret_cast<void**>(&oGetKeyState));

  g_hooksCreated = true;

  if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
    logger->Error("Failed to enable User32 hooks.");
    return false;
  }

  logger->Info("User32 hooks installed successfully.");
  return true;
}

void User32Hook::Uninstall() {
  if (g_hooksCreated) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("User32Hook");
    // Disable hooks individually instead of using MH_ALL_HOOKS
    MH_DisableHook(reinterpret_cast<LPVOID>(&SetCursorPos));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyboardState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyState));
    logger->Info("User32 hooks disabled successfully.");
  }
}

void User32Hook::Remove() {
  if (g_hooksCreated) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("User32Hook");

    // Disable and remove hooks individually
    MH_DisableHook(reinterpret_cast<LPVOID>(&SetCursorPos));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyboardState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&SetCursorPos));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetKeyboardState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetKeyState));

    logger->Info("User32 hooks removed.");

    g_hooksCreated = false;
    oSetCursorPos = nullptr;
    oGetKeyboardState = nullptr;
    oGetAsyncKeyState = nullptr;
    oGetKeyState = nullptr;
  }
}
}  // namespace Hooks

SPF_NS_END
