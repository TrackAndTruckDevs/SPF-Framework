#include <SPF/Modules/JoystickInput.hpp>
#include <SPF/System/JoystickButtonMapping.hpp>

SPF_NS_BEGIN
namespace Modules {

JoystickInput::JoystickInput(const nlohmann::json& config) : m_buttonIndex(-1) {
    if (config.contains("key") && config["key"].is_string()) {
        m_buttonIndex = System::JoystickButtonMapping::GetInstance().FromString(config["key"].get<std::string>());
    }
}

bool JoystickInput::IsTriggeredBy(const Input::JoystickEvent& event) const {
    return m_buttonIndex == event.buttonIndex;
}

nlohmann::json JoystickInput::ToJson() const {
    return {
        {"type", "joystick"},
        {"key", System::JoystickButtonMapping::GetInstance().ToString(m_buttonIndex)}
    };
}

std::string JoystickInput::GetDisplayName() const {
    return System::JoystickButtonMapping::GetInstance().GetButtonDisplayName(m_buttonIndex);
}

bool JoystickInput::IsValid() const {
    // Valid if the button index is within the typical DirectInput range.
    return m_buttonIndex >= 0 && m_buttonIndex < 128;
}

bool JoystickInput::IsSameAs(const IBindableInput& other) const {
    if (other.GetType() != InputType::Joystick) {
        return false;
    }

    const auto& otherJoystickInput = static_cast<const JoystickInput&>(other);
    return this->m_buttonIndex == otherJoystickInput.m_buttonIndex;
}

} // namespace Modules
SPF_NS_END