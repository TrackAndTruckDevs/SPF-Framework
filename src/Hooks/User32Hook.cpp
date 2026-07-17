#include "SPF/Hooks/User32Hook.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Input/InputEvents.hpp"
#include "SPF/Input/InputManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/System/MouseButtonMapping.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"

#include <cstdint>
#include <cstring>
#include <MinHook.h>
#include <minwindef.h>
#include <windef.h>
#include <windows.h>
#include <winnt.h>


// --- State Management ---
// Flag to track if hooks have been created, to support reload.
static bool g_hooksCreated = false;

// State tracking for keyboard. We only care about the high-order bit (pressed or not).
static BYTE g_previousKeyboardState[256] = {};

// Define pointer types for the original functions
using SetCursorPos_t = BOOL(WINAPI*)(int, int);
using GetCursorPos_t = BOOL(WINAPI*)(LPPOINT);
using GetKeyboardState_t = BOOL(WINAPI*)(PBYTE);
using GetAsyncKeyState_t = SHORT(WINAPI*)(int);
using GetKeyState_t = SHORT(WINAPI*)(int);

// Pointers to the original functions
static SetCursorPos_t oSetCursorPos = nullptr;
static GetCursorPos_t oGetCursorPos = nullptr;
static GetKeyboardState_t oGetKeyboardState = nullptr;
static GetAsyncKeyState_t oGetAsyncKeyState = nullptr;
static GetKeyState_t oGetKeyState = nullptr;

// --- Internal Helper Functions ---

static bool ProcessSingleKey(int vkCode, bool isDownWinAPI, bool isDownPhysical) {
  auto& inputManager = SPF::Input::InputManager::GetInstance();
  auto& keyMapper = SPF::System::VirtualKeyMapping::GetInstance();
  SPF::System::Keyboard key = keyMapper.FromWinAPI(vkCode);

  // The framework follows physical state to maintain chords even if keys are blocked for the game.
  bool isDownForFramework = isDownWinAPI || isDownPhysical;
  bool wasDown = (g_previousKeyboardState[vkCode] & 0x80) != 0;

  bool publishedBlock = false;
  if (key != SPF::System::Keyboard::Unknown && isDownForFramework != wasDown) {
    publishedBlock = inputManager.PublishKeyboardEvent(SPF::Input::KeyboardEvent{key, isDownForFramework});
  }

  // Update state tracker
  if (isDownForFramework)
    g_previousKeyboardState[vkCode] |= 0x80;
  else
    g_previousKeyboardState[vkCode] &= ~0x80;

  // determine if the key or mouse button (represented by VK) should be blocked for the game.
  // A key is blocked if either the framework says so (via publishedBlock) OR if it's already in a blocked state.
  bool blocked = inputManager.IsKeyBlocked(key) || publishedBlock;

  // CRUCIAL: If any UI consumer (like ImGui) is capturing keyboard, we block ALL keys from the game.
  // This handles polling functions like GetKeyboardState even for keys that haven't changed state.
  if (!blocked && inputManager.IsKeyboardCaptured() && key != SPF::System::Keyboard::Escape) {
    blocked = true;
  }

  if (!blocked) {
    auto mouseBtn = SPF::System::MouseButtonMapping::GetInstance().FromWinAPI(vkCode);
    if (mouseBtn != SPF::System::MouseButton::Unknown) {
      blocked = inputManager.IsMouseButtonBlocked(mouseBtn);
      // Also check for UI mouse capture
      if (!blocked && inputManager.IsMouseCaptured()) {
        blocked = true;
      }
    }
  }

  return blocked;
}

// Our hook functions
BOOL WINAPI hkSetCursorPos(int X, int Y) {
  if (!SPF::Input::InputManager::GetInstance().ShouldGameControlMouseAxes()) {
    return TRUE;  // Lie that we set the position
  }
  return oSetCursorPos(X, Y);
}

BOOL WINAPI hkGetCursorPos(LPPOINT lpPoint) {
  BOOL result = oGetCursorPos(lpPoint);
  if (result && lpPoint != nullptr && !SPF::Input::InputManager::GetInstance().ShouldGameControlMouseAxes()) {
    // If blocked, we return a fixed position (e.g., center of the screen or last known position)
    // to prevent the game from detecting movement.
    // For many games, returning the same position every time effectively blocks mouse look.
    static POINT lastPoint = {0, 0};
    if (lastPoint.x == 0 && lastPoint.y == 0) lastPoint = *lpPoint;
    *lpPoint = lastPoint;
  } else if (result && lpPoint != nullptr) {
    // Update last known position when not blocked
  }
  return result;
}

BOOL WINAPI hkGetKeyboardState(PBYTE lpKeyState) {
  BOOL result = oGetKeyboardState(lpKeyState);
  if (result && lpKeyState != nullptr) {
    for (int i = 0; i < 256; ++i) {
      bool winDown = (lpKeyState[i] & 0x80) != 0;
      bool physDown = (GetAsyncKeyState(i) & 0x8000) != 0;

      if (ProcessSingleKey(i, winDown, physDown)) {
        lpKeyState[i] &= ~0x80;  // Block for game
      }
    }
  }
  return result;
}

SHORT WINAPI hkGetAsyncKeyState(int vKey) {
  SHORT result = oGetAsyncKeyState(vKey);
  if (vKey < 0 || vKey >= 256) return result;

  bool winDown = (result & 0x8000) != 0;
  // GetAsyncKeyState is the physical state itself
  if (ProcessSingleKey(vKey, winDown, winDown)) {
    return 0;  // Block for game
  }

  return result;
}

SHORT WINAPI hkGetKeyState(int nVirtKey) {
  SHORT result = oGetKeyState(nVirtKey);
  if (nVirtKey < 0 || nVirtKey >= 256) return result;

  bool winDown = (result & 0x8000) != 0;
  bool physDown = (GetAsyncKeyState(nVirtKey) & 0x8000) != 0;

  if (ProcessSingleKey(nVirtKey, winDown, physDown)) {
    return 0;  // Block for game
  }

  return result;
}

SPF_NS_BEGIN

namespace Hooks {

void User32Hook::SendVirtualKeyRelease(uint32_t hardwareCode) {
  uint8_t type = (hardwareCode >> 24) & 0xFF;
  uint32_t rawCode = hardwareCode & 0x00FFFFFF;
  int vkCode = -1;

  if (type == 0x01) {  // Keyboard
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

  if (vkCode == -1) return;  // Not supported or not found

  // Send WM_KEYUP to the game window
  HWND hwnd = GetActiveWindow();
  if (!hwnd) {
    hwnd = GetForegroundWindow();
  }

  if (hwnd) {
    UINT msg = WM_KEYUP;
    UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
    LPARAM lParam = 1;  // Repeat count
    lParam |= (scanCode << 16);
    lParam |= (1 << 30);  // Previous state was down
    lParam |= (1 << 31);  // Transition state is up

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

  // Reset keyboard state tracker to prevent stuck keys after reload
  memset(g_previousKeyboardState, 0, sizeof(g_previousKeyboardState));

  // Initialize the baseline keyboard state
  if (!GetKeyboardState(g_previousKeyboardState)) {
    logger->Warn("Could not get initial keyboard state.");
  }

  // --- Create all hooks ---
  MH_CreateHook(reinterpret_cast<LPVOID>(&SetCursorPos), reinterpret_cast<LPVOID>(&hkSetCursorPos), reinterpret_cast<void**>(&oSetCursorPos));
  MH_CreateHook(reinterpret_cast<LPVOID>(&GetCursorPos), reinterpret_cast<LPVOID>(&hkGetCursorPos), reinterpret_cast<void**>(&oGetCursorPos));
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
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetCursorPos));
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
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetCursorPos));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyboardState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState));
    MH_DisableHook(reinterpret_cast<LPVOID>(&GetKeyState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&SetCursorPos));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetCursorPos));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetKeyboardState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetAsyncKeyState));
    MH_RemoveHook(reinterpret_cast<LPVOID>(&GetKeyState));

    logger->Info("User32 hooks removed.");

    g_hooksCreated = false;
    oSetCursorPos = nullptr;
    oGetCursorPos = nullptr;
    oGetKeyboardState = nullptr;
    oGetAsyncKeyState = nullptr;
    oGetKeyState = nullptr;
  }
}
}  // namespace Hooks

SPF_NS_END
