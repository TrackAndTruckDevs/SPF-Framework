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

BindingProperties ParseBindingProperties(const nlohmann::json& config) {
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

KeyBindsManager::~KeyBindsManager() {
  s_instance = nullptr;
  m_inputManager.UnregisterConsumer(this);
}

Core::InitializationReport KeyBindsManager::Initialize(const nlohmann::json* keyBindsConfig, const std::map<std::string, Config::ComponentInfo>& componentInfo) {
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

      const nlohmann::json* inputs = nullptr;
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
          action.Inputs.emplace_back(Binding{std::move(input), props.policy, props.pressType, props.Behavior, props.pressThreshold});
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

  logger->Info("Keybinds initialization complete. Issues found: {}", report.HasIssues() ? "Yes" : "No");
  return report;
}

void KeyBindsManager::UpdateKeybindings(const nlohmann::json* keyBindsConfig) {
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

      const nlohmann::json* inputs = nullptr;
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
      // IMPORTANT: Preserve the callback from the existing action.
      newAction.Callback = actionIt->second.Callback;

      // Populate the new action's input list from the configuration.
      for (const auto& inputConfig : *inputs) {
        auto input = InputFactory::CreateFromJson(inputConfig);
        if (!input || !input->IsValid()) {
          logger->Error("Invalid input configuration for action '{}': {}", fullActionKey, inputConfig.dump());
        } else {
          auto props = ParseBindingProperties(inputConfig);
          newAction.Inputs.emplace_back(Binding{std::move(input), props.policy, props.pressType, props.Behavior, props.pressThreshold});
        }
      }

      // Replace the old action with the newly created one.
      actionIt->second = std::move(newAction);
    }
  }
  logger->Info("Keybinding update complete.");
}
void KeyBindsManager::RegisterAction(const std::string& actionKey, ActionCallback callback) {
  auto it = m_actions.find(actionKey);
  if (it != m_actions.end()) {
    it->second.Callback = callback;
  } else {
    auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
    if (logger) logger->Warn("Attempted to register a callback for an unknown action '{}'.", actionKey);
  }
}

void KeyBindsManager::UnregisterOwner(const std::string& owner) {
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  if (logger) logger->Info("Unregistering all actions for owner '{}'.", owner);
  std::erase_if(m_actions, [&](const auto& item) {
    const std::string& actionKey = item.first;
    // Check if the actionKey starts with the owner's name followed by a dot.
    return actionKey.rfind(owner + ".", 0) == 0;
  });
}

std::optional<std::string> KeyBindsManager::GetActionBoundToInput(const IBindableInput& input) const {
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      if (binding.Input && binding.Input->IsSameAs(input)) {
        return actionName;
      }
    }
  }
  return std::nullopt;
}

const Binding* KeyBindsManager::GetBindingForInput(System::Keyboard key, Input::PressType pressType) const {
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      Input::KeyboardEvent event{key, (pressType == Input::PressType::Long), pressType};
      if (binding.PressType == pressType && binding.Input && binding.Input->IsTriggeredBy(event)) {
        return &binding;
      }
    }
  }
  return nullptr;
}

const Binding* KeyBindsManager::GetBindingForInput(System::GamepadButton button, Input::PressType pressType) const {
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      Input::GamepadEvent event{0, button, (pressType == Input::PressType::Long), 1.0f, pressType};
      if (binding.PressType == pressType && binding.Input && binding.Input->IsTriggeredBy(event)) {
        return &binding;
      }
    }
  }
  return nullptr;
}

const Binding* KeyBindsManager::GetBindingForInput(System::MouseButton button, Input::PressType pressType) const {
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            Input::MouseButtonEvent event{(int)button, (pressType == Input::PressType::Long), pressType};
            if (binding.PressType == pressType && binding.Input && binding.Input->IsTriggeredBy(event)) {
                return &binding;
            }
        }
    }
    return nullptr;
}

const Binding* KeyBindsManager::GetBindingForInput(int buttonIndex, Input::PressType pressType) const {
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            Input::JoystickEvent event{buttonIndex, (pressType == Input::PressType::Long), pressType};
            if (binding.PressType == pressType && binding.Input && binding.Input->IsTriggeredBy(event)) {
                return &binding;
            }
        }
    }
    return nullptr;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::KeyboardEvent& event, Input::PressType pressType) const {
  ConsumptionPolicy strictestPolicy = ConsumptionPolicy::Never;
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      // When determining the initial blocking policy, we check if ANY binding for this key wants to consume it,
      // regardless of the press type. The InputManager needs to know immediately on press if the input
      // should be blocked, even if the action is for a long press.
      if (binding.Input && binding.Input->IsTriggeredBy(event)) {
        if (binding.Policy > strictestPolicy) {
          strictestPolicy = binding.Policy;
        }
      }
    }
  }
  return strictestPolicy;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::GamepadEvent& event) const {
  ConsumptionPolicy strictestPolicy = ConsumptionPolicy::Never;
  for (const auto& [actionName, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      // When determining the initial blocking policy, we check if ANY binding for this button wants to consume it,
      // regardless of the press type. The InputManager needs to know immediately on press if the input
      // should be blocked, even if the action is for a long press.
      if (binding.Input && binding.Input->IsTriggeredBy(event)) {
        if (binding.Policy > strictestPolicy) {
          strictestPolicy = binding.Policy;
        }
      }
    }
  }
  return strictestPolicy;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::MouseButtonEvent& event) const {
    ConsumptionPolicy strictestPolicy = ConsumptionPolicy::Never;
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (binding.Input && binding.Input->IsTriggeredBy(event)) {
                if (binding.Policy > strictestPolicy) {
                    strictestPolicy = binding.Policy;
                }
            }
        }
    }
    return strictestPolicy;
}

ConsumptionPolicy KeyBindsManager::GetPolicyForEvent(const Input::JoystickEvent& event) const {
    ConsumptionPolicy strictestPolicy = ConsumptionPolicy::Never;
    for (const auto& [actionName, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            if (binding.Input && binding.Input->IsTriggeredBy(event)) {
                if (binding.Policy > strictestPolicy) {
                    strictestPolicy = binding.Policy;
                }
            }
        }
    }
    return strictestPolicy;
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

bool KeyBindsManager::OnGamepadButtonRelease(const GamepadEvent& event) {
  return false;
}

void KeyBindsManager::TriggerAction(System::GamepadButton button, Input::PressType pressType) {
  // auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  // logger->Trace("TriggerAction called for button: {}, pressType: {}", (int)button, (int)pressType);

  for (const auto& [actionKey, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      // Create a temporary event to check the binding
      Input::GamepadEvent event{0, button, (pressType == Input::PressType::Long), 1.0f, pressType};

      bool isTriggered = binding.Input && binding.Input->IsTriggeredBy(event);
      // logger->Trace("  - Checking action '{}': binding.PressType={}, received.pressType={}, isTriggeredBy={}",
      //               actionKey, (int)binding.PressType, (int)pressType, isTriggered);

      if (binding.PressType == pressType && isTriggered) {
        if (action.Callback) {
          // logger->Info("Action '{}' triggered by gamepad {} press!", actionKey, (pressType == Input::PressType::Short ? "short" : "long"));
          action.Callback();
          return;  // Assume one action per trigger for simplicity
        }
      }
    }
  }
}

void KeyBindsManager::TriggerAction(System::Keyboard key, Input::PressType pressType) {
  // auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  // logger->Trace("TriggerAction called for key: {}, pressType: {}", (int)key, (int)pressType);

  for (const auto& [actionKey, action] : m_actions) {
    for (const auto& binding : action.Inputs) {
      // Create a temporary event to check the binding
      Input::KeyboardEvent event{key, (pressType == Input::PressType::Long), pressType};

      bool isTriggered = binding.Input && binding.Input->IsTriggeredBy(event);
      // logger->Trace("  - Checking action '{}': binding.PressType={}, received.pressType={}, isTriggeredBy={}",
      // actionKey, (int)binding.PressType, (int)pressType, isTriggered);

      if (binding.PressType == pressType && isTriggered) {
        if (action.Callback) {
          // logger->Info("Action '{}' triggered by keyboard {} press!", actionKey, (pressType == Input::PressType::Short ? "short" : "long"));
          action.Callback();
          return;  // Assume one action per trigger for simplicity
        }
      }
    }
  }
}

void KeyBindsManager::TriggerAction(System::MouseButton button, Input::PressType pressType) {
    for (const auto& [actionKey, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            Input::MouseButtonEvent event{(int)button, (pressType == Input::PressType::Long), pressType};
            bool isTriggered = binding.Input && binding.Input->IsTriggeredBy(event);
            if (binding.PressType == pressType && isTriggered) {
                if (action.Callback) {
                    action.Callback();
                    return; 
                }
            }
        }
    }
}

void KeyBindsManager::TriggerAction(int buttonIndex, Input::PressType pressType) {
    for (const auto& [actionKey, action] : m_actions) {
        for (const auto& binding : action.Inputs) {
            Input::JoystickEvent event{buttonIndex, (pressType == Input::PressType::Long), pressType};
            bool isTriggered = binding.Input && binding.Input->IsTriggeredBy(event);
            if (binding.PressType == pressType && isTriggered) {
                if (action.Callback) {
                    action.Callback();
                    return; 
                }
            }
        }
    }
}

bool KeyBindsManager::OnGamepadAxisMove(const GamepadEvent& event) {
  auto logger = LoggerFactory::GetInstance().GetLogger("KeyBindsManager");
  auto& mapper = System::GamepadButtonMapping::GetInstance();
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

bool KeyBindsManager::OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) {
  // This component only cares about the 'keybinds' system.
  // The actual update is handled by the OnKeybindsModified event flow.
  // We just return true here to confirm ownership of the system and prevent
  // the event from being incorrectly forwarded to a plugin.
  return systemName == "keybinds";
}

}  // namespace Modules
SPF_NS_END
