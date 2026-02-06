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
#include "SPF/Modules/JoystickInput.hpp"  // New include for JoystickInput
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

  if (m_capturedKeyThisFrame.has_value() && m_capturedKeyThisFrame.value() == event.key) {
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
    m_keyboardStates[event.key].isDown = false;
    m_keyboardStates[event.key].wasDown = false;
    m_keyboardStates[event.key].longPressTriggered = false;
    
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

  // This function now mirrors PublishKeyboardEvent. It's the main entry point from hooks.
  // It orchestrates deciding, blocking, and notifying consumers.

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
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto now = std::chrono::steady_clock::now();
  auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();

  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_buttonStates) {
      auto button = pair.first;
      auto& state = pair.second;

      // --- Detect and Handle Release (for Short Press) ---
      if (!state.isDown && state.wasDown) {
        auto it = m_heldGamepadButtons.find(button);
        if (it != m_heldGamepadButtons.end()) {
          PressType originalPressType = it->second;
          keyBindsManager.TriggerAction(button, originalPressType);
          m_heldGamepadButtons.erase(it);
        } else if (!state.longPressTriggered) {
          // logger->Info("Short press action triggered for button: {}", (int)button);
          keyBindsManager.TriggerAction(button, PressType::Short);
        }
      }
      // --- Detect and Handle Hold (for Long Press) ---
      else if (state.isDown && state.wasDown) {
        if (!state.longPressTriggered) {
          uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(button);
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

                    keyBindsManager.TriggerAction(button, PressType::Long);

                    // Update block policy based on long press action
                    GamepadEvent longPressEvent{ 0, button, true, 1.0f, PressType::Long };
                    auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent, PressType::Long);
                    bool shouldBlock = false;
                    switch (policy) {
                        case Config::ConsumptionPolicy::Always: shouldBlock = true; break;
                        case Config::ConsumptionPolicy::OnUIFocus: shouldBlock = !m_gameControlsMouseButtons; break;
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
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_buttonStates) {
    pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::HandleRetroactiveBlocking(uint32_t hardwareCode, bool shouldBlock) {
    // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    // logger->Trace("HandleRetroactiveBlocking: code={:#08x}, shouldBlock={}", hardwareCode, shouldBlock);

    uint8_t type = (hardwareCode >> 24) & 0xFF;
    uint32_t rawCode = hardwareCode & 0x00FFFFFF;
    
    // 1. Update internal block state
    if (type == 0x01) { // Keyboard
        auto it = m_keyboardStates.find(static_cast<System::Keyboard>(rawCode));
        if (it != m_keyboardStates.end()) it->second.blockInput = shouldBlock;
    } else if (type == 0x02) { // Gamepad
        auto it = m_buttonStates.find(static_cast<System::GamepadButton>(rawCode));
        if (it != m_buttonStates.end()) it->second.blockInput = shouldBlock;
    } else if (type == 0x03) { // Mouse
        auto it = m_mouseButtonStates.find(static_cast<System::MouseButton>(rawCode));
        if (it != m_mouseButtonStates.end()) it->second.blockInput = shouldBlock;
    } else if (type == 0x04) { // Joystick
        auto it = m_joystickButtonStates.find(static_cast<int>(rawCode));
        if (it != m_joystickButtonStates.end()) it->second.blockInput = shouldBlock;
    }

    // 2. Handle retroactive release if needed
    if (shouldBlock && m_keysLeakedToGame.count(hardwareCode)) {
        if (type == 0x01) { // Keyboard
            Hooks::User32Hook::SendVirtualKeyRelease(hardwareCode);
            m_pendingVirtualReleases.insert(hardwareCode);
        } else if (type == 0x03) { // Mouse
            // Instead of SendInput, we queue a release for DInput8Hook to inject into the game's buffer
            // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
            // logger->Trace("Queueing virtual mouse release for DInput8 injection: button={}", rawCode);
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
    for (auto& pair : m_keyboardStates) {
      auto key = pair.first;
      auto& state = pair.second;

      // --- Detect and Handle Release (for Short Press) ---
      if (!state.isDown && state.wasDown) {
        auto it = m_heldKeyboardKeys.find(key);
        if (it != m_heldKeyboardKeys.end()) {
          PressType originalPressType = it->second;
          keyBindsManager.TriggerAction(key, originalPressType);
          m_heldKeyboardKeys.erase(it);
        } else if (!state.longPressTriggered) {
          // logger->Info("Short press action triggered for key: {}", (int)key);
          keyBindsManager.TriggerAction(key, PressType::Short);
        }
      }
      // --- Detect and Handle Hold (for Long Press) ---
      else if (state.isDown && state.wasDown) {
        if (!state.longPressTriggered) {
          uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(key);
          const auto* longPressBinding = keyBindsManager.FindBestBinding(hardwareCode, PressType::Long);
          const auto* shortPressBinding = keyBindsManager.FindBestBinding(hardwareCode, PressType::Short);

          // Determine the 'active' start time and threshold
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

            // Trigger LONG action only if it exists and this key is the 'lead' for the chord
            // (Lead = highest hardware code in the chord, to avoid double triggers)
            if (longPressBinding) {
                bool isLead = true;
                std::vector<uint32_t> keysToBlock;
                keysToBlock.push_back(hardwareCode); // Default to blocking self

                if (auto* chord = dynamic_cast<const Modules::ChordInput*>(longPressBinding->Input.get())) {
                    auto codes = chord->GetConstituentHardwareCodes();
                    uint32_t maxCode = 0;
                    for (uint32_t c : codes) if (c > maxCode) maxCode = c;
                    if (hardwareCode != maxCode) isLead = false;
                    
                    if (isLead) keysToBlock = codes; // If we are lead, handle blocking for ALL keys
                }

                if (isLead) {
                    keyBindsManager.TriggerAction(key, PressType::Long);

                    // Check for Hold behavior
                    if (longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
                        for (uint32_t code : keysToBlock) {
                            SetHoldState(code, PressType::Long);
                        }
                    }

                    // Update block policy
                    KeyboardEvent longPressEvent{ key, true, PressType::Long };
                    auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent, PressType::Long);
                    bool shouldBlock = false;
                    switch (policy) {
                        case Config::ConsumptionPolicy::Always: shouldBlock = true; break;
                        case Config::ConsumptionPolicy::OnUIFocus: shouldBlock = !m_gameControlsMouseButtons; break;
                        default: shouldBlock = false; break;
                    }

                    // Apply blocking to ALL keys in the chord (retroactively if needed)
                    for (uint32_t code : keysToBlock) {
                        HandleRetroactiveBlocking(code, shouldBlock);
                    }
                }
            }
          }
        }
      }
    }
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_keyboardStates) {
    pair.second.wasDown = pair.second.isDown;
  }

  // Reset the cooldown flag at the very end of all processing
  if (m_inPostCaptureCooldown) {
    m_inPostCaptureCooldown = false;
  }

  // Reset frame-specific blacklists at the end of all processing.
  m_capturedButtonThisFrame.reset();
  m_capturedKeyThisFrame.reset();
  m_capturedMouseButtonThisFrame.reset();
  m_capturedJoystickButtonThisFrame.reset();
}

void InputManager::ProcessMouseActions() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto now = std::chrono::steady_clock::now();
  auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();

  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_mouseButtonStates) {
      auto button = pair.first;
      auto& state = pair.second;

      // --- Detect and Handle Release (for Short Press) ---
      if (!state.isDown && state.wasDown) {
        auto it = m_heldMouseButtons.find(button);
        if (it != m_heldMouseButtons.end()) {
          PressType originalPressType = it->second;
          keyBindsManager.TriggerAction(button, originalPressType);
          m_heldMouseButtons.erase(it);
        } else if (!state.longPressTriggered) {
          keyBindsManager.TriggerAction(button, PressType::Short);
        }
      }
      // --- Detect and Handle Hold (for Long Press) ---
      else if (state.isDown && state.wasDown) {
        if (!state.longPressTriggered) {
          uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(button);
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
                    if (longPressBinding && longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
                        for (uint32_t code : keysToBlock) {
                            SetHoldState(code, PressType::Long);
                        }
                    }

                                keyBindsManager.TriggerAction(button, PressType::Long);

                                MouseButtonEvent longPressEvent{(int)button, true, PressType::Long};

                                auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent, PressType::Long);
                    bool shouldBlock = false;
                    switch (policy) {
                        case Config::ConsumptionPolicy::Always: shouldBlock = true; break;
                        case Config::ConsumptionPolicy::OnUIFocus: shouldBlock = !m_gameControlsMouseButtons; break;
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
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_mouseButtonStates) {
    pair.second.wasDown = pair.second.isDown;
  }
}

void InputManager::ProcessJoystickActions() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto now = std::chrono::steady_clock::now();
  auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();

  if (!m_inPostCaptureCooldown) {
    for (auto& pair : m_joystickButtonStates) {
      auto buttonIndex = pair.first;
      auto& state = pair.second;

      // --- Detect and Handle Release (for Short Press) ---
      if (!state.isDown && state.wasDown) {
        auto it = m_heldJoystickButtons.find(buttonIndex);
        if (it != m_heldJoystickButtons.end()) {
          PressType originalPressType = it->second;
          keyBindsManager.TriggerAction(buttonIndex, originalPressType);
          m_heldJoystickButtons.erase(it);
        } else if (!state.longPressTriggered) {
          keyBindsManager.TriggerAction(buttonIndex, PressType::Short);
        }
      }
      // --- Detect and Handle Hold (for Long Press) ---
      else if (state.isDown && state.wasDown) {
        if (!state.longPressTriggered) {
          uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(buttonIndex);
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

                    keyBindsManager.TriggerAction(buttonIndex, PressType::Long);

                    // Update block policy based on long press action
                    JoystickEvent longPressEvent{buttonIndex, true, PressType::Long};
                    auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent, PressType::Long);
                    bool shouldBlock = false;
                    switch (policy) {
                        case Config::ConsumptionPolicy::Always: shouldBlock = true; break;
                        case Config::ConsumptionPolicy::OnUIFocus: shouldBlock = !m_gameControlsMouseButtons; break;
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
  }

  // Sync states for the next frame's logic
  for (auto& pair : m_joystickButtonStates) {
    pair.second.wasDown = pair.second.isDown;
  }
}

bool InputManager::ProcessAndDecide(const GamepadEvent& event) {
  if (m_capturedButtonThisFrame.has_value() && m_capturedButtonThisFrame.value() == event.button) {
    return true;  // Consume event from the second hook
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  if (m_captureState == InputCaptureState::Capturing && !IsAxis(event.button)) {
    uint32_t code = 0x02000000 | static_cast<uint32_t>(event.button);

    if (event.pressed) {
        logger->Info("Captured gamepad button for action: {}", m_capturingActionFullName);
        m_captureHeldCodes.insert(code);
        m_captureRecordedCodes.insert(code);
        m_captureCodeToInputMap[code] = std::make_shared<Modules::GamepadInput>(nlohmann::ordered_json{{"type", "gamepad"}, {"button", GamepadButtonMapping::GetInstance().GetButtonName(event.button)}});
        m_isWaitingForCaptureFinalize = false; 
    } else {
        m_captureHeldCodes.erase(code);
        m_lastCaptureReleaseTime = std::chrono::steady_clock::now();
        m_isWaitingForCaptureFinalize = true; 
    }

    UpdateCaptureUI();

    // Reset the state of the captured button to prevent immediate action trigger
    m_buttonStates[event.button].isDown = false;
    m_buttonStates[event.button].wasDown = false;
    m_buttonStates[event.button].longPressTriggered = false;
    return true;  // Consume the event
  }

  // Handle axes separately from buttons.
  if (IsAxis(event.button)) {
    // Axes are never blocked from the game, per user clarification.
    // The parent PublishGamepadEvent function handles sending the axis move to UI consumers.
    return false;
  }

  uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);

  // Track buttons leaked to game
  if (event.pressed) {
     m_keysLeakedToGame.insert(hardwareCode);
  } else {
     m_keysLeakedToGame.erase(hardwareCode);
  }

  // This function is now purely for state management and blocking decisions for buttons.
  auto& state = m_buttonStates[event.button];
  bool wasDown = state.isDown;
  state.isDown = event.pressed;

  // On new press (Up -> Down transition)
  if (!wasDown && state.isDown) {
    state.pressTimestamp = std::chrono::steady_clock::now();
    state.longPressTriggered = false;

    // --- Chord Reset Logic ---
    uint32_t hardwareCode = 0x02000000 | static_cast<uint32_t>(event.button);
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

    // Check for "Hold" behavior first
    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(event.button, PressType::Short);  // Hold is based on short press
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(event.button, PressType::Short);
      m_heldGamepadButtons[event.button] = PressType::Short;
    }

    // Determine initial block policy based on the short press action.
    GamepadEvent shortPressEvent = event;
    shortPressEvent.pressType = PressType::Short;
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent, PressType::Short);

    bool shouldBlock = false;
    switch (policy) {
      case Config::ConsumptionPolicy::Always:
        shouldBlock = true;
        break;
      case Config::ConsumptionPolicy::OnUIFocus:
        // Block if any interactive UI is visible.
        shouldBlock = !m_gameControlsMouseButtons;
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
  if (state.blockInput && event.pressed) {
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
  } else if (!event.pressed) {
      m_keysLeakedToGame.erase(hardwareCode);
      state.blockInput = false;
  }

  // For held buttons or releases, return the stored blocking decision.
  return state.blockInput;
}

bool InputManager::IsGamepadButtonBlocked(System::GamepadButton button) const {
  auto it = m_buttonStates.find(button);
  if (it != m_buttonStates.end()) {
    return it->second.blockInput;
  }
  return false;
}

bool InputManager::ProcessAndDecide(const MouseButtonEvent& event) {
  auto button = static_cast<MouseButton>(event.iButton);
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  uint32_t hardwareCode = 0x03000000 | static_cast<uint32_t>(button);
  // logger->Trace("ProcessAndDecide (Mouse): button={}, pressed={}, code={:#08x}", event.iButton, event.bPressed, hardwareCode);

  // Track buttons leaked to game
  if (event.bPressed) {
     m_keysLeakedToGame.insert(hardwareCode);
  } else {
     m_keysLeakedToGame.erase(hardwareCode);
  }

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

    // Reset state
    m_mouseButtonStates[button].isDown = false;
    m_mouseButtonStates[button].wasDown = false;
    m_mouseButtonStates[button].longPressTriggered = false;

    // CRUCIAL FIX: For any non-left button, always consume the event (press and release)
    // to prevent it from closing the ImGui capture popup.
    return true;
  }

  // --- Regular (Non-Capture) Logic ---

  auto& state = m_mouseButtonStates[button];
  bool wasDown = state.isDown;
  state.isDown = event.bPressed;

  if (!wasDown && state.isDown) {  // Press
    state.pressTimestamp = std::chrono::steady_clock::now();
    state.longPressTriggered = false;

    // --- Chord Reset Logic ---
    const auto* bestShort = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Short);
    const auto* bestLong = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Long);
    const auto* bestChord = (bestShort && dynamic_cast<const Modules::ChordInput*>(bestShort->Input.get())) ? bestShort : 
                           ((bestLong && dynamic_cast<const Modules::ChordInput*>(bestLong->Input.get())) ? bestLong : nullptr);

    if (bestChord) {
        if (auto* chord = dynamic_cast<const Modules::ChordInput*>(bestChord->Input.get())) {
            for (uint32_t hardwareCode : chord->GetConstituentHardwareCodes()) {
                ResetStateForCode(hardwareCode);
            }
        }
    }

    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(button, PressType::Short);
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(button, PressType::Short);
      m_heldMouseButtons[button] = PressType::Short;
    }

    MouseButtonEvent shortPressEvent = {event.iButton, event.bPressed, PressType::Short};
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent, PressType::Short);

    bool shouldBlock = false;
    switch (policy) {
      case Config::ConsumptionPolicy::Always:
        shouldBlock = true;
        break;
      case Config::ConsumptionPolicy::OnUIFocus:
        shouldBlock = !m_gameControlsMouseButtons;
        break;
      default:
        shouldBlock = false;
        break;
    }
    state.blockInput = shouldBlock;
  } else if (wasDown && !state.isDown) {  // Release
    state.blockInput = false;
  }

  // --- Retroactive Blocking Logic ---
  if (state.blockInput && event.bPressed) {
      m_keysLeakedToGame.erase(hardwareCode); // We are blocking this one

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
  } else if (state.blockInput) {
      m_keysLeakedToGame.erase(hardwareCode);
  }

  return state.blockInput;
}

bool InputManager::ProcessAndDecide(const JoystickEvent& event) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto buttonIndex = event.buttonIndex;
  uint32_t hardwareCode = 0x04000000 | static_cast<uint32_t>(buttonIndex);

  if (m_capturedJoystickButtonThisFrame.has_value() && m_capturedJoystickButtonThisFrame.value() == buttonIndex) {
    return true;  // Consume event from a potential second hook
  }

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

    // Reset state
    m_joystickButtonStates[buttonIndex].isDown = false;
    m_joystickButtonStates[buttonIndex].wasDown = false;
    m_joystickButtonStates[buttonIndex].longPressTriggered = false;

    // Always consume joystick events in capture mode (press and release)
    return true;
  }

  // --- Regular (Non-Capture) Logic ---

  auto& state = m_joystickButtonStates[buttonIndex];
  bool wasDown = state.isDown;
  state.isDown = event.pressed;

  if (!wasDown && state.isDown) {  // Press
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

    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(buttonIndex, PressType::Short);
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(buttonIndex, PressType::Short);
      m_heldJoystickButtons[buttonIndex] = PressType::Short;
    }

    JoystickEvent shortPressEvent = {buttonIndex, event.pressed, PressType::Short};
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent, PressType::Short);

    bool shouldBlock = false;
    switch (policy) {
      case Config::ConsumptionPolicy::Always:
        shouldBlock = true;
        break;
      case Config::ConsumptionPolicy::OnUIFocus:
        shouldBlock = !m_gameControlsMouseButtons;  // Assuming joystick buttons might need similar blocking
        break;
      default:
        shouldBlock = false;
        break;
    }
    
    // Always reset block state to the current policy on a new physical press.
    // This prevents the button from being "stuck" in a blocked state from a previous interaction.
    // if (state.blockInput != shouldBlock) {
    //     logger->Trace("Joystick button {} block state changed: {} -> {} (Press)", buttonIndex, state.blockInput, shouldBlock);
    // }
    state.blockInput = shouldBlock;

    if (!state.blockInput) {
        m_keysLeakedToGame.insert(hardwareCode);
    }
  } else if (wasDown && !state.isDown) {  // Release
    m_keysLeakedToGame.erase(hardwareCode);
    m_pendingJoystickReleases.erase(buttonIndex);
    m_pendingVirtualReleases.erase(hardwareCode);
    
    bool finalBlockDecision = state.blockInput;
    
    // Always clear the block state on physical release, because the button is now up.
    // if (state.blockInput) {
    //     logger->Trace("Joystick button {} block state reset to false (Release)", buttonIndex);
    // }
    state.blockInput = false;
    
    return finalBlockDecision;
  }

  // --- Retroactive Blocking Logic ---
  if (state.blockInput && event.pressed) {
      // Immediate retroactive block if we are pressed but still marked as leaked
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
  } else if (!event.pressed) {
      m_keysLeakedToGame.erase(hardwareCode);
      state.blockInput = false; // Ensure block is cleared for any non-press event
  }

  return state.blockInput;
}

void InputManager::ResetStateForCode(uint32_t code) {
    uint8_t type = (code >> 24) & 0xFF;
    uint32_t raw = code & 0x00FFFFFF;
    if (type == 0x01) {
        auto it = m_keyboardStates.find(static_cast<System::Keyboard>(raw));
        if (it != m_keyboardStates.end()) {
            it->second.longPressTriggered = false;
            it->second.blockInput = false;
        }
    } else if (type == 0x02) {
        auto it = m_buttonStates.find(static_cast<System::GamepadButton>(raw));
        if (it != m_buttonStates.end()) {
            it->second.longPressTriggered = false;
            it->second.blockInput = false;
        }
    } else if (type == 0x03) {
        auto it = m_mouseButtonStates.find(static_cast<System::MouseButton>(raw));
        if (it != m_mouseButtonStates.end()) {
            it->second.longPressTriggered = false;
            it->second.blockInput = false;
        }
    } else if (type == 0x04) {
        auto it = m_joystickButtonStates.find(static_cast<int>(raw));
        if (it != m_joystickButtonStates.end()) {
            it->second.longPressTriggered = false;
            it->second.blockInput = false;
        }
    }
}

bool InputManager::ProcessAndDecide(const KeyboardEvent& event) {
  // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  // logger->Trace("ProcessAndDecide (Keyboard): key={}, pressed={}", (int)event.key, event.pressed);
  uint32_t hardwareCode = 0x01000000 | static_cast<uint32_t>(event.key);

  // Track keys leaked to game
  if (event.pressed) {
     m_keysLeakedToGame.insert(hardwareCode);
  } else {
     m_keysLeakedToGame.erase(hardwareCode);
  }

  auto& state = m_keyboardStates[event.key];
  bool wasDown = state.isDown;
  state.isDown = event.pressed;

  // On new press (Up -> Down transition)
  if (!wasDown && state.isDown) {
    state.pressTimestamp = std::chrono::steady_clock::now();
    state.longPressTriggered = false;

    // --- Chord Reset Logic ---
    // If this press forms an active chord, reset the 'longPressTriggered' status 
    // for all constituent keys to allow the chord to have a fresh timing window.
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

    // Check for "Hold" behavior first
    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(event.key, PressType::Short);  // Hold is based on short press
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(event.key, PressType::Short);
      m_heldKeyboardKeys[event.key] = PressType::Short;
    }

    // Determine initial block policy based on the short press action.
    KeyboardEvent shortPressEvent = event;
    shortPressEvent.pressType = PressType::Short;
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent, PressType::Short);

    bool shouldBlock = false;
    switch (policy) {
      case Config::ConsumptionPolicy::Always:
        shouldBlock = true;
        break;
      case Config::ConsumptionPolicy::OnUIFocus:
        // Block if any interactive UI is visible.
        shouldBlock = !m_gameControlsMouseButtons;
        break;
      default:
        shouldBlock = false;
        break;
    }
    state.blockInput = shouldBlock;
  }
  // On release (Down -> Up transition)
  else if (wasDown && !state.isDown) {
    // When a key is released, we never need to block the release event itself.
    // The action is triggered in ProcessKeyboardActions.
    state.blockInput = false;
  }

  // --- Retroactive Blocking Logic ---
  // If we decided to block this key press, we should check if it's part of a chord
  // and if other parts of that chord have already leaked to the game.
  if (state.blockInput && event.pressed) {
      m_keysLeakedToGame.erase(hardwareCode); // We are blocking this one, so it's not leaked.

      // Find the binding that caused this block (likely a chord)
      const auto* bestBinding = Modules::KeyBindsManager::GetInstance().FindBestBinding(hardwareCode, PressType::Short);
      if (bestBinding) {
          if (auto* chord = dynamic_cast<const Modules::ChordInput*>(bestBinding->Input.get())) {
              for (uint32_t constituentCode : chord->GetConstituentHardwareCodes()) {
                  // If a constituent key was previously leaked to the game
                  if (m_keysLeakedToGame.count(constituentCode)) {
                      HandleRetroactiveBlocking(constituentCode, true);
                  }
              }
          }
      }
  } else if (state.blockInput) {
      // If blocked but not a press (e.g. held state), ensure it's not in leaked set
      m_keysLeakedToGame.erase(hardwareCode);
  }

  // For held keys or releases, return the stored blocking decision.
  return state.blockInput;
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
  auto it = m_keyboardStates.find(key);
  if (it != m_keyboardStates.end()) {
    return it->second.blockInput;
  }
  return false;
}

bool InputManager::IsMouseButtonBlocked(System::MouseButton button) const {
  auto it = m_mouseButtonStates.find(button);
  if (it != m_mouseButtonStates.end()) {
    return it->second.blockInput;
  }
  return false;
}

bool InputManager::IsJoystickButtonBlocked(int buttonIndex) const {
  auto it = m_joystickButtonStates.find(buttonIndex);
  if (it != m_joystickButtonStates.end()) {
    return it->second.blockInput;
  }
  return false;
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
    uint8_t deviceType = (hardwareCode >> 24) & 0xFF;
    uint32_t rawCode = hardwareCode & 0x00FFFFFF;

    if (deviceType == 0x01) { // Keyboard
        m_heldKeyboardKeys[static_cast<System::Keyboard>(rawCode)] = type;
    } else if (deviceType == 0x02) { // Gamepad
        m_heldGamepadButtons[static_cast<System::GamepadButton>(rawCode)] = type;
    } else if (deviceType == 0x03) { // Mouse
        m_heldMouseButtons[static_cast<System::MouseButton>(rawCode)] = type;
    } else if (deviceType == 0x04) { // Joystick
        m_heldJoystickButtons[static_cast<int>(rawCode)] = type;
    }
}

std::chrono::steady_clock::time_point InputManager::GetChordPressTimestamp(const std::vector<uint32_t>& codes) const {
    auto maxTs = (std::chrono::steady_clock::time_point::min)();
    for (uint32_t code : codes) {
        std::chrono::steady_clock::time_point ts;
        uint8_t type = (code >> 24) & 0xFF;
        uint32_t rawCode = code & 0x00FFFFFF;

        if (type == 0x01) { // Keyboard
            auto it = m_keyboardStates.find(static_cast<System::Keyboard>(rawCode));
            if (it != m_keyboardStates.end()) ts = it->second.pressTimestamp;
        } else if (type == 0x02) { // Gamepad
            auto it = m_buttonStates.find(static_cast<System::GamepadButton>(rawCode));
            if (it != m_buttonStates.end()) ts = it->second.pressTimestamp;
        } else if (type == 0x03) { // Mouse
            auto it = m_mouseButtonStates.find(static_cast<System::MouseButton>(rawCode));
            if (it != m_mouseButtonStates.end()) ts = it->second.pressTimestamp;
        } else if (type == 0x04) { // Joystick
            auto it = m_joystickButtonStates.find(static_cast<int>(rawCode));
            if (it != m_joystickButtonStates.end()) ts = it->second.pressTimestamp;
        }

        if (ts > maxTs) maxTs = ts;
    }
    return maxTs;
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
