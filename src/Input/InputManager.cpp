#include <Windows.h>  // Pre-include for safety

#include "SPF/Input/InputManager.hpp"
#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Hooks/XInputHook.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/System/VirtualKeyMapping.hpp"
#include "SPF/System/MouseButtonMapping.hpp"
#include "SPF/Modules/KeyBindsManager.hpp"
#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Modules/KeyboardInput.hpp"
#include "SPF/Modules/GamepadInput.hpp"
#include "SPF/Modules/MouseInput.hpp"
#include "SPF/Modules/JoystickInput.hpp"  // New include for JoystickInput

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
  // Process the event through the state machine to determine blocking
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
  if (m_capturedKeyThisFrame.has_value() && m_capturedKeyThisFrame.value() == event.key) {
    return true;  // Consume event from a second hook/source
  }
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  if (m_captureState == InputCaptureState::Capturing && event.pressed) {
    auto capturedInput = std::make_shared<Modules::KeyboardInput>(nlohmann::json{{"type", "keyboard"}, {"key", VirtualKeyMapping::GetInstance().GetKeyName(event.key)}});

    auto conflictingAction = Modules::KeyBindsManager::GetInstance().GetActionBoundToInput(*capturedInput);

    if (conflictingAction.has_value()) {
      m_eventManager.System.OnInputCaptureConflict.Call({m_capturingActionFullName, std::move(capturedInput), conflictingAction.value(), m_capturingOriginalBinding});
    } else {
      logger->Info("Captured key for action: {}", m_capturingActionFullName);
      m_eventManager.System.OnInputCaptured.Call({std::move(capturedInput), m_capturingActionFullName, m_capturingOriginalBinding});

      // Blacklist this key for the rest of the frame
      m_capturedKeyThisFrame = event.key;
    }

    // Reset the state of the captured key to prevent immediate action trigger
    m_keyboardStates[event.key].isDown = false;
    m_keyboardStates[event.key].wasDown = false;
    m_keyboardStates[event.key].longPressTriggered = false;
    logger->Debug("[InputManager] Consuming keyboard event due to active key capture.");
    return true;  // Consume the input and exit immediately
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
          // Get the default threshold
          auto longPressThreshold = keyBindsManager.GetLongPressThreshold();

          // Check for a binding-specific threshold, prioritizing the long press binding
          const auto* longPressBinding = keyBindsManager.GetBindingForInput(button, PressType::Long);
          if (longPressBinding && longPressBinding->PressThreshold.has_value()) {
            longPressThreshold = longPressBinding->PressThreshold.value();
          } else {
            // If not found, check if a short press binding has a threshold defined
            const auto* shortPressBinding = keyBindsManager.GetBindingForInput(button, PressType::Short);
            if (shortPressBinding && shortPressBinding->PressThreshold.has_value()) {
              longPressThreshold = shortPressBinding->PressThreshold.value();
            }
          }

          auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.pressTimestamp);

          if (pressedDuration >= longPressThreshold) {
            // logger->Info("Long press action triggered for button: {}", (int)button);
            state.longPressTriggered = true;

            // Check if this long press should initiate a hold
            if (longPressBinding && longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
              m_heldGamepadButtons[button] = PressType::Long;
            }

            keyBindsManager.TriggerAction(button, PressType::Long);

            // Update block policy based on long press action
            GamepadEvent longPressEvent{0, button, true, 1.0f, PressType::Long};
            auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent);
            switch (policy) {
              case Config::ConsumptionPolicy::Always:
                state.blockInput = true;
                break;
              case Config::ConsumptionPolicy::OnUIFocus:
                state.blockInput = !m_gameControlsMouseButtons;
                break;
              default:
                state.blockInput = false;
                break;
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

void InputManager::ProcessKeyboardActions() {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto now = std::chrono::steady_clock::now();
  auto& keyBindsManager = Modules::KeyBindsManager::GetInstance();

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
          // Get the default threshold
          auto longPressThreshold = keyBindsManager.GetLongPressThreshold();

          // Check for a binding-specific threshold, prioritizing the long press binding
          const auto* longPressBinding = keyBindsManager.GetBindingForInput(key, PressType::Long);
          if (longPressBinding && longPressBinding->PressThreshold.has_value()) {
            longPressThreshold = longPressBinding->PressThreshold.value();
          } else {
            // If not found, check if a short press binding has a threshold defined
            const auto* shortPressBinding = keyBindsManager.GetBindingForInput(key, PressType::Short);
            if (shortPressBinding && shortPressBinding->PressThreshold.has_value()) {
              longPressThreshold = shortPressBinding->PressThreshold.value();
            }
          }

          auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.pressTimestamp);

          if (pressedDuration >= longPressThreshold) {
            // logger->Info("Long press action triggered for key: {}", (int)key);
            state.longPressTriggered = true;

            // Check if this long press should initiate a hold
            if (longPressBinding && longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
              m_heldKeyboardKeys[key] = PressType::Long;
            }

            keyBindsManager.TriggerAction(key, PressType::Long);

            // Update block policy based on long press action
            KeyboardEvent longPressEvent{key, true, PressType::Long};
            auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent, PressType::Long);
            switch (policy) {
              case Config::ConsumptionPolicy::Always:
                state.blockInput = true;
                break;
              case Config::ConsumptionPolicy::OnUIFocus:
                state.blockInput = !m_gameControlsMouseButtons;
                break;
              default:
                state.blockInput = false;
                break;
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
          auto longPressThreshold = keyBindsManager.GetLongPressThreshold();
          const auto* longPressBinding = keyBindsManager.GetBindingForInput(button, PressType::Long);
          if (longPressBinding && longPressBinding->PressThreshold.has_value()) {
            longPressThreshold = longPressBinding->PressThreshold.value();
          } else {
            const auto* shortPressBinding = keyBindsManager.GetBindingForInput(button, PressType::Short);
            if (shortPressBinding && shortPressBinding->PressThreshold.has_value()) {
              longPressThreshold = shortPressBinding->PressThreshold.value();
            }
          }

          auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.pressTimestamp);

          if (pressedDuration >= longPressThreshold) {
            state.longPressTriggered = true;

            if (longPressBinding && longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
              m_heldMouseButtons[button] = PressType::Long;
            }

            keyBindsManager.TriggerAction(button, PressType::Long);

            MouseButtonEvent longPressEvent{(int)button, true, PressType::Long};
            auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent);
            switch (policy) {
              case Config::ConsumptionPolicy::Always:
                state.blockInput = true;
                break;
              case Config::ConsumptionPolicy::OnUIFocus:
                state.blockInput = !m_gameControlsMouseButtons;
                break;
              default:
                state.blockInput = false;
                break;
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
          auto longPressThreshold = keyBindsManager.GetLongPressThreshold();
          const auto* longPressBinding = keyBindsManager.GetBindingForInput(buttonIndex, PressType::Long);
          if (longPressBinding && longPressBinding->PressThreshold.has_value()) {
            longPressThreshold = longPressBinding->PressThreshold.value();
          } else {
            const auto* shortPressBinding = keyBindsManager.GetBindingForInput(buttonIndex, PressType::Short);
            if (shortPressBinding && shortPressBinding->PressThreshold.has_value()) {
              longPressThreshold = shortPressBinding->PressThreshold.value();
            }
          }

          auto pressedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - state.pressTimestamp);

          if (pressedDuration >= longPressThreshold) {
            state.longPressTriggered = true;

            if (longPressBinding && longPressBinding->Behavior == Modules::ActivationBehavior::Hold) {
              m_heldJoystickButtons[buttonIndex] = PressType::Long;
            }

            keyBindsManager.TriggerAction(buttonIndex, PressType::Long);

            JoystickEvent longPressEvent{buttonIndex, true, PressType::Long};
            auto policy = keyBindsManager.GetPolicyForEvent(longPressEvent);
            switch (policy) {
              case Config::ConsumptionPolicy::Always:
                state.blockInput = true;
                break;
              case Config::ConsumptionPolicy::OnUIFocus:
                state.blockInput = !m_gameControlsMouseButtons;  // Assuming joystick buttons might need similar blocking
                break;
              default:
                state.blockInput = false;
                break;
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
  if (m_captureState == InputCaptureState::Capturing && event.pressed && !IsAxis(event.button)) {
    auto capturedInput = std::make_shared<Modules::GamepadInput>(nlohmann::json{{"type", "gamepad"}, {"button", GamepadButtonMapping::GetInstance().GetButtonName(event.button)}});

    auto conflictingAction = Modules::KeyBindsManager::GetInstance().GetActionBoundToInput(*capturedInput);

    if (conflictingAction.has_value()) {
      m_eventManager.System.OnInputCaptureConflict.Call({m_capturingActionFullName, std::move(capturedInput), conflictingAction.value(), m_capturingOriginalBinding});
    } else {
      logger->Info("Captured gamepad button for action: {}", m_capturingActionFullName);
      m_eventManager.System.OnInputCaptured.Call({std::move(capturedInput), m_capturingActionFullName, m_capturingOriginalBinding});

      // Blacklist this button for the rest of the frame
      m_capturedButtonThisFrame = event.button;

      // Reset the state of the captured button to prevent immediate action trigger
      m_buttonStates[event.button].isDown = false;
      m_buttonStates[event.button].wasDown = false;
      m_buttonStates[event.button].longPressTriggered = false;
    }

    logger->Debug("[InputManager] Consuming gamepad event due to active key capture.");
    return true;  // Consume the event and exit immediately
  }

  // Handle axes separately from buttons.
  if (IsAxis(event.button)) {
    // Axes are never blocked from the game, per user clarification.
    // The parent PublishGamepadEvent function handles sending the axis move to UI consumers.
    return false;
  }

  // This function is now purely for state management and blocking decisions for buttons.
  auto& state = m_buttonStates[event.button];
  bool wasDown = state.isDown;
  state.isDown = event.pressed;

  // On new press (Up -> Down transition)
  if (!wasDown && state.isDown) {
    state.pressTimestamp = std::chrono::steady_clock::now();
    state.longPressTriggered = false;

    // Check for "Hold" behavior first
    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(event.button, PressType::Short);  // Hold is based on short press
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(event.button, PressType::Short);
      m_heldGamepadButtons[event.button] = PressType::Short;
    }

    // Determine initial block policy based on the short press action.
    GamepadEvent shortPressEvent = event;
    shortPressEvent.pressType = PressType::Short;
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent);

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
    // When a button is released, we never need to block the release event itself.
    state.blockInput = false;
  }

  // For held buttons or releases, return the stored blocking decision.
  return state.blockInput;
}

bool InputManager::ProcessAndDecide(const MouseButtonEvent& event) {
  auto button = static_cast<MouseButton>(event.iButton);
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");

  if (m_captureState == InputCaptureState::Capturing) {
    // In capture mode, we handle input differently.

    // Per user request, NEVER capture the left mouse button. Let it pass through to the UI.
    if (button == MouseButton::Left) {
      logger->Trace("Ignoring Left Mouse Button during input capture to allow UI interaction.");
      return false;
    }

    // For any other button that is pressed down, attempt to capture it.
    if (event.bPressed) {
      if (m_capturedMouseButtonThisFrame.has_value() && m_capturedMouseButtonThisFrame.value() == button) {
        return true;  // Already captured this frame, consume to prevent duplicates.
      }

      auto capturedInput = std::make_shared<Modules::MouseInput>(nlohmann::json{{"type", "mouse"}, {"key", MouseButtonMapping::GetInstance().ToString(button)}});

      if (!capturedInput->IsValid()) {
        return true;  // Consume invalid inputs (like a programmatic error) but don't capture.
      }

      auto conflictingAction = Modules::KeyBindsManager::GetInstance().GetActionBoundToInput(*capturedInput);

      if (conflictingAction.has_value()) {
        m_eventManager.System.OnInputCaptureConflict.Call({m_capturingActionFullName, std::move(capturedInput), conflictingAction.value(), m_capturingOriginalBinding});
      } else {
        logger->Info("Captured mouse button for action: {}", m_capturingActionFullName);
        m_eventManager.System.OnInputCaptured.Call({std::move(capturedInput), m_capturingActionFullName, m_capturingOriginalBinding});
        m_capturedMouseButtonThisFrame = button;
      }
    }

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

    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(button, PressType::Short);
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(button, PressType::Short);
      m_heldMouseButtons[button] = PressType::Short;
    }

    MouseButtonEvent shortPressEvent = {event.iButton, event.bPressed, PressType::Short};
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent);

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

  return state.blockInput;
}

bool InputManager::ProcessAndDecide(const JoystickEvent& event) {
  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  auto buttonIndex = event.buttonIndex;

  if (m_capturedJoystickButtonThisFrame.has_value() && m_capturedJoystickButtonThisFrame.value() == buttonIndex) {
    return true;  // Consume event from a potential second hook
  }

  if (m_captureState == InputCaptureState::Capturing) {
    if (event.pressed) {
      auto capturedInput = std::make_shared<Modules::JoystickInput>(nlohmann::json{{"type", "joystick"}, {"key", JoystickButtonMapping::GetInstance().ToString(buttonIndex)}});

      if (!capturedInput->IsValid()) {
        return true;  // Consume invalid inputs but don't capture.
      }

      auto conflictingAction = Modules::KeyBindsManager::GetInstance().GetActionBoundToInput(*capturedInput);

      if (conflictingAction.has_value()) {
        m_eventManager.System.OnInputCaptureConflict.Call({m_capturingActionFullName, std::move(capturedInput), conflictingAction.value(), m_capturingOriginalBinding});
      } else {
        logger->Info("Captured joystick button for action: {}", m_capturingActionFullName);
        m_eventManager.System.OnInputCaptured.Call({std::move(capturedInput), m_capturingActionFullName, m_capturingOriginalBinding});
        m_capturedJoystickButtonThisFrame = buttonIndex;
      }
    }
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

    const auto* binding = Modules::KeyBindsManager::GetInstance().GetBindingForInput(buttonIndex, PressType::Short);
    if (binding && binding->Behavior == Modules::ActivationBehavior::Hold) {
      Modules::KeyBindsManager::GetInstance().TriggerAction(buttonIndex, PressType::Short);
      m_heldJoystickButtons[buttonIndex] = PressType::Short;
    }

    JoystickEvent shortPressEvent = {buttonIndex, event.pressed, PressType::Short};
    auto policy = Modules::KeyBindsManager::GetInstance().GetPolicyForEvent(shortPressEvent);

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
    state.blockInput = shouldBlock;
  } else if (wasDown && !state.isDown) {  // Release
    state.blockInput = false;
  }

  return state.blockInput;
}

bool InputManager::ProcessAndDecide(const KeyboardEvent& event) {
  // auto logger = Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  // logger->Trace("ProcessAndDecide (Keyboard): key={}, pressed={}", (int)event.key, event.pressed);

  auto& state = m_keyboardStates[event.key];
  bool wasDown = state.isDown;
  state.isDown = event.pressed;

  // On new press (Up -> Down transition)
  if (!wasDown && state.isDown) {
    state.pressTimestamp = std::chrono::steady_clock::now();
    state.longPressTriggered = false;

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

  // For held keys or releases, return the stored blocking decision.
  return state.blockInput;
}

bool InputManager::ShouldGameControlMouseAxes() const { return m_gameControlsMouseAxes; }
bool InputManager::ShouldGameControlMouseButtons() const { return m_gameControlsMouseButtons; }
bool InputManager::ShouldGameControlMouseWheel() const { return m_gameControlsMouseWheel; }

void InputManager::SetMouseAxesControl(bool gameHasControl) { m_gameControlsMouseAxes = gameHasControl; }
void InputManager::SetMouseButtonsControl(bool gameHasControl) { m_gameControlsMouseButtons = gameHasControl; }
void InputManager::SetMouseWheelControl(bool gameHasControl) { m_gameControlsMouseWheel = gameHasControl; }

void InputManager::StartInputCapture(const std::string& actionFullName, const nlohmann::json& originalBinding) {
  auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
  logger->Info("Starting key capture for action: {}", actionFullName);
  m_captureState = InputCaptureState::Capturing;
  m_capturingActionFullName = actionFullName;
  m_capturingOriginalBinding = originalBinding;
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
    auto logger = SPF::Logging::LoggerFactory::GetInstance().GetLogger("InputManager");
    // logger->Trace("IsKeyBlocked called for key: {} (blocked: {}).", static_cast<int>(key), it->second.blockInput);
    return it->second.blockInput;
  }
  return false;
}

// --- Device Detection Implementations ---

void InputManager::UpdateDeviceType(UINT_PTR deviceId, const std::wstring& productName, DWORD vid, DWORD pid) {
  System::DeviceType detectedType = System::DeviceType::Joystick;

  // --- Primary detection via Vendor ID (VID) ---
  if (vid == 0x045E) {  // Microsoft
    detectedType = System::DeviceType::Xbox;
  } else if (vid == 0x054C) {  // Sony
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

}  // namespace Input
SPF_NS_END
