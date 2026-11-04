#pragma once

#include "imgui.h"

namespace SPF::UI {
namespace Colors {
constexpr ImVec4 GOLD = ImVec4(1.00f, 0.75f, 0.00f, 1.00f);
constexpr ImVec4 RED = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
constexpr ImVec4 GREEN = ImVec4(0.40f, 0.80f, 0.40f, 1.00f);
constexpr ImVec4 VERY_LIGHT_GREEN = ImVec4(0.5f, 1.0f, 0.75f, 1.00f);
constexpr ImVec4 BLUE = ImVec4(0.010f, 0.40f, 0.90f, 1.00f);
constexpr ImVec4 LIGHT_BLUE = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
constexpr ImVec4 VERY_LIGHT_BLUE = ImVec4(0.5f, 0.75f, 1.0f, 1.00f);
constexpr ImVec4 URL_LINK = ImVec4(0.010f, 0.50f, 0.90f, 1.00f);
constexpr ImVec4 GRAY = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
constexpr ImVec4 LIGHT_GRAY = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
constexpr ImVec4 YELLOW = ImVec4(1.00f, 1.00f, 0.00f, 1.00f);
constexpr ImVec4 ORANGE = ImVec4(1.00f, 0.50f, 0.00f, 1.00f);
constexpr ImVec4 WHITE = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
constexpr ImVec4 BLACK = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
constexpr ImVec4 CODE_BG = ImVec4(0.15f, 0.15f, 0.15f, 0.6f);
constexpr ImVec4 SILVER = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
constexpr ImVec4 LIME = ImVec4(0.20f, 0.80f, 0.20f, 1.00f);
constexpr ImVec4 BRIGHT_RED = ImVec4(1.00f, 0.20f, 0.20f, 1.00f);
constexpr ImVec4 DARK_RED = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
constexpr ImVec4 PURPLE = ImVec4(0.60f, 0.40f, 1.00f, 1.00f);
constexpr ImVec4 SEPIA = ImVec4(0.62f, 0.39f, 0.20f, 1.00f);
constexpr ImVec4 CYAN = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
constexpr ImVec4 MAGENTA = ImVec4(1.00f, 0.00f, 1.00f, 1.00f);
}  // namespace Colors

class Style {
 public:
  /**
   * @brief Rebuilds the framework style from scratch at the given scale.
   * @details Always starts from a fresh ImGuiStyle, so applying a new scale never compounds with the previous one.
   *          Fonts follow the scale through ImGuiStyle::FontScaleMain (crisp at any size since ImGui 1.92).
   */
  static void ApplyGameStyle(float scaleFactor = 1.0f);

  /** @brief Scale currently applied to the whole UI (resolution scale x user setting). */
  static float GetScale();

  /**
   * @brief Scale derived from the screen resolution, 1080p being the reference.
   * @details 1080p (and 2560x1080 ultrawide) -> 1.0, 1440p -> 1.33, 4K -> 2.0. Never below 1.0, so small screens keep
   *          the sizes they always had.
   */
  static float ComputeResolutionScale(float displayHeight);
};

/** @brief Converts a size designed for 1080p into the current UI scale. Use it for every hard-coded pixel size. */
inline float Scaled(float px) { return px * Style::GetScale(); }
inline ImVec2 Scaled(const ImVec2& px) { return ImVec2(px.x * Style::GetScale(), px.y * Style::GetScale()); }
}  // namespace SPF::UI
