#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Input/InputEvents.hpp"
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules {
enum class InputType { Keyboard, Gamepad, Mouse, Joystick, Unknown };

/**
 * @brief An interface for a specific input that can be bound to an action.
 *
 * This represents a single, concrete input, like "the 'A' key" or "Gamepad button 'X'".
 * It's used by the KeyBindsManager to check if incoming events match a binding.
 */
struct IBindableInput {
  virtual ~IBindableInput() = default;

  /**
   * @brief Checks if this bindable input is triggered by a specific event.
   * @param event The incoming input event from the InputManager.
   * @return True if the event matches this input, false otherwise.
   */
  virtual bool IsTriggeredBy(const Input::MouseMoveEvent& event) const { return false; }
  virtual bool IsTriggeredBy(const Input::MouseButtonEvent& event) const { return false; }
  virtual bool IsTriggeredBy(const Input::KeyboardEvent& event) const { return false; }
  virtual bool IsTriggeredBy(const Input::GamepadEvent& event) const { return false; }
  virtual bool IsTriggeredBy(const Input::JoystickEvent& event) const { return false; }

  // Overload for raw keyboard key checks
  virtual bool IsTriggeredBy(System::Keyboard key) const { return false; }

  /**
   * @brief Checks if this bindable input is the same as another one.
   * @param other The other IBindableInput to compare against.
   * @return True if they represent the same physical input.
   */
  virtual bool IsSameAs(const IBindableInput& other) const = 0;

  /**
   * @brief Serializes the input to a JSON object for configuration.
   * @return A json object representing the input.
   */
  virtual nlohmann::json ToJson() const = 0;

  /**
   * @brief Gets a user-friendly display name for the input (e.g., "KEY_A", "Gamepad A").
   * @return A string containing the display name.
   */
  virtual std::string GetDisplayName() const = 0;

  /**
   * @brief Checks if the input was successfully configured.
   * @return True if the input is valid, false otherwise.
   */
  virtual bool IsValid() const = 0;

  /**
   * @brief Gets the type of the input (Keyboard, Gamepad, etc.).
   * @return The InputType enum value.
   */
  virtual InputType GetType() const = 0;
};

}  // namespace Modules
SPF_NS_END