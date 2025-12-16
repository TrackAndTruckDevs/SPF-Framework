#pragma once

// --- Framework ---
#include "SPF/Namespace.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"  // For DeviceType

#include <set>    // For frame-based deduplication
#include <mutex>  // For thread-safe deduplication

#include <chrono>  // For time-based deduplication
#include <map>     // For time-based deduplication

// --- 3rd-party ---
#include "SPF/System/MouseButtonMapping.hpp"

// --- 3rd-party ---
#include <nlohmann/json.hpp>

// --- Standard ---
#include <vector>
#include <string>
#include <memory>
#include <array>
#include <optional>

// --- System ---
#include <Xinput.h>  // For XINPUT_STATE and XUSER_MAX_COUNT

SPF_NS_BEGIN

// Forward declaration to avoid circular dependencies
namespace System {
enum class DeviceType;
}  // namespace System
namespace Input {
class IInputConsumer;
}  // namespace Input

namespace Events {
struct GamepadEvent;
}  // namespace Events

namespace Input {
struct ButtonState {
  bool isDown = false;
  bool wasDown = false;  // State in the previous frame
  bool blockInput = false;
  bool longPressTriggered = false;
  std::chrono::steady_clock::time_point pressTimestamp;
};

class InputManager {
 public:
  InputManager(Events::EventManager& eventManager);
  ~InputManager();

  InputManager(const InputManager&) = delete;
  InputManager& operator=(const InputManager&) = delete;

  static InputManager& GetInstance();

  void Initialize();
  void Shutdown();

  /**
   * @brief Processes state changes from hooks and determines if input should be blocked.
   * This is called synchronously from the hooks.
   * @param event A struct containing the button and its current pressed state.
   * @return True if the input should be blocked from the game, false otherwise.
   */
  bool ProcessAndDecide(const GamepadEvent& event);
  bool ProcessAndDecide(const KeyboardEvent& event);
  bool ProcessAndDecide(const MouseButtonEvent& event);
  bool ProcessAndDecide(const JoystickEvent& event);

  /**
   * @brief Processes actions based on the current state of all buttons.
   * This is called once per frame from the main loop (e.g., UIManager).
   */
  void ProcessButtonActions();
  void ProcessKeyboardActions();
  void ProcessMouseActions();
  void ProcessJoystickActions();

  // --- Key Capture ---
  void StartInputCapture(const std::string& actionFullName, const nlohmann::json& originalBinding);
  void CancelInputCapture();

  // --- Consumer Management ---
  void RegisterConsumer(IInputConsumer* consumer);
  void UnregisterConsumer(IInputConsumer* consumer);

  // --- Event Publishing (from hooks) ---
  void PublishMouseMove(const MouseMoveEvent& event);
  bool PublishMouseButton(const MouseButtonEvent& event);
  bool PublishMouseWheel(const MouseWheelEvent& event);
  bool PublishKeyboardEvent(const KeyboardEvent& event);
  bool PublishGamepadEvent(const GamepadEvent& event);
  bool PublishJoystickEvent(const JoystickEvent& event);

  // --- Cursor Control ---
  void SetMouseAxesControl(bool gameHasControl);
  void SetMouseButtonsControl(bool gameHasControl);
  void SetMouseWheelControl(bool gameHasControl);

  bool ShouldGameControlMouseAxes() const;
  bool ShouldGameControlMouseButtons() const;
  bool ShouldGameControlMouseWheel() const;

  bool IsKeyBlocked(System::Keyboard key) const;

  // --- Device Detection ---
    void UpdateDeviceType(UINT_PTR deviceId, const std::wstring& productName, DWORD vid, DWORD pid);
    void RegisterXInputDevice(DWORD userIndex, BYTE subType);
  
    void SetXInputDeviceActive(bool isActive);
    System::DeviceType GetDetectedGamepadType() const;
    System::DeviceType GetXInputDeviceType(DWORD userIndex) const;
    System::DeviceType GetDeviceType(UINT_PTR deviceId) const;

 private:
  void OnXInputStateGet(DWORD deviceID, XINPUT_STATE* pState);

  inline static InputManager* s_instance = nullptr;

  Events::EventManager& m_eventManager;
  std::vector<IInputConsumer*> m_consumers;
  bool m_gameControlsMouseAxes = true;
  bool m_gameControlsMouseButtons = true;
  bool m_gameControlsMouseWheel = true;

  // The central state machine for all inputs
  std::map<System::GamepadButton, ButtonState> m_buttonStates;
  std::map<System::Keyboard, ButtonState> m_keyboardStates;
  std::map<System::MouseButton, ButtonState> m_mouseButtonStates;
  std::map<int, ButtonState> m_joystickButtonStates;  // Using int for generic button index

  // State for tracking keys/buttons that are in a "hold" behavior state
  std::map<System::GamepadButton, PressType> m_heldGamepadButtons;
  std::map<System::Keyboard, PressType> m_heldKeyboardKeys;
  std::map<System::MouseButton, PressType> m_heldMouseButtons;
  std::map<int, PressType> m_heldJoystickButtons;  // Using int for generic button index

  // State for XInputHook (legacy, might be removed later)
  std::unique_ptr<Utils::Sink<void(DWORD, XINPUT_STATE*)>> m_xinputSink;
  std::array<XINPUT_STATE, XUSER_MAX_COUNT> m_previousGamepadStates{};

  // --- Key Capture State ---
  enum class InputCaptureState { Idle, Capturing };

  InputCaptureState m_captureState = InputCaptureState::Idle;
  bool m_inPostCaptureCooldown = false;
  std::string m_capturingActionFullName;
  nlohmann::json m_capturingOriginalBinding;

  // Frame-specific blacklist to prevent input processing from multiple hooks
  std::optional<System::GamepadButton> m_capturedButtonThisFrame;
  std::optional<System::Keyboard> m_capturedKeyThisFrame;
  std::optional<System::MouseButton> m_capturedMouseButtonThisFrame;
  std::optional<int> m_capturedJoystickButtonThisFrame;  // Using int for generic button index

  // --- Device Detection State ---
    std::map<UINT_PTR, System::DeviceType> m_dinputDeviceTypes;
    std::array<System::DeviceType, 4> m_xinputDeviceTypes{};
    bool m_isXInputDeviceActive = false;
};

}  // namespace Input
SPF_NS_END