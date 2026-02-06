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
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
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
  void StartInputCapture(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding);
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

  // --- Programmatic (Plugin) Mouse Blocking ---
  void SetProgrammaticMouseBlock(bool blockAxes, bool blockButtons, bool blockWheel);
  bool IsProgrammaticMouseAxesBlockRequested() const { return m_pluginRequestedMouseAxesBlock; }
  bool IsProgrammaticMouseButtonsBlockRequested() const { return m_pluginRequestedMouseButtonsBlock; }
  bool IsProgrammaticMouseWheelBlockRequested() const { return m_pluginRequestedMouseWheelBlock; }

  const std::set<uint32_t>& GetCurrentlyPressedHardwareCodes() const { return m_currentlyPressedHardwareCodes; }

  bool ShouldGameControlMouseAxes() const;
  bool ShouldGameControlMouseButtons() const;
  bool ShouldGameControlMouseWheel() const;

  bool IsKeyBlocked(System::Keyboard key) const;
  bool IsMouseButtonBlocked(System::MouseButton button) const;
  bool IsJoystickButtonBlocked(int buttonIndex) const;
  bool IsGamepadButtonBlocked(System::GamepadButton button) const;
  
  /**
   * @brief Consumes a virtual mouse release request. Used by DInput8Hook to inject fake Up events.
   */
  bool ConsumeMouseReleaseRequest(System::MouseButton button);

  /**
   * @brief Consumes a virtual joystick release request. Used by DInput8Hook to inject fake Up events.
   */
  bool ConsumeJoystickReleaseRequest(int buttonIndex);
  bool HasPendingJoystickRelease(int buttonIndex) const;

  bool ConsumeGamepadReleaseRequest(System::GamepadButton button);
  bool HasPendingGamepadRelease(System::GamepadButton button) const;
  
  /**
   * @brief Checks if a key release event is a virtual one sent by the framework itself.
   * If the code is found in the pending set, it is removed and true is returned.
   */
  bool IsPendingVirtualRelease(uint32_t hardwareCode);

  /**
   * @brief Returns the timestamp of the most recently pressed key among the provided hardware codes.
   * Used to calculate the duration of chords based on the last key pressed.
   */
  std::chrono::steady_clock::time_point GetChordPressTimestamp(const std::vector<uint32_t>& codes) const;

  /**
   * @brief Resets the longPressTriggered state for a specific hardware code.
   * Used when a chord is formed to allow the chord to have its own timing cycle.
   */
  void ResetStateForCode(uint32_t code);

  // --- Device Detection ---
    void UpdateDeviceType(UINT_PTR deviceId, const std::wstring& productName, DWORD vid, DWORD pid);
    void RegisterXInputDevice(DWORD userIndex, BYTE subType);
  
    void SetXInputDeviceActive(bool isActive);
    System::DeviceType GetDetectedGamepadType() const;
    System::DeviceType GetXInputDeviceType(DWORD userIndex) const;
    System::DeviceType GetDeviceType(UINT_PTR deviceId) const;

 private:
  void UpdateCaptureUI();
  void OnXInputStateGet(DWORD deviceID, XINPUT_STATE* pState);

  inline static InputManager* s_instance = nullptr;

  Events::EventManager& m_eventManager;
  std::vector<IInputConsumer*> m_consumers;
  bool m_gameControlsMouseAxes = true;
  bool m_gameControlsMouseButtons = true;
  bool m_gameControlsMouseWheel = true;

  bool m_pluginRequestedMouseAxesBlock = false;
  bool m_pluginRequestedMouseButtonsBlock = false;
  bool m_pluginRequestedMouseWheelBlock = false;

  std::set<uint32_t> m_currentlyPressedHardwareCodes;

  // --- Chord Capture State ---
  std::set<uint32_t> m_captureHeldCodes;      // Keys currently physically held
  std::set<uint32_t> m_captureRecordedCodes;  // All keys that were part of this chord attempt
  std::map<uint32_t, std::shared_ptr<Modules::IBindableInput>> m_captureCodeToInputMap; // To reconstruct inputs
  std::chrono::steady_clock::time_point m_lastCaptureReleaseTime;
  bool m_isWaitingForCaptureFinalize = false;

  // The central state machine for all inputs
  std::map<uint32_t, ButtonState> m_inputStates;

  // Unified state for tracking inputs that are in a "hold" behavior state
  std::map<uint32_t, PressType> m_heldInputs;

  // State for XInputHook (legacy, might be removed later)
  std::unique_ptr<Utils::Sink<void(DWORD, XINPUT_STATE*)>> m_xinputSink;
  std::array<XINPUT_STATE, XUSER_MAX_COUNT> m_previousGamepadStates{};

  // --- Key Capture State ---
  enum class InputCaptureState { Idle, Capturing };

  InputCaptureState m_captureState = InputCaptureState::Idle;
  bool m_inPostCaptureCooldown = false;
  std::string m_capturingActionFullName;
  nlohmann::ordered_json m_capturingOriginalBinding;

  // Frame-specific blacklist to prevent input processing from multiple hooks
  std::set<uint32_t> m_capturedHardwareCodesThisFrame;

  // Keys that were passed to the game (not blocked)
  // Used to send retroactive "Key Up" events if a blocking chord activates later.
  std::set<uint32_t> m_keysLeakedToGame;
  
  // Keys for which we have sent a virtual "Key Up" event and are waiting for the message loop to process it.
  // Used to prevent the framework from reacting to its own fake events.
  std::set<uint32_t> m_pendingVirtualReleases;

  // Mouse buttons that need a virtual release event injected into the DirectInput8 buffer.
  std::set<System::MouseButton> m_pendingMouseReleases;

  // Joystick buttons that need a virtual release event injected into the DirectInput8 buffer.
  std::set<int> m_pendingJoystickReleases;

  // Gamepad buttons that need a virtual release event injected into the DirectInput8 buffer.
  std::set<System::GamepadButton> m_pendingGamepadReleases;

  /**
   * @brief Unified helper method to handle input state updates and blocking logic.
   * Replaces the duplicated logic in ProcessAndDecide for different device types.
   * @param hardwareCode The unique 32-bit hardware code for the input.
   * @param isDown Whether the button/key is currently pressed.
   * @param value The analog value (0.0f-1.0f) or digital value (0.0f/1.0f).
   * @param state Reference to the persistent state for this specific input.
   * @return True if the input should be blocked, false otherwise.
   */
  bool HandleInputState(uint32_t hardwareCode, bool isDown, float value, ButtonState& state);

  /**
   * @brief Evaluates action logic (Short Press, Long Press, Chords) for a specific input.
   * Replaces the duplicated loop logic in Process...Actions methods.
   */
  void EvaluateActionLogic(uint32_t hardwareCode, ButtonState& state);

  void HandleRetroactiveBlocking(uint32_t hardwareCode, bool shouldBlock);
  void SetHoldState(uint32_t hardwareCode, PressType type);

  // --- Chord Capture State ---
    std::map<UINT_PTR, System::DeviceType> m_dinputDeviceTypes;
    std::array<System::DeviceType, 4> m_xinputDeviceTypes{};
    bool m_isXInputDeviceActive = false;
};

}  // namespace Input
SPF_NS_END