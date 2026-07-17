#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/UI/UITypographyHelper.hpp"  // Required for TextStyle

#include "imgui.h"

SPF_NS_BEGIN
namespace UI {
/**
 * @brief Renders a unified framework button.
 *
 * Behavior:
 * - Idle: White (or style.color)
 * - Hover: Gold (or style.hoverColor)
 * - Active: Dark background color (or style.activeColor)
 *
 * @param label Text or icon to display.
 * @param style TextStyle to apply. Defaults to TextStyle::DefaultButton().
 * @param size Optional fixed size. If (0,0), it auto-sizes to label.
 * @param tooltip Optional tooltip text to display on hover.
 * @return True if clicked.
 */
bool Button(const char* label, const TextStyle& style = TextStyle::DefaultButton(), const ImVec2& size = ImVec2(0, 0), const char* tooltip = nullptr);

}  // namespace UI
SPF_NS_END
