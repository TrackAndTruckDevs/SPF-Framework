#include <Windows.h>  // Pre-include for safety

#include "SPF/Input/InputManager.hpp"
#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Hooks/XInputHook.hpp"
#include "SPF/Hooks/User32Hook.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/System/MouseButtonMapping.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Modules/KeyboardInput.hpp"
#include "SPF/Modules/GamepadInput.hpp"
#include "SPF/Modules/MouseInput.hpp"
#include "SPF/Modules/JoystickInput.hpp"
#include "SPF/Modules/ChordInput.hpp"
#include "SPF/Modules/InputFactory.hpp"

#include <chrono>

#include <cassert>
#include <algorithm>  // for std::remove

SPF_NS_BEGIN

namespace Input {
using namespace SPF::Logging;
using namespace SPF::System;

// Helper to distinguish axis enums from button enums
bool IsAxis(GamepadButton button) {
  switch (button) {
    case GamepadButton::LeftStickX:
    case GamepadButton::LeftStickY:
    case GamepadButton::RightStickX:
    case GamepadButton::RightStickY:
    case GamepadButton::LeftTrigger:
    case GamepadButton::RightTrigger:
      return true;
    default:
      return false;
  }
}

InputManager& InputManager::GetInstance() {
  assert(s_instance != nullptr && "InputManager instance is not available. It should be created by Core.");
  return *s_instance;
}

InputManager::InputManager(Events::EventManager& eventManager) : m_eventManager(eventManager) {
  assert(s_instance == nullptr && "An instance of InputManager already exists.");
  s_instance = this;
  m_xinputDeviceTypes.fill(System::DeviceType::Unknown);
}

InputManager::~InputManager() { s_instance = nullptr; }

void InputManager::Initialize() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("InputManager initialized.");
}

void InputManager::Shutdown() {
  // m_xinputSink is no longer used
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("InputManager shut down.");
}

void InputManager::RegisterConsumer(IInputConsumer* consumer) {
  if (consumer) {
    m_consumers.push_back(consumer);
  }
}

void InputManager::UnregisterConsumer(IInputConsumer* consumer) { m_consumers.erase(std::remove(m_consumers.begin(), m_consumers.end(), consumer), m_consumers.end()); }

void InputManager::PublishMouseMove(const MouseMoveEvent& event) {
  for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
    if ((*it)->OnMouseMove(event)) {
      // Event was consumed, stop propagation
      return;
    }
  }
}

bool InputManager::PublishMouseButton(const MouseButtonEvent& event) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(event.iButton);
  if (event.bPressed) {
      m_currentlyPressedHardwareCodes.insert(hardwareCode);
  } else {
      m_currentlyPressedHardwareCodes.erase(hardwareCode);
  }

  if (m_capturedHardwareCodesThisFrame.count(hardwareCode)) {
      return true;
  }

  bool shouldBlock = ProcessAndDecide(event);

  if (shouldBlock) {
    // If the event should be blocked, do not propagate to consumers and inform the caller to block it.
    return true;
  }

  // Propagate to consumers if not blocked
  for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
    if ((*it)->OnMouseButton(event)) {
      return true;
    }
  }
  return false;
}

bool InputManager::PublishMouseWheel(const MouseWheelEvent& event) {
  for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
    if ((*it)->OnMouseWheel(event)) {
      // Event was consumed, stop propagation
      return true;
    }
  }
  return false;
}

bool InputManager::PublishKeyboardEvent(const KeyboardEvent& event) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(event.key);
  if (event.pressed) {
      m_currentlyPressedHardwareCodes.insert(hardwareCode);
  } else {
      m_currentlyPressedHardwareCodes.erase(hardwareCode);
  }

  if (m_capturedHardwareCodesThisFrame.count(hardwareCode)) {
    return true;  // Consume event from a second hook/source
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  if (m_captureState == InputCaptureState::Capturing) {
    uint32_t code = 0x01000000 | static_cast<uint32_t>(event.key);
    
    if (event.pressed) {
        m_captureHeldCodes.insert(code);
        m_captureRecordedCodes.insert(code);
        // Map the code to a proper KeyboardInput for later reconstruction
        m_captureCodeToInputMap[code] = std::make_shared<Modules::KeyboardInput>(nlohmann::ordered_json{{"type", "keyboard"}, {"key", VirtualKeyMapping::GetInstance().GetKeyName(event.key)}});
        m_isWaitingForCaptureFinalize = false; // Stop timer on any new press
    } else {
        m_captureHeldCodes.erase(code);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true; // Start/restart the 300ms timer
    }

    UpdateCaptureUI();

    // Reset the state of the key to prevent immediate action trigger while recording
    auto& state = m_inputStates[hardwareCode];
    state.isDown = false;
    state.wasDown = false;
    state.longPressTriggered = false;
    
    return true;  // Consume the input entirely during capture
  }

  // Propagate to consumers first, to let UI elements like ImGui capture input.
  bool consumedByConsumer = false;
  if (event.pressed) {
    for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
      if ((*it)->OnKeyPress(event)) {
        consumedByConsumer = true;
        break;
      }
    }
  } else {
    for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
      if ((*it)->OnKeyRelease(event)) {
        consumedByConsumer = true;
        break;
      }
    }
  }

  // If a consumer (like ImGui) handled the event, we block it from the game and, crucially,
  // we do not process it for our own keybinds system.
  // Exception: We never block the Escape key, allowing it to be processed by our keybind system
  // even if ImGui captures it (e.g., to close a modal).
  if (consumedByConsumer && event.key != System::Keyboard::Escape) {
    return true;
  }

  // If not consumed by UI, then process for keybinds and decide on blocking.
  bool shouldBlock = ProcessAndDecide(event);

  if (shouldBlock) {
    return true;
  }

  return false;  // Not blocked, not consumed
}

bool InputManager::PublishGamepadEvent(const GamepadEvent& event) {
  // Update currently pressed hardware codes for chords (only for digital buttons)
  if (!IsAxis(event.button)) {
      uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);
      if (event.pressed) {
          m_currentlyPressedHardwareCodes.insert(hardwareCode);
      } else {
          m_currentlyPressedHardwareCodes.erase(hardwareCode);
      }
  }

  // Check frame-based capture set
  if (!IsAxis(event.button)) {
    uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);
    if (m_capturedHardwareCodesThisFrame.count(hardwareCode)) {
      return true;
    }
  }

  // First, process the event through the state machine to determine if it should be blocked from the game.
  bool shouldBlock = ProcessAndDecide(event);

  // In capture mode, the decision from ProcessAndDecide is final.
  if (m_captureState == InputCaptureState::Capturing && event.pressed && !IsAxis(event.button)) {
    return shouldBlock;
  }

  // Second, regardless of blocking, propagate the event to internal UI consumers.
  if (IsAxis(event.button)) {
    for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
      if ((*it)->OnGamepadAxisMove(event)) {
        break;  // A consumer handled it.
      }
    }
  } else {
    if (event.pressed) {
      for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
        if ((*it)->OnGamepadButtonPress(event)) {
          break;  // A consumer handled it.
        }
      }
    } else {
      for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
        if ((*it)->OnGamepadButtonRelease(event)) {
          break;  // A consumer handled it.
        }
      }
    }
  }

  // Finally, return the decision that was made by ProcessAndDecide.
  return shouldBlock;
}

bool InputManager::PublishJoystickEvent(const JoystickEvent& event) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(event.buttonIndex);
  if (event.pressed) {
      m_currentlyPressedHardwareCodes.insert(hardwareCode);
  } else {
      m_currentlyPressedHardwareCodes.erase(hardwareCode);
  }

  if (m_capturedHardwareCodesThisFrame.count(hardwareCode)) {
      return true;
  }

  // This function now mirrors PublishKeyboardEvent.
  // Process the event through the state machine to determine blocking
  bool shouldBlock = ProcessAndDecide(event);

  if (m_captureState == InputCaptureState::Capturing && event.pressed) {
    return shouldBlock;
  }

  // Propagate to consumers
  if (event.pressed) {
    for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
      if ((*it)->OnJoystickButtonPress(event)) {
        break;
      }
    }
  } else {
    for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
      if ((*it)->OnJoystickButtonRelease(event)) {
        break;
      }
    }
  }

  return shouldBlock;
}

void InputManager::ProcessButtonActions() {
  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      if ((pair.first >> 24) == 0x02) { // Gamepad
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    if ((pair.first >> 24) == 0x02) pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::HandleRetroactiveBlocking(uint32_t hardwareCode, bool shouldBlock) {
    uint8_t type = (hardwareCode >> 24) & 0xFF;
    uint32_t rawCode = hardwareCode & 0x00FFFFFF;
    
    // 1. Update internal block state
    auto it = m_inputStates.find(hardwareCode);
    if (it != m_inputStates.end()) {
        it->second.blockInput = shouldBlock;
    }

    // 2. Handle retroactive release if needed
    if (shouldBlock && m_keysLeakedToGame.count(hardwareCode)) {
        if (type == 0x01) { // Keyboard
            Hooks::User32Hook::SendVirtualKeyRelease(hardwareCode);
            m_pendingVirtualReleases.insert(hardwareCode);
        } else if (type == 0x03) { // Mouse
            m_pendingMouseReleases.insert(static_cast<System::MouseButton>(rawCode));
            m_pendingVirtualReleases.insert(hardwareCode);
        } else if (type == 0x04) { // Joystick
            m_pendingJoystickReleases.insert(static_cast<int>(rawCode));
        } else if (type == 0x02) { // Gamepad
            m_pendingGamepadReleases.insert(static_cast<System::GamepadButton>(rawCode));
        }
        m_keysLeakedToGame.erase(hardwareCode);
    }
}

void InputManager::ProcessKeyboardActions() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto now = std::chrono::steady_clock::now();
  auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();

  // --- Chord Capture Finalization Logic ---
  if (m_captureState == InputCaptureState::Capturing && m_isWaitingForCaptureFinalize) {
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastCaptureReleaseTime);
      if (elapsed.count() >= 300) {
          if (m_captureHeldCodes.empty()) {
              // ALL keys released - FINALIZE!
              if (!m_captureRecordedCodes.empty()) {
                  std::shared_ptr<Modules::IBindableInput> result;
                  
                  if (m_captureRecordedCodes.size() == 1) {
                      // Single key
                      result = m_captureCodeToInputMap[*m_captureRecordedCodes.begin()];
                  } else {
                      // True chord
                      auto chord = std::make_shared<Modules::ChordInput>();
                      for (uint32_t c : m_captureRecordedCodes) {
                          // We need to clone or create a new unique_ptr here since ChordInput takes ownership
                          // but for simplicity in this flow, we'll re-parse the JSON of the input
                          chord->AddInput(Modules::InputFactory::CreateFromJson(m_captureCodeToInputMap[c]->ToJson()));
                      }
                      result = chord;
                  }

                  logger->Info("Chord capture finalized: {}", result->GetDisplayName());
                  InputCaptured captured_data{result, m_capturingActionFullName, m_capturingOriginalBinding};
                  m_eventManager.System.OnInputCaptured.Call(captured_data);
                  m_captureState = InputCaptureState::Idle;
              }
          } else {
              // Some keys still held - TRIMMING (Example 4 logic)
              // We reset Recorded keys to only those currently being Held.
              m_captureRecordedCodes = m_captureHeldCodes;
              
              // Inform UI about the trimmed chord
              InputCaptureUpdate update;
              update.actionFullName = m_capturingActionFullName;
              for (uint32_t c : m_captureRecordedCodes) {
                  if (m_captureCodeToInputMap.count(c)) {
                      update.currentChordInputs.push_back(m_captureCodeToInputMap[c]);
                  }
              }
              m_eventManager.System.OnInputCaptureUpdate.Call(update);
          }
          m_isWaitingForCaptureFinalize = false;
      }
  }

  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      if ((pair.first >> 24) == 0x01) { // Keyboard
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    if ((pair.first >> 24) == 0x01) pair.second.wasDown = pair.second.isDown;
  }

  // Reset the cooldown flag at the very end of all processing
  if (m_inPostCaptureCooldown) {
    m_inPostCaptureCooldown = false;
  }

  // Reset frame-specific blacklists at the end of all processing.
  m_capturedHardwareCodesThisFrame.clear();
}

void InputManager::ProcessMouseActions() {
  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      if ((pair.first >> 24) == 0x03) { // Mouse
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    if ((pair.first >> 24) == 0x03) pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::ProcessJoystickActions() {
  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      if ((pair.first >> 24) == 0x04) { // Joystick
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    if ((pair.first >> 24) == 0x04) pair.second.wasDown = pair.second.isDown;
  }
}

bool InputManager::ProcessAndDecide(const GamepadEvent& event) {
  uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);

  // Handle axes separately from buttons (always pass through for now, as requested)
  if (IsAxis(event.button)) {
      return false; 
  }

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  if (m_captureState == InputCaptureState::Capturing) {
    if (event.pressed) {
        logger->Info("Captured gamepad button for action: {}", m_capturingActionFullName);
        m_captureHeldCodes.insert(hardwareCode);
        m_captureRecordedCodes.insert(hardwareCode);
        m_captureCodeToInputMap[hardwareCode] = std::make_shared<Modules::GamepadInput>(nlohmann::ordered_json{{"type", "gamepad"}, {"button", GamepadButtonMapping::GetInstance().GetButtonName(event.button)}});
        m_isWaitingForCaptureFinalize = false; 
    } else {
        m_captureHeldCodes.erase(hardwareCode);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true; 
    }

    UpdateCaptureUI();

    // Reset the state in the unified map
    auto& state = m_inputStates[hardwareCode];
    state.isDown = false;
    state.wasDown = false;
    state.longPressTriggered = false;
    return true;  // Consume the event
  }

  // Delegate to unified handler
  return HandleInputState(hardwareCode, event.pressed, event.value, m_inputStates[hardwareCode]);
}

bool InputManager::IsGamepadButtonBlocked(System::GamepadButton button) const {
  uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(button);
  auto it = m_inputStates.find(hardwareCode);
  return (it != m_inputStates.end()) ? it->second.blockInput : false;
}

bool InputManager::HandleInputState(uint32_t hardwareCode, bool isDown, float value, ButtonState& state) {
    // Track keys leaked to game
    if (isDown) {
        m_keysLeakedToGame.insert(hardwareCode);
    } else {
        m_keysLeakedToGame.erase(hardwareCode);
    }

    bool wasDown = state.isDown;
    state.isDown = isDown;

    // On new press (Up -> Down transition)
    if (!wasDown && state.isDown) {
        state.pressTimestamp = std::chrono::steady_clock::now();
        state.longPressTriggered = false;

        // --- Chord Reset Logic ---
        const auto* bestShort = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Short);
        const auto* bestLong = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Long);
        const auto* bestChord = (bestShort && dynamic_cast<const Modules::ChordInput*>(bestShort->Input.get())) ? bestShort :
            ((bestLong && dynamic_cast<const Modules::ChordInput*>(bestLong->Input.get())) ? bestLong : nullptr);

        if (bestChord) {
            if (auto* chord = dynamic_cast<const Modules::ChordInput*>(bestChord->Input.get())) {
                for (uint32_t code : chord->GetConstituentHardwareCodes()) {
                    ResetStateForCode(code);
                }
            }
        }

        if (const auto* binding = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Short)) {
             if (binding->Behavior == Modules::ActivationBehavior::Hold) {
                 Modules::KeyBindsManager::GetInstance().TriggerAction(hardwareCode, PressType::Short);
                 SetHoldState(hardwareCode, PressType::Short);
             }
        }

        // Determine initial block policy based on the short press action.
        Config::ConsumptionPolicy policy = Config::ConsumptionPolicy::Never;
        
        if (bestShort) {
             policy = bestShort->Policy;
        }

        bool shouldBlock = false;
        switch (policy) {
        case Config::ConsumptionPolicy::Always:
            shouldBlock = true;
            break;
        case Config::ConsumptionPolicy::OnUIFocus:
            shouldBlock = !m_gameControlsMouseButtons;
            break;
        case Config::ConsumptionPolicy::Manual:
            shouldBlock = bestShort->programmaticallyBlocked;
            break;
        default:
            shouldBlock = false;
            break;
        }
        state.blockInput = shouldBlock;

        if (state.blockInput) {
            m_keysLeakedToGame.erase(hardwareCode);
        }
    }
    // On release (Down -> Up transition)
    else if (wasDown && !state.isDown) {
        m_keysLeakedToGame.erase(hardwareCode);
        bool finalBlockDecision = state.blockInput;
        state.blockInput = false;
        return finalBlockDecision;
    }

    // --- Retroactive Blocking Logic ---
    if (state.blockInput && isDown) {
        if (m_keysLeakedToGame.count(hardwareCode)) {
            HandleRetroactiveBlocking(hardwareCode, true);
        }

        const auto* bestBinding = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Short);
        if (bestBinding) {
            if (auto* chord = dynamic_cast<const Modules::ChordInput*>(bestBinding->Input.get())) {
                for (uint32_t constituentCode : chord->GetConstituentHardwareCodes()) {
                    if (m_keysLeakedToGame.count(constituentCode)) {
                        HandleRetroactiveBlocking(constituentCode, true);
                    }
                }
            }
        }
    } else if (!isDown) {
        m_keysLeakedToGame.erase(hardwareCode);
    }

    return state.blockInput;
}

bool InputManager::ProcessAndDecide(const MouseButtonEvent& event) {
  auto button = static_cast<MouseButton>(event.iButton);
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(button);

  if (m_captureState == InputCaptureState::Capturing) {
    // In capture mode, we handle input differently.

    // Per user request, NEVER capture the left mouse button. Let it pass through to the UI.
    if (button == MouseButton::Left) {
      // logger->Trace("Ignoring Left Mouse Button during input capture to allow UI interaction.");
      return false;
    }

    if (event.bPressed) {
        logger->Info("Captured mouse button for action: {}", m_capturingActionFullName);
        m_captureHeldCodes.insert(hardwareCode);
        m_captureRecordedCodes.insert(hardwareCode);
        m_captureCodeToInputMap[hardwareCode] = std::make_shared<Modules::MouseInput>(nlohmann::ordered_json{{"type", "mouse"}, {"key", MouseButtonMapping::GetInstance().ToString(button)}});
        m_isWaitingForCaptureFinalize = false;
    } else {
        m_captureHeldCodes.erase(hardwareCode);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true;
    }

    UpdateCaptureUI();

    // Reset state in unified map
    auto& state = m_inputStates[hardwareCode];
    state.isDown = false;
    state.wasDown = false;
    state.longPressTriggered = false;

    // CRUCIAL FIX: For any non-left button, always consume the event (press and release)
    // to prevent it from closing the ImGui capture popup.
    return true;
  }

  // --- Regular (Non-Capture) Logic ---
  return HandleInputState(hardwareCode, event.bPressed, event.bPressed ? 1.0f : 0.0f, m_inputStates[hardwareCode]);
}

bool InputManager::ProcessAndDecide(const JoystickEvent& event) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto buttonIndex = event.buttonIndex;
  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(buttonIndex);

  if (m_captureState == InputCaptureState::Capturing) {
    if (event.pressed) {
        logger->Info("Captured joystick button for action: {}", m_capturingActionFullName);
        m_captureHeldCodes.insert(hardwareCode);
        m_captureRecordedCodes.insert(hardwareCode);
        m_captureCodeToInputMap[hardwareCode] = std::make_shared<Modules::JoystickInput>(nlohmann::ordered_json{{"type", "joystick"}, {"key", JoystickButtonMapping::GetInstance().ToString(buttonIndex)}});
        m_isWaitingForCaptureFinalize = false;
    } else {
        m_captureHeldCodes.erase(hardwareCode);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true;
    }

    UpdateCaptureUI();

    // Reset state in unified map
    auto& state = m_inputStates[hardwareCode];
    state.isDown = false;
    state.wasDown = false;
    state.longPressTriggered = false;

    // Always consume joystick events in capture mode (press and release)
    return true;
  }

  // --- Regular (Non-Capture) Logic ---
  bool block = HandleInputState(hardwareCode, event.pressed, event.pressed ? 1.0f : 0.0f, m_inputStates[hardwareCode]);
  
  // Specific fix for Joystick release cleanup:
  if (!event.pressed) {
      m_pendingJoystickReleases.erase(buttonIndex);
      m_pendingVirtualReleases.erase(hardwareCode);
  }
  return block;
}

void InputManager::ResetStateForCode(uint32_t code) {
    auto it = m_inputStates.find(code);
    if (it != m_inputStates.end()) {
        it->second.longPressTriggered = false;
    }
}

bool InputManager::ProcessAndDecide(const KeyboardEvent& event) {
  // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  // logger->Trace("ProcessAndDecide (Keyboard): key={}, pressed={}", (int)event.key, event.pressed);
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(event.key);

  // Keyboard has no analog value, so passing 0.0f/1.0f
  // Logic for capture is handled in PublishKeyboardEvent, so here we only do State/Blocking
  return HandleInputState(hardwareCode, event.pressed, event.pressed ? 1.0f : 0.0f, m_inputStates[hardwareCode]);
}

bool InputManager::ShouldGameControlMouseAxes() const { return m_gameControlsMouseAxes; }
bool InputManager::ShouldGameControlMouseButtons() const { return m_gameControlsMouseButtons; }
bool InputManager::ShouldGameControlMouseWheel() const { return m_gameControlsMouseWheel; }

void InputManager::SetMouseAxesControl(bool gameHasControl) { m_gameControlsMouseAxes = gameHasControl; }
void InputManager::SetMouseButtonsControl(bool gameHasControl) { m_gameControlsMouseButtons = gameHasControl; }
void InputManager::SetMouseWheelControl(bool gameHasControl) { m_gameControlsMouseWheel = gameHasControl; }

void InputManager::SetProgrammaticMouseBlock(bool blockAxes, bool blockButtons, bool blockWheel) {
    m_pluginRequestedMouseAxesBlock = blockAxes;
    m_pluginRequestedMouseButtonsBlock = blockButtons;
    m_pluginRequestedMouseWheelBlock = blockWheel;
}

void InputManager::StartInputCapture(const std::string& actionFullName, const nlohmann::ordered_json& originalBinding) {
  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("Starting key capture for action: {}", actionFullName);
  m_captureState = InputCaptureState::Capturing;
  m_capturingActionFullName = actionFullName;
  m_capturingOriginalBinding = originalBinding;

  // Reset chord capture state
  m_captureHeldCodes.clear();
  m_captureRecordedCodes.clear();
  m_captureCodeToInputMap.clear();
  m_isWaitingForCaptureFinalize = false;
}

void InputManager::CancelInputCapture() {
  if (m_captureState == InputCaptureState::Idle) return;

  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("Input capture cancelled for action: {}", m_capturingActionFullName);
  m_captureState = InputCaptureState::Idle;
  m_inPostCaptureCooldown = true;
  m_eventManager.System.OnInputCaptureCancelled.Call({m_capturingActionFullName});
}

bool InputManager::IsKeyBlocked(System::Keyboard key) const {
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(key);
  auto it = m_inputStates.find(hardwareCode);
  return (it != m_inputStates.end()) ? it->second.blockInput : false;
}

bool InputManager::IsMouseButtonBlocked(System::MouseButton button) const {
  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(button);
  auto it = m_inputStates.find(hardwareCode);
  return (it != m_inputStates.end()) ? it->second.blockInput : false;
}

bool InputManager::IsJoystickButtonBlocked(int buttonIndex) const {
  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(buttonIndex);
  auto it = m_inputStates.find(hardwareCode);
  return (it != m_inputStates.end()) ? it->second.blockInput : false;
}

bool InputManager::ConsumeMouseReleaseRequest(System::MouseButton button) {
    auto it = m_pendingMouseReleases.find(button);
    if (it != m_pendingMouseReleases.end()) {
        m_pendingMouseReleases.erase(it);
        return true;
    }
    return false;
}

bool InputManager::ConsumeJoystickReleaseRequest(int buttonIndex) {
    auto it = m_pendingJoystickReleases.find(buttonIndex);
    if (it != m_pendingJoystickReleases.end()) {
        m_pendingJoystickReleases.erase(it);
        return true;
    }
    return false;
}

bool InputManager::HasPendingJoystickRelease(int buttonIndex) const {
    return m_pendingJoystickReleases.count(buttonIndex) > 0;
}

bool InputManager::ConsumeGamepadReleaseRequest(System::GamepadButton button) {
    auto it = m_pendingGamepadReleases.find(button);
    if (it != m_pendingGamepadReleases.end()) {
        m_pendingGamepadReleases.erase(it);
        return true;
    }
    return false;
}

bool InputManager::HasPendingGamepadRelease(System::GamepadButton button) const {
    return m_pendingGamepadReleases.count(button) > 0;
}

bool InputManager::IsPendingVirtualRelease(uint32_t hardwareCode) {
    auto it = m_pendingVirtualReleases.find(hardwareCode);
    if (it != m_pendingVirtualReleases.end()) {
        m_pendingVirtualReleases.erase(it); // Consume the event
        return true;
    }
    return false;
}

void InputManager::SetHoldState(uint32_t hardwareCode, PressType type) {
    m_heldInputs[hardwareCode] = type;
}

std::chrono::steady_clock::time_point InputManager::GetChordPressTimestamp(const std::vector<uint32_t>& codes) const {
    auto maxTs = (std::chrono::steady_clock::time_point::min)();
    for (uint32_t code : codes) {
        auto it = m_inputStates.find(code);
        if (it != m_inputStates.end()) {
            if (it->second.pressTimestamp > maxTs) {
                maxTs = it->second.pressTimestamp;
            }
        }
    }
    return maxTs;
}

void InputManager::EvaluateActionLogic(uint32_t hardwareCode, ButtonState& state) {
    // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();
    auto now = std::chrono::steady_clock::now();

    // --- Detect and Handle Release (for Short Press) ---
    if (!state.isDown && state.wasDown) {
        auto it = m_heldInputs.find(hardwareCode);
        if (it != m_heldInputs.end()) {
            PressType originalPressType = it->second;
            keyBindsManager.TriggerAction(hardwareCode, originalPressType);
            m_heldInputs.erase(it);
        } else if (!state.longPressTriggered) {
            keyBindsManager.TriggerAction(hardwareCode, PressType::Short);
        }
    }
    // --- Detect and Handle Hold (for Long Press) ---
    else if (state.isDown && state.wasDown) {
        if (!state.longPressTriggered) {
            const auto* longPressBinding = keyBindsManager.FindBestBinding(hardwareCode, PressType::Long);
            const auto* shortPressBinding = keyBindsManager.FindBestBinding(hardwareCode, PressType::Short);

            auto longPressThreshold = keyBindsManager.GetLongPressThreshold();
            auto pressTs = state.pressTimestamp;

            const auto* dominantBinding = longPressBinding ? longPressBinding : shortPressBinding;
            if (dominantBinding) {
                if (auto* chord = dynamic_cast<const Modules::ChordInput*>(dominantBinding->Input.get())) {
                    pressTs = GetChordPressTimestamp(chord->GetConstituentHardwareCodes());
                }
            }

            if (longPressBinding && longPressBinding->PressThreshold.has_value()) {
                longPressThreshold = longPressBinding->PressThreshold.value();
            } else if (shortPressBinding && shortPressBinding->PressThreshold.has_value()) {
                longPressThreshold = shortPressBinding->PressThreshold.value();
            }

            auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - pressTs);

            if (pressedDuration >= longPressThreshold) {
                state.longPressTriggered = true;

                if (longPressBinding) {
                    bool isLead = true;
                    std::vector<uint32_t> keysToBlock;
                    keysToBlock.push_back(hardwareCode);

                    if (auto* chord = dynamic_cast<const Modules::ChordInput*>(longPressBinding->Input.get())) {
                        auto codes = chord->GetConstituentHardwareCodes();
                        uint32_t maxCode = 0;
                        for (uint32_t c : codes) if (c > maxCode) maxCode = c;
                        if (hardwareCode != maxCode) isLead = false;
                        if (isLead) keysToBlock = codes;
                    }

                    if (isLead) {
                        if (longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
                            for (uint32_t code : keysToBlock) {
                                SetHoldState(code, PressType::Long);
                            }
                        }

                        keyBindsManager.TriggerAction(hardwareCode, PressType::Long);

                        // Update block policy based on long press action
                        // We need the policy. Since we have the binding, use it directly.
                        auto policy = longPressBinding->Policy;
                        
                        bool shouldBlock = false;
                        switch (policy) {
                        case Config::ConsumptionPolicy::Always: shouldBlock = true; break;
                        case Config::ConsumptionPolicy::OnUIFocus: shouldBlock = !m_gameControlsMouseButtons; break;
                        case Config::ConsumptionPolicy::Manual: shouldBlock = longPressBinding->programmaticallyBlocked; break;
                        default: shouldBlock = false; break;
                        }

                        for (uint32_t code : keysToBlock) {
                            HandleRetroactiveBlocking(code, shouldBlock);
                        }
                    }
                }
            }
        }
    }
}

// --- Device Detection Implementations ---

void InputManager::UpdateDeviceType(UINT_PTR deviceId, const std::wstring& productName, DWORD vid, DWORD pid) {
  System::DeviceType detectedType = System::DeviceType::Joystick;

  // --- Primary detection via Vendor ID (VID) ---
  if (vid == 0x045E || vid == 0x045e) {  // Microsoft
    detectedType = System::DeviceType::Xbox;
  } else if (vid == 0x054C || vid == 0x054c) {  // Sony
    detectedType = System::DeviceType::PlayStation;
  } else {
    // --- Fallback to string matching for unknown VIDs ---
    std::wstring lowerProductName = productName;
    std::transform(lowerProductName.begin(), lowerProductName.end(), lowerProductName.begin(), ::towlower);

    if (lowerProductName.find(L"dualshock") != std::wstring::npos || lowerProductName.find(L"dualsense") != std::wstring::npos) {
      detectedType = System::DeviceType::PlayStation;
    } else if (lowerProductName.find(L"xbox") != std::wstring::npos || lowerProductName.find(L"xinput") != std::wstring::npos) {
      detectedType = System::DeviceType::Xbox;
    }
  }

  if (m_dinputDeviceTypes.find(deviceId) == m_dinputDeviceTypes.end() || m_dinputDeviceTypes[deviceId] != detectedType) {
    m_dinputDeviceTypes[deviceId] = detectedType;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    // Updated log message to include VID/PID for easier debugging
    logger->Info("DirectInput device detected/updated: ID={}, Type={}, VID={:#06x}, PID={:#06x}", deviceId, (int)detectedType, vid, pid);
  }
}

System::DeviceType InputManager::GetDeviceType(UINT_PTR deviceId) const {
  auto it = m_dinputDeviceTypes.find(deviceId);
  if (it != m_dinputDeviceTypes.end()) {
    return it->second;
  }
  return System::DeviceType::Joystick;  // Default to generic if not specifically identified
}

void InputManager::RegisterXInputDevice(DWORD userIndex, BYTE subType) {
  if (userIndex >= XUSER_MAX_COUNT) {
    return;
  }

  System::DeviceType detectedType = System::DeviceType::Joystick;  // Default for non-gamepads
  if (subType == XINPUT_DEVSUBTYPE_GAMEPAD) {
    detectedType = System::DeviceType::Xbox;
  }

  m_xinputDeviceTypes[userIndex] = detectedType;

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("XInput device registered: Index={}, SubType={}, Classified as Type={}", userIndex, (int)subType, (int)detectedType);
}

System::DeviceType InputManager::GetXInputDeviceType(DWORD userIndex) const {
  if (userIndex >= XUSER_MAX_COUNT) {
    return System::DeviceType::Joystick;  // Out of bounds, return default
  }
  return m_xinputDeviceTypes[userIndex];
}

void InputManager::SetXInputDeviceActive(bool isActive) {
  if (m_isXInputDeviceActive != isActive) {
    m_isXInputDeviceActive = isActive;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    logger->Info("XInput device status changed: Active={}", isActive);
  }
}

System::DeviceType InputManager::GetDetectedGamepadType() const {
  for (const auto& pair : m_dinputDeviceTypes) {
    if (pair.second == System::DeviceType::PlayStation) {
      return System::DeviceType::PlayStation;
    }
  }

  if (m_isXInputDeviceActive) {
    return System::DeviceType::Xbox;
  }

  return System::DeviceType::Xbox;
}

void InputManager::UpdateCaptureUI() {
    InputCaptureUpdate update;
    update.actionFullName = m_capturingActionFullName;
    for (uint32_t c : m_captureRecordedCodes) {
        if (m_captureCodeToInputMap.count(c)) {
            update.currentChordInputs.push_back(m_captureCodeToInputMap[c]);
        }
    }
    m_eventManager.System.OnInputCaptureUpdate.Call(update);
}

}  // namespace Input
SPF_NS_END
