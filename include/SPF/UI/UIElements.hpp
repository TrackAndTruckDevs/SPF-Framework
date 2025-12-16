#pragma once

#include "SPF/Namespace.hpp"
#include <imgui.h>

#include "SPF/UI/UITypographyHelper.hpp" // Required for TextStyle

SPF_NS_BEGIN
namespace UI
{
    /**
     * @brief Renders a button that mimics a hyperlink, changing its text color on hover.
     * 
     * This function manually constructs a button to allow for separate control over the label's color
     * when hovered, which is not possible with the standard ImGui::Button.
     * The button's background behavior (normal, hovered, active) respects the current ImGui style.
     * 
     * @param label The text or icon to display on the button.
     * @param style The TextStyle to apply to the label. It defines the default style and can optionally contain a `hoverColor`.
     * @return True if the button was clicked, false otherwise.
     */
    bool HyperlinkButton(const char* label, const TextStyle& style);

} // namespace UI
SPF_NS_END
