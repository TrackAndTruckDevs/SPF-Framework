#pragma once

#include <SPF/Modules/IBindableInput.hpp>
#include <SPF/System/JoystickButtonMapping.hpp>
#include <nlohmann/json.hpp>
#include <SPF/Input/InputEvents.hpp> // For JoystickEvent

SPF_NS_BEGIN
namespace Modules {

/**
 * @class JoystickInput
 * @brief Represents a generic joystick/wheel/custom gamepad button binding.
 *
 * This class encapsulates a generic button by its index (e.g., button 15)
 * and provides logic to check for event triggers and validate the binding.
 */
class JoystickInput : public IBindableInput {
public:
    /**
     * @brief Constructs a JoystickInput object from a JSON configuration.
     * @param config The JSON object defining the binding (e.g., {"type": "joystick", "key": "BUTTON_15"}).
     */
    explicit JoystickInput(const nlohmann::json& config);

    // --- IBindableInput Overrides ---
    
    bool IsTriggeredBy(const Input::JoystickEvent& event) const override;
    
    nlohmann::json ToJson() const override;
    
    std::string GetDisplayName() const override;

    bool IsValid() const override;
    
    InputType GetType() const override { return InputType::Joystick; }
    
    bool IsSameAs(const IBindableInput& other) const override;

    int GetButtonIndex() const { return m_buttonIndex; }

private:
    int m_buttonIndex = -1;
};

} // namespace Modules
SPF_NS_END
