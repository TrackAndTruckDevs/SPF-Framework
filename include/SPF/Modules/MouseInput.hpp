#pragma once

#include <SPF/Modules/IBindableInput.hpp>
#include <SPF/System/MouseButtonMapping.hpp>
#include <SPF/Input/InputEvents.hpp>
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules {

/**
 * @class MouseInput
 * @brief Represents a mouse button binding, implementing the IBindableInput interface.
 *
 * This class encapsulates a specific mouse button and press type (e.g., short press on Middle Mouse)
 * and provides logic to check for event triggers and validate the binding.
 */
class MouseInput : public IBindableInput {
public:
    /**
     * @brief Constructs a MouseInput object from a JSON configuration.
     * @param config The JSON object defining the binding (e.g., {"type": "mouse", "button": "MOUSE_MIDDLE"}).
     */
    explicit MouseInput(const nlohmann::json& config);

    // --- IBindableInput Overrides ---
    
    bool IsTriggeredBy(const Input::MouseButtonEvent& event) const override;
    
    nlohmann::json ToJson() const override;
    
    std::string GetDisplayName() const override;

    /**
     * @brief Validates the binding. A binding is invalid if it uses an unknown button or the reserved Left Mouse Button.
     */
    bool IsValid() const override;
    
    InputType GetType() const override { return InputType::Mouse; }
    
    bool IsSameAs(const IBindableInput& other) const override;

private:
    System::MouseButton m_button;
    // Note: pressType is handled by InputManager's state machine, not stored here,
    // similar to KeyboardInput. It is part of the binding configuration, not the input itself.
};

} // namespace Modules
SPF_NS_END
