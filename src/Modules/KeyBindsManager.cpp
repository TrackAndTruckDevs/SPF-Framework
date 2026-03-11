#include "SPF/Modules/KeyBindsManager.hpp"

#include <Windows.h>  // Pre-include for safety
#include "SPF/Events/PluginEvents.hpp"
#include "SPF/Events/EventManager.hpp"

#include "SPF/Config/EnumMappings.hpp"

#include "SPF/Input/InputManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Modules/InputFactory.hpp"
#include "SPF/Modules/IBindableInput.hpp"
#include "SPF/System/GamepadButtonMapping.hpp"
#include "SPF/System/MouseButtonMapping.hpp"

#include <nlohmann/json.hpp>
#include <string_view>

SPF_NS_BEGIN
namespace Modules {
using namespace SPF::Logging;
using namespace SPF::Input;
using namespace SPF::System;
using namespace SPF::Config;

// Helper to parse binding properties from JSON
struct BindingProperties {
  Config::ConsumptionPolicy policy = Config::ConsumptionPolicy::Never;
  Input::PressType pressType = Input::PressType::Short;
  ActivationBehavior Behavior = ActivationBehavior::Toggle;
  std::optional<std::chrono::milliseconds> pressThreshold;
};

BindingProperties ParseBindingProperties(const nlohmann::ordered_json& config) {
  BindingProperties props;

  if (config.contains("consume")) {
    std::string policyStr = config.value("consume", "never");
    for (const auto& pair : Config::ConsumptionPolicyMap) {
      if (pair.second.string_id == policyStr) {
        props.policy = pair.first;
        break;
      }
    }
  }

  if (config.contains("press_type")) {
    std::string pressTypeStr = config.value("press_type", "short");
    for (const auto& pair : Config::PressTypeMap) {
      if (pair.second.string_id == pressTypeStr) {
        props.pressType = pair.first;
        break;
      }
    }
  }

  if (config.contains("behavior")) {
    std::string behaviorStr = config.value("behavior", "toggle");
    if (behaviorStr == "hold") {
      props.Behavior = ActivationBehavior::Hold;
    } else {
      props.Behavior = ActivationBehavior::Toggle;
    }
  }

  if (config.contains("press_threshold_ms")) {
    props.pressThreshold = std::chrono::milliseconds(config.value("press_threshold_ms", 500));
  }

  return props;
}

KeyBindsManager& KeyBindsManager::GetInstance() {
  // TODO: Add assert(s_instance != nullptr);
  return *s_instance;
}

KeyBindsManager::KeyBindsManager(Input::InputManager& inputManager, Events::EventManager& eventManager)
    : m_inputManager(inputManager),
      m_eventManager(eventManager),
      m_onPluginDidLoadSink(eventManager.System.OnPluginDidLoad),
      m_onPluginWillBeUnloadedSink(eventManager.System.OnPluginWillBeUnloaded) {
  s_instance = this;
  m_onPluginDidLoadSink.Connect<&KeyBindsManager::OnPluginLoaded>(this);
  m_onPluginWillBeUnloadedSink.Connect<&KeyBindsManager::OnPluginUnloaded>(this);
}

void KeyBindsManager::ApplyAxisPropertiesToInputManager() {
    m_inputManager.ResetAxisProperties();

    struct AxisAggr {
        Config::ConsumptionPolicy policy = Config::ConsumptionPolicy::Never;
        bool emulation = false;
        bool isAccumulator = false;
        bool invert = false;
        std::string side = "both";
        float threshold = 0.5f;
        float sensitivity = 1.0f;
        float rMin = -1.0f;
        float rMax = 1.0f;
        bool found = false;
    };
    std::map<uint32_t, AxisAggr> axisAggregation;

    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (!binding.Input) continue;

            uint32_t code = binding.Input->GetHardwareCode();
            bool isAxis = ((code >> 16) & 0xFF) == 0x01; // Axis flag

            if (isAxis) {
                auto& aggr = axisAggregation[code];
                aggr.found = true;

                // If ANY binding wants a stricter policy, use it
                if (binding.Policy > aggr.policy) aggr.policy = binding.Policy;

                // Capture sensitivity, range, invert, side and accumulator settings
                aggr.sensitivity = binding.originalBindingJson.value("sensitivity", aggr.sensitivity);
                aggr.rMin = binding.originalBindingJson.value("range_min", aggr.rMin);
                aggr.rMax = binding.originalBindingJson.value("range_max", aggr.rMax);
                aggr.invert = binding.originalBindingJson.value("invert", aggr.invert);
                aggr.side = binding.originalBindingJson.value("side", aggr.side);
                
                // Mouse is always an accumulator, for others read from JSON
                if (binding.originalBindingJson.value("type", "") == "mouse_axis") {
                    aggr.isAccumulator = true;
                } else {
                    aggr.isAccumulator = binding.originalBindingJson.value("accumulator", false);
                }

                // If ANY binding is digital, enable emulation for the axis
                if (binding.originalBindingJson.value("mode", "analog") == "digital") {
                    aggr.emulation = true;
                    aggr.threshold = binding.originalBindingJson.value("threshold", 0.5f);
                }
            }
        }
    }

    // Push aggregated properties
    for (const auto& [code, aggr] : axisAggregation) {
        m_inputManager.SetAxisProperties(code, aggr.policy, aggr.emulation, aggr.isAccumulator, aggr.invert, aggr.side, aggr.threshold, aggr.sensitivity, aggr.rMin, aggr.rMax);
    }
}

KeyBindsManager::~KeyBindsManager() {
  s_instance = nullptr;
  m_inputManager.UnregisterConsumer(this);
}

Core::InitializationReport KeyBindsManager::Initialize(const nlohmann::ordered_json* keyBindsConfig, const std::map<std::string, Config::ComponentInfo>& componentInfo) {
  m_inputManager.RegisterConsumer(this);
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  logger->Info("KeyBindsManager initialized and registered as InputConsumer.");

  m_actions.clear();
  m_inactiveActions.clear();
  logger->Info("Initializing keybinds from merged config...");
  Core::InitializationReport report;
  report.ServiceName = "keybinds";

  if (!keyBindsConfig || !keyBindsConfig->is_object()) {
    report.Warnings.push_back({"'keybinds' data not found or is not an object.", ""});
    return report;
  }

  for (auto const& [groupName, actions] : keyBindsConfig->items()) {
    if (!actions.is_object()) {
      report.Errors.push_back(Core::InitializationReport::Issue{fmt::format("Action group '{}' must be an object.", groupName), groupName});
      continue;
    }

    // Extract the root component name (owner) from the group name.
    // e.g., "TestPlugin.ui.test_plugin_window" -> "TestPlugin"
    std::string owner = groupName;
    size_t firstDot = owner.find('.');
    if (firstDot != std::string::npos) {
      owner = owner.substr(0, firstDot);
    }

    for (auto const& [actionName, actionNode] : actions.items()) {
      std::string fullActionKey = groupName + "." + actionName;

      const nlohmann::ordered_json* inputs = nullptr;
      if (actionNode.is_object() && actionNode.contains("bindings")) {
        inputs = &actionNode["bindings"];
      } else if (actionNode.is_array()) {
        inputs = &actionNode;
      }

      if (!inputs || !inputs->is_array()) {
        report.Errors.push_back(Core::InitializationReport::Issue{fmt::format("Inputs for action '{}' must be an array.", fullActionKey), fullActionKey});
        continue;
      }

      Action action;  // Create the action struct
      for (size_t i = 0; i < inputs->size(); ++i) {
        const auto& inputConfig = (*inputs)[i];
        auto input = InputFactory::CreateFromJson(inputConfig);
        if (!input || !input->IsValid()) {
          report.Errors.push_back(Core::InitializationReport::Issue{fmt::format("Invalid input configuration for action '{}': {}", fullActionKey, inputConfig.dump()),
                                                                    fmt::format("{}[{}]", fullActionKey, i)});
        } else {
          auto props = ParseBindingProperties(inputConfig);
          action.Inputs.emplace_back(Binding{std::move(input), props.policy, props.pressType, props.Behavior, props.pressThreshold, inputConfig});
        }
      }

      auto componentIt = componentInfo.find(owner);
      if (componentIt == componentInfo.end()) {
        report.Errors.push_back({fmt::format("Action '{}' has an unknown owner '{}'.", fullActionKey, owner), fullActionKey});
        continue;
      }

      // It is a valid component, check if it's the framework or an enabled plugin.
      const auto& info = componentIt->second;
      if (info.isFramework || info.isEnabled) {
        m_actions[fullActionKey] = std::move(action);
        logger->Info("Registered active action: {}", fullActionKey);
      } else {
        m_inactiveActions[owner][fullActionKey] = std::move(action);
        logger->Info("Stored inactive action: {} for owner: {}", fullActionKey, owner);
      }
    }
  }

  ApplyAxisPropertiesToInputManager();

  logger->Info("Keybinds initialization complete. Issues found: {}", report.HasIssues() ? "Yes" : "No");
  return report;
}

void KeyBindsManager::UpdateKeybindings(const nlohmann::ordered_json* keyBindsConfig) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  logger->Info("Updating keybindings from new config...");

  if (!keyBindsConfig || !keyBindsConfig->is_object()) {
    logger->Warn("UpdateKeybindings failed: 'keybinds' data not found or is not an object.");
    return;
  }

  for (auto const& [groupName, actions] : keyBindsConfig->items()) {
    if (!actions.is_object()) continue;

    for (auto const& [actionName, actionNode] : actions.items()) {
      std::string fullActionKey = groupName + "." + actionName;

      auto actionIt = m_actions.find(fullActionKey);
      if (actionIt == m_actions.end()) {
        continue;
      }

      const nlohmann::ordered_json* inputs = nullptr;
      if (actionNode.is_object() && actionNode.contains("bindings")) {
        inputs = &actionNode["bindings"];
      } else if (actionNode.is_array()) {
        inputs = &actionNode;
      }

      if (!inputs || !inputs->is_array()) {
        logger->Error("Inputs for action '{}' must be an array, skipping update for this action.", fullActionKey);
        continue;
      }

      // Create a new Action object to replace the old one.
      Action newAction;
      // IMPORTANT: Preserve the callbacks and user data from the existing action.
      newAction.Callback = actionIt->second.Callback;
      newAction.CallbackEx = actionIt->second.CallbackEx;
      newAction.UserData = actionIt->second.UserData;

      // Populate the new action's input list from the configuration.
      for (const auto& inputConfig : *inputs) {
        auto input = InputFactory::CreateFromJson(inputConfig);
        if (!input || !input->IsValid()) {
          logger->Error("Invalid input configuration for action '{}': {}", fullActionKey, inputConfig.dump());
        } else {
          auto props = ParseBindingProperties(inputConfig);
          newAction.Inputs.emplace_back(Binding{std::move(input), props.policy, props.pressType, props.Behavior, props.pressThreshold, inputConfig});
        }
      }

      // Replace the old action with the newly created one.
      actionIt->second = std::move(newAction);
    }
  }

  ApplyAxisPropertiesToInputManager();

  logger->Info("Keybinding update complete.");
}

void KeyBindsManager::EnsureActionExists(const std::string& actionKey) {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    if (m_actions.find(actionKey) == m_actions.end()) {
        m_actions[actionKey] = Action();
        auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
        if (logger) logger->Info("Dynamically created action entry for '{}'.", actionKey);
    }
}

void KeyBindsManager::RemoveAction(const std::string& actionKey) {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    auto it = m_actions.find(actionKey);
    if (it != m_actions.end()) {
        m_actions.erase(it);
        auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
        if (logger) logger->Info("Dynamically removed action entry for '{}'.", actionKey);
    }
}

void KeyBindsManager::RegisterAction(const std::string& actionKey, ActionCallback callback) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  auto it = m_actions.find(actionKey);
  if (it != m_actions.end()) {
    it->second.Callback = callback;
  } else {
    auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
    if (logger) logger->Warn("Attempted to register a callback for an unknown action '{}'.", actionKey);
  }
}

void KeyBindsManager::RegisterActionEx(const std::string& actionKey, ActionCallbackEx callback, void* userData) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  auto it = m_actions.find(actionKey);
  if (it != m_actions.end()) {
    it->second.CallbackEx = callback;
    it->second.UserData = userData;
  } else {
    auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
    if (logger) logger->Warn("Attempted to register an extended callback for an unknown action '{}'.", actionKey);
  }
}

void KeyBindsManager::UnregisterOwner(const std::string& owner) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  if (logger) logger->Info("Unregistering all actions for owner '{}'.", owner);
  std::erase_if(m_actions, [&](const auto& item) {
    const std::string& actionKey = item.first;
    // Check if the actionKey starts with the owner's name followed by a dot.
    return actionKey.rfind(owner + ".", 0) == 0;
  });
}

void KeyBindsManager::SetBlockState(const std::string& actionKey, bool blocked) {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    auto it = m_actions.find(actionKey);
    if (it != m_actions.end()) {
        for (auto& binding : it->second.Inputs) {
            binding.programmaticallyBlocked = blocked;
        }
    }
}

std::vector<std::pair<std::string, nlohmann::ordered_json>> KeyBindsManager::GetBindingsForInput(const IBindableInput& input) const {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  std::vector<std::pair<std::string, nlohmann::ordered_json>> conflicts;
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (binding.Input && binding.Input->IsSameAs(input)) {
        conflicts.emplace_back(actionName, binding.originalBindingJson);
      }
    }
  }
  return conflicts;
}

const Binding* KeyBindsManager::GetBindingForInput(System::Keyboard key, Input::PressType pressType) const {
  uint32_t code = 0x01000000 | static_cast<uint32_t>(key);
  return FindBestBinding(code, pressType);
}

const Binding* KeyBindsManager::GetBindingForInput(System::GamepadButton button, Input::PressType pressType) const {
  uint32_t code = 0x02000000 | static_cast<uint32_t>(button);
  return FindBestBinding(code, pressType);
}

const Binding* KeyBindsManager::GetBindingForInput(System::MouseButton button, Input::PressType pressType) const {
  uint32_t code = 0x03000000 | static_cast<uint32_t>(button);
  return FindBestBinding(code, pressType);
}

const Binding* KeyBindsManager::GetBindingForInput(int buttonIndex, Input::PressType pressType) const {
  uint32_t code = 0x04000000 | static_cast<uint32_t>(buttonIndex);
  return FindBestBinding(code, pressType);
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::KeyboardEvent& event, Input::PressType pressType) const {
  uint32_t code = 0x01000000 | static_cast<uint32_t>(event.key);
  const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();

  const Binding* bestActiveBinding = nullptr;
  size_t maxComplexity = 0;

  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (!binding.Input || binding.PressType != pressType) continue;
      if (!binding.Input->InvolvesHardwareCode(code)) continue;

      // Only consider bindings that are FULLY ACTIVE right now
      if (binding.Input->IsActive(pressedCodes)) {
          size_t complexity = 1;
          if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
              complexity = chord->GetInputs().size();
          }

          if (complexity > maxComplexity) {
              maxComplexity = complexity;
              bestActiveBinding = &binding;
          }
      }
    }
  }

  if (bestActiveBinding) {
      ConsumptionPolicy policy = bestActiveBinding->Policy;
      if (policy == ConsumptionPolicy::Manual) {
          policy = bestActiveBinding->programmaticallyBlocked ? ConsumptionPolicy::Always : ConsumptionPolicy::Never;
      }
      return policy;
  }

  return ConsumptionPolicy::Never;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::GamepadEvent& event, Input::PressType pressType) const {
  uint32_t code = 0x02000000 | static_cast<uint32_t>(event.button);
  const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();

  const Binding* bestActiveBinding = nullptr;
  size_t maxComplexity = 0;

  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (!binding.Input || binding.PressType != pressType) continue;
      if (!binding.Input->InvolvesHardwareCode(code)) continue;

      if (binding.Input->IsActive(pressedCodes)) {
          size_t complexity = 1;
          if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
              complexity = chord->GetInputs().size();
          }

          if (complexity > maxComplexity) {
              maxComplexity = complexity;
              bestActiveBinding = &binding;
          }
      }
    }
  }

  if (bestActiveBinding) {
      ConsumptionPolicy policy = bestActiveBinding->Policy;
      if (policy == ConsumptionPolicy::Manual) {
          policy = bestActiveBinding->programmaticallyBlocked ? ConsumptionPolicy::Always : ConsumptionPolicy::Never;
      }
      return policy;
  }
  return ConsumptionPolicy::Never;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::MouseButtonEvent& event, Input::PressType pressType) const {
  uint32_t code = 0x03000000 | static_cast<uint32_t>(event.iButton);
  const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();

  const Binding* bestActiveBinding = nullptr;
  size_t maxComplexity = 0;

  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (!binding.Input || binding.PressType != pressType) continue;
      if (!binding.Input->InvolvesHardwareCode(code)) continue;

      if (binding.Input->IsActive(pressedCodes)) {
          size_t complexity = 1;
          if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
              complexity = chord->GetInputs().size();
          }

          if (complexity > maxComplexity) {
              maxComplexity = complexity;
              bestActiveBinding = &binding;
          }
      }
    }
  }

  if (bestActiveBinding) {
      ConsumptionPolicy policy = bestActiveBinding->Policy;
      if (policy == ConsumptionPolicy::Manual) {
          policy = bestActiveBinding->programmaticallyBlocked ? ConsumptionPolicy::Always : ConsumptionPolicy::Never;
      }
      return policy;
  }
  return ConsumptionPolicy::Never;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::JoystickEvent& event, Input::PressType pressType) const {
  uint32_t code = 0x04000000 | static_cast<uint32_t>(event.buttonIndex);
  const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();

  const Binding* bestActiveBinding = nullptr;
  size_t maxComplexity = 0;

  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (!binding.Input || binding.PressType != pressType) continue;
      if (!binding.Input->InvolvesHardwareCode(code)) continue;

      if (binding.Input->IsActive(pressedCodes)) {
          size_t complexity = 1;
          if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
              complexity = chord->GetInputs().size();
          }

          if (complexity > maxComplexity) {
              maxComplexity = complexity;
              bestActiveBinding = &binding;
          }
      }
    }
  }

  if (bestActiveBinding) {
      ConsumptionPolicy policy = bestActiveBinding->Policy;
      if (policy == ConsumptionPolicy::Manual) {
          policy = bestActiveBinding->programmaticallyBlocked ? ConsumptionPolicy::Always : ConsumptionPolicy::Never;
      }
      return policy;
  }
  return ConsumptionPolicy::Never;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForHardwareCode(uint32_t hardwareCode) const {
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (binding.Input && binding.Input->InvolvesHardwareCode(hardwareCode)) {
                ConsumptionPolicy policy = binding.Policy;
                if (policy == ConsumptionPolicy::Manual) {
                    policy = binding.programmaticallyBlocked ? ConsumptionPolicy::Always : ConsumptionPolicy::Never;
                }
                return policy;
            }
        }
    }
    return ConsumptionPolicy::Never;
}

float KeyBindsManager::GetThresholdForHardwareCode(uint32_t hardwareCode) const {
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (binding.Input && binding.Input->InvolvesHardwareCode(hardwareCode)) {
                // Return the threshold from the JSON config of this binding
                return binding.originalBindingJson.value("threshold", 0.5f);
            }
        }
    }
    return 0.5f;
}

std::chrono::milliseconds KeyBindsManager::GetLongPressThreshold() const {
  // TODO: Make this configurable
  return std::chrono::milliseconds(500);
}

bool KeyBindsManager::OnKeyPress(const KeyboardEvent& event) {
  // DEPRECATED: In the new architecture, InputManager handles the state machine
  // and decides when to call actions. This consumer method should not trigger actions directly
  // to avoid conflicts with the short/long press logic.
  // auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  // if(logger) logger->Trace("OnKeyPress received key: {}. Handled by InputManager.", static_cast<int>(event.key));

  // We must return false so that the input is not considered "consumed" by this layer.
  return false;
}

bool KeyBindsManager::OnKeyRelease(const KeyboardEvent& event) {
  // DEPRECATED: See OnKeyPress.
  // auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  // if(logger) logger->Trace("OnKeyRelease received for key: {}. Handled by InputManager.", static_cast<int>(event.key));
  return false;
}

bool KeyBindsManager::OnGamepadButtonPress(const GamepadEvent& event) {
  // DEPRECATED: In the new architecture, InputManager handles the state machine
  // and decides when to call actions. This consumer method should not trigger actions directly
  // to avoid conflicts with the short/long press logic.
  // We must return false so that the input is not considered "consumed" by this layer.
  return false;
}

bool KeyBindsManager::OnGamepadButtonRelease(const GamepadEvent& event) { return false; }

void KeyBindsManager::TriggerAction(uint32_t hardwareCode, Input::PressType pressType) {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    uint8_t type = (hardwareCode >> 24) & 0xFF;
    uint32_t raw = hardwareCode & 0x00FFFFFF;

    switch (type) {
    case 0x01: TriggerAction(static_cast<System::Keyboard>(raw), pressType); break;
    case 0x02: TriggerAction(static_cast<System::GamepadButton>(raw), pressType); break;
    case 0x03: TriggerAction(static_cast<System::MouseButton>(raw), pressType); break;
    case 0x04: TriggerAction(static_cast<int>(raw), pressType); break;
    }
}

void KeyBindsManager::TriggerAction(System::GamepadButton button, Input::PressType pressType) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  uint32_t code = 0x02000000 | static_cast<uint32_t>(button);
  const Binding* best = FindBestBinding(code, pressType);
  if (best) {
    for (const auto& [actionKey, action] : m_actions) {
      for (const auto& b : action.Inputs) {
        if (&b == best) {
          if (action.Callback) action.Callback();
          if (action.CallbackEx) action.CallbackEx(actionKey, action.UserData);
          return;
        }
      }
    }
  }
}

void KeyBindsManager::TriggerAction(System::Keyboard key, Input::PressType pressType) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  uint32_t code = 0x01000000 | static_cast<uint32_t>(key);
  const Binding* best = FindBestBinding(code, pressType);
  if (best) {
    for (const auto& [actionKey, action] : m_actions) {
      for (const auto& b : action.Inputs) {
        if (&b == best) {
          if (action.Callback) action.Callback();
          if (action.CallbackEx) action.CallbackEx(actionKey, action.UserData);
          return;
        }
      }
    }
  }
}

void KeyBindsManager::TriggerAction(System::MouseButton button, Input::PressType pressType) {
  std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
  uint32_t code = 0x03000000 | static_cast<uint32_t>(button);
  const Binding* best = FindBestBinding(code, pressType);
  if (best) {
    for (const auto& [actionKey, action] : m_actions) {
      for (const auto& b : action.Inputs) {
        if (&b == best) {
          if (action.Callback) action.Callback();
          if (action.CallbackEx) action.CallbackEx(actionKey, action.UserData);
          return;
        }
      }
    }
  }
}

void KeyBindsManager::TriggerAction(int buttonIndex, Input::PressType pressType) {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    uint32_t code = 0x04000000 | static_cast<uint32_t>(buttonIndex);
    const Binding* best = FindBestBinding(code, pressType);
    if (best) {
        for (const auto& [actionKey, action] : m_actions) {
            for (const auto& b : action.Inputs) {
                if (&b == best) {
                    if (action.Callback) action.Callback();
                    if (action.CallbackEx) action.CallbackEx(actionKey, action.UserData);
                    return;
                }
            }
        }
    }
}
  
  float KeyBindsManager::GetActionValue(const std::string& actionName) const {
      std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
      auto it = m_actions.find(actionName);
      if (it == m_actions.end()) {
          return 0.0f;
      }
  
      float maxVal = 0.0f;
      const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();
      const auto& axisValues = m_inputManager.GetCurrentlyActiveAxisValues();
  
      for (const auto& binding : it->second.Inputs) {
          if (!binding.Input) continue;
          
          // We only consider bindings that are programmatically active (not blocked)
          if (binding.programmaticallyBlocked) continue;
  
          float val = binding.Input->GetValue(pressedCodes, axisValues);
          if (std::abs(val) > std::abs(maxVal)) {
              maxVal = val;
          }
      }
      return maxVal;
  }

  size_t KeyBindsManager::GetBindingCount(const std::string& actionName) const {
      std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
      auto it = m_actions.find(actionName);
      if (it == m_actions.end()) {
          return 0;
      }
      return it->second.Inputs.size();
  }

  const Binding* KeyBindsManager::GetBinding(const std::string& actionName, size_t index) const {
      std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
      auto it = m_actions.find(actionName);
      if (it == m_actions.end() || index >= it->second.Inputs.size()) {
          return nullptr;
      }
      return &it->second.Inputs[index];
  }
  
  bool KeyBindsManager::OnGamepadAxisMove(const GamepadEvent& event) {
    auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");  auto& mapper = System::GamepadButtonMapping::GetInstance();
  if (logger) {
    // logger->TraceThrottled(
    //     std::chrono::milliseconds(1000),
    //     "OnGamepadAxisMove received axis: {}, value: {:.2f}",
    //     mapper.GetButtonName(event.button),
    //     event.value
    // );
  }
  // We don't have axis-based actions yet, so we don't consume the event.
  return false;
}

void KeyBindsManager::OnPluginLoaded(const Events::OnPluginDidLoad& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  if (logger) logger->Info("Plugin '{}' loaded, activating its keybinds...", e.pluginName);

  auto it = m_inactiveActions.find(e.pluginName);
  if (it != m_inactiveActions.end()) {
    // Move all actions from inactive to active map
    for (auto& [actionKey, action] : it->second) {
      m_actions[actionKey] = std::move(action);
      logger->Info("  -> Activated action: {}", actionKey);
    }
    // Erase the entry from the inactive map
    m_inactiveActions.erase(it);
  }
}

void KeyBindsManager::OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e) {
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  if (logger) logger->Info("Plugin '{}' unloading, deactivating its keybinds...", e.pluginName);

  std::vector<std::string> keysToMove;
  for (const auto& [actionKey, action] : m_actions) {
    if (actionKey.rfind(e.pluginName + ".", 0) == 0) {
      keysToMove.push_back(actionKey);
    }
  }

  for (const auto& key : keysToMove) {
    auto node = m_actions.extract(key);
    m_inactiveActions[e.pluginName][key] = std::move(node.mapped());
    logger->Info("  -> Deactivated action: {}", key);
  }
}

bool KeyBindsManager::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) {
  // This component only cares about the 'keybinds' system.
  // The actual update is handled by the OnKeybindsModified event flow.
  // We just return true here to confirm ownership of the system and prevent
  // the event from being incorrectly forwarded to a plugin.
  return systemName == "keybinds";
}

KeyBindsManager::PressTypeConflictAnalysis KeyBindsManager::AnalyzeConflictsForInput(const IBindableInput& input) const {
  PressTypeConflictAnalysis analysis;
  auto allPhysicalConflicts = GetBindingsForInput(input);
  auto inputType = input.GetType();
  bool isAxis = (inputType == InputType::GamepadAxis || inputType == InputType::MouseAxis || inputType == InputType::JoystickAxis);

  for (const auto& conflict : allPhysicalConflicts) {
    const auto& bindingJson = conflict.second;
    
    if (isAxis) {
        std::string side = bindingJson.value("side", "both");
        if (side == "both") {
            analysis.isPositiveAvailable = false;
            analysis.isNegativeAvailable = false;
            analysis.bothConflict = conflict;
        } else if (side == "positive") {
            analysis.isPositiveAvailable = false;
            analysis.positiveConflict = conflict;
        } else if (side == "negative") {
            analysis.isNegativeAvailable = false;
            analysis.negativeConflict = conflict;
        }
    } else {
        std::string pressType = bindingJson.value("press_type", "short");
        if (pressType == "short") {
          analysis.isShortPressAvailable = false;
          analysis.shortPressConflict = conflict;
        } else if (pressType == "long") {
          analysis.isLongPressAvailable = false;
          analysis.longPressConflict = conflict;
        }
    }
  }

  return analysis;
}

std::optional<std::pair<std::string, nlohmann::ordered_json>> KeyBindsManager::FindConflictForBinding(const IBindableInput& input, Input::PressType pressType,
                                                                                              const std::string& actionToExclude) const {
  for (const auto& [actionName, action] : m_actions) {
    // Skip the action that is currently being edited.
    if (actionName == actionToExclude) {
      continue;
    }

    for (const auto& binding : action.Inputs) {
      if (binding.PressType == pressType && binding.Input && binding.Input->IsSameAs(input)) {
        // Found a conflict
        return std::make_pair(actionName, binding.originalBindingJson);
      }
    }
  }

  // No conflict found
  return std::nullopt;
}

const Binding* KeyBindsManager::FindBestBinding(uint32_t triggerHardwareCode, Input::PressType pressType) const {
    std::lock_guard<std::recursive_mutex> lock(m_actionsMutex);
    const Binding* bestBinding = nullptr;
    size_t maxActiveComplexity = 0;

    const auto& pressedCodes = m_inputManager.GetCurrentlyPressedHardwareCodes();

    // First pass: Determine the maximum complexity among ALL active bindings involving this trigger.
    // This establishes which "layer" of input (single vs chord) currently owns this hardware code.
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (!binding.Input) continue;

            bool involvesTrigger = false;
            size_t complexity = 1;

            if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
                complexity = chord->GetInputs().size();
                for (const auto& inner : chord->GetInputs()) {
                    if (inner->GetHardwareCode() == triggerHardwareCode) {
                        involvesTrigger = true;
                        break;
                    }
                }
            } else {
                involvesTrigger = (binding.Input->GetHardwareCode() == triggerHardwareCode);
            }

            if (involvesTrigger) {
                bool isActive = binding.Input->IsActive(pressedCodes);
                if (!isActive) {
                    if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
                        bool allOthersActive = true;
                        for (const auto& inner : chord->GetInputs()) {
                            if (inner->GetHardwareCode() == triggerHardwareCode) continue;
                            if (!inner->IsActive(pressedCodes)) {
                                allOthersActive = false;
                                break;
                            }
                        }
                        isActive = allOthersActive;
                    } else {
                        isActive = true;
                    }
                }

                if (isActive && complexity > maxActiveComplexity) {
                    maxActiveComplexity = complexity;
                }
            }
        }
    }

    // Second pass: Find the binding that matches the requested pressType AND matches the maximum active complexity.
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (!binding.Input || binding.PressType != pressType) continue;

            size_t complexity = 1;
            bool involvesTrigger = false;
            if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
                complexity = chord->GetInputs().size();
                for (const auto& inner : chord->GetInputs()) {
                    if (inner->GetHardwareCode() == triggerHardwareCode) {
                        involvesTrigger = true;
                        break;
                    }
                }
            } else {
                involvesTrigger = (binding.Input->GetHardwareCode() == triggerHardwareCode);
            }

            if (involvesTrigger && complexity == maxActiveComplexity) {
                // IMPORTANT: Re-verify activation state to ensure we pick the CORRECT active binding of this complexity.
                bool isActive = binding.Input->IsActive(pressedCodes);
                if (!isActive) {
                    if (auto* chord = dynamic_cast<const ChordInput*>(binding.Input.get())) {
                        bool allOthersActive = true;
                        for (const auto& inner : chord->GetInputs()) {
                            if (inner->GetHardwareCode() == triggerHardwareCode) continue;
                            if (!inner->IsActive(pressedCodes)) {
                                allOthersActive = false;
                                break;
                            }
                        }
                        isActive = allOthersActive;
                    } else {
                        isActive = true;
                    }
                }

                if (isActive) {
                    bestBinding = &binding;
                    break; // Found the best match for this pressType
                }
            }
        }
    }
    return bestBinding;
}

}  // namespace Modules
SPF_NS_END
