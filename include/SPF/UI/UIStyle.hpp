#pragma once
#include "SPF/Namespace.hpp"
#include <imgui.h>

SPF_NS_BEGIN
namespace UI
{
    namespace Colors {
        constexpr ImVec4 GOLD         = ImVec4(1.00f, 0.75f, 0.00f, 1.00f);
        constexpr ImVec4 RED          = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        constexpr ImVec4 GREEN        = ImVec4(0.40f, 0.80f, 0.40f, 1.00f);
        constexpr ImVec4 BLUE         = ImVec4(0.010f, 0.40f, 0.90f, 1.00f);
        constexpr ImVec4 URL_LINK     = ImVec4(0.010f, 0.50f, 0.90f, 1.00f);
        constexpr ImVec4 GRAY         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        constexpr ImVec4 LIGHT_GRAY   = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        constexpr ImVec4 YELLOW       = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
        constexpr ImVec4 ORANGE       = ImVec4(1.00f, 0.50f, 0.00f, 1.00f);
        constexpr ImVec4 WHITE        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        constexpr ImVec4 BLACK        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        constexpr ImVec4 CODE_BG      = ImVec4(0.15f, 0.15f, 0.15f, 0.6f);
        constexpr ImVec4 SILVER       = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        constexpr ImVec4 LIME         = ImVec4(0.20f, 0.80f, 0.20f, 1.00f);
        constexpr ImVec4 BRIGHT_RED   = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
        constexpr ImVec4 DARK_RED     = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
        constexpr ImVec4 PURPLE       = ImVec4(0.60f, 0.40f, 1.00f, 1.00f);
        constexpr ImVec4 LIGHT_BLUE   = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
        constexpr ImVec4 SEPIA        = ImVec4(0.62f, 0.39f, 0.20f, 1.00f);
    }

    class Style
    {
    public:
        static void ApplyGameStyle();
    };
}
SPF_NS_END