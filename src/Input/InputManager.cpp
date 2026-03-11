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
    case GamepadButton::LeftTriggerAxis:
    case GamepadButton::RightTriggerAxis:
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

static std::mutex s_axisConfigMutex;

void InputManager::SetAxisProperties(uint32_t hardwareCode, Config::ConsumptionPolicy policy, bool emulationEnabled, bool isAccumulator, bool invert, const std::string& side, float threshold, float sensitivity, float rMin, float rMax) {
    std::lock_guard<std::mutex> lock(s_axisConfigMutex);
    auto& state = m_axisStates[hardwareCode];
    state.policy = policy;
    state.emulationEnabled = emulationEnabled;
    state.isAccumulator = isAccumulator;
    state.invert = invert;
    state.side = side;
    state.threshold = threshold;
    state.sensitivity = sensitivity;
    state.rangeMin = rMin;
    state.rangeMax = rMax;

    // If emulation is disabled, remove the axis from the digital state machine
    if (!emulationEnabled) {
        m_inputStates.erase(hardwareCode);
        m_currentlyPressedHardwareCodes.erase(hardwareCode);
    }
}

void InputManager::ResetAxisProperties() {
    std::lock_guard<std::mutex> lock(s_axisConfigMutex);
    for (auto const& [code, state] : m_axisStates) {
        m_inputStates.erase(code);
        m_currentlyPressedHardwareCodes.erase(code);
    }
    m_axisStates.clear();
    m_accumulatorTargets.clear();
}

void InputManager::Initialize() {
  m_lastFrameTimestamp = std::chrono::steady_clock::now();
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

bool InputManager::PublishMouseButton(const MouseButtonEvent& event, uint8_t priority) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(event.iButton);
  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) return state.blockInput;
  state.lastPriority = priority; state.lastTimestamp = now;

  if (event.bPressed) {
      m_currentlyPressedHardwareCodes.insert(hardwareCode);
  } else {
      m_currentlyPressedHardwareCodes.erase(hardwareCode);
  }

  if (m_capturedHardwareCodesThisFrame.count(hardwareCode)) {
      return true;
  }

  bool shouldBlock = ProcessAndDecide(event, priority);

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

bool InputManager::PublishMouseWheel(const MouseWheelEvent& event, uint8_t priority) {
  // New logic: Mouse wheel is now treated as an axis
  bool blocked = PublishAxisMove(0x03, 2, event.delta, priority);

  if (blocked) return true;

  for (auto it = m_consumers.rbegin(); it != m_consumers.rend(); ++it) {
    if ((*it)->OnMouseWheel(event)) {
      // Event was consumed, stop propagation
      return true;
    }
  }
  return false;
}

bool InputManager::PublishAxisMove(uint8_t deviceType, int axisIndex, float value, uint8_t priority) {
    uint32_t hardwareCode = (static_cast<uint32_t>(deviceType) << 24) | 0x00010000 | static_cast<uint32_t>(axisIndex);
    
    auto& state = m_axisStates[hardwareCode];
    uint64_t now = GetTickCount64();
    
    // Priority and Anti-Bounce Logic:
    // 1. If a higher priority source (lower number) is active, ignore this lower priority update.
    // 2. If it's the same priority, allow update but apply 100ms anti-bounce if needed.
    bool isLowerPriority = (priority > state.lastPriority);
    bool isRecentlyUpdated = (now - state.lastTimestamp < 500); // 500ms window for priority lock

    if (isLowerPriority && isRecentlyUpdated) {
        return IsAxisConsumed(deviceType, axisIndex);
    }

    state.lastPriority = priority;
    state.lastTimestamp = now;

    if (state.isAccumulator) {
        if (deviceType == 0x03) { // Mouse (Relative)
            if (m_activeAxisValues.find(hardwareCode) == m_activeAxisValues.end()) {
                state.value = (state.rangeMin + state.rangeMax) * 0.5f;
            }
            state.value += (value * state.sensitivity);
            state.value = std::clamp(state.value, state.rangeMin, state.rangeMax);
            m_activeAxisValues[hardwareCode] = state.value;
        } else { // Gamepad/Joystick (Absolute) in Accumulator mode
            // Apply noise deadzone (0.05) before storing as target speed
            float speedToStore = value;
            if (std::abs(speedToStore) < 0.05f) speedToStore = 0.0f;
            m_accumulatorTargets[hardwareCode] = speedToStore;
        }
    } else {
        state.value = value;
        m_activeAxisValues[hardwareCode] = state.value;
    }

    // --- Capture Logic ---
    if (m_captureState == InputCaptureState::Capturing) {
        float startValue = 0.0f;
        if (m_captureInitialAxisValues.count(hardwareCode)) {
            startValue = m_captureInitialAxisValues[hardwareCode];
        }

        // Capture only if the axis has moved significantly from its starting position
        if (std::abs(value - startValue) > 0.5f) {
            bool captured = false;
            std::shared_ptr<Modules::IBindableInput> inputObj;

            if (deviceType == 0x02) { // Gamepad
                GamepadButton btn = GamepadButton::Unknown;
                switch (axisIndex) {
                    case 0: btn = GamepadButton::LeftStickX; break;
                    case 1: btn = GamepadButton::LeftStickY; break;
                    case 2: btn = GamepadButton::RightStickX; break;
                    case 3: btn = GamepadButton::RightStickY; break;
                    case 4: btn = GamepadButton::LeftTriggerAxis; break;
                    case 5: btn = GamepadButton::RightTriggerAxis; break;
                }
                
                inputObj = std::make_shared<Modules::GamepadAxisInput>(nlohmann::ordered_json{{"type", "gamepad_axis"}, {"key", GamepadButtonMapping::GetInstance().GetButtonName(btn)}});
                captured = true;
            } else if (deviceType == 0x03) { // Mouse
                if (axisIndex == 2) { // ONLY Scroll Wheel
                    inputObj = std::make_shared<Modules::MouseAxisInput>(nlohmann::ordered_json{{"type", "mouse_axis"}, {"key", std::to_string(axisIndex)}});
                    captured = true;
                }
            } else if (deviceType == 0x04) { // Joystick
                inputObj = std::make_shared<Modules::JoystickAxisInput>(nlohmann::ordered_json{{"type", "joystick_axis"}, {"key", std::to_string(axisIndex)}});
                captured = true;
            }
            
            if (captured) {               
                // FOR AXES: We finalize IMMEDIATELY. No chords allowed.
                m_captureState = InputCaptureState::Idle;
                InputCaptured captured_data{inputObj, m_capturingActionFullName, m_capturingOriginalBinding};
                m_eventManager.System.OnInputCaptured.Call(captured_data);
                
                // Determine if we should block this specific event even during capture
                return (state.policy == Config::ConsumptionPolicy::Always);
            }
        }
    }

    // --- Digital Emulation ---
    if (m_captureState != InputCaptureState::Capturing && state.emulationEnabled && deviceType != 0x03) {
        float absVal = std::abs(value);
        bool wasPressed = m_currentlyPressedHardwareCodes.count(hardwareCode) > 0;
        
        // Relative Hysteresis logic
        bool isPressed = wasPressed ? (absVal >= (state.threshold * 0.5f)) : (absVal >= state.threshold);       

        if (isPressed) m_currentlyPressedHardwareCodes.insert(hardwareCode);
        else m_currentlyPressedHardwareCodes.erase(hardwareCode);

        HandleInputState(hardwareCode, isPressed, value, m_inputStates[hardwareCode]);
    }

    // --- Autonomous Blocking Logic ---
    bool shouldBlock = false;
    if (state.policy == Config::ConsumptionPolicy::Always) {
        shouldBlock = true;
    } else if (state.policy == Config::ConsumptionPolicy::OnUIFocus) {
        // Block if UI has control
        if (deviceType == 0x03) { // Mouse
            if (axisIndex == 2) shouldBlock = !ShouldGameControlMouseWheel();
            else shouldBlock = !ShouldGameControlMouseAxes();
        } else {
            // For others (gamepad), block if any UI is focusing buttons
            shouldBlock = !ShouldGameControlMouseButtons(); 
        }
    } else if (state.policy == Config::ConsumptionPolicy::Manual) {
        // Manual policy uses the pre-existing blockInput flag in m_inputStates
        shouldBlock = m_inputStates[hardwareCode].blockInput;
    }

    return shouldBlock;
}

bool InputManager::IsAxisConsumed(uint8_t deviceType, int axisIndex) const {
    uint32_t hardwareCode = (static_cast<uint32_t>(deviceType) << 24) | 0x00010000 | static_cast<uint32_t>(axisIndex);
    
    auto it = m_axisStates.find(hardwareCode);
    if (it == m_axisStates.end()) return false;

    const auto& state = it->second;

    if (state.policy == Config::ConsumptionPolicy::Always) return true;
    if (state.policy == Config::ConsumptionPolicy::OnUIFocus) {
        if (deviceType == 0x03) {
            if (axisIndex == 2) return !ShouldGameControlMouseWheel();
            return !ShouldGameControlMouseAxes();
        }
        return !ShouldGameControlMouseButtons();
    }
    if (state.policy == Config::ConsumptionPolicy::Manual) {
        auto itState = m_inputStates.find(hardwareCode);
        return (itState != m_inputStates.end()) ? itState->second.blockInput : false;
    }

    return false;
}

bool InputManager::IsAxisAccumulator(uint32_t hardwareCode) const {
    auto it = m_axisStates.find(hardwareCode);
    return (it != m_axisStates.end()) ? it->second.isAccumulator : false;
}

bool InputManager::PublishKeyboardEvent(const KeyboardEvent& event, uint8_t priority) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(event.key);
  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) return state.blockInput;
  state.lastPriority = priority; state.lastTimestamp = now;

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
    if (event.key == System::Keyboard::Unknown) return true; // Ignore unknown keys during capture

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

  // If a consumer (like ImGui) handled the event, we block it from the game
  // and stop propagation to our own keybinds system to prevent accidental triggers.
  if (consumedByConsumer && event.key != System::Keyboard::Escape) {
    state.blockInput = event.pressed;
    state.isDown = event.pressed;
    state.wasDown = event.pressed;
    return true;
  }

  // If not consumed by UI, then process for keybinds and decide on blocking.
  return ProcessAndDecide(event, priority);
}

bool InputManager::PublishGamepadEvent(const GamepadEvent& event, uint8_t priority) {
  // Update currently pressed hardware codes for chords (only for digital buttons)
  if (!IsAxis(event.button)) {
      uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);
      
      auto& state = m_inputStates[hardwareCode];
      uint64_t now = GetTickCount64();
      if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) {
          return state.blockInput;
      }
      state.lastPriority = priority;
      state.lastTimestamp = now;

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
  bool shouldBlock = ProcessAndDecide(event, priority);

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

bool InputManager::PublishJoystickEvent(const JoystickEvent& event, uint8_t priority) {
  // Update currently pressed hardware codes for chords
  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(event.buttonIndex);
  
  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) {
      return state.blockInput;
  }
  state.lastPriority = priority;
  state.lastTimestamp = now;

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
  bool shouldBlock = ProcessAndDecide(event, priority);

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
  auto now = std::chrono::steady_clock::now();
  float dt = std::chrono::duration<float>(now - m_lastFrameTimestamp).count();
  m_lastFrameTimestamp = now;

  if (!m_inPostCaptureCooldown) {
    // --- Accumulator Integration ---
    for (auto& [code, rawSpeed] : m_accumulatorTargets) {
        auto& state = m_axisStates[code];
        if (state.isAccumulator) {
            float speed = rawSpeed;
            
            // 1. Noise Deadzone (0.01) - ignore tiny stick drift
            if (std::abs(speed) < 0.01f) speed = 0.0f;
            
            // 2. Invert (apply to speed direction)
            if (state.invert) speed *= -1.0f;

            // Integration: value += speed * sensitivity * deltaTime
            state.value += (speed * state.sensitivity * dt);
            
            // 3. Side-aware Clamping
            float effectiveMin = state.rangeMin;
            float effectiveMax = state.rangeMax;
            
            if (state.side == "positive") {
                effectiveMin = (state.rangeMin < 0.0f) ? 0.0f : state.rangeMin;
            } else if (state.side == "negative") {
                effectiveMax = (state.rangeMax > 0.0f) ? 0.0f : state.rangeMax;
            }

            state.value = std::clamp(state.value, effectiveMin, effectiveMax);
            m_activeAxisValues[code] = state.value;
        }
    }

    for (auto& pair : m_inputStates) {
      uint8_t type = (pair.first >> 24) & 0xFF;
      if (type == 0x02) { // Gamepad (Button or Axis)
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    uint8_t type = (pair.first >> 24) & 0xFF;
    if (type == 0x02) pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::HandleRetroactiveBlocking(uint32_t hardwareCode, bool shouldBlock) {
    uint8_t type = (hardwareCode >> 24) & 0xFF;
    uint32_t rawCode = hardwareCode & 0x0000FFFF; // Index is in lower 16 bits
    
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
                      logger->Info("[Capture] Finalizing chord with {} keys", m_captureRecordedCodes.size());
                      auto chord = std::make_shared<Modules::ChordInput>();
                      for (uint32_t c : m_captureRecordedCodes) {
                          auto input = m_captureCodeToInputMap[c];
                          if (input && input->IsValid()) {
                              std::string dn = input->GetDisplayName();
                              if (dn.find("Unknown") == std::string::npos && dn.find("UNKNOWN") == std::string::npos) {
                                  chord->AddInput(Modules::InputFactory::CreateFromJson(input->ToJson()));
                              }
                          }
                      }
                      result = chord;
                  }

                  logger->Info("[Capture] SUCCESS: Finalized capture: {}", result->GetDisplayName());
                  InputCaptured captured_data{result, m_capturingActionFullName, m_capturingOriginalBinding};
                  m_eventManager.System.OnInputCaptured.Call(captured_data);
                  m_captureState = InputCaptureState::Idle;
              } else {
                  logger->Warn("[Capture] Timer expired but no keys were ever recorded!");
                  m_captureState = InputCaptureState::Idle;
              }
          } else {
              // Some keys still held - TRIMMING
              logger->Info("[Capture] Some keys still held, trimming recorded chord to current state.");
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
      uint8_t type = (pair.first >> 24) & 0xFF;
      if (type == 0x01) { // Keyboard
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    uint8_t type = (pair.first >> 24) & 0xFF;
    if (type == 0x01) pair.second.wasDown = pair.second.isDown;
  }

  // Reset the cooldown flag at the very end of all processing
  if (m_inPostCaptureCooldown) {
    m_inPostCaptureCooldown = false;
  }

  // Reset frame-specific blacklists at the end of all processing.
  m_capturedHardwareCodesThisFrame.clear();
  m_minPriorityCapturedThisFrame = 255;
  m_lastCaptureTimestamp = 0;
}

void InputManager::ProcessMouseActions() {
  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      uint8_t type = (pair.first >> 24) & 0xFF;
      if (type == 0x03) { // Mouse
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    uint8_t type = (pair.first >> 24) & 0xFF;
    if (type == 0x03) pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::ProcessJoystickActions() {
  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_inputStates) {
      uint8_t type = (pair.first >> 24) & 0xFF;
      if (type == 0x04) { // Joystick
          EvaluateActionLogic(pair.first, pair.second);
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_inputStates) {
    uint8_t type = (pair.first >> 24) & 0xFF;
    if (type == 0x04) pair.second.wasDown = pair.second.isDown;
  }
}

bool InputManager::ProcessAndDecide(const GamepadEvent& event, uint8_t priority) {
  if (event.button == System::GamepadButton::Unknown) return true; // Ignore unknown gamepad buttons

  uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);
  
  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) {
      return state.blockInput;
  }
  state.lastPriority = priority;
  state.lastTimestamp = now;

  bool isAxis = IsAxis(event.button);

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  if (m_captureState == InputCaptureState::Capturing) {
    uint64_t now_ms = GetTickCount64();
    
    // Cross-API Filtering: If we already captured a higher priority event very recently, ignore this one.
    // This handles emulated controllers that spam both XInput and DInput.
    if (priority > m_minPriorityCapturedThisFrame && (now_ms - m_lastCaptureTimestamp < 50)) {
        return true; // Consume but ignore for recording
    }

    bool isTriggered = false;
    if (isAxis) {
        // For axes during capture, we require a significant displacement (threshold)
        // to avoid accidental capture from stick drift.
        isTriggered = std::abs(event.value) > 0.5f;
    } else {
        isTriggered = event.pressed;
    }

    if (isTriggered) {
        // Architectural fix: During capture, we prefer axes for triggers.
        // If this is a digital "button" event for a trigger, ignore it to let axis capture win.
        if (!isAxis && (event.button == System::GamepadButton::LeftTrigger || event.button == System::GamepadButton::RightTrigger)) {
            return true;
        }

        // Optimization: If this code is already held, ignore repeated polling events
        // during capture to avoid resetting the chord finalization timer.
        if (m_captureHeldCodes.count(hardwareCode)) {
            return true;
        }
        
        m_captureHeldCodes.insert(hardwareCode);
        m_captureRecordedCodes.insert(hardwareCode);
        m_minPriorityCapturedThisFrame = priority;
        m_lastCaptureTimestamp = now_ms;
        
        if (isAxis) {
            m_captureCodeToInputMap[hardwareCode] = std::make_shared<Modules::GamepadAxisInput>(nlohmann::ordered_json{{"type", "gamepad_axis"}, {"key", GamepadButtonMapping::GetInstance().GetButtonName(event.button)}});
        } else {
            m_captureCodeToInputMap[hardwareCode] = std::make_shared<Modules::GamepadInput>(nlohmann::ordered_json{{"type", "gamepad"}, {"key", GamepadButtonMapping::GetInstance().GetButtonName(event.button)}});
        }
        m_isWaitingForCaptureFinalize = false; 
    } else if (m_captureHeldCodes.count(hardwareCode)) {
        m_captureHeldCodes.erase(hardwareCode);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true; 
    }

    UpdateCaptureUI();

    // Reset the state in the unified map to prevent actions during capture
    auto& state = m_inputStates[hardwareCode];
    state.isDown = false;
    state.wasDown = false;
    state.longPressTriggered = false;
    return true;  // Consume the event during capture
  }

  // Handle axes separately from buttons for normal gameplay (pass through)
  if (isAxis) {
      return false; 
  }

  // Delegate to unified handler for buttons
  return HandleInputState(hardwareCode, event.pressed, event.value, m_inputStates[hardwareCode]);
}

bool InputManager::IsGamepadButtonBlocked(System::GamepadButton button) const {
  uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(button);
  auto it = m_inputStates.find(hardwareCode);
  return (it != m_inputStates.end()) ? it->second.blockInput : false;
}

bool InputManager::IsKeyboardCaptured() const {
  for (auto* consumer : m_consumers) {
    if (consumer->IsCapturingKeyboard()) return true;
  }
  return false;
}

bool InputManager::IsMouseCaptured() const {
  for (auto* consumer : m_consumers) {
    if (consumer->IsCapturingMouse()) return true;
  }
  return false;
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

    // if (wasDown != state.isDown) {
    //     auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    //     logger->Info("[HandleInputState] Code {:#010x} transition: {} -> {}", 
    //         hardwareCode, wasDown ? "Down" : "Up", state.isDown ? "Down" : "Up");
    // }

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

        // Determine initial block policy.
        Config::ConsumptionPolicy policy = Config::ConsumptionPolicy::Never;
        
        if (bestShort) {
             policy = bestShort->Policy;
        }

        // If there's a long press binding for a single key (not a chord) that requires blocking,
        // we should block immediately.
        if (bestLong && bestLong->Policy > policy) {
            bool isLongPressChord = false;
            if (auto* chord = dynamic_cast<const Modules::ChordInput*>(bestLong->Input.get())) {
                isLongPressChord = true;
            }
            
            if (!isLongPressChord) {
                policy = bestLong->Policy;
            }
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

bool InputManager::ProcessAndDecide(const MouseButtonEvent& event, uint8_t priority) {
  auto button = static_cast<MouseButton>(event.iButton);
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  
  if (event.iButton < 0 || event.iButton > 7 || button == MouseButton::Unknown) return true; // Ignore invalid button indices

  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(button);

  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) return state.blockInput;
  state.lastPriority = priority; state.lastTimestamp = now;

  if (m_captureState == InputCaptureState::Capturing) {
    // In capture mode, we handle input differently.

    // Per user request, NEVER capture the left mouse button. Let it pass through to the UI.
    if (button == MouseButton::Left) {
      // logger->Trace("Ignoring Left Mouse Button during input capture to allow UI interaction.");
      return false;
    }

    if (event.bPressed) {
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

bool InputManager::ProcessAndDecide(const JoystickEvent& event, uint8_t priority) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto buttonIndex = event.buttonIndex;

  if (buttonIndex < 0 || buttonIndex >= 128) return true; // Ignore invalid joystick buttons

  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(buttonIndex);

  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) return state.blockInput;
  state.lastPriority = priority; state.lastTimestamp = now;

  if (m_captureState == InputCaptureState::Capturing) {
    if (event.pressed) {
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

bool InputManager::ProcessAndDecide(const KeyboardEvent& event, uint8_t priority) {
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(event.key);
  auto& state = m_inputStates[hardwareCode];
  uint64_t now = GetTickCount64();
  if (priority > state.lastPriority && (now - state.lastTimestamp < 100)) return state.blockInput;
  state.lastPriority = priority; state.lastTimestamp = now;

  // Keyboard has no analog value, so passing 0.0f/1.0f
  // Logic for capture is handled in PublishKeyboardEvent, so here we only do State/Blocking
  return HandleInputState(hardwareCode, event.pressed, event.pressed ? 1.0f : 0.0f, state);
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
  m_captureInitialAxisValues = m_activeAxisValues; // Snapshot!
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
            long long thresholdMs = (long long)longPressThreshold.count();


            if (pressedDuration.count() >= thresholdMs) {
                if (longPressBinding) {
                    state.longPressTriggered = true;
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
                            
                            // For axes, we must also update the policy in m_axisStates
                            if ((code >> 16) & 0x01) {
                                std::lock_guard<std::mutex> lock(s_axisConfigMutex);
                                m_axisStates[code].policy = policy;
                            }
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
            auto input = m_captureCodeToInputMap[c];
            if (input && input->IsValid()) {
                std::string displayName = input->GetDisplayName();
                // Filter out any "Unknown" display names that might have leaked through
                if (displayName.find("Unknown") == std::string::npos && 
                    displayName.find("UNKNOWN") == std::string::npos) {
                    update.currentChordInputs.push_back(input);
                }
            }
        }
    }
    m_eventManager.System.OnInputCaptureUpdate.Call(update);
}

}  // namespace Input
SPF_NS_END
