#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <SPF/Namespace.hpp>

SPF_NS_BEGIN

namespace System {

/**
 * @class JoystickButtonMapping
 * @brief Singleton for mapping generic joystick button indices to/from string representations.
 *
 * This class provides a standardized way to handle joystick button names like "BUTTON_1", "BUTTON_2", etc.
 * It pre-calculates and stores mappings in internal maps for performance and reliability, ensuring
 * that user-facing button numbers are 1-based (e.g., "Button 1") while the underlying system index
 * is 0-based (index 0).
 */
class JoystickButtonMapping {
public:
    /**
     * @brief Deleted copy constructor to enforce singleton pattern.
     */
    JoystickButtonMapping(const JoystickButtonMapping&) = delete;

    /**
     * @brief Deleted copy assignment operator to enforce singleton pattern.
     */
    JoystickButtonMapping& operator=(const JoystickButtonMapping&) = delete;

    /**
     * @brief Returns the single, thread-safe instance of the class.
     */
    static JoystickButtonMapping& GetInstance();

    /**
     * @brief Converts a string name (e.g., "BUTTON_1") to its corresponding 0-based button index.
     * @param name The string representation of the button.
     * @return The 0-based button index, or -1 if the name is not found.
     */
    int FromString(const std::string& name) const;

    /**
     * @brief Converts a 0-based button index to its 1-based string representation (e.g., "BUTTON_1").
     * @param buttonIndex The 0-based index of the button.
     * @return The string name of the button, or an empty string if the index is out of bounds.
     */
    std::string ToString(int buttonIndex) const;

    /**
     * @brief Gets the human-readable, 1-based display name for a joystick button (e.g., "Button 1").
     * @param buttonIndex The 0-based index of the button.
     * @return The display name for the button.
     */
    std::string GetButtonDisplayName(int buttonIndex) const;

private:
    /**
     * @brief Private constructor to enforce singleton pattern. Calls InitializeMapping.
     */
    JoystickButtonMapping();

    /**
     * @brief Populates the internal maps with button names and their corresponding indices.
     */
    void InitializeMapping();

    /// The maximum number of joystick buttons to map (0-127).
    static constexpr int MAX_JOYSTICK_BUTTONS = 128;
    /// The prefix used for string representations of buttons.
    const std::string BUTTON_PREFIX = "BUTTON_";

    /// Map for converting string names to 0-based button indices.
    std::unordered_map<std::string, int> m_stringToButton;
    /// Map for converting 0-based button indices to string names.
    std::unordered_map<int, std::string> m_buttonToString;
};

} // namespace System

SPF_NS_END