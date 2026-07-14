#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Input/InputEvents.hpp"
#include "SPF/System/GamepadButton.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/System/MouseButtonMapping.hpp"
#include "SPF/Utils/Signal.hpp"

#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <minwindef.h>
#include <set>
#include <string>
#include <vector>
#include <winnt.h>
#include <xinput.h>

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
  uint8_t lastPriority = 255;  // Lower is higher priority
  uint64_t lastTimestamp = 0;
};

struct AxisState {
  float value = 0.0f;
  Config::ConsumptionPolicy policy = Config::ConsumptionPolicy::Never;
  bool emulationEnabled = false;
  bool isAccumulator = false;
  bool invert = false;
  std::string side = "both";
  float threshold = 0.5f;
  float sensitivity = 1.0f;
  float rangeMin = -1.0f;
  float rangeMax = 1.0f;
  uint8_t lastPriority = 255;
  uint64_t lastTimestamp = 0;
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

  // --- Axis Configuration (Called by KeyBindsManager when config changes) ---
  void SetAxisProperties(uint32_t hardwareCode, Config::ConsumptionPolicy policy, bool emulationEnabled, bool isAccumulator, bool invert, const std::string& side, float threshold,
                         float sensitivity = 1.0f, float rMin = -1.0f, float rMax = 1.0f);
  void ResetAxisProperties();

  /**
   * @brief Processes state changes from hooks and determines if input should be blocked.
   * This is called synchronously from the hooks.
   * @param event A struct containing the button and its current pressed state.
   * @param priority Priority of the source (1 = Highest, e.g. XInput).
   * @return True if the input should be blocked from the game, false otherwise.
   */
  bool ProcessAndDecide(const GamepadEvent& event, uint8_t priority = 2);
  bool ProcessAndDecide(const KeyboardEvent& event, uint8_t priority = 2);
  bool ProcessAndDecide(const MouseButtonEvent& event, uint8_t priority = 2);
  bool ProcessAndDecide(const JoystickEvent& event, uint8_t priority = 2);

  /**
   * @brief Processes actions based on the current state of all buttons.
   * This is called once per frame from the main loop (e.g., UIManager).
   */
  void ProcessButtonActions();
  void ProcessKeyboardActions();
  void ProcessMouseActions();
  void ProcessJoystickActions();

  // --- Key Capture ---
  enum class InputCaptureState { Idle, Capturing };
  void StartInputCapture(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding);
  void CancelInputCapture();
  InputCaptureState GetCaptureState() const { return m_captureState; }

  // --- Consumer Management ---
  void RegisterConsumer(IInputConsumer* consumer);
  void UnregisterConsumer(IInputConsumer* consumer);

  // --- Event Publishing (from hooks) ---
  void PublishMouseMove(const MouseMoveEvent& event);
  bool PublishMouseButton(const MouseButtonEvent& event, uint8_t priority = 2);
  bool PublishMouseWheel(const MouseWheelEvent& event, uint8_t priority = 2);

  // New: Generic Axis Event Publishing
  // deviceType: 0x02 (Gamepad), 0x03 (Mouse), 0x04 (Joystick)
  // axisIndex: For Mouse (0=X, 1=Y, 2=WheelY). For Gamepad/Joystick (0..N)
  bool PublishAxisMove(uint8_t deviceType, int axisIndex, float value, uint8_t priority = 2);

  bool PublishKeyboardEvent(const KeyboardEvent& event, uint8_t priority = 2);
  bool PublishGamepadEvent(const GamepadEvent& event, uint8_t priority = 2);
  bool PublishJoystickEvent(const JoystickEvent& event, uint8_t priority = 2);

  // --- Cursor Control ---
  void SetMouseAxesControl(bool gameHasControl);
  void SetMouseButtonsControl(bool gameHasControl);
  void SetMouseWheelControl(bool gameHasControl);

  // --- Programmatic (Plugin) Mouse Blocking ---
  void SetProgrammaticMouseBlock(bool blockAxes, bool blockButtons, bool blockWheel);
  bool IsAxisAccumulator(uint32_t hardwareCode) const;
  bool IsProgrammaticMouseAxesBlockRequested() const;
  bool IsProgrammaticMouseButtonsBlockRequested() const;
  bool IsProgrammaticMouseWheelBlockRequested() const;

  const std::set<uint32_t>& GetCurrentlyPressedHardwareCodes() const { return m_currentlyPressedHardwareCodes; }
  const std::map<uint32_t, float>& GetCurrentlyActiveAxisValues() const { return m_activeAxisValues; }

  bool ShouldGameControlMouseAxes() const;
  bool ShouldGameControlMouseButtons() const;
  bool ShouldGameControlMouseWheel() const;

  bool IsKeyBlocked(System::Keyboard key) const;
  bool IsMouseButtonBlocked(System::MouseButton button) const;
  bool IsJoystickButtonBlocked(int buttonIndex) const;
  bool IsGamepadButtonBlocked(System::GamepadButton button) const;

  bool IsKeyboardCaptured() const;
  bool IsMouseCaptured() const;

  // New: Check if an axis is consumed by the framework
  bool IsAxisConsumed(uint8_t deviceType, int axisIndex) const;

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

  struct MouseBlockRequest {
    bool axes = false;
    bool buttons = false;
    bool wheel = false;
  };
  std::map<void*, MouseBlockRequest> m_programmaticMouseBlocks;

  std::set<uint32_t> m_currentlyPressedHardwareCodes;
  std::map<uint32_t, float> m_activeAxisValues;
  std::map<uint32_t, float> m_accumulatorTargets;  // Raw values for integration
  std::chrono::steady_clock::time_point m_lastFrameTimestamp;

  // --- Chord Capture State ---
  std::set<uint32_t> m_captureHeldCodes;                                                 // Keys currently physically held
  std::set<uint32_t> m_captureRecordedCodes;                                             // All keys that were part of this chord attempt
  std::map<uint32_t, float> m_captureInitialAxisValues;                                  // Snapshot of axes when capture starts
  std::map<uint32_t, std::shared_ptr<Modules::IBindableInput>> m_captureCodeToInputMap;  // To reconstruct inputs
  std::chrono::steady_clock::time_point m_lastCaptureReleaseTime;
  bool m_isWaitingForCaptureFinalize = false;

  // The central state machine for all inputs
  std::map<uint32_t, ButtonState> m_inputStates;

  // State machine for analog axes (hardwareCode -> AxisState)
  std::map<uint32_t, AxisState> m_axisStates;

  // Unified state for tracking inputs that are in a "hold" behavior state
  std::map<uint32_t, PressType> m_heldInputs;

  // State for XInputHook (legacy, might be removed later)
  std::unique_ptr<Utils::Sink<void(DWORD, XINPUT_STATE*)>> m_xinputSink;
  std::array<XINPUT_STATE, XUSER_MAX_COUNT> m_previousGamepadStates{};

  // --- Key Capture State ---
  InputCaptureState m_captureState = InputCaptureState::Idle;
  bool m_inPostCaptureCooldown = false;
  std::string m_capturingActionFullName;
  nlohmann::ordered_json m_capturingOriginalBinding;

  // Frame-specific blacklist to prevent input processing from multiple hooks
  std::set<uint32_t> m_capturedHardwareCodesThisFrame;
  uint8_t m_minPriorityCapturedThisFrame = 255;
  uint64_t m_lastCaptureTimestamp = 0;

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