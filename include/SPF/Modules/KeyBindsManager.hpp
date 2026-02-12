#pragma once
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Input/IInputConsumer.hpp"
#include "SPF/Modules/IBindableInput.hpp"
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
#include <mutex>

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
  nlohmann::ordered_json originalBindingJson; // Store the original JSON for better conflict reporting
  bool programmaticallyBlocked = false;       // Used when Policy is set to Manual
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

   Core::InitializationReport Initialize(const nlohmann::ordered_json* keyBindsConfig, const std::map<std::string, Config::ComponentInfo>& componentInfo);

   /**
   * @brief Non-destructively updates the key assignments for all actions from a new config.
   * This preserves the registered action callbacks.
   * @param keyBindsConfig The new keybinds configuration object.
   */
  void UpdateKeybindings(const nlohmann::ordered_json* keyBindsConfig);

  void RegisterAction(const std::string& actionKey, ActionCallback callback);
  void UnregisterOwner(const std::string& owner);

  /**
   * @brief Manually sets the blocking state for an action.
   * This is only effective if the binding's policy is set to 'manual'.
   */
  void SetBlockState(const std::string& actionKey, bool blocked);

  /**
   * @brief Describes the conflicts for short and long press types for a given physical input.
   */
  struct PressTypeConflictAnalysis {
    bool isShortPressAvailable = true;
    bool isLongPressAvailable = true;
    
    // Axis specific analysis
    bool isPositiveAvailable = true;
    bool isNegativeAvailable = true;

    std::optional<std::pair<std::string, nlohmann::ordered_json>> shortPressConflict;
    std::optional<std::pair<std::string, nlohmann::ordered_json>> longPressConflict;

    // Detailed axis conflicts
    std::optional<std::pair<std::string, nlohmann::ordered_json>> positiveConflict;
    std::optional<std::pair<std::string, nlohmann::ordered_json>> negativeConflict;
    std::optional<std::pair<std::string, nlohmann::ordered_json>> bothConflict;
  };

  /**
   * @brief Finds all actions that are bound to a given physical input.
   * @param input The input to check.
   * @return A vector of pairs, where each pair contains the name of a conflicting action and its full binding JSON object.
   */
  std::vector<std::pair<std::string, nlohmann::ordered_json>> GetBindingsForInput(const IBindableInput& input) const;

  /**
   * @brief Analyzes a given physical input and determines the availability of short and long press slots.
   * @param input The input to check.
   * @return A PressTypeConflictAnalysis struct detailing which press types are taken and by which actions.
   */
  PressTypeConflictAnalysis AnalyzeConflictsForInput(const IBindableInput& input) const;

  /**
   * @brief Finds the first action that conflicts with a given input and press type.
   * @param input The physical input to check.
   * @param pressType The press type to check for.
   * @param actionToExclude The full name of the action to exclude from the search (usually the one being edited).
   * @return An optional pair containing the name and binding JSON of the conflicting action, if found.
   */
  std::optional<std::pair<std::string, nlohmann::ordered_json>> FindConflictForBinding(const IBindableInput& input, Input::PressType pressType,
                                                                             const std::string& actionToExclude) const;

  const Binding* GetBindingForInput(System::Keyboard key, Input::PressType pressType) const;
  const Binding* GetBindingForInput(System::GamepadButton button, Input::PressType pressType) const;
  const Binding* GetBindingForInput(System::MouseButton button, Input::PressType pressType) const;
  const Binding* GetBindingForInput(int buttonIndex, Input::PressType pressType) const;

  /**
   * @brief Selects the best active binding for a given trigger and press type, prioritizing chords.
   */
  const Binding* FindBestBinding(uint32_t triggerHardwareCode, Input::PressType pressType) const;

  Config::ConsumptionPolicy GetPolicyForEvent(const Input::KeyboardEvent& event, Input::PressType pressType) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::GamepadEvent& event, Input::PressType pressType) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::MouseButtonEvent& event, Input::PressType pressType) const;
  Config::ConsumptionPolicy GetPolicyForEvent(const Input::JoystickEvent& event, Input::PressType pressType) const;

  /**
   * @brief Gets the effective consumption policy for a specific hardware code.
   * This is useful for axes which need to know their blocking state continuously.
   */
  Config::ConsumptionPolicy GetPolicyForHardwareCode(uint32_t hardwareCode) const;

  /**
   * @brief Gets the user-defined threshold for an analog axis.
   */
  float GetThresholdForHardwareCode(uint32_t hardwareCode) const;

  std::chrono::milliseconds GetLongPressThreshold() const;

  void TriggerAction(System::GamepadButton button, Input::PressType pressType);
  void TriggerAction(System::Keyboard key, Input::PressType pressType);
  void TriggerAction(System::MouseButton button, Input::PressType pressType);
  void TriggerAction(int buttonIndex, Input::PressType pressType);
  void TriggerAction(uint32_t hardwareCode, Input::PressType pressType);

  /**
   * @brief Gets the current normalized value (0.0 to 1.0 or -1.0 to 1.0) for an action.
   * This aggregates all active bindings for the action and returns the one with the highest absolute value.
   */
  float GetActionValue(const std::string& actionName) const;

  /**
   * @brief Gets the number of bindings assigned to an action.
   */
  size_t GetBindingCount(const std::string& actionName) const;

  /**
   * @brief Gets a pointer to a specific binding by index.
   */
  const Binding* GetBinding(const std::string& actionName, size_t index) const;

  // IInputConsumer implementation
  bool OnKeyPress(const Input::KeyboardEvent& event) override;
  bool OnKeyRelease(const Input::KeyboardEvent& event) override;
  bool OnGamepadButtonPress(const Input::GamepadEvent& event) override;
  bool OnGamepadButtonRelease(const Input::GamepadEvent& event) override;
  bool OnGamepadAxisMove(const Input::GamepadEvent& event) override;

  // IConfigurable implementation
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::ordered_json& newValue) override;

 private:
  void ApplyAxisPropertiesToInputManager();
  void OnPluginLoaded(const Events::OnPluginDidLoad& e);
  void OnPluginUnloaded(const Events::OnPluginWillBeUnloaded& e);

  inline static KeyBindsManager* s_instance = nullptr;

  Input::InputManager& m_inputManager;
  Events::EventManager& m_eventManager;
  std::map<std::string, Action> m_actions;                                 // Key: "Owner.Name"
  std::map<std::string, std::map<std::string, Action>> m_inactiveActions;  // Key: Owner, Key: ActionName
  mutable std::recursive_mutex m_actionsMutex;

  Utils::Sink<void(const Events::OnPluginDidLoad&)> m_onPluginDidLoadSink;
  Utils::Sink<void(const Events::OnPluginWillBeUnloaded&)> m_onPluginWillBeUnloadedSink;
};
}  // namespace Modules
SPF_NS_END