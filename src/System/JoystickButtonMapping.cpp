#include <SPF/System/JoystickButtonMapping.hpp>
#include <fmt/core.h>

SPF_NS_BEGIN

namespace System {

JoystickButtonMapping& JoystickButtonMapping::GetInstance() {
    static JoystickButtonMapping instance;
    return instance;
}

JoystickButtonMapping::JoystickButtonMapping() {
    InitializeMapping();
}

void JoystickButtonMapping::InitializeMapping() {
    for (int i = 0; i < MAX_JOYSTICK_BUTTONS; ++i) {
        // User-facing button number is 1-based (1-128)
        int buttonNumber = i + 1;
        std::string name = fmt::format("{}{}", BUTTON_PREFIX, buttonNumber);

        // Store the mapping in both directions
        m_stringToButton[name] = i; // "BUTTON_1" -> 0
        m_buttonToString[i] = name; // 0 -> "BUTTON_1"
    }
}

int JoystickButtonMapping::FromString(const std::string& name) const {
    auto it = m_stringToButton.find(name);
    if (it != m_stringToButton.end()) {
        return it->second;
    }
    return -1; // Return -1 for invalid/unknown button names
}

std::string JoystickButtonMapping::ToString(int buttonIndex) const {
    auto it = m_buttonToString.find(buttonIndex);
    if (it != m_buttonToString.end()) {
        return it->second;
    }
    return ""; // Return empty string for invalid indices
}

std::string JoystickButtonMapping::GetButtonDisplayName(int buttonIndex) const {
    // Display name is always 1-based for the user
    return fmt::format("Button {}", buttonIndex + 1);
}

} // namespace System

SPF_NS_END