#pragma once

#include <string>
#include <unordered_map>

#include <SPF/Namespace.hpp>

SPF_NS_BEGIN

namespace System {

// Represents the different mouse buttons that can be bound.
// The integer values correspond to the offsets from DIMOFS_BUTTON0 in DirectInput.
enum class MouseButton {
    Unknown = -1,
    Left = 0,    // DIMOFS_BUTTON0
    Right = 1,   // DIMOFS_BUTTON1
    Middle = 2,  // DIMOFS_BUTTON2
    X1 = 3,      // "Mouse 4"
    X2 = 4,      // "Mouse 5"
    X3 = 5,      // "Mouse 6"
    X4 = 6,      // "Mouse 7"
    X5 = 7,      // "Mouse 8"
};

/**
 * @class MouseButtonMapping
 * @brief A singleton utility class to map between MouseButton enum values and their string representations.
 *
 * This is used for configuration (keybinds.json) and logging purposes, providing a consistent
 * way to refer to mouse buttons throughout the framework.
 */
class MouseButtonMapping {
public:
    /**
     * @brief Returns the single instance of the class.
     */
    static MouseButtonMapping& GetInstance();

    /**
     * @brief Converts a string name to its corresponding MouseButton enum value.
     * @param name The string representation of the button (e.g., "MOUSE_MIDDLE"). Case-insensitive.
     * @return The MouseButton enum value, or MouseButton::Unknown if not found.
     */
    MouseButton FromString(const std::string& name) const;

    /**
     * @brief Converts a MouseButton enum value to its string representation.
     * @param button The MouseButton enum value.
     * @return The string name of the button (e.g., "MOUSE_MIDDLE"), or "UNKNOWN" if not found.
     */
    std::string ToString(MouseButton button) const;

    /**
     * @brief An alias for ToString, providing a consistent naming convention with other mapping classes.
     */
    std::string GetButtonName(MouseButton button) const { return ToString(button); }

    /**
     * @brief Gets the human-readable display name for a mouse button.
     * @param button The MouseButton enum value.
     * @return The display name (e.g., "Middle Mouse").
     */
    std::string GetButtonDisplayName(MouseButton button) const;

private:
    MouseButtonMapping();
    ~MouseButtonMapping() = default;

    // Delete copy and move operations to enforce singleton pattern
    MouseButtonMapping(const MouseButtonMapping&) = delete;
    MouseButtonMapping& operator=(const MouseButtonMapping&) = delete;
    MouseButtonMapping(MouseButtonMapping&&) = delete;
    MouseButtonMapping& operator=(MouseButtonMapping&&) = delete;

    std::unordered_map<std::string, MouseButton> m_stringToButton;
    std::unordered_map<MouseButton, std::string> m_buttonToString;
    std::unordered_map<MouseButton, std::string> m_buttonToDisplayName;
};

} // namespace System

SPF_NS_END