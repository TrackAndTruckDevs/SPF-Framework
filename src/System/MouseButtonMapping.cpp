#include <SPF/System/MouseButtonMapping.hpp>
#include <algorithm> // For std::transform

SPF_NS_BEGIN

namespace System {

MouseButtonMapping& MouseButtonMapping::GetInstance() {
    static MouseButtonMapping instance;
    return instance;
}

// Private constructor, called by GetInstance on first use.
MouseButtonMapping::MouseButtonMapping() {
    m_buttonToString = {
        {MouseButton::Left, "MOUSE_LEFT"},
        {MouseButton::Right, "MOUSE_RIGHT"},
        {MouseButton::Middle, "MOUSE_MIDDLE"},
        {MouseButton::X1, "MOUSE_X1"},
        {MouseButton::X2, "MOUSE_X2"},
        {MouseButton::X3, "MOUSE_X3"},
        {MouseButton::X4, "MOUSE_X4"},
        {MouseButton::X5, "MOUSE_X5"},
    };

    // Create the reverse map for case-insensitive lookup
    for (const auto& pair : m_buttonToString) {
        std::string upper_name = pair.second;
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
        m_stringToButton[upper_name] = pair.first;
    }

    m_buttonToDisplayName = {
        {MouseButton::Left, "Left Mouse"},
        {MouseButton::Right, "Right Mouse"},
        {MouseButton::Middle, "Middle Mouse"},
        {MouseButton::X1, "Mouse X1"},
        {MouseButton::X2, "Mouse X2"},
        {MouseButton::X3, "Mouse X3"},
        {MouseButton::X4, "Mouse X4"},
        {MouseButton::X5, "Mouse X5"},
    };
}

MouseButton MouseButtonMapping::FromString(const std::string& name) const {
    std::string upper_name = name;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);

    auto it = m_stringToButton.find(upper_name);
    if (it != m_stringToButton.end()) {
        return it->second;
    }
    return MouseButton::Unknown;
}

std::string MouseButtonMapping::ToString(MouseButton button) const {
    auto it = m_buttonToString.find(button);
    if (it != m_buttonToString.end()) {
        return it->second;
    }
    return "UNKNOWN";
}

std::string MouseButtonMapping::GetButtonDisplayName(MouseButton button) const {
    auto it = m_buttonToDisplayName.find(button);
    if (it != m_buttonToDisplayName.end()) {
        return it->second;
    }
    return "Unknown";
}

} // namespace System

SPF_NS_END