#pragma once
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Config/IConfigurable.hpp"
#include "SPF/Config/IConfigService.hpp"
#include "SPF/Config/EnumMappings.hpp"
#include "SPF/Events/PluginEvents.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/System/Keyboard.hpp"
#include "SPF/System/GamepadButton.hpp"
#include "SPF/System/MouseButtonMapping.hpp"
#include <functional>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Input {
class InputManager;
struct KeyboardEvent;
struct GamepadEvent;
struct MouseButtonEvent;
}
namespace Events {
class EventManager;
}
namespace Modules {
class IBindableInput;
}

namespace Modules {
using ActionCallback = std::function<void()>;

enum class ActivationBehavior {
  Toggle,  // Action is triggered once
  Hold     // Action is triggered on press, and again on release
};

struct Binding {
  std::unique_ptr<IBindableInput> Input;
  Config::ConsumptionPolicy Policy = Config::ConsumptionPolicy::Never;
  Input::PressType PressType = Input::PressType::Short;      // Specifies if this binding is for a short or long press
  ActivationBehavior Behavior = ActivationBehavior::Toggle;  // Specifies how the action is triggered over time
  std::optional<std::chrono::milliseconds> PressThreshold;
};

struct Action {
  ActionCallback Callback;
  std::vector<Binding> Inputs;
};

class KeyBindsManager : public Input::IInputConsumer, public Config::IConfigurable {
 public:
  static KeyBindsManager& GetInstance();

  KeyBindsManager(Input::InputManager& inputManager, Events::EventManager& eventManager);
  ~KeyBindsManager();

  Core::InitializationReport Initialize(const nlohmann::json* keyBindsConfig, const std::map<std::string, Config::ComponentInfo>& componentInfo);

  /**
   * @brief Non-destructively updates the key assignments for all actions from a new config.
   * This preserves the registered action callbacks.
   * @param keyBindsConfig The new keybinds configuration object.
   */
  void UpdateKeybindings(const nlohmann::json* keyBindsConfig);

  void RegisterAction(const std::string& actionKey, ActionCallback callback);
  void UnregisterOwner(const std::string& owner);

  /**
   * @brief Checks if a given input is already bound to any active action.
   * @param input The input to check.
   * @return The name of the conflicting action if found, otherwise std::nullopt.
   */
  std::optional<std::string> GetActionBoundToInput(const IBindableInput& input) const;

  const Binding* GetBindingForInput(System::Keyboard key, Input::PressType pressType) const;
  const Binding* GetBindingForInput(System::GamepadButton button, Input::PressType pressType) const;
  const Binding* GetBindingForInput(System::MouseButton button, Input::PressType pressType) const;
  const Binding* GetBindingForInput(int buttonIndex, Input::PressType pressType) const;

  Config::ConsumptionPolicy GetPolicyForEvent(const Input::KeyboardEvent& event, Input::PressType pressType) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::GamepadEvent& event) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::MouseButtonEvent& event) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::JoystickEvent& event) const;

  std::chrono::milliseconds GetLongPressThreshold() const;

  void TriggerAction(System::GamepadButton button, Input::PressType pressType);
  void TriggerAction(System::Keyboard key, Input::PressType pressType);
  void TriggerAction(System::MouseButton button, Input::PressType pressType);
  void TriggerAction(int buttonIndex, Input::PressType pressType);

  // IInputConsumer implementation
  bool OnKeyPress(const Input::KeyboardEvent& event) override;
  bool OnKeyRelease(const Input::KeyboardEvent& event) override;
  bool OnGamepadButtonPress(const Input::GamepadEvent& event) override;
  bool OnGamepadButtonRelease(const Input::GamepadEvent& event) override;
  bool OnGamepadAxisMove(const Input::GamepadEvent& event) override;

  // IConfigurable implementation
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override;

 private:
  void OnPluginLoaded(const Events::OnPluginDidLoad& e);
  void OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e);

  inline static KeyBindsManager* s_instance = nullptr;

  Input::InputManager& m_inputManager;
  Events::EventManager& m_eventManager;
  std::map<std::string, Action> m_actions;                                 // Key: "Owner.Name"
  std::map<std::string, std::map<std::string, Action>> m_inactiveActions;  // Key: Owner, Key: ActionName

  Utils::Sink<void(const Events::OnPluginDidLoad&)> m_onPluginDidLoadSink;
  Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)> m_onPluginWillBeUnloadedSink;
};
}  // namespace Modules
SPF_NS_END